// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <dt-bindings/clock/sun55iw6-cpupll-ccu.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_nm.h"
#include "ccu_nkmp.h"

#define SUNXI_CPUPLL_CCU_VERSION		"0.0.2"

#define SUN55IW6_PLL_CPU_L0_REG	0x0004
#define SUN55IW6_PLL_DSU_CLK	0x0008

#define SUN55IW6_PLL_CPU_L0_SSC_REG   0x003c
#define SUN55IW6_PLL_DSU_SSC_REG   0x0040

static struct ccu_nkmp pll_cpu_l0_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 20, 85),
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in cpu_clk reg */
	.p_reg		= 0x0044,
	.common		= {
		.reg		= 0x0004,
		.ssc_reg	= 0x3c,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu-l0", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

static struct ccu_nkmp pll_dsu_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 20, 85),
	.p		= _SUNXI_CCU_DIV(16, 2), /* P in dsu_clk reg */
	.p_reg		= 0x004c,
	.common		= {
		.reg		= 0x0008,
		.ssc_reg	= 0x40,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD | CCU_FEATURE_CLAC_CACHED | CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-dsu", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

#define SUN55IW6_CPU_REG	0x0044
#define SUN55IW6_DSU_REG	0x004c

static const char * const cpu_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-cpu-l0", "pll-peri0-2x", "pll-peri0-600m", "pll-peri1-2x" };

static SUNXI_CCU_MUX(cpu_clk, "cpu", cpu_parents,
		     0x0044, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const dsu_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-cpu-l0", "pll-peri0-2x", "pll-peri0-600m", "pll-peri1-2x" };

static SUNXI_CCU_MUX(dsu_clk, "dsu", dsu_parents,
		     0x004c, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static struct ccu_common *sunxi_pll_cpu_clks[] = {
	&pll_cpu_l0_clk.common,
	&pll_dsu_clk.common,
	&cpu_clk.common,
	&dsu_clk.common,
};

static struct clk_hw_onecell_data sunxi_cpupll_hw_clks = {
	.hws	= {
		[CLK_PLL_CPU_L0]	= &pll_cpu_l0_clk.common.hw,
		[CLK_PLL_DSU]		= &pll_dsu_clk.common.hw,
		[CLK_CPU]		= &cpu_clk.common.hw,
		[CLK_DSU]		= &dsu_clk.common.hw,
	},
	.num = CLK_CPUPLL_MAX_NO,
};

static const struct sunxi_ccu_desc cpupll_desc = {
	.ccu_clks	= sunxi_pll_cpu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sunxi_pll_cpu_clks),
	.hw_clks	= &sunxi_cpupll_hw_clks,
	.resets		= NULL,
	.num_resets	= 0,
};

static const u32 sun55iw6_pll_cpu_regs[] = {
	SUN55IW6_PLL_CPU_L0_REG,
	SUN55IW6_PLL_DSU_CLK,
};

static const u32 sun55iw6_pll_cpu_ssc_regs[] = {
	SUN55IW6_PLL_CPU_L0_SSC_REG,
	SUN55IW6_PLL_DSU_SSC_REG,
};

static void ccupll_helper_wait_for_lock(void __iomem *addr, u32 lock)
{
	u32 reg;

	WARN_ON(readl_relaxed_poll_timeout(addr, reg, reg & lock, 100, 70000));
}

static void cpupll_helper_wait_for_clear(void __iomem *addr, u32 clear)
{
	u32 reg;

	reg = readl(addr);
	writel(reg | clear, addr);

	WARN_ON(readl_relaxed_poll_timeout_atomic(addr, reg, !(reg & clear), 100, 10000));
}

static int cpupll_notifier_cb(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct ccu_pll_nb *pll = to_ccu_pll_nb(nb);
	int ret = 0;

	if (event == PRE_RATE_CHANGE) {
		/* Enable ssc function */
		set_reg(pll->common->base + pll->common->ssc_reg, 1, 1, pll->enable);
	} else if (event == POST_RATE_CHANGE) {
		/* Disable ssc function */
		set_reg(pll->common->base + pll->common->ssc_reg, 0, 1, pll->enable);
		ccu_helper_wait_for_clear(pll->common, pll->common->clear);
	}

	return notifier_from_errno(ret);
}

static struct ccu_pll_nb cpupll_l0_nb = {
	.common = &pll_cpu_l0_clk.common,
	.enable = 31, /* switch ssc mode */
	.clk_nb = {
		.notifier_call = cpupll_notifier_cb,
	},
};

static struct ccu_pll_nb cpudsu_nb = {
	.common = &pll_dsu_clk.common,
	.enable = 31,
	.clk_nb = {
		.notifier_call = cpupll_notifier_cb,
	},
};

static int sun55iw6_cpupll_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 val;
	int i;
	unsigned int step = 0, ssc = 0;
	struct device_node *np = pdev->dev.of_node;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	if (of_property_read_u32(np, "pll_step", &step))
		step = 0x8;

	if (of_property_read_u32(np, "pll_ssc", &ssc))
		ssc = 0x3300;
	/* TODO: assume boot use the cpupll */
	for (i = 0; i < ARRAY_SIZE(sun55iw6_pll_cpu_ssc_regs); i++) {
		/*
		 * 1. Config n,m1,m0,p: default:480M
		 * 2. Enable pll_en pll_ldo_en lock_en pll_output
		 * 3. wait for update and lock
		 */
		val = readl(reg + sun55iw6_pll_cpu_regs[i]);
		val |= BIT(27) | BIT(29) | BIT(30) | BIT(31);
		set_reg(reg + sun55iw6_pll_cpu_regs[i], val, 32, 0);

		cpupll_helper_wait_for_clear(reg + sun55iw6_pll_cpu_regs[i], BIT(26));
		ccupll_helper_wait_for_lock(reg + sun55iw6_pll_cpu_regs[i], BIT(28));

		/*
		 * set ssc/step in ssc reg
		 */
		val = 0;
		val = (ssc << 12 | step << 0);
		set_reg(reg + sun55iw6_pll_cpu_ssc_regs[i], val, 29, 0);

		cpupll_helper_wait_for_clear(reg + sun55iw6_pll_cpu_regs[i], BIT(26));
	}

	/* set default pll to cpu */
	set_reg(reg + SUN55IW6_CPU_REG, 0x3, 3, 24);
	set_reg(reg + SUN55IW6_DSU_REG, 0x3, 3, 24);

	sunxi_ccu_probe(pdev->dev.of_node, reg, &cpupll_desc);

	ccu_pll_notifier_register(&cpupll_l0_nb);
	ccu_pll_notifier_register(&cpudsu_nb);

	sunxi_info(NULL, "sunxi pll_cpu driver version: %s\n", SUNXI_CPUPLL_CCU_VERSION);

	return 0;
}

static const struct of_device_id sun55iw6_cpupll_ids[] = {
	{ .compatible = "allwinner,sun55iw6-cpupll" },
	{ }
};

static struct platform_driver sun55iw6_cpupll_driver = {
	.probe	= sun55iw6_cpupll_probe,
	.driver	= {
		.name	= "sun55iw6-cpupll",
		.of_match_table	= sun55iw6_cpupll_ids,
	},
};

static int __init sunxi_ccu_cpupll_init(void)
{
	int ret;

	ret = platform_driver_register(&sun55iw6_cpupll_driver);
	if (ret)
		sunxi_err(NULL, "register ccu sun55iw6 cpupll failed\n");

	return ret;
}
core_initcall(sunxi_ccu_cpupll_init);

static void __exit sunxi_ccu_cpupll_exit(void)
{
	return platform_driver_unregister(&sun55iw6_cpupll_driver);
}
module_exit(sunxi_ccu_cpupll_exit);

MODULE_DESCRIPTION("Allwinner sun55iw6 cpupll clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_CPUPLL_CCU_VERSION);
MODULE_AUTHOR("emma<liujuan1@allwinnertech.com>");
