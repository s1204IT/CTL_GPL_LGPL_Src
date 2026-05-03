/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file		ms7200_drv_misc.h
* @author
* @version	V1.0.0
* @date		15-Nov-2016
* @brief	misc module driver declare
* @history
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_DRV_DVOUT_H__
#define __MACROSILICON_MS7200_DRV_DVOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

MS7200_DRV_API VOID ms7200drv_dvout_mode_config(DVOUT_CONFIG_T *t_dvout_config);

MS7200_DRV_API VOID ms7200drv_dvout_clk_reset_release(BOOL b_release);

MS7200_DRV_API VOID ms7200drv_dvout_data_swap_all(VOID);

MS7200_DRV_API VOID ms7200drv_dvout_data_swap_rb_channel(VOID);

MS7200_DRV_API VOID ms7200drv_dvout_data_swap_yc_channel(VOID);

MS7200_DRV_API VOID ms7200drv_dvout_data_swap_16bit_all_y(VOID);

MS7200_DRV_API VOID ms7200drv_dvout_pa_adjust(BOOL b_invert, UINT8 u8_delay);

MS7200_DRV_API VOID ms7200drv_dvout_clk_sel(UINT8 u8_clk_rpt, UINT16 u16_video_clk);

#ifdef __cplusplus
}
#endif

#endif //__MACROSILICON_MS7200_DVOUT_H__
