/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/sbi.h>

static u32 time_to_wakeup_ms;
static u32 use_ultra_standby;
static u32 set_ldo_onoff;

#define PMU_NO_INTERRUPT	-1
#define SET_WAKEUP_TIME_MS(ms)  ((3 << 30) | (ms))

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t time_to_wakeup_ms_show(struct class *class, struct class_attribute *attr,
		char *buf)
#else
static ssize_t time_to_wakeup_ms_show(const struct class *class, const struct class_attribute *attr,
		char *buf)
#endif
{
	ssize_t size = 0;

	size = sprintf(buf, "%u\n", time_to_wakeup_ms);

	return size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t time_to_wakeup_ms_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#else
static ssize_t time_to_wakeup_ms_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	u32 value = 0;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		sunxi_err(NULL, "%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	time_to_wakeup_ms = value;

	sbi_andes_set_wakeup_source(SET_WAKEUP_TIME_MS(time_to_wakeup_ms), 1);

	sunxi_info(NULL, "time_to_wakeup_ms change to %d\n", time_to_wakeup_ms);

	return count;
}
static CLASS_ATTR_RW(time_to_wakeup_ms);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t wakeup_reason_show(struct class *class, struct class_attribute *attr,
		char *buf)
#else
static ssize_t wakeup_reason_show(const struct class *class, const struct class_attribute *attr,
		char *buf)
#endif
{
	ssize_t size = 0;
	u32 ret = 0;

	ret = sbi_andes_get_wakeup_source();
	size = sprintf(buf, "wakeup_reason:%d\n", ret);

	return size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t wakeup_reason_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#else
static ssize_t wakeup_reason_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	return count;
}
static CLASS_ATTR_RW(wakeup_reason);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t standby_ldo_onoff_show(struct class *class, struct class_attribute *attr,
		char *buf)
#else
static ssize_t standby_ldo_onoff_show(const struct class *class, const struct class_attribute *attr,
		char *buf)
#endif
{
	ssize_t size = 0;
	size = sprintf(buf, "%u\n", set_ldo_onoff);

	return size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t standby_ldo_onoff_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#else
static ssize_t standby_ldo_onoff_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	u32 value = 0;
	u32 ret = 0;

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		sunxi_err(NULL, "%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	set_ldo_onoff = value;

	ret = sbi_andes_set_ldo_onoff(set_ldo_onoff);
	return count;
}
static CLASS_ATTR_RW(standby_ldo_onoff);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t use_ultra_standby_show(struct class *class, struct class_attribute *attr,
		char *buf)
#else
static ssize_t use_ultra_standby_show(const struct class *class, const struct class_attribute *attr,
		char *buf)
#endif
{
	ssize_t size = 0;

	size = sprintf(buf, "%u\n", use_ultra_standby);

	return size;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t use_ultra_standby_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#else
static ssize_t use_ultra_standby_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	u32 value = 0;
	u32 ret = 0;

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		sunxi_err(NULL, "%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	use_ultra_standby = value;

	ret = sbi_andes_set_ultra_standby(use_ultra_standby);
	return count;
}
static CLASS_ATTR_RW(use_ultra_standby);

static struct attribute *ae350_standby_class_attrs[] = {
	&class_attr_time_to_wakeup_ms.attr,
	&class_attr_wakeup_reason.attr,
	&class_attr_standby_ldo_onoff.attr,
	&class_attr_use_ultra_standby.attr,
	NULL,
};
ATTRIBUTE_GROUPS(ae350_standby_class);

struct class ae350_standby_class = {
	.name = "ae350_standby",
	.class_groups = ae350_standby_class_groups,
};

static int __init ae350_standby_debug_init(void)
{
	int ret;

	ret = class_register(&ae350_standby_class);
	if (ret < 0)
		sunxi_err(NULL, "%s,%d err, ret:%d\n", __func__, __LINE__, ret);

	return ret;
}

static void __exit ae350_standby_debug_exit(void)
{
	class_unregister(&ae350_standby_class);
}

module_init(ae350_standby_debug_init);
module_exit(ae350_standby_debug_exit);

MODULE_DESCRIPTION("ANDES STANDBY DEBUG");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALLWINNER");
MODULE_VERSION("1.0.0");
