/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//#define DEBUG
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/reset.h>

#if IS_ENABLED(CONFIG_IIO)
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#endif

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif

#define GIC_BASE 0x03400000
#define GIC_SIZE 0x1000
#define GICD_ISPENDR_BASE  0X200


static struct class *vmmsg_class;
static void __iomem *gic_base;
static int remote_irq;


struct sunxi_vmmsg {
	struct platform_device	*pdev;
	struct class *vmmsg_class;
	struct device *dev;
	struct resource *res;
	struct device_node *np;
	spinlock_t lock;  /* syn */
	void __iomem *reg_base;
	void __iomem *rx_base;
	void __iomem *tx_base;
	void __iomem *gic_base;
	int nirq;
	int remote_irq;
} vmmsg_chip;

enum vmmsg_data {
	SUNXI55IW3,
};

static struct of_device_id const sunxi_vmmsg_of_match[] = {
	{ .compatible = "allwinner,vmmsg", .data = (void *)SUNXI55IW3, },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_vmmsg_of_match);

static inline unsigned int OFFSET(unsigned int base, int irq)
{
	return (base + (irq >> 5) * 4);
}

static inline unsigned int BIT_VAL(int irq)
{
	return (1 << (irq % 32));
}

static irqreturn_t sunxi_vmmsg_irq_handler(int irq, void *dummy)
{
	//writel(PENDING_BIT, gic_base + CLEAR_PENDING_OFFSET);
	//unsigned int val = readl(gic_base + PENDING_OFFSET);
	struct sunxi_vmmsg *chip = &vmmsg_chip;
	pr_err("---- get data = %d ----\n", *(int *)chip->reg_base);

	return IRQ_HANDLED;
}

static int sunxi_vmmsg_resource_get(struct sunxi_vmmsg *chip)
{
	int err;

	chip->np = chip->dev->of_node;


	chip->reg_base = ioremap(0x4ac00000, 0x1000);
	if (IS_ERR(chip->reg_base)) {
		dev_err(chip->dev, "get ioremap resource failed\n");
		return PTR_ERR(chip->reg_base);
	}

	err = of_property_read_u32(chip->np, "remote-irq", &chip->remote_irq);
	if (err) {
		dev_err(chip->dev, "No remote-irq node\n");
		return err;
	}
	remote_irq = chip->remote_irq; // TODO

	chip->nirq = platform_get_irq(chip->pdev, 0);
	if (chip->nirq < 0)
		return chip->nirq;

	err = devm_request_irq(chip->dev, chip->nirq, sunxi_vmmsg_irq_handler, 0, "vmmsg", chip);
	if (err) {
		dev_err(chip->dev, "request irq failed\n");
		return err;
	}

	return 0;
}


static ssize_t sunxi_vmmsg_show(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", readl(gic_base + OFFSET(GICD_ISPENDR_BASE, remote_irq)));
}

static bool vmmsg_notify(unsigned int val)
{
	struct sunxi_vmmsg *chip = &vmmsg_chip;
	*(int *)chip->reg_base = val;
	pr_err("---- send data = %d ----\n", val);
	// pr_err("addr_offset = %x", OFFSET(GICD_ISPENDR_BASE, remote_irq));
	// pr_err("set_bit = %x", BIT_VAL(remote_irq));
	writel(BIT_VAL(remote_irq), gic_base + OFFSET(GICD_ISPENDR_BASE, remote_irq));

	return 0;
}


static ssize_t sunxi_vmmsg_store(struct class *class, struct class_attribute *attr,
	   const char *buf, size_t count)
{
	unsigned int user_data;

	if (kstrtouint(buf, 10, &user_data))
		return -EINVAL;

	vmmsg_notify(user_data);

	return count;
}

static struct class_attribute vmmsg_class_attrs[] = {
	__ATTR(irq,  0664, sunxi_vmmsg_show,  sunxi_vmmsg_store),
};

static int sunxi_vmmsg_probe(struct platform_device *pdev)
{

	int err, i;
	struct sunxi_vmmsg *chip = &vmmsg_chip;

	/* sys/class/vmmsg */
	vmmsg_class = class_create(THIS_MODULE, "vmmsg");
	if (IS_ERR(vmmsg_class)) {
		pr_err("%s:%u class_create() failed\n", __func__, __LINE__);
		return PTR_ERR(vmmsg_class);
	}

	/* sys/class/sunxi_dump/xxx */
	for (i = 0; i < ARRAY_SIZE(vmmsg_class_attrs); i++) {
		err = class_create_file(vmmsg_class, &vmmsg_class_attrs[i]);
		if (err) {
			pr_err("%s:%u class_create_file() failed. err=%d\n", __func__, __LINE__, err);
			while (i--) {
				class_remove_file(vmmsg_class, &vmmsg_class_attrs[i]);
			}
			class_destroy(vmmsg_class);
			vmmsg_class = NULL;
			return err;
		}
	}


	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	chip->gic_base = gic_base = ioremap(GIC_BASE, GIC_SIZE);
	if (!chip->gic_base) {
		err = -ENOMEM;
		goto err0;
	}

	err = sunxi_vmmsg_resource_get(chip);
	if (err) {
		dev_err(chip->dev, "sunxi vmmsg get resource failed\n");
		goto err0;
	}

err0:
	return err;
}

static int sunxi_vmmsg_remove(struct platform_device *pdev)
{
	return 0;
}

#if IS_ENABLED(CONFIG_PM)

static int sunxi_vmmsg_suspend(struct device *dev)
{
	// TODO
	return 0;
}

static int sunxi_vmmsg_resume(struct device *dev)
{
	// TODO
	return 0;
}

static const struct dev_pm_ops sunxi_vmmsg_pm_ops = {
	.suspend = sunxi_vmmsg_suspend,
	.resume = sunxi_vmmsg_resume,
};

#define SUNXI_VMMSG_PM_OPS (&sunxi_vmmsg_pm_ops)
#else
#define SUNXI_VMMSG_PM_OPS NULL
#endif

static struct platform_driver sunxi_vmmsg_driver = {
	.probe  = sunxi_vmmsg_probe,
	.remove = sunxi_vmmsg_remove,
	.driver = {
		.name   = "sunxi-vmmsg",
		.owner  = THIS_MODULE,
		.pm	= SUNXI_VMMSG_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_vmmsg_of_match),
	},
};

static int __init sunxi_vmmsg_init(void)
{
	int ret;
	ret = platform_driver_register(&sunxi_vmmsg_driver);
	return ret;
}

static void __exit sunxi_vmmsg_exit(void)
{
	platform_driver_unregister(&sunxi_vmmsg_driver);
}

module_init(sunxi_vmmsg_init);
module_exit(sunxi_vmmsg_exit);

MODULE_AUTHOR("Qin");
MODULE_DESCRIPTION("sunxi vmmsg driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
