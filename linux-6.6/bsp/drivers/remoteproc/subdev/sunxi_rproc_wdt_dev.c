// SPDX-License-Identifier: GPL-2.0
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

#include "sunxi_rproc_wdt_dev.h"
#include <linux/pm_domain.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#define wdt_to_wdt_dev(_wdt)		container_of(_wdt, struct sunxi_rproc_wdt_dev, wdt)
#define dev_to_wdt_dev(_dev)		container_of(_dev, struct sunxi_rproc_wdt_dev, dev)
#define subdev_to_wdt_dev(_subdev)	container_of(_subdev, struct sunxi_rproc_wdt_dev, subdev)

static inline void sunxi_rproc_wdt_trace(struct sunxi_rproc_wdt_dev *wdt_dev, const char *event)
{
	struct device *dev = &wdt_dev->dev;

	dev_dbg(dev, "%s\n", event);
}

static inline const char *wdt_dev_reset_type_str(u32 reset_type)
{
	if (reset_type == RESET_ALL)
		return "rst_all";
	else if (reset_type == RESET_CORE)
		return "rst_core";
	else if (reset_type == RESET_NONE)
		return "rst_none";
	else
		return "unknown";
}

#ifdef SUNXI_RPROC_WDT_CLASS
static ssize_t enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	int enable = sunxi_rproc_wdt_is_enable(&wdt_dev->wdt);

	return sprintf(buf, "%s\n", enable ? "enabled" : "disabled");
}

static ssize_t reset_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	int ret, enable = sunxi_rproc_wdt_is_enable(&wdt_dev->wdt);
	u32 val;
	const char *val_str;

	ret = sunxi_rproc_wdt_get_reset_type(&wdt_dev->wdt, &val);
	if (ret)
		val_str = "error";
	else if (val == RESET_SYS)
		val_str = "rst_sys";
	else if (val == RESET_INT)
		val_str = "rst_int";
	else
		val_str = "unknown";

	return sprintf(buf, "%s%s\n", val_str, enable ? "" : " (disabled)");
}

static ssize_t reset_type_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	int ret;
	u32 val;

	if (sysfs_streq(buf, "rst_sys"))
		val = RESET_SYS;
	else if (sysfs_streq(buf, "rst_int"))
		val = RESET_INT;
	else
		return -EINVAL;

	ret = sunxi_rproc_wdt_set_reset_type(&wdt_dev->wdt, val);
	if (ret) {
		dev_err(dev, "sunxi_rproc_wdt_set_reset_type failed! ret: %d\n", ret);
		return ret;
	}

	return count;
}

static ssize_t timeout_ms_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	int ret, enable = sunxi_rproc_wdt_is_enable(&wdt_dev->wdt);
	u32 val;

	ret = sunxi_rproc_wdt_get_timeout(&wdt_dev->wdt, &val);

	return sprintf(buf, "%u%s\n", val, enable ? "" : " (disabled)");
}

static ssize_t timeout_ms_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	int ret;
	long val;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	if (val <= 0)
		return -EINVAL;

	ret = sunxi_rproc_wdt_set_timeout(&wdt_dev->wdt, (u32)val);
	if (ret) {
		dev_err(dev, "sunxi_rproc_wdt_set_timeout failed! ret: %d\n", ret);
		return ret;
	}

	return count;
}

static ssize_t reset_type_default_show(struct device *dev, struct device_attribute *attr,
				       char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	const char *val_str = wdt_dev_reset_type_str(wdt_dev->reset_type);

	return sprintf(buf, "%s\n", val_str);
}

static ssize_t reset_type_default_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	u32 *val = &wdt_dev->reset_type;

	if (sysfs_streq(buf, "rst_all"))
		*val = RESET_ALL;
	else if (sysfs_streq(buf, "rst_core"))
		*val = RESET_CORE;
	else if (sysfs_streq(buf, "rst_none"))
		*val = RESET_NONE;
	else
		return -EINVAL;

	return count;
}

