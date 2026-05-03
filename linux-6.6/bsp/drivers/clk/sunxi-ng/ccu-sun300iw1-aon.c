// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>

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

#include "ccu-sun300iw1-aon.h"
/* ccu_des_start */

#define SUN300IW1_PLL_CPU_CTRL_REG   0x0000
static struct ccu_nm pll_cpu_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 8, 27),
	.min_rate	= 320000000,
	.max_rate	= 1080000000,
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-cpu", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL),
	},
};

#define SUN300IW1_PLL_PERI_CTRL_REG   0x0020
#define SUN300IW1_PLL_PERI_CTRL1_REG  0x0024
static struct ccu_nm pll_peri_parent_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	/* VCO = HOSC * N * 2 / M = 3072M */
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 84, 134),
	.min_rate	= 2016000000,
	.max_rate	= 3216000000U,
	.sdm		= _SUNXI_CCU_SDM_INFO(BIT(24), 0x120),
	.fixed_post_div = 5,
	.fixed_pre_div = 2,
	.common		= {
		.reg		= 0x0020,
		.features       = CCU_FEATURE_FIXED_POSTDIV | CCU_FEATURE_FIXED_PREDIV,
		.hw.init	= CLK_HW_INIT("pll-peri-parent", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE | CLK_IGNORE_UNUSED),
	},
};

static CLK_FIXED_FACTOR_HW(pll_peri_cko_1536_clk, "pll-peri-cko-1536m",
		&pll_peri_parent_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_1024_clk, "pll-peri-cko-1024m",
		&pll_peri_parent_clk.common.hw,
		3, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_768_clk, "pll-peri-cko-768m",
		&pll_peri_parent_clk.common.hw,
		4, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_614_clk, "pll-peri-cko-614m",
		&pll_peri_parent_clk.common.hw,
		5, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_512_clk, "pll-peri-cko-512m",
		&pll_peri_parent_clk.common.hw,
		6, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_384_clk, "pll-peri-cko-384m",
		&pll_peri_parent_clk.common.hw,
		8, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_341_clk, "pll-peri-cko-341m",
		&pll_peri_parent_clk.common.hw,
		9, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_307_clk, "pll-peri-cko-307m",
		&pll_peri_parent_clk.common.hw,
		10, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_236_clk, "pll-peri-cko-236m",
		&pll_peri_parent_clk.common.hw,
		13, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_219_clk, "pll-peri-cko-219m",
		&pll_peri_parent_clk.common.hw,
		14, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_192_clk, "pll-peri-cko-192m",
		&pll_peri_parent_clk.common.hw,
		16, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_118_clk, "pll-peri-cko-118m",
		&pll_peri_parent_clk.common.hw,
		26, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_96_clk, "pll-peri-cko-96m",
		&pll_peri_parent_clk.common.hw,
		32, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_48_clk, "pll-peri-cko-48m",
		&pll_peri_parent_clk.common.hw,
		64, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_24_clk, "pll-peri-cko-24m",
		&pll_peri_parent_clk.common.hw,
		128, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri_cko_12_clk, "pll-peri-cko-12m",
		&pll_peri_parent_clk.common.hw,
		256, 1, 0);

#define SUN300IW1_PLL_VIDEO0_CTRL_REG   0x0040
static struct ccu_nm pll_video_4x_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 13, 37),
	.min_rate	= 520000000,
	.max_rate	= 1480000000,
	.sdm            = _SUNXI_CCU_SDM_INFO(BIT(24), 0x140),
	.common		= {
		.reg		= 0x0040,
		.hw.init	= CLK_HW_INIT("pll-video-4x", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE | CLK_IGNORE_UNUSED),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video_2x_clk, "pll-video-2x",
		&pll_video_4x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_video_1x_clk, "pll-video-1x",
		&pll_video_4x_clk.common.hw,
		4, 1, 0);

static struct clk_div_table pll_csi_div_table[] = {
    { .val = 0, .div = 1 },
    { .val = 1, .div = 2 },
    { .val = 2, .div = 4 },
    { /* Sentinel */ },
};

static struct ccu_sdm_setting pll_csi_4x_sdm_table[] = {
	{ .rate = 675000000, .pattern = 0xc0010000, .m = 4, .n = 67 },
};

#define SUN300IW1_PLL_CSI_CTRL_REG   0x0048
static struct ccu_nm pll_csi_4x_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 13, 37),
	.m              = _SUNXI_CCU_DIV_TABLE(1, 2, pll_csi_div_table), /* input_div */
	.min_rate	= 520000000,
	.max_rate	= 1480000000,
	.sdm		= _SUNXI_CCU_SDM_COMPLETE(pll_csi_4x_sdm_table, BIT(24),
			0x0148, 0x0,  	  /* pattern0 */
			0x014c, BIT(31)), /* pattern1 sig_delt_pat_en */
	.common		= {
		.reg		= 0x0048,
		.features	= CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init	= CLK_HW_INIT("pll-csi-4x", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE | CLK_IGNORE_UNUSED),
	},
};

