// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *  An sram ecc  driver for Sunxi Platform of Allwinner SoC
 *
 * Copyright (C) 2024 panzhijian@allwinnertech.com
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <sunxi-sid.h>
#include <asm/io.h>

#define MSRAMOC_ECC_REG_BASE                        (sunxi_sram_ecc_dev->base)

#define MSRAMOC_ECC_CTRL_REG                        (MSRAMOC_ECC_REG_BASE)
#define MSRAMOC_ECC_CTRL_INJECT_EN_SHIFT            (0)
#define MSRAMOC_ECC_INJECT_MODE_SHIFT               (4)
#define MSRAMOC_ECC_INJECT_MODE_MASK                (0x3 << MSRAMOC_ECC_INJECT_MODE_SHIFT)
#define MSRAMOC_ECC_INJECT_TYPE_SHIFT               (6)
#define MSRAMOC_ECC_INJECT_TYPE_MASK                (0x3 << MSRAMOC_ECC_INJECT_TYPE_SHIFT)
#define MSRAMOC_ECC_CTRL_ECC_EN_SHIFT               (8)
#define MSRAMOC_ECC_CTRL_INIT_EN_SHIFT              (12)
#define MSRAMOC_ECC_CTRL_IRQ_EN_SHIFT               (16)
#define MSRAMOC_ECC_CTRL_INIT_DONE_SHIFT            (31)

#define MSRAMOC_ECC_IRQ_CLR_REG                     (MSRAMOC_ECC_REG_BASE + 0x4)
#define MSRAMOC_ECC_CTRL_IRQ_CLR_SHIFT              (0)

#define MSRAMOC_ECC_INT_STA0_REG                    (MSRAMOC_ECC_REG_BASE + 0x8)
#define MSRAMOC_ECC_INT_STA0_SIG_BIT_SHIFT          (24)
#define MSRAMOC_ECC_INT_STA0_DOU_BIT_SHIFT          (25)
#define MSRAMOC_ECC_INT_STA0_CHECK_CODE_BIT_SHIFT   (26)

#define MSRAMOC_ECC_INT_STA1_REG                    (MSRAMOC_ECC_REG_BASE + 0xC)
#define MSRAMOC_ECC_INT_STA1_SIG_BIT_SHIFT          (24)
#define MSRAMOC_ECC_INT_STA1_DOU_BIT_SHIFT          (25)
#define MSRAMOC_ECC_INT_STA1_CHECK_CODE_BIT_SHIFT   (26)

#define MSRAMOC_ECC_INT_STA2_REG                    (MSRAMOC_ECC_REG_BASE + 0x10)
#define MSRAMOC_ECC_INT_STA2_SIG_BIT_SHIFT          (24)
#define MSRAMOC_ECC_INT_STA2_DOU_BIT_SHIFT          (25)
#define MSRAMOC_ECC_INT_STA2_CHECK_CODE_BIT_SHIFT   (26)

#define MSRAMOC_ECC_INT_STA3_REG                    (MSRAMOC_ECC_REG_BASE + 0x14)
#define MSRAMOC_ECC_INT_STA3_SIG_BIT_SHIFT          (24)
#define MSRAMOC_ECC_INT_STA3_DOU_BIT_SHIFT          (25)
#define MSRAMOC_ECC_INT_STA3_CHECK_CODE_BIT_SHIFT   (26)

#define MSRAMOC_ECC_INJECT_DATA_CMP_REG             (MSRAMOC_ECC_REG_BASE + 0x18)

#define MSRAMOC_ECC_ORI_ERR_DATA_REG                (MSRAMOC_ECC_REG_BASE + 0x1C)

#define PRCM_DUM_REG                                (MSRAMOC_ECC_REG_BASE + 0x9C)
#define PRCM_DUM_INJECT_EN_ECO_SHIFT                (0)
#define PRCM_DUM_INJECT_EN_ECO_MASK                 (0x7 < PRCM_DUM_INJECT_EN_ECO_SHIFT)

struct sunxi_sram_ecc_dev_t {
	struct device          *dev;
	void __iomem           *base;
	int                    irq;
};

struct sunxi_sram_ecc_dev_t *sunxi_sram_ecc_dev;

#ifdef CONFIG_AW_SRAM_ECC_TEST

