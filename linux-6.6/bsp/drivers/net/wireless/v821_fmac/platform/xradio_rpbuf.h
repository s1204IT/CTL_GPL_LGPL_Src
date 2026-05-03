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

#ifndef _XRADIO_RPBUF_H_
#define _XRADIO_RPBUF_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/rpbuf.h>

#include "xradio.h"

#define RPBUF_BUFFER_NAME_XRADIO_RX         "xradio_mrx" // Master RX, Linux <- RTOS
#define RPBUF_BUFFER_NAME_XRADIO_TX         "xradio_mtx" // Master TX, Linux -> RTOS
#define RPBUF_BUFFER_XRADIO_TX_AGG_MAX      12//12
#define XRADIO_MAX_TX_RPBUF_SIZE            (12 * 1024)//(12 * 1024)
#define XRADIO_MAX_RX_RPBUF_SIZE            (20 * 1024)//(12 * 1024)

//#define RPBUF_NO_CHECK_RSP

struct xradio_rpbuf_dev {
	struct platform_device *pdev;
	struct device *dev;
	struct xradio_hw *hw_priv;
	struct xradio_bus *bus_if;
	struct rpbuf_controller *controller;
	char name_mrx[RPBUF_NAME_SIZE]; /* Master RX */
	int len_mrx;
	struct rpbuf_buffer *buffer_mrx;
	char name_mtx[RPBUF_NAME_SIZE]; /* Master TX */
	int len_mtx;
	struct rpbuf_buffer *buffer_mtx;
	u8 *buffer_mtx_tail;
	int buffer_mtx_agg_count;
	int buffer_mtx_used_len;
	int buffer_mtx_free_len;
	int buffer_mtx_agg_max;
};

int xradio_rpbuf_init(struct xradio_bus *bus_if, struct xradio_hw *hw_priv);
void xradio_rpbuf_deinit(struct xradio_bus *bus_if);

#endif
