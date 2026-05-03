// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 huangzhenwei@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun251iw1.h"

#define SUNXI_CCU_VERSION		"0.0.9"
/* ccu_des_start */

#define SUN251IW1_PLL_DDR_CTRL_REG   0x0010
static struct ccu_nm pll_ddr_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 20, 84),
	.m			= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.min_rate		= 240000000,
	.max_rate		= 2016000000,
	.common			= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN251IW1_PLL_PERI_CTRL_REG   0x0020
static struct ccu_nm pll_peri_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate		= 1272000000,
	.max_rate		= 2520000000,
	.common			= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-peri", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri_2x_clk, "pll-peri-2x",
			"pll-peri", 0x0020,
			16, 3, 0);

static SUNXI_CCU_M(pll_peri_800m_clk, "pll-peri-800m",
			"pll-peri", 0x0020,
			20, 3, 0);

static CLK_FIXED_FACTOR(pll_peri_1x_clk, "pll-peri-1x",
			"pll-peri-2x", 2, 1, 0);

static CLK_FIXED_FACTOR(gmac_50m_clk, "gmac-50m",
			"pll-peri-1x", 12, 1, 0);

static CLK_FIXED_FACTOR(gmac_25m_clk, "gmac-25m",
			"gmac-50m", 2, 1, 0);

static CLK_FIXED_FACTOR(clk25m_clk, "clk25m",
			"pll-peri-1x", 24, 1, 0);

static CLK_FIXED_FACTOR(clk16m_clk, "clk16m",
			"pll-peri-2x", 75, 1, 0);

static CLK_FIXED_FACTOR(clk12m_clk, "clk12m",
			"dcxo24M", 2, 1, 0);

static CLK_FIXED_FACTOR(clk32k_clk, "clk32k",
			"pll-peri-2x", 36621, 1, 0);

static CLK_FIXED_FACTOR(hdmi_cec_clk32k_clk, "hdmi-cec-clk32k",
			"pll-peri-2x", 36621, 1, 0);

