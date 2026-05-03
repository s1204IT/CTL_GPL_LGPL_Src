// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 huangzhenwei@allwinnertech.com
 */

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

#include "ccu-sun251iw1-r.h"

#define SUNXI_R_CCU_VERSION	"0.0.2"
static const char * const ahbs_apbs0_parents[] = { "dcxo24M", "rtc32k",
						   "rc-16m", "pll-peri" };
static SUNXI_CCU_MP_WITH_MUX(r_ahb_clk, "r-ahb",
			     ahbs_apbs0_parents, 0x000,
			     0, 5,
			     8, 2,
			     24, 3,
			     CLK_IGNORE_UNUSED);

static SUNXI_CCU_MP_WITH_MUX(r_apb0_clk, "r-apb0",
			     ahbs_apbs0_parents, 0x00c,
			     0, 5,
			     8, 2,
			     24, 3,
			     CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_apb0_timer_clk, "r-apb0-timer", "r-apb0",
		      0x11c, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_apb0_twd_clk, "r-apb0-twd", "r-apb0",
		      0x12c, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu", "r-apb0",
		      0x1ac, BIT(0), CLK_IGNORE_UNUSED);

static const char * const r_apb0_ir_rx_parents[] = { "rtc32k", "dcxo24M" };
static SUNXI_CCU_MP_WITH_MUX_GATE(r_apb0_ir_rx_clk, "r-apb0-ir-rx",
				  r_apb0_ir_rx_parents, 0x1c0,
				  0, 5,		/* M */
				  8, 2,		/* P */
				  24, 2,	/* mux */
				  BIT(31),	/* gate */
				  CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_apb0_bus_ir_rx_clk, "r-apb0-bus-ir-rx", "r-apb0",
		      0x1cc, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_ahb_bus_rtc_clk, "r-ahb-rtc", "r-ahb",
		      0x20c, BIT(0), CLK_IGNORE_UNUSED);

static struct ccu_common *sun251iw1_r_ccu_clks[] = {
	&r_ahb_clk.common,
	&r_apb0_clk.common,
	&r_apb0_timer_clk.common,
	&r_apb0_twd_clk.common,
	&r_ppu_clk.common,
	&r_apb0_ir_rx_clk.common,
	&r_apb0_bus_ir_rx_clk.common,
	&r_ahb_bus_rtc_clk.common,
};

static struct clk_hw_onecell_data sun251iw1_r_hw_clks = {
	.hws	= {
		[CLK_R_AHB]		= &r_ahb_clk.common.hw,
		[CLK_R_APB0]		= &r_apb0_clk.common.hw,
		[CLK_R_APB0_TIMER]	= &r_apb0_timer_clk.common.hw,
		[CLK_R_APB0_TWD]	= &r_apb0_twd_clk.common.hw,
		[CLK_R_PPU]		= &r_ppu_clk.common.hw,
		[CLK_R_APB0_IRRX]	= &r_apb0_ir_rx_clk.common.hw,
		[CLK_R_APB0_BUS_IRRX]	= &r_apb0_bus_ir_rx_clk.common.hw,
		[CLK_R_AHB_BUS_RTC]	= &r_ahb_bus_rtc_clk.common.hw,
	},
	.num	= CLK_NUMBER,
};

static struct ccu_reset_map sun251iw1_r_ccu_resets[] = {
	[RST_BUS_R_TIMER]	=  { 0x11c, BIT(16) },
	[RST_BUS_R_PPU]		=  { 0x1ac, BIT(16) },
	[RST_BUS_R_IRRX]	=  { 0x1cc, BIT(16) },
	[RST_BUS_R_RTC]		=  { 0x20c, BIT(16) },
};

static const struct sunxi_ccu_desc sun251iw1_r_ccu_desc = {
	.ccu_clks	= sun251iw1_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun251iw1_r_ccu_clks),

	.hw_clks	= &sun251iw1_r_hw_clks,

	.resets		= sun251iw1_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun251iw1_r_ccu_resets),
};

static int sun251iw1_r_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun251iw1_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun251iw1_r_ccu_clks,
			     ARRAY_SIZE(sun251iw1_r_ccu_clks),
			     NULL, 0);

	sunxi_info(NULL, "sunxi r_ccu driver version: %s\n", SUNXI_R_CCU_VERSION);
	return 0;
}

static const struct of_device_id sun251iw1_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun251iw1-r-ccu" },
	{ }
};

static struct platform_driver sun251iw1_r_ccu_driver = {
	.probe	= sun251iw1_r_ccu_probe,
	.driver	= {
		.name	= "sun251iw1-r-ccu",
		.of_match_table	= sun251iw1_r_ccu_ids,
	},
};

static int __init sunxi_r_ccu_sun251iw1_init(void)
{
	int ret;

	ret = platform_driver_register(&sun251iw1_r_ccu_driver);
	if (ret)
		pr_err("register ccu sun251iw1 failed\n");

	return ret;
}
core_initcall(sunxi_r_ccu_sun251iw1_init);

static void __exit sunxi_r_ccu_sun251iw1_exit(void)
{
	return platform_driver_unregister(&sun251iw1_r_ccu_driver);
}
module_exit(sunxi_r_ccu_sun251iw1_exit);

MODULE_VERSION(SUNXI_R_CCU_VERSION);
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
