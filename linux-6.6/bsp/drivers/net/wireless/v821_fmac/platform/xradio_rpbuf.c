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

#include "xradio_platform.h"
#include "xradio_rpbuf.h"
#include "xradio_txrx.h"
#include "debug.h"
#include "cmd_proto.h"

#define rpbuf_printk                            io_printk
#define rpbuf_dump(pref, buf, len)              //data_hex_dump(pref, 16, buf, len)
#define rpbuf_err_dump(pref, buf, len)          data_hex_dump(pref, 16, buf, len)

static struct semaphore rpbuf_up_sem;
static struct xradio_rpbuf_dev *xr_rpbuf_dev;

static int xradio_rpbuf_alloc_payload_memory(struct rpbuf_buffer *buffer, void *priv)
{
	int ret;
	void *va;
	dma_addr_t pa;
	struct device *dev = priv;

	va = dma_alloc_coherent(dev, rpbuf_buffer_len(buffer), &pa, GFP_KERNEL);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "dma_alloc_coherent for len %d failed\n", rpbuf_buffer_len(buffer));
		ret = -ENOMEM;
		goto err_out;
	}
	dev_dbg(dev, "allocate payload memory: va 0x%pK, pa %pad, len %d\n", va, &pa, rpbuf_buffer_len(buffer));
	rpbuf_buffer_set_va(buffer, va);
	rpbuf_buffer_set_pa(buffer, (phys_addr_t)pa);

	return 0;
err_out:
	return ret;
}

static void xradio_rpbuf_free_payload_memory(struct rpbuf_buffer *buffer, void *priv)
{
	void *va;
	dma_addr_t pa;
	struct device *dev = priv;

	va = rpbuf_buffer_va(buffer);
	pa = (dma_addr_t)(rpbuf_buffer_pa(buffer));

	dev_dbg(dev, "free payload memory: va %pK, pa %pad, len %d\n", va, &pa, rpbuf_buffer_len(buffer));
	dma_free_coherent(dev, rpbuf_buffer_len(buffer), va, pa);
}

/*
 * Optional:
 *   We can define our own operations to overwrite the default ones.
 */
static const struct rpbuf_buffer_ops xradio_rpbuf_ops = {
	.alloc_payload_memory = xradio_rpbuf_alloc_payload_memory,
	.free_payload_memory = xradio_rpbuf_free_payload_memory,
};

