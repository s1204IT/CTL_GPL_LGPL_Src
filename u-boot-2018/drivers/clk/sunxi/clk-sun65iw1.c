/*
 * Copyright (C) 2013 Allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "clk-sun65iw1.h"
#include "clk-sun65iw1_tbl.c"
#include "clk_factor.h"
#include "clk_periph.h"
#include <linux/compat.h>
#include <fdt_support.h>
#include <div64.h>

#define FACTOR_SIZEOF(name) (sizeof(factor_pll##name##_tbl)/ \
		sizeof(struct sunxi_clk_factor_freq))

#define FACTOR_SEARCH(name) (sunxi_clk_com_ftr_sr( \
			&sunxi_clk_factor_pll_##name, factor, \
			factor_pll##name##_tbl, index, \
			FACTOR_SIZEOF(name)))

#ifndef CONFIG_EVB_PLATFORM
#define LOCKBIT(x) 31
#else
#define LOCKBIT(x) x
#endif

DEFINE_SPINLOCK(clk_lock);
void __iomem *sunxi_clk_base;
void __iomem *sunxi_clk_cpus_base;
#define REG_ADDR(x) (sunxi_clk_base + x)

/*                                	   	   ns  nw  ks kw   ms  mw  ps  pw  d1s d1w d2s d2w {frac   out	mode}	en-s    sdmss   sdmsw   sdmpat  sdmval*/
SUNXI_CLK_FACTORS(pll_periph0_2x,          	   8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,   0,    0,   0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_periph0_800m,        	   8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,   0,    0,   0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_periph0_480m,        	   8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  2,   3,   0,    0,   0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_video0x4,		           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,   0,    0,   0,      31,     0,      0,      0,	0);
SUNXI_CLK_FACTORS(pll_video0x3,		           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,   0,    0,   0,      31,     0,      0,      0,	0);
SUNXI_CLK_FACTORS(pll_video1x4,   	           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,   0,    0,   0,      31,     0,      0,      0, 	0);
SUNXI_CLK_FACTORS(pll_video1x3,   	           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,   0,    0,   0,      31,     0,      0,      0, 	0);
SUNXI_CLK_FACTORS(pll_video2x4,		           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,   0,    0,   0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_video2x3,		           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,   0,    0,   0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_ve,		           8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,   0,    0,   0,      31,     0,      0,      0,      0);


static int get_factors_pll_periph0_2x(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph0_2x_max ? pllperiph0_2x_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph0_2x))
		return -1;

	return 0;
}

static int get_factors_pll_periph0_800m(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph0_800m_max ? pllperiph0_800m_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph0_800m))
		return -1;

	return 0;
}

static int get_factors_pll_periph0_480m(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph0_480m_max ? pllperiph0_480m_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph0_480m))
		return -1;

	return 0;
}

/* pll_video0x4/pll_video1x4: 24*N/D2 */
static unsigned long calc_rate_video0(u32 parent_rate,
		struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);
	tmp_rate = tmp_rate * (factor->factorn + 1);
	do_div(tmp_rate, factor->factord2 + 1);
	return (unsigned long)tmp_rate;
}

static int get_factors_pll_video(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	u8 min_n = 53, max_n = 105;
	u8 min_d2 = 0, max_d2 = 1;
	u8 best_n = 0, best_d2 = 0;
	u64 best_rate = 0;

	for (factor->factorn = min_n; factor->factorn <= max_n; factor->factorn++) {
		for (factor->factord2 = min_d2; factor->factord2 <= max_d2; factor->factord2++) {
			u64 tmp_rate;

			tmp_rate = calc_rate_video0(parent_rate, factor);

			if (tmp_rate > rate)
				continue;

			if ((rate - tmp_rate) < (rate - best_rate)) {
				best_rate = tmp_rate;
				best_n = factor->factorn;
				best_d2 = factor->factord2;
			}
		}
	}

	factor->factorn = best_n;
	factor->factord2 = best_d2;

	return 0;
}

static unsigned long calc_rate_pll_periph(u32 parent_rate,
		struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);

	factor->factorn = factor->factorn >= 0xff ? 0 : factor->factorn;
	factor->factorm = factor->factorm >= 0xff ? 0 : factor->factorm;
	factor->factork = factor->factork >= 0xff ? 0 : factor->factork;
	factor->factorp = factor->factorp >= 0xff ? 0 : factor->factorp;
	factor->factord1 = factor->factord1 >= 0xff ? 0 : factor->factord1;
	factor->factord2 = factor->factord2 >= 0xff ? 0 : factor->factord2;

	tmp_rate = tmp_rate * (factor->factorn + 1);
	do_div(tmp_rate, (factor->factord1 + 1) * (factor->factord2 + 1));

	return (unsigned long)tmp_rate;
}

