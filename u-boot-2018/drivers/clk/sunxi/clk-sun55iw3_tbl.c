/*
 * Allwinner sun55iw3p1 SoCs clk driver.
 *
 * Copyright(c) 2012-2016 Allwinnertech Co., Ltd.
 * Author: huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "clk-sun55iw3.h"

/* PLLPERIPH1_2X(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph0_2x_tbl[] = {
PLL_PERIPH0_2X(99,	0,	1,	1200000000U),
};

/* PLLPERIPH1_2X(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph1_2x_tbl[] = {
PLL_PERIPH1_2X(49,	0,	0,	600000000U),
};

/* PLLPERIPH1_800M(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph1_800m_tbl[] = {
PLL_PERIPH1_800M(49,	0,	0,	800000000U),
};

static unsigned int pllperiph0_2x_max, pllperiph1_2x_max, pllperiph1_800m_max;

#define PLL_MAX_ASSIGN(name) (pll##name##_max = \
	factor_pll##name##_tbl[ARRAY_SIZE(factor_pll##name##_tbl)-1].freq)

void sunxi_clk_factor_initlimits(void)
{
	PLL_MAX_ASSIGN(periph0_2x);
	PLL_MAX_ASSIGN(periph1_2x);
	PLL_MAX_ASSIGN(periph1_800m);
}
