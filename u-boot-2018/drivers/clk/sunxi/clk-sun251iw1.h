/*
 * Copyright (C) 2013 Allwinnertech, huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN251IW1_H
#define __MACH_SUNXI_CLK_SUN251IW1_H

#include "clk_factor.h"

#define SUNXI_CCMU_BASE     0x02001000

/* CCU Register List */
#define PLL_PERIPH         	0x0020
#define PLL_VIDEO0         	0x0040
#define PLL_VIDEO1         	0x0048
#define PLL_AUDIO1         	0x0080

#define DE_REG              0x0600
#define DE_BUS_REG          0x060c
#define DPSS_TOP_REG        0x0abc
#define TCONLCD_REG         0x0b60
#define TCONLCD_BUS_REG     0x0b7c
#define LVDS_BUS_REG        0x0bac
#define KSC_BUS_REG         0x061c
#define DSI_REG             0x0b24
#define DSI_BUS_REG         0x0b4c
#define DI_BUS_REG          0x062c
#define G2D_REG             0x0630
#define G2D_BUS_REG         0x063c
#define MBUS_MST_REG        0x0804

#endif
