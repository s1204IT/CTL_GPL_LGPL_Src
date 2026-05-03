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
#include "clk-sun251iw1.h"
#include "clk-sun251iw1_tbl.c"
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

/*                                  ns  nw  ks  kw  ms  mw  ps  pw  d1s	d1w	d2s d2w {frac   out mode}	en-s    sdmss   sdmsw   sdmpat  sdmval */
SUNXI_CLK_FACTORS(pll_periph_2x,    8,  8,  0,  0,  0,  0,  0,  0,  0, 	0,	16, 3,   0,     0,  0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_periph_800m,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,	20, 3,   0,     0,  0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_video0_4x,    8,  8,  0,  0,  0,  0,  0,  0,  0,  0,	0,  0,   0,     0,  0,      31,     0,      0,      0,      0);
SUNXI_CLK_FACTORS(pll_video1_4x,    8,  8,  0,  0,  0,  0,  0,  0,  0,  0, 	0,  0,   0,     0,  0,      31,     0,      0,      0,      0);

static int get_factors_pll_periph_2x(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph_2x_max ? pllperiph_2x_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph_2x))
		return -1;

	return 0;
}

static int get_factors_pll_periph_800m(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    int index;
	u64 tmp_rate;

	if (!factor)
		return -1;
	tmp_rate = rate > pllperiph_800m_max ? pllperiph_800m_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph_800m))
		return -1;

	return 0;
}

static unsigned long calc_rate_pll_periph(u32 parent_rate, struct clk_factors_value *factor)
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

static unsigned long calc_rate_video(u32 parent_rate, struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);
	tmp_rate = tmp_rate * (factor->factorn + 1);
	return (unsigned long)tmp_rate;
}

static int get_factors_pll_video(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	u8 min_n = 53, max_n = 105;
	u8 best_n = 0;
	u64 best_rate = 0;

	for (factor->factorn = min_n; factor->factorn <= max_n; factor->factorn++) {
		u64 tmp_rate;

		tmp_rate = calc_rate_video(parent_rate, factor);

		if (tmp_rate > rate)
			continue;

		if ((rate - tmp_rate) < (rate - best_rate)) {
			best_rate = tmp_rate;
			best_n = factor->factorn;
		}
	}

	factor->factorn = best_n;

	return 0;
}

static const char *hosc_parents[] = {"hosc"};
struct factor_init_data sunxi_factos[] = {
/* name	parent	parent_num	flags	reg	lock_reg	lock_bit	pll_lock_ctrl_reg	lock_en_bit	lock_mode	config	get_factors	calc_rate	priv_ops*/
{"pll_periph_2x",	hosc_parents,	1,	0,	PLL_PERIPH,	PLL_PERIPH,	LOCKBIT(28),	PLL_PERIPH,	29,	PLL_LOCK_NEW_MODE,	&sunxi_clk_factor_pll_periph_2x,	&get_factors_pll_periph_2x,	&calc_rate_pll_periph,	(struct clk_ops *)NULL},
{"pll_periph_800m",	hosc_parents,	1,	0,	PLL_PERIPH, PLL_PERIPH,	LOCKBIT(28),	PLL_PERIPH,	29,	PLL_LOCK_NEW_MODE,	&sunxi_clk_factor_pll_periph_800m,	&get_factors_pll_periph_800m,	&calc_rate_pll_periph,	(struct clk_ops *)NULL},
{"pll_video0_4x",	hosc_parents,	1,	0,	PLL_VIDEO0,	PLL_VIDEO0,	LOCKBIT(28),	PLL_VIDEO0,	29,	PLL_LOCK_NEW_MODE,	&sunxi_clk_factor_pll_video0_4x,	&get_factors_pll_video,	&calc_rate_video,	(struct clk_ops *)NULL},
{"pll_video1_4x",	hosc_parents,	1,	0,	PLL_VIDEO1, PLL_VIDEO1,	LOCKBIT(28),	PLL_VIDEO1,	29,	PLL_LOCK_NEW_MODE,	&sunxi_clk_factor_pll_video1_4x,	&get_factors_pll_video,	&calc_rate_video,	(struct clk_ops *)NULL},
};

