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

#include <linux/version.h>

#include "os_dep/os_intf.h"
#include "os_dep/os_net.h"
#include "xradio.h"
#include "xradio_platform.h"
#include "queue.h"
#include "debug.h"
#include "checksum.h"
#include "cmd_proto.h"
#include "up_cmd.h"
#include "low_cmd.h"
#include "xradio_txrx.h"
#include "xradio_firmware.h"
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include "uapi/linux/sched/types.h"
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include "linux/sched/types.h"
#else
#include "linux/sched/rt.h"
#endif

#define txrx_dump(pref, width, buf, len)     //data_hex_dump(pref, width, buf, len)
#define txrx_err_dump(pref, width, buf, len) data_hex_dump(pref, width, buf, len)
#define THREAD_SHOULD_STOP                   xradio_k_thread_should_stop
#define XR_TX_DATA_RETRY_MAX_CNT             1000
#define XR_TX_CMD_EVTENT_TO_MS               (10 * HZ)

u16 txparse_flags;
u16 rxparse_flags;

static u8 xradio_data_tx_thread_prio;
static u8 xradio_data_rx_thread_prio = 1;
static u8 xradio_rx_com_thread_prio = 1;

#ifdef XRADIO_RPBUF_PERF_TEST
int xradio_rp_test_rx_process(struct sk_buff *skb);
#endif

void xradio_wake_up_data_tx_work(struct xradio_hw *xradio_hw)
{
	txrx_printk(XRADIO_DBG_TRC, "xradio_wake_up_data_tx_work\n");
	wake_up_interruptible(&xradio_hw->data_tx_wq);
}

void xradio_wake_up_data_rx_work(struct xradio_hw *xradio_hw)
{
	txrx_printk(XRADIO_DBG_TRC, "xradio_wake_up_data_rx_work\n");
	wake_up_interruptible(&xradio_hw->data_rx_wq);
}

void xradio_wake_up_rx_com_work(struct xradio_hw *xradio_hw)
{
	txrx_printk(XRADIO_DBG_TRC, "xradio_wake_up_rx_com_work\n");
	wake_up_interruptible(&xradio_hw->rx_com_wq);
}

void xradio_wake_up_txcmd_rsp_work(struct xradio_hw *xradio_hw)
{
	txrx_printk(XRADIO_DBG_TRC, "xradio_wake_up_txcmd_rsp_work\n");
	wake_up_interruptible(&xradio_hw->tx_cmd_rsp_wq);
}

int xradio_tx_cmd_ack(struct xradio_hw *xradio_hw, void *data, u16 data_len)
{
	struct xradio_hdr *hdr = NULL;
	int ret = 0;
	int max_retry = 10;

	hdr = (struct xradio_hdr *)xradio_k_zmalloc(XR_HDR_SZ + data_len);
	if (!hdr)
		return -ENOMEM;

	if (data)
		memcpy(hdr->payload, data, data_len);

	hdr->type = XR_TYPE_CMD_RSP;
	hdr->len = data_len;
#ifdef XRADIO_HDR_CKSUM
	hdr->checksum = xradio_k_cpu_to_le16(xradio_crc_16(hdr->payload, hdr->len));
#endif
	while (max_retry-- && xradio_hw->txrx_enable) {
		ret = xradio_bus_txmsg(xradio_platform_get_bus(xradio_hw->plat), (void *)hdr, hdr->len + XR_HDR_SZ);
		if (ret) {
			txrx_printk(XRADIO_DBG_ERROR, "msg send fail %d retry %d.\n", ret, max_retry);
			msleep(1);
		} else {
			break;
		}
	}
	if (ret)
		txrx_printk(XRADIO_DBG_ERROR, "cmd ack send fail %d.\n", ret);
	else
		txrx_printk(XRADIO_DBG_MSG, "cmd ack send sucess.\n");

	xradio_k_free(hdr);

	return ret;
}

