/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN300IW1_H
#define __MACH_SUNXI_CLK_SUN300IW1_H

#include "clk_factor.h"


/* AON Register List */
#define PLL_PERIPH          0x0020
#define PLL_VIDEO           0x0040
#define PLL_CSI             0x0048

/* APP Register List */
#define TCON_LCD            0x0034
#define DE                  0x0038
#define BUS_CLK_GATE0       0x0080
#define BUS_CLK_GATE2       0x0088
#define BUS_RESET0          0x0090
#define BUS_RESET1          0x0094

#define SUNXI_AON_CLK_MAX_REG   0x600
#define SUNXI_APP_CLK_MAX_REG   0x100

#endif
