// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2024 geyoupengs@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"
#include <asm/io.h>

#define SUN300IW1_IRCUT_VERSION          "0.0.1"

static struct class ircut_class = {
	.name	=	"ircut",
	.owner	=	THIS_MODULE,
};

struct ircut_prvdata {
	u32 __iomem *addr;
	struct resource *res;
	struct platform_device  *pdev;
	struct device *dev;
	struct class *class;
	struct device *class_dev;
	raw_spinlock_t lock;
};

static ssize_t
ircut_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ircut_prvdata *chip = dev_get_drvdata(dev);
	unsigned long flags;
	u32 reg_val = 0;

	raw_spin_lock_irqsave(&chip->lock, flags);
	reg_val = readl(chip->addr);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	if (reg_val & (3))
		sunxi_info(NULL, "enabled\n");
	else
		sunxi_info(NULL, "disabled\n");

	return 0;
}

static ssize_t
ircut_status_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct ircut_prvdata *chip = dev_get_drvdata(dev);
	unsigned long flags;
	unsigned enable_flag;
	char param[10];
	u32 reg_val = 0;

	if (count > 10) {
		sunxi_info(NULL, "param is only enable or disable\n");
		return -EINVAL;
	}
	strlcpy(param, buf, count);

	if (!strcmp(param, "enable")) {
		enable_flag = 1;
	} else if (!strcmp(param, "disable")) {
		enable_flag = 0;
	} else {
		sunxi_info(NULL, "param is only enable or disable\n");
		return -EINVAL;
	}

	raw_spin_lock_irqsave(&chip->lock, flags);
	reg_val = readl(chip->addr);

	if (enable_flag)
		reg_val |= 0x3;
	else
		reg_val &= ~(0x3);

	writel(reg_val, chip->addr);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	return count;
}

static struct device_attribute ircut_attrs[] = {
	__ATTR(ircut_ctr,   0644, ircut_status_show,  ircut_status_store),
};

static int sun300iw1_sysfs_crete(struct ircut_prvdata *chip)
{
	int ret;

	ret = class_register(&ircut_class);
	if (ret) {
		sunxi_err(NULL, "register ircut_class failed. err=%d\n", ret);
		return ret;
	}

	chip->class_dev = device_create(&ircut_class, chip->dev, 1, chip, "ircut");

	 /* need some class specific sysfs attributes */
	ret = device_create_file(chip->class_dev, &ircut_attrs[0]);
	if (ret) {
		sunxi_err(NULL, "%s(): class_create_file() failed. err=%d\n", __func__, ret);
		device_remove_file(chip->class_dev, &ircut_attrs[0]);
		device_destroy(&ircut_class, 1);
		class_unregister(&ircut_class);
		return ret;
	}

	return 0;
}
static int sun300iw1_ircut_probe(struct platform_device *pdev)
{
	int ret;
	struct ircut_prvdata *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (IS_ERR_OR_NULL(chip))
		return -ENOMEM;

	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	chip->res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ircut");
	if (chip->res) {
		chip->addr = ioremap(chip->res->start, resource_size(chip->res));
		if (!chip->addr)
			return -ENXIO;
	}

	ret = sun300iw1_sysfs_crete(chip);
	if (ret) {
		sunxi_err(chip->dev, "failed to node_init\n");
		return ret;
	}

	platform_set_drvdata(pdev, chip);
	sunxi_info(NULL, "sunxi ircut version: %s\n", SUN300IW1_IRCUT_VERSION);

	return 0;
}

static struct of_device_id sun300iw1_ircut_match[] = {
	{ .compatible = "allwinner,sun300iw1-ircut", },
	{}
};
MODULE_DEVICE_TABLE(of, sun300iw1_ircut_match);

static struct platform_driver sun300iw1_ircut_driver = {
	.probe	= sun300iw1_ircut_probe,
	.driver	= {
		.name		= "sun300iw1-ircut",
		.of_match_table	= sun300iw1_ircut_match,
	},
};

static int __init sun300iw1_ircut_init(void)
{
	return platform_driver_register(&sun300iw1_ircut_driver);
}
postcore_initcall(sun300iw1_ircut_init);

MODULE_DESCRIPTION("Allwinner sun300iw1 Ircut driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<geyoupengs@allwinnertech>");
MODULE_VERSION(SUN300IW1_IRCUT_VERSION);