/* the usage of the audio pll is not allowed in u-boot */
static const char *de_parents[] = {"pll_periph_2x", "pll_video0_4x", "pll_video1_4x",	""};
static const char *tconlcd_parents[] = {"pll_video0_1x", "pll_video0_4x", "pll_video1_1x",	"pll_video1_4x", "pll_periph_2x", ""};
static const char *dsi_parents[] = {"hosc", "pll_periph_1x", "pll_video0_2x", "pll_video1_2x",	""};
static const char *g2d_parents[] = {"pll_periph_2x", "pll_video0_4x", "pll_video1_4x",	""};

/*
SUNXI_CLK_PERIPH(name,	mux_reg,	mux_sft,	mux_wid,	div_reg,	div_msft,	div_mwid,	div_nsft,	div_nwid,	gate_flag,	en_reg,	rst_reg,	bus_gate_reg,	drm_gate_reg,	en_sft,	rst_sft,	bus_gate_sft,	dram_gate_sft,	lock,	com_gate,	com_gate_off)
*/
SUNXI_CLK_PERIPH(de,	DE_REG,	24,	3,	DE_REG,	0,	5,	0,	0,	0,	DE_REG,	DE_BUS_REG, 	DE_BUS_REG,	0,	31,	16,	0,	0,	&clk_lock,  NULL,       0);
SUNXI_CLK_PERIPH(dpss_top_bus,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	DPSS_TOP_REG,	DPSS_TOP_REG,	0,	0,	16,	0,	0,	&clk_lock,  NULL,       0);
SUNXI_CLK_PERIPH(tconlcd,	TCONLCD_REG,	24,	3,	TCONLCD_REG,	0,	4,	8,	2,	0,	TCONLCD_REG,	TCONLCD_BUS_REG,	TCONLCD_BUS_REG,	0,	31,	16,	0,	0,	&clk_lock,  NULL,       0);
SUNXI_CLK_PERIPH(lvds_bus,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	LVDS_BUS_REG,	0,	0,	0,	16,	0,	0,	&clk_lock,	NULL,	0);
SUNXI_CLK_PERIPH(ksc_bus,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	KSC_BUS_REG,	0,	0,	0,	16,	0,	0,	&clk_lock,	NULL,	0);
SUNXI_CLK_PERIPH(dsi,	DSI_REG,	24,	3,	DSI_REG,	0,	4,	0,	0,	0,	DSI_REG,	DSI_BUS_REG,	DSI_BUS_REG,	0,	31,	16,	0,	0,	&clk_lock,	NULL,	0);
SUNXI_CLK_PERIPH(g2d,	G2D_REG,	24,	3,	G2D_REG,	0,	5,	0,	0,	0,	G2D_REG,	0,	0,	0,	31,	0,	0,	0,	&clk_lock,	NULL,	0);
SUNXI_CLK_PERIPH(g2d_bus,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	G2D_BUS_REG,	G2D_BUS_REG,	0,	0,	16,	0,	0,	&clk_lock,	NULL,	0);
SUNXI_CLK_PERIPH(mbus_g2d,	0,	0,	0,	0,	0,	0,	0,	0,	0,	MBUS_MST_REG,	0,	0,	0,	10,	0,	0,	0,	&clk_lock,	NULL,	0);

