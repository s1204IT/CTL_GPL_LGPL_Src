/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "clk-sun300iw1.h"
#include "clk_factor.h"
#include "clk_periph.h"
#include <linux/compat.h>
#include <fdt_support.h>
#include <div64.h>

DEFINE_SPINLOCK(clk_lock);
void __iomem *sunxi_aon_clk_base;
void __iomem *sunxi_app_clk_base;

/*                                     ns  nw  ks kw   ms  mw  ps  pw  d1s d1w d2s d2w {frac   out mode}   en-s    sdmss   sdmsw   sdmpat        sdmval*/
SUNXI_CLK_FACTORS(pll_periph,          8,  8,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_videox4,         8,  8,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,    0,    0,  0,      31,     0,      0,      0,               0);

#define SUN300IW1_DCXO_ST_REG	(0x404)
static unsigned long get_host_freq(void)
{
	unsigned long time_freq;
	u32 val;

	val = readl(sunxi_aon_clk_base + SUN300IW1_DCXO_ST_REG);
	if (!!(val & BIT(31)))
		time_freq = 24000000;
	else
		time_freq = 40000000;

	return time_freq;
}

static int get_factors_pll_periph(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	return 0;
}

static unsigned long calc_rate_pll_periph(u32 parent_rate,
		struct clk_factors_value *factor)
{
	u8 post_div = 5;
	unsigned long time_freq = get_host_freq();
	u64 tmp_rate = (parent_rate ? parent_rate : time_freq);

	if (time_freq == 24000000)
		post_div = 3;

	factor->factorn = factor->factorn >= 0xff ? 0 : factor->factorn;

	tmp_rate = tmp_rate * (factor->factorn + 1) * 2;
	do_div(tmp_rate, post_div);

	return (unsigned long)tmp_rate;
}

/*    pll_videox4/pll_csix4: 24*N/D1    */
static unsigned long calc_rate_video(u32 parent_rate,
		struct clk_factors_value *factor)
{
	unsigned long time_freq = get_host_freq();
	u64 tmp_rate = (parent_rate ? parent_rate : time_freq);
	tmp_rate = tmp_rate * (factor->factorn + 1);
	do_div(tmp_rate, 1);
	return (unsigned long)tmp_rate;
}

static int get_factors_pll_video(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	u8 min_n = 12, max_n = 36;
	u8 best_n = 0;
	u64 best_rate = 0;
	unsigned long time_freq = get_host_freq();

	if (time_freq == 24000000) {
		min_n = 20;
		max_n = 61;
	}

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
	/* name                  parent      parent_num,      flags               reg          lock_reg       lock_bit     pll_lock_ctrl_reg   lock_en_bit            lock_mode           config                            get_factors                      calc_rate              priv_ops*/
	{"pll_periph",       hosc_parents,      1,        CLK_NO_DISABLE,      PLL_PERIPH,    PLL_PERIPH,         28,    PLL_PERIPH,           29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_periph,       &get_factors_pll_periph,            &calc_rate_pll_periph, (struct clk_ops *)NULL},
	{"pll_videox4",      hosc_parents,      1,        CLK_NO_DISABLE,      PLL_VIDEO,     PLL_VIDEO,          28,    PLL_VIDEO,            29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_videox4,      &get_factors_pll_video,             &calc_rate_video,      (struct clk_ops *)NULL},
};

static const char *de_parents[] = {"pll_periph_307m", "pll_videox1"};
static const char *tcon_lcd_parents[] = {"pll_videox4", "pll_periph_512m", "pll_csix4"};

/*
   SUNXI_CLK_PERIPH(name,                mux_reg,     mux_sft, mux_wid,      div_reg,      div_msft,  div_mwid,   div_nsft,   div_nwid,    gate_flag,   en_reg,       rst_reg,      bus_gate_reg,   drm_gate_reg,     en_sft,    rst_sft,   bus_gate_sft,   dram_gate_sft,      lock,   com_gate,   com_gate_off)
 */
