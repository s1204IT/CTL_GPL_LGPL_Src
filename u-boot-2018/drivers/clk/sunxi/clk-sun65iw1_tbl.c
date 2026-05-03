/*
 * Allwinner sun55iw3p1 SoCs clk driver.
 *
 * Copyright(c) 2012-2016 Allwinnertech Co., Ltd.
 * Author: huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "clk-sun65iw1.h"

/* PLLPERIPH1_2X(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph0_2x_tbl[] = {
PLL_PERIPH0_2X(99,	0,	1,	1200000000U),
};

/* PLLPERIPH1_2X(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph0_800m_tbl[] = {
PLL_PERIPH0_800M(99,	0,	2,	800000000U),
};

/* PLLPERIPH1_800M(n, d1, d2, freq) */
struct sunxi_clk_factor_freq factor_pllperiph0_480m_tbl[] = {
PLL_PERIPH0_480M(99,	0,	5,	480000000U),
};

static unsigned int pllperiph0_2x_max, pllperiph0_800m_max, pllperiph0_480m_max;

#define PLL_MAX_ASSIGN(name) (pll##name##_max = \
	factor_pll##name##_tbl[ARRAY_SIZE(factor_pll##name##_tbl)-1].freq)

void sunxi_clk_factor_initlimits(void)
{
	PLL_MAX_ASSIGN(periph0_2x);
	PLL_MAX_ASSIGN(periph0_800m);
	PLL_MAX_ASSIGN(periph0_480m);
}