static int xradio_tx_com_send(struct xradio_hw *xradio_hw, struct xradio_hdr *hdr, void *cfm, u16 cfm_len)
{
	bool need_rsp;
	int ret = 0;
	int tmo;
	long status;
	struct sk_buff *skb = NULL;

	if (hdr->type == XR_TYPE_CMD)
		need_rsp = true;
	else
		need_rsp = false;

	txrx_printk(XRADIO_DBG_MSG, "cmd len %d total %d.\n", hdr->len, hdr->len + XR_HDR_SZ);

	ret = xradio_bus_txmsg(xradio_platform_get_bus(xradio_hw->plat), (void *)hdr, hdr->len + XR_HDR_SZ);
	if (ret) {
		txrx_printk(XRADIO_DBG_ERROR, "msg send fail %d.\n", ret);
		return ret;
	}
	if (!need_rsp)
		return ret;

	tmo = XR_TX_CMD_EVTENT_TO_MS;
	status = wait_event_interruptible_timeout(xradio_hw->tx_cmd_rsp_wq,
											 !xradio_queue_is_empty(&xradio_hw->queue_cmd_rsp),
											 msecs_to_jiffies(tmo));
	if (status == 0) {
		txrx_printk(XRADIO_DBG_ERROR, "cmd rsp timeout, timeout %d ms.\n", tmo);
		return -1;
	} else if (status < 0) {
		txrx_printk(XRADIO_DBG_ERROR, "wait_event error %ld.\n", status);
		return -1;
	}
	ret = -1;
	skb = xradio_queue_get(&xradio_hw->queue_cmd_rsp);
	if (skb) {
		struct xradio_hdr *rsp;
#ifdef XRADIO_HDR_CKSUM
		u16 checksum = 0, c_checksum = 0;
#endif

		rsp = (void *)skb->data;
#ifdef XRADIO_HDR_CKSUM
		checksum = le16_to_cpu(hdr->checksum);
		c_checksum = le16_to_cpu(xradio_crc_16((u8 *)hdr + XR_HDR_SZ, hdr->len));
		if (checksum != c_checksum) {
			txrx_printk(XRADIO_DBG_ERROR, "checksum failed S:%x M:%x type:%d seq:%d\n",
						checksum, c_checksum, hdr->type, hdr->seq);
			txrx_err_dump("rx:", 32, skb->data, skb->len);
			xradio_free_skb(skb, __func__);
			return -1;
		}
#endif

		if (rsp->type == XR_TYPE_CMD_RSP) {
			if (cfm) {
				if (cfm_len == rsp->len) {
					memcpy(cfm, rsp->payload, cfm_len);
					ret = 0;
				} else {
					txrx_printk(XRADIO_DBG_ERROR, "rsp len %d error, req:%d.\n", rsp->len, cfm_len);
					txrx_err_dump("rsp:", 16, (u8 *)rsp, rsp->len > 64 ? 64 : rsp->len);
				}
			} else {
				ret = 0;
			}
		} else {
			txrx_printk(XRADIO_DBG_ERROR, "rsp type err type:%d len:%d.\n", rsp->type, rsp->len);
		}
		txrx_printk(XRADIO_DBG_MSG, "cmd rsp type %d.\n", rsp->type);
		xradio_free_skb(skb, __func__);
	} else {
		txrx_printk(XRADIO_DBG_ERROR, "cmd rsp recive err from queue.\n");
	}

	return ret;
}

int xradio_tx_com_process(struct xradio_hdr *xr_hdr, void *cfm, u16 cfm_len)
{
	int ret = 0;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(xradio_get_platform());

	if (!xradio_hw->txrx_enable) {
		txrx_printk(XRADIO_DBG_ERROR, "txrx thread not ready.\n");
		return -EPERM;
	}

	txrx_printk(XRADIO_DBG_MSG, ">>> req cmd lock, type %d\n", xr_hdr->type);
	xradio_k_mutex_lock(&xradio_hw->cmd_tx_mutex);
	xr_hdr->seq = xradio_hw->com_tx_seq;
#ifdef XRADIO_HDR_CKSUM
	xr_hdr->checksum = xradio_k_cpu_to_le16(xradio_crc_16(xr_hdr->payload, xr_hdr->len));
#endif
	ret = xradio_tx_com_send(xradio_hw, xr_hdr, cfm, cfm_len);
	if (ret) {
		txrx_printk(XRADIO_DBG_ERROR, "send %s fail %d, type:%d len:%d seq:%d.\n",
					xr_hdr->type == XR_TYPE_CMD ? "CMD" : "EVENT", ret, xr_hdr->type,
					xr_hdr->len, xradio_hw->com_tx_seq);
	}
	xradio_hw->com_tx_seq++;
	xradio_k_mutex_unlock(&xradio_hw->cmd_tx_mutex);

	return ret;
}