struct periph_init_data sunxi_periphs_init[] = {
	{"de",              CLK_SET_RATE_PARENT,    de_parents,         ARRAY_SIZE(de_parents),	&sunxi_clk_periph_de},
	{"dpss_top_bus",    0,                      hosc_parents,       ARRAY_SIZE(hosc_parents),	&sunxi_clk_periph_dpss_top_bus},
	{"tconlcd",         CLK_SET_RATE_PARENT,    tconlcd_parents,    ARRAY_SIZE(tconlcd_parents),	&sunxi_clk_periph_tconlcd},
	{"lvds_bus",        0,                      hosc_parents,       ARRAY_SIZE(hosc_parents),	&sunxi_clk_periph_lvds_bus},
	{"ksc_bus",         0,                      hosc_parents,       ARRAY_SIZE(hosc_parents),	&sunxi_clk_periph_ksc_bus},
	{"dsi",             CLK_SET_RATE_PARENT,    dsi_parents,        ARRAY_SIZE(dsi_parents),	&sunxi_clk_periph_dsi},
	{"g2d",             CLK_SET_RATE_PARENT,    g2d_parents,        ARRAY_SIZE(g2d_parents),	&sunxi_clk_periph_g2d},
	{"g2d_bus",         0,                      hosc_parents,       ARRAY_SIZE(hosc_parents),	&sunxi_clk_periph_g2d_bus},
	{"mbus_g2d",        0,                      hosc_parents,       ARRAY_SIZE(hosc_parents),	&sunxi_clk_periph_mbus_g2d},
};

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

static const u32 sun251iw1_pll_regs[] = {
	PLL_VIDEO0,
	PLL_VIDEO1,
	PLL_AUDIO1,
};

void init_clocks(void)
{
	int i;
	struct factor_init_data *factor;
	struct periph_init_data *periph;
	unsigned long reg;

	/* get clk register base address */
	sunxi_clk_base = (void *)SUNXI_CCMU_BASE; // fixed base address.

	/* init periph clock */
	sunxi_clk_factor_initlimits();
	clk_register_fixed_rate(NULL, "hosc", NULL, CLK_IS_ROOT, 24000000);

	sunxi_set_clk_priv_ops("de",        &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tconlcd",   &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("dsi",       &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("g2d",       &disp_priv_ops, set_disp_priv_ops);

	/* Enable the output of video Plls */
	for (i = 0; i < ARRAY_SIZE(sun251iw1_pll_regs); i++) {
		reg = readl(sunxi_clk_base + sun251iw1_pll_regs[i]);
		reg = SET_BITS(27, 1, reg, 0x1);
		writel(reg, sunxi_clk_base + sun251iw1_pll_regs[i]);
	}

	/* Enable the lock enable of video Plls */
	for (i = 0; i < ARRAY_SIZE(sun251iw1_pll_regs); i++) {
		reg = readl(sunxi_clk_base + sun251iw1_pll_regs[i]);
		reg = SET_BITS(29, 1, reg, 0x1);
		writel(reg, sunxi_clk_base + sun251iw1_pll_regs[i]);
	}

	/* register normal factors, based on sunxi factor framework */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {
		factor = &sunxi_factos[i];
		factor->priv_regops = NULL;
		sunxi_clk_register_factors(NULL, (void *)sunxi_clk_base,
				(struct factor_init_data *)factor);
	}

	/* PLL_VIDEO0 */
	reg = readl(sunxi_clk_base + PLL_VIDEO0);
	reg &= ~((0x1 << 0) | (0x1 << 1));
	writel(reg, sunxi_clk_base + PLL_VIDEO0);

	/* PLL_VIDEO1 */
	reg = readl(sunxi_clk_base + PLL_VIDEO1);
	reg &= ~((0x1 << 0) | (0x1 << 1));
	writel(reg, sunxi_clk_base + PLL_VIDEO1);

	clk_register_fixed_factor(NULL, "pll_periph_1x", "pll_periph_2x", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_video0_2x", "pll_video0_4x", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_video0_1x", "pll_video0_4x", 0, 1, 4);
	clk_register_fixed_factor(NULL, "pll_video1_2x", "pll_video1_4x", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_video1_1x", "pll_video1_4x", 0, 1, 4);

	/* register periph clock */
	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {
		periph = &sunxi_periphs_init[i];
		periph->periph->priv_regops = NULL;
		sunxi_clk_register_periph(periph, sunxi_clk_base);
	}
}
