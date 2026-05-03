/*
 * v821_wlan/xr_hdr.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * xradio transmit protocol define for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XR_HDR_H__
#define __XR_HDR_H__

#ifdef XRADIO_CHECKSUM
#define XRADIO_HDR_CKSUM
#endif
#ifdef XRADIO_HDR_CKSUM
#include "checksum.h"
#endif

struct xradio_hdr {
	u8  type;
	u8  rsv;
	u16 seq;
	u16 len; /* payload len */
	u16 rsv1;
#ifdef XRADIO_HDR_CKSUM
	u16 checksum;
	u16 checksum_pad;
#endif
	u8  payload[];
} __packed;
#define XR_HDR_SZ sizeof(struct xradio_hdr)

struct xradio_agg_hdr {
	u8 agg_cnt; /* Input */
	u8 start_idx; /* Input */
	u8 ack_cnt;  /* Output, rx port rxed cnt of this agg data */
	u8 rsp_code; /* Output */
} __packed;
#define XR_AGG_HDR_SZ sizeof(struct xradio_agg_hdr)


/* Xradio xrlink xradio_hdr's type */
#define XR_TYPE_DATA           0x00
#define XR_TYPE_DATA_AP        0x01
#define XR_TYPE_CMD            0x02 // need respond !
#define XR_TYPE_EVENT          0x03
#define XR_TYPE_CMD_RSP        0x04
#define XR_TYPE_FW_BIN         0x05 // need respond !
#define XR_TYPE_RAW_CMD_DATA   0x06 // only use for rtos send raw data

#endif
