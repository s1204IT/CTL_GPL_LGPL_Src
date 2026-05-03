/*
 * v821_wlan/xradio_txrxif.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface APIs for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _XRADIO_FIRMWARE_H_
#define _XRADIO_FIRMWARE_H_

#include <linux/firmware.h>
#include "xradio.h"

#define FIRMWARE_BASE_PATH  ""
#define V821_WLAN_BOOTLOADER    ("boot_v821.bin")
#define V821_WLAN_FIRMWARE      ("fw_mac_v821.bin")
#define V821_WLAN_SDD           ("sdd_v821.bin")

#define FIELD_OFFSET(type, field) ((u8 *)&((type *)0)->field - (u8 *)0)
#define FIND_NEXT_ELT(e) (struct xradio_sdd *)((u8 *)&e->data + e->length)
struct xradio_sdd {
	u8 id;
	u8 length;
	u8 data[];
};

enum xr_patch {
	XR_BOOTLOADER,
	XR_FIRMWARE,
	XR_SDD_BIN,
	XR_ETF_FW,
};

#define XR_FW_SET_TYPE_OFFSET(op, type, offset) ((op) << 24 | (type) << 20 | (offset))
#define XR_FW_GET_OPERATE(v) ((v) >> 24)
#define XR_FW_GET_TYPE(v) (((v) >> 20) & 0xF)
#define XR_FW_GET_OFFSET(v) ((v) & 0xFFFFF)

int xradio_request_firmware(struct xradio_hw *xradio_hw, enum xr_patch patch, int len);
int xradio_release_firmware(struct xradio_hw *xradio_hw, enum xr_patch patch);
int xradio_update_firmware_bin(struct xradio_hw *xradio_hw, struct xradio_hdr *xr_hdr, void *out, u16 out_len);

#endif
