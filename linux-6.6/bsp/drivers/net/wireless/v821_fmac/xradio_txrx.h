/*
 * v821_wlan/xradio_txrxif.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface APIs for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _XRADIO_TXRXIF_H_
#define _XRADIO_TXRXIF_H_

#include "xradio.h"

int xradio_tx_com_process(struct xradio_hdr *xr_hdr, void *cfm, u16 cfm_len);
int xradio_tx_data_process(struct xradio_plat *priv, struct sk_buff *skb, u8 if_type);

int xradio_register_trans(struct xradio_plat *priv);
void xradio_unregister_trans(struct xradio_plat *priv);
void xradio_wake_up_data_rx_work(struct xradio_hw *xradio_hw);
int xradio_rx_data_pre_process(struct xradio_hw *xradio_hw, struct sk_buff *skb, void *out, u16 out_len);
int xradio_rx_process(struct xradio_hw *xradio_hw, struct sk_buff *skb);
void xradio_wake_up_data_tx_work(struct xradio_hw *xradio_hw);
void xradio_wake_up_rx_com_work(struct xradio_hw *xradio_hw);
void xradio_wake_up_txcmd_rsp_work(struct xradio_hw *xradio_hw);

#endif
