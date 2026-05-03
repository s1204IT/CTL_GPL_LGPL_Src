/*
 * xradio_rpmsg.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface rpbuf implementation for xradio drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _XRADIO_RPMSG_H_
#define _XRADIO_RPMSG_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct xradio_rpmsg_dev {
	struct rpmsg_device *msg_dev;
	struct device *dev;
	struct xradio_hw *hw_priv;
	struct xradio_bus *bus_if;
	int len;
};

int xradio_rpmsg_init(struct xradio_bus *bus_if, struct xradio_hw *hw_priv);
void xradio_rpmsg_deinit(struct xradio_bus *bus_if);

#endif