static ssize_t sram_ecc_inject_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	u32 inject_bit = 0;
	volatile u32 reg_val = 0x0;

	ret = sscanf(buf, "%u", &inject_bit);

	reg_val = readl(MSRAMOC_ECC_CTRL_REG);

	if (inject_bit == 1) {
		reg_val &= (~MSRAMOC_ECC_INJECT_TYPE_MASK);
		reg_val |= (0x1 << MSRAMOC_ECC_INJECT_TYPE_SHIFT);
	} else if (inject_bit == 2) {
		reg_val &= (~MSRAMOC_ECC_INJECT_TYPE_MASK);
		reg_val |= (0x2 << MSRAMOC_ECC_INJECT_TYPE_SHIFT);
	} else if (inject_bit == 3) {
		reg_val &= (~MSRAMOC_ECC_INJECT_TYPE_MASK);
		reg_val |= (0x3 << MSRAMOC_ECC_INJECT_TYPE_SHIFT);
	} else {
		pr_warn("sram ecc inject bit err, %d!\n", inject_bit);
		return -1;
	}

	writel(reg_val, MSRAMOC_ECC_CTRL_REG);

	reg_val = readl(MSRAMOC_ECC_CTRL_REG);
	reg_val &= (~MSRAMOC_ECC_INJECT_MODE_MASK);
	reg_val |= (0x1 << MSRAMOC_ECC_INJECT_MODE_SHIFT);
	writel(reg_val, MSRAMOC_ECC_CTRL_REG);

	reg_val = readl(PRCM_DUM_REG);
	reg_val &= (~PRCM_DUM_INJECT_EN_ECO_MASK);
	reg_val |= (0x6 << PRCM_DUM_INJECT_EN_ECO_SHIFT);
	writel(reg_val, PRCM_DUM_REG);

	reg_val = readl(MSRAMOC_ECC_CTRL_REG);
	reg_val |= (0x1 << MSRAMOC_ECC_CTRL_INJECT_EN_SHIFT);
	writel(reg_val, MSRAMOC_ECC_CTRL_REG);

	return count;
}
static CLASS_ATTR_WO(sram_ecc_inject);

static struct attribute *sram_ecc_class_attrs[] = {
	&class_attr_sram_ecc_inject.attr,
	NULL,
};
ATTRIBUTE_GROUPS(sram_ecc_class);

static struct class sram_ecc_class = {
	.name		= "sram_ecc",
	.class_groups	= sram_ecc_class_groups,
};
#endif /* #ifdef CONFIG_AW_SRAM_ECC_TEST */

static irqreturn_t sunxi_sram_ecc_handler(int irq, void *dev_id)
{
	struct sunxi_sram_ecc_dev_t *sram_ecc_dev = (struct sunxi_sram_ecc_dev_t *)dev_id;
	volatile u32 reg_val = 0x0;

#ifdef CONFIG_AW_SRAM_ECC_TEST
	reg_val = readl(MSRAMOC_ECC_CTRL_REG);
	reg_val &= (~(0x1 << MSRAMOC_ECC_CTRL_INJECT_EN_SHIFT));
	writel(reg_val, MSRAMOC_ECC_CTRL_REG);
#endif
	reg_val = readl(MSRAMOC_ECC_INT_STA0_REG);

	if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA0_SIG_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta0 singel bit err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA0_DOU_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta0 doub bits err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA0_CHECK_CODE_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta0 check code err!\n");

	reg_val = readl(MSRAMOC_ECC_INT_STA1_REG);

	if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA1_SIG_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta1 singel bit err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA1_DOU_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta1 doub bits err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA1_CHECK_CODE_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta1 check code err!\n");

	reg_val = readl(MSRAMOC_ECC_INT_STA2_REG);

	if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA2_SIG_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta2 singel bit err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA2_DOU_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta2 doub bits err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA2_CHECK_CODE_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta2 check code err!\n");

	reg_val = readl(MSRAMOC_ECC_INT_STA3_REG);

	if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA3_SIG_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta3 singel bit err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA3_DOU_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta3 doub bits err!\n");
	else if (reg_val & (0x1 << MSRAMOC_ECC_INT_STA3_CHECK_CODE_BIT_SHIFT))
		dev_warn(sram_ecc_dev->dev, "sta3 check code err!\n");;

	reg_val = readl(MSRAMOC_ECC_IRQ_CLR_REG);
	reg_val |= (0x1 << MSRAMOC_ECC_CTRL_IRQ_CLR_SHIFT);
	writel(reg_val, MSRAMOC_ECC_IRQ_CLR_REG);
	reg_val &= (~(0x1 << MSRAMOC_ECC_CTRL_IRQ_CLR_SHIFT));
	writel(reg_val, MSRAMOC_ECC_IRQ_CLR_REG);

	return IRQ_HANDLED;
}