static ssize_t timeout_ms_default_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	return sprintf(buf, "%u\n", wdt_dev->wdt.param.timeout_ms);
}

static ssize_t timeout_ms_default_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	long val;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	if (val <= 0)
		return -EINVAL;

	wdt_dev->wdt.param.timeout_ms = (u32)val;

	return count;
}

static ssize_t try_times_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	return sprintf(buf, "%u\n", wdt_dev->try_times);
}

static ssize_t try_times_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);
	long val;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	if (val <= 0)
		return -EINVAL;

	wdt_dev->try_times = (u32)val;

	return count;
}

static ssize_t timeout_cnt_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	return sprintf(buf, "%u\n", wdt_dev->timeout_cnt);
}

// hardware
static DEVICE_ATTR_RO(enable);
static DEVICE_ATTR_RW(reset_type);
static DEVICE_ATTR_RW(timeout_ms);
// config
static DEVICE_ATTR_RW(reset_type_default);
static DEVICE_ATTR_RW(timeout_ms_default);
static DEVICE_ATTR_RW(try_times);
// stat
static DEVICE_ATTR_RO(timeout_cnt);

static struct attribute *wdt_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_reset_type.attr,
	&dev_attr_timeout_ms.attr,
	&dev_attr_reset_type_default.attr,
	&dev_attr_timeout_ms_default.attr,
	&dev_attr_try_times.attr,
	&dev_attr_timeout_cnt.attr,
	NULL
};

static const struct attribute_group wdt_devgroup = {
	.attrs = wdt_attrs
};

static const struct attribute_group *wdt_devgroups[] = {
	&wdt_devgroup,
	NULL
};

static int wdt_class_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	int ret;
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	ret = add_uevent_var(env, "RESET_TYPE=%s", wdt_dev_reset_type_str(wdt_dev->reset_type));
	if (ret)
		return ret;

	ret = add_uevent_var(env, "TIMEOUT_MS=%u", wdt_dev->wdt.param.timeout_ms);
	if (ret)
		return ret;

	ret = add_uevent_var(env, "TRY_TIMES=%u", wdt_dev->try_times);
	if (ret)
		return ret;

	ret = add_uevent_var(env, "TIMEOUT_CNT=%u", wdt_dev->timeout_cnt);
	if (ret)
		return ret;

	return ret;
}

struct class wdt_class = {
	.name		= "rproc_wdt",
	.dev_groups	= wdt_devgroups,
	.dev_uevent	= wdt_class_uevent,
};

int wdt_class_init;

static inline struct class *get_wdt_class(void)
{
	int ret;

	if (wdt_class_init)
		return &wdt_class;

	ret = class_register(&wdt_class);
	if (ret) {
		pr_err("rproc_wdt: unable to register class\n");
		return NULL;
	}

	wdt_class_init = 1;
	return &wdt_class;
}
#endif

static inline void sunxi_rproc_wdt_notify(struct sunxi_rproc_wdt_dev *wdt_dev, const char *event)
{
	struct device *dev = &wdt_dev->dev;
	char tmp[32];
	char *envp[2];
	int ret = snprintf(tmp, sizeof(tmp) - 1, "EVENT=%s", event);

	sunxi_rproc_wdt_trace(wdt_dev, "notify");
	tmp[ret] = '\0';
	envp[0] = tmp;
	envp[1] = NULL;

	ret = kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
	if (ret)
		dev_err(dev, "kobject_uevent_env return: %d\n", ret);
}

static inline void sunxi_rproc_wdt_reset(struct sunxi_rproc_wdt_dev *wdt_dev)
{
	struct device *dev = &wdt_dev->dev;

	wdt_dev->timeout_cnt++;

	if (wdt_dev->reset_type == RESET_NONE) {
		dev_info(dev, "detected timeout\n");
		return;
	}

	if (wdt_dev->try_times > 0 && wdt_dev->timeout_cnt > wdt_dev->try_times) {
		dev_info(dev, "detected timeout, timeout_cnt: %u, try_times: %u\n",
			 wdt_dev->timeout_cnt, wdt_dev->try_times);
		return;
	}

	dev_info(dev, "detected timeout & report crash, timeout_cnt: %u, try_times: %u\n",
		 wdt_dev->timeout_cnt, wdt_dev->try_times);
	rproc_report_crash(wdt_dev->rproc, RPROC_WATCHDOG);
}