static CLK_FIXED_FACTOR_HW(pll_csi_2x_clk, "pll-csi-2x",
		&pll_csi_4x_clk.common.hw,
		2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_csi_1x_clk, "pll-csi-1x",
		&pll_csi_4x_clk.common.hw,
		4, 1, 0);

#define SUN300IW1_PLL_DDR_CTRL_REG   0x0080
static struct ccu_nm pll_ddr_clk = {
	.output		= BIT(27),
	.lock		= BIT(28),
	.lock_enable	= BIT(29),
	.ldo_en		= BIT(30),
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN_MAX(8, 8, 13, 71),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.min_rate	= 312000000,
	.max_rate	= 1704000000,
	.sdm            = _SUNXI_CCU_SDM_INFO(BIT(24), 0x180),
	.common		= {
		.reg		= 0x0080,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE | CLK_IGNORE_UNUSED),
	},
};

#define SUN300IW1_DCXO_STATUS_REG   0x0404
static const char * const dcxo_parents[] = { "dcxo40M", "dcxo24M" };

static SUNXI_CCU_MUX(dcxo_clk, "dcxo", dcxo_parents,
		0x0404, 31, 1, 0);

static const char * const ahb_parents[] = { "dcxo", "pll-peri-cko-768m", "rc-1m" };

static SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", ahb_parents,
		0x0500, 0, 5, 24, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const apb_parents[] = { "dcxo", "pll-peri-cko-384m", "rc-1m" };