static void xradio_rpbuf_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	//struct device *dev = priv;
	//struct xradio_rpbuf_dev *rpbuf_dev = dev_get_drvdata(dev);

	rpbuf_printk(XRADIO_DBG_NIY, "buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int xradio_rpbuf_rx_cb(struct rpbuf_buffer *buffer, void *data, int data_len, void *priv)
{
	struct device *dev = priv;
	struct xradio_rpbuf_dev *rpbuf_dev = dev_get_drvdata(dev);
	struct sk_buff *skb = NULL;
	struct xradio_agg_hdr *agg_hdr;
	struct xradio_hdr *hdr;
	int i = 0, rx_len = 0, ret = 0;
	u8 agg_cnt, recived = 0;
	u8 *src;

	src = data;
	agg_hdr = (struct xradio_agg_hdr *)src;
	agg_cnt = agg_hdr->agg_cnt;
	src += XR_AGG_HDR_SZ;
	rpbuf_printk(XRADIO_DBG_MSG, "buffer \"%s\" received data (offset: %d, len: %d), agg_cnt:%d\n",
				rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len,
				agg_cnt);

	for (i = 0; i < agg_cnt; i++) {
		hdr = (struct xradio_hdr *)src;
		rpbuf_printk(XRADIO_DBG_MSG, "agg:%d/%d len=%d hdr:%x\n", i, agg_cnt, XR_HDR_SZ + hdr->len, (u32)hdr);
		skb = NULL;
		rx_len += XR_HDR_SZ + hdr->len;
		if ((rx_len > data_len) || (rx_len > rpbuf_dev->len_mrx)) {
			int loop;
			u8 *loop_buf;
			struct xradio_hdr *loop_hdr;

			rpbuf_printk(XRADIO_DBG_ERROR, "agg:%d/%d len=%d, data:%x hdr:%x\n",
						i, agg_cnt, XR_HDR_SZ + hdr->len, (u32)data, (u32)hdr);
			rpbuf_printk(XRADIO_DBG_ERROR, "data err:rx_len:%d data_len:%d bufsz:%d now:%d\n",
				rx_len, data_len, rpbuf_dev->len_mrx, XR_HDR_SZ + hdr->len);

			loop_buf = data;
			loop_buf += XR_AGG_HDR_SZ;
			for (loop = 0; loop < agg_cnt; loop++) {
				loop_hdr = (struct xradio_hdr *)loop_buf;
				rpbuf_printk(XRADIO_DBG_ERROR, "[LOOP] agg:%d/%d len=%d, hdr:%x\n",
						loop + 1, agg_cnt, XR_HDR_SZ + loop_hdr->len, (u32)loop_hdr);
				rpbuf_err_dump("LOOP:", (void *)loop_hdr, (XR_HDR_SZ + loop_hdr->len) > 32 ? 32 : (XR_HDR_SZ + loop_hdr->len));
				loop_buf += (XR_HDR_SZ + loop_hdr->len);
			}
			rpbuf_err_dump("DATA:", data, data_len > 128 ? 128 : data_len);
			rpbuf_err_dump("HDR :", (void *)hdr, (XR_HDR_SZ + hdr->len) > 128 ? 128 : (XR_HDR_SZ + hdr->len));
			agg_hdr->ack_cnt = i;
			agg_hdr->rsp_code = XR_TXD_ST_DATA_ERR;
			goto end;
		}
		if (i >= agg_hdr->start_idx) {
			struct cmd_payload *payload = (void *)hdr->payload;

			rpbuf_printk(XRADIO_DBG_TRC, "agg:%d/%d len=%d\n", i, agg_cnt, XR_HDR_SZ + hdr->len);
			txrx_printk(XRADIO_DBG_TRC, "%s xr type:%d cmd %d\n", __func__, hdr->type, payload->type);
			skb = __dev_alloc_skb(hdr->len + XR_HDR_SZ, GFP_KERNEL);
			if (!skb) {
				//XRADIO_DBG_NIY
				rpbuf_printk(XRADIO_DBG_NIY, "Malloc fail len:%d, discard data %d!\n",
					hdr->len + XR_HDR_SZ, agg_cnt - i);
				agg_hdr->ack_cnt = i;
				agg_hdr->rsp_code = XR_TXD_ST_NO_MEM;
				goto end;
			}
			memcpy(skb_put(skb, hdr->len + XR_HDR_SZ), src, hdr->len + XR_HDR_SZ);
			ret = xradio_rx_data_pre_process(rpbuf_dev->hw_priv, skb, src, hdr->len + XR_HDR_SZ);
			if (ret > 0) {
				ret = xradio_data_rx_queue_put(&rpbuf_dev->hw_priv->queue_rx_data, skb);
				if (ret) {
					kfree_skb(skb);
					//XRADIO_DBG_NIY
					rpbuf_printk(XRADIO_DBG_NIY, "push rx queue fail:%d, discard data %d!\n",
						ret, agg_cnt - i);
					agg_hdr->ack_cnt = i;
					agg_hdr->rsp_code = XR_TXD_ST_NO_QUEUE;
					goto end;
				}
				recived++;
			}
		}
		src += (XR_HDR_SZ + hdr->len);
		rpbuf_printk(XRADIO_DBG_MSG, "agg:%d/%d len=%d hdr:%x src:%x\n", i, agg_cnt, XR_HDR_SZ + hdr->len, (u32)hdr, (u32)src);
	}

	agg_hdr->ack_cnt = i;
	agg_hdr->rsp_code = XR_TXD_ST_SUCESS;
end:
	if (recived)
		xradio_wake_up_data_rx_work(rpbuf_dev->hw_priv);
	rpbuf_printk(XRADIO_DBG_NIY, "==>agg:%d/%d ack:%d start:%d\n", i, agg_cnt, agg_hdr->ack_cnt, agg_hdr->start_idx);
	return 0;
}

static int xrdio_rpbuf_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	rpbuf_printk(XRADIO_DBG_ALWY, "buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));

	return 0;
}

static const struct rpbuf_buffer_cbs xradio_rpbuf_mtx_cbs = {
	.available_cb = xradio_rpbuf_available_cb,
	.rx_cb = NULL,
	.destroyed_cb = xrdio_rpbuf_destroyed_cb,
};

static const struct rpbuf_buffer_cbs xradio_rpbuf_mrx_cbs = {
	.available_cb = xradio_rpbuf_available_cb,
	.rx_cb = xradio_rpbuf_rx_cb,
	.destroyed_cb = xrdio_rpbuf_destroyed_cb,
};

#ifdef XRADIO_RPBUF_PERF_TEST
#define RPBUF_AGG_PERF_DBG
#endif
#ifdef RPBUF_AGG_PERF_DBG
static int g_rpbuf_tx_cnt, g_rpbuf_agg_cnt, g_rpbuf_agg_min, g_rpbuf_agg_max;
#define RPBUF_AGG_PERF_DBG_THRESH    5000
#endif

static void rpbuf_txbuf_reset(struct xradio_rpbuf_dev *rpbuf_dev)
{
	struct rpbuf_buffer *buffer = rpbuf_dev->buffer_mtx;
	struct xradio_agg_hdr *agg_hdr;

	rpbuf_dev->buffer_mtx_tail = rpbuf_buffer_va(buffer);
	rpbuf_dev->buffer_mtx_used_len = 0;
	rpbuf_dev->buffer_mtx_agg_count = 0;
	rpbuf_dev->buffer_mtx_free_len = rpbuf_buffer_len(buffer);

	// first pkt
	agg_hdr = (void *)rpbuf_dev->buffer_mtx_tail;
	memset(agg_hdr, 0, XR_AGG_HDR_SZ);
	agg_hdr->rsp_code = XR_TXD_ST_NO_RSP;
	rpbuf_dev->buffer_mtx_tail += XR_AGG_HDR_SZ;
	rpbuf_dev->buffer_mtx_used_len += XR_AGG_HDR_SZ;
}

static __inline__ int rpbuf_send(struct device *dev, struct xradio_rpbuf_dev *rpbuf_dev)
{
	int ret, i;
	struct rpbuf_buffer *buffer = rpbuf_dev->buffer_mtx;
	struct xradio_agg_hdr *agg_hdr;
	u32 total_len = 0;
	u8 *data;

#ifdef RPBUF_AGG_PERF_DBG
	g_rpbuf_tx_cnt++;
	g_rpbuf_agg_cnt += rpbuf_dev->buffer_mtx_agg_count;
	if (rpbuf_dev->buffer_mtx_agg_count < g_rpbuf_agg_min)
		g_rpbuf_agg_min = rpbuf_dev->buffer_mtx_agg_count;
	else if (g_rpbuf_agg_min == 0)
		g_rpbuf_agg_min = rpbuf_dev->buffer_mtx_agg_count;
	if (rpbuf_dev->buffer_mtx_agg_count > g_rpbuf_agg_max)
		g_rpbuf_agg_max = rpbuf_dev->buffer_mtx_agg_count;
	else if (g_rpbuf_agg_max == 0)
		g_rpbuf_agg_max = rpbuf_dev->buffer_mtx_agg_count;

	if (g_rpbuf_tx_cnt >= RPBUF_AGG_PERF_DBG_THRESH) {
		int avg = g_rpbuf_agg_cnt / g_rpbuf_tx_cnt;

		rpbuf_printk(XRADIO_DBG_ALWY, "cnt:%d agged:%d %d-%d-%d l:%d b:%d\n", g_rpbuf_tx_cnt, g_rpbuf_agg_cnt,
				g_rpbuf_agg_min, avg, g_rpbuf_agg_max, rpbuf_dev->buffer_mtx_used_len, rpbuf_buffer_len(buffer));
		g_rpbuf_tx_cnt = g_rpbuf_agg_cnt = g_rpbuf_agg_min = g_rpbuf_agg_max = 0;
	}
#endif

	agg_hdr = (void *)rpbuf_buffer_va(buffer);
	if ((agg_hdr->agg_cnt != rpbuf_dev->buffer_mtx_agg_count) ||
		(agg_hdr->agg_cnt > rpbuf_dev->buffer_mtx_agg_max)) {
		rpbuf_printk(XRADIO_DBG_ERROR, "data err! hdr aggcnt:%d, agg cnt:%d max:%d\n",
					agg_hdr->agg_cnt, rpbuf_dev->buffer_mtx_agg_count, rpbuf_dev->buffer_mtx_agg_max);
		goto data_err;
	}
	if (rpbuf_dev->buffer_mtx_used_len <= XR_AGG_HDR_SZ) {
		rpbuf_printk(XRADIO_DBG_ERROR, "data err! used_len too small:%d agg hdr sz:%d\n",
						rpbuf_dev->buffer_mtx_used_len, XR_AGG_HDR_SZ);
		goto data_err;
	}

	data = ((u8 *)agg_hdr) + XR_AGG_HDR_SZ;
	total_len += XR_AGG_HDR_SZ;
	for (i = 0; i < agg_hdr->agg_cnt; i++) {
		struct xradio_hdr *hdr;

		hdr = (void *)data;
		total_len += XR_HDR_SZ + hdr->len;
		if (total_len > rpbuf_buffer_len(buffer)) {
			rpbuf_printk(XRADIO_DBG_ERROR, "data err! agg:%d/%d, len:%d total-len:%d bufsz:%d\n",
						i + 1, agg_hdr->agg_cnt, XR_HDR_SZ + hdr->len, total_len, rpbuf_buffer_len(buffer));

			data = (u8 *)agg_hdr + XR_AGG_HDR_SZ;
			for (i = 0; i < agg_hdr->agg_cnt; i++) {
				hdr = (void *)data;
				rpbuf_printk(XRADIO_DBG_ERROR, "dump: agg:%d/%d, len:%d\n",
							i + 1, agg_hdr->agg_cnt, XR_HDR_SZ + hdr->len);
				data += XR_HDR_SZ + hdr->len;
			}
			goto data_err;
		}
		data += XR_HDR_SZ + hdr->len;
	}
	if (total_len != rpbuf_dev->buffer_mtx_used_len) {
		rpbuf_printk(XRADIO_DBG_ERROR, "data err! len err, used_len:%d total_len:%d\n",
						rpbuf_dev->buffer_mtx_used_len, total_len);
		goto data_err;
	}

	ret = rpbuf_transmit_buffer(buffer, 0, rpbuf_dev->buffer_mtx_used_len);
	if (ret) {
		rpbuf_printk(XRADIO_DBG_ERROR, "rpbuf_transmit_buffer failed, len:%d, agg:%d, ret:%d\n",
			rpbuf_dev->buffer_mtx_used_len, agg_hdr->agg_cnt, ret);
		ret = XR_TXD_ST_BUS_TX_FAIL;
	} else {
#ifdef RPBUF_NO_CHECK_RSP
		rpbuf_txbuf_reset(rpbuf_dev);
		return 0;
#endif
		if (agg_hdr->rsp_code == XR_TXD_ST_SUCESS) {
			rpbuf_txbuf_reset(rpbuf_dev);
			ret = XR_TXD_ST_SUCESS;
		} else {
			ret = agg_hdr->rsp_code;
			rpbuf_printk(XRADIO_DBG_NIY, "rsp_code:%d, ack:%d, start:%d agg:%d %x\n",
					agg_hdr->rsp_code, agg_hdr->ack_cnt, agg_hdr->start_idx, agg_hdr->agg_cnt, (u32)agg_hdr);
		}
	}

	return ret;
data_err:
	ret = XR_TXD_ST_DATA_ERR;
	return ret;
}

static int xradio_rpbuf_write(struct device *dev, const char *buf, size_t count, enum xradio_txbus_op op_code)
{
	struct xradio_rpbuf_dev *rpbuf_dev = dev_get_drvdata(dev);
	struct rpbuf_buffer *buffer = rpbuf_dev->buffer_mtx;
	const char *buf_name;
	int buf_len;
	int data_len = count;
	struct xradio_agg_hdr *agg_hdr;
	int ret = 0;
	struct xradio_hdr *xr_hdr;

	if (!buffer) {
		rpbuf_printk(XRADIO_DBG_ERROR, "buffer \"%s\" not created\n", rpbuf_dev->name_mtx);
		return -ENOMEM;
	}
	buf_name = rpbuf_buffer_name(buffer);
	buf_len = rpbuf_buffer_len(buffer);
	if (!rpbuf_buffer_is_available(buffer)) {
		rpbuf_printk(XRADIO_DBG_ERROR, "buffer \"%s\" not available\n", buf_name);
		return -EACCES;
	}
	agg_hdr = rpbuf_buffer_va(buffer);
	rpbuf_printk(XRADIO_DBG_TRC, "op %d, buf %x agg_cnt:%d %d\n", op_code, (u32)agg_hdr,
				agg_hdr->agg_cnt, rpbuf_dev->buffer_mtx_agg_count);

	if (op_code == XR_TXBUS_OP_RESET_BUF) {
		rpbuf_txbuf_reset(rpbuf_dev);
		return 0;
	} else if (op_code == XR_TXBUS_OP_TX_RETRY) {
		agg_hdr->start_idx = agg_hdr->ack_cnt;
		agg_hdr->rsp_code = XR_TXD_ST_NO_RSP;
		rpbuf_printk(XRADIO_DBG_MSG, "retry start:%d agg:%d ack:%d %x\n",
			agg_hdr->start_idx, agg_hdr->agg_cnt, agg_hdr->ack_cnt, (u32)agg_hdr);
		ret = rpbuf_send(dev, rpbuf_dev);
		if (ret)
			return ret;
	}

	if ((rpbuf_dev->buffer_mtx_used_len + data_len) > buf_len) {
		if (rpbuf_dev->buffer_mtx_used_len > XR_AGG_HDR_SZ) {
			ret = rpbuf_send(dev, rpbuf_dev);
			if (ret)
				return ret;
		} else {
			rpbuf_printk(XRADIO_DBG_ERROR, "no data, len:%d bufsz:%d\n", data_len, rpbuf_buffer_len(buffer));
			return -EINVAL;
		}
	}
	if (!buf || !count) {
		rpbuf_printk(XRADIO_DBG_ERROR, "no data, len:%d pkt:%p\n", data_len, buf);
		return -EINVAL;
	}
	if (data_len > buf_len) {
		rpbuf_printk(XRADIO_DBG_ERROR, "data to big, len:%d bufsz:%d\n", data_len, rpbuf_buffer_len(buffer));
		return -EINVAL;
	}

	// agg data
	if ((rpbuf_dev->buffer_mtx_used_len == 0) || !rpbuf_dev->buffer_mtx_tail) {
		rpbuf_txbuf_reset(rpbuf_dev);
	}
	memcpy(rpbuf_dev->buffer_mtx_tail, buf, data_len);
	xr_hdr = (void *)rpbuf_dev->buffer_mtx_tail;
	rpbuf_dev->buffer_mtx_tail += data_len;
	rpbuf_dev->buffer_mtx_used_len += data_len;
	rpbuf_dev->buffer_mtx_agg_count += 1;
	rpbuf_dev->buffer_mtx_free_len = buf_len - rpbuf_dev->buffer_mtx_used_len;
	agg_hdr->agg_cnt += 1;
	rpbuf_printk(XRADIO_DBG_TRC, "agg data %d len:%d used:%d free:%d xr_type %d\n",
				agg_hdr->agg_cnt, data_len, rpbuf_dev->buffer_mtx_used_len,
				rpbuf_dev->buffer_mtx_free_len, xr_hdr->type);
	rpbuf_dump("data: ", (void *)xr_hdr, ((XR_HDR_SZ + xr_hdr->len) > 32 ? 32 : (XR_HDR_SZ + xr_hdr->len)));

	if ((op_code == XR_TXBUS_OP_FORCE_TX) || (rpbuf_dev->buffer_mtx_agg_count >=
		rpbuf_dev->buffer_mtx_agg_max)) {
		ret = rpbuf_send(dev, rpbuf_dev);
		if (ret)
			return ret;
	}

	return ret;
}

static int xradio_rpbuf_txdata(struct device *dev, struct sk_buff *skb, enum xradio_txbus_op op_code)
{
	return xradio_rpbuf_write(dev, skb->data, skb->len, op_code);
}

static int xradio_rpbuf_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *tmp_np;
	struct rpbuf_controller *controller;
	struct rpbuf_buffer *buffer_mrx, *buffer_mtx;
	struct xradio_rpbuf_dev *rpbuf_dev;

	/*
	 * Optional:
	 *   We can define our own "memory-region" in DTS to use specific reserved
	 *   memory in our own alloc_payload_memory/free_payload_memory operations.
	 */
	tmp_np = of_parse_phandle(np, "memory-region", 0);
	if (tmp_np) {
		ret = of_reserved_mem_device_init(dev);
		if (ret < 0) {
			rpbuf_printk(XRADIO_DBG_ERROR, "failed to get reserved memory (ret: %d)\n", ret);
			goto err_out;
		}
	}

	rpbuf_dev = devm_kzalloc(dev, sizeof(struct xradio_rpbuf_dev), GFP_KERNEL);
	if (!rpbuf_dev) {
		ret = -ENOMEM;
		rpbuf_printk(XRADIO_DBG_ERROR, "no mem, size %d\n", sizeof(struct xradio_rpbuf_dev));
		goto err_out;
	}

	controller = rpbuf_get_controller_by_of_node(np, 0);
	if (!controller) {
		rpbuf_printk(XRADIO_DBG_ERROR, "cannot get rpbuf controller\n");
		ret = -ENOENT;
		goto err_free_inst;
	}
	rpbuf_dev->controller = controller;

	strncpy(rpbuf_dev->name_mrx, RPBUF_BUFFER_NAME_XRADIO_RX, RPBUF_NAME_SIZE);
	rpbuf_dev->len_mrx = XRADIO_MAX_RX_RPBUF_SIZE;
	strncpy(rpbuf_dev->name_mtx, RPBUF_BUFFER_NAME_XRADIO_TX, RPBUF_NAME_SIZE);
	rpbuf_dev->len_mtx = XRADIO_MAX_TX_RPBUF_SIZE;

	buffer_mrx = rpbuf_alloc_buffer(controller, rpbuf_dev->name_mrx, rpbuf_dev->len_mrx, &xradio_rpbuf_ops,
								&xradio_rpbuf_mrx_cbs, (void *)dev);
	if (!buffer_mrx) {
		rpbuf_printk(XRADIO_DBG_ERROR, "alloc %s failed\n", RPBUF_BUFFER_NAME_XRADIO_RX);
		ret = -ENOENT;
		goto err_free_inst;
	}
	rpbuf_dev->buffer_mrx = buffer_mrx;
	rpbuf_buffer_set_sync(buffer_mrx, true);
	rpbuf_printk(XRADIO_DBG_MSG, "xrlink %s rpbuf sucess!\n", RPBUF_BUFFER_NAME_XRADIO_RX);

	buffer_mtx = rpbuf_alloc_buffer(controller, rpbuf_dev->name_mtx, rpbuf_dev->len_mtx, &xradio_rpbuf_ops,
								&xradio_rpbuf_mtx_cbs, (void *)dev);
	if (!buffer_mtx) {
		rpbuf_printk(XRADIO_DBG_ERROR, "alloc %s failed\n", RPBUF_BUFFER_NAME_XRADIO_TX);
		ret = -ENOENT;
		goto err_free_rx_buf;
	}
	rpbuf_dev->buffer_mtx = buffer_mtx;
	rpbuf_buffer_set_sync(buffer_mtx, true);
	rpbuf_printk(XRADIO_DBG_MSG, "xrlink %s rpbuf sucess!\n", RPBUF_BUFFER_NAME_XRADIO_TX);

	rpbuf_txbuf_reset(rpbuf_dev);
	rpbuf_dev->buffer_mtx_agg_max = RPBUF_BUFFER_XRADIO_TX_AGG_MAX;
	rpbuf_dev->dev = dev;
	rpbuf_dev->pdev = pdev;
	xr_rpbuf_dev = rpbuf_dev;
	dev_set_drvdata(dev, rpbuf_dev);

	rpbuf_printk(XRADIO_DBG_MSG, "xrlink rpbuf up!\n");
	up(&rpbuf_up_sem);

	return ret;
err_free_rx_buf:
	rpbuf_free_buffer(buffer_mrx);
err_free_inst:
	devm_kfree(dev, rpbuf_dev);
err_out:
	return ret;
}

