/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc wdt ops
 * operation interface of watchdog timer for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SUNXI_RPROC_WDT_H
#define SUNXI_RPROC_WDT_H

#include <linux/kernel.h>
//#include <linux/spinlock.h>
#include <linux/ioport.h>

struct sunxi_rproc_wdt;

enum sunxi_rproc_wdt_reset_type {
	RESET_DBG = 0,
	RESET_SYS,
	RESET_INT,
};

struct sunxi_rproc_wdt_param {
	void (*cb)(struct sunxi_rproc_wdt *wdt, void *priv);
	void *cb_priv;

	// wdt param
	struct resource reg_res;
	int irq_num;
	u32 timeout_ms;
	u32 reset_type;
};

struct sunxi_rproc_wdt {
	//spinlock_t lock;

	// res
	void __iomem *base;
	const struct sunxi_wdt_reg *regs;

	// wdt default param for start
	struct sunxi_rproc_wdt_param param;

	// pm
	u32 saved_timeout;
	u32 saved_rst_type;
};

// direct
int sunxi_rproc_wdt_set_timeout(struct sunxi_rproc_wdt *wdt, u32 timeout_ms);
int sunxi_rproc_wdt_get_timeout(struct sunxi_rproc_wdt *wdt, u32 *timeout_ms);
int sunxi_rproc_wdt_set_reset_type(struct sunxi_rproc_wdt *wdt, u32 type);
int sunxi_rproc_wdt_get_reset_type(struct sunxi_rproc_wdt *wdt, u32 *type);
int sunxi_rproc_wdt_is_enable(struct sunxi_rproc_wdt *wdt);

int sunxi_rproc_wdt_start(struct sunxi_rproc_wdt *wdt);
void sunxi_rproc_wdt_stop(struct sunxi_rproc_wdt *wdt);

int sunxi_rproc_wdt_suspend(struct sunxi_rproc_wdt *wdt);
int sunxi_rproc_wdt_resume(struct sunxi_rproc_wdt *wdt);

int sunxi_rproc_wdt_deinit(struct sunxi_rproc_wdt *wdt);
int sunxi_rproc_wdt_init(struct sunxi_rproc_wdt *wdt, struct sunxi_rproc_wdt_param *param);

#endif /* SUNXI_RPROC_WDT_H */
