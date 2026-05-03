/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc ecc dev driver
 * register or unregister rsc helper device for rproc.
 *
 * Copyright (C) 2025 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SUNXI_RPROC_ECC_DEV_H
#define SUNXI_RPROC_ECC_DEV_H

#include <linux/device.h>
#include <linux/remoteproc.h>

#define RV_CFG_ICACHE_ECC_IRQ_CLEAR_REG		(0x0404) 	/* RV_CFG ICACHE ECC IRQ CLEAR Register */
#define RV_CFG_DCACHE_ECC_IRQ_CLEAR_REG		(0x040c) 	/* RV_CFG DCACHE ECC IRQ CLEAR Register */

struct sunxi_rproc_ecc_dev {
	struct device dev;
	struct rproc_subdev subdev;
	struct rproc *rproc;

	struct work_struct work;

	bool error_flag;

	void (*on_release)(void *priv);
	void *on_release_priv;

	void __iomem *base;
	int irq_num;
};

int sunxi_rproc_ecc_dev_register(struct sunxi_rproc_ecc_dev *ecc_dev, struct rproc *rproc);
int sunxi_rproc_ecc_dev_unregister(struct sunxi_rproc_ecc_dev *ecc_dev);
int sunxi_rproc_ecc_dev_set_release(struct sunxi_rproc_ecc_dev *ecc_dev,
				    void (*release)(void *), void *priv);
int sunxi_rproc_ecc_dev_set_parent(struct sunxi_rproc_ecc_dev *ecc_dev,
					  struct device *parent);

#endif