static void sunxi_rproc_wdt_callback(struct sunxi_rproc_wdt *wdt, void *priv)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = wdt_to_wdt_dev(wdt);

	sunxi_rproc_wdt_trace(wdt_dev, "callback");
	sunxi_rproc_wdt_reset(wdt_dev);
	sunxi_rproc_wdt_notify(wdt_dev, "WDT_TIMEOUT");
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_rproc_wdt_dev_resume(struct device *dev)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	sunxi_rproc_wdt_trace(wdt_dev, "resume");
	return sunxi_rproc_wdt_resume(&wdt_dev->wdt);
}

static int sunxi_rproc_wdt_dev_suspend(struct device *dev)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	sunxi_rproc_wdt_trace(wdt_dev, "suspend");
	return sunxi_rproc_wdt_suspend(&wdt_dev->wdt);
}

static struct dev_pm_domain pm_domain = {
	.ops = {
		.runtime_suspend = sunxi_rproc_wdt_dev_suspend,
		.runtime_resume = sunxi_rproc_wdt_dev_resume,
	},
};
#endif

static void sunxi_rproc_wdt_release(struct device *dev)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = dev_to_wdt_dev(dev);

	sunxi_rproc_wdt_trace(wdt_dev, "release");
	sunxi_rproc_wdt_deinit(&wdt_dev->wdt);

	if (wdt_dev->on_release)
		wdt_dev->on_release(wdt_dev->on_release_priv);
}

static inline int sunxi_rproc_wdt_dev_init(struct sunxi_rproc_wdt_dev *wdt_dev)
{
	int ret;
	struct device *dev = &wdt_dev->dev;

	device_initialize(dev);
	// the parent has not been init yet, this dev will be moved to the parent later.
	//dev->parent = &wdt_dev->rproc->dev;

#if IS_ENABLED(CONFIG_PM)
	dev_pm_domain_set(dev, &pm_domain);
#endif

	dev_set_name(dev, "%s-wdt", dev_name(&wdt_dev->rproc->dev));
	dev->release = sunxi_rproc_wdt_release;

#ifdef SUNXI_RPROC_WDT_CLASS
	dev->class = get_wdt_class();
#endif

	ret = device_add(dev);
	if (ret < 0) {
		dev_err(&wdt_dev->rproc->dev, "device_add failed, ret: %d\n", ret);
		put_device(dev);
	}

	return ret;
}

#ifdef SUNXI_RPROC_WDT_SUBDEV
static int sunxi_rproc_wdt_subdev_prepare(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = subdev_to_wdt_dev(subdev);
	int ret;

	sunxi_rproc_wdt_trace(wdt_dev, "prepare");

	if (wdt_dev->rproc->state == RPROC_DETACHED || wdt_dev->rproc->state == RPROC_ATTACHED) {
		dev_info(&wdt_dev->dev, "rproc has been booted, "
			 "note that the config will be overwritten\n");
	}

	if (wdt_dev->reset_type == RESET_NONE)
		wdt_dev->timeout_cnt = 0;
	else if (wdt_dev->try_times > 0 && wdt_dev->timeout_cnt > wdt_dev->try_times)
		wdt_dev->timeout_cnt = 0;

	if (wdt_dev->reset_type == RESET_ALL)
		wdt_dev->wdt.param.reset_type = RESET_SYS;

	ret = sunxi_rproc_wdt_start(&wdt_dev->wdt);
	if (ret)
		sunxi_rproc_wdt_notify(wdt_dev, "WDT_START_FAILED");
	else
		sunxi_rproc_wdt_notify(wdt_dev, "WDT_RUNNING");

	return ret;
}

