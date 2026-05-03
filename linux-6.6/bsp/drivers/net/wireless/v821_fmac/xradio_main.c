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
#include <generated/uapi/linux/version.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/errno.h>
#include <sunxi-sid.h>

#include "xradio.h"
#include "xradio_platform.h"
#include "os_dep/os_intf.h"
#include "xradio_txrx.h"
#include "up_cmd.h"
#include "debug.h"
#include "low_cmd.h"
#include "xr_version.h"
#include "cmd_proto.h"

#define XRADIO_TX_MAX_HEADROOM (sizeof(struct xradio_hdr) + INTERFACE_HEADER_PADDING)

#ifdef XRADIO_MACPARAM_HEX
/* insmod v821_fmac.ko macaddr=0xDC,0x44,0x6D,0x00,0x00,0x00 */
static u8 xradio_macaddr_param[ETH_ALEN] = { 0x0 };

module_param_array_named(macaddr, xradio_macaddr_param, byte, NULL, S_IRUGO);
#else
/* insmod v821_fmac.ko macaddr=xx:xx:xx:xx:xx:xx */
static char *xradio_macaddr_param;
module_param_named(macaddr, xradio_macaddr_param, charp, S_IRUGO);
#endif

typedef int (*xr_net_data_tx_t)(void *priv, struct sk_buff *skb);

struct xradio_net_priv {
	struct net_device *ndev;
	struct net_device_stats stats;

	u8 mac_address[6];
	u8 if_type;
	u8 if_id;

	xr_net_data_tx_t tx_func;
	void *xradio_priv;
};

static int xradio_iface_open(struct net_device *ndev)
{
	xradio_printk(XRADIO_DBG_MSG, "carrier_off\n");
	if (netif_carrier_ok(ndev)) {
		netif_carrier_off(ndev);
	}
	return 0;
}

static int xradio_iface_stop(struct net_device *ndev)
{
	xradio_printk(XRADIO_DBG_MSG, "carrier_off\n");
	netif_stop_queue(ndev);
	if (netif_carrier_ok(ndev)) {
		netif_carrier_off(ndev);
	}
	return 0;
}

static struct net_device_stats *xradio_iface_get_stats(struct net_device *ndev)
{
	struct xradio_net_priv *priv = netdev_priv(ndev);

	return &priv->stats;
}

static int xradio_iface_set_mac_address(struct net_device *ndev, void *data)
{
	struct xradio_net_priv *priv = netdev_priv(ndev);
	struct sockaddr *mac_addr = data;

	if (!priv)
		return -EINVAL;

	ether_addr_copy(priv->mac_address, mac_addr->sa_data);
	ether_addr_copy(ndev->dev_addr, mac_addr->sa_data);
	return 0;
}

static int xradio_iface_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int ret;
	struct xradio_net_priv *priv = netdev_priv(ndev);

	if (!priv) {
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	if (netif_queue_stopped((const struct net_device *)priv->ndev))
		return NETDEV_TX_BUSY;

	if (!skb->len || (skb->len > ETH_FRAME_LEN)) {
		priv->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}
	ret = priv->tx_func(priv->xradio_priv, skb);
	if (ret) {
		priv->stats.tx_errors++;
	} else {
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += skb->len;
	}

	return ret;
}

void xradio_iface_net_tx_pause(void *priv)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (net_priv->ndev && !netif_queue_stopped(net_priv->ndev))
		netif_stop_queue(net_priv->ndev);
}

void xradio_iface_net_tx_resume(void *priv)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (net_priv->ndev && netif_queue_stopped(net_priv->ndev))
		netif_wake_queue(net_priv->ndev);
}

int xradio_iface_net_input(void *priv, struct sk_buff *skb)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	skb->dev = net_priv->ndev;
	net_priv->stats.rx_bytes += skb->len;
	net_priv->stats.rx_packets++;

	//debug_dump("[iface1]", 32, (char *)skb->data, 20);
	skb->protocol = eth_type_trans(skb, skb->dev);
	skb->ip_summed = CHECKSUM_NONE;
//	printk("[ifacex]rx protocol:0x%x skb len:%d\n", skb->protocol, skb->len);
//	debug_dump("[iface2]", 32, (char *)skb->data, 20);
	netif_rx_ni(skb);
	return 0;
}

int xradio_iface_net_set_mac(void *priv, u8 *mac)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (!priv)
		return -EINVAL;

	ether_addr_copy(net_priv->mac_address, mac);
	ether_addr_copy(net_priv->ndev->dev_addr, mac);
	return 0;
}

