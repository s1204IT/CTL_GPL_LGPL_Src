/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner DDR Clock driver.
 *
 * Copyright (C) 2020 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Implementation of ddr clock source driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <sunxi-sip.h>

#define DRIVER_NAME	"DDR-Clock-Driver"

struct sunxi_ddrclk_plat_data {
	unsigned int dram_clk_ctrl;
	/* the freq factor about ddrc and memory die, 1:2 or 1:4*/
	unsigned int factor;
};

/*
 * PLL_DDR / (DIV + 1) = DDR_CLK;
 * DDR_PHY_CLK = DDR_CLK * 2;
 * DDR_PHY_IO_CLK = DDR_CLK * 4;
 * So (DIV + 1) = PLL_DDR / (DDR_PHY_CLK / 2);
 * So (DIV + 1) = (PLL_DDR / DDR_PHY_CLK) * 2;
 * The freq in opp table is DDR_PHY_CLK.
 */

struct sunxi_ddrclk {
	struct device *dev;
	void __iomem	*dramphy_base;
	unsigned int	dram_clk;
	unsigned int	dram_div;
	unsigned long	freq[4];
	struct clk_hw	hw;
	struct mutex  ddrfreq_lock;
	struct clk *ddrpll0_clk;
	struct clk *ddrpll1_clk;
	struct clk *ddrpll2_clk;
	unsigned long ddrpll[3];
	const struct sunxi_ddrclk_plat_data *plat_data;
	spinlock_t      lock;
};

#define to_sunxi_ddrclk_hw(_hw) container_of(_hw, struct sunxi_ddrclk, hw)

static int dbg_enable;
module_param_named(dbg_level, dbg_enable, int, 0644);

#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			sunxi_info(NULL, args); \
		} \
	} while (0)

static inline int set_ddrfreq(struct sunxi_ddrclk *ddrclk, unsigned int freq_id)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_SUNXI_DDRFREQ, freq_id, 0, 0);

	return result;
}

static unsigned long sunxi_ddr_clk_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	const struct sunxi_ddrclk_plat_data *plat_data = ddrclk->plat_data;
	unsigned long rate;
	unsigned int reg_val;

	reg_val = readl_relaxed(ddrclk->dramphy_base + plat_data->dram_clk_ctrl);
	rate = (ddrclk->ddrpll[(reg_val >> 8) & 0x3] >> 1) / ((reg_val & 0x7) + 1);

	return rate;
}

static long sunxi_ddr_clk_round_rate(struct clk_hw *hw,
				     unsigned long target_rate,
				     unsigned long *prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);

	if (target_rate <= ddrclk->freq[0])
		return ddrclk->freq[0];
	else if (target_rate <= ddrclk->freq[1])
		return ddrclk->freq[1];
	else if (target_rate <= ddrclk->freq[2])
		return ddrclk->freq[2];
	else
		return ddrclk->freq[3];
}

static int sunxi_ddr_clk_set_rate(struct clk_hw *hw, unsigned long drate,
				  unsigned long prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	unsigned int freq_id;

	if (drate <= ddrclk->freq[0])
		freq_id = 3;
	else if (drate <= ddrclk->freq[1])
		freq_id = 2;
	else if (drate <= ddrclk->freq[2])
		freq_id = 1;
	else
		freq_id = 0;

	DBG("drate:%ldM\n", drate / 1000000);
	mutex_lock(&ddrclk->ddrfreq_lock);
	set_ddrfreq(ddrclk, freq_id);
	ddrclk->dram_clk = drate;
	mutex_unlock(&ddrclk->ddrfreq_lock);

	return 0;
}

const struct clk_ops sunxi_ddrclk_ops = {
	.recalc_rate = sunxi_ddr_clk_recalc_rate,
	.round_rate = sunxi_ddr_clk_round_rate,
	.set_rate = sunxi_ddr_clk_set_rate,
};

static const struct sunxi_ddrclk_plat_data ddrclk_sun65iw1_data = {
	.dram_clk_ctrl = 0x1c,
	.factor = 1,
};

static const struct of_device_id clk_ddr_of_match[] = {
	{ .compatible = "allwinner,sun65iw1_clock_ddr", .data = &ddrclk_sun65iw1_data},
	{ },
};
MODULE_DEVICE_TABLE(of, clk_ddr_of_match);

