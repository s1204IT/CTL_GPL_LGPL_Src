/*
 * xradio_rpbuf.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface rpbuf implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/rpmsg.h>
#include <linux/semaphore.h>

#include "xradio_platform.h"
#include "xradio_rpmsg.h"
#include "xradio_txrx.h"
#include "debug.h"
#include "cmd_proto.h"

#define XRADIO_RPMSG_NAME      "xrlink_rpmsg"
#define XRADIO_RPMSG_DATA_LEN  496
#define rpmsg_printk           io_printk

static struct xradio_rpmsg_dev *xr_rpmsg_dev;
static struct semaphore rpmsg_up_sem;

int xradio_rpmsg_tx(struct device *dev, u8 *msg, uint len)
{
	int ret;
	struct xradio_rpmsg_dev *rpmsg_dev = dev_get_drvdata(dev);

	rpmsg_printk(XRADIO_DBG_MSG, "xradio_rpmsg_tx len:%d\n", len);

	if (len > rpmsg_dev->len) {
		rpmsg_printk(XRADIO_DBG_ERROR, "rpmsg_send too long: %d, max:%d\n", len, rpmsg_dev->len);
		return -EINVAL;
	}

	//will block wait for rpmsg buffer is avaliable then return
	ret = rpmsg_send(rpmsg_dev->msg_dev->ept, msg, len);
	if (ret)
		rpmsg_printk(XRADIO_DBG_ERROR, "rpmsg_send failed: %d\n", ret);

	return ret;
}

static int xradio_rpmsg_cb(struct rpmsg_device *rpdev, void *data, int len, void *priv, u32 src)
{
	struct sk_buff *skb = NULL;
	struct xradio_hdr *hdr;
	int ret = 0;
	struct xradio_rpmsg_dev *rpmsg_dev = dev_get_drvdata(&rpdev->dev);
	char *q_name = NULL;
	struct cmd_payload *payload;

	rpmsg_printk(XRADIO_DBG_MSG, "recv len:%d\n", len);

	skb = __dev_alloc_skb(len, GFP_KERNEL);
	if (!skb) {
		rpmsg_printk(XRADIO_DBG_ERROR, "skb alloc failed!\n");
		return -ENOMEM;
	}

	memcpy(skb->data, data, len);
	skb_put(skb, len);
	hdr = (void *)skb->data;
	payload = (void *)hdr->payload;
	txrx_printk(XRADIO_DBG_TRC, "%s xr type:%d cmd %d\n", __func__, hdr->type, payload->type);

	if (hdr->type == XR_TYPE_CMD_RSP) {
		ret = xradio_msg_queue_put(&rpmsg_dev->hw_priv->queue_cmd_rsp, skb);
		q_name = rpmsg_dev->hw_priv->queue_cmd_rsp.name;
		if (!ret)
			xradio_wake_up_txcmd_rsp_work(rpmsg_dev->hw_priv);
	} else {
		ret = xradio_msg_queue_put(&rpmsg_dev->hw_priv->queue_rx_com, skb);
		q_name = rpmsg_dev->hw_priv->queue_rx_com.name;
		if (!ret)
			xradio_wake_up_rx_com_work(rpmsg_dev->hw_priv);
	}
	if (ret) {
		kfree_skb(skb);
		rpmsg_printk(XRADIO_DBG_WARN, "push %s queue fail:%d, discard type %d!\n",
				q_name, ret, hdr->type);
		return ret;
	}

	return ret;
}

static int xradio_rpmsg_probe(struct rpmsg_device *rpdev)
{
	struct xradio_rpmsg_dev *rpmsg_dev;

	rpmsg_dev = devm_kzalloc(&rpdev->dev, sizeof(struct xradio_rpmsg_dev), GFP_KERNEL);
	if (!rpmsg_dev) {
		rpmsg_printk(XRADIO_DBG_ERROR, "no mem!\n");
		return -ENOMEM;
	}

	rpmsg_printk(XRADIO_DBG_MSG, "new channel: 0x%x -> 0x%x!\n", rpdev->src, rpdev->dst);

	/* we need to announce the new ept to remote */
	rpdev->announce = (rpdev->src != RPMSG_ADDR_ANY);

	rpmsg_dev->dev = &rpdev->dev;
	rpmsg_dev->msg_dev = rpdev;
	rpmsg_dev->len = XRADIO_RPMSG_DATA_LEN;
	xr_rpmsg_dev = rpmsg_dev;
	dev_set_drvdata(rpmsg_dev->dev, rpmsg_dev);

	rpmsg_printk(XRADIO_DBG_MSG, "xrlink msg up!\n");
	up(&rpmsg_up_sem);
	rpmsg_printk(XRADIO_DBG_MSG, "xrlink msg ready!\n");
	return 0;
}

static void xradio_rpmsg_remove(struct rpmsg_device *rpdev)
{
	if (!xr_rpmsg_dev) {
		rpmsg_printk(XRADIO_DBG_ERROR, "xr_rpmsg_dev is NULL\n");
		return;
	}
	devm_kfree(&rpdev->dev, xr_rpmsg_dev);
	xr_rpmsg_dev = NULL;
	rpmsg_printk(XRADIO_DBG_ALWY, "xradio master rpmsg client driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_xradio_id_table[] = {
	{ .name = XRADIO_RPMSG_NAME },
	{ },
};

static struct rpmsg_driver rpmsg_xradio_client = {
	.drv.name	= XRADIO_RPMSG_NAME,
	.id_table	= rpmsg_driver_xradio_id_table,
	.probe		= xradio_rpmsg_probe,
	.callback	= xradio_rpmsg_cb,
	.remove		= xradio_rpmsg_remove,
};

int xradio_rpmsg_init(struct xradio_bus *bus_if, struct xradio_hw *hw_priv)
{
	int ret = 0;

	sema_init(&rpmsg_up_sem, 0);
	ret = register_rpmsg_driver(&rpmsg_xradio_client);
	if (ret) {
		rpmsg_printk(XRADIO_DBG_ERROR, "register_rpmsg_driver fail %d\n", ret);
		return ret;
	}

	if (down_timeout(&rpmsg_up_sem, msecs_to_jiffies(10000)) != 0) {
		unregister_rpmsg_driver(&rpmsg_xradio_client);
		rpmsg_printk(XRADIO_DBG_ERROR, "probe timeout!\n");
		return -1;
	}
	rpmsg_printk(XRADIO_DBG_MSG, "xr_rpmsg_dev %x\n", (u32)xr_rpmsg_dev);
	bus_if->rpmsg_dev = xr_rpmsg_dev;
	bus_if->rpmsg_dev->hw_priv = hw_priv;
	bus_if->rpmsg_dev->bus_if = bus_if;
	bus_if->ops->txmsg = xradio_rpmsg_tx;
	bus_if->ops->rxmsg = NULL;
	rpmsg_printk(XRADIO_DBG_ALWY, "xrlink rpmsg creat succeed\n");

	return 0;
}

void xradio_rpmsg_deinit(struct xradio_bus *bus_if)
{
	unregister_rpmsg_driver(&rpmsg_xradio_client);
}