#ifdef CONFIG_XRADIO_ETF
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static int xradio_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
#else
static int xradio_do_ioctl(struct net_device *dev, struct ifreq *req, void __user *data, int cmd)
#endif
{
	u16 len;
	int ret = 0;
	char buf[1024];
	struct xradio_vif *vif = netdev_priv(dev);

	///TODO: add ioctl command handler later
	switch (cmd) {
	case (SIOCDEVPRIVATE + 1):
		memset(buf, 0, 100);
		ret = copy_from_user(&len, req->ifr_data, 2);
		if (ret) {
			pr_err("%s_%d:copy_from_user failed: ret = %d\n", __func__, __LINE__, ret);
			return ret;
		}
		if (len > 100) {
			pr_err("invalid msg len: %d\n", len);
			break;
		}
		ret = copy_from_user(buf, req->ifr_data, len);
		if (ret) {
			pr_err("%s_%d:copy_from_user failed: ret = %d\n", __func__, __LINE__, ret);
			return ret;
		}
		//pr_info("send msg len: %d\n", len);
		//print_hex_dump(KERN_ERR, "addr ", DUMP_PREFIX_OFFSET, 16, 1, buf, len, 0);
		ret = xradio_send_etf_set_cmd_req(vif->xradio_hw, (u8 *)buf, (u8 *)buf, len);
		if (ret) {
			pr_err("%s_%d:set cmd req err: ret = %d\n", __func__, __LINE__, ret);
			return ret;
		}
		len = ((u16 *)buf)[0];
		//pr_info("recv msg len: %d\n", len);
		//print_hex_dump(KERN_ERR, "addr ", DUMP_PREFIX_OFFSET, 16, 1, buf, len, 0);

		ret = copy_to_user(req->ifr_data, buf, len);
		if (ret) {
			pr_err("%s_%d:copy_to_user failed: ret = %d\n", __func__, __LINE__, ret);
			return ret;
		}

		break;
	default:
		ret = -EOPNOTSUPP;
	}
	return ret;
}
#endif

static const struct net_device_ops xradio_netdev_ops = {
	.ndo_open = xradio_iface_open,
	.ndo_stop = xradio_iface_stop,
#ifdef CONFIG_XRADIO_ETF
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	.ndo_do_ioctl = xradio_do_ioctl,
#else
	.ndo_siocdevprivate = xradio_do_ioctl,
#endif
#endif
	.ndo_start_xmit = xradio_iface_hard_start_xmit,
	.ndo_set_mac_address = xradio_iface_set_mac_address,
	.ndo_get_stats = xradio_iface_get_stats,
};

static void xradio_netdev_setup(struct net_device *dev)
{
	ether_setup(dev);
	dev->needed_headroom = XRADIO_TX_MAX_HEADROOM;
}

void *xradio_iface_add_net_dev(void *priv, u8 if_type, u8 if_id, char *name, xr_net_data_tx_t tx_func)
{
	struct net_device *ndev = NULL;
	struct xradio_net_priv *net_priv = NULL;
	int ret = 0;

	ndev = alloc_netdev_mqs(sizeof(struct xradio_net_priv), name, NET_NAME_ENUM, xradio_netdev_setup, 1, 1);
	if (!ndev)
		return NULL;

	net_priv = netdev_priv(ndev);
	net_priv->ndev = ndev;
	net_priv->if_type = if_type;
	net_priv->if_id = if_id;
	memset(&net_priv->stats, 0, sizeof(net_priv->stats));

	net_priv->xradio_priv = priv;
	net_priv->tx_func = tx_func;
	ndev->netdev_ops = &xradio_netdev_ops;
	ether_addr_copy(ndev->dev_addr, net_priv->mac_address);

	ret = register_netdev(ndev);
	if (ret) {
		free_netdev(ndev);
		return NULL;
	}
	return (void *)net_priv;
}

void xradio_iface_remove_net_dev(void *priv)
{
	struct xradio_net_priv *net_priv = (struct xradio_net_priv *)priv;

	if (net_priv->ndev) {
		netif_stop_queue(net_priv->ndev);
		unregister_netdev(net_priv->ndev);
		free_netdev(net_priv->ndev);
	}
}

static int xradio_net_data_output(void *priv, struct sk_buff *skb)
{
	xradio_printk(XRADIO_DBG_MSG, "len:%d\n", skb->len);
	return xradio_tx_data_process((struct xradio_plat *)priv, skb, XR_STA_IF);
}