static int xradio_rpbuf_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct xradio_rpbuf_dev *rpbuf_dev = dev_get_drvdata(dev);
	struct rpbuf_buffer *buffer_mrx = rpbuf_dev->buffer_mrx;
	struct rpbuf_buffer *buffer_mtx = rpbuf_dev->buffer_mtx;

	if (!xr_rpbuf_dev) {
		rpbuf_printk(XRADIO_DBG_ERROR, "xr_rpbuf_dev is NULL\n");
		return -1;
	}
	if (buffer_mrx)
		rpbuf_free_buffer(buffer_mrx);
	if (buffer_mtx)
		rpbuf_free_buffer(buffer_mtx);

	devm_kfree(dev, rpbuf_dev);
	xr_rpbuf_dev = NULL;
	rpbuf_printk(XRADIO_DBG_ALWY, "xradio master rpbuf client driver is removed\n");

	return 0;
}

static const struct of_device_id xradio_rpbuf_ids[] = {
	{ .compatible = "allwinner,rpbuf-xradio" },
	{}
};

static struct platform_driver xradio_rpbuf_driver = {
	.probe = xradio_rpbuf_probe,
	.remove = xradio_rpbuf_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "xradio-rpbuf",
		.of_match_table = xradio_rpbuf_ids,
	},
};

