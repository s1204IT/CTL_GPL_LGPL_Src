/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc wdt dev driver
 * register or unregister watchdog timer device for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SUNXI_RPROC_WDT_DEV_H
#define SUNXI_RPROC_WDT_DEV_H

#include "sunxi_rproc_wdt.h"

#define SUNXI_RPROC_WDT_CLASS
#define SUNXI_RPROC_WDT_SUBDEV

#include <linux/device.h>
#include <linux/remoteproc.h>

enum sunxi_rproc_wdt_reset_ext_type {
	RESET_ALL = RESET_SYS,
	RESET_CORE = RESET_INT,
	RESET_NONE,
};

struct sunxi_rproc_wdt_dev {
	struct device dev;
#ifdef SUNXI_RPROC_WDT_SUBDEV
	struct rproc_subdev subdev;
#endif
	struct sunxi_rproc_wdt wdt;
	struct rproc *rproc;

	void (*on_release)(void *priv);
	void *on_release_priv;

	u32 reset_type;
	u32 timeout_cnt;
	u32 try_times;
};

struct sunxi_rproc_wdt_dev_res {
	struct resource reg_res;
	int irq_num;
	u32 try_times;
	u32 timeout_ms;
	u32 reset_type;
};

int sunxi_rproc_wdt_dev_register(struct sunxi_rproc_wdt_dev *wdt_dev,
				 struct rproc *rproc, struct sunxi_rproc_wdt_dev_res *res);
int sunxi_rproc_wdt_dev_unregister(struct sunxi_rproc_wdt_dev *wdt_dev);
int sunxi_rproc_wdt_dev_set_release(struct sunxi_rproc_wdt_dev *wdt_dev,
				    void (*release)(void *), void *priv);
int sunxi_rproc_wdt_dev_set_parent(struct sunxi_rproc_wdt_dev *wdt_dev, struct device *parent);

#endif /* SUNXI_RPROC_WDT_DEV_H */
