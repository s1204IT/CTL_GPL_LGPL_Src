// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_CATEGORY UCLASS_BLK

#include <common.h>
#include <types.h>
#include <ufs.h>
#include <./uinclude/scsi.h>
#include <device.h>
#include <./uinclude/blk.h>
#include <log.h>
#include "errno.h"


int blk_create_device(struct udevice *parent, const char *drv_name,
		      const char *name, int if_type, int devnum, int blksz,
		      lbaint_t lba, struct udevice **devp)
{
	struct blk_desc *desc;
	struct udevice *dev;
	//int ret;

	if (devnum == -1) {
		devnum = 0;
	}
	if (devnum < 0)
		return devnum;
	//ret = device_bind_driver(parent, drv_name, name, &dev);
	//if (ret)
	//	return ret;
	dev = parent;
	desc = dev->bd;
	//desc->if_type = if_type;
	desc->blksz = blksz;
	/*desc->log2blksz = LOG2(desc->blksz);*/
	desc->lba = lba;
	//desc->part_type = PART_TYPE_UNKNOWN;
	//desc->bdev = dev;
	desc->devnum = devnum;
	*devp = dev;

	return 0;
}

int blk_create_devicef(struct udevice *parent, const char *drv_name,
		       const char *name, int if_type, int devnum, int blksz,
		       lbaint_t lba, struct udevice **devp)
{
	int ret = 0;
	ret = blk_create_device(parent, drv_name, NULL, if_type, devnum,
				blksz, lba, devp);
	return ret;
}
