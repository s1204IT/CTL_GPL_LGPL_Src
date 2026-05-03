/*
 * v821_wlan/queue.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Transmit Queue implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/list.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

#include "os_dep/os_intf.h"
#include "queue.h"
#include "debug.h"

bool xradio_queue_is_empty(struct xradio_queue_t *queue)
{
	return list_empty(&queue->queue);
}

int xradio_queue_get_queued_num(struct xradio_queue_t *queue)
{
	return queue->num_queued;
}

// return > 0: push fail; return < 0: push ok but nearly full; return == 0: push sucess
int xradio_queue_put(struct xradio_queue_t *queue, struct sk_buff *skb, u8 check_q_full)
{
	int ret = 1;

	xradio_k_spin_lock_bh(&queue->lock);

	ret = list_empty(&queue->free_pool);
	if (!ret) {
		struct xradio_queue_item_t *item;

		item = list_first_entry(&queue->free_pool, struct xradio_queue_item_t, head);
		if (!item) {
			txrx_printk(XRADIO_DBG_ERROR, "push queue, find first entry error\n");
			ret = 2;
			goto end;
		}

		item->skb = skb;
		list_move_tail(&item->head, &queue->queue);
		++queue->num_queued;

		/* nearly full if the x% watermark is reached */
		if ((queue->num_queued > (queue->capacity * check_q_full / 100))) {
			txrx_printk(XRADIO_DBG_MSG, "%s nearly full, queued:%d, cap:%d\n",
					queue->name, queue->num_queued, queue->capacity);
			xradio_k_spin_unlock_bh(&queue->lock);
			return -ENOMEM;
		}
		ret = 0;
	} else {
		ret = 1;
		txrx_printk(XRADIO_DBG_NIY, "%s push queue fail, free_pool empty\n", queue->name);
	}

end:
	xradio_k_spin_unlock_bh(&queue->lock);

	return ret;
}

struct sk_buff *xradio_queue_get(struct xradio_queue_t *queue)
{
	struct xradio_queue_item_t *item = NULL;
	struct sk_buff *skb = NULL;

	xradio_k_spin_lock_bh(&queue->lock);
	if (!list_empty(&queue->queue)) {
		item = list_first_entry(&queue->queue, struct xradio_queue_item_t, head);
		if (!item) {
			txrx_printk(XRADIO_DBG_ERROR, "%s pop queue,find fist entry error\n", queue->name);
			goto end;
		}

		skb = item->skb;
		item->skb = NULL;
		list_move_tail(&item->head, &queue->free_pool);
		--queue->num_queued;
	}
end:
	xradio_k_spin_unlock_bh(&queue->lock);
	return skb;
}

int xradio_queue_clear(struct xradio_queue_t *queue)
{
	struct xradio_queue_item_t *item = NULL;
	struct sk_buff *skb = NULL;
	int ret = 0;

	xradio_k_spin_lock_bh(&queue->lock);
	while (!list_empty(&queue->queue)) {
		item = list_first_entry(&queue->queue, struct xradio_queue_item_t, head);
		if (!item) {
			txrx_printk(XRADIO_DBG_ERROR, "%s pop queue error\n", queue->name);
			ret = -1;
			goto end;
		}

		skb = item->skb;
		item->skb = NULL;
		list_move_tail(&item->head, &queue->free_pool);
		--queue->num_queued;
		if (queue->free_skb)
			queue->free_skb(item->skb, __func__);
		else
			txrx_printk(XRADIO_DBG_WARN, "%s skb %p not free\n", queue->name, skb);
	}

end:
	xradio_k_spin_unlock_bh(&queue->lock);
	return ret;
}


int xradio_queue_init(struct xradio_queue_t *queue, size_t capacity, char *name, xr_free_skb_t cb)
{
	int i = 0;

	memset(queue, 0, sizeof(struct xradio_queue_t));
	queue->capacity = capacity;
	queue->free_skb = cb;
	strncpy(queue->name, name, XR_QUEUE_NAME_SIZE);

	INIT_LIST_HEAD(&queue->queue);
	INIT_LIST_HEAD(&queue->free_pool);
	xradio_k_spin_lock_init(&queue->lock);

	queue->pool = xradio_k_zmalloc(sizeof(struct xradio_queue_item_t) * capacity);
	if (!queue->pool)
		return -ENOMEM;

	for (i = 0; i < capacity; ++i)
		list_add_tail(&queue->pool[i].head, &queue->free_pool);

	return 0;
}

void xradio_queue_deinit(struct xradio_queue_t *queue)
{
	xradio_queue_clear(queue);
	INIT_LIST_HEAD(&queue->free_pool);
	xradio_k_free(queue->pool);
	queue->pool = NULL;
	queue->capacity = 0;
	queue->free_skb = NULL;
}
