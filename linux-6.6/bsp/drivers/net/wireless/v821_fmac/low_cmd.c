/*
 * v821_wlan/iface.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * low protocol command implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include "os_dep/os_intf.h"
#include "os_dep/os_net.h"
#include "queue.h"
#include "xradio.h"
#include "xr_version.h"
#include "low_cmd.h"
#include "debug.h"
#include "xradio_txrx.h"
#include "cmd_proto.h"
#include "xradio_firmware.h"

static struct cmd_payload *xradio_low_cmd_payload_zalloc(u16 type, u16 len, void *param, u32 param_len)
{
	struct cmd_payload *payload = NULL;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = (struct xradio_hdr *)xradio_k_zmalloc(XR_HDR_SZ + sizeof(struct cmd_payload) + len);
	if (!xr_hdr)
		return NULL;

	payload = (void *)xr_hdr->payload;
	payload->type = type;
	payload->len = len;
	if (param)
		memcpy(payload->param, param, len);

	return payload;
}

static void xradio_low_cmd_payload_free(struct cmd_payload *cmd_payload)
{
	if (cmd_payload) {
		struct xradio_hdr *xr_hdr = NULL;

		xr_hdr = container_of((void *)cmd_payload, struct xradio_hdr, payload);
		xradio_k_free(xr_hdr);
	}
}

static int xradio_low_cmd_send_cmd(u8 *cmd_payload, u16 len, void *cfm, u16 cfm_len)
{
	int ret = -1;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = container_of((void *)cmd_payload, struct xradio_hdr, payload);
	xr_hdr->type = XR_TYPE_CMD;
	xr_hdr->len = len;
	ret = xradio_tx_com_process(xr_hdr, cfm, cfm_len);

	return ret;
}

int xradio_low_cmd_send_event(u8 *cmd_payload, u16 len)
{
	int ret = -1;
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = container_of((void *)cmd_payload, struct xradio_hdr, payload);
	xr_hdr->type = XR_TYPE_EVENT;
	xr_hdr->len = len;
	ret = xradio_tx_com_process(xr_hdr, NULL, 0);

	return ret;
}

static int xradio_low_cmd_res_handle(struct cmd_payload *payload)
{
	if (!payload)
		return -1;

	cmd_printk(XRADIO_DBG_MSG, "rxed type:%d len:%d\n", payload->type, payload->len);

	switch (payload->type) {

	}

	return 0;
}

int xradio_rx_low_cmd_push(u8 *buff, u32 len)
{
	struct cmd_payload *payload = NULL;

	payload = (struct cmd_payload *)buff;

	return xradio_low_cmd_res_handle(payload);
}

int xraido_low_cmd_dev_hand_way(void)
{
	int ret = -1;
	struct cmd_payload *payload = NULL;
	struct cmd_para_hand_way hand_way = {
		.id = XRADIO_VERSION_ID,
	};
	struct cmd_para_hand_way_res event = {0};

	payload = xradio_low_cmd_payload_zalloc(XR_WIFI_HOST_HAND_WAY, sizeof(struct cmd_para_hand_way),
						&hand_way, sizeof(struct cmd_para_hand_way));
	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_cmd((u8 *)payload, sizeof(struct cmd_payload) + payload->len,
						&event, sizeof(struct cmd_para_hand_way_res));
	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "hand way send data failed\n");
	if (event.id == XRADIO_VERSION_ID + 1) {
		cmd_printk(XRADIO_DBG_ALWY, "Host hand way dev success.\n");
	} else {
		cmd_printk(XRADIO_DBG_ERROR, "Host hand way dev failed:%d\n", event.id);
		ret = -1;
	}
	xradio_low_cmd_payload_free(payload);

	return ret;
}

int xradio_get_fmac_macaddr_drv(struct cmd_para_mac_info *mac_info)
{
	int ret = -1;
	struct cmd_para_mac_info mi = {0};
	struct cmd_payload *payload = NULL;

	payload = xradio_low_cmd_payload_zalloc(XR_WIFI_HOST_GET_DEVICE_MAC_DRV_REQ, 0, NULL, 0);
	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_cmd((u8 *)payload, sizeof(struct cmd_payload) + payload->len, &mi, sizeof(struct cmd_para_mac_info));
	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "get mac send data failed\n");
	xradio_low_cmd_payload_free(payload);

	memcpy(mac_info, &mi, sizeof(struct cmd_para_mac_info));

	return ret;
}

int xradio_set_fmac_macaddr(uint8_t *mac_addr)
{
	int ret = -1;
	struct cmd_para_mac_info mi = {0};
	struct cmd_payload *payload = NULL;
	int res = 1;

	memcpy(mi.mac, mac_addr, 6);
	memcpy(mi.ifname, "xr", 3);
	payload = xradio_low_cmd_payload_zalloc(XR_WIFI_HOST_SET_DEVICE_MAC_REQ, sizeof(struct cmd_para_mac_info),
						&mi, sizeof(struct cmd_para_mac_info));
	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_cmd((u8 *)payload, sizeof(struct cmd_payload) + payload->len, &res, sizeof(int));
	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "set mac send data failed\n");
	if (res) {
		cmd_printk(XRADIO_DBG_ERROR, "set mac send data failed res:%d\n", res);
		ret = res;
	}
	xradio_low_cmd_payload_free(payload);

	return ret;
}

int xradio_wlan_device_power(u8 enable)
{
	int ret = -1;
	int result;
	struct cmd_payload *payload = NULL;
	struct cmd_wlan_power_data wlan_power = {
		.enable = enable,
		.mode = 0, /* default STA */
	};

	payload = xradio_low_cmd_payload_zalloc(XR_WIFI_HOST_WLAN_POWER, sizeof(struct cmd_wlan_power_data),
						&wlan_power, sizeof(struct cmd_wlan_power_data));
	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_cmd((u8 *)payload, sizeof(struct cmd_payload) + payload->len,
						&result, sizeof(int));

	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "wlan device power on timeout\n");

	if (result) {
		cmd_printk(XRADIO_DBG_ERROR, "wlan device power on failed\n");
		ret = result;
	}

	xradio_low_cmd_payload_free(payload);
	return ret;
}

