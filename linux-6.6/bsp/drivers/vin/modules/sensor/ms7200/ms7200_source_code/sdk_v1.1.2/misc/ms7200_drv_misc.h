/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file    ms7200_drv_misc.h
* @author
* @version V1.0.0
* @date    15-Nov-2016
* @brief   misc module driver declare
* @history
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_DRV_MISC_H__
#define __MACROSILICON_MS7200_DRV_MISC_H__


#ifdef __cplusplus
extern "C" {
#endif

MS7200_DRV_API UINT8 ms7200drv_misc_package_sel_get(VOID);

/***************************************************************
*  Function name:   ms7200drv_misc_chipisvalid
*  Description:     check ms7200 chip is valid or not
*  Entry:           NULL
*  Returned value:  BOOL (valid is true)
*  Remark:
***************************************************************/
MS7200_DRV_API BOOL ms7200drv_misc_chipisvalid(VOID);

/***************************************************************
*  Function name:   ms7200drv_misc_audio_out_pad_enable
*  Description:     I2S and SPDIF output PAD enable. Don't ctrl i2s mclk
*  Entry:           [in]b_enable, if true enable output, else disable output
*  Returned value:  None
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_misc_audio_out_pad_enable(BOOL b_enable);

MS7200_DRV_API VOID ms7200drv_misc_audio_pad_output_spdif(BOOL b_spdif);

/***************************************************************
*  Function name:   ms7200drv_misc_audio_i2s_mclk_pad_enable
*  Description:     I2S mclk output PAD enable
*  Entry:           [in]b_enable, if true enable output, else disable output
*  Returned value:  None
*  Remark:
**************************************************************/
MS7200_DRV_API VOID ms7200drv_misc_audio_mclk_pad_enable(BOOL b_enable);


/***************************************************************
*  Function name:   ms7200drv_misc_audio_i2s_mclk_div
*  Description:     I2S mclk divider, default 128fs
*  Entry:           [in]u8_div
*                       0: 128fs
*                       1: 64fs
*                       2: 32fs
*                       3: 16fs
*
*  Returned value:  None
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_misc_audio_mclk_div(UINT8 u8_div);

MS7200_DRV_API VOID ms7200drv_misc_vclk_driver_set(UINT8 u8_drv_sel);

MS7200_DRV_API VOID ms7200drv_csc_config_input(HDMI_CS_E e_cs);
MS7200_DRV_API VOID ms7200drv_csc_config_output(DVOUT_CS_MODE_E e_cs);

MS7200_DRV_API VOID ms7200drv_tp_pure_color_set(UINT8 u8_vr, UINT8 u8_yg, UINT8 u8_ub);
MS7200_DRV_API VOID ms7200drv_tp_output_enable(BOOL b_enable);


#ifdef __cplusplus
}
#endif

#endif //__MACROSILICON_MS7200_DRV_MISC_H__
