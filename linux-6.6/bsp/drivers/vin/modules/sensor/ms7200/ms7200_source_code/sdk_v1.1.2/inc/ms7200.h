/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file    ms7200.h
* @author
* @version V1.0.0
* @date    24-Nov-2020
* @brief   ms7200 SDK Library interfaces declare
* @history
*
* Copyright (c) 2009-2020, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_H__
#define __MACROSILICON_MS7200_H__

#include "ms7200_comm.h"


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
*  Function name:     ms7200_chip_connect_detect
*  Description:       get the chip connect status
*  Input parameters:  u8_chip_addr: 0 = auto check chip addr, others = set i2c slave addr
*  Output parameters: None
*  Returned value:    connect status, 0: disconnect 1: connect
***************************************************************/
BOOL ms7200_chip_connect_detect(UINT8 u8_chip_addr);

/***************************************************************
*  Function name:     ms7200_hdmirx_init
*  Description:       hdmi rx module init
*  Input parameters:  None
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_init(VOID);

/***************************************************************
*  Function name:     ms7200_hdmirx_hdcp_init
*  Description:       set hdmi rx hdcp ksv & key
*  Input parameters:  u8_rx_ksv: 5-byte
*                     u8_rx_key 280-byte
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_hdcp_init(UINT8 *u8_rx_ksv, UINT8 *u8_rx_key);

/***************************************************************
*  Function name:     ms7200_hdmirx_interrupt_pin_set
*  Description:       set which interrupt need send to int pin
*  Input parameters:  e_int_index: sel int type
*                     u32_int_flag: sel int bit
*                     b_set: enable or disable int to pin
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_interrupt_pin_set(HDMIRX_INT_INDEX_E e_int_index, UINT32 u32_int_flag, BOOL b_set);

/***************************************************************
*  Function name:     ms7200_hdmirx_interrupt_get_and_clear
*  Description:       get and clear interrupt status
*  Input parameters:  e_int_index: sel int type
*                     u32_int_flag: sel int bit
*                     b_clr: clear int status after read
*  Output parameters: None
*  Returned value:    interrupt status
***************************************************************/
UINT32 ms7200_hdmirx_interrupt_get_and_clear(HDMIRX_INT_INDEX_E e_int_index, UINT32 u32_int_flag, BOOL b_clr);

/***************************************************************
*  Function name:     ms7200_hdmirx_source_connect_detect
*  Description:       get hdmi rx 5v detect status
*  Input parameters:  None
*  Output parameters: None
*  Returned value:    connect status, 0: disconnect 1: connect
***************************************************************/
BOOL ms7200_hdmirx_source_connect_detect(VOID);

/***************************************************************
*  Function name:     ms7200_hdmirx_input_clk_get
*  Description:       get hdmi rx input clk freq
*  Input parameters:  None
*  Output parameters: None
*  Returned value:    tmds clk freq, unit: 10kHz
***************************************************************/
UINT16 ms7200_hdmirx_input_clk_get(VOID);

/***************************************************************
*  Function name:     ms7200_hdmirx_input_timing_get
*  Description:       get hdmi rx input timing
*  Input parameters:  None
*  Output parameters: ptTiming: contain h/v total, active etc.
*  Returned value:    timing valid flag
***************************************************************/
BOOL ms7200_hdmirx_input_timing_get(VIDEOTIMING_T *ptTiming);

/***************************************************************
*  Function name:     ms7200_hdmirx_input_infoframe_get
*  Description:       get hdmi rx input infoframe
*  Input parameters:  None
*  Output parameters: pt_hdmi_rx: contain AVI, AIF, VSI etc.
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_input_infoframe_get(HDMI_CONFIG_T *pt_hdmi_rx);

/***************************************************************
*  Function name:     ms7200_hdmirx_audio_fifo_status_get
*  Description:       get hdmi rx audio fifo status
*  Input parameters:  None
*  Output parameters: None
*  Returned value:    audio fifo status, refer to AUDIO_FIFO_STATUS_E
***************************************************************/
UINT8 ms7200_hdmirx_audio_fifo_status_get(VOID);

/***************************************************************
*  Function name:     ms7200_hdmirx_hpd_config
*  Description:       set hdmi rx hpd status & edid
*                     if edid no need to update, set the pointer to null,
*                     otherwise will pull down rx hpd signal before config edid.
*  Input parameters:  b_config: hpd status
*                     u8_edid: edid for rx, no change set the pointer to null
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_hpd_config(BOOL b_config, UINT8 *u8_edid);

/***************************************************************
*  Function name:     ms7200_hdmirx_rxphy_config
*  Description:       set hdmi rx hpd status & edid
*                     if get clk over 5MHz, will config rxphy,
*                     otherwise power down analog part.
*  Input parameters:  None
*  Output parameters: u16_tmds_clk: clk input value while config
*  Returned value:    bit7: 1 = input clk valid, 0 = input clk invalid
*                     bit0: 1 = rxphy config done, 0 = rxphy config error
***************************************************************/
UINT8 ms7200_hdmirx_rxphy_config(UINT16 *u16_tmds_clk);