int xradio_low_cmd_set_efuse(u32 addr)
{
	int ret = -1;
	struct cmd_para_efuse efuse = {0};
	struct cmd_payload *payload = NULL;
	int res = 1;

	efuse.addr = addr;
	payload = xradio_low_cmd_payload_zalloc(XR_WIFI_HOST_SET_EFUSE, sizeof(struct cmd_para_efuse),
						&efuse, sizeof(struct cmd_para_efuse));
	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_cmd((u8 *)payload, sizeof(struct cmd_payload) + payload->len, &res, sizeof(int));
	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "set efuse send data failed\n");
	if (res) {
		cmd_printk(XRADIO_DBG_ERROR, "set efuse send data failed res:%d\n", res);
		ret = res;
	}
	xradio_low_cmd_payload_free(payload);

	return ret;
}

#define KEEP_AVLIVE_TIME_MS (1000 * 6) /*6S*/
#define CMD_KEEP_AVLIVE_TIMEOUT (HZ * 3)

#if KEEP_ALIVE_USE_TIMER
static struct timer_list keep_alive_gc;
static u8 timer_init;

struct workqueue_struct *keep_alive_wq;
static struct work_struct keep_alive_work;
static kp_timeout_cb keep_alive_timeout_cb;

static u32 active_count_ref;
static u32 tx_active_count;
#else
static xr_thread_handle_t keep_alive_thread;
#endif

