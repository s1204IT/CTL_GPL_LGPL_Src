/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi_rproc_standby.h for remoteproc standby api
 *
 * Copyright (c) 2024 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Chen Jinkun <chenjinkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SUNXI_RPROC_STANDBY_H
#define SUNXI_RPROC_STANDBY_H

#include <linux/slab.h>
#include <asm/io.h>
#include <linux/platform_device.h>

struct sunxi_rproc;
struct sunxi_rproc_priv;
struct sunxi_rproc_standby;
struct sunxi_rproc_standby_dev;

struct sunxi_rproc_standby_ops {
	int (*init)(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev);
	void (*exit)(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev);
	int (*attach)(struct sunxi_rproc_standby *rproc_standby);
	int (*detach)(struct sunxi_rproc_standby *rproc_standby);
	int (*start)(struct sunxi_rproc_standby *rproc_standby);
	int (*stop)(struct sunxi_rproc_standby *rproc_standby);
	int (*prepare)(struct sunxi_rproc_standby *rproc_standby);
	int (*suspend)(struct sunxi_rproc_standby *rproc_standby);
	int (*suspend_late)(struct sunxi_rproc_standby *rproc_standby);
	int (*resume_early)(struct sunxi_rproc_standby *rproc_standby);
	int (*resume)(struct sunxi_rproc_standby *rproc_standby);
	void (*complete)(struct sunxi_rproc_standby *rproc_standby);
};

struct standby_memory_store {
	uint32_t start;
	uint32_t size;
	void *buf;
	void __iomem *iomap;
};

struct sunxi_rproc_standby {
	const char *name;
	struct sunxi_rproc_priv *rproc_priv;
	struct sunxi_rproc_standby_ops *ops;
	struct sunxi_rproc_standby_dev *stdby_dev;
	struct device *dev;
	struct list_head list;
	uint32_t ctrl_en;
	uint32_t num_memstore;
	void *priv;
};

struct sunxi_rproc_standby *sunxi_rproc_standby_find(const char *name);
int sunxi_rproc_standby_ops_register(const char *name, struct sunxi_rproc_standby_ops *ops, void *priv);
int sunxi_rproc_standby_ops_unregister(const char *name);
int sunxi_rproc_standby_init(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev);
void sunxi_rproc_standby_exit(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev);
int sunxi_rproc_standby_start(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_stop(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_attach(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_detach(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_init_prepare(struct sunxi_rproc_standby *rproc_standby);

int sunxi_rproc_standby_prepare(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_suspend(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_suspend_late(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_resume_early(struct sunxi_rproc_standby *rproc_standby);
int sunxi_rproc_standby_resume(struct sunxi_rproc_standby *rproc_standby);
void sunxi_rproc_standby_complete(struct sunxi_rproc_standby *rproc_standby);

struct sunxi_rproc_standby *sunxi_rproc_get_standby_handler(struct sunxi_rproc *chip);

#endif /* SUNXI_RPROC_STANDBY_H */
