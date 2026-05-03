// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_nm.h"

#include "ccu-sun252iw1-r.h"

/* ccu_des_start */
static const char * const r_ahb_parents[] = { "dcxo24M", "rtc32k", "rc-16m", "pll-peri-200m" };
SUNXI_CCU_M_WITH_MUX(r_ahb_clk, "r-ahb", r_ahb_parents,
		0x0000, 0, 5, 24, 3, CLK_IGNORE_UNUSED);

static const char * const r_apbs_parents[] = { "dcxo24M", "rtc32k", "rc-16m", "pll-peri-200m" };
SUNXI_CCU_M_WITH_MUX(r_apbs0_clk, "r-apbs0", r_apbs_parents,
		0x000c, 0, 5, 24, 3, CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu",
						"dcxo24M",
						0x01AC, BIT(0), 0);

static SUNXI_CCU_GATE(r_rtc_clk, "r-rtc",
						"dcxo24M",
						0x020C, BIT(0), 0);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun252iw1_r_ccu_resets[] = {
	[RST_BUS_R_PPU]		= { 0x01ac, BIT(16) },
	[RST_BUS_R_RTC]		= { 0x020c, BIT(16) },
	[RST_BUS_R_CPUCFG]		= { 0x022c, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun252iw1_r_hw_clks = {
		.hws    = {
		[CLK_R_AHB]			= &r_ahb_clk.common.hw,
		[CLK_R_APBS0]			= &r_apbs0_clk.common.hw,
		[CLK_R_PPU]			= &r_ppu_clk.common.hw,
		[CLK_R_RTC]			= &r_rtc_clk.common.hw,
		},
		.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun252iw1_r_ccu_clks[] = {
	&r_ahb_clk.common,
	&r_apbs0_clk.common,
	&r_ppu_clk.common,
	&r_rtc_clk.common,
};

static const struct sunxi_ccu_desc sun252iw1_r_ccu_desc = {
	.ccu_clks	= sun252iw1_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun252iw1_r_ccu_clks),

	.hw_clks	= &sun252iw1_r_hw_clks,

	.resets		= sun252iw1_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun252iw1_r_ccu_resets),
};

static int sun252iw1_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun252iw1_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun252iw1_r_ccu_clks,
			     ARRAY_SIZE(sun252iw1_r_ccu_clks),
			     NULL, 0);

	return 0;
}

static const struct of_device_id sun252iw1_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun252iw1-r-ccu" },
	{ }
};

static struct platform_driver sun252iw1_r_ccu_driver = {
	.probe	= sun252iw1_r_ccu_probe,
	.driver	= {
		.name	= "sun252iw1-r-ccu",
		.of_match_table	= sun252iw1_r_ccu_ids,
	},
};

static int __init sunxi_r_ccu_sun252iw1_init(void)
{
	int ret;

	ret = platform_driver_register(&sun252iw1_r_ccu_driver);
	if (ret)
		pr_err("register ccu sun252iw1 failed\n");

	return ret;
}
core_initcall(sunxi_r_ccu_sun252iw1_init);

static void __exit sunxi_r_ccu_sun252iw1_exit(void)
{
	return platform_driver_unregister(&sun252iw1_r_ccu_driver);
}
module_exit(sunxi_r_ccu_sun252iw1_exit);

MODULE_VERSION("0.0.2");
MODULE_AUTHOR("weizhouxiang<weizhouxiang@allwinnertech.com>");