static const char *hosc_parents[] = {"hosc"};
struct factor_init_data sunxi_factos[] = {
	/* name			parent		parent_num,	flags			reg			lock_reg	lock_bit	pll_lock_ctrl_reg	lock_en_bit		lock_mode			config					get_factors			calc_rate			priv_ops*/
	{"pll_periph0_2x",	hosc_parents,	1,		0,			PLL_PERIPH0,		PLL_PERIPH0,	LOCKBIT(28),	PLL_PERIPH0,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_periph0_2x,	&get_factors_pll_periph0_2x,	&calc_rate_pll_periph,		(struct clk_ops *)NULL},
	{"pll_periph0_800m",	hosc_parents,	1,		0,			PLL_PERIPH0,		PLL_PERIPH0,	LOCKBIT(28),	PLL_PERIPH0,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_periph0_800m,	&get_factors_pll_periph0_800m,	&calc_rate_pll_periph,		(struct clk_ops *)NULL},
	{"pll_periph0_480m",	hosc_parents,	1,		0,			PLL_PERIPH0,		PLL_PERIPH0,	LOCKBIT(28),	PLL_PERIPH0,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_periph0_480m,	&get_factors_pll_periph0_480m,	&calc_rate_pll_periph,		(struct clk_ops *)NULL},
	{"pll_video0x4",	hosc_parents,	1,		CLK_NO_DISABLE, 	PLL_VIDEO0,		PLL_VIDEO0,	LOCKBIT(28),	PLL_VIDEO0,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_video0x4,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
	{"pll_video0x3",	hosc_parents,	1,		CLK_NO_DISABLE,		PLL_VIDEO0,		PLL_VIDEO0,	LOCKBIT(28),	PLL_VIDEO0,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_video0x3,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
	{"pll_video1x4",	hosc_parents,	1,		CLK_NO_DISABLE,		PLL_VIDEO1,		PLL_VIDEO1,	LOCKBIT(28),	PLL_VIDEO1,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_video1x4,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
	{"pll_video1x3",	hosc_parents,	1,		CLK_NO_DISABLE,		PLL_VIDEO1,		PLL_VIDEO1,	LOCKBIT(28),	PLL_VIDEO1,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_video1x3,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
	{"pll_video2x4",	hosc_parents,	1,		CLK_NO_DISABLE,		PLL_VIDEO2,		PLL_VIDEO2,	LOCKBIT(28),	PLL_VIDEO2,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_video2x4,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
	{"pll_video2x2",	hosc_parents,	1,		CLK_NO_DISABLE,		PLL_VIDEO2,		PLL_VIDEO2,	LOCKBIT(28),	PLL_VIDEO2,		29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_video2x3,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
	{"pll_ve",		hosc_parents,	1,		CLK_NO_DISABLE,		PLL_VE,			PLL_VE,		LOCKBIT(28),	PLL_VE,			29,			PLL_LOCK_NEW_MODE,		&sunxi_clk_factor_pll_ve,		&get_factors_pll_video,		&calc_rate_video0,		(struct clk_ops *)NULL},
};

static const char *de_parents[] = {"pll_periph0_600m", "pll_periph0_480m", "pll_periph0_400m", "pll_ve"};
static const char *tcon_lcd0_parents[] = { "pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x", "pll_video0x3", "pll_video1x3", "pll_video2x3" };
static const char *mipi_dsi0_parents[] = { "hosc", "pll_periph0_200m", "pll_periph0_150m" };
static const char *combophy0_parents[] = { "pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x", "pll_video0x3", "pll_video1x3", "pll_video2x3" };
static const char *tcon_tv0_parents[] = { "pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_300m", "pll_video0x3", "pll_video1x3", "pll_video2x3" };

/*
   SUNXI_CLK_PERIPH(name,                 mux_reg,         mux_sft, mux_wid,      div_reg,       div_msft,  div_mwid,	 div_nsft,   div_nwid,	gate_flag,    en_reg,	     rst_reg,		 bus_gate_reg,	drm_gate_reg,	en_sft,	 rst_sft,    bus_gate_sft,    dram_gate_sft,   lock,	   com_gate,	com_gate_off)
 */
SUNXI_CLK_PERIPH(de,                      DE_CFG,          24,      3,            DE_CFG,        0,         5,            0,          0,         0,           DE_CFG,        DE_AHB,         DE_AHB,        0,              31,      16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(de_sys_gate,             0,               0,       0,            0,             0,         0,            0,          0,         0,           MBUS_MASTER,   0,              0,             0,              5,       0,          0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(tcon_lcd0,               TCON_LCD0_CFG,   24,      3,            TCON_LCD0_CFG, 0,         5,            0,          0,         0,           TCON_LCD0_CFG, TCON_LCD0_AHB,  TCON_LCD0_AHB, 0,              31,      16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(tcon_tv0,                TCON_TV0_CFG,    24,      3,            TCON_TV0_CFG,  0,         5,            0,          0,         0,           TCON_TV0_CFG,  TCON_TV0_AHB,   TCON_TV0_AHB,  0,              31,      16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(mipi_dsi_combphy0,       COMBPHY0_CFG,    24,      3,            COMBPHY0_CFG,  0,         5,            0,          0,         0,           COMBPHY0_CFG,  COMBPHY0_AHB,   COMBPHY0_AHB,  0,              31,      16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(combophy0,  		  COMBOPHY0,       24,      3,            COMBOPHY0,     0,         5,            0,          0,         0,           COMBOPHY0,     0,              0,	            0,              31,      0,          0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(lvds0,  		  0,               0,       0,            0,             0,         0,            0,          0,         0,           0,             RST_BUS_LVDS0,  0,	            0,              0,       16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(vo0_ahb,  		  0,               0,       0,            0,             0,         0,            0,          0,         0,           0,             VO0_AHB,        VO0_AHB,	    0,              0,       16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(vo1_ahb,  		  0,               0,       0,            0,             0,         0,            0,          0,         0,           0,             VO1_AHB,        VO1_AHB,	    0,              0,       16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(video_out0,  		  0,               0,       0,            0,             0,         0,            0,          0,         0,           0,             RST_VIDEO_OUT0, 0,	            0,              0,       16,         0,               0,               &clk_lock,  NULL,        0);
SUNXI_CLK_PERIPH(edp,  			  0,               0,       0,            0,             0,         0,            0,          0,         0,           0,             EDP,            EDP,	    0,              0,       16,         0,               0,               &clk_lock,  NULL,        0);


struct periph_init_data sunxi_periphs_init[] = {
	{"de",                CLK_SET_RATE_PARENT,   de_parents,        ARRAY_SIZE(de_parents),             &sunxi_clk_periph_de},
	{"de_sys_gate",       0,                     hosc_parents,      ARRAY_SIZE(hosc_parents),           &sunxi_clk_periph_de_sys_gate},
	{"tcon_lcd0",         CLK_SET_RATE_PARENT,   tcon_lcd0_parents, ARRAY_SIZE(tcon_lcd0_parents),      &sunxi_clk_periph_tcon_lcd0},
	{"tcon_tv0",          CLK_SET_RATE_PARENT,   tcon_tv0_parents,  ARRAY_SIZE(tcon_tv0_parents),       &sunxi_clk_periph_tcon_tv0},
	{"mipi_dsi_combphy0", CLK_SET_RATE_PARENT,   mipi_dsi0_parents, ARRAY_SIZE(mipi_dsi0_parents),      &sunxi_clk_periph_mipi_dsi_combphy0},
	{"combophy0",         CLK_SET_RATE_PARENT,   combophy0_parents, ARRAY_SIZE(combophy0_parents),      &sunxi_clk_periph_combophy0},
	{"lvds0",             0,                     hosc_parents,      ARRAY_SIZE(hosc_parents),           &sunxi_clk_periph_lvds0},
	{"vo0_ahb",           0,                     hosc_parents,      ARRAY_SIZE(hosc_parents),           &sunxi_clk_periph_vo0_ahb},
	{"vo1_ahb",           0,                     hosc_parents,      ARRAY_SIZE(hosc_parents),           &sunxi_clk_periph_vo1_ahb},
	{"video_out0",        0,                     hosc_parents,      ARRAY_SIZE(hosc_parents),           &sunxi_clk_periph_video_out0},
	{"edp",               0,                     hosc_parents,      ARRAY_SIZE(hosc_parents),           &sunxi_clk_periph_edp},
};

/*
 * sunxi_clk_get_factor_by_name() - Get factor clk init config
 */
struct factor_init_data *sunxi_clk_get_factor_by_name(const char *name)
{
	struct factor_init_data *factor;
	int i;

	/* get pll clk init config */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {
		factor = &sunxi_factos[i];
		if (strcmp(name, factor->name))
			continue;
		return factor;
	}

	return NULL;
}

/*
 * sunxi_clk_get_periph_by_name() - Get periph clk init config
 */
struct periph_init_data *sunxi_clk_get_periph_by_name(const char *name)
{
	struct periph_init_data *perpih;
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {
		perpih = &sunxi_periphs_init[i];
		if (strcmp(name, perpih->name))
			continue;
		return perpih;
	}

	return NULL;
}

/*
 * sunxi_clk_get_periph_cpus_by_name() - Get periph clk init config
 */
struct periph_init_data *sunxi_clk_get_periph_cpus_by_name(const char *name)
{
	return NULL;
}

static int clk_video_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	unsigned long factor_m = 0;
	unsigned long reg;
	struct sunxi_clk_periph *periph = to_clk_periph(hw);
	struct sunxi_clk_periph_div *divider = &periph->divider;
	unsigned long div, div_m = 0;

	div = DIV_ROUND_UP_ULL(parent_rate, rate);

	if (!div) {
		div_m = 0;
	} else {
		div_m = 1 << divider->mwidth;

		factor_m = (div > div_m ? div_m : div) - 1;
		div_m = factor_m;
	}

	reg = periph_readl(periph, divider->reg);
	if (divider->mwidth)
		reg = SET_BITS(divider->mshift, divider->mwidth, reg, div_m);
	periph_writel(periph, reg, divider->reg);

	return 0;
}

struct clk_ops disp_priv_ops;
void set_disp_priv_ops(struct clk_ops *priv_ops)
{
	priv_ops->determine_rate = clk_divider_determine_rate;
	priv_ops->set_rate = clk_video_set_rate;
}

void sunxi_set_clk_priv_ops(char *clk_name, struct clk_ops *clk_priv_ops,
	void (*set_priv_ops)(struct clk_ops *priv_ops))
{
	int i = 0;
	sunxi_clk_get_periph_ops(clk_priv_ops);
	set_priv_ops(clk_priv_ops);
	for (i = 0; i < (ARRAY_SIZE(sunxi_periphs_init)); i++) {
		if (!strcmp(sunxi_periphs_init[i].name, clk_name))
			sunxi_periphs_init[i].periph->priv_clkops = clk_priv_ops;
	}
}

static const u32 sun65iw1_pll_regs[] = {
	PLL_VIDEO0,
	PLL_VIDEO1,
	PLL_VIDEO2,
	PLL_VE,
};

void init_clocks(void)
{
	int i;
	struct factor_init_data *factor;
	struct periph_init_data *periph;
	unsigned long reg;

	/* get clk register base address */
	sunxi_clk_base = (void *)SUNXI_CCMU_BASE; // fixed base address.

	sunxi_clk_factor_initlimits();
	clk_register_fixed_rate(NULL, "hosc", NULL, CLK_IS_ROOT, 24000000);

	sunxi_set_clk_priv_ops("de",                &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tcon_lcd0",         &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("mipi_dsi_combphy0", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tcon_tv0",          &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("combophy0",         &disp_priv_ops, set_disp_priv_ops);

	/* Enable the output of video Plls */
	for (i = 0; i < ARRAY_SIZE(sun65iw1_pll_regs); i++) {
		reg = readl(sunxi_clk_base + sun65iw1_pll_regs[i]);
		reg = SET_BITS(27, 1, reg, 0x1);
		writel(reg, sunxi_clk_base + sun65iw1_pll_regs[i]);
	}

	/* Enable the lock enable of video Plls */
	for (i = 0; i < ARRAY_SIZE(sun65iw1_pll_regs); i++) {
		reg = readl(sunxi_clk_base + sun65iw1_pll_regs[i]);
		reg = SET_BITS(29, 1, reg, 0x1);
		writel(reg, sunxi_clk_base + sun65iw1_pll_regs[i]);
	}

	/* register normal factors, based on sunxi factor framework */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {
		factor = &sunxi_factos[i];
		factor->priv_regops = NULL;
		sunxi_clk_register_factors(NULL, (void *)sunxi_clk_base,
				(struct factor_init_data *)factor);
	}

	clk_register_fixed_factor(NULL, "pll_periph0_600m", "pll_periph0_2x", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph0_400m", "pll_periph0_2x", 0, 1, 3);
	clk_register_fixed_factor(NULL, "pll_periph0_300m", "pll_periph0_600m", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph0_200m", "pll_periph0_400m", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph0_150m", "pll_periph0_300m", 0, 1, 2);

	/* register periph clock */
	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {
		periph = &sunxi_periphs_init[i];
		periph->periph->priv_regops = NULL;
		sunxi_clk_register_periph(periph, sunxi_clk_base);
	}
}
