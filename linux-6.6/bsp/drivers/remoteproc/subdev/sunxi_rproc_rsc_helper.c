// SPDX-License-Identifier: GPL-2.0
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

#include "sunxi_rproc_rsc_helper.h"
#include <linux/pm_domain.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/list.h>
#include <linux/version.h>
#include <remoteproc_internal.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#endif

static const struct rsc_ops * const rsc_ops_table[] = {
#if IS_ENABLED(CONFIG_AW_RPROC_USER_RESOURCE)
	&user_resource_ops,
#endif
};

int load_from_file(const char *path, void *dst, size_t size)
{
	struct file *filp;
	int ret, bytes = 0;

	if (!path || !dst || !size)
		return -EINVAL;

	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp))
		return PTR_ERR(filp);

	while (bytes < size) {
		ret = kernel_read(filp, dst + bytes, size - bytes, &filp->f_pos);
		if (ret < 0)
			goto err_out;
		else if (ret > 0)
			bytes += ret;
		else
			break; // success ?
	}

	filp_close(filp, NULL);
	return bytes;
err_out:
	filp_close(filp, NULL);
	memset(dst, 0, size);
	return ret;
}
EXPORT_SYMBOL(load_from_file);

int load_from_partition(const char *partition, void *dst, size_t size)
{
	char path[64];

	memset(path, 0, sizeof(path));
	scnprintf(path, sizeof(path) - 1, "/dev/by-name/%s", partition);

	return load_from_file(path, dst, size);
}
EXPORT_SYMBOL(load_from_partition);

int copy_to_rproc_da(struct rproc *rproc, u64 da, const void *src, size_t size)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
	void *va = rproc_da_to_va(rproc, da, size, NULL);
#else
	void *va = rproc_da_to_va(rproc, da, size);
#endif

	if (!va)
		return -ENOMEM;

	memcpy(va, src, size);
	return 0;
}
EXPORT_SYMBOL(copy_to_rproc_da);

static inline int prepare_all_rsc(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev)
{
	struct rsc_node *rsc, *rsc_tmp;
	const struct rsc_ops *ops;
	int ret = 0;

	mutex_lock(&rsc_helper_dev->list_lock);
	list_for_each_entry_safe(rsc, rsc_tmp, &rsc_helper_dev->data_list, list) {
		ops = rsc->ops;
		if (ops->prepare_rsc)
			ret += ops->prepare_rsc(&rsc_helper_dev->dev, rsc);
	}
	mutex_unlock(&rsc_helper_dev->list_lock);

	return ret;
}

static inline void unregister_all_rsc(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev)
{
	struct rsc_node *rsc, *rsc_tmp;

	mutex_lock(&rsc_helper_dev->list_lock);
	list_for_each_entry_safe(rsc, rsc_tmp, &rsc_helper_dev->data_list, list) {
		if (rsc) {
			list_del(&rsc->list);
			if (rsc->cache)
				kfree(rsc->cache);
			kfree(rsc);
		}
	}
	mutex_unlock(&rsc_helper_dev->list_lock);
}

const struct rsc_ops *rsc_type_match(u32 rsc_type, int avail)
{
	const struct rsc_ops *ops;
	int i;

	for (i = 0; i < ARRAY_SIZE(rsc_ops_table); i++) {
		ops = rsc_ops_table[i];
		if (ops->type == rsc_type && avail >= ops->rsc_size)
			return ops;
	}

	return NULL;
}

static inline int register_rsc(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
			       const struct rsc_ops *ops, void *info)
{
	int rsc_size = ops->rsc_size;
	struct rsc_node *rsc = kzalloc(sizeof(*rsc), GFP_KERNEL);

	if (!rsc || rsc_size < 0)
		return -ENOMEM;

	rsc->ops = ops;
	memcpy(&rsc->info, info, rsc_size);
	INIT_LIST_HEAD(&rsc->list);

	dev_info(&rsc_helper_dev->dev, "register_rsc: %s\n", ops->name);

	mutex_lock(&rsc_helper_dev->list_lock);
	list_add(&rsc->list, &rsc_helper_dev->data_list);
	mutex_unlock(&rsc_helper_dev->list_lock);

	return 0;
}

#ifdef SUNXI_RPROC_RSC_HELPER_CLASS
struct class rsc_helper_class = {
	.name		= "rsc_helper",
};

int rsc_helper_class_init;

static inline struct class *get_rsc_helper_class(void)
{
	int ret;

	if (rsc_helper_class_init)
		return &rsc_helper_class;

	ret = class_register(&rsc_helper_class);
	if (ret) {
		pr_err("rsc_helper: unable to register class\n");
		return NULL;
	}

	rsc_helper_class_init = 1;
	return &rsc_helper_class;
}
#endif

static inline
void sunxi_rproc_rsc_helper_trace(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
				  const char *event)
{
	struct device *dev = &rsc_helper_dev->dev;

	dev_dbg(dev, "%s\n", event);
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_rproc_rsc_helper_dev_resume(struct device *dev)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = dev_to_rsc_helper_dev(dev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "resume");
	return 0;
}

static int sunxi_rproc_rsc_helper_dev_suspend(struct device *dev)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = dev_to_rsc_helper_dev(dev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "suspend");
	return 0;
}

static struct dev_pm_domain pm_domain = {
	.ops = {
		.runtime_suspend = sunxi_rproc_rsc_helper_dev_suspend,
		.runtime_resume = sunxi_rproc_rsc_helper_dev_resume,
	},
};
#endif

