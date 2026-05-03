/*
 * v821_wlan/up_cmd.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * usrer protocol flow implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>

#include "os_dep/os_intf.h"
#include "xradio.h"
#include "up_cmd.h"
#include "xradio_txrx.h"
#include "debug.h"

struct up_cmd_priv {
	struct sock *nlsk;
	void *dev_priv;
	u32 user_pid;
	xr_mutex_t tx_mutex;
};

extern struct net init_net;

static struct up_cmd_priv cmd_priv = {
	.nlsk = NULL,
	.user_pid = 0,
};

#define XR_WLAN_NETLINK_ID 28

#define XR_NETLINK_TYPE_CMD 0
#define XR_NETLINK_TYPE_ACK 1

#define XR_NETLINK_PREFIX_LEN 3
#define XR_NETLINK_PREFIX_CMD "cmd"
#define XR_NETLINK_PREFIX_ACK "ack"

static struct xradio_hdr *xradio_up_cmd_zalloc(void *payload, u32 payload_len)
{
	struct xradio_hdr *xr_hdr = NULL;

	xr_hdr = (struct xradio_hdr *)xradio_k_zmalloc(XR_HDR_SZ + payload_len);
	if (!xr_hdr)
		return NULL;

	if (payload)
		memcpy(xr_hdr->payload, payload, payload_len);

	return xr_hdr;
}

static void xradio_up_cmd_free(struct xradio_hdr *xr_hdr)
{
	if (xr_hdr) {
		xradio_k_free(xr_hdr);
	}
}

static int xradio_up_cmd_send_cmd(struct xradio_hdr *xr_hdr, u16 len, void *cfm, u16 cfm_len)
{
	int ret = -1;

	xr_hdr->type = XR_TYPE_CMD;
	xr_hdr->len = len;
	ret = xradio_tx_com_process(xr_hdr, cfm, cfm_len);

	return ret;
}

int xradio_up_cmd_send_event(struct xradio_hdr *xr_hdr, u16 len, void *cfm, u16 cfm_len)
{
	int ret = -1;

	xr_hdr->type = XR_TYPE_EVENT;
	xr_hdr->len = len;
	ret = xradio_tx_com_process(xr_hdr, cfm, cfm_len);

	return ret;
}

static int xradio_netlink_send_msg(u8 type, u8 *buff, u32 len)
{
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *skb = NULL;
	int total_len;
	int ret = 0;

	if (!cmd_priv.nlsk || !cmd_priv.user_pid)
		return -EINVAL;

	total_len = XR_NETLINK_PREFIX_LEN + len;

	skb = nlmsg_new(total_len, GFP_ATOMIC);
	if (!skb) {
		cmd_printk(XRADIO_DBG_ERROR, "malloc nl msg failed\n");
		return -ENOMEM;
	}

	nlh = nlmsg_put(skb, 0, 0, XR_WLAN_NETLINK_ID, total_len, 0);

	if (type == XR_NETLINK_TYPE_CMD) {
		memcpy(NLMSG_DATA(nlh), XR_NETLINK_PREFIX_CMD, XR_NETLINK_PREFIX_LEN);
		memcpy(NLMSG_DATA(nlh) + XR_NETLINK_PREFIX_LEN, buff, len);
	} else if (type == XR_NETLINK_TYPE_ACK) {
		memcpy(NLMSG_DATA(nlh), XR_NETLINK_PREFIX_ACK, XR_NETLINK_PREFIX_LEN);
	}

	ret = netlink_unicast(cmd_priv.nlsk, skb, cmd_priv.user_pid, MSG_DONTWAIT);
	if (ret < 0)
		cfg_printk(XRADIO_DBG_ERROR, "Error while sending back to user\n");
	cfg_printk(XRADIO_DBG_MSG, "type %d ret %d\n", type, ret);

	return (ret < 0);
}

int xradio_rx_up_cmd_push(u8 *buf, u32 len)
{
	cfg_printk(XRADIO_DBG_MSG, "rx up cmd len %d\n", len);
	if (dbg_cfg & XRADIO_DBG_TRC)
		data_hex_dump("up-cmd:", 16, (void *)buf, len > 64 ? 64 : len);

	return xradio_netlink_send_msg(XR_NETLINK_TYPE_CMD, buf, len);
}

int xradio_send_up_is_cmd(void *buf)
{
	//struct cmd_payload *cmd = buf;
	int is_cmd = 1;

	return is_cmd;
}

static void xradio_netlink_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	int ret;

	xradio_k_mutex_lock(&cmd_priv.tx_mutex);
	if (skb->len >= nlmsg_total_size(0)) {
		nlh = nlmsg_hdr(skb);

		cmd_priv.user_pid = nlh->nlmsg_pid;
		if (nlh) {
			struct xradio_hdr *hdr;

			hdr = xradio_up_cmd_zalloc(NLMSG_DATA(nlh), nlh->nlmsg_len - NLMSG_HDRLEN);
			if (!hdr)
				return;
			xradio_netlink_send_msg(XR_NETLINK_TYPE_ACK, NULL, 0);
			if (xradio_send_up_is_cmd(hdr->payload))
				ret = xradio_up_cmd_send_cmd(hdr, nlh->nlmsg_len - NLMSG_HDRLEN, NULL, 0);
			else
				ret = xradio_up_cmd_send_event(hdr, nlh->nlmsg_len - NLMSG_HDRLEN, NULL, 0);
			xradio_up_cmd_free(hdr);
			if (ret)
				cfg_printk(XRADIO_DBG_ERROR, "cmd process error %d\n", ret);
		}
	}
	xradio_k_mutex_unlock(&cmd_priv.tx_mutex);

}

struct netlink_kernel_cfg xradio_up_cmd = {
	.input = xradio_netlink_recv_msg,
};

int xradio_up_cmd_init(void *priv)
{
	memset(&cmd_priv, 0, sizeof(struct up_cmd_priv));
	cmd_priv.nlsk = (struct sock *)netlink_kernel_create(&init_net, XR_WLAN_NETLINK_ID, &xradio_up_cmd);
	if (!cmd_priv.nlsk)
		return -ENOMEM;

	xradio_k_mutex_init(&cmd_priv.tx_mutex);

	cmd_priv.dev_priv = priv;

	return 0;
}

int xradio_up_cmd_deinit(void *priv)
{
	if (cmd_priv.nlsk) {
		netlink_kernel_release(cmd_priv.nlsk);
		cmd_priv.nlsk = NULL;
		cmd_priv.user_pid = 0;
		xradio_k_mutex_deinit(&cmd_priv.tx_mutex);
		cmd_priv.dev_priv = NULL;
	}

	return 0;
}
