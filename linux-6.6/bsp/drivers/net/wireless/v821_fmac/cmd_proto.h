/*
 * v821_wlan/cmd_proto.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * cmd protocol implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XR_CMD_PROTO_H__
#define __XR_CMD_PROTO_H__

/*XR_WIFI_HOST_HAND_WAY */
struct cmd_para_hand_way {
	u8 id;
};

struct cmd_para_keep_alive {
	u8 data;
};

/*XR_WIFI_HOST_DATA_TEST */
struct cmd_para_data_test {
	u16 len;
	u8 data[0];
};

/*XR_WIFI_HOST_HAND_WAY_RES */
struct cmd_para_hand_way_res {
	u8 id;
	u8 mac[6];
	u8 ip_addr[4];
	u8 netmask[4];
	u8 gw[4];
};

struct cmd_para_block_rw {
	u32 memaddr;
	u32 memsize;
	u32 memdata[];
};

struct param_wlan_bin {
	u32 type;
	u32 offset;
	u32 len;
};

struct cmd_para_fw {
	u32 fw_type;
	u32 fw_len;
	u32 memsize;
	u32 memdata[];
};

struct cmd_para_mac_info {
	u8 ifname[5];
	u8 mac[6];
};

struct cmd_para_efuse {
	u32 addr;
};

/* opcode */
enum cmd_host_opcode {
	XR_WIFI_HOST_HAND_WAY = 0,
	XR_WIFI_HOST_WLAN_POWER,
	XR_WIFI_HOST_KEEP_AVLIE,
	XR_WIFI_HOST_BLOCK_RW_REQ,
	XR_WIFI_HOST_SET_DEVICE_MAC_REQ,
	XR_WIFI_HOST_GET_DEVICE_MAC_DRV_REQ,
	XR_WIFI_HOST_SET_EFUSE,
	XR_WIFI_HOST_KERNEL_MAX,
};

enum cmd_dev_opcode {
	XR_WIFI_DEV_HAND_WAY_RES = 0,
	XR_WIFI_DEV_WLAN_POWER_STATE,
	XR_WIFI_DEV_RX_PAUSE,
	XR_WIFI_DEV_RX_RESUME,
	XR_WIFI_DEV_KEEP_AVLIE,
	XR_WIFI_DEV_BLOCK_RW_REQ,
	XR_WIFI_DEV_REQUEST_FW_REQ,
	XR_WIFI_DEV_UPDATE_FW_REQ,
	XR_WIFI_DEV_RELEASE_FW_REQ,
	XR_WIFI_DEV_SET_DEVICE_MAC_RES,
	XR_WIFI_DEV_ETH_STATE,
	XR_WIFI_DEV_KERNEL_MAX,
};

/* config payload */
struct cmd_payload { //protocol_command
	u16 type;
	u16 len; /* param len */
	u32 param[0];
};

#define CMD_HEAD_SIZE (sizeof(struct cmd_payload))
#endif
