/*
 * v821_wlan/xradio.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * xradio common private APIs for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XRADIO_H__
#define __XRADIO_H__

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/slab.h>

#include "xr_hdr.h"
#include "queue.h"

/* Xradio xrlink Queue */
#define XR_RX_COM_QUEUE_SZ   (24)
#define XR_CMD_RSP_QUEUE_SZ  (2)
#define XR_DATA_TX_QUEUE_SZ  (160)
#define XR_DATA_RX_QUEUE_SZ  (256)

struct xradio_plat;
struct xradio_debug_common;
struct xradio_hw {
	struct device *dev;
	struct xradio_plat *plat;

	u8 txrx_enable;
	struct xradio_queue_t queue_rx_com;
	struct xradio_queue_t queue_cmd_rsp;
	struct xradio_queue_t queue_rx_data;
	struct xradio_queue_t queue_tx_data;

	wait_queue_head_t rx_com_wq;
	xr_thread_handle_t rx_com_thread;

	xr_mutex_t cmd_tx_mutex;
	wait_queue_head_t tx_cmd_rsp_wq;

	wait_queue_head_t data_tx_wq;
	xr_thread_handle_t data_tx_thread;
	xr_mutex_t data_tx_mutex;
	xr_atomic_t tx_data_pause_master; // linux no buffer
	xr_atomic_t tx_data_pause_slave; // rtos no buffer

	wait_queue_head_t data_rx_wq;
	xr_thread_handle_t data_rx_thread;

	u16 data_tx_seq;
	u16 com_tx_seq;
	const struct firmware *fw_bl;
	const struct firmware *fw_sdd;
	const struct firmware *fw_mac;

	struct xradio_debug_common *debug;
};

#endif
