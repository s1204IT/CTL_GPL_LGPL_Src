/*
 * v821_wlan/xradio_txrxif.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * tx and rx interface implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "os_dep/os_intf.h"
#include "os_dep/os_net.h"
#include "xradio.h"
#include "xradio_platform.h"
#include "debug.h"
#include "xradio_txrx.h"
#include "xradio_firmware.h"
#include "cmd_proto.h"
#include "low_cmd.h"

int xradio_update_firmware_bin(struct xradio_hw *xradio_hw, struct xradio_hdr *xr_hdr, void *out, u16 out_len)
{
	const struct firmware *fw = NULL;
	struct cmd_payload *payload = NULL;
	struct cmd_para_fw *cmd_fw = NULL;
	enum xr_patch patch;
	int offset, remain, req_len;
	u16 copy_len, all_hdr_len, _out_len;
	struct xradio_hdr *hdr;

	payload = (void *)xr_hdr->payload;
	patch = payload->param[0];
	offset = payload->param[1];
	req_len = payload->param[2];
	all_hdr_len = XR_HDR_SZ + sizeof(struct cmd_payload) + sizeof(struct cmd_para_fw);
	if (all_hdr_len >= out_len) {
		xradio_printk(XRADIO_DBG_ERROR, "out_len %d too small, hdr_len %d", out_len, all_hdr_len);
		return -EINVAL;
	}
	xradio_printk(XRADIO_DBG_MSG, "update fireware:%d", patch);

	switch (patch) {
	case XR_BOOTLOADER:
		fw = xradio_hw->fw_bl;
		break;
	case XR_FIRMWARE:
		fw = xradio_hw->fw_mac;
		break;
	case XR_SDD_BIN:
		fw = xradio_hw->fw_sdd;
		break;
	default:
		break;
	}
	if (!fw) {
		xradio_printk(XRADIO_DBG_ERROR, "request fireware:%d first!", patch);
		return -EACCES;
	}
	xradio_printk(XRADIO_DBG_TRC, "fw->data:%p, offset:%d len:%d,type:%d\n",
				fw->data, offset, req_len, patch);
	if (offset > fw->size) {
		xradio_printk(XRADIO_DBG_ERROR, "offset>fw size, fw->data:%p, fw size:%d offset:%d len:%d,type:%d\n",
				fw->data, fw->size, offset, req_len, patch);
		return -EACCES;
	}

	hdr = out;
	payload = (struct cmd_payload *)hdr->payload;
	cmd_fw = (struct cmd_para_fw *)payload->param;

	remain = fw->size - offset;
	copy_len = min(remain, req_len);
	_out_len = out_len - all_hdr_len;
	copy_len = min(copy_len, _out_len);

	cmd_fw->fw_type = patch;
	cmd_fw->fw_len = fw->size;
	cmd_fw->memsize = copy_len;
	payload->len = sizeof(struct cmd_para_fw) + copy_len;
	hdr->len = sizeof(struct cmd_payload) + payload->len;

	cmd_printk(XRADIO_DBG_MSG, "copy_len=%d payload_len=%d hdr_len=%d memsize=%d\n",
				copy_len, payload->len, hdr->len, cmd_fw->memsize);

	memcpy(cmd_fw->memdata, fw->data + offset, cmd_fw->memsize);
	//data_hex_dump("BIN:", 16, (void *)cmd_fw, payload->len > 64 ? 64 : payload->len);

	return 0;
}

int xradio_request_firmware(struct xradio_hw *xradio_hw, enum xr_patch patch, int len)
{
	int ret = 0;
	const struct firmware *fw = NULL;
	char *filename;
	struct device *dev = xradio_hw->dev;

	switch (patch) {
	case XR_BOOTLOADER:
		filename = V821_WLAN_BOOTLOADER;
		ret = request_firmware(&fw, filename, dev);
		if (ret) {
			xradio_printk(XRADIO_DBG_ERROR, "request bootloader err!");
			return ret;
		}
		if (xradio_hw->fw_bl) {
			xradio_printk(XRADIO_DBG_ERROR, "release bootloader before request!");
			goto err;
		}
		xradio_hw->fw_bl = fw;
		break;
	case XR_FIRMWARE:
		filename = V821_WLAN_FIRMWARE;
		ret = request_firmware(&fw, filename, dev);
		if (ret) {
			xradio_printk(XRADIO_DBG_ERROR, "request firmware err!");
			return ret;
		}
		if (xradio_hw->fw_mac) {
			xradio_printk(XRADIO_DBG_ERROR, "release firmware before request!");
			goto err;
		}
		xradio_hw->fw_mac = fw;
		break;
	case XR_SDD_BIN:
		filename = V821_WLAN_SDD;
		ret = request_firmware(&fw, filename, dev);
		if (ret) {
			xradio_printk(XRADIO_DBG_ERROR, "request sdd err!");
			return ret;
		}
		if (xradio_hw->fw_sdd) {
			xradio_printk(XRADIO_DBG_ERROR, "release sdd before request!");
			goto err;
		}
		xradio_hw->fw_sdd = fw;
		break;
	default:
		break;
	}
	xradio_printk(XRADIO_DBG_MSG, "%s size= %d\n", filename, fw->size);

	return fw->size;
err:
	release_firmware(fw);
	xradio_printk(XRADIO_DBG_ERROR, "request bin file err %d !", ret);
	return -EACCES;
}

int xradio_release_firmware(struct xradio_hw *xradio_hw, enum xr_patch patch)
{
	const struct firmware *fw = NULL;

	switch (patch) {
	case XR_BOOTLOADER:
		if (xradio_hw) {
			fw = xradio_hw->fw_bl;
			xradio_hw->fw_bl = NULL;
		}
		break;
	case XR_FIRMWARE:
		if (xradio_hw) {
			fw = xradio_hw->fw_mac;
			xradio_hw->fw_mac = NULL;
		}
		break;
	case XR_SDD_BIN:
		if (xradio_hw) {
			fw = xradio_hw->fw_sdd;
			xradio_hw->fw_sdd = NULL;
		}
		break;
	default:
		xradio_printk(XRADIO_DBG_ERROR, "patch not found %d\n", patch);
		return -1;
		break;
	}
	xradio_printk(XRADIO_DBG_MSG, "release %d\n", patch);
	release_firmware(fw);

	return 0;
}