static int sunxi_rproc_wdt_subdev_start(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = subdev_to_wdt_dev(subdev);

	sunxi_rproc_wdt_trace(wdt_dev, "start");
	return 0;
}

static void sunxi_rproc_wdt_subdev_stop(struct rproc_subdev *subdev, bool crashed)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = subdev_to_wdt_dev(subdev);

	sunxi_rproc_wdt_trace(wdt_dev, crashed ? "stop crashed" : "stop normal");
}

static void sunxi_rproc_wdt_subdev_unprepare(struct rproc_subdev *subdev)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = subdev_to_wdt_dev(subdev);

	sunxi_rproc_wdt_trace(wdt_dev, "unprepare");
	sunxi_rproc_wdt_stop(&wdt_dev->wdt);
	sunxi_rproc_wdt_notify(wdt_dev, "WDT_STOPPED");
}
#endif

static inline void sunxi_rproc_wdt_dev_cleanup(struct sunxi_rproc_wdt_dev *wdt_dev)
{
	memset(wdt_dev, 0, sizeof(*wdt_dev));
}

int sunxi_rproc_wdt_dev_register(struct sunxi_rproc_wdt_dev *wdt_dev,
				 struct rproc *rproc, struct sunxi_rproc_wdt_dev_res *res)
{
	struct device *dev = &rproc->dev;
	int ret;
	struct sunxi_rproc_wdt_param param = {
		.cb = sunxi_rproc_wdt_callback,
		.cb_priv = NULL,
	};

	sunxi_rproc_wdt_dev_cleanup(wdt_dev);
	wdt_dev->rproc = rproc;
	wdt_dev->reset_type =  res->reset_type;
	wdt_dev->try_times = 0;

	param.reg_res = res->reg_res;
	param.irq_num = res->irq_num;
	param.timeout_ms = res->timeout_ms;

	if (wdt_dev->reset_type == RESET_ALL)
		param.reset_type = RESET_SYS;
	else
		param.reset_type = RESET_INT;

	ret = sunxi_rproc_wdt_init(&wdt_dev->wdt, &param);
	if (ret) {
		dev_err(dev, "sunxi_rproc_wdt_init failed! ret: %d", ret);
		goto err_out;
	}

	ret = sunxi_rproc_wdt_dev_init(wdt_dev);
	if (ret) {
		dev_err(dev, "sunxi_rproc_wdt_dev_init failed! ret: %d", ret);
		goto err_out;
	}

#ifdef SUNXI_RPROC_WDT_SUBDEV
	wdt_dev->subdev.prepare = sunxi_rproc_wdt_subdev_prepare;
	wdt_dev->subdev.start = sunxi_rproc_wdt_subdev_start;
	wdt_dev->subdev.stop = sunxi_rproc_wdt_subdev_stop;
	wdt_dev->subdev.unprepare = sunxi_rproc_wdt_subdev_unprepare;
#endif

	sunxi_rproc_wdt_trace(wdt_dev, "register");
	return 0;
err_out:
	return ret;
}
EXPORT_SYMBOL(sunxi_rproc_wdt_dev_register);

int sunxi_rproc_wdt_dev_unregister(struct sunxi_rproc_wdt_dev *wdt_dev)
{
	struct device *dev = &wdt_dev->dev;

	sunxi_rproc_wdt_trace(wdt_dev, "unregister");
	device_del(dev);
	put_device(dev);

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_wdt_dev_unregister);

int sunxi_rproc_wdt_dev_set_release(struct sunxi_rproc_wdt_dev *wdt_dev,
				    void (*release)(void *), void *priv)
{
	wdt_dev->on_release = release;
	wdt_dev->on_release_priv = priv;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_wdt_dev_set_release);

int sunxi_rproc_wdt_dev_set_parent(struct sunxi_rproc_wdt_dev *wdt_dev, struct device *parent)
{
	return device_move(&wdt_dev->dev, parent, DPM_ORDER_PARENT_BEFORE_DEV);
}
EXPORT_SYMBOL(sunxi_rproc_wdt_dev_set_parent);