static int xradio_low_cmd_keep_alive(void)
{
	int ret = -1;
	struct cmd_payload *payload = NULL;
	struct cmd_para_keep_alive alive = {
		.data = XRADIO_VERSION_ID,
	};
	struct cmd_para_keep_alive res;

	payload = xradio_low_cmd_payload_zalloc(XR_WIFI_HOST_KEEP_AVLIE, sizeof(struct cmd_para_keep_alive),
						&alive, sizeof(struct cmd_para_keep_alive));
	if (!payload)
		return -ENOMEM;

	ret = xradio_low_cmd_send_cmd((u8 *)payload, sizeof(struct cmd_payload) + payload->len,
						&res, sizeof(struct cmd_para_keep_alive));
	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "keep alive send data failed\n");

	if (res.data == XRADIO_VERSION_ID + 1) {
		cmd_printk(XRADIO_DBG_MSG, "kepp alive success.\n");
	} else {
		cmd_printk(XRADIO_DBG_ERROR, "kepp alive timeout:%d\n", res.data);
		ret = -1;
	}
	xradio_low_cmd_payload_free(payload);
	return ret;
}

#if KEEP_ALIVE_USE_TIMER
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
static void xradio_keep_alive_gc(unsigned long arg)
#else
static void xradio_keep_alive_gc(struct timer_list *arg)
#endif
{
	if (tx_active_count == active_count_ref) {
		if (queue_work(keep_alive_wq, &keep_alive_work) <= 0)
			cmd_printk(XRADIO_DBG_ERROR, "queue keep alive work failed.\n");
	} else {
		active_count_ref = tx_active_count;
		mod_timer(&keep_alive_gc, jiffies + msecs_to_jiffies(KEEP_AVLIVE_TIME_MS));
	}
}

void xradio_keep_alive_work(struct work_struct *work)
{
	if (xradio_low_cmd_keep_alive()) {
		cmd_printk(XRADIO_DBG_ALWY, "Reset xrlink driver:%s\n", XRADIO_VERSION);
		if (keep_alive_timeout_cb)
			keep_alive_timeout_cb();
		return ;
	}

	mod_timer(&keep_alive_gc, jiffies + msecs_to_jiffies(KEEP_AVLIVE_TIME_MS));
}

void xradio_keep_alive_update_tx_status(int value)
{
	if (!value)
		tx_active_count++;
	else {
		active_count_ref = tx_active_count;
		mod_timer(&keep_alive_gc, jiffies + msecs_to_jiffies(500));
	}
}
#else
static int xradio_keep_alive_thread(void *arg)
{
	kp_timeout_cb cb = (kp_timeout_cb)arg;

	while (1) {
		msleep(KEEP_AVLIVE_TIME_MS);

		if (xradio_low_cmd_keep_alive()) {
			cmd_printk(XRADIO_DBG_ALWY,
					"Reset xrlink driver:%s\n", XRADIO_VERSION);
			break;
		}
	}
	xradio_k_thread_exit(&keep_alive_thread);
	cb();
	return 0;
}
#endif

int xradio_keep_alive(kp_timeout_cb cb)
{
	int ret = 0;

#if KEEP_ALIVE_USE_TIMER
	if (!timer_init) {
		keep_alive_wq = create_singlethread_workqueue("kp_workqueue");
		INIT_WORK(&keep_alive_work, xradio_keep_alive_work);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
		init_timer(&keep_alive_gc);
#else
		timer_setup(&keep_alive_gc, xradio_keep_alive_gc, 0);
#endif
		keep_alive_timeout_cb = cb;
		keep_alive_gc.function = xradio_keep_alive_gc;
		timer_init = 1;
	}
	mod_timer(&keep_alive_gc, jiffies + msecs_to_jiffies(KEEP_AVLIVE_TIME_MS));
	return ret;
#else
	ret = xradio_k_thread_create(&keep_alive_thread, "keep_alive", xradio_keep_alive_thread, (void *)cb, 0, 4096);
	if (ret)
		cmd_printk(XRADIO_DBG_ERROR, "create tx and rx thread failed\n");
	return ret;
#endif
}

int xradio_low_cmd_init(void *priv)
{
	return 0;
}

void xradio_low_cmd_deinit(void *priv)
{
}