int xradio_tx_data_process(struct xradio_plat *priv, struct sk_buff *skb, u8 if_type)
{
	u16 len = 0;
	int pad_len = 0;
	struct sk_buff *new_skb = NULL;
	struct xradio_hdr *hdr = NULL;
	u8 *pos = NULL;
	int ret = -1;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	if (!xradio_hw->txrx_enable) {
		txrx_printk(XRADIO_DBG_ERROR, "txrx thread not ready.\n");
		return -1;
	}

	if (txparse_flags)
		xradio_parse_frame(skb->data, 1, txparse_flags, skb->len);

	pad_len = XR_HDR_SZ;
	len = skb->len;
	if ((skb_headroom(skb) < pad_len)
		/*|| !IS_ALIGNED((unsigned long)skb->data, SKB_DATA_ADDR_ALIGNMENT)*/
		) {
		txrx_printk(XRADIO_DBG_WARN, "reallocate skb\n");
		if (skb_linearize(skb)) {
			dev_kfree_skb(skb);
			return -1;
		}
		new_skb = xradio_alloc_skb(skb->len + pad_len, __func__);
		if (!new_skb) {
			txrx_printk(XRADIO_DBG_ERROR, "failed to allocate skb\n");
			dev_kfree_skb(skb);
			return -1;
		}

		pos = new_skb->data;
		pos += pad_len;
		skb_copy_from_linear_data(skb, pos, skb->len);
		skb_put(new_skb, skb->len + pad_len);
		dev_kfree_skb_any(skb);
		skb = new_skb;
	} else {
		skb_push(skb, pad_len);
	}

	hdr = (struct xradio_hdr *)skb->data;
	memset(hdr, 0, pad_len);
	hdr->len = len;

#ifdef STA_AP_COEXIST
	if (XR_AP_IF == if_type) {
		hdr->type = XR_TYPE_DATA_AP;
		txrx_printk(XRADIO_DBG_TRC, "%s mode ap\n", __func__);
	} else
		hdr->type = XR_TYPE_DATA;
#else
	hdr->type = XR_TYPE_DATA;
#endif

	hdr->seq = xradio_hw->data_tx_seq;
#ifdef XRADIO_HDR_CKSUM
	hdr->checksum = xradio_k_cpu_to_le16(xradio_crc_16((u8 *)hdr + pad_len, len));
#endif
	xradio_hw->data_tx_seq++;

	txrx_printk(XRADIO_DBG_TRC, "type:%2.2X, seq number:%d, len:%d\n", hdr->type, hdr->seq, len);

	ret = xradio_data_tx_queue_put(&xradio_hw->queue_tx_data, skb);
	if (ret) {
		if (ret > 0) {
			kfree_skb(skb);
			txrx_printk(XRADIO_DBG_NIY, "tx data queue push fail, drop data:%x seq:%d!\n", (u32)skb, hdr->seq);
		}
		if (!xradio_k_atomic_read(&xradio_hw->tx_data_pause_master)) {
			xradio_k_atomic_set(&xradio_hw->tx_data_pause_master, 1);
			xradio_net_tx_pause(priv);
			txrx_printk(XRADIO_DBG_MSG, "tx data queue will full, tx data pause:%d\n",
						xradio_queue_get_queued_num(&xradio_hw->queue_tx_data));
		}
	}
	if (ret <= 0) {
		xradio_wake_up_data_tx_work(xradio_platform_get_hw(priv));
	}

	return ret;
}