SUNXI_CLK_PERIPH(de,                      DE,            24,      1,            DE,           0,         5,          0,          0,          0,          DE,         BUS_RESET0,    BUS_CLK_GATE2,      0,             31,         25,         1,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(de_hb,                    0,             0,      0,             0,           0,         0,          0,          0,          0,          0,            0,           BUS_CLK_GATE2,      0,              0,         0,          0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(dpss_top0_bus,            0,             0,      0,             0,           0,         0,          0,          0,          0,          0,          BUS_RESET0,    BUS_CLK_GATE0,      0,              0,         31,        31,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(disp_ahb,                 0,             0,      0,             0,           0,         0,          0,          0,          0,          0,            0,           BUS_CLK_GATE0,      0,              0,          0,        26,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(disp_mbus,                0,             0,      0,             0,           0,         0,          0,          0,          0,          0,            0,           BUS_CLK_GATE0,      0,              0,          0,        25,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon_lcd,              TCON_LCD,        24,      2,          TCON_LCD,       0,         4,         16,          2,          0,       TCON_LCD,      BUS_RESET1,          0,            0,             31,         11,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon,                     0,             0,      0,             0,           0,         0,          0,          0,          0,          0,          BUS_RESET1,    BUS_CLK_GATE2,      0,              0,         10,         3,             0,             &clk_lock, NULL,             0);

struct periph_init_data sunxi_periphs_init[] = {
	{"de",                         0,           de_parents,               ARRAY_SIZE(de_parents),              &sunxi_clk_periph_de             },
	{"de_hb",                      0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_de_hb          },
	{"dpss_top0_bus",              0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_dpss_top0_bus  },
	{"disp_ahb",                   0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_disp_ahb       },
	{"disp_mbus",                  0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_disp_mbus      },
	{"tcon_lcd",                   0,          tcon_lcd_parents,           ARRAY_SIZE(tcon_lcd_parents),        &sunxi_clk_periph_tcon_lcd       },
	{"tcon",                       0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_tcon           },
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

#define SUN300IW1_PLL_PERIPH_CTRL0_REG	0x20
#define SUN300IW1_PLL_PERIPH_CTRL1_REG	0x24
#define SUN300IW1_PLL_VIDEO_CTRL_REG	0x40
#define SUN300IW1_PLL_CSI_CTRL_REG	0x48

static const u32 sun300iw1_pll_regs[] = {
	SUN300IW1_PLL_PERIPH_CTRL0_REG,
	SUN300IW1_PLL_VIDEO_CTRL_REG,
	SUN300IW1_PLL_CSI_CTRL_REG,
};

void init_clocks(void)
{
	int i;
	struct factor_init_data *factor;
	struct periph_init_data *periph;
	unsigned long reg, time_freq;

	/* get clk register base address */
	sunxi_app_clk_base = (void *)0x42001000; // fixed base address.
	sunxi_aon_clk_base = (void *)0x4a010000; // fixed base address.

	time_freq = get_host_freq();
	clk_register_fixed_rate(NULL, "hosc", NULL, CLK_IS_ROOT, time_freq);
	sunxi_set_clk_priv_ops("de0", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tcon_lcd", &disp_priv_ops, set_disp_priv_ops);

	/* Enable the output of Plls */
	for (i = 0; i < ARRAY_SIZE(sun300iw1_pll_regs); i++) {
		reg = readl(sunxi_aon_clk_base + sun300iw1_pll_regs[i]);
		reg = SET_BITS(27, 1, reg, 0x1);
		writel(reg, sunxi_aon_clk_base + sun300iw1_pll_regs[i]);
	}
	/* Enable the output of Pll_periph */
	writel(0xFFFF, sunxi_aon_clk_base + SUN300IW1_PLL_PERIPH_CTRL1_REG);

	/* Enable the lock enable of Plls */
	for (i = 0; i < ARRAY_SIZE(sun300iw1_pll_regs); i++) {
		reg = readl(sunxi_aon_clk_base + sun300iw1_pll_regs[i]);
		reg = SET_BITS(29, 1, reg, 0x1);
		writel(reg, sunxi_aon_clk_base + sun300iw1_pll_regs[i]);
	}

	/* register normal factors, based on sunxi factor framework */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {
		factor = &sunxi_factos[i];
		factor->priv_regops = NULL;
		sunxi_clk_register_factors(NULL, (void *)sunxi_aon_clk_base,
				(struct factor_init_data *)factor);
	}

	clk_register_fixed_factor(NULL, "pll_periph_307m", "pll_periph", 0, 1, 10);
	clk_register_fixed_factor(NULL, "pll_videox1", "pll_videox4", 0, 1, 4);
	/* register periph clock */
	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {
		periph = &sunxi_periphs_init[i];
		periph->periph->priv_regops = NULL;
		sunxi_clk_register_periph(periph, sunxi_app_clk_base);
	}
}
