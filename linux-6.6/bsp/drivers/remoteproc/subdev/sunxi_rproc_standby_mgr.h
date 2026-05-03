/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc standby dev driver
 * register or unregister rsc helper device for rproc.
 *
 * Copyright (C) 2025 Allwinnertech - All Rights Reserved
 *
 * Author: chenjinkun <chenjinkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef SUNXI_RPROC_STANDBY_MGR_DEV_H
#define SUNXI_RPROC_STANDBY_MGR_DEV_H
#include <linux/remoteproc.h>
#include <linux/device.h>

typedef enum {
	RPROC_STANDBY_STAY_ALIVE,
	RPROC_STANDBY_STAY_DEEP_SLEEP,
	RPROC_STANDBY_MODE_NULL
} rproc_stby_mode_t;

struct sunxi_rproc_standby_dev {
	struct device dev;
	struct rproc_subdev subdev;
	struct rproc *rproc;

	rproc_stby_mode_t stby_mode;

	void (*on_release)(void *priv);
	void *on_release_priv;
};

int sunxi_rproc_standby_dev_register(struct sunxi_rproc_standby_dev *stdby_dev, struct rproc *rproc);
int sunxi_rproc_standby_dev_unregister(struct sunxi_rproc_standby_dev *stdby_dev);
int sunxi_rproc_standby_dev_set_release(struct sunxi_rproc_standby_dev *stdby_dev,
				    void (*release)(void *), void *priv);
int sunxi_rproc_standby_dev_set_parent(struct sunxi_rproc_standby_dev *stdby_dev,
					  struct device *parent);

#endif