static int xradio_rx_data_process(struct xradio_plat *priv, struct sk_buff *skb, u8 type)
{
	//skb_trim(skb, len); //ensure it?
	if (rxparse_flags)
		xradio_parse_frame(skb->data, 0, rxparse_flags, skb->len);
#ifdef XRADIO_RPBUF_PERF_TEST
	if (!xradio_rp_test_rx_process(skb))
		return 0;
#endif

#ifdef STA_AP_COEXIST
	if (type == XR_TYPE_DATA_AP) {
		xradio_net_data_input_ap(priv, skb);
	} else {
		xradio_net_data_input(priv, skb);
	}
#else
	xradio_net_data_input(priv, skb);
#endif
	return 0;
}

static int xradio_rx_cmd_process(struct xradio_plat *priv, struct sk_buff *skb,
				u8 type, struct sk_buff **rsp_skb)
{
	int ret = 0;
	struct cmd_payload *cmd = NULL;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	if (!priv || !skb)
		return -EFAULT;

	cmd = (struct cmd_payload *)skb->data;
	txrx_printk(XRADIO_DBG_MSG, "cmd->type=%d addr:%x\n", cmd->type, (u32)cmd);
	if (cmd->type == XR_WIFI_DEV_RX_PAUSE) {
		if (!xradio_k_atomic_read(&xradio_hw->tx_data_pause_slave)) {
			xradio_net_tx_pause(priv);
			xradio_k_atomic_set(&xradio_hw->tx_data_pause_slave, 1);
			txrx_printk(XRADIO_DBG_MSG, "config dev tx pause\n");
		}
		goto end;
	} else if (cmd->type == XR_WIFI_DEV_RX_RESUME) {
		if (xradio_k_atomic_read(&xradio_hw->tx_data_pause_slave)) {
			xradio_k_atomic_set(&xradio_hw->tx_data_pause_slave, 0);
			xradio_net_tx_resume(priv);
			xradio_wake_up_data_tx_work(xradio_platform_get_hw(priv));
			txrx_printk(XRADIO_DBG_MSG, "config dev tx resume\n");
		}
		goto end;
	} else if (cmd->type == XR_WIFI_DEV_REQUEST_FW_REQ) {
		struct param_wlan_bin *bin = (void *)cmd->param;
		txrx_printk(XRADIO_DBG_MSG, "request_firmware:%d,%d\n", bin->type, bin->len);
		ret = xradio_request_firmware(xradio_hw, bin->type, bin->len);
		goto end;
	} else if (cmd->type == XR_WIFI_DEV_RELEASE_FW_REQ) {
		struct param_wlan_bin *bin = (void *)cmd->param;
		txrx_printk(XRADIO_DBG_MSG, "release_firmware\n");
		ret = xradio_release_firmware(xradio_hw, bin->type);
		goto end;
	} else if (cmd->type == XR_WIFI_DEV_ETH_STATE) {
		txrx_printk(XRADIO_DBG_MSG, "notify_eth_event\n");
		xradio_notify_eth_state(xradio_hw, cmd->param);
		goto end;
	}

	if (cmd->type >= XR_WIFI_DEV_HAND_WAY_RES && cmd->type < XR_WIFI_DEV_KERNEL_MAX)
		ret = xradio_rx_low_cmd_push(skb->data, skb->len);
	else
		ret = xradio_rx_up_cmd_push(skb->data, skb->len);
end:
	return ret;
}