static void sunxi_rproc_rsc_helper_release(struct device *dev)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = dev_to_rsc_helper_dev(dev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "release");

	mutex_destroy(&rsc_helper_dev->list_lock);

	if (rsc_helper_dev->on_release)
		rsc_helper_dev->on_release(rsc_helper_dev->on_release_priv);
}

static inline
int sunxi_rproc_rsc_helper_dev_init(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev)
{
	int ret;
	struct device *dev = &rsc_helper_dev->dev;

	mutex_init(&rsc_helper_dev->list_lock);
	device_initialize(dev);
	// the parent has not been init yet, this dev will be moved to the parent later.
	//dev->parent = &rsc_helper_dev->rproc->dev;

#if IS_ENABLED(CONFIG_PM)
	dev_pm_domain_set(dev, &pm_domain);
#endif

	dev_set_name(dev, "%s-rsc_helper", dev_name(&rsc_helper_dev->rproc->dev));
	dev->release = sunxi_rproc_rsc_helper_release;

#ifdef SUNXI_RPROC_RSC_HELPER_CLASS
	dev->class = get_rsc_helper_class();
#endif

	ret = device_add(dev);
	if (ret < 0) {
		dev_err(&rsc_helper_dev->rproc->dev, "device_add failed, ret: %d\n", ret);
		put_device(dev);
	}

	return ret;
}

#ifdef SUNXI_RPROC_RSC_HELPER_SUBDEV
static int sunxi_rproc_rsc_helper_subdev_prepare(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = subdev_to_rsc_helper_dev(subdev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "prepare");
	return prepare_all_rsc(rsc_helper_dev);
}

static int sunxi_rproc_rsc_helper_subdev_start(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = subdev_to_rsc_helper_dev(subdev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "start");
	return 0;
}

static void sunxi_rproc_rsc_helper_subdev_stop(struct rproc_subdev *subdev, bool crashed)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = subdev_to_rsc_helper_dev(subdev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, crashed ? "stop crashed" : "stop normal");

	// rsc_table will not be parsed when crashed, so rsc not need to unregister at this time.
	rsc_helper_dev->clan_list = crashed ? false : true;
}

static void sunxi_rproc_rsc_helper_subdev_unprepare(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = subdev_to_rsc_helper_dev(subdev);

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "unprepare");
	if (rsc_helper_dev->clan_list) {
		sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "clean list");
		unregister_all_rsc(rsc_helper_dev);
	}
}
#endif

static inline
void sunxi_rproc_rsc_helper_dev_cleanup(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev)
{
	memset(rsc_helper_dev, 0, sizeof(*rsc_helper_dev));
	INIT_LIST_HEAD(&rsc_helper_dev->data_list);
	rsc_helper_dev->clan_list = true;
}

int sunxi_rproc_rsc_helper_dev_register(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
					struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	int ret;

	sunxi_rproc_rsc_helper_dev_cleanup(rsc_helper_dev);
	rsc_helper_dev->rproc = rproc;

	ret = sunxi_rproc_rsc_helper_dev_init(rsc_helper_dev);
	if (ret) {
		dev_err(dev, "sunxi_rproc_rsc_helper_dev_init failed! ret: %d", ret);
		goto err_out;
	}

#ifdef SUNXI_RPROC_RSC_HELPER_SUBDEV
	rsc_helper_dev->subdev.prepare = sunxi_rproc_rsc_helper_subdev_prepare;
	rsc_helper_dev->subdev.start = sunxi_rproc_rsc_helper_subdev_start;
	rsc_helper_dev->subdev.stop = sunxi_rproc_rsc_helper_subdev_stop;
	rsc_helper_dev->subdev.unprepare = sunxi_rproc_rsc_helper_subdev_unprepare;
#endif

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "register");
	return 0;
err_out:
	return ret;
}
EXPORT_SYMBOL(sunxi_rproc_rsc_helper_dev_register);

int sunxi_rproc_rsc_helper_dev_unregister(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev)
{
	struct device *dev = &rsc_helper_dev->dev;

	sunxi_rproc_rsc_helper_trace(rsc_helper_dev, "unregister");
	device_del(dev);
	put_device(dev);

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_rsc_helper_dev_unregister);

int sunxi_rproc_rsc_helper_dev_set_release(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
					   void (*release)(void *), void *priv)
{
	rsc_helper_dev->on_release = release;
	rsc_helper_dev->on_release_priv = priv;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_rsc_helper_dev_set_release);

int sunxi_rproc_rsc_helper_handle_rsc(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
				      struct rproc *rproc, u32 rsc_type, void *rsc, int offset,
				      int avail)
{
	struct device *dev = &rproc->dev;
	const struct rsc_ops *ops;
	int ret;

	if (!rsc_helper_dev) {
		dev_err(dev, "rsc_helper not probe!\n");
		return RSC_IGNORED;
	}
	dev = &rsc_helper_dev->dev;

	// not match
	ops = rsc_type_match(rsc_type, avail);
	if (!rproc || !ops)
		return RSC_IGNORED;

	// The fw has not been loaded yet, so data cannot be overwriten at this time.
	ret = register_rsc(rsc_helper_dev, ops, rsc);
	if (ret)
		return RSC_IGNORED;

	return RSC_HANDLED;
}
EXPORT_SYMBOL(sunxi_rproc_rsc_helper_handle_rsc);

int sunxi_rproc_rsc_helper_dev_set_parent(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev,
					  struct device *parent)
{
	return device_move(&rsc_helper_dev->dev, parent, DPM_ORDER_PARENT_BEFORE_DEV);
}
EXPORT_SYMBOL(sunxi_rproc_rsc_helper_dev_set_parent);