int xradio_rpbuf_init(struct xradio_bus *bus_if, struct xradio_hw *hw_priv)
{
	int ret = 0;

	sema_init(&rpbuf_up_sem, 0);
	ret = platform_driver_register(&xradio_rpbuf_driver);
	if (ret) {
		rpbuf_printk(XRADIO_DBG_ERROR, "platform_driver_register fail %d\n", ret);
		return ret;
	}

	if (down_timeout(&rpbuf_up_sem, msecs_to_jiffies(10000)) != 0) {
		platform_driver_unregister(&xradio_rpbuf_driver);
		rpbuf_printk(XRADIO_DBG_ERROR, "probe timeout!\n");
		return -1;
	}

	bus_if->rpbuf_dev = xr_rpbuf_dev;
	bus_if->rpbuf_dev->hw_priv = hw_priv;
	bus_if->rpbuf_dev->bus_if = bus_if;
	bus_if->ops->txdata = xradio_rpbuf_txdata;
	bus_if->ops->rxdata = NULL;
	rpbuf_printk(XRADIO_DBG_ALWY, "xrlink master rpbuf creat succeed\n");

	return 0;
}

void xradio_rpbuf_deinit(struct xradio_bus *bus_if)
{
	bus_if->ops->txdata = NULL;
	bus_if->ops->rxdata = NULL;
	platform_driver_unregister(&xradio_rpbuf_driver);
	bus_if->rpbuf_dev = NULL;
	rpbuf_printk(XRADIO_DBG_ALWY, "xrlink master rpbuf deinit\n");
}