// return > 0: need send to next process; < 0: pre process err; = 0: pre process sucess
int xradio_rx_data_pre_process(struct xradio_hw *xradio_hw, struct sk_buff *skb, void *out, u16 out_len)
{
	int ret = 1;
	struct xradio_hdr *hdr = NULL;
	struct cmd_payload *payload;
#ifdef XRADIO_HDR_CKSUM
	u16 checksum = 0, c_checksum = 0;
#endif

	hdr = (struct xradio_hdr *)skb->data;

#ifdef XRADIO_HDR_CKSUM
	checksum = le16_to_cpu(hdr->checksum);
	c_checksum = le16_to_cpu(xradio_crc_16((u8 *)hdr + XR_HDR_SZ, hdr->len));
	if (checksum != c_checksum) {
		txrx_printk(XRADIO_DBG_ERROR, "checksum failed S:%x M:%x type:%d seq:%d\n",
					checksum, c_checksum, hdr->type, hdr->seq);
		txrx_err_dump("rx:", 32, skb->data, skb->len);
		xradio_free_skb(skb, __func__);
		return -1;
	}
#endif

	payload = (void *)hdr->payload;

	if ((hdr->type == XR_TYPE_FW_BIN) && (payload->type == XR_WIFI_DEV_UPDATE_FW_REQ)) {
		txrx_printk(XRADIO_DBG_MSG, "xr type:%d cmd %d\n", hdr->type, payload->type);
		ret = xradio_update_firmware_bin(xradio_hw, hdr, out, out_len);
		if (ret > 0)
			ret = -ret;
		kfree_skb(skb);
	}

	return ret;
}

int xradio_rx_process(struct xradio_hw *xradio_hw, struct sk_buff *skb)
{
	struct xradio_hdr *hdr = NULL;
	u8 type_id = 0;
	int ret;
#ifdef XRADIO_HDR_CKSUM
	u16 checksum = 0, c_checksum = 0;
#endif
	struct xradio_plat *priv = xradio_hw->plat;

	if (!priv || !skb) {
		txrx_printk(XRADIO_DBG_ERROR, "err! xradio_hw:%p plat:%p skb:%p\n", xradio_hw, priv, skb);
		if (skb)
			xradio_free_skb(skb, __func__);
		return -EINVAL;
	}
	hdr = (struct xradio_hdr *)skb->data;
#ifdef XRADIO_HDR_CKSUM
	checksum = le16_to_cpu(hdr->checksum);
	c_checksum = le16_to_cpu(xradio_crc_16((u8 *)hdr + XR_HDR_SZ, hdr->len));
	if (checksum != c_checksum) {
		txrx_printk(XRADIO_DBG_ERROR, "checksum failed S:%x M:%x type:%d seq:%d\n",
					checksum, c_checksum, hdr->type, hdr->seq);
		txrx_err_dump("rx:", 32, skb->data, skb->len);
		xradio_free_skb(skb, __func__);
		return 1;
	}
#endif
	type_id = hdr->type;
	skb_pull(skb, XR_HDR_SZ);

	if ((type_id == XR_TYPE_CMD) || (type_id == XR_TYPE_EVENT) || (type_id == XR_TYPE_RAW_CMD_DATA)) {
		struct sk_buff *rsp_skb = NULL;

		txrx_printk(XRADIO_DBG_TRC, "cmd type_id=%d seq:%d\n", type_id, hdr->seq);
		ret = xradio_rx_cmd_process(priv, skb, type_id, &rsp_skb);
		if ((type_id == XR_TYPE_CMD) || (type_id == XR_TYPE_FW_BIN)) {
			if (rsp_skb) {
				xradio_tx_cmd_ack(xradio_hw, rsp_skb->data, rsp_skb->len);
				xradio_free_skb(rsp_skb, __func__);
			} else {
				xradio_tx_cmd_ack(xradio_hw, &ret, sizeof(int));
			}
		}
		xradio_free_skb(skb, __func__);
	} else {
		txrx_printk(XRADIO_DBG_TRC, "data type_id=%d seq:%d\n", type_id, hdr->seq);
		xradio_rx_data_process(priv, skb, type_id);
	}

	return 0;
}

static void xradio_check_tx_resume(struct xradio_plat *priv)
{
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	if (xradio_queue_get_queued_num(&xradio_hw->queue_tx_data) < (xradio_hw->queue_tx_data.capacity * 2 / 5)) {
		if (xradio_k_atomic_read(&xradio_hw->tx_data_pause_master)) {
			xradio_k_atomic_set(&xradio_hw->tx_data_pause_master, 0);
			txrx_printk(XRADIO_DBG_MSG, "tx data resume:%d, q:%d\n",
					xradio_k_atomic_read(&xradio_hw->tx_data_pause_master), xradio_queue_get_queued_num(&xradio_hw->queue_tx_data));
			xradio_net_tx_resume(priv);
		}
	}
}