static int ddr_clock_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dram_np;
	struct sunxi_ddrclk *ddrclk;
	struct clk_init_data init;
	struct clk *clk, *parent_clk;
	const char *parent_name;
	unsigned int sdiv;
	int ret = 0;

	if (!np) {
		sunxi_err(&pdev->dev, "failed to match ddr clock\n");
		return -ENODEV;
	}

	ddrclk = devm_kzalloc(&pdev->dev, sizeof(*ddrclk), GFP_KERNEL);
	if (!ddrclk)
		return -ENOMEM;

	ddrclk->dev = &pdev->dev;
	platform_set_drvdata(pdev, ddrclk);

	mutex_init(&ddrclk->ddrfreq_lock);
	dram_np = of_find_node_by_path("/dram");
	if (!dram_np) {
		sunxi_err(&pdev->dev, "failed to find dram node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dram_np, "dram_para[00]", &ddrclk->dram_clk);
	if (ret) {
		ret = of_property_read_u32(dram_np, "dram_para00", &ddrclk->dram_clk);
		if (ret) {
			sunxi_err(&pdev->dev, "failed to find dram_clk\n");
			return -ENODEV;
		}
	}

	sunxi_err(NULL, "dram_clk:%d\n", ddrclk->dram_clk);

	ret = of_property_read_u32(dram_np, "dram_para[24]", &ddrclk->dram_div);
	if (ret) {
		ret = of_property_read_u32(dram_np, "dram_para24", &ddrclk->dram_div);
		if (ret) {
			sunxi_err(&pdev->dev, "failed to find dram_div\n");
			return -ENODEV;
		}
	}
	sunxi_err(NULL, "dram_div:0x%x\n", ddrclk->dram_div);

	ddrclk->dramphy_base = of_iomap(np, 0);
	if (!ddrclk->dramphy_base) {
		sunxi_err(&pdev->dev, "map dramphy failed\n");
		return -ENODEV;
	}

	ddrclk->ddrpll0_clk = devm_clk_get(dev, "ddrpll0");
	if (IS_ERR(ddrclk->ddrpll0_clk)) {
		sunxi_err(&pdev->dev, "ddrpll0 get error!\n");
		return PTR_ERR(ddrclk->ddrpll0_clk);
	}
	ddrclk->ddrpll[0] = clk_get_rate(ddrclk->ddrpll0_clk);

	ddrclk->ddrpll1_clk = devm_clk_get(dev, "ddrpll1");
	if (IS_ERR(ddrclk->ddrpll1_clk)) {
		sunxi_err(&pdev->dev, "ddrpll1 get error!\n");
		return PTR_ERR(ddrclk->ddrpll1_clk);
	}
	ddrclk->ddrpll[1] = clk_get_rate(ddrclk->ddrpll1_clk);

	ddrclk->ddrpll2_clk = devm_clk_get(dev, "ddrpll2");
	if (IS_ERR(ddrclk->ddrpll2_clk)) {
		sunxi_err(&pdev->dev, "ddrpll2 get error!\n");
		return PTR_ERR(ddrclk->ddrpll2_clk);
	}
	ddrclk->ddrpll[2] = clk_get_rate(ddrclk->ddrpll2_clk);

	sdiv = ddrclk->dram_div >> 24;
	ddrclk->freq[0] = (ddrclk->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);
	sdiv = ddrclk->dram_div >> 16;
	ddrclk->freq[1] = (ddrclk->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);
	sdiv = ddrclk->dram_div >> 8;
	ddrclk->freq[2] = (ddrclk->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);
	sdiv = ddrclk->dram_div >> 0;
	ddrclk->freq[3] = (ddrclk->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);

	parent_clk = devm_clk_get(&pdev->dev, "dfi_clk");
	if (IS_ERR(parent_clk)) {
		sunxi_err(&pdev->dev, "clk_get dfi_clk failed\n");
		ret = -ENODEV;
		goto out;
	}

	parent_name = __clk_get_name(parent_clk);
	if (!parent_name) {
		sunxi_err(&pdev->dev, "get clk name failed\n");
		ret = -ENODEV;
		goto out;
	}

	ddrclk->hw.init = &init;
	init.name = "sdram";
	init.ops = &sunxi_ddrclk_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags |= CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE;

	ddrclk->plat_data = of_device_get_match_data(dev);
	clk = devm_clk_register(&pdev->dev, &ddrclk->hw);
	if (IS_ERR(clk)) {
		sunxi_err(&pdev->dev, "clk_register failed\n");
		ret = -ENODEV;
		goto out;
	}

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return 0;

out:
	return ret;
}

static int ddr_clock_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	of_clk_del_provider(np);
	return 0;
}

static struct platform_driver ddr_clock_driver = {
	.probe   = ddr_clock_probe,
	.remove  = ddr_clock_remove,
	.driver  = {
		.name  = "sunxi-ddrclock",
		.of_match_table = clk_ddr_of_match,
	},
};

module_platform_driver(ddr_clock_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner DDR Clock driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.1");
