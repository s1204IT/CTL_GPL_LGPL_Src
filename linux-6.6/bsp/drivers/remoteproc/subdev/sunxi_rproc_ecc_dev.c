// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi's rproc ecc dev driver
 * register or unregister ecc device for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: chenjinkun <chenjinkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sunxi_rproc_ecc_dev.h"
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

#define dev_to_ecc_dev(_dev)		container_of(_dev, struct sunxi_rproc_ecc_dev, dev)

static irqreturn_t sunxi_rproc_ecc_handler(int irq, void *dev_id)
{
	struct sunxi_rproc_ecc_dev *ecc = dev_id;
	schedule_work(&ecc->work);

	return IRQ_HANDLED;
}

static void sunxi_rproc_ecc_irq_work(struct work_struct *work)
{
	struct sunxi_rproc_ecc_dev *ecc_dev;

	ecc_dev = container_of(work, struct sunxi_rproc_ecc_dev, work);

	ecc_dev->error_flag = true;

	writel(0x01, ecc_dev->base + RV_CFG_ICACHE_ECC_IRQ_CLEAR_REG);
	writel(0x01, ecc_dev->base + RV_CFG_DCACHE_ECC_IRQ_CLEAR_REG);

	writel(0x00, ecc_dev->base + RV_CFG_ICACHE_ECC_IRQ_CLEAR_REG);
	writel(0x00, ecc_dev->base + RV_CFG_DCACHE_ECC_IRQ_CLEAR_REG);

	dev_err(&ecc_dev->dev, "sunxi_rproc_ecc_dev, ecc error happen!!!\n");
}

static ssize_t ecc_error_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rproc_ecc_dev *ecc_dev = dev_to_ecc_dev(dev);

	return sprintf(buf, "%s\n", ecc_dev->error_flag ? "there was ecc error happened, \
			please check e907 ecc register with sunxi_dump" : "no ecc error");
}

static DEVICE_ATTR_RO(ecc_error_type);

static struct attribute *ecc_attrs[] = {
	&dev_attr_ecc_error_type.attr,
	NULL
};

static const struct attribute_group ecc_devgroup = {
	.attrs = ecc_attrs
};

static const struct attribute_group *ecc_devgroups[] = {
	&ecc_devgroup,
	NULL
};

int ecc_class_init;
struct class ecc_class = {
	.name		= "rproc_ecc",
	.dev_groups	= ecc_devgroups,
};

static inline struct class *get_ecc_class(void)
{
	int ret;

	if (ecc_class_init)
		return &ecc_class;

	ret = class_register(&ecc_class);
	if (ret) {
		pr_err("rproc_ecc: unable to register class\n");
		return NULL;
	}

	ecc_class_init = 1;
	return &ecc_class;
}

static void sunxi_rproc_ecc_release(struct device *dev)
{
	return;
}

static inline int sunxi_rproc_ecc_dev_init(struct sunxi_rproc_ecc_dev *ecc_dev)
{
	int ret;
	struct device *dev = &ecc_dev->dev;

	device_initialize(dev);
	// the parent has not been init yet, this dev will be moved to the parent later.
	//dev->parent = &ecc_dev->rproc->dev;

	dev_set_name(dev, "%s-ecc", dev_name(&ecc_dev->rproc->dev));
	dev->release = sunxi_rproc_ecc_release;
	dev->class = get_ecc_class();

	if (dev->class)
		pr_err("%s %s\n", __func__, dev_name(&ecc_dev->rproc->dev));

	ret = device_add(dev);
	if (ret < 0) {
		dev_err(&ecc_dev->rproc->dev, "device_add failed, ret: %d\n", ret);
		put_device(dev);
	}

	return ret;
}

int sunxi_rproc_ecc_dev_register(struct sunxi_rproc_ecc_dev *ecc_dev, struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	int ret;

	ecc_dev->rproc = rproc;

	INIT_WORK(&ecc_dev->work, sunxi_rproc_ecc_irq_work);

	ret = sunxi_rproc_ecc_dev_init(ecc_dev);
	if (ret) {
		dev_err(dev, "sunxi_rproc_ecc_dev_init failed! ret: %d", ret);
		return -1;
	}

	ret = devm_request_irq(&ecc_dev->dev, ecc_dev->irq_num, sunxi_rproc_ecc_handler,
			       IRQF_SHARED, "rproc_ecc_irq", ecc_dev);
	if (ret) {
		ecc_dev->irq_num = -1;
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_ecc_dev_register);

int sunxi_rproc_ecc_dev_unregister(struct sunxi_rproc_ecc_dev *ecc_dev)
{
	struct device *dev = &ecc_dev->dev;

	device_del(dev);
	put_device(dev);

	if (ecc_dev->irq_num >= 0) {
		free_irq(ecc_dev->irq_num, ecc_dev);
		ecc_dev->irq_num = -1;
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_ecc_dev_unregister);

int sunxi_rproc_ecc_dev_set_release(struct sunxi_rproc_ecc_dev *ecc_dev,
				    void (*release)(void *), void *priv)
{
	ecc_dev->on_release = release;
	ecc_dev->on_release_priv = priv;

	cancel_work_sync(&ecc_dev->work);

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_ecc_dev_set_release);

int sunxi_rproc_ecc_dev_set_parent(struct sunxi_rproc_ecc_dev *ecc_dev,
					  struct device *parent)
{
	return device_move(&ecc_dev->dev, parent, DPM_ORDER_PARENT_BEFORE_DEV);
}
EXPORT_SYMBOL(sunxi_rproc_ecc_dev_set_parent);