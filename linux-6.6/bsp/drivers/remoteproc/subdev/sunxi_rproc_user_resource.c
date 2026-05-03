// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi's rproc user data handle
 * operation interface of user data handle for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sunxi_rproc_rsc_helper.h"
#include "sunxi_rproc_rsc_helper_i.h"
#include <linux/slab.h>

#define LOAD_FROM_PARTITION		(0xff)

#define USER_RESOURCE_TYPE		(RSC_VENDOR_START + 2)

static inline int load_user_resource(struct device *dev, struct rsc_node *rsc)
{
	int ret;
	struct fw_rsc_user_resource *info = &rsc->info.user_resource;

	switch (info->src_type) {
	case LOAD_FROM_PARTITION:
		ret = load_from_partition(info->src_name, rsc->cache, info->len);
		dev_info(dev, "user_resource: load_from_partition(%s, %u) return %d\n",
			 info->src_name, (unsigned int)info->len, ret);
		return ret;
	default:
		dev_info(dev, "src type unknown: %u\n", (unsigned int)info->src_type);
		return -EINVAL;
	}
}

static void show_rsc_user_resource(struct device *dev, void *_info)
{
	struct fw_rsc_user_resource *info = _info;

	dev_info(dev, "user_resource: src_type: %u, da: %lx, len: %u, reserved: %u, name: %s\n",
		 (unsigned int)info->src_type, (unsigned long)info->da,
		 (unsigned int)info->len, (unsigned int)info->reserved, info->src_name);
}

static int prepare_rsc_user_resource(struct device *dev, struct rsc_node *rsc)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = dev_to_rsc_helper_dev(dev);
	struct rproc *rproc = rsc_helper_dev->rproc;
	struct fw_rsc_user_resource *info = &rsc->info.user_resource;
	int ret;

	if (rproc->state == RPROC_DETACHED || rproc->state == RPROC_ATTACHED) {
		// if booted by bootloader
		dev_info(dev, "user_resource: %s, rproc has been booted, skip\n", info->src_name);
		ret = 0;
		goto ignore_out;
	}

	if (!rsc->cache) {
		rsc->cache = kzalloc(info->len, GFP_KERNEL);
		if (!rsc->cache) {
			ret = -ENOMEM;
			goto err_out;
		}
		ret = load_user_resource(dev, rsc);
		if (ret < 0)
			goto err_out;
	}

	ret = copy_to_rproc_da(rproc, (u64)info->da, rsc->cache, info->len);
	if (ret) {
		dev_err(dev, "user_resource: copy_to_rproc_da(%lu, %u) failed! return %d\n",
			(unsigned long)info->da, (unsigned int)info->len, ret);
		goto err_out;
	}

	// this rsc's src may be overwriten by user, so it need be reload when next time.
	kfree(rsc->cache);
	rsc->cache = NULL;

	ret = 0;
err_out:
	if (ret && rsc->cache) {
		kfree(rsc->cache);
		rsc->cache = NULL;
	}
ignore_out:
	return ret;
}

const struct rsc_ops user_resource_ops = {
	.name = "user_resource",
	.type = USER_RESOURCE_TYPE,
	.rsc_size = sizeof(struct fw_rsc_user_resource),
	.show_rsc = show_rsc_user_resource,
	.prepare_rsc = prepare_rsc_user_resource,
};
