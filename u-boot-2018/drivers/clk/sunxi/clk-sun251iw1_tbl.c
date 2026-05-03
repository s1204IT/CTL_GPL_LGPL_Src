/*
 * Allwinner sun55iw3p1 SoCs clk driver.
 *
 * Copyright(c) 2012-2016 Allwinnertech Co., Ltd.
 * Author: huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "clk-sun251iw1.h"

#define PLL_PERIPH_2X_FACTOR(nv, d1v, d2v)    (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 16, 3))
#define PLL_PERIPH_1X_FACTOR(nv, d1v, d2v)    (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 16, 3))
#define PLL_PERIPH_800M_FACTOR(nv, d1v, d2v)  (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 20, 3))

#define PLL_PERIPH_2X(n, d1, d2, freq)        {PLL_PERIPH_2X_FACTOR(n, d1, d2), freq}
#define PLL_PERIPH_1X(n, d1, d2, freq)        {PLL_PERIPH_1X_FACTOR(n, d1, d2), freq}
#define PLL_PERIPH_800M(n, d1, d2, freq)      {PLL_PERIPH_800M_FACTOR(n, d1, d2), freq}

/* PLLPERIPH_2X(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph_2x_tbl[] = {
PLL_PERIPH_2X(99,	0, 1, 1200000000U),
};

/* PLLPERIPH_800M(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph_800m_tbl[] = {
PLL_PERIPH_800M(99, 0, 2, 800000000U),
};

static unsigned int pllperiph_2x_max, pllperiph_800m_max;

#define PLL_MAX_ASSIGN(name) (pll##name##_max = \
	factor_pll##name##_tbl[ARRAY_SIZE(factor_pll##name##_tbl)-1].freq)

void sunxi_clk_factor_initlimits(void)
{
	PLL_MAX_ASSIGN(periph_2x);
	PLL_MAX_ASSIGN(periph_800m);
}
