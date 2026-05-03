// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi's rproc subdev driver
 * add or remove subdev for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/remoteproc.h>
#include <linux/slab.h>
#include "remoteproc_internal.h"
#include "sunxi_rproc_subdev.h"

extern int sunxi_sid_get_ecc_status(void);

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
static inline
int sunxi_rproc_wdt_get_resource(struct platform_device *pdev, struct sunxi_rproc_wdt_dev_res *res)
{
	int ret;
	struct resource *r;
	struct device_node *np;
	struct device *dev = &pdev->dev;
	const char *str;

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rproc_wdt_reg");
	if (!r) {
		dev_err(dev, "can not find rproc_wdt_reg\n");
		return -ENODEV;
	}
	memcpy(&res->reg_res, r, sizeof(res->reg_res));

	res->irq_num = platform_get_irq_byname(pdev, "rproc_wdt_irq");
	if (!res->irq_num) {
		dev_err(dev, "can not find rproc_wdt_irq\n");
		return -ENODEV;
	}

	np = of_find_node_by_name(pdev->dev.of_node, "rproc_wdt");
	if (!np) {
		dev_err(dev, "can not find node: rproc_wdt\n");
		return -ENODEV;
	}

	ret = of_property_read_string(np, "status", &str);
	if (ret || !str) {
		dev_err(dev, "can not read_string: status, ret: %d\n", ret);
		return ret;
	}

	if (strcmp(str, "okay") && strcmp(str, "ok")) {
		dev_info(dev, "node not okay\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "timeout_ms", &res->timeout_ms);
	if (ret) {
		dev_err(dev, "can not get timeout_ms, ret: %d\n", ret);
		return ret;
	}

	ret = of_property_read_u32(np, "try_times", &res->try_times);
	if (ret) {
		dev_err(dev, "can not get try_times, ret: %d\n", ret);
		return ret;
	}

	ret = of_property_read_u32(np, "reset_type", &res->reset_type);
	if (ret) {
		dev_err(dev, "can not get reset_type, ret: %d\n", ret);
		return ret;
	}

	dev_info(dev, "timeout_ms: %u\n", res->timeout_ms);
	dev_info(dev, "reset_type: %u\n", res->reset_type);
	dev_info(dev, "reg:        %lx\n", (unsigned long)res->reg_res.start);
	dev_info(dev, "irq_num:    %d\n", res->irq_num);

	return 0;
}

static inline struct sunxi_rproc_wdt_dev *sunxi_rproc_wdt_probe(struct rproc *rproc,
								struct platform_device *pdev)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc_wdt_dev *wdt_dev = kzalloc(sizeof(*wdt_dev), GFP_KERNEL);
	struct sunxi_rproc_wdt_dev_res wdt_dev_res;
	int ret = -ENOMEM;

	if (!wdt_dev)
		goto err_out;

	ret = sunxi_rproc_wdt_get_resource(pdev, &wdt_dev_res);
	if (ret) {
		dev_err(dev, "sunxi_rproc_wdt_get_resource failed, ret: %d\n", ret);
		goto err_out;
	}

	ret = sunxi_rproc_wdt_dev_register(wdt_dev, rproc, &wdt_dev_res);
	if (ret) {
		dev_err(dev, "sunxi_rproc_wdt_dev_register failed, ret: %d\n", ret);
		goto err_out;
	}

	return wdt_dev;
err_out:
	kfree(wdt_dev);
	return NULL;
}

static void wdt_dev_release(void *priv)
{
	struct sunxi_rproc_wdt_dev *wdt_dev = priv;

	kfree(wdt_dev);
}

static inline void sunxi_rproc_wdt_remove(struct sunxi_rproc_wdt_dev *wdt_dev)
{
	if (wdt_dev) {
		sunxi_rproc_wdt_dev_set_release(wdt_dev, wdt_dev_release, wdt_dev);
		sunxi_rproc_wdt_dev_unregister(wdt_dev);
	}
}
#endif /* CONFIG_AW_RPROC_SUBDEV_WDT */

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
static inline struct sunxi_rproc_rsc_helper_dev *sunxi_rproc_rsc_helper_probe(struct rproc *rproc,
								struct platform_device *pdev)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev;
	int ret = -ENOMEM;

	rsc_helper_dev = kzalloc(sizeof(*rsc_helper_dev), GFP_KERNEL);
	if (!rsc_helper_dev)
		goto err_out;

	ret = sunxi_rproc_rsc_helper_dev_register(rsc_helper_dev, rproc);
	if (ret) {
		dev_err(dev, "sunxi_rproc_rsc_helper_dev_register failed, ret: %d\n", ret);
		goto err_out;
	}

	return rsc_helper_dev;
err_out:
	kfree(rsc_helper_dev);
	return NULL;
}