static bool sunxi_is_ecc_enable(void)
{
	return sunxi_sid_get_ecc_status() ? true : false;
}

static int sunxi_sram_ecc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct sunxi_sram_ecc_dev_t *sram_ecc_dev = NULL;
	struct device *dev = &pdev->dev;
	volatile u32 reg_val = 0x0;
	int ret = 0;

	if (!sunxi_is_ecc_enable())
		return -ENODEV;

	if (!np)
		return -ENODEV;

	sram_ecc_dev = devm_kzalloc(dev, sizeof(*sram_ecc_dev), GFP_KERNEL);
	if (!sram_ecc_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_warn(dev, "Unable to find resource region\n");
		ret = -ENOENT;
		goto err_res;
	}

	sram_ecc_dev->base = devm_ioremap_resource(dev, res);
	if (!sram_ecc_dev->base) {
		dev_warn(dev, "Unable to map sram ecc @ PA:%#x\n",
				(unsigned int)res->start);
		ret = -ENOENT;
		goto err_res;
	}

	sram_ecc_dev->irq = platform_get_irq(pdev, 0);
	if (sram_ecc_dev->irq < 0) {
		dev_err(dev, "sram ecc fail to get irq!\n");
		ret = -ENOENT;
		goto err_irq;
	}

	ret = devm_request_irq(dev, sram_ecc_dev->irq, sunxi_sram_ecc_handler, 0,
		dev_name(dev), sram_ecc_dev);
	if (ret) {
		dev_err(dev, "sram ecc could not request irq!\n");
		goto err_irq;
	}

	sram_ecc_dev->dev = dev;
	sunxi_sram_ecc_dev = sram_ecc_dev;
	platform_set_drvdata(pdev, sram_ecc_dev);

	reg_val = readl(MSRAMOC_ECC_CTRL_REG);
	reg_val |= (0x1 << MSRAMOC_ECC_CTRL_IRQ_EN_SHIFT);
	writel(reg_val, MSRAMOC_ECC_CTRL_REG);

	sram_ecc_dev->dev = dev;
	sunxi_sram_ecc_dev = sram_ecc_dev;
	platform_set_drvdata(pdev, sram_ecc_dev);

#ifdef CONFIG_AW_SRAM_ECC_TEST
	ret = class_register(&sram_ecc_class);
	if (ret) {
		dev_err(dev, "failed to sram_ecc class register, ret = %d\n", ret);
		goto err_class;
	}
#endif

	dev_dbg(&pdev->dev, "sram ecc probe succeed\n");

	return 0;

#ifdef CONFIG_AW_SRAM_ECC_TEST
err_class:
	devm_free_irq(dev, sram_ecc_dev->irq, sram_ecc_dev);
#endif

err_irq:
	devm_iounmap(dev, sram_ecc_dev->base);
err_res:
	dev_err(dev, "failed to initialize\n");

	return ret;
}

static int sunxi_sram_ecc_remove(struct platform_device *pdev)
{
	struct sunxi_sram_ecc_dev_t *sram_ecc_dev = platform_get_drvdata(pdev);

#ifdef CONFIG_AW_SRAM_ECC_TEST
	class_unregister(&sram_ecc_class);
#endif
	devm_free_irq(sram_ecc_dev->dev, sram_ecc_dev->irq, sram_ecc_dev);
	devm_iounmap(sram_ecc_dev->dev, sram_ecc_dev->base);

	return 0;
}

static const struct of_device_id sunxi_sram_ecc_of_match[] = {
	{ .compatible = "allwinner,sun55iw6-sram-ecc", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_sram_ecc_of_match);

static struct platform_driver sunxi_sram_ecc_driver = {
	.probe = sunxi_sram_ecc_probe,
	.remove	= sunxi_sram_ecc_remove,
	.driver = {
		.name = "sunxi-sram-ecc",
		.of_match_table = sunxi_sram_ecc_of_match,
	},
};
module_platform_driver(sunxi_sram_ecc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner sram ecc driver");
MODULE_ALIAS("platform: sunxi_sram_ecc");
MODULE_AUTHOR("panzhijian <panzhijian@allwinnertech.com>");
MODULE_VERSION("1.0.0");
