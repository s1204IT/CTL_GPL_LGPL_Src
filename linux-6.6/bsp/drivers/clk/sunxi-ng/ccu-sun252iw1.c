// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

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

#include "ccu-sun252iw1.h"

/* ccu_des_start */

#define UPD_KEY_VALUE 0x8000000

#define SUN252IW1_PLL_DDR_CTRL_REG   0x0010

#define SUN252IW1_PLL_PERI_CTRL_REG   0x0020

#define CPU_RESET_KEY_VALUE 0x16AA0000

static struct ccu_nm pll_cpu_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 12, 75),
	.m		= _SUNXI_CCU_DIV(16, 3), /* output divider */
	.min_rate	= 288000000,
	.max_rate	= 1800000000,
	.common		= {
		.reg		= 0x0,
		.hw.init	= CLK_HW_INIT("pll-cpu", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nm pll_ddr_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 8, 125),
	.m		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.min_rate	= 192000000,
	.max_rate	= 3000000000,
	.common		= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static struct ccu_nm pll_peri_parent_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 8, 133),
	.min_rate	= 192000000,
	.max_rate	= 3192000000,
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-peri-parent", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri_2x_clk, "pll-peri-2x",
		"pll-peri-parent", 0x0020, 16, 3, 0);

static SUNXI_CCU_M(pll_peri_800m_clk, "pll-peri-800m",
		"pll-peri-parent", 0x0020, 20, 3, 0);

static SUNXI_CCU_M(pll_peri_480m_clk, "pll-peri-480m",
		"pll-peri-parent", 0x0020, 2, 3, 0);


static CLK_FIXED_FACTOR_HW(pll_peri_600m_clk, "pll-peri-600m",
		&pll_peri_2x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_400m_clk, "pll-peri-400m",
		&pll_peri_2x_clk.common.hw,
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_300m_clk, "pll-peri-300m",
		"pll-peri-600m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_200m_clk, "pll-peri-200m",
		"pll-peri-400m",
		2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_160m_clk, "pll-peri-160m",
		"pll-peri-480m",
		3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri_150m_clk, "pll-peri-150m",
		"pll-peri-300m",
		2, 1, 0);

