/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef SUNXI_RPROC_SUBDEV_H
#define SUNXI_RPROC_SUBDEV_H

#include <linux/platform_device.h>
#include <linux/remoteproc.h>

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
#include "subdev/sunxi_rproc_wdt_dev.h"
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
#include "subdev/sunxi_rproc_rsc_helper.h"
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
#include "subdev/sunxi_rproc_standby_mgr.h"
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_ECC)
#include "subdev/sunxi_rproc_ecc_dev.h"
#endif

struct sunxi_rproc_subdev {
	struct rproc *rproc;
	void __iomem *io_base;
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_WDT)
	struct sunxi_rproc_wdt_dev *wdt_dev;
#endif
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER)
	struct sunxi_rproc_rsc_helper_dev *rsc_helper_dev;
#endif
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
	struct sunxi_rproc_standby_dev *stdby_dev;
#endif

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_ECC_ENABLE)
	struct sunxi_rproc_ecc_dev *ecc_dev;
#endif
};

int sunxi_rproc_add_subdev(struct sunxi_rproc_subdev *subdev, struct rproc *rproc,
			   struct platform_device *pdev);
int sunxi_rproc_subdev_set_parent(struct sunxi_rproc_subdev *subdev);
int sunxi_rproc_remove_subdev(struct sunxi_rproc_subdev *subdev);
int sunxi_rproc_subdev_handle_rsc(struct sunxi_rproc_subdev *subdev, u32 rsc_type, void *rsc,
				  int offset, int avail);

#endif /* SUNXI_RPROC_SUBDEV_H */