static SUNXI_CCU_M_WITH_MUX(apb_clk, "apb", apb_parents,
		0x0504, 0, 5, 24, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const rtc_apb_parents[] = { "rc-1m", "pll-peri-cko-96m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX(rtc_apb_clk, "rtc-apb", rtc_apb_parents,
		0x0508, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(pwrctrl_bus_clk, "pwrctrl",
		"dcxo",
		0x0550, BIT(6), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(tccal_bus_clk, "trral",
		"dcxo",
		0x0550, BIT(2), 0);

static const char * const apb_spc_clk_parents[] = { "dcxo", "", "rc-1m", "pll-peri-cko-192m" };

static SUNXI_CCU_M_WITH_MUX(apb_spc_clk, "apb-spc", apb_spc_clk_parents,
		0x0580, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const e907_clk_parents[] = { "dcxo", "pll-video-2x", "rc-1m", "rc-1m", "pll-cpu", "pll-peri-cko-1024m", "pll-peri-cko-614m", "pll-peri-cko-614m" };

static SUNXI_CCU_M_WITH_MUX(e907_clk, "e907", e907_clk_parents,
		0x0584, 0, 5, 24, 3, CLK_SET_RATE_NO_REPARENT);

static const char * const a27l2_clk_parents[] = { "dcxo", "", "rc-1m", "rc-1m", "pll-cpu", "pll-peri-cko-1024m", "pll-peri-cko-768m", "pll-peri-cko-768m" };
static SUNXI_CCU_M_WITH_MUX_GATE(a27l2_clk, "a27l2",
			a27l2_clk_parents, 0x0588,
			0, 5,   /* M */
			24, 3,  /* mux */
			BIT(31), CLK_IGNORE_UNUSED | CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT);

static struct ccu_nm pll_peri1_clk = {
	.common         = {
		.reg            = 0x24,
	},
};

#ifdef CONFIG_PM_SLEEP
static struct ccu_nm pll_csi_pat0_clk = {
	.common         = {
		.reg            = 0x148,
	},
};

static struct ccu_nm pll_csi_pat1_clk = {
	.common         = {
		.reg            = 0x14c,
	},
};
#endif
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun300iw1_aon_ccu_resets[] = {
	[RST_BUS_WLAN]			= { 0x0518, BIT(0) },
	/* rst_def_end */
};

/* ccu_def_start */
static struct clk_hw_onecell_data sun300iw1_aon_hw_clks = {
	.hws    = {
		[CLK_PLL_CPU]			= &pll_cpu_clk.common.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI_PARENT]		= &pll_peri_parent_clk.common.hw,
		[CLK_PLL_PERI_CKO_1536M]	= &pll_peri_cko_1536_clk.hw,
		[CLK_PLL_PERI_CKO_1024M]	= &pll_peri_cko_1024_clk.hw,
		[CLK_PLL_PERI_CKO_768M]		= &pll_peri_cko_768_clk.hw,
		[CLK_PLL_PERI_CKO_614M]		= &pll_peri_cko_614_clk.hw,
		[CLK_PLL_PERI_CKO_512M]		= &pll_peri_cko_512_clk.hw,
		[CLK_PLL_PERI_CKO_384M]		= &pll_peri_cko_384_clk.hw,
		[CLK_PLL_PERI_CKO_341M]		= &pll_peri_cko_341_clk.hw,
		[CLK_PLL_PERI_CKO_307M]		= &pll_peri_cko_307_clk.hw,
		[CLK_PLL_PERI_CKO_236M]		= &pll_peri_cko_236_clk.hw,
		[CLK_PLL_PERI_CKO_219M]		= &pll_peri_cko_219_clk.hw,
		[CLK_PLL_PERI_CKO_192M]		= &pll_peri_cko_192_clk.hw,
		[CLK_PLL_PERI_CKO_118M]		= &pll_peri_cko_118_clk.hw,
		[CLK_PLL_PERI_CKO_96M]		= &pll_peri_cko_96_clk.hw,
		[CLK_PLL_PERI_CKO_48M]		= &pll_peri_cko_48_clk.hw,
		[CLK_PLL_PERI_CKO_24M]		= &pll_peri_cko_24_clk.hw,
		[CLK_PLL_PERI_CKO_12M]		= &pll_peri_cko_12_clk.hw,
		[CLK_PLL_VIDEO_4X]		= &pll_video_4x_clk.common.hw,
		[CLK_PLL_VIDEO_2X]              = &pll_video_2x_clk.hw,
		[CLK_PLL_VIDEO_1X]		= &pll_video_1x_clk.hw,
		[CLK_PLL_CSI_4X]		= &pll_csi_4x_clk.common.hw,
		[CLK_PLL_CSI_2X]                = &pll_csi_2x_clk.hw,
		[CLK_PLL_CSI_1X]		= &pll_csi_1x_clk.hw,
		[CLK_DCXO]			= &dcxo_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb_clk.common.hw,
		[CLK_RTC_APB]			= &rtc_apb_clk.common.hw,
		[CLK_PWRCTRL]			= &pwrctrl_bus_clk.common.hw,
		[CLK_TCCAL]			= &tccal_bus_clk.common.hw,
		[CLK_APB_SPC]			= &apb_spc_clk.common.hw,
		[CLK_E907]			= &e907_clk.common.hw,
		[CLK_A27L2]			= &a27l2_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun300iw1_aon_ccu_clks[] = {
	&pll_cpu_clk.common,
	&pll_ddr_clk.common,
	&pll_peri_parent_clk.common,
	&pll_video_4x_clk.common,
	&pll_csi_4x_clk.common,
	&dcxo_clk.common,
	&ahb_clk.common,
	&apb_clk.common,
	&rtc_apb_clk.common,
	&pwrctrl_bus_clk.common,
	&tccal_bus_clk.common,
	&apb_spc_clk.common,
	&e907_clk.common,
	&a27l2_clk.common,
#ifdef CONFIG_PM_SLEEP
	&pll_peri1_clk.common,
	&pll_csi_pat0_clk.common,
	&pll_csi_pat1_clk.common,
	/* We don't need repeat restore wlan_bus_rstn(0x518). which would be used in e907 */
#endif
};

static const struct sunxi_ccu_desc sun300iw1_aon_ccu_desc = {
	.ccu_clks	= sun300iw1_aon_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun300iw1_aon_ccu_clks),

	.hw_clks	= &sun300iw1_aon_hw_clks,

	.resets		= sun300iw1_aon_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun300iw1_aon_ccu_resets),
};

static struct ccu_pll_nb sun300iw1_pll_cpu_nb = {
	.common = &pll_cpu_clk.common,
	/* copy from pll_cpux_clk */
	.enable = BIT(27),
	.lock   = BIT(28),
};

static struct ccu_mux_nb sun300iw1_a27_cpu_nb = {
	.common         = &a27l2_clk.common,
	.cm             = &a27l2_clk.mux,
	.delay_us       = 1,
	.bypass_index   = 6, /* index of pll peri-cko-768m */
};

static struct ccu_mux_nb sun300iw1_e907_cpu_nb = {
	.common         = &e907_clk.common,
	.cm             = &e907_clk.mux,
	.delay_us       = 1,
	.bypass_index   = 6, /* index of pll peri-cko-614m */
};

static void sun300iw1_pll_vco_range_self_adaption(unsigned int dcxo)
{
	if (dcxo == 24000000) {
		pll_cpu_clk.n.min = 14;
		pll_cpu_clk.min_rate = 336000000;
		pll_cpu_clk.n.max = 45;
		pll_cpu_clk.max_rate = 1080000000;

		pll_peri_parent_clk.fixed_post_div = 3;

		pll_video_4x_clk.n.min = 21;
		pll_video_4x_clk.min_rate = 504000000;
		pll_video_4x_clk.n.max = 62;
		pll_video_4x_clk.max_rate = 1488000000;

		pll_csi_4x_sdm_table[0].pattern = 0xc0008000;
		pll_csi_4x_sdm_table[0].m = 2;
		pll_csi_4x_sdm_table[0].n = 56;
		pll_csi_4x_clk.n.min = 21;
		pll_csi_4x_clk.min_rate = 504000000;
		pll_csi_4x_clk.n.max = 62;
		pll_csi_4x_clk.max_rate = 1488000000;

		return;
	} else if (dcxo == 40000000) {
		return;
	} else {
		sunxi_err(NULL, "hosc rate:%d is error! plead check!", dcxo);
		return;
	}
}

static int sun300iw1_aon_ccu_really_probe(struct device_node *node)
{
	void __iomem *reg;
	int ret, dcxo_rate;
	unsigned long dcxo_flag;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	/* TODO:
	 * Each pll-peri-cko has an independent enable switch, which is temporarily turned on to reduce power consumption.
	 * It will be placed in the definition of ccu later.
	 */
	set_reg(reg + SUN300IW1_PLL_PERI_CTRL1_REG, 0xffff, 16, 0);

	ret = sunxi_parse_sdm_info(node);
	if (ret)
		pr_debug("%s: sdm_info not enabled", __func__);

	dcxo_flag = test_field_set((reg + SUN300IW1_DCXO_STATUS_REG), BIT(31));
	if (dcxo_flag)
		dcxo_rate = 24000000;
	else
		dcxo_rate = 40000000;

	sunxi_info(NULL, "Current HOSC rate is %uHZ\n", dcxo_rate);

	sun300iw1_pll_vco_range_self_adaption(dcxo_rate);

	ret = sunxi_ccu_probe(node, reg, &sun300iw1_aon_ccu_desc);
	if (ret)
		return ret;

	/* Gate then ungate PLL CPU after any rate changes */
	ccu_pll_notifier_register(&sun300iw1_pll_cpu_nb);

	/* a27:Reparent CPU during PLL CPU rate changes */
	ccu_mux_notifier_register(pll_cpu_clk.common.hw.clk,
				  &sun300iw1_a27_cpu_nb);

	/* e907:Reparent CPU during PLL CPU rate changes */
	ccu_mux_notifier_register(pll_cpu_clk.common.hw.clk,
				  &sun300iw1_e907_cpu_nb);

	sunxi_ccu_sleep_init(reg, sun300iw1_aon_ccu_clks,
			ARRAY_SIZE(sun300iw1_aon_ccu_clks),
			NULL, 0);

	return 0;
}

#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
static void __init of_sun300iw1_aon_ccu_init(struct device_node *node)
{
	int ret;

	ret = sun300iw1_aon_ccu_really_probe(node);
	if (ret)
		sunxi_err(NULL, "sun300iw1_aon_ccu probe failed\n");
}

CLK_OF_DECLARE(sun300iw1_aon_ccu_init, "allwinner,sun300iw1-aon-ccu", of_sun300iw1_aon_ccu_init);
#else
static int sun300iw1_aon_ccu_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *node = pdev->dev.of_node;

	ret = sun300iw1_aon_ccu_really_probe(node);
	if (ret)
		sunxi_err(NULL, "sun300iw1_aon_ccu probe failed\n");

	return ret;
}

static const struct of_device_id sun300iw1_aon_ccu_ids[] = {
	{ .compatible = "allwinner,sun300iw1-aon-ccu" },
	{ }
};

static struct platform_driver sun300iw1_aon_ccu_driver = {
	.probe	= sun300iw1_aon_ccu_probe,
	.driver	= {
		.name	= "sun300iw1-aon-ccu",
		.of_match_table	= sun300iw1_aon_ccu_ids,
	},
};

static int __init sun300iw1_aon_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun300iw1_aon_ccu_driver);
	if (err)
		sunxi_err(NULL, "register ccu sun300iw1_aon failed\n");

	return err;
}

core_initcall(sun300iw1_aon_ccu_init);

static void __exit sun300iw1_aon_ccu_exit(void)
{
	platform_driver_unregister(&sun300iw1_aon_ccu_driver);
}
module_exit(sun300iw1_aon_ccu_exit);
#endif

MODULE_DESCRIPTION("Allwinner sun300iw1_aon clk driver");
MODULE_AUTHOR("rgm<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2.9");