static void rsc_helper_dev_release(void *priv)
{
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = priv;

	kfree(rsc_helper_dev);
}

static inline void sunxi_rproc_rsc_helper_remove(struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev)
{
	if (rsc_helper_dev) {
		sunxi_rproc_rsc_helper_dev_set_release(rsc_helper_dev, rsc_helper_dev_release,
						       rsc_helper_dev);
		sunxi_rproc_rsc_helper_dev_unregister(rsc_helper_dev);
	}
}
#endif /* IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER) */

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
static inline struct sunxi_rproc_standby_dev *sunxi_rproc_standby_probe(struct rproc *rproc,
								struct platform_device *pdev)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc_standby_dev *stdby_dev;
	int ret = -ENOMEM;

	stdby_dev = kzalloc(sizeof(struct sunxi_rproc_standby_dev), GFP_KERNEL);
	if (!stdby_dev)
		goto err_out;

	ret = sunxi_rproc_standby_dev_register(stdby_dev, rproc);
	if (ret) {
		dev_err(dev, "sunxi_rproc_standby_dev_register failed, ret: %d\n", ret);
		goto err_out;
	}

	return stdby_dev;
err_out:
	kfree(stdby_dev);
	return NULL;
}

static void stdby_dev_release(void *priv)
{
	struct sunxi_rproc_standby_dev *stdby_dev = priv;

	kfree(stdby_dev);
}

static inline void sunxi_rproc_stdby_remove(struct sunxi_rproc_standby_dev *stdby_dev)
{
	if (stdby_dev) {
		sunxi_rproc_standby_dev_set_release(stdby_dev, stdby_dev_release,
						       stdby_dev);
		sunxi_rproc_standby_dev_unregister(stdby_dev);
	}
}
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_ECC)
static inline struct sunxi_rproc_ecc_dev *sunxi_rproc_ecc_probe(struct rproc *rproc,
								struct platform_device *pdev)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc_ecc_dev *ecc_dev;
	int ret = -ENOMEM;

	int ecc_status;

	ecc_status = sunxi_sid_get_ecc_status();
	if (!ecc_status) {
		dev_warn(dev, "sys ecc status is close, do not need open ecc fun in e907 module\n");
		return NULL;
	}

	ecc_dev = kzalloc(sizeof(struct sunxi_rproc_ecc_dev), GFP_KERNEL);
	if (!ecc_dev)
		goto err_out;

	ecc_dev->irq_num = platform_get_irq_byname(pdev, "rproc_ecc_irq");
	if (!ecc_dev->irq_num) {
		dev_err(dev, "can not find rproc_ecc_irq\n");
		return NULL;
	}

	ret = sunxi_rproc_ecc_dev_register(ecc_dev, rproc);
	if (ret) {
		dev_err(dev, "sunxi_rproc_ecc_dev_register failed, ret: %d\n", ret);
		goto err_out;
	}

	return ecc_dev;
err_out:
	kfree(ecc_dev);
	return NULL;
}

static void ecc_dev_release(void *priv)
{
	struct sunxi_rproc_ecc_dev *ecc_dev = priv;

	kfree(ecc_dev);
}