static int xradio_data_rx_thread(void *data)
{
	struct xradio_plat *priv = (struct xradio_plat *)data;
	int rx = 0, term = 0;
	int status = 0;
	int rx_status = 0;
	struct sk_buff *rx_skb = NULL;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	if (xradio_data_rx_thread_prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		sched_set_fifo_low(current);
#else
		struct sched_param param;
		param.sched_priority = (xradio_data_rx_thread_prio < MAX_RT_PRIO) ? xradio_data_rx_thread_prio : (MAX_RT_PRIO - 1);
		sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}

	while (1) {
		status = wait_event_interruptible(xradio_hw->data_rx_wq,
											({rx = !xradio_queue_is_empty(&xradio_hw->queue_rx_data);
												term = THREAD_SHOULD_STOP(&xradio_hw->data_rx_thread);
												(rx || term);
											}));
		if (term) {
			txrx_printk(XRADIO_DBG_ALWY, "xradio data rx thread exit!\n");
			break;
		}
		txrx_printk(XRADIO_DBG_TRC, "data rx thread:status:%d rx:%d\n", status, rx);
		while (!xradio_queue_is_empty(&xradio_hw->queue_rx_data) &&
			!THREAD_SHOULD_STOP(&xradio_hw->data_rx_thread)) {
			rx_skb = xradio_queue_get(&xradio_hw->queue_rx_data);
			if (rx_skb) {
				rx_status = xradio_rx_process(xradio_hw, rx_skb);
				if (rx_status) {
					txrx_printk(XRADIO_DBG_ERROR, "hwio exception, rx_status=%d\n", rx_status);
				}
			} else {
				txrx_printk(XRADIO_DBG_ERROR, "Asynchronous issue, empty:%d queued:%d\n",
							xradio_queue_is_empty(&xradio_hw->queue_rx_data),
							xradio_queue_get_queued_num(&xradio_hw->queue_rx_data));
				msleep(1);
			}
		}
	}
	xradio_k_thread_exit(&xradio_hw->data_rx_thread);
	return 0;
}

static __inline__ bool xradio_check_tx_retry(int status)
{
	if ((status == XR_TXD_ST_NO_MEM) || (status == XR_TXD_ST_NO_QUEUE) ||
		(status == XR_TXD_ST_BUS_TX_FAIL))
		return true;
	else
		return false;
}

