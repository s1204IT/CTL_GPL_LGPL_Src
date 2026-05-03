/*
 * Copyright (C) 2013 Allwinnertech, huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN65IW1_H
#define __MACH_SUNXI_CLK_SUN65IW1_H

#include "clk_factor.h"

/* CCMU Register List */
#define PLL_PERIPH0 	0x00A0
#define PLL_VIDEO0		0x0120
#define PLL_VIDEO1		0x0140
#define PLL_VIDEO2		0x0160
#define PLL_VE			0x0220

#define MBUS_MASTER     0x05E0
#define DE_CFG          0x0A00
#define DE_AHB			0x0A04
#define TCON_LCD0_CFG	0x1500
#define TCON_LCD0_AHB	0x1504
#define RST_BUS_LVDS0	0x1544
#define COMBPHY0_CFG	0x1580
#define COMBPHY0_AHB	0x1584
#define COMBOPHY0		0x15C0
#define TCON_TV0_CFG	0x1600
#define TCON_TV0_AHB	0x1604
#define EDP				0x164C
#define VO0_AHB			0x16C4
#define VO1_AHB			0x16CC
#define RST_VIDEO_OUT0  0x16E4


#define PLL_PERIPH0_2X_FACTOR(nv, d1v, d2v)   (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 16, 3))
#define PLL_PERIPH0_800M_FACTOR(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 20, 3))
#define PLL_PERIPH0_480M_FACTOR(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 2, 3))

#define PLL_PERIPH0_2X(n, d1, d2, freq)    {PLL_PERIPH0_2X_FACTOR(n, d1, d2), freq}
#define PLL_PERIPH0_800M(n, d1, d2, freq)  {PLL_PERIPH0_800M_FACTOR(n, d1, d2), freq}
#define PLL_PERIPH0_480M(n, d1, d2, freq)  {PLL_PERIPH0_480M_FACTOR(n, d1, d2), freq}

#endif
