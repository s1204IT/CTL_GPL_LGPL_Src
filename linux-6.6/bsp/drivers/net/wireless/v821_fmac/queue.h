/*
 * v821_wlan/queue.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Transmit Queue APIs for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XRADIO_QUEUE_H__
#define __XRADIO_QUEUE_H__

#include "os_dep/os_intf.h"

struct xradio_queue_item_t {
	struct list_head head;
	struct sk_buff *skb;
};

typedef void (*xr_free_skb_t)(struct sk_buff *skb, const char *func);
#define XR_QUEUE_NAME_SIZE    32

struct xradio_queue_t {
	char name[XR_QUEUE_NAME_SIZE];
	size_t capacity;
	size_t num_queued;
	struct xradio_queue_item_t *pool;
	struct list_head queue;
	struct list_head free_pool;
	bool overfull;
	xr_spinlock_t lock;
	xr_free_skb_t free_skb;
};

bool xradio_queue_is_empty(struct xradio_queue_t *queue);
int xradio_queue_get_queued_num(struct xradio_queue_t *queue);
int xradio_queue_put(struct xradio_queue_t *queue, struct sk_buff *skb, u8 check_q_full);
struct sk_buff *xradio_queue_get(struct xradio_queue_t *queue);
int xradio_queue_clear(struct xradio_queue_t *queue);
int xradio_queue_init(struct xradio_queue_t *queue, size_t capacity, char *name, xr_free_skb_t cb);
void xradio_queue_deinit(struct xradio_queue_t *queue);

static inline int xradio_data_tx_queue_put(struct xradio_queue_t *queue, struct sk_buff *skb)
{
	return xradio_queue_put(queue, skb, 80);
}

static inline int xradio_data_rx_queue_put(struct xradio_queue_t *queue, struct sk_buff *skb)
{
	return xradio_queue_put(queue, skb, 100);
}

static inline int xradio_msg_queue_put(struct xradio_queue_t *queue, struct sk_buff *skb)
{
	return xradio_queue_put(queue, skb, 100);
}

#endif