int xradio_add_net_dev(struct xradio_plat *priv, u8 if_type, u8 if_id, char *name)
{
	priv->net_dev_priv = xradio_iface_add_net_dev((void *)priv, if_type, if_id, name, xradio_net_data_output);
	if (priv->net_dev_priv == NULL)
		return -1;
	else
		priv->link_state = XR_LINK_UP;
	return 0;
}

void xradio_remove_net_dev(struct xradio_plat *priv)
{
	xradio_iface_remove_net_dev(priv->net_dev_priv);
	priv->link_state = XR_LINK_DOWN;
}

void xradio_net_tx_pause(struct xradio_plat *priv)
{
	xradio_printk(XRADIO_DBG_MSG, "tx_pause\n");
	xradio_iface_net_tx_pause(priv->net_dev_priv);
#ifdef STA_AP_COEXIST
	xradio_iface_net_tx_pause(priv->ap_dev_priv);
#endif
}

void xradio_net_tx_resume(struct xradio_plat *priv)
{
	xradio_printk(XRADIO_DBG_MSG, "tx_resume\n");
	xradio_iface_net_tx_resume(priv->net_dev_priv);
#ifdef STA_AP_COEXIST
	xradio_iface_net_tx_resume(priv->ap_dev_priv);
#endif
}

int xradio_net_data_input(struct xradio_plat *priv, struct sk_buff *skb)
{
	xradio_printk(XRADIO_DBG_MSG, "len:%d\n", skb->len);
	return xradio_iface_net_input(priv->net_dev_priv, skb);
}

#ifdef STA_AP_COEXIST
int xradio_net_data_input_ap(struct xradio_plat *priv, struct sk_buff *skb)
{
	xradio_printk(XRADIO_DBG_TRC, "len:%d\n", skb->len);
	return xradio_iface_net_input(priv->ap_dev_priv, skb);
}

static int xradio_net_data_output_ap(void *priv, struct sk_buff *skb)
{
	xradio_printk(XRADIO_DBG_TRC, "len:%d\n", skb->len);
	return xradio_tx_data_process((struct xradio_plat *)priv, skb, XR_AP_IF);
}

int xradio_add_net_dev_ap(struct xradio_plat *priv, u8 if_type, u8 if_id, char *name)
{
	priv->ap_dev_priv = xradio_iface_add_net_dev((void *)priv, if_type, if_id, name,
							xradio_net_data_output_ap);
	if (priv->ap_dev_priv == NULL) {
		xradio_printk(XRADIO_DBG_ERROR, "add_dev fail\n");
		return -1;
	}

	priv->ap_link_state = XR_LINK_UP;
	xradio_printk(XRADIO_DBG_NIY, "add_dev_ap success\n");

	return 0;
}

void xradio_remove_net_dev_ap(struct xradio_plat *priv)
{
	xradio_iface_remove_net_dev(priv->ap_dev_priv);
	priv->ap_link_state = XR_LINK_DOWN;
}

int xradio_net_set_mac_ap(struct xradio_plat *priv, u8 *mac)
{
	return xradio_iface_net_set_mac(priv->ap_dev_priv, mac);
}
#endif /* STA_AP_COEXIST */

int xradio_net_set_mac(struct xradio_plat *priv, u8 *mac)
{
	return xradio_iface_net_set_mac(priv->net_dev_priv, mac);
}

void xradio_notify_eth_state(struct xradio_hw *hw, void *param)
{
	xr_wifi_dev_eth_state_t *state = (xr_wifi_dev_eth_state_t *)param;
	struct xradio_net_priv *priv0 = hw->plat->net_dev_priv;

#ifdef STA_AP_COEXIST
	struct xradio_net_priv *priv1 = hw->plat->ap_dev_priv;
#endif

	xradio_printk(XRADIO_DBG_NIY, "eth_state_event is %d\n", state->event);
	if (state->event == XR_ETH0_START) {
		netif_carrier_on(priv0->ndev);
		netif_tx_start_all_queues(priv0->ndev); /* actully, only one tx queue */
	} else if (state->event == XR_ETH0_STOP) {
		netif_tx_stop_all_queues(priv0->ndev); /* actully, only one tx queue */
		if (netif_carrier_ok(priv0->ndev)) {
			netif_carrier_off(priv0->ndev);
		}
	}

#ifdef STA_AP_COEXIST
	if (state->event == XR_ETH1_START) {
		netif_carrier_on(priv1->ndev);
		netif_tx_start_all_queues(priv1->ndev); /* actully, only one tx queue */
	} else if (state->event == XR_ETH1_STOP) {
		netif_tx_stop_all_queues(priv1->ndev); /* actully, only one tx queue */
		if (netif_carrier_ok(priv1->ndev)) {
			netif_carrier_off(priv1->ndev);
		}
	}
#endif
}

