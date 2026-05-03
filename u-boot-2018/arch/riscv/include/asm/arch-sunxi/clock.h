/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 */

#ifndef _SUNXI_CLOCK_H
#define _SUNXI_CLOCK_H

#include <linux/types.h>

#define CLK_GATE_OPEN			0x1
#define CLK_GATE_CLOSE			0x0

/* clock control module regs definition */
#if defined(CONFIG_MACH_SUN20IW1) || defined(CONFIG_MACH_SUN8IW20)
#include <asm/arch/clock_sun20iw1.h>
#elif defined (CONFIG_MACH_SUN300IW1)
#include <asm/arch/plat-sun300iw1p1/clock_sun300iw1_app.h>
#include <asm/arch/plat-sun300iw1p1/clock_sun300iw1_aon.h>
#elif defined (CONFIG_MACH_SUN251IW1)
#include <asm/arch/plat-sun251iw1p1/clock_sun251iw1.h>
#else
#include <asm/arch/clock_sun4i.h>
#endif

struct core_pll_freq_tbl {
    int FactorN;
    int FactorK;
    int FactorM;
    int FactorP;
    int pading;
};

#ifndef __ASSEMBLY__
int clock_init(void);
int clock_twi_onoff(int port, int state);
void clock_set_de_mod_clock(u32 *clk_cfg, unsigned int hz);
void clock_init_safe(void);
void clock_init_sec(void);
void clock_init_uart(void);

uint clock_get_corepll(void);
int clock_set_corepll(int frequency);
uint clock_get_pll6(void);
uint clock_get_ahb(void);
uint clock_get_apb1(void);
uint clock_get_apb2(void);
uint clock_get_axi(void);
uint clock_get_mbus(void);

#endif

#endif /* _SUNXI_CLOCK_H */
