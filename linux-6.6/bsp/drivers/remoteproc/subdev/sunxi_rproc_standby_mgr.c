// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi's rproc rsc helper dev driver
 * register or unregister rsc helper device for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: chenjinkun <chenjinkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sunxi_rproc_standby_mgr.h"

#define dev_to_stb_dev(_dev)	container_of(_dev, struct sunxi_rproc_standby_dev, dev)

static ssize_t standby_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_standby_dev *stby_dev = dev_to_stb_dev(dev);

	switch (stby_dev->stby_mode) {
	case RPROC_STANDBY_STAY_ALIVE:
		return sprintf(buf, "rproc stay alive when sys standby\n");
	case RPROC_STANDBY_STAY_DEEP_SLEEP:
		return sprintf(buf, "rproc stay deep sleep when sys standby\n");
	default:
		return sprintf(buf, "unknow standby mode!!!\n");
	}
}

static ssize_t standby_mode_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct sunxi_rproc_standby_dev *stdy_dev = dev_to_stb_dev(dev);

	if (!strncmp(buf, "alive", 5)) {
		if (stdy_dev->stby_mode == RPROC_STANDBY_STAY_ALIVE)
			return count;
		stdy_dev->stby_mode = RPROC_STANDBY_STAY_ALIVE;
	} else if (!strncmp(buf, "sleep", 5)) {
		if (stdy_dev->stby_mode == RPROC_STANDBY_STAY_DEEP_SLEEP)
			return count;
		stdy_dev->stby_mode = RPROC_STANDBY_STAY_DEEP_SLEEP;
	}  else {
		pr_err("Invalid param!\n");
	}

	return count;
}

static DEVICE_ATTR(standby_mode, 0660, standby_mode_show, standby_mode_store);

static struct attribute *stdby_mode_attr[] = {
	&dev_attr_standby_mode.attr,
	NULL
};

static const struct attribute_group stdby_mode_group = {
	.name = "standby_mode",
	.attrs = stdby_mode_attr
};

static const struct attribute_group *stdby_mode_groups[] = {
	&stdby_mode_group,
	NULL
};

static int stdby_class_init;
struct class stdby_class = {
	.name		= "rproc_stdby_mode",
	.dev_groups	= stdby_mode_groups,
};

static inline struct class *get_stdby_class(void)
{
	int ret;

	if (stdby_class_init)
		return &stdby_class;

	ret = class_register(&stdby_class);
	if (ret) {
		pr_err("rproc_stdby: unable to register class\n");
		return NULL;
	}

	stdby_class_init = 1;
	return &stdby_class;
}

static void sunxi_rproc_stdby_release(struct device *dev)
{
	return;
}

static inline int sunxi_rproc_stdby_dev_init(struct sunxi_rproc_standby_dev *stdby_dev)
{
	int ret;
	struct device *dev = &stdby_dev->dev;

	stdby_dev->stby_mode = RPROC_STANDBY_STAY_DEEP_SLEEP;

	device_initialize(dev);
	// the parent has not been init yet, this dev will be moved to the parent later.
	//dev->parent = &stdby_dev->rproc->dev;

	dev_set_name(dev, "%s-stdby", dev_name(&stdby_dev->rproc->dev));
	dev->release = sunxi_rproc_stdby_release;
	dev->class = get_stdby_class();

	if (dev->class)
		pr_err("%s %s\n", __func__, dev_name(&stdby_dev->rproc->dev));

	ret = device_add(dev);
	if (ret < 0) {
		dev_err(&stdby_dev->rproc->dev, "device_add failed, ret: %d\n", ret);
		put_device(dev);
	}

	return ret;
}

int sunxi_rproc_standby_dev_register(struct sunxi_rproc_standby_dev *stdby_dev, struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	int ret;

	stdby_dev->rproc = rproc;

	ret = sunxi_rproc_stdby_dev_init(stdby_dev);
	if (ret) {
		dev_err(dev, "sunxi_rproc_stdby_dev_init failed! ret: %d", ret);
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_standby_dev_register);

int sunxi_rproc_standby_dev_unregister(struct sunxi_rproc_standby_dev *stdby_dev)
{
	struct device *dev = &stdby_dev->dev;

	device_del(dev);
	put_device(dev);

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_standby_dev_unregister);

int sunxi_rproc_standby_dev_set_release(struct sunxi_rproc_standby_dev *stdby_dev,
				    void (*release)(void *), void *priv)
{
	stdby_dev->on_release = release;
	stdby_dev->on_release_priv = priv;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_standby_dev_set_release);

int sunxi_rproc_standby_dev_set_parent(struct sunxi_rproc_standby_dev *stdby_dev,
					  struct device *parent)
{
	return device_move(&stdby_dev->dev, parent, DPM_ORDER_PARENT_BEFORE_DEV);
}
EXPORT_SYMBOL(sunxi_rproc_standby_dev_set_parent);