static int xradio_data_tx_thread(void *data)
{
	struct xradio_plat *priv = (struct xradio_plat *)data;
	struct sk_buff *tx_skb = NULL;
	int tx = 0, term = 0;
	int status = 0;
	int tx_status = -1;
	int sleep_time = 1;
#if 0
	int wait_dev_time = 1, wait_dev_cnt;
#endif
	bool txq_empty = false;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	if (xradio_data_tx_thread_prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		sched_set_fifo_low(current);
#else
		struct sched_param param;
		param.sched_priority = (xradio_data_tx_thread_prio < MAX_RT_PRIO) ? xradio_data_tx_thread_prio : (MAX_RT_PRIO - 1);
		sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}

	while (1) {
		txrx_printk(XRADIO_DBG_TRC, "xradio data tx thread running\n");
		status = wait_event_interruptible(xradio_hw->data_tx_wq,
											({tx = (!xradio_queue_is_empty(&xradio_hw->queue_tx_data) &&
												!xradio_k_atomic_read(&xradio_hw->tx_data_pause_slave));
												term = THREAD_SHOULD_STOP(&xradio_hw->data_tx_thread);
												(tx || term);
											}));
		if (term) {
			txrx_printk(XRADIO_DBG_ERROR, "xradio data tx thread exit!\n");
			break;
		}
		txrx_printk(XRADIO_DBG_TRC, "status:%d tx:%d\n", status, tx);
		if (tx) {
			txq_empty = xradio_queue_is_empty(&xradio_hw->queue_tx_data);
			while (!txq_empty && !xradio_k_atomic_read(&xradio_hw->tx_data_pause_slave)) {
				int max_retry = XR_TX_DATA_RETRY_MAX_CNT, retry = 0;
				enum xradio_txbus_op op = XR_TXBUS_OP_AUTO;

#if 0
				wait_dev_cnt = 0;
#endif
				sleep_time = 1;
				tx_skb = xradio_queue_get(&xradio_hw->queue_tx_data);
				txq_empty = xradio_queue_is_empty(&xradio_hw->queue_tx_data);
				tx_status = 0;
				if (txq_empty)
					op = XR_TXBUS_OP_FORCE_TX;
				while (tx_skb && max_retry-- && !THREAD_SHOULD_STOP(&xradio_hw->data_tx_thread)) {
					txrx_printk(XRADIO_DBG_TRC, "data tx_skb len:%d\n", tx_skb->len);
					tx_status = xradio_bus_txdata(xradio_platform_get_bus(priv), tx_skb, op);
					if (tx_status == 0) {
						xradio_free_skb(tx_skb, __func__);
						xradio_check_tx_resume(priv);
						tx_skb = NULL;
						break;
					} else if (!xradio_check_tx_retry(tx_status)) {
						break;
					} else if (max_retry) {
						op = XR_TXBUS_OP_TX_RETRY;
						//XRADIO_DBG_NIY
						txrx_printk(XRADIO_DBG_NIY, "ret:%d data tx sleep:%dms retryed:%d\n",
								tx_status, sleep_time, retry);
						msleep(sleep_time);
						retry++;
						if (retry > 1)
							sleep_time = 2;
						else if (retry > 5)
							sleep_time = 3;
						else if (retry > 100)
							sleep_time = 4;
						sleep_time = max(sleep_time, 10);
					}
				}
				if (tx_status) {
					txrx_printk(XRADIO_DBG_ERROR, "txdata err:%d retryed:%d\n", tx_status, retry);
					op = XR_TXBUS_OP_RESET_BUF;
					xradio_bus_txdata(xradio_platform_get_bus(priv), tx_skb, op);
					xradio_free_skb(tx_skb, __func__);
					xradio_check_tx_resume(priv);
					break;
				}

				if (THREAD_SHOULD_STOP(&xradio_hw->data_tx_thread))
					break;
			}
		} else {
			txrx_printk(XRADIO_DBG_ERROR, "Nothing to do, status:%d tx:%d\n", status, tx);
			msleep(1);
		}
	}
	xradio_k_thread_exit(&xradio_hw->data_tx_thread);
	return 0;
}

static int xradio_rx_com_thread(void *data)
{
	struct xradio_plat *priv = (struct xradio_plat *)data;
	int rx = 0, term = 0;
	int status = 0;
	int rx_status = 0;
	struct sk_buff *rx_skb = NULL;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	if (xradio_rx_com_thread_prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		sched_set_fifo_low(current);
#else
		struct sched_param param;
		param.sched_priority = (xradio_rx_com_thread_prio < MAX_RT_PRIO) ? xradio_rx_com_thread_prio : (MAX_RT_PRIO - 1);
		sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}

	while (1) {
		status = wait_event_interruptible(xradio_hw->rx_com_wq,
											({rx = !xradio_queue_is_empty(&xradio_hw->queue_rx_com);
												term = THREAD_SHOULD_STOP(&xradio_hw->rx_com_thread);
												(rx || term);
											}));
		if (term) {
			txrx_printk(XRADIO_DBG_ALWY, "xradio rx com thread exit!\n");
			break;
		}
		txrx_printk(XRADIO_DBG_TRC, "rx com thread:status:%d rx:%d\n", status, rx);
		rx_skb = NULL;
		if (!xradio_queue_is_empty(&xradio_hw->queue_rx_com)) {
			if (!xradio_queue_is_empty(&xradio_hw->queue_rx_com))
				rx_skb = xradio_queue_get(&xradio_hw->queue_rx_com);

			if (rx_skb) {
				rx_status = xradio_rx_process(xradio_hw, rx_skb);
				if (rx_status) {
					txrx_printk(XRADIO_DBG_ERROR, "hwio exception, rx_status=%d\n", rx_status);
				}
			} else {
				txrx_printk(XRADIO_DBG_ERROR, "Asynchronous issue, queue %s empty:%d queued:%d\n",
							xradio_hw->queue_rx_com.name,
							xradio_queue_is_empty(&xradio_hw->queue_rx_com),
							xradio_queue_get_queued_num(&xradio_hw->queue_rx_com));
				msleep(1);
			}
		}
	}
	xradio_k_thread_exit(&xradio_hw->data_rx_thread);
	return 0;
}


