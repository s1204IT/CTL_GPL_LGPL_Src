/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc rsc helper dev driver
 * register or unregister rsc helper device for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SUNXI_RPROC_RSC_HELPER_DEV_H
#define SUNXI_RPROC_RSC_HELPER_DEV_H

#include <linux/device.h>
#include <linux/remoteproc.h>
//#include <linux/spinlock.h>
#include <linux/mutex.h>
#if IS_ENABLED(CONFIG_AW_RPROC_USER_RESOURCE)
#include "sunxi_rproc_user_resource.h"
#endif

#define SUNXI_RPROC_RSC_HELPER_CLASS
#define SUNXI_RPROC_RSC_HELPER_SUBDEV

union rsc_info {
#if IS_ENABLED(CONFIG_AW_RPROC_USER_RESOURCE)
	struct fw_rsc_user_resource user_resource;
#endif
};

struct rsc_node {
	struct list_head list;
	const struct rsc_ops *ops;
	void *cache;
	union rsc_info info;
};

struct rsc_ops {
	const char *name;
	u32 type;
	u32 rsc_size;
	void (*show_rsc)(struct device *dev, void *_info);
	int (*prepare_rsc)(struct device *dev, struct rsc_node *rsc);
};

struct sunxi_rproc_rsc_helper_dev {
	struct device dev;
#ifdef SUNXI_RPROC_RSC_HELPER_SUBDEV
	struct rproc_subdev subdev;
#endif
	struct rproc *rproc;

	bool clan_list;
	struct mutex list_lock;
	struct list_head data_list;

	void (*on_release)(void *priv);
	void *on_release_priv;
};

#define dev_to_rsc_helper_dev(_dev) \
	container_of(_dev, struct sunxi_rproc_rsc_helper_dev, dev)

#define subdev_to_rsc_helper_dev(_subdev) \
	container_of(_subdev, struct sunxi_rproc_rsc_helper_dev, subdev)

int sunxi_rproc_rsc_helper_dev_register(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
					struct rproc *rproc);
int sunxi_rproc_rsc_helper_dev_unregister(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev);
int sunxi_rproc_rsc_helper_dev_set_release(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
					   void (*release)(void *), void *priv);
int sunxi_rproc_rsc_helper_handle_rsc(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
				      struct rproc *rproc, u32 rsc_type, void *rsc, int offset,
				      int avail);
int sunxi_rproc_rsc_helper_dev_set_parent(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
					  struct device *parent);

#endif /* SUNXI_RPROC_RSC_HELPER_DEV_H */