#define MACADDR_VAILID(a) ( \
!(a[0] & 0x3) && \
(a[0] != 0 || a[1] != 0 ||  \
 a[2] != 0 || a[3] != 0 ||  \
 a[4] != 0 || a[5] != 0))

#if (IS_ENABLED(CONFIG_AW_MACADDR_MGT) || IS_ENABLED(CONFIG_SUNXI_ADDR_MGT)) && \
    (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
extern int get_custom_mac_address(int fmt, char *name, char *addr);
#ifndef XRADIO_MACPARAM_HEX
static int xradio_macaddr_char2val(u8 *v_mac, const char *c_mac)
{
	int i = 0;
	const char *tmp_char = c_mac;
	BUG_ON(!v_mac || !c_mac);

	for (i = 0; i < ETH_ALEN; i++) {
		if (*tmp_char != 0) {
			v_mac[i] = simple_strtoul(tmp_char, (char **)&tmp_char, 16);
		} else {
			xradio_printk(XRADIO_DBG_ERROR, "%s, Len Error\n", __func__);
			return -1;
		}
		if (i < ETH_ALEN - 1 && *tmp_char != ':') {
			xradio_printk(XRADIO_DBG_ERROR, "%s, Format or Len Error\n", __func__);
			return -1;
		}
		tmp_char++;
	}
	return 0;
}
#endif

static int xradio_get_mac_addrs(u8 *macaddr)
{
	int ret = 0;
	BUG_ON(!macaddr);
	/* Check mac addrs param, if exsist, use it first.*/
#ifdef XRADIO_MACPARAM_HEX
	memcpy(macaddr, xradio_macaddr_param, ETH_ALEN);
#else
	if (xradio_macaddr_param) {
		ret = xradio_macaddr_char2val(macaddr, xradio_macaddr_param);
	}
#endif

	if (ret == 0 && MACADDR_VAILID(macaddr))
		return 0;

	ret = get_custom_mac_address(1, "wifi", macaddr);
	if (ret > 0)
		return 0;

	return ret;
}
#else
static int xradio_get_mac_addrs(u8 *macaddr)
{
	return -1;
}
#endif

static void xradio_set_kernel_macaddr(struct xradio_plat *priv, u8 *mac)
{
	int ret;

	struct cmd_para_mac_info mac_info = {0};

	xradio_get_fmac_macaddr_drv(&mac_info);
	if (mac_info.mac[0] == 0 && mac_info.mac[1] == 0 && mac_info.mac[2] == 0 &&
		mac_info.mac[3] == 0 && mac_info.mac[4] == 0 && mac_info.mac[5] == 0) {
		/*1.First get MAC address from system.*/
		ret = xradio_get_mac_addrs(mac);
	} else {
		memcpy(mac, mac_info.mac, 6);
		ret = 0;
	}

	if (ret == 0 && MACADDR_VAILID(mac)) {
		xradio_printk(XRADIO_DBG_NIY, "Use System MAC address!\n");
		goto exit;
	}

	/*2.Use random MAC address, but don't try to save it in file. */
	get_random_bytes(mac, ETH_ALEN);
	mac[0] &= 0xFC;
	if (MACADDR_VAILID(mac)) {
		xradio_printk(XRADIO_DBG_NIY, "Use Random MAC address!\n");
	} else {
		xradio_printk(XRADIO_DBG_WARN, "Use AW OUI MAC address!\n");
		mac[0] = 0xDC;
		mac[1] = 0x44;
		mac[2] = 0x6D;
	}

exit:
	xradio_net_set_mac(priv, mac);

	xradio_printk(XRADIO_DBG_ALWY, "MACADDR=%02x:%02x:%02x:%02x:%02x:%02x\n",
		   mac[0], mac[1], mac[2],
		   mac[3], mac[4], mac[5]);

}

static int xradio_get_efuse(struct xradio_plat *priv)
{
	int ret = 1;
	struct device_node *dev_node = NULL;
	struct resource efuse_ram_res = {0};

	priv->efuse_start_addr = 0;

	dev_node = of_find_compatible_node(NULL, NULL, EFUSE_SID_BASE);
	if (IS_ERR_OR_NULL(dev_node)) {
		xradio_printk(XRADIO_DBG_ERROR, "Failed to find \"%s\" in dts.\n", EFUSE_SID_BASE);
		return -ENOMEM;
	}
	/* get efuse_ram_reserved from dts for efuse sram read */
	efuse_ram_res.start = 0;
	dev_node = of_parse_phandle(dev_node, "memory-region", 0);
	if (dev_node) {
		ret = of_address_to_resource(dev_node, 0, &efuse_ram_res);
	}
	if (efuse_ram_res.start) {
#if 0
		void __iomem *start;

		start = ioremap(efuse_ram_res.start, resource_size(&efuse_ram_res));
		if (IS_ERR_OR_NULL(start))
			return 2;
		data_hex_dump("efuse", 16, start, 256);
		iounmap(start);
#endif
		priv->efuse_start_addr = (u32)efuse_ram_res.start;
		xradio_printk(XRADIO_DBG_TRC, "Efuse addr 0x%x.\n", priv->efuse_start_addr);
		return 0;
	}
	xradio_printk(XRADIO_DBG_ERROR, "Failed to get efuse %d.\n", ret);

	return ret;
}

int xradio_all_queue_init(struct xradio_hw *xradio_hw)
{
	int ret = 0;

	xradio_printk(XRADIO_DBG_MSG, "rx_com queue init\n");
	ret = xradio_queue_init(&xradio_hw->queue_rx_com, XR_RX_COM_QUEUE_SZ, "rx_com", xradio_free_skb);
	if (ret)
		return ret;
	xradio_printk(XRADIO_DBG_MSG, "cmd_rsp queue init\n");
	ret = xradio_queue_init(&xradio_hw->queue_cmd_rsp, XR_CMD_RSP_QUEUE_SZ, "cmd_rsp", xradio_free_skb);
	if (ret)
		goto err0;
	xradio_printk(XRADIO_DBG_MSG, "rx_data queue init\n");
	ret = xradio_queue_init(&xradio_hw->queue_rx_data, XR_DATA_RX_QUEUE_SZ, "rx_data", xradio_free_skb);
	if (ret)
		goto err1;
	xradio_printk(XRADIO_DBG_MSG, "tx_data queue init\n");
	ret = xradio_queue_init(&xradio_hw->queue_tx_data, XR_DATA_TX_QUEUE_SZ, "tx_data", xradio_free_skb);
	if (ret)
		goto err2;
	return ret;
err2:
	xradio_queue_deinit(&xradio_hw->queue_rx_data);
err1:
	xradio_queue_deinit(&xradio_hw->queue_cmd_rsp);
err0:
	xradio_queue_deinit(&xradio_hw->queue_rx_com);
	return ret;
}

void xradio_all_queue_deinit(struct xradio_hw *xradio_hw)
{
	xradio_queue_deinit(&xradio_hw->queue_tx_data);
	xradio_queue_deinit(&xradio_hw->queue_rx_data);
	xradio_queue_deinit(&xradio_hw->queue_cmd_rsp);
	xradio_queue_deinit(&xradio_hw->queue_rx_com);
}

static int xradio_core_init(void)
{
	int ret = 0;
	struct xradio_plat *xradio_plat = NULL;
	struct xradio_hw *xradio_hw;
	u8 macaddr[6] = {0};

	xradio_printk(XRADIO_DBG_ALWY, "xradio core init\n");
	ret = xradio_platform_init(&xradio_plat);
	if (ret) {
		xradio_printk(XRADIO_DBG_ERROR, "alloc xradio_platform_init fail %d\n", ret);
		goto error;
	}
	xradio_hw = &xradio_plat->hw_priv;
	xradio_printk(XRADIO_DBG_ALWY, "xradio_plat %x\n", (u32)xradio_plat);
	xradio_hw->dev = xradio_platform_get_dev(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "xradio add_net_dev\n");
	if (xradio_add_net_dev(xradio_plat, XR_STA_IF, 0, "wlan0") != 0)
		goto error1;
#ifdef STA_AP_COEXIST
	if (xradio_add_net_dev_ap(xradio_plat, XR_AP_IF, 1, "wlan1") != 0)
		goto error2;
#endif
	xradio_printk(XRADIO_DBG_ALWY, "xradio debug com init\n");
	if (xradio_debug_init_common(xradio_hw))
		goto error3;

	xradio_printk(XRADIO_DBG_ALWY, "xradio queue init\n");
	ret = xradio_all_queue_init(xradio_hw);
	if (ret)
		goto error4;

	xradio_printk(XRADIO_DBG_ALWY, "xradio platform on\n");
	ret = xradio_platform_on(xradio_plat);
	if (ret)
		goto error5;

	xradio_printk(XRADIO_DBG_ALWY, "register tx and rx task\n");
	ret = xradio_register_trans(xradio_plat);
	if (ret)
		goto error6;

	xradio_printk(XRADIO_DBG_ALWY, "up cmd init\n");
	ret = xradio_up_cmd_init(xradio_plat);
	if (ret)
		goto error7;

	xradio_printk(XRADIO_DBG_ALWY, "low cmd init\n");
	ret = xradio_low_cmd_init(xradio_plat);
	if (ret)
		goto error8;

	xradio_printk(XRADIO_DBG_ALWY, "hand way\n");
	ret = xraido_low_cmd_dev_hand_way();
	if (ret)
		goto error9;

	xradio_printk(XRADIO_DBG_ALWY, "xradio core init Suceess\n");

	xradio_get_efuse(xradio_plat);
	ret = xradio_low_cmd_set_efuse(xradio_plat->efuse_start_addr);
	if (ret) {
		xradio_printk(XRADIO_DBG_ERROR, "xradio_low_cmd_set_efuse fail %d\n", ret);
	}

	xradio_set_kernel_macaddr(xradio_plat, macaddr);
	ret = xradio_set_fmac_macaddr(macaddr);
	if (ret)
		goto error9;
#ifdef STA_AP_COEXIST
	macaddr[4] ^= 0x80;
	macaddr[5] += 1;
	ret = xradio_net_set_mac_ap(xradio_plat, macaddr);
	xradio_printk(XRADIO_DBG_ALWY, "AP_ADDR=%02x:%02x:%02x:%02x:%02x:%02x\n",
		   macaddr[0], macaddr[1], macaddr[2],
		   macaddr[3], macaddr[4], macaddr[5]);
#endif
	if (ret)
		goto error9;

	xradio_printk(XRADIO_DBG_ALWY, "wlan device is opening...\n");
	ret = xradio_wlan_device_power(1);
	if (ret)
		goto error9;

	xradio_printk(XRADIO_DBG_ALWY, "wlan device is ready\n");
	return 0;
error9:
	xradio_low_cmd_deinit(xradio_plat);
error8:
	xradio_up_cmd_deinit(xradio_plat);
error7:
	xradio_unregister_trans(xradio_plat);
error6:
	xradio_platform_off(xradio_plat);
error5:
	xradio_all_queue_deinit(xradio_hw);
error4:
	xradio_debug_deinit_common(xradio_hw);
error3:
#ifdef STA_AP_COEXIST
	xradio_remove_net_dev_ap(xradio_plat);
#endif
error2:
	xradio_remove_net_dev(xradio_plat);
error1:
	xradio_platform_deinit(xradio_plat);
error:
	xradio_printk(XRADIO_DBG_ERROR, "xradio core init Failed\n");
	return ret;
}

static void xradio_core_deinit(void)
{
	struct xradio_plat *xradio_plat = xradio_get_platform();
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "wlan device power off\n");
	xradio_wlan_device_power(0);

	xradio_printk(XRADIO_DBG_ALWY, "low cmd deinit\n");
	xradio_low_cmd_deinit(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "up cmd deinit\n");
	xradio_up_cmd_deinit(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "unregister tx rx transmit\n");
	xradio_unregister_trans(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "xradio platform off\n");
	xradio_platform_off(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "xradio queue deinit\n");
	xradio_all_queue_deinit(xradio_hw);

	xradio_debug_deinit_common(xradio_hw);

	xradio_printk(XRADIO_DBG_ALWY, "xradio wlan interface dev remove\n");
#ifdef STA_AP_COEXIST
	xradio_remove_net_dev_ap(xradio_plat);
#endif
	xradio_remove_net_dev(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "xradio platform deinit\n");
	xradio_platform_deinit(xradio_plat);

	xradio_printk(XRADIO_DBG_ALWY, "xradio core deinit over\n");
}

static int __init xradio_mod_init(void)
{
	xradio_printk(XRADIO_DBG_ALWY, "xradio wlan version:%s\n", XRADIO_VERSION);

	return xradio_core_init();
}

static void __exit xradio_mod_exit(void)
{
	xradio_core_deinit();
}

module_init(xradio_mod_init);
module_exit(xradio_mod_exit);

MODULE_AUTHOR("XRadioTech");
MODULE_DESCRIPTION("XRadioTech WLAN driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xradio");