/***************************************************************
*  Function name:     ms7200_hdmirx_core_reset
*  Description:       reset submodule of hdmi rx core
*  Input parameters:  u32_reset_ctrl: refer to hdmi_rx_reset_ctrl in typedef.h
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_core_reset(UINT32 u32_reset_ctrl);

/***************************************************************
*  Function name:     ms7200_hdmirx_vmute_enable
*  Description:       enable/disable video mute function
*  Input parameters:  b_vmute: enable/disable
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_vmute_enable(BOOL b_vmute);

/***************************************************************
*  Function name:     ms7200_hdmirx_video_config
*  Description:       config for deep color, color space, pixel repeat input
*  Input parameters:  pt_hdmi_rx: refer to HDMI_CONFIG_T in typedef.h
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_hdmirx_video_config(HDMI_CONFIG_T *pt_hdmi_rx);

/***************************************************************
*  Function name:     ms7200_hdmirx_audio_config
*  Description:       config audio fifo, aupll for input change
*  Input parameters:  b_clk_change: 1 = need to reconfig aupll, 0 = audio fifo reset
*                     u16_tmds_clk: input tmds clk, use to config aupll
*  Output parameters: None
*  Returned value:    audio sample rate, unit: 100Hz
***************************************************************/
UINT16 ms7200_hdmirx_audio_config(BOOL b_clk_change, UINT16 u16_tmds_clk);

/***************************************************************
*  Function name:     ms7200_dvout_mode_config
*  Description:       config dvout mode, only config when system init
*  Input parameters:  t_dvout_config: dvout mode select
*                     b_spdif_out: 1 = use spdif out
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_init(DVOUT_CONFIG_T *t_dvout_config, BOOL b_spdif_out);

/***************************************************************
*  Function name:     ms7200_dvout_clk_driver_set
*  Description:       drive caoability select for pclk pad
*  Input parameters:  u8_drv_sel: 0 = 6mA, 1 = 12mA, 2 = 18mA, 3 = 24mA
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_clk_driver_set(UINT8 u8_drv_sel);

/***************************************************************
*  Function name:     ms7200_dvout_data_swap
*  Description:       config dvout mode, only config when system init
*  Input parameters:  u8_swap_mode: 0 = D0~D23 swap, 1 = 24-bit RB/UV swap, 2 = 16bit YC swap
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_data_swap(UINT8 u8_swap_mode);

/***************************************************************
*  Function name:     ms7200_dvout_data_swap_for_16bit_patch
*  Description:       swap all channel to y channel
*  Input parameters:  None
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_data_swap_for_16bit_patch(VOID);

/***************************************************************
*  Function name:     ms7200_dvout_phase_adjust
*  Description:       output phase adjust
*  Input parameters:  b_invert: output clk invert
*                     u8_delay: 1ns of one step, max 3ns
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_phase_adjust(BOOL b_invert, UINT8 u8_delay);

/***************************************************************
*  Function name:     ms7200_dvout_video_config
*  Description:       enable/disable video pad output
*  Input parameters:  b_config: 1 = enable output, 0 = disable output
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_video_config(BOOL b_config);

/***************************************************************
*  Function name:     ms7200_dvout_audio_config
*  Description:       enable/disable audio pad output
*  Input parameters:  b_config: 1 = enable output, 0 = disable output
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_dvout_audio_config(BOOL b_config);

/***************************************************************
*  Function name:     ms7200_tp_pure_color_set
*  Description:       test pattern value set
*  Input parameters:  u8_vr: v for yuv, r for rgb
*                     u8_yg: y for yuv, g for rgb
*                     u8_ub: u for yuv, b for rgb
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_tp_pure_color_set(UINT8 u8_vr, UINT8 u8_yg, UINT8 u8_ub);

/***************************************************************
*  Function name:     ma7200_tp_output_enable
*  Description:       enable/disable test pattern
*  Input parameters:  b_enable: 1 = enable output, 0 = disable output
*                     e_cs: input color space select
*  Output parameters: None
*  Returned value:    None
***************************************************************/
VOID ms7200_tp_output_enable(BOOL b_enable, HDMI_CS_E e_cs);


#ifdef __cplusplus
}
#endif

#endif  //__MACROSILICON_MS7200_H__
