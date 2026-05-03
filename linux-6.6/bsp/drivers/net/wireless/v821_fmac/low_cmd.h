/*
 * v821_wlan/low_cmd.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * wlan interface APIs for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __LOW_CMD_H__
#define __LOW_CMD_H__

#include "xradio_firmware.h"
#include "cmd_proto.h"

int xradio_low_cmd_init(void *priv);

void xradio_low_cmd_deinit(void *priv);

int xradio_rx_low_cmd_push(u8 *buff, u32 len);
int xraido_low_cmd_dev_hand_way(void);
int xradio_wlan_device_power(u8 enable);
int xradio_set_fmac_macaddr(uint8_t *mac_addr);
void xradio_notify_eth_state(struct xradio_hw *hw, void *param);
int xradio_get_fmac_macaddr_drv(struct cmd_para_mac_info *mac_info);
int xradio_low_cmd_set_efuse(u32 addr);

#define KEEP_ALIVE_USE_TIMER 1

/* only be used in low cmd */
enum XR_ETH_STATE {
    XR_ETH0_STOP = 0,
    XR_ETH0_START,
    XR_ETH1_STOP,
    XR_ETH1_START,
};

typedef struct {
    uint8_t event;
} xr_wifi_dev_eth_state_t;

struct cmd_wlan_power_data {
    uint8_t enable;
    uint8_t mode;
    uint8_t runing;
};

#if KEEP_ALIVE_USE_TIMER
void xradio_keep_alive_update_tx_status(int value);
#endif
typedef void (*kp_timeout_cb)(void);

int xradio_keep_alive(kp_timeout_cb cb);
#endif