#define SUN251IW1_PLL_VIDEO0_CTRL_REG   0x0040
static struct ccu_nm pll_video0_4x_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate		= 1272000000,
	.max_rate		= 2520000000,
	.common			= {
		.reg		= 0x0040,
		.hw.init	= CLK_HW_INIT("pll-video0-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR(pll_video0_2x_clk, "pll-video0-2x",
			"pll-video0-4x", 2, 1, CLK_SET_RATE_PARENT);

static CLK_FIXED_FACTOR(pll_video0_1x_clk, "pll-video0-1x",
			"pll-video0-4x", 4, 1, CLK_SET_RATE_PARENT);

#define SUN251IW1_PLL_VIDEO1_CTRL_REG   0x0048
static struct ccu_nm pll_video1_4x_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 53, 105),
	.min_rate		= 1272000000,
	.max_rate		= 2520000000,
	.common			= {
		.reg		= 0x0048,
		.hw.init	= CLK_HW_INIT("pll-video1-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR(pll_video1_2x_clk, "pll-video1-2x",
			"pll-video1-4x", 2, 1, CLK_SET_RATE_PARENT);

/*Noting: This formula maybe need to modify*/
static CLK_FIXED_FACTOR(pll_video1_1x_clk, "pll-video1-1x",
			"pll-video1-4x", 4, 1, CLK_SET_RATE_PARENT);

#define SUN251IW1_PLL_VE_CTRL_REG   0x0058
static struct ccu_nm pll_ve_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 20, 84),
	.m			= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.min_rate		= 240000000,
	.max_rate		= 2016000000,
	.common			= {
		.reg		= 0x0058,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN251IW1_PLL_AUDIO0_CTRL_REG   0x0078
static struct ccu_sdm_setting pll_audio0_sdm_table[] = {
	{ .rate = 90316800, .pattern = 0xc000872b, .m = 20, .n = 75 },	/* pll_audio0_4x 22.5792*4 = 90.3168M */
};
static struct ccu_nm pll_audio0_4x_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 20, 84),
	.m			= _SUNXI_CCU_DIV(16, 6), /* output divider */
	.sdm			= _SUNXI_CCU_SDM(pll_audio0_sdm_table, BIT(24),
				0x178, BIT(31)),
	.min_rate		= 90316800,
	.common			= {
		.reg		= 0x0078,
		.features	= CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init	= CLK_HW_INIT("pll-audio0-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR(pll_audio0_2x_clk, "pll-audio0-2x",
			"pll-audio0-4x", 2, 1, CLK_SET_RATE_PARENT);

static CLK_FIXED_FACTOR(pll_audio0_1x_clk, "pll-audio0-1x",
			"pll-audio0-4x", 4, 1, CLK_SET_RATE_PARENT);

#define SUN251IW1_PLL_AUDIO1_CTRL_REG   0x0080
static struct ccu_nm pll_audio1_clk = {
	.output			= BIT(27),
	.lock			= BIT(28),
	.lock_enable		= BIT(29),
	.ldo_en			= BIT(30),
	.enable			= BIT(31),
	.n			= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 64, 130),
	.m			= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.min_rate		= 768000000,
	.max_rate		= 3120000000,
	.common			= {
		.reg		= 0x0080,
		.hw.init	= CLK_HW_INIT("pll-audio1", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_audio1_div2_clk, "pll-audio1-div2",
			"pll-audio1", 0x0080,
			16, 3, CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_audio1_div5_clk, "pll-audio1-div5",
			"pll-audio1", 0x0080,
			20, 3, CLK_SET_RATE_PARENT);

static const char * const psi_parents[] = { "dcxo24M", "rtc32k", "clk16m-rc", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX(psi_clk, "psi",
			psi_parents,
			0x0510,
			0, 2,	/* M */
			8, 2,	/* P */
			24, 2,	/* mux */
			0);

static const char * const apb0_parents[] = { "dcxo24M", "rtc32k", "psi-clk", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX(apb0_clk, "apb0",
			apb0_parents,
			0x0520,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 2,	/* mux */
			0);

static const char * const apb1_parents[] = { "dcxo24M", "rtc32k", "psi-clk", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX(apb1_clk, "apb1",
			apb1_parents,
			0x0524,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 2,	/* mux */
			0);

static const char * const de_parents[] = { "pll-peri-2x", "pll-video0-4x", "pll-video1-4x", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
			de_parents, 0x0600,
			0, 5,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(de_bus_clk, "de-bus",
			"dcxo24M",
			0x060C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const di_parents[] = { "pll-peri-2x", "pll-video0-4x", "pll-video1-4x", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(di_clk, "di",
			di_parents, 0x0620,
			0, 5,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(di_bus_clk, "di-bus",
			"dcxo24M",
			0x062C, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri-2x", "pll-video0-4x", "pll-video1-4x", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
			g2d_parents, 0x0630,
			0, 5,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2d-bus",
			"dcxo24M",
			0x063C, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-peri-2x", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(ce_clk, "ce",
			ce_parents, 0x0680,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
			"dcxo24M",
			0x068C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const ve_parents[] = { "pll-ve", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
			ve_parents, 0x0690,
			0, 5,	/* M */
			24, 1,	/* mux */
			BIT(31),	/* gate */
			CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(ve_bus_clk, "ve-bus",
			"dcxo24M",
			0x069C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(dma_bus_clk, "dma-bus",
			"dcxo24M",
			0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_bus_clk, "spinlock-bus",
			"dcxo24M",
			0x072C, BIT(0), 0);

static SUNXI_CCU_GATE(hstimer_bus_clk, "hstimer-bus",
			"dcxo24M",
			0x073C, BIT(0), 0);

static SUNXI_CCU_GATE(avs_bus_clk, "avs-bus",
			"dcxo24M",
			0x0740, BIT(31), 0);

static SUNXI_CCU_GATE(dbgsys_bus_clk, "dbgsys-bus",
			"dcxo24M",
			0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm_bus_clk, "pwm-bus",
			"dcxo24M",
			0x07AC, BIT(0), CLK_IGNORE_UNUSED);

static const char * const dram_parents[] = { "pll-ddr", "pll-audio1-div2", "pll-peri-2x", "pll-peri-800m" };

static SUNXI_CCU_MP_WITH_MUX_GATE(dram_clk, "dram",
			dram_parents, 0x0800,
			0, 2,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(g2d_m_bus_clk, "g2d-m-bus",
			"dcxo24M",
			0x0804, BIT(10), 0);

static SUNXI_CCU_GATE(csi_m_bus_clk, "csi-m-bus",
			"dcxo24M",
			0x0804, BIT(8), 0);

static SUNXI_CCU_GATE(ce_m_bus_clk, "ce-m-bus",
			"dcxo24M",
			0x0804, BIT(2), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(ve_m_bus_clk, "ve-m-bus",
			"dcxo24M",
			0x0804, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(dma_m_bus_clk, "dma-m-bus",
			"dcxo24M",
			0x0804, BIT(0), 0);

static SUNXI_CCU_GATE(dram_bus_clk, "dram-bus",
			"dcxo24M",
			0x080C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "pll-audio1-div2" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc0_clk, "smhc0",
			smhc0_parents, 0x0830,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),/* gate */
			0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "pll-audio1-div2" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc1_clk, "smhc1",
			smhc1_parents, 0x0834,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "pll-peri-800m", "pll-audio1-div2" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc2_clk, "smhc2",
			smhc2_parents, 0x0838,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
			"dcxo24M",
			0x084C, BIT(2), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
			"dcxo24M",
			0x084C, BIT(1), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
			"dcxo24M",
			0x084C, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_bus_clk, "uart5-bus",
			"dcxo24M",
			0x090C, BIT(5), 0);

static SUNXI_CCU_GATE(uart4_bus_clk, "uart4-bus",
			"dcxo24M",
			0x090C, BIT(4), 0);

static SUNXI_CCU_GATE(uart3_bus_clk, "uart3-bus",
			"dcxo24M",
			0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_bus_clk, "uart2-bus",
			"dcxo24M",
			0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(uart1_bus_clk, "uart1-bus",
			"dcxo24M",
			0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(uart0_bus_clk, "uart0-bus",
			"dcxo24M",
			0x090C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(twi3_bus_clk, "twi3-bus",
			"dcxo24M",
			0x091C, BIT(3), 0);

static SUNXI_CCU_GATE(twi2_bus_clk, "twi2-bus",
			"dcxo24M",
			0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_bus_clk, "twi1-bus",
			"dcxo24M",
			0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_bus_clk, "twi0-bus",
			"dcxo24M",
			0x091C, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi0_clk, "spi0",
			spi0_parents, 0x0940,
			0, 4,	/* M */
			8, 2,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri-1x", "pll-peri-2x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi1_clk, "spi1",
			spi1_parents, 0x0944,
			0, 4,	/* M */
			8, 2,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
			"dcxo24M",
			0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
			"dcxo24M",
			0x096C, BIT(0), 0);

static const char * const gmac0_phy_parents[] = { "gmac-25m", "gmac-50m" };

static SUNXI_CCU_MUX_WITH_GATE(gmac0_phy_clk, "gmac0-phy",
			gmac0_phy_parents, 0x0970,
			24, 3,	/* mux */
			BIT(31) | BIT(30), 0);

static const char * const gmac1_phy_parents[] = { "gmac-25m", "gmac-50m" };

static SUNXI_CCU_MUX_WITH_GATE(gmac1_phy_clk, "gmac1-phy",
			gmac1_phy_parents, 0x0974,
			24, 3,	/* mux */
			BIT(31) | BIT(30), 0);

static SUNXI_CCU_GATE(gmac1_bus_clk, "gmac1-bus",
			"dcxo24M",
			0x097C, BIT(1), 0);

static SUNXI_CCU_GATE(gmac0_bus_clk, "gmac0-bus",
			"dcxo24M",
			0x097C, BIT(0), 0);

static const char * const irtx_parents[] = { "dcxo24M", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(irtx_clk, "irtx",
			irtx_parents, 0x09C0,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(irtx_bus_clk, "irtx-bus",
			"dcxo24M",
			0x09CC, BIT(0), 0);

static SUNXI_CCU_GATE(gpadc_bus_clk, "gpadc-bus",
			"dcxo24M",
			0x09EC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_bus_clk, "ths-bus",
			"dcxo24M",
			0x09FC, BIT(0), 0);

static const char * const i2s0_parents[] = { "pll-audio0-1x", "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(i2s0_clk, "i2s0",
			i2s0_parents, 0x0A10,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const i2s1_parents[] = { "pll-audio0-1x", "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(i2s1_clk, "i2s1",
			i2s1_parents, 0x0A14,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const i2s2_parents[] = { "pll-audio0-1x", "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(i2s2_clk, "i2s2",
			i2s2_parents, 0x0A18,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s2_bus_clk, "i2s2-bus",
			"dcxo24M",
			0x0A20, BIT(2), 0);

static SUNXI_CCU_GATE(i2s1_bus_clk, "i2s1-bus",
			"dcxo24M",
			0x0A20, BIT(1), 0);

static SUNXI_CCU_GATE(i2s0_bus_clk, "i2s0-bus",
			"dcxo24M",
			0x0A20, BIT(0), 0);

static const char * const owa_tx_parents[] = { "pll-audio0-1x", "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(owa_tx_clk, "owa-tx",
			owa_tx_parents, 0x0A24,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const owa_rx_parents[] = { "pll-peri-1x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(owa_rx_clk, "owa-rx",
			owa_rx_parents, 0x0A28,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(owa_bus_clk, "owa-bus",
			"dcxo24M",
			0x0A2C, BIT(0), 0);

static const char * const dmic_parents[] = { "pll-audio0-1x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(dmic_clk, "dmic",
			dmic_parents, 0x0A40,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
			"dcxo24M",
			0x0A4C, BIT(0), 0);

static const char * const audio_codec_dac_parents[] = { "pll-audio0-1x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(audio_codec_dac_clk, "audio-codec-dac",
			audio_codec_dac_parents, 0x0A50,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const audio_codec_adc_parents[] = { "pll-audio0-1x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE(audio_codec_adc_clk, "audio-codec-adc",
			audio_codec_adc_parents, 0x0A54,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(audio_codec_bus_clk, "audio-codec-bus",
			"dcxo24M",
			0x0A5C, BIT(0), 0);

static SUNXI_CCU_GATE(usb0_bus_clk, "usb0-bus",
			"dcxo24M",
			0x0A70, BIT(31), 0);

static SUNXI_CCU_GATE(usb1_bus_clk, "usb1-bus",
			"dcxo24M",
			0x0A74, BIT(31), 0);

static SUNXI_CCU_GATE(usbotg0_bus_clk, "usbotg0-bus",
			"dcxo24M",
			0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci1_bus_clk, "usbehci1-bus",
			"dcxo24M",
			0x0A8C, BIT(5), 0);

static SUNXI_CCU_GATE(usbehci0_bus_clk, "usbehci0-bus",
			"dcxo24M",
			0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci1_bus_clk, "usbohci1-bus",
			"dcxo24M",
			0x0A8C, BIT(1), 0);

static SUNXI_CCU_GATE(usbohci0_bus_clk, "usbohci0-bus",
			"dcxo24M",
			0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(lradc_bus_clk, "lradc-bus",
			"dcxo24M",
			0x0A9C, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top_bus_clk, "dpss-top-bus",
			"dcxo24M",
			0x0ABC, BIT(0), CLK_IGNORE_UNUSED);

static const char * const hdmi_rx_dtl_parents[] = { "pll-peri-1x", "pll-video0-2x", "pll-video1-2x", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(hdmi_rx_dtl_clk, "hdmi-rx-dtl",
			hdmi_rx_dtl_parents, 0x0B00,
			0, 5,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static const char * const hdmi_rx_cb_parents[] = { "pll-video0-1x", "pll-video0-4x", "pll-video1-1x", "pll-video1-4x", "pll-peri-2x", "pll-audio1-div2" };

static SUNXI_CCU_MP_WITH_MUX_GATE(hdmi_rx_cb_clk, "hdmi-rx-cb",
			hdmi_rx_cb_parents, 0x0B04,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_NO_REPARENT);

static const char * const hdmi_cec_parents[] = { "rtc32k", "hdmi-cec-clk32k" };

static SUNXI_CCU_MUX_WITH_GATE(hdmi_cec_clk, "hdmi-cec",
			hdmi_cec_parents, 0x0B08,
			24, 1,	/* mux */
			BIT(31), 0);

static SUNXI_CCU_GATE(cec_peri_gate_clk, "cec-peri-gate",
			"dcxo24M",
			0x0B08, BIT(30), 0);

static SUNXI_CCU_GATE(hdmi_rx_bus_clk, "hdmi-rx-bus",
			"dcxo24M",
			0x0B0C, BIT(0), 0);

static const char * const hrc_parents[] = { "dcxo24M", "pll-peri-2x", "pll-video0-4x", "pll-video1-4x", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(hrc_clk, "hrc",
			hrc_parents, 0x0B10,
			0, 5,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(hrc_bus_clk, "hrc-bus",
			"dcxo24M",
			0x0B1C, BIT(0), 0);

static const char * const dsi_parents[] = { "dcxo24M", "pll-peri-1x", "pll-video0-2x", "pll-video1-2x", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi_clk, "dsi",
			dsi_parents, 0x0B24,
			0, 4,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_IGNORE_UNUSED | CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(dsi_bus_clk, "dsi-bus",
			"dcxo24M",
			0x0B4C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const tconlcd_parents[] = { "pll-video0-1x", "pll-video0-4x", "pll-video1-1x", "pll-video1-4x", "pll-peri-2x", "pll-audio1-div2" };

static SUNXI_CCU_MP_WITH_MUX_GATE(tconlcd_clk, "tconlcd",
			tconlcd_parents, 0x0B60,
			0, 4,	/* M */
			8, 2,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_IGNORE_UNUSED |	\
			CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(tconlcd_bus_clk, "tconlcd-bus",
			"dcxo24M",
			0x0B7C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const ledc_parents[] = { "dcxo24M", "pll-peri-1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(ledc_clk, "ledc",
			ledc_parents, 0x0BF0,
			0, 4,	/* M */
			8, 2,	/* P */
			24, 1,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(ledc_bus_clk, "ledc-bus",
			"dcxo24M",
			0x0BFC, BIT(0), 0);

static const char * const csi_parents[] = { "pll-peri-2x", "pll-video0-2x", "pll-video1-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
			csi_parents, 0x0C04,
			0, 4,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT);

static const char * const csi_master_parents[] = { "dcxo24M", "pll-peri-1x", "pll-video0-1x", "pll-video1-1x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master_clk, "csi-master",
			csi_master_parents, 0x0C08,
			0, 5,	/* M */
			24, 3,	/* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
			"dcxo24M",
			0x0C1C, BIT(0), 0);

static const char * const tpadc_parents[] = { "dcxo24M", "pll-audio0-1x" };

static SUNXI_CCU_MUX_WITH_GATE(tpadc_clk, "tpadc",
			tpadc_parents, 0x0C50,
			24, 3,	/* mux */
			BIT(31), 0);

static SUNXI_CCU_GATE(tpadc_bus_clk, "tpadc-bus",
			"dcxo24M",
			0x0C5C, BIT(0), 0);

static const char * const riscv_parents[] = { "dcxo24M", "rtc32k", "clk16m-rc", "pll-peri-800m", "pll-peri-1x", "pll-cpu", "pll-audio1-div2" };

static SUNXI_CCU_M_WITH_MUX(riscv_axi_clk, "riscv-axi", riscv_parents,
			0x0D00, 8, 2, 24, 3, 0);

static SUNXI_CCU_GATE(riscv_cfg_bus_clk, "riscv-cfg-bus",
			"dcxo24M",
			0x0D0C, BIT(0), 0);

static SUNXI_CCU_GATE(clk32k_bus_clk, "clk32k-bus",
			"clk32k",
			0x0F30, BIT(4), 0);

static SUNXI_CCU_GATE(clk25m_bus_clk, "clk25m-bus",
			"clk25m",
			0x0F30, BIT(3), 0);

static SUNXI_CCU_GATE(clk16m_bus_clk, "clk16m-bus",
			"clk16m",
			0x0F30,	BIT(2), 0);

static SUNXI_CCU_GATE(clk12m_bus_clk, "clk12m-bus",
			"clk12m",
			0x0F30, BIT(1), 0);

static SUNXI_CCU_GATE(clk24m_bus_clk, "clk24m-bus",
			"dcxo24M",
			0x0F30, BIT(0), 0);

static const char * const clk27m_parents[] = { "pll-video0-1x", "pll-video1-1x" };
static SUNXI_CCU_MP_WITH_MUX(clk27m_clk, "clk27m",
			clk27m_parents,
			0x0F34,
			0, 5,	/* M */
			8, 2,	/* P */
			24, 2,	/* mux */
			0);

static SUNXI_CCU_M(pclk_clk, "pclk",
		"apb0", 0x0F38, 0, 5, 0);

static const char * const fanout2_parents[] = { "clk32k-bus", "clk12m-bus", "clk16m-bus", "clk24m-bus", "clk25m-bus", "clk27m", "pclk"};

static SUNXI_CCU_MUX_WITH_GATE(fanout2_clk, "fanout2",
			fanout2_parents, 0x0F3C,
			6, 3,
			BIT(23), 0);

static SUNXI_CCU_MUX_WITH_GATE(fanout1_clk, "fanout1",
			fanout2_parents, 0x0F3C,
			3, 3,
			BIT(22), 0);

static SUNXI_CCU_MUX_WITH_GATE(fanout0_clk, "fanout0",
			fanout2_parents, 0x0F3C,
			0, 3,
			BIT(21), 0);

#ifdef CONFIG_PM_SLEEP
static struct ccu_nm pll_audio0_sdm_pat0_clk = {
	.common		= {
		.reg		= 0x140,
	},
};

static struct ccu_nm pll_audio1_sdm_pat0_clk = {
	.common		= {
		.reg		= 0x180,
	},
};
#endif
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun251iw1_ccu_resets[] = {
	[RST_BUS_PLL_SSC_RSTN]		= { 0x0200, BIT(30) },
	[RST_MBUS]			= { 0x0540, BIT(30) },
	[RST_BUS_DE]			= { 0x060c, BIT(16) },
	[RST_BUS_KSC]			= { 0x061c, BIT(16) },
	[RST_BUS_DI]			= { 0x062c, BIT(16) },
	[RST_BUS_G2D]			= { 0x063c, BIT(16) },
	[RST_BUS_CE]			= { 0x068c, BIT(16) },
	[RST_BUS_VE]			= { 0x069c, BIT(16) },
	[RST_BUS_DMA]			= { 0x070c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x072c, BIT(16) },
	[RST_BUS_HSTIME]		= { 0x073c, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x078c, BIT(16) },
	[RST_BUS_PWM]			= { 0x07ac, BIT(16) },
	[RST_BUS_DRAM_MODULE]		= { 0x0800, BIT(30) },
	[RST_BUS_DRAM]			= { 0x080c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x084c, BIT(18) },
	[RST_BUS_SMHC1]			= { 0x084c, BIT(17) },
	[RST_BUS_SMHC0]			= { 0x084c, BIT(16) },
	[RST_BUS_UART5]			= { 0x090c, BIT(21) },
	[RST_BUS_UART4]			= { 0x090c, BIT(20) },
	[RST_BUS_UART3]			= { 0x090c, BIT(19) },
	[RST_BUS_UART2]			= { 0x090c, BIT(18) },
	[RST_BUS_UART1]			= { 0x090c, BIT(17) },
	[RST_BUS_UART0]			= { 0x090c, BIT(16) },
	[RST_BUS_TWI3]			= { 0x091c, BIT(19) },
	[RST_BUS_TWI2]			= { 0x091c, BIT(18) },
	[RST_BUS_TWI1]			= { 0x091c, BIT(17) },
	[RST_BUS_TWI0]			= { 0x091c, BIT(16) },
	[RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	[RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	[RST_BUS_GMAC1]			= { 0x097c, BIT(17) },
	[RST_BUS_GMAC0]			= { 0x097c, BIT(16) },
	[RST_BUS_IRTX]			= { 0x09cc, BIT(16) },
	[RST_BUS_GPADC]			= { 0x09ec, BIT(16) },
	[RST_BUS_TH]			= { 0x09fc, BIT(16) },
	[RST_BUS_I2S2]			= { 0x0a20, BIT(18) },
	[RST_BUS_I2S1]			= { 0x0a20, BIT(17) },
	[RST_BUS_I2S0]			= { 0x0a20, BIT(16) },
	[RST_BUS_OWA]			= { 0x0a2c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x0a4c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x0a5c, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x0a70, BIT(30) },
	[RST_USB_PHY1_RSTN]		= { 0x0a74, BIT(30) },
	[RST_USB_OTG0]			= { 0x0a8c, BIT(24) },
	[RST_USB_EHCI1]			= { 0x0a8c, BIT(21) },
	[RST_USB_EHCI0]			= { 0x0a8c, BIT(20) },
	[RST_USB_OHCI1]			= { 0x0a8c, BIT(17) },
	[RST_USB_OHCI0]			= { 0x0a8c, BIT(16) },
	[RST_BUS_LRADC]			= { 0x0a9c, BIT(16) },
	[RST_BUS_DPSS_TOP]		= { 0x0abc, BIT(16) },
	[RST_BUS_HDMI_RX]		= { 0x0b0c, BIT(16) },
	[RST_BUS_HRC]			= { 0x0b1c, BIT(16) },
	[RST_BUS_DSI]			= { 0x0b4c, BIT(16) },
	[RST_BUS_TCONLCD]		= { 0x0b7c, BIT(16) },
	[RST_BUS_LVDS0]			= { 0x0bac, BIT(16) },
	[RST_BUS_LEDC]			= { 0x0bfc, BIT(16) },
	[RST_BUS_CSI]			= { 0x0c1c, BIT(16) },
	[RST_BUS_TPADC]			= { 0x0c5c, BIT(16) },
	[RST_BUS_RISCV_CFG]		= { 0x0d0c, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun251iw1_hw_clks = {
	.hws    = {
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI]                  = &pll_peri_clk.common.hw,
		[CLK_PLL_PERI_2X]		= &pll_peri_2x_clk.common.hw,
		[CLK_PLL_PERI_1X]		= &pll_peri_1x_clk.hw,
		[CLK_PLL_PERI_800M]		= &pll_peri_800m_clk.common.hw,
		[CLK_GMAC_50M]			= &gmac_50m_clk.hw,
		[CLK_GMAC_25M]			= &gmac_25m_clk.hw,
		[CLK_CLK25M]			= &clk25m_clk.hw,
		[CLK_CLK16M]			= &clk16m_clk.hw,
		[CLK_CLK12M]			= &clk12m_clk.hw,
		[CLK_CLK32K]			= &clk32k_clk.hw,
		[CLK_HDMI_CEC_CLK32K]		= &hdmi_cec_clk32k_clk.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_VIDEO0_2X]		= &pll_video0_2x_clk.hw,
		[CLK_PLL_VIDEO0_1X]		= &pll_video0_1x_clk.hw,
		[CLK_PLL_VIDEO1_4X]		= &pll_video1_4x_clk.common.hw,
		[CLK_PLL_VIDEO1_2X]		= &pll_video1_2x_clk.hw,
		[CLK_PLL_VIDEO1_1X]		= &pll_video1_1x_clk.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_PLL_AUDIO0_4X]		= &pll_audio0_4x_clk.common.hw,
		[CLK_PLL_AUDIO0_2X]             = &pll_audio0_2x_clk.hw,
		[CLK_PLL_AUDIO0_1X]             = &pll_audio0_1x_clk.hw,
		[CLK_PLL_AUDIO1]		= &pll_audio1_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV2]		= &pll_audio1_div2_clk.common.hw,
		[CLK_PLL_AUDIO1_DIV5]		= &pll_audio1_div5_clk.common.hw,
		[CLK_PSI]			= &psi_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_BUS_DE]			= &de_bus_clk.common.hw,
		[CLK_DI]			= &di_clk.common.hw,
		[CLK_BUS_DI]			= &di_bus_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_BUS_G2D]			= &g2d_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_BUS_VE]			= &ve_bus_clk.common.hw,
		[CLK_BUS_DMA]			= &dma_bus_clk.common.hw,
		[CLK_BUS_SPINLOCK]		= &spinlock_bus_clk.common.hw,
		[CLK_BUS_HSTIMER]		= &hstimer_bus_clk.common.hw,
		[CLK_BUS_AVS]			= &avs_bus_clk.common.hw,
		[CLK_BUS_DBGSYS]		= &dbgsys_bus_clk.common.hw,
		[CLK_BUS_PWM]			= &pwm_bus_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_BUS_G2D_M]			= &g2d_m_bus_clk.common.hw,
		[CLK_BUS_CSI_M]			= &csi_m_bus_clk.common.hw,
		[CLK_BUS_CE_M]			= &ce_m_bus_clk.common.hw,
		[CLK_BUS_VE_M]			= &ve_m_bus_clk.common.hw,
		[CLK_BUS_DMA_M]			= &dma_m_bus_clk.common.hw,
		[CLK_BUS_DRAM]			= &dram_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_BUS_UART5]			= &uart5_bus_clk.common.hw,
		[CLK_BUS_UART4]			= &uart4_bus_clk.common.hw,
		[CLK_BUS_UART3]			= &uart3_bus_clk.common.hw,
		[CLK_BUS_UART2]			= &uart2_bus_clk.common.hw,
		[CLK_BUS_UART1]			= &uart1_bus_clk.common.hw,
		[CLK_BUS_UART0]			= &uart0_bus_clk.common.hw,
		[CLK_BUS_TWI3]			= &twi3_bus_clk.common.hw,
		[CLK_BUS_TWI2]			= &twi2_bus_clk.common.hw,
		[CLK_BUS_TWI1]			= &twi1_bus_clk.common.hw,
		[CLK_BUS_TWI0]			= &twi0_bus_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_GMAC0_PHY]			= &gmac0_phy_clk.common.hw,
		[CLK_GMAC1_PHY]			= &gmac1_phy_clk.common.hw,
		[CLK_BUS_GMAC1]			= &gmac1_bus_clk.common.hw,
		[CLK_BUS_GMAC0]			= &gmac0_bus_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_BUS_IRTX]			= &irtx_bus_clk.common.hw,
		[CLK_BUS_GPADC]			= &gpadc_bus_clk.common.hw,
		[CLK_BUS_THS]			= &ths_bus_clk.common.hw,
		[CLK_I2S0]			= &i2s0_clk.common.hw,
		[CLK_I2S1]			= &i2s1_clk.common.hw,
		[CLK_I2S2]			= &i2s2_clk.common.hw,
		[CLK_BUS_I2S2]			= &i2s2_bus_clk.common.hw,
		[CLK_BUS_I2S1]			= &i2s1_bus_clk.common.hw,
		[CLK_BUS_I2S0]			= &i2s0_bus_clk.common.hw,
		[CLK_OWA_TX]			= &owa_tx_clk.common.hw,
		[CLK_OWA_RX]			= &owa_rx_clk.common.hw,
		[CLK_BUS_OWA]			= &owa_bus_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC]		= &audio_codec_dac_clk.common.hw,
		[CLK_AUDIO_CODEC_ADC]		= &audio_codec_adc_clk.common.hw,
		[CLK_BUS_AUDIO_CODEC]		= &audio_codec_bus_clk.common.hw,
		[CLK_BUS_USB0]			= &usb0_bus_clk.common.hw,
		[CLK_BUS_USB1]			= &usb1_bus_clk.common.hw,
		[CLK_BUS_USBOTG0]		= &usbotg0_bus_clk.common.hw,
		[CLK_BUS_USBEHCI1]		= &usbehci1_bus_clk.common.hw,
		[CLK_BUS_USBEHCI0]		= &usbehci0_bus_clk.common.hw,
		[CLK_BUS_USBOHCI1]		= &usbohci1_bus_clk.common.hw,
		[CLK_BUS_USBOHCI0]		= &usbohci0_bus_clk.common.hw,
		[CLK_BUS_LRADC]			= &lradc_bus_clk.common.hw,
		[CLK_BUS_DPSS_TOP]		= &dpss_top_bus_clk.common.hw,
		[CLK_HDMI_RX_DTL]		= &hdmi_rx_dtl_clk.common.hw,
		[CLK_HDMI_RX_CB]		= &hdmi_rx_cb_clk.common.hw,
		[CLK_HDMI_CEC]			= &hdmi_cec_clk.common.hw,
		[CLK_BUS_HDMI_RX]		= &hdmi_rx_bus_clk.common.hw,
		[CLK_HRC]			= &hrc_clk.common.hw,
		[CLK_BUS_HRC]			= &hrc_bus_clk.common.hw,
		[CLK_DSI]			= &dsi_clk.common.hw,
		[CLK_BUS_DSI]			= &dsi_bus_clk.common.hw,
		[CLK_TCONLCD]			= &tconlcd_clk.common.hw,
		[CLK_BUS_TCONLCD]		= &tconlcd_bus_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_BUS_LEDC]			= &ledc_bus_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_CSI_MASTER]		= &csi_master_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_TPADC]			= &tpadc_clk.common.hw,
		[CLK_BUS_TPADC]			= &tpadc_bus_clk.common.hw,
		[CLK_RISCV_AXI]			= &riscv_axi_clk.common.hw,
		[CLK_BUS_RISCV_CFG]		= &riscv_cfg_bus_clk.common.hw,
		[CLK_BUS_CLK32K]		= &clk32k_bus_clk.common.hw,
		[CLK_BUS_CLK25M]		= &clk25m_bus_clk.common.hw,
		[CLK_BUS_CLK16M]		= &clk16m_bus_clk.common.hw,
		[CLK_BUS_CLK12M]		= &clk12m_bus_clk.common.hw,
		[CLK_BUS_CLK24M]		= &clk24m_bus_clk.common.hw,
		[CLK_CLK27M]			= &clk27m_clk.common.hw,
		[CLK_PCLK]			= &pclk_clk.common.hw,
		[CLK_FANOUT2]			= &fanout2_clk.common.hw,
		[CLK_FANOUT1]			= &fanout1_clk.common.hw,
		[CLK_FANOUT0]			= &fanout0_clk.common.hw,
		[CLK_CEC_PERI_GATE]		= &cec_peri_gate_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun251iw1_ccu_clks[] = {
	&pll_ddr_clk.common,
	&pll_peri_clk.common,
	&pll_peri_2x_clk.common,
	&pll_peri_800m_clk.common,
	&pll_video0_4x_clk.common,
	&pll_video1_4x_clk.common,
	&pll_ve_clk.common,
	&pll_audio0_4x_clk.common,
	&pll_audio1_clk.common,
	&pll_audio1_div2_clk.common,
	&pll_audio1_div5_clk.common,
	&psi_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&de_clk.common,
	&de_bus_clk.common,
	&di_clk.common,
	&di_bus_clk.common,
	&g2d_clk.common,
	&g2d_bus_clk.common,
	&ce_clk.common,
	&ce_bus_clk.common,
	&ve_clk.common,
	&ve_bus_clk.common,
	&dma_bus_clk.common,
	&spinlock_bus_clk.common,
	&hstimer_bus_clk.common,
	&avs_bus_clk.common,
	&dbgsys_bus_clk.common,
	&pwm_bus_clk.common,
	&dram_clk.common,
	&g2d_m_bus_clk.common,
	&csi_m_bus_clk.common,
	&ce_m_bus_clk.common,
	&ve_m_bus_clk.common,
	&dma_m_bus_clk.common,
	&dram_bus_clk.common,
	&smhc0_clk.common,
	&smhc1_clk.common,
	&smhc2_clk.common,
	&smhc2_bus_clk.common,
	&smhc1_bus_clk.common,
	&smhc0_bus_clk.common,
	&uart5_bus_clk.common,
	&uart4_bus_clk.common,
	&uart3_bus_clk.common,
	&uart2_bus_clk.common,
	&uart1_bus_clk.common,
	&uart0_bus_clk.common,
	&twi3_bus_clk.common,
	&twi2_bus_clk.common,
	&twi1_bus_clk.common,
	&twi0_bus_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spi1_bus_clk.common,
	&spi0_bus_clk.common,
	&gmac0_phy_clk.common,
	&gmac1_phy_clk.common,
	&gmac1_bus_clk.common,
	&gmac0_bus_clk.common,
	&irtx_clk.common,
	&irtx_bus_clk.common,
	&gpadc_bus_clk.common,
	&ths_bus_clk.common,
	&i2s0_clk.common,
	&i2s1_clk.common,
	&i2s2_clk.common,
	&i2s2_bus_clk.common,
	&i2s1_bus_clk.common,
	&i2s0_bus_clk.common,
	&owa_tx_clk.common,
	&owa_rx_clk.common,
	&owa_bus_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&audio_codec_dac_clk.common,
	&audio_codec_adc_clk.common,
	&audio_codec_bus_clk.common,
	&usb0_bus_clk.common,
	&usb1_bus_clk.common,
	&usbotg0_bus_clk.common,
	&usbehci1_bus_clk.common,
	&usbehci0_bus_clk.common,
	&usbohci1_bus_clk.common,
	&usbohci0_bus_clk.common,
	&lradc_bus_clk.common,
	&dpss_top_bus_clk.common,
	&hdmi_rx_dtl_clk.common,
	&hdmi_rx_cb_clk.common,
	&hdmi_cec_clk.common,
	&hdmi_rx_bus_clk.common,
	&hrc_clk.common,
	&hrc_bus_clk.common,
	&dsi_clk.common,
	&dsi_bus_clk.common,
	&tconlcd_clk.common,
	&tconlcd_bus_clk.common,
	&ledc_clk.common,
	&ledc_bus_clk.common,
	&csi_clk.common,
	&csi_master_clk.common,
	&csi_bus_clk.common,
	&tpadc_clk.common,
	&tpadc_bus_clk.common,
	&riscv_axi_clk.common,
	&riscv_cfg_bus_clk.common,
	&clk32k_bus_clk.common,
	&clk25m_bus_clk.common,
	&clk16m_bus_clk.common,
	&clk12m_bus_clk.common,
	&clk24m_bus_clk.common,
	&clk27m_clk.common,
	&pclk_clk.common,
	&fanout2_clk.common,
	&fanout1_clk.common,
	&fanout0_clk.common,
	&cec_peri_gate_clk.common,
#ifdef CONFIG_PM_SLEEP
	&pll_audio0_sdm_pat0_clk.common,
	&pll_audio1_sdm_pat0_clk.common,
#endif
};

static const struct sunxi_ccu_desc sun251iw1_ccu_desc = {
	.ccu_clks	= sun251iw1_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun251iw1_ccu_clks),

	.hw_clks	= &sun251iw1_hw_clks,

	.resets		= sun251iw1_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun251iw1_ccu_resets),
};

static int sun251iw1_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun251iw1_ccu_desc);
	if (ret)
		return ret;

	/* set pll_clk input_div to 1 */
	set_reg(reg + SUN251IW1_PLL_VIDEO0_CTRL_REG, 0x0, 1, 1);
	set_reg(reg + SUN251IW1_PLL_VIDEO1_CTRL_REG, 0x0, 1, 1);

	sunxi_ccu_sleep_init(reg, sun251iw1_ccu_clks,
			ARRAY_SIZE(sun251iw1_ccu_clks),
			NULL, 0);

	sunxi_info(NULL, "sunxi ccu driver version: %s\n", SUNXI_CCU_VERSION);
	return 0;
}

static const struct of_device_id sun251iw1_ccu_ids[] = {
	{ .compatible = "allwinner,sun251iw1-ccu" },
	{ }
};

static struct platform_driver sun251iw1_ccu_driver = {
	.probe	= sun251iw1_ccu_probe,
	.driver	= {
		.name	= "sun251iw1-ccu",
		.of_match_table	= sun251iw1_ccu_ids,
	},
};

static int __init sunxi_ccu_sun251iw1_init(void)
{
	int ret;

	ret = platform_driver_register(&sun251iw1_ccu_driver);
	if (ret)
		pr_err("register ccu sun251iw1 failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun251iw1_init);

static void __exit sunxi_ccu_sun251iw1_exit(void)
{
	return platform_driver_unregister(&sun251iw1_ccu_driver);
}
module_exit(sunxi_ccu_sun251iw1_exit);

MODULE_VERSION(SUNXI_CCU_VERSION);
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