int xradio_register_trans(struct xradio_plat *priv)
{
	int ret = 0;
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	xradio_k_atomic_set(&xradio_hw->tx_data_pause_master, 0);
	xradio_k_atomic_set(&xradio_hw->tx_data_pause_slave, 0);
	init_waitqueue_head(&xradio_hw->data_tx_wq);
	init_waitqueue_head(&xradio_hw->data_rx_wq);
	init_waitqueue_head(&xradio_hw->rx_com_wq);
	init_waitqueue_head(&xradio_hw->tx_cmd_rsp_wq);
	xradio_k_mutex_init(&xradio_hw->data_tx_mutex);
	xradio_k_mutex_init(&xradio_hw->cmd_tx_mutex);

	if (xradio_k_thread_create(&xradio_hw->data_tx_thread, "xr_data_tx", xradio_data_tx_thread, (void *)priv, 0, 4096)) {
		txrx_printk(XRADIO_DBG_ERROR, "create xr_data_tx failed\n");
		return -1;
	}

	if (xradio_k_thread_create(&xradio_hw->data_rx_thread, "xr_data_rx", xradio_data_rx_thread, (void *)priv, 0, 4096)) {
		txrx_printk(XRADIO_DBG_ERROR, "create xr_data_rx failed\n");
		ret = -1;
		goto err0;
	}

	if (xradio_k_thread_create(&xradio_hw->rx_com_thread, "xr_rx_com", xradio_rx_com_thread, (void *)priv, 0, 4096)) {
		txrx_printk(XRADIO_DBG_ERROR, "create xr_rx_com failed\n");
		ret = -1;
		goto err1;
	}

	xradio_hw->txrx_enable = 1;
	return ret;
err1:
	wake_up(&xradio_hw->data_rx_wq);
	xradio_k_thread_delete(&xradio_hw->data_rx_thread);
	xradio_hw->data_rx_thread.handle = NULL;
err0:
	wake_up(&xradio_hw->data_tx_wq);
	xradio_k_thread_delete(&xradio_hw->data_tx_thread);
	xradio_hw->data_tx_thread.handle = NULL;
	return ret;
}

void xradio_unregister_trans(struct xradio_plat *priv)
{
	struct xradio_hw *xradio_hw = xradio_platform_get_hw(priv);

	txrx_printk(XRADIO_DBG_ALWY, "txrx thread unregister.\n");
	xradio_hw->txrx_enable = 0;

	if (!IS_ERR_OR_NULL(&xradio_hw->rx_com_thread)) {
		wake_up(&xradio_hw->rx_com_wq);
		xradio_k_thread_delete(&xradio_hw->rx_com_thread);
		xradio_hw->rx_com_thread.handle = NULL;
	}

	if (!IS_ERR_OR_NULL(&xradio_hw->data_rx_thread)) {
		wake_up(&xradio_hw->data_rx_wq);
		xradio_k_thread_delete(&xradio_hw->data_rx_thread);
		xradio_hw->data_rx_thread.handle = NULL;
	}

	if (!IS_ERR_OR_NULL(&xradio_hw->data_tx_thread)) {
		wake_up(&xradio_hw->data_tx_wq);
		xradio_k_thread_delete(&xradio_hw->data_tx_thread);
		xradio_hw->data_tx_thread.handle = NULL;
	}

	wake_up(&xradio_hw->tx_cmd_rsp_wq);

	xradio_k_mutex_deinit(&xradio_hw->cmd_tx_mutex);
	xradio_k_mutex_deinit(&xradio_hw->data_tx_mutex);
}

