// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2023 haili@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <dt-bindings/clock/sun251iw1-cpupll-ccu.h>

#include "ccu_common.h"
#include "ccu_reset.h"
#include "ccu_nm.h"
#include "ccu_nkmp.h"

#define SUNXI_CPUPLL_CCU_VERSION	"0.0.1"

#define SUN251IW1_PLL_CPU_REG		0x0000
#define SUN251IW1_PLL_CPU_SSC_REG	0x0200

static struct ccu_nkmp pll_cpu_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 0, 20, 84),
	.p		= _SUNXI_CCU_DIV(0, 5), /* P in cpu_clk reg */
	.p_reg		= 0x0d00,
	.common		= {
		.reg		= 0x0000,
		.ssc_reg	= 0x0200,
		.clear		= BIT(26),
		.features	= CCU_FEATURE_CLEAR_MOD		\
				| CCU_FEATURE_CLAC_CACHED	\
				| CCU_FEATURE_TYPE_NKMP,
		.hw.init	= CLK_HW_INIT("pll-cpu", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL \
				| CLK_SET_RATE_UNGATE),
	},
};

#define SUN251IW1_CPU_REG       0x0d00

static const char * const riscv_parents[] = { "dcxo24M", "rtc32k", "rc-16m",
	"pll-peri-800m", "pll-peri-1x", "pll-cpu", "pll-audio1-div2" };

static SUNXI_CCU_MUX(riscv_clk, "riscv", riscv_parents,
		     0x0d00, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static struct clk_hw_onecell_data sunxi_cpupll_hw_clks = {
	.hws	= {
		[CLK_PLL_CPU]	= &pll_cpu_clk.common.hw,
		[CLK_RISCV]	= &riscv_clk.common.hw,
	},
	.num = CLK_CPUPLL_MAX_NO,
};

static struct ccu_common *sunxi_pll_cpu_clks[] = {
	&pll_cpu_clk.common,
	&riscv_clk.common,
};

static const struct sunxi_ccu_desc cpupll_desc = {
	.ccu_clks	= sunxi_pll_cpu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sunxi_pll_cpu_clks),
	.hw_clks	= &sunxi_cpupll_hw_clks,
	.resets		= NULL,
	.num_resets	= 0,
};

static const u32 sun251iw1_pll_cpu_regs[] = {
	SUN251IW1_PLL_CPU_REG,
};

static const u32 sun251iw1_pll_cpu_ssc_regs[] = {
	SUN251IW1_PLL_CPU_SSC_REG,
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

static int cpupll_notifier_cb(struct notifier_block *nb, unsigned long event, void *data)
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

static struct ccu_pll_nb cpupll_nb = {
	.common = &pll_cpu_clk.common,
	.enable = 31, /* switch ssc mode */
	.clk_nb = {
		.notifier_call = cpupll_notifier_cb,
	},
};

void __iomem *reg_pll_cpu;
void __iomem *reg_pll_cpu_ssc;
void __iomem *reg_riscv;
static int sun251iw1_cpupll_probe(struct platform_device *pdev)
{
	int i;
	u32 val;
	struct resource *res;
	unsigned int step = 0, ssc = 0;
	struct device_node *np = pdev->dev.of_node;

	/*
	 * Don't use devm_ioremap_resource() here! Or else the RTC driver will
	 * not able to get the same resource later in rtc-sunxi.c.
	 */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pll_cpu");
	if (!res) {
		sunxi_err(&pdev->dev, "find pll_cpu resource\n");
		return -ENOMEM;
	}
	reg_pll_cpu = ioremap(res->start, resource_size(res));
	if (!reg_pll_cpu) {
		sunxi_err(&pdev->dev, "reg_pll_cpu ioremap\n");
		return -ENOMEM;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pll_cpu_ssc");
	if (!res) {
		sunxi_err(&pdev->dev, "find pll_cpu_ssc resource\n");
		return -ENOMEM;
	}
	reg_pll_cpu_ssc = ioremap(res->start, resource_size(res));
	if (!reg_pll_cpu_ssc) {
		sunxi_err(&pdev->dev, "reg_pll_cpu_ssc ioremap\n");
		return -ENOMEM;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "riscv");
	if (!res) {
		sunxi_err(&pdev->dev, "find riscv resource\n");
		return -ENOMEM;
	}
	reg_riscv = ioremap(res->start, resource_size(res));
	if (!reg_riscv) {
		sunxi_err(&pdev->dev, "reg_riscv ioremap\n");
		return -ENOMEM;
	}

	if (of_property_read_u32(np, "pll_step", &step))
		step = 0x8;

	if (of_property_read_u32(np, "pll_ssc", &ssc))
		ssc = 0x3300;

	/* TODO: assume boot use the cpupll */
	for (i = 0; i < ARRAY_SIZE(sun251iw1_pll_cpu_ssc_regs); i++) {
		/*
		 * 1. Config n,m1,m0,p: default:480M
		 * 2. Enable pll_en pll_ldo_en lock_en pll_output
		 * 3. wait for update and lock
		 */
		val = readl(reg_pll_cpu);
		val |= BIT(27) | BIT(29) | BIT(30) | BIT(31);
		set_reg(reg_pll_cpu, val, 32, 0);

		cpupll_helper_wait_for_clear(reg_pll_cpu, BIT(26));
		ccupll_helper_wait_for_lock(reg_pll_cpu, BIT(28));

		/*
		 * set ssc/step in ssc reg
		 */
		val = 0;
		val = (ssc << 12 | step << 0);
		set_reg(reg_pll_cpu_ssc, val, 29, 0);

		cpupll_helper_wait_for_clear(reg_pll_cpu, BIT(26));
	}

	/* set default pll to cpu */
	set_reg(reg_riscv, 0x5, 3, 24);

	sunxi_ccu_probe(pdev->dev.of_node, reg_pll_cpu, &cpupll_desc);

	ccu_pll_notifier_register(&cpupll_nb);

	sunxi_info(NULL, "sunxi cpupll driver version: %s\n", SUNXI_CPUPLL_CCU_VERSION);

	return 0;
}

static const struct of_device_id sun251iw1_cpupll_ids[] = {
	{ .compatible = "allwinner,sun251iw1-cpupll" },
	{ }
};

static struct platform_driver sun251iw1_cpupll_driver = {
	.probe	= sun251iw1_cpupll_probe,
	.driver	= {
		.name	= "sun251iw1-cpupll",
		.of_match_table	= sun251iw1_cpupll_ids,
	},
};

static int __init sunxi_ccu_cpupll_init(void)
{
	int ret;

	ret = platform_driver_register(&sun251iw1_cpupll_driver);
	if (ret)
		sunxi_err(NULL, "register ccu sun251iw1 cpupll failed\n");

	return ret;
}
core_initcall(sunxi_ccu_cpupll_init);

static void __exit sunxi_ccu_cpupll_exit(void)
{
	iounmap(reg_pll_cpu);
	iounmap(reg_pll_cpu_ssc);
	iounmap(reg_riscv);

	return platform_driver_unregister(&sun251iw1_cpupll_driver);
}
module_exit(sunxi_ccu_cpupll_exit);

MODULE_DESCRIPTION("Allwinner sun251iw1 cpupll clk driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_CPUPLL_CCU_VERSION);
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
