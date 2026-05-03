/*
 * v821_wlan/main.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Main code init for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include "xradio_platform.h"
#include "debug.h"

static struct xradio_bus_ops xradio_rpbuf_bus_ops;
static struct xradio_plat *xradio_plat_local;

int xradio_platform_on(struct xradio_plat *xradio_plat)
{
	if (xradio_plat->enabled)
		return 0;

	xradio_plat->enabled = true;
	xradio_bus_start(xradio_plat->bus_if);

	return 0;
}

void xradio_platform_off(struct xradio_plat *xradio_plat)
{
	if (!xradio_plat->enabled) {
		return;
	}
	xradio_bus_stop(xradio_plat->bus_if);
	xradio_plat->enabled = false;
}

int xradio_platform_init(struct xradio_plat **plat)
{
	int ret = -1;
	struct xradio_bus *bus_if;
	struct xradio_plat *xradio_plat;

	plat_printk(XRADIO_DBG_NIY, "xradio_platform_init start\n");

	xradio_plat = kzalloc(sizeof(struct xradio_plat), GFP_KERNEL);
	if (!xradio_plat) {
		plat_printk(XRADIO_DBG_ERROR, "alloc xradio_plat fail\n");
		ret = -ENOMEM;
		return ret;
	}
	xradio_plat->hw_priv.plat = xradio_plat;

	bus_if = kzalloc(sizeof(struct xradio_bus), GFP_KERNEL);
	if (!bus_if) {
		plat_printk(XRADIO_DBG_ERROR, "alloc bus fail\n");
		ret = -ENOMEM;
		goto bus_fail;
	}
	xradio_plat->bus_if = bus_if;
	bus_if->ops = &xradio_rpbuf_bus_ops;

#ifdef XRADIO_RPBUF_SUPPORT
	ret = xradio_rpmsg_init(bus_if, &xradio_plat->hw_priv);
	if (ret) {
		plat_printk(XRADIO_DBG_ERROR, "xradio rpmsg init Failed %d\n", ret);
		goto rpmsg_fail;
	}
	ret = xradio_rpbuf_init(bus_if, &xradio_plat->hw_priv);
	if (ret) {
		plat_printk(XRADIO_DBG_ERROR, "xradio rpbuf init Failed %d\n", ret);
		goto rpbuf_fail;
	}
#endif
	*plat = xradio_plat;
	xradio_plat_local = xradio_plat;
	plat_printk(XRADIO_DBG_ALWY, "xradio_platform_init sucess 0x%x\n", (u32)xradio_plat);
	return 0;

#ifdef XRADIO_RPBUF_SUPPORT
rpbuf_fail:
	xradio_rpmsg_deinit(bus_if);
rpmsg_fail:
	kfree(xradio_plat->bus_if);
	xradio_plat->bus_if = NULL;
bus_fail:
	kfree(xradio_plat);
#endif
	return ret;
}

void xradio_platform_deinit(struct xradio_plat *xradio_plat)
{
#ifdef XRADIO_RPBUF_SUPPORT
	xradio_rpbuf_deinit(xradio_plat->bus_if);
	xradio_rpmsg_deinit(xradio_plat->bus_if);
#endif
	memset(&xradio_rpbuf_bus_ops, 0, sizeof(struct xradio_bus_ops));
	kfree(xradio_plat->bus_if);
	kfree(xradio_plat);
	xradio_plat_local = NULL;
}

struct xradio_plat *xradio_get_platform(void)
{
	return xradio_plat_local;
}
