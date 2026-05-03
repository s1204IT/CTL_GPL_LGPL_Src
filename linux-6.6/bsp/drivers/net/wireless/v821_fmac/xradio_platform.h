/*
 * v821_wlan/xradio_platform.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * xradio common private APIs for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _XRADIO_PLAT_H_
#define _XRADIO_PLAT_H_

#include "xr_hdr.h"
#include "xradio.h"
#include "queue.h"
#ifdef XRADIO_RPBUF_SUPPORT
#include "platform/xradio_rpmsg.h"
#include "platform/xradio_rpbuf.h"
#endif

typedef enum {
	XR_STA_IF,
	XR_AP_IF,
} XR_INTERFACE_TYPE;

typedef enum {
	XR_LINK_DOWN,
	XR_LINK_UP,
} XR_LINK_STATUS;

enum xradio_bus_state {
	XR_BUS_DOWN_ST,
	XR_BUS_UP_ST
};

enum xradio_txbus_op {
	XR_TXBUS_OP_AUTO = 0,
	XR_TXBUS_OP_FORCE_TX,
	XR_TXBUS_OP_TX_RETRY,
	XR_TXBUS_OP_RESET_BUF,
	XR_TXBUS_OP_FLUSH_BUF
};

enum xradio_txdata_status {
	XR_TXD_ST_SUCESS = 0,
	XR_TXD_ST_NO_MEM = 1,
	XR_TXD_ST_NO_QUEUE = 2,
	XR_TXD_ST_DATA_ERR = 3,
	XR_TXD_ST_NO_RSP = 4,
	XR_TXD_ST_BUS_TX_FAIL = 5,
};

typedef void (*rx_ind_func)(void *arg);

struct xradio_bus_ops {
	int (*start) (struct device *dev);
	void (*stop) (struct device *dev);
	int (*txdata)(struct device *dev, struct sk_buff *skb, enum xradio_txbus_op op_code);
	int (*txmsg)(struct device *dev, u8 *msg, uint len);
	struct sk_buff *(*rxdata)(struct device *dev, uint len);
	int (*rxmsg)(struct device *dev, u8 *msg, uint len);
};

struct xradio_bus {
#ifdef XRADIO_RPBUF_SUPPORT
	struct xradio_rpmsg_dev *rpmsg_dev;
	struct xradio_rpbuf_dev *rpbuf_dev;
#endif
	struct xradio_bus_ops *ops;
	enum xradio_bus_state state;
};

static inline int xradio_bus_start(struct xradio_bus *bus)
{
	bus->state = XR_BUS_UP_ST;
	return 0;
}

static inline void xradio_bus_stop(struct xradio_bus *bus)
{
	bus->state = XR_BUS_DOWN_ST;
}

static inline int xradio_bus_txdata(struct xradio_bus *bus, struct sk_buff *skb, enum xradio_txbus_op op)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return bus->ops->txdata(bus->rpbuf_dev->dev, skb, op);
#endif
}

static inline int xradio_bus_txmsg(struct xradio_bus *bus, u8 *msg, uint len)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return bus->ops->txmsg(bus->rpmsg_dev->dev, msg, len);
#endif
}

static inline int xradio_bus_get_txmsg_max_len(struct xradio_bus *bus)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return bus->rpmsg_dev->len;
#endif
}

static inline int xradio_bus_get_txdata_max_len(struct xradio_bus *bus)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return bus->rpbuf_dev->len_mtx;
#endif
}

#if 0
static inline struct sk_buff *xradio_bus_rxdata(struct xradio_bus *bus, uint len)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return bus->ops->rxdata(bus->rpbuf_dev->dev, len);
#endif
}

static inline int xradio_bus_rxmsg(struct xradio_bus *bus, u8 *msg, uint len)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return bus->ops->rxmsg(bus->rpmsg_dev->dev, msg, len);
#endif
}
#endif

struct xradio_plat {
	struct xradio_hw hw_priv;
	struct xradio_bus *bus_if;
	bool enabled;
	u8 link_state;
	void *net_dev_priv;
#ifdef STA_AP_COEXIST
	u8 ap_link_state;
	void *ap_dev_priv;
#endif
	u32 efuse_start_addr;
};

static inline struct device *xradio_platform_get_dev(struct xradio_plat *xradio_plat)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return xradio_plat->bus_if->rpbuf_dev->dev;
#endif
}

static inline struct xradio_bus *xradio_platform_get_bus(struct xradio_plat *xradio_plat)
{
#ifdef XRADIO_RPBUF_SUPPORT
	return xradio_plat->bus_if->rpbuf_dev->bus_if;
#endif
}

static inline struct xradio_hw *xradio_platform_get_hw(struct xradio_plat *xradio_plat)
{
	return &xradio_plat->hw_priv;
}

int xradio_platform_init(struct xradio_plat **plat);
void xradio_platform_deinit(struct xradio_plat *xradio_plat);
int xradio_platform_on(struct xradio_plat *xradio_plat);
void xradio_platform_off(struct xradio_plat *xradio_plat);
struct xradio_plat *xradio_get_platform(void);

#endif