static inline void sunxi_rproc_ecc_remove(struct sunxi_rproc_ecc_dev *ecc_dev)
{
	if (ecc_dev) {
		sunxi_rproc_ecc_dev_set_release(ecc_dev, ecc_dev_release,
						       ecc_dev);
		sunxi_rproc_ecc_dev_unregister(ecc_dev);

		kfree(ecc_dev);
	}
}
#endif

int sunxi_rproc_add_subdev(struct sunxi_rproc_subdev *subdev,
			   struct rproc *rproc, struct platform_device *pdev)
{
	subdev->rproc = rproc;

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
	subdev->wdt_dev = sunxi_rproc_wdt_probe(rproc, pdev);
	if (subdev->wdt_dev)
		rproc_add_subdev(subdev->rproc, &subdev->wdt_dev->subdev);
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	subdev->rsc_helper_dev = sunxi_rproc_rsc_helper_probe(rproc, pdev);
	if (subdev->rsc_helper_dev)
		rproc_add_subdev(subdev->rproc, &subdev->rsc_helper_dev->subdev);
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
	subdev->stdby_dev = sunxi_rproc_standby_probe(rproc, pdev);
	if (subdev->stdby_dev)
		rproc_add_subdev(subdev->rproc, &subdev->stdby_dev->subdev);
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_ECC)
	subdev->ecc_dev = sunxi_rproc_ecc_probe(rproc, pdev);
	if (subdev->ecc_dev) {
		rproc_add_subdev(subdev->rproc, &subdev->ecc_dev->subdev);
		subdev->ecc_dev->base = subdev->io_base;
	}
#endif

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_add_subdev);

int sunxi_rproc_subdev_set_parent(struct sunxi_rproc_subdev *subdev)
{
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
	if (subdev->wdt_dev)
		sunxi_rproc_wdt_dev_set_parent(subdev->wdt_dev, &subdev->rproc->dev);
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	if (subdev->rsc_helper_dev)
		sunxi_rproc_rsc_helper_dev_set_parent(subdev->rsc_helper_dev, &subdev->rproc->dev);
#endif


#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
	if (subdev->stdby_dev)
		sunxi_rproc_standby_dev_set_parent(subdev->stdby_dev, &subdev->rproc->dev);
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_ECC)
	if (subdev->ecc_dev)
		sunxi_rproc_ecc_dev_set_parent(subdev->ecc_dev, &subdev->rproc->dev);
#endif

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_subdev_set_parent);

int sunxi_rproc_remove_subdev(struct sunxi_rproc_subdev *subdev)
{
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev = subdev->rsc_helper_dev;
#endif
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
	struct sunxi_rproc_wdt_dev *wdt_dev = subdev->wdt_dev;
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	if (rsc_helper_dev) {
		subdev->rsc_helper_dev = NULL;
		rproc_remove_subdev(subdev->rproc, &rsc_helper_dev->subdev);
		sunxi_rproc_rsc_helper_remove(rsc_helper_dev);
	}
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
	if (wdt_dev) {
		subdev->wdt_dev = NULL;
		rproc_remove_subdev(subdev->rproc, &wdt_dev->subdev);
		sunxi_rproc_wdt_remove(wdt_dev);
	}
#endif

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_remove_subdev);

int sunxi_rproc_subdev_handle_rsc(struct sunxi_rproc_subdev *subdev, u32 rsc_type, void *rsc,
				  int offset, int avail)
{
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	int ret;
	struct rproc *rproc = subdev->rproc;
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	ret = sunxi_rproc_rsc_helper_handle_rsc(subdev->rsc_helper_dev, rproc, rsc_type, rsc,
						offset, avail);
	if (ret == RSC_HANDLED)
		return ret;
#endif

	return RSC_IGNORED;
}
EXPORT_SYMBOL(sunxi_rproc_subdev_handle_rsc);