#define SUN252IW1_PLL_VIDEO_CTRL_REG   0x0040
static struct ccu_nm pll_video0_4x_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 11, 125),
	.m		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.min_rate	= 264000000,
	.max_rate	= 3000000000,
	.common		= {
		.reg		= SUN252IW1_PLL_VIDEO_CTRL_REG,
		.hw.init	= CLK_HW_INIT("pll-video-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN252IW1_PLL_CSI_CTRL_REG   0x0048
static struct ccu_nm pll_csi_4x_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 8, 100),
	.m		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.min_rate	= 192000000,
	.max_rate	= 2400000000,
	.common		= {
		.reg		= SUN252IW1_PLL_CSI_CTRL_REG,
		.hw.init	= CLK_HW_INIT("pll-csi-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN252IW1_PLL_AUDIO_CTRL_REG   0x0078
static struct ccu_nm pll_audio_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 8, 125),
	.min_rate	= 192000000,
	.max_rate	= 3000000000,
	.common		= {
		.reg		= SUN252IW1_PLL_AUDIO_CTRL_REG,
		.hw.init	= CLK_HW_INIT("pll-audio", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_audio_div2_clk, "pll-audio-div2",
		"pll-audio", SUN252IW1_PLL_AUDIO_CTRL_REG,
		16, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_audio_div3_clk, "pll-audio-div3",
		"pll-audio", SUN252IW1_PLL_AUDIO_CTRL_REG,
		20, 3,
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_M(pll_audio_div5_clk, "pll-audio-div5",
		"pll-audio", SUN252IW1_PLL_AUDIO_CTRL_REG,
		2, 3,
		CLK_SET_RATE_PARENT);  /* The default frequency is 614.4MHZ (24.576MHZ * 25) */

static SUNXI_CCU_M_WITH_GATE(ipmc_clk, "ipmc",
		"dcxo24M", 0x04FC,
		0, 5,
		BIT(31),
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(cpu_gatin_clk, "cpu-gatin",
		"dcxo24M",
		0x0504, BIT(16), 0);

static SUNXI_CCU_GATE_WITH_KEY(cpu_clk, "cpu",
		"dcxo24M",
		0x0504, CPU_RESET_KEY_VALUE, BIT(0), 0);


static const char * const pic_parents[] = { "pll-peri-200m", "pll-peri-400m" };

static SUNXI_CCU_MUX_WITH_GATE(pic_clk, "pic",
		pic_parents, 0x0508,
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(cpu_cfg_clk, "cpu-cfg",
		"dcxo24M",
		0x050C, BIT(0), 0);

static SUNXI_CCU_GATE(c907_aximon_clk, "c907-aximon",
		"dcxo24M",
		0x050C, BIT(1), 0);

static const char * const ahb_parents[] = { "dcxo24M", "clk32k", "clk16m-rc", "pll-peri-600m" };

static SUNXI_CCU_MP_WITH_MUX(ahb_clk, "ahb",
		ahb_parents,
		0x0510,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb0_parents[] = { "dcxo24M", "clk32k", "clk16m-rc", "pll-peri-600m" };

static SUNXI_CCU_MP_WITH_MUX(apb0_clk, "apb0",
		apb0_parents,
		0x0520,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb1_parents[] = { "dcxo24M", "clk32k", "clk16m-rc", "pll-peri-600m" };

static SUNXI_CCU_MP_WITH_MUX(apb1_clk, "apb1",
		apb1_parents,
		0x0524,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb_uart_parents[] = { "dcxo24M", "clk32k", "clk16m-rc", "pll-peri-600m", "pll-audio-div2" };

static SUNXI_CCU_MP_WITH_MUX(apb_uart_clk, "apb-uart",
		apb_uart_parents,
		0x0528,
		0, 5,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const de_parents[] = { "pll-peri-300m", "videopll1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
		de_parents, 0x0600,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(de_bus_clk, "de-bus",
		"dcxo24M",
		0x060C, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri-300m", "videopll1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x0630,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2-bus",
		"dcxo24M",
		0x063C, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-peri-400m", "pll-peri-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
		ce_parents, 0x0680,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
		"dcxo24M",
		0x068C, BIT(1), 0);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
		"dcxo24M",
		0x068C, BIT(0), 0);

static const char * const ve_parents[] = { "pll-peri-300m", "pll-peri-400m", "pll-peri-480m", "pll-peri-600m", "pll-video-4x", "pll-csi-4x", "pll-audio-div3" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
		ve_parents, 0x0690,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ve_bus_clk, "ve-bus",
		"dcxo24M",
		0x069C, BIT(0), 0);

static const char * const npu_parents[] = { "pll-peri-800m", "pll-peri-2x", "pll-audio-div5", "pll-audio-div3", "pll-audio-div2", "pll-video-4x", "pll-csi-4x", "pll-ddr" };

static SUNXI_CCU_M_WITH_MUX_GATE(npu_clk, "npu",
		npu_parents, 0x06E0,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(npu_bus_clk, "npu-bus",
		"dcxo24M",
		0x06EC, BIT(0), 0);

static SUNXI_CCU_GATE(ndma_clk, "ndma",
		"dcxo24M",
		0x070C, BIT(1), 0);

static SUNXI_CCU_GATE(sgdma_clk, "sgdma",
		"dcxo24M",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox1_clk, "msgbox1",
		"dcxo24M",
		0x071C, BIT(1), 0);

static SUNXI_CCU_GATE(msgbox0_clk, "msgbox0",
		"dcxo24M",
		0x071C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
		"dcxo24M",
		0x072C, BIT(0), 0);

static SUNXI_CCU_GATE(hstimer_clk, "hstimer",
		"dcxo24M",
		0x073C, BIT(0), 0);

static SUNXI_CCU_GATE(avs_clk, "avs",
		"dcxo24M",
		0x0740, BIT(31), 0);

static SUNXI_CCU_GATE(timer_apb_clk, "timer-apb",
		"dcxo24M",
		0x0744, BIT(31), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo24M",
		0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static SUNXI_CCU_GATE(iommu_clk, "iommu",
		"dcxo24M",
		0x07BC, BIT(0), 0);

static const char * const dram_parents[] = { "pll-ddr", "pll-peri-2x", "pll-peri-800m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX_KEY(dram_clk, "dram",
		dram_parents, 0x0800,
		0, 5,	/* M */
		8, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0, UPD_KEY_VALUE);

static SUNXI_CCU_GATE(npu_mbus_gate_clk, "npu-mbus-gate",
		"dcxo24M",
		0x0804, BIT(21), 0);

static SUNXI_CCU_GATE(vid_in_mbus_gate_clk, "vid-in-mbus-gate",
		"dcxo24M",
		0x0804, BIT(20), 0);

static SUNXI_CCU_GATE(vid_out_mbus_gate_clk, "vid-out-mbus-gate",
		"dcxo24M",
		0x0804, BIT(19), 0);

static SUNXI_CCU_GATE(ce_mbus_gate_clk, "ce-mbus-gate",
		"dcxo24M",
		0x0804, BIT(18), 0);

static SUNXI_CCU_GATE(ve_mbus_gate_clk, "ve-mbus-gate",
		"dcxo24M",
		0x0804, BIT(17), 0);

static SUNXI_CCU_GATE(sgdma_mbus_gate_clk, "sgdma-mbus-gate",
		"dcxo24M",
		0x0804, BIT(16), 0);

static SUNXI_CCU_GATE(dma_mbus_en_clk, "dma-mbus-en",
		"dcxo24M",
		0x0804, BIT(0), 0);

static SUNXI_CCU_GATE(dram_bus_clk, "dram-bus",
		"dcxo24M",
		0x080C, BIT(0), 0);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri-400m", "pll-peri-300m", "pll-video-4x", "pll-ddr" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc0_clk, "smhc0",
		smhc0_parents, 0x0830,
		0, 4,	/* M */
		8, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri-600m", "pll-peri-800m", "pll-video-4x", "pll-ddr" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc1_clk, "smhc1",
		smhc1_parents, 0x0834,
		0, 4,	/* M */
		8, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0x084C, BIT(1), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0x084C, BIT(0), 0);

static const char * const lpsramctrl_opi_clk2x_parents[] = { "dcxo24M", "pll-peri-480m", "pll-peri-400m", "pll-peri-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(lpsramctrl_opi_clk2x_clk, "lpsramctrl-opi-clk2x",
		lpsramctrl_opi_clk2x_parents, 0x0850,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(psram_ctrl_clk, "psram-ctrl",
		"dcxo24M",
		0x085C, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo24M",
		0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo24M",
		0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo24M",
		0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo24M",
		0x090C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
		"dcxo24M",
		0x091C, BIT(4), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
		"dcxo24M",
		0x091C, BIT(3), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo24M",
		0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo24M",
		0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo24M",
		0x091C, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri-300m", "pll-peri-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi0_clk, "spi0",
		spi0_parents, 0x0940,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri-300m", "pll-peri-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi1_clk, "spi1",
		spi1_parents, 0x0944,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const spif_parents[] = { "dcxo24M", "pll-peri-400m", "pll-peri-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spif_clk, "spif",
		spif_parents, 0x0950,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(spif_bus_clk, "spif-bus",
		"dcxo24M",
		0x096C, BIT(4), 0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo24M",
		0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo24M",
		0x096C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac_25m_clk, "gmac-25m",
		"dcxo24M",
		0x0970, BIT(31), 0);

static SUNXI_CCU_GATE(gmac_25m_clk_src_clk, "gmac-25m-clk-src",
		"dcxo24M",
		0x0970, BIT(30), 0);

static SUNXI_CCU_GATE(gmac_clk, "gmac",
		"dcxo24M",
		0x097C, BIT(0), 0);

static SUNXI_CCU_GATE(gpadc_clk, "gpadc",
		"dcxo24M",
		0x09EC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_clk, "ths",
		"dcxo24M",
		0x09FC, BIT(0), 0);

static const char * const i2s0_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s0_clk, "i2s0",
		i2s0_parents, 0x0A10,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s0_bus_clk, "i2s0-bus",
		"dcxo24M",
		0x0A2C, BIT(0), 0);

static const char * const audio_codec_dac_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_dac_clk, "audio-codec-dac",
		audio_codec_dac_parents, 0x0A50,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_codec_adc_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_adc_clk, "audio-codec-adc",
		audio_codec_adc_parents, 0x0A54,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x0A5C, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
		"dcxo24M",
		0x0A70, BIT(31), 0);

static SUNXI_CCU_GATE(usbotg0_clk, "usbotg0",
		"dcxo24M",
		0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci0_clk, "usbehci0",
		"dcxo24M",
		0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci0_clk, "usbohci0",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top_clk, "dpss-top",
		"dcxo24M",
		0x0ABC, BIT(0), 0);

static const char * const tconlcd_parents[] = { "pll-video-4x", "pll-peri-2x", "pll-csi-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(tconlcd_clk, "tconlcd",
		tconlcd_parents, 0x0B60,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(tconlcd_bus_clk, "tconlcd-bus",
		"dcxo24M",
		0x0B7C, BIT(0), 0);

static const char * const csi_parents[] = { "pll-peri-300m", "pll-peri-400m", "pll-video-4x", "pll-csi-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
		csi_parents, 0x0C04,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master0_parents[] = { "dcxo24M", "pll-csi-4x", "pll-video-4x", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master0_clk, "csi-master0",
		csi_master0_parents, 0x0C08,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master1_parents[] = { "dcxo24M", "pll-csi-4x", "pll-video-4x", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master1_clk, "csi-master1",
		csi_master1_parents, 0x0C0C,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const csi_master2_parents[] = { "dcxo24M", "pll-csi-4x", "pll-video-4x", "pll-peri-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_master2_clk, "csi-master2",
		csi_master2_parents, 0x0C10,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(csi_bus_clk, "csi-bus",
		"dcxo24M",
		0x0C2C, BIT(0), 0);

static SUNXI_CCU_GATE(e907_gating_rs_clk, "e907-gating-rs",
		"dcxo24M",
		0x0D04, BIT(16), 0);

static SUNXI_CCU_GATE(e907_clk, "e907",
		"dcxo24M",
		0x0D04, BIT(0), 1);

static SUNXI_CCU_GATE(riscv_cfg_clk, "riscv-cfg",
		"dcxo24M",
		0x0D0C, BIT(0), 0);

static SUNXI_CCU_GATE(e907_aximon_clk, "e907-aximon",
		"dcxo24M",
		0x0D0C, BIT(1), 0);

static SUNXI_CCU_GATE(cpus_hclk_gate_clk, "cpus-hclk-gate",
		"dcxo24M",
		0x0E04, BIT(28), 0);

static SUNXI_CCU_GATE(gmac_mbus_ahb_gate_clk, "gmac-mbus-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(13), 0);

static SUNXI_CCU_GATE(smhc1_mbus_ahb_gate_clk, "smhc1-mbus-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(11), 0);

static SUNXI_CCU_GATE(smhc0_mbus_ahb_gate_clk, "smhc0-mbus-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(10), 0);

static SUNXI_CCU_GATE(usb_mbus_ahb_gate_clk, "usb-mbus-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(9), 0);

static SUNXI_CCU_GATE(gmac_ahb_gate_clk, "gmac-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(8), 0);

static SUNXI_CCU_GATE(smhc1_ahb_gate_clk, "smhc1-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(6), 0);

static SUNXI_CCU_GATE(smhc0_ahb_gate_clk, "smhc0-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(5), 0);

static SUNXI_CCU_GATE(usb_ahb_gate_clk, "usb-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(4), 0);

static SUNXI_CCU_GATE(vid_out_ahb_gate_clk, "vid-out-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(3), 0);

static SUNXI_CCU_GATE(vid_in_ahb_gate_clk, "vid-in-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(2), 0);

static SUNXI_CCU_GATE(ve_ahb_gate_clk, "ve-ahb-gate",
		"dcxo24M",
		0x0E04, BIT(1), 0);

static SUNXI_CCU_GATE(res_dcap_24m_clk, "res-dcap-24m-",
		"dcxo24M",
		0x0E0C, BIT(3), 0);

static SUNXI_CCU_GATE(gpadc_24m_clk, "gpadc-24m-",
		"dcxo24M",
		0x0E0C, BIT(2), 0);

static SUNXI_CCU_GATE(usb_24m_clk, "usb-24m-",
		"dcxo24M",
		0x0E0C, BIT(0), 0);

static SUNXI_CCU_GATE(pll_output_gate_clk, "pll-output-gate",
		"dcxo24M",
		0x0E10, BIT(0), 0);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun252iw1_ccu_resets[] = {
	[RST_BUS_PLL_SSC_RSTN]		= { 0x0200, BIT(30) },
	[RST_BUS_CPU_IPMC_SOFT_RSTN]		= { 0x0504, BIT(12), CPU_RESET_KEY_VALUE },
	[RST_BUS_PIC_SOFT_RSTN]		= { 0x0504, BIT(3), CPU_RESET_KEY_VALUE },
	[RST_BUS_CPU_SYS_APB_SOFT_RSTN]		= { 0x0504, BIT(2), CPU_RESET_KEY_VALUE },
	[RST_BUS_CPU_SOFT_RSTN]		= { 0x0504, BIT(1), CPU_RESET_KEY_VALUE },
	[RST_BUS_C907_AXIMON]		= { 0x050c, BIT(17) },
	[RST_BUS_CPU_CFG]		= { 0x050c, BIT(16) },
	[RST_MBUS]			= { 0x0540, BIT(30) },
	[RST_BUS_DE]			= { 0x060c, BIT(16) },
	[RST_BUS_G2D]			= { 0x063c, BIT(16) },
	[RST_BUS_CE_SY]		= { 0x068c, BIT(17) },
	[RST_BUS_CE]			= { 0x068c, BIT(16) },
	[RST_BUS_VE]			= { 0x069c, BIT(16) },
	[RST_BUS_NPU_SW]		= { 0x06e4, BIT(8) },
	[RST_BUS_NPU]			= { 0x06ec, BIT(16) },
	[RST_BUS_NDMA]			= { 0x070c, BIT(17) },
	[RST_BUS_SGDMA]			= { 0x070c, BIT(16) },
	[RST_BUS_MSGBOX1]		= { 0x071c, BIT(17) },
	[RST_BUS_MSGBOX0]		= { 0x071c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x072c, BIT(16) },
	[RST_BUS_HSTIME]		= { 0x073c, BIT(16) },
	[RST_BUS_TIMER]		= { 0x074c, BIT(16) },
	[RST_BUS_DBGSY]		= { 0x078c, BIT(16) },
	[RST_BUS_PWM]			= { 0x07ac, BIT(16) },
	[RST_BUS_DRAM]			= { 0x080c, BIT(16) },
	[RST_BUS_SMHC1]		= { 0x084c, BIT(17) },
	[RST_BUS_SMHC0]		= { 0x084c, BIT(16) },
	[RST_BUS_PSRAM_CTRL]		= { 0x085c, BIT(16) },
	[RST_BUS_UART3]		= { 0x090c, BIT(19) },
	[RST_BUS_UART2]		= { 0x090c, BIT(18) },
	[RST_BUS_UART1]		= { 0x090c, BIT(17) },
	[RST_BUS_UART0]		= { 0x090c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x091c, BIT(20) },
	[RST_BUS_TWI3]			= { 0x091c, BIT(19) },
	[RST_BUS_TWI2]			= { 0x091c, BIT(18) },
	[RST_BUS_TWI1]			= { 0x091c, BIT(17) },
	[RST_BUS_TWI0]			= { 0x091c, BIT(16) },
	[RST_BUS_SPIF]			= { 0x096c, BIT(20) },
	[RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	[RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	[RST_BUS_GMAC]			= { 0x097c, BIT(16) },
	[RST_BUS_GPADC]		= { 0x09ec, BIT(16) },
	[RST_BUS_TH]			= { 0x09fc, BIT(16) },
	[RST_BUS_I2S0]			= { 0x0a2c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x0a5c, BIT(16) },
	[RST_USB_PHY0_RSTN]		= { 0x0a70, BIT(30) },
	[RST_USB_OTG0]			= { 0x0a8c, BIT(24) },
	[RST_USB_EHCI0]		= { 0x0a8c, BIT(20) },
	[RST_USB_OHCI0]		= { 0x0a8c, BIT(16) },
	[RST_BUS_DPSS_TOP]     = { 0x0abc, BIT(16) },
	[RST_BUS_TCONLCD]      = { 0x0b7c, BIT(16) },
	[RST_BUS_CSI]          = { 0x0c2c, BIT(16) },
	[RST_BUS_E907_CORE_SOFT_RSTN]      	= { 0x0d04, BIT(3) },
	[RST_BUS_RISCV_SYS_APB_SOFT_RSTN]      = { 0x0d04, BIT(2) },
	[RST_BUS_E907_SOFT_RSTN]       = { 0x0d04, BIT(1) },
	[RST_BUS_E907_AXIMON]      = { 0x0d0c, BIT(17) },
	[RST_BUS_RISCV_CFG]        = { 0x0d0c, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun252iw1_hw_clks = {
	.hws    = {
		[CLK_PLL_CPU]			= &pll_cpu_clk.common.hw,
		[CLK_PLL_PERI_PARENT]	= &pll_peri_parent_clk.common.hw,
		[CLK_PLL_PERI_2X]		= &pll_peri_2x_clk.common.hw,
		[CLK_PLL_PERI_800M]		= &pll_peri_800m_clk.common.hw,
		[CLK_PLL_PERI_480M]		= &pll_peri_480m_clk.common.hw,
		[CLK_PLL_PERI_600M]		= &pll_peri_600m_clk.hw,
		[CLK_PLL_PERI_400M]		= &pll_peri_400m_clk.hw,
		[CLK_PLL_PERI_300M]		= &pll_peri_300m_clk.hw,
		[CLK_PLL_PERI_200M]		= &pll_peri_200m_clk.hw,
		[CLK_PLL_PERI_160M]		= &pll_peri_160m_clk.hw,
		[CLK_PLL_PERI_150M]		= &pll_peri_150m_clk.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_CSI_4X]		= &pll_csi_4x_clk.common.hw,
		[CLK_PLL_AUDIO] 		= &pll_audio_clk.common.hw,
		[CLK_PLL_AUDIO_DIV2] 		= &pll_audio_div2_clk.common.hw,
		[CLK_PLL_AUDIO_DIV3] 		= &pll_audio_div3_clk.common.hw,
		[CLK_PLL_AUDIO_DIV5] 		= &pll_audio_div5_clk.common.hw,
		[CLK_PLL_IPMC]		 	= &ipmc_clk.common.hw,
		[CLK_CPU_GATIN]			= &cpu_gatin_clk.common.hw,
		[CLK_CPU]			= &cpu_clk.common.hw,
		[CLK_PIC]			= &pic_clk.common.hw,
		[CLK_CPU_CFG]			= &cpu_cfg_clk.common.hw,
		[CLK_C907_AXIMON]			= &c907_aximon_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB_UART]			= &apb_uart_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_BUS_DE]			= &de_bus_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_BUS_G2D]			= &g2d_bus_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_BUS_VE]			= &ve_bus_clk.common.hw,
		[CLK_NPU]			= &npu_clk.common.hw,
		[CLK_BUS_NPU]			= &npu_bus_clk.common.hw,
		[CLK_NDMA]			= &ndma_clk.common.hw,
		[CLK_SGDMA]			= &sgdma_clk.common.hw,
		[CLK_MSGBOX1]			= &msgbox1_clk.common.hw,
		[CLK_MSGBOX0]			= &msgbox0_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_HSTIMER]			= &hstimer_clk.common.hw,
		[CLK_AVS]			= &avs_clk.common.hw,
		[CLK_TIMER_APB]			= &timer_apb_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_IOMMU]			= &iommu_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_MBUS_NPU_GATE]		= &npu_mbus_gate_clk.common.hw,
		[CLK_MBUS_VID_IN_GATE]		= &vid_in_mbus_gate_clk.common.hw,
		[CLK_MBUS_VID_OUT_GATE]		= &vid_out_mbus_gate_clk.common.hw,
		[CLK_MBUS_CE_GATE]		= &ce_mbus_gate_clk.common.hw,
		[CLK_MBUS_VE_GATE]		= &ve_mbus_gate_clk.common.hw,
		[CLK_MBUS_SGDMA_GATE]		= &sgdma_mbus_gate_clk.common.hw,
		[CLK_MBUS_DMA_EN]		= &dma_mbus_en_clk.common.hw,
		[CLK_BUS_DRAM]			= &dram_bus_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_LPSRAMCTRL_OPI_CLK2X]		= &lpsramctrl_opi_clk2x_clk.common.hw,
		[CLK_PSRAM_CTRL]		= &psram_ctrl_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_UART1]			= &uart1_clk.common.hw,
		[CLK_UART0]			= &uart0_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_SPIF]			= &spif_clk.common.hw,
		[CLK_BUS_SPIF]			= &spif_bus_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_GMAC_25M]			= &gmac_25m_clk.common.hw,
		[CLK_GMAC_25M_CLK_SRC]		= &gmac_25m_clk_src_clk.common.hw,
		[CLK_GMAC]			= &gmac_clk.common.hw,
		[CLK_GPADC]			= &gpadc_clk.common.hw,
		[CLK_THS]			= &ths_clk.common.hw,
		[CLK_I2S0]			= &i2s0_clk.common.hw,
		[CLK_BUS_I2S0]			= &i2s0_bus_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC]		= &audio_codec_dac_clk.common.hw,
		[CLK_AUDIO_CODEC_ADC]		= &audio_codec_adc_clk.common.hw,
		[CLK_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_USBOTG0]			= &usbotg0_clk.common.hw,
		[CLK_USBEHCI0]			= &usbehci0_clk.common.hw,
		[CLK_USBOHCI0]			= &usbohci0_clk.common.hw,
		[CLK_DPSS_TOP]			= &dpss_top_clk.common.hw,
		[CLK_TCONLCD]			= &tconlcd_clk.common.hw,
		[CLK_BUS_TCONLCD]		= &tconlcd_bus_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_BUS_CSI]			= &csi_bus_clk.common.hw,
		[CLK_E907_GATING_RS]		= &e907_gating_rs_clk.common.hw,
		[CLK_E907]			= &e907_clk.common.hw,
		[CLK_RISCV_CFG]			= &riscv_cfg_clk.common.hw,
		[CLK_E907_AXIMON]			= &e907_aximon_clk.common.hw,
		[CLK_CPUS_HCLK_GATE]		= &cpus_hclk_gate_clk.common.hw,
		[CLK_MBUS_GMAC_AHB_GATE]		= &gmac_mbus_ahb_gate_clk.common.hw,
		[CLK_MBUS_SMHC1_AHB_GATE]		= &smhc1_mbus_ahb_gate_clk.common.hw,
		[CLK_MBUS_SMHC0_AHB_GATE]		= &smhc0_mbus_ahb_gate_clk.common.hw,
		[CLK_MBUS_USB_AHB_GATE]		= &usb_mbus_ahb_gate_clk.common.hw,
		[CLK_GMAC_AHB_GATE]		= &gmac_ahb_gate_clk.common.hw,
		[CLK_SMHC1_AHB_GATE]		= &smhc1_ahb_gate_clk.common.hw,
		[CLK_SMHC0_AHB_GATE]		= &smhc0_ahb_gate_clk.common.hw,
		[CLK_USB_AHB_GATE]		= &usb_ahb_gate_clk.common.hw,
		[CLK_VID_OUT_AHB_GATE]		= &vid_out_ahb_gate_clk.common.hw,
		[CLK_VID_IN_AHB_GATE]		= &vid_in_ahb_gate_clk.common.hw,
		[CLK_VE_AHB_GATE]		= &ve_ahb_gate_clk.common.hw,
		[CLK_RES_DCAP_24M]		= &res_dcap_24m_clk.common.hw,
		[CLK_GPADC_24M]			= &gpadc_24m_clk.common.hw,
		[CLK_USB_24M]			= &usb_24m_clk.common.hw,
		[CLK_PLL_OUTPUT_GATE]		= &pll_output_gate_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun252iw1_ccu_clks[] = {
	&pll_cpu_clk.common,
	&pll_peri_parent_clk.common,
	&pll_peri_2x_clk.common,
	&pll_peri_800m_clk.common,
	&pll_peri_480m_clk.common,
	&pll_ddr_clk.common,
	&pll_video0_4x_clk.common,
	&pll_csi_4x_clk.common,
	&pll_audio_clk.common,
	&pll_audio_div2_clk.common,
	&pll_audio_div3_clk.common,
	&pll_audio_div5_clk.common,
	&ipmc_clk.common,
	&cpu_gatin_clk.common,
	&cpu_clk.common,
	&pic_clk.common,
	&cpu_cfg_clk.common,
	&c907_aximon_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&apb_uart_clk.common,
	&de_clk.common,
	&de_bus_clk.common,
	&g2d_clk.common,
	&g2d_bus_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&ce_bus_clk.common,
	&ve_clk.common,
	&ve_bus_clk.common,
	&npu_clk.common,
	&npu_bus_clk.common,
	&ndma_clk.common,
	&sgdma_clk.common,
	&msgbox1_clk.common,
	&msgbox0_clk.common,
	&spinlock_clk.common,
	&hstimer_clk.common,
	&avs_clk.common,
	&timer_apb_clk.common,
	&dbgsys_clk.common,
	&pwm_clk.common,
	&iommu_clk.common,
	&dram_clk.common,
	&npu_mbus_gate_clk.common,
	&vid_in_mbus_gate_clk.common,
	&vid_out_mbus_gate_clk.common,
	&ce_mbus_gate_clk.common,
	&ve_mbus_gate_clk.common,
	&sgdma_mbus_gate_clk.common,
	&dma_mbus_en_clk.common,
	&dram_bus_clk.common,
	&smhc0_clk.common,
	&smhc1_clk.common,
	&smhc1_bus_clk.common,
	&smhc0_bus_clk.common,
	&lpsramctrl_opi_clk2x_clk.common,
	&psram_ctrl_clk.common,
	&uart3_clk.common,
	&uart2_clk.common,
	&uart1_clk.common,
	&uart0_clk.common,
	&twi4_clk.common,
	&twi3_clk.common,
	&twi2_clk.common,
	&twi1_clk.common,
	&twi0_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spif_clk.common,
	&spif_bus_clk.common,
	&spi1_bus_clk.common,
	&spi0_bus_clk.common,
	&gmac_25m_clk.common,
	&gmac_25m_clk_src_clk.common,
	&gmac_clk.common,
	&gpadc_clk.common,
	&ths_clk.common,
	&i2s0_clk.common,
	&i2s0_bus_clk.common,
	&audio_codec_dac_clk.common,
	&audio_codec_adc_clk.common,
	&audio_codec_clk.common,
	&usb_clk.common,
	&usbotg0_clk.common,
	&usbehci0_clk.common,
	&usbohci0_clk.common,
	&dpss_top_clk.common,
	&tconlcd_clk.common,
	&tconlcd_bus_clk.common,
	&csi_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_bus_clk.common,
	&e907_gating_rs_clk.common,
	&e907_clk.common,
	&riscv_cfg_clk.common,
	&e907_aximon_clk.common,
	&cpus_hclk_gate_clk.common,
	&gmac_mbus_ahb_gate_clk.common,
	&smhc1_mbus_ahb_gate_clk.common,
	&smhc0_mbus_ahb_gate_clk.common,
	&usb_mbus_ahb_gate_clk.common,
	&gmac_ahb_gate_clk.common,
	&smhc1_ahb_gate_clk.common,
	&smhc0_ahb_gate_clk.common,
	&usb_ahb_gate_clk.common,
	&vid_out_ahb_gate_clk.common,
	&vid_in_ahb_gate_clk.common,
	&ve_ahb_gate_clk.common,
	&res_dcap_24m_clk.common,
	&gpadc_24m_clk.common,
	&usb_24m_clk.common,
	&pll_output_gate_clk.common,
};

static const struct sunxi_ccu_desc sun252iw1_ccu_desc = {
	.ccu_clks	= sun252iw1_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun252iw1_ccu_clks),

	.hw_clks	= &sun252iw1_hw_clks,

	.resets		= sun252iw1_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun252iw1_ccu_resets),
};

static const u32 pll_regs[] = {
	SUN252IW1_PLL_DDR_CTRL_REG,
	SUN252IW1_PLL_PERI_CTRL_REG,
	SUN252IW1_PLL_VIDEO_CTRL_REG,
	SUN252IW1_PLL_CSI_CTRL_REG,
	SUN252IW1_PLL_AUDIO_CTRL_REG,
};

static int sun252iw1_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun252iw1_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun252iw1_ccu_clks,
			ARRAY_SIZE(sun252iw1_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun252iw1_ccu_ids[] = {
	{ .compatible = "allwinner,sun252iw1-ccu" },
	{ }
};

static struct platform_driver sun252iw1_ccu_driver = {
	.probe	= sun252iw1_ccu_probe,
	.driver	= {
		.name	= "sun252iw1-ccu",
		.of_match_table	= sun252iw1_ccu_ids,
	},
};

static int __init sunxi_ccu_sun252iw1_init(void)
{
	int ret;

	ret = platform_driver_register(&sun252iw1_ccu_driver);
	if (ret)
		pr_err("register ccu sun252iw1 failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun252iw1_init);

static void __exit sunxi_ccu_sun252iw1_exit(void)
{
	return platform_driver_unregister(&sun252iw1_ccu_driver);
}
module_exit(sunxi_ccu_sun252iw1_exit);

MODULE_VERSION("0.0.4");
MODULE_AUTHOR("weizhouxiang<weizhouxiang@allwinnertech.com>");
