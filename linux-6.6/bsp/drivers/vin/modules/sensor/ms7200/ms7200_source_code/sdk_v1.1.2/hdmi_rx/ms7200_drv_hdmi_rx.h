/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file    ms7200_drv_hdmi_rx.h
* @author
* @version V1.0.0
* @date    20-Nov-2017
* @brief   hdmi rx module driver declare
* @history
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_DRV_HDMI_RX_H__
#define __MACROSILICON_MS7200_DRV_HDMI_RX_H__

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_init
*  Description:     hdmi rx module init
*  Entry:           [in]None
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_init(VOID);
MS7200_DRV_API VOID ms7200drv_hdmi_rx_audio_output_enable(BOOL b_enable);
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hdcp_data_enable(BOOL b_enable);
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_5v_det(VOID);
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_5v_interrupt_get_status(UINT8 u8_int_flag);
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_5v_interrupt_get_status_ext(UINT8 u8_int_flag);
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_get_tmds_overload(VOID);
MS7200_DRV_API VOID ms7200drv_hdmi_rx_tmds_overload_protect_disable(BOOL b_disable);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_edid_config
*  Description:     rx edid config
*  Entry:           [in]u8Edid
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_edid_config(UINT8 *u8Edid);


MS7200_DRV_API BOOL ms7200drv_errdet_wrong_status(UINT16 u16_error_val);
MS7200_DRV_API BOOL ms7200drv_rxpll_lock_status(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_pi_phy_init
*  Description:     rx pi phy config, default init term enable
*  Entry:           None
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API BOOL ms7200drv_hdmi_rx_pi_phy_init(VOID);


MS7200_DRV_API VOID ms7200drv_hdmi_rx_pi_phy_term_enable(BOOL b_enable);


MS7200_DRV_API VOID ms7200drv_hdmi_rx_pi_pll_release(BOOL bEnable);

MS7200_DRV_API VOID ms7200drv_hdmi_rx_phy_power_down(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_pi_pll_config
*  Description:     rx pi phy config
*  Entry:           [in]u16TmdsClk
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_pi_phy_config(UINT16 u16TmdsClk, UINT8 u8_eq_gain);

//for 28eq
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_pi_phy_config_with_28eq(UINT16 u16TmdsClk, UINT8 u8_eq_gain);


/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hpd_set
*  Description:     set rx hpd status
*  Entry:           [in]bReady
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hpd_set(BOOL bReady);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hpd_set
*  Description:     get rx hpd status
*  Entry:           None
*
*  Returned value:  if rx hpd is ready
*  Remark:
***************************************************************/
MS7200_DRV_API BOOL ms7200drv_hdmi_rx_controller_hpd_get(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_encryption_enable
*  Description:     rx hdcp enable
*  Entry:
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hdcp_encryption_enable(BOOL bEnable);


MS7200_DRV_API VOID ms7200drv_hdmi_rx_hdcp_detect_enable(BOOL bEnable);


/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_hdmi_mode_set
*  Description:     rx hdcp hdmi mode config
*  Entry:           [in]bHdmiMode
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hdcp_hdmi_mode_set(UINT8 bHdmiMode);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_bksv_set
*  Description:     rx hdcp bksv config
*  Entry:           [in]u8RxKsv
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hdcp_bksv_set(UINT8 *u8RxKsv);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_key_set
*  Description:     rx hdcp key config
*  Entry:           [in]u8RxKey
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hdcp_key_set(UINT8 *u8RxKey);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_init
*  Description:     rx hdcp init config
*  Entry:           [in]u8RxKsv
*                   [in]u8RxKey
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_hdcp_init(UINT8 *u8RxKsv, UINT8 *u8RxKey);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_get_status
*  Description:     hdmi rx hdcp working status
*  Entry:           [in]None
*
*  Returned value:  return true if hdcp working
*  Remark:
***************************************************************/
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_controller_hdcp_get_status(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_hdcp_get_Ri
*  Description:     hdmi rx hdcp get Link verification response value
*  Entry:           [in]None
*
*  Returned value:  return UINT16 value
*  Remark:
***************************************************************/
MS7200_DRV_API UINT16 ms7200drv_hdmi_rx_controller_hdcp_get_Ri(VOID);


/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_get_tmds_clk
*  Description:     get hdmi input tmds clk
*  Entry:           [in]None
*
*  Returned value:  return UINT16 value, uint 10000Hz
*  Remark:
***************************************************************/
MS7200_DRV_API UINT16 ms7200drv_hdmi_rx_get_tmds_clk(VOID);


/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_get_mdt_interrupt_status
*  Description:     get mdt interrupt status
*  Entry:           None
*
*  Returned value:  return status in bits MSRT_BITS11_0, enum refer to MDTISTS_E
*  Remark:
***************************************************************/
MS7200_DRV_API UINT16 ms7200drv_hdmi_rx_get_mdt_interrupt_status(UINT16 u16_mask);


//read but don't clear interrupt status
MS7200_DRV_API UINT16 ms7200drv_hdmi_rx_get_mdt_interrupt_status_ext(UINT16 u16_mask);


/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_get_input_timing
*  Description:     get input video timing information
*  Entry:           [out]ptTiming
*                        u8_polarity, enum to e_RxMdtPol
*                        u16_htotal, uint pixel clk
*                        u16_vtotal, uint line
*                        u16_hactive
*                        u16_vactive
*                        u16_pixclk, input tmds clk, uint 10000Hz,
*                        u16_vfreq, uint 0.01Hz
*                        u16_hoffset, uint pixel clk
*                        u16_voffset, uint line
*                        u16_hsyncwidth, reserve
*                        u16_vsyncwidth, reserve
*                        others para, reserve
*
*  Returned value:  if input timing valid return true, else return false
*  Remark:
***************************************************************/
MS7200_DRV_API BOOL ms7200drv_hdmi_rx_get_input_timing(VIDEOTIMING_T *ptTiming);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_dvidet
*  Description:     get dvi mode detect result
*  Entry:           None
*
*  Returned value:  if rx input is dvi mode
*  Remark:
***************************************************************/
MS7200_DRV_API BOOL ms7200drv_hdmi_rx_controller_dvidet(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_scdc_config
*  Description:     scdc config
*  Entry:           [in]bPowerBySource
*
*  Returned value:  if rx input is dvi mode
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_scdc_config(BOOL bPowerBySource);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_reset
*  Description:     choose module to do reset operation
*  Entry:           [in]eModule, refer to e_RxCtrl
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_reset(UINT32 eModule);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_enable
*  Description:     choose module to do enable operation
*  Entry:           [in]eModule
*                   [in]bEnable
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_enable(UINT32 eModule, BOOL bEnable);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_get_audio_fifo_status
*  Description:     get hdmi rx audio fifo status
*  Entry:           [in]None
*
*  Returned value:  return UINT8 value, refer to AUDIO_FIFO_STATUS_E
*
*  Remark:
***************************************************************/
MS7200_DRV_API UINT8 ms7200drv_hdmi_rx_get_audio_fifo_status(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_audio_fifo_reset
*  Description:     hdmi rx module audio fifo reset
*  Entry:           [in]b_reset, if true reset fifo, else release fifo reset
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_audio_fifo_reset(BOOL b_reset);

MS7200_DRV_API VOID ms7200drv_hdmi_rx_video_fifo_reset(BOOL b_reset);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status
*  Description:     hdmi rx module audio fifo reset
*  Entry:           None
*
*  Returned value:  return status in bits MSRT_BITS31_0, enum refer to PDECISTS_E
*  Remark:
***************************************************************/
MS7200_DRV_API UINT32 ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status(UINT32 u32_mask);

MS7200_DRV_API UINT32 ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status(UINT32 u32_mask);

MS7200_DRV_API UINT32 ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status(UINT32 u32_mask);

MS7200_DRV_API UINT32 ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status_ext(UINT32 u32_mask);

MS7200_DRV_API UINT32 ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status_ext(UINT32 u32_mask);

MS7200_DRV_API UINT32 ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status_ext(UINT32 u32_mask);

MS7200_DRV_API VOID ms7200drv_hdmi_rx_interrupt_to_pin_ctrl(UINT8 u8_module, UINT32 u32_mask, BOOL b_enable_to_pin);

MS7200_DRV_API BOOL ms7200drv_hdmi_rx_controller_get_gcp_avmute(VOID);

//enum to HDMI_CS_E
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_set_avmute_black_color(UINT8 u8_cs);

MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_set_vmute_enable(BOOL b_vmute);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_get_input_config
*  Description:     hdmi rx get input config status
*  Entry:           [out]pt_hdmi_rx
*                        u8_hdmi_flag;          // FALSE = dvi out;  TRUE = hdmi out
*                        u8_vic;                // reference to CEA-861 VIC
*                        u16_video_clk;         // TMDS video clk, uint 10000Hz
*                        u8_clk_rpt;            // enum refer to HDMI_CLK_RPT_E. X2CLK = 480i/576i, others = X1CLK
*                        u8_aspect_ratio;       // enum refer to HDMI_ASPECT_RATIO_E
*                        u8_color_space;        // enum refer to HDMI_CS_E
*                        u8_color_depth;        // enum refer to HDMI_COLOR_DEPTH_E
*                        u8_colorimetry;        // enum refer to HDMI_COLORIMETRY_E. IT601 = 480i/576i/480p/576p, ohters = IT709
*                        u8_audio_mode;         // enum refer to HDMI_AUDIO_MODE_E
*                        u8_audio_rate;         // enum refer to HDMI_AUDIO_RATE_E
*                        u8_audio_bits;         // enum refer to HDMI_AUDIO_LENGTH_E
*                        u8_audio_channels;     // enum refer to HDMI_AUDIO_CHN_E
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_get_input_config(HDMI_CONFIG_T *pt_hdmi_rx);

MS7200_DRV_API BOOL ms7200drv_hdmi_rx_controller_audio_channel_config(UINT8 u8_chn, UINT8 u8_speaker_locations);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_controller_pixel_clk_config
*  Description:     hdmi rx set pixel clock based on deep color mode
*  Entry:           [in]u8ColorDepth
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_controller_pixel_clk_config(UINT8 u8ColorDepth, UINT8 u8ClkRepeat);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_pi_phy_eq_sel
*  Description:     hdmi rx pi phy eq band select
*  Entry:           None
*
*  Returned value:
*  Remark:
***************************************************************/
MS7200_DRV_API VOID ms7200drv_hdmi_rx_pi_phy_eq_sel(VOID);

/***************************************************************
*  Function name:   ms7200drv_hdmi_rx_audio_config
*  Description:     config audio clk
*  Entry:           [in]u8_audio_clk_mode: refer to e_RxAudioClkMode
*                       u16_tmds_clk: video clk, uint:10000Hz
*
*  Returned value: return UINT16 audio fs, uint:100Hz
*  Remark:
***************************************************************/
//u16_fs, uint: 100Hz; u16_tmds_clk, uint:10000Hz
MS7200_DRV_API UINT16 ms7200drv_hdmi_rx_audio_config(UINT8 u8_audio_clk_mode, UINT16 u16_tmds_clk);

#ifdef __cplusplus
}
#endif

#endif //__MACROSILICON_MS7200_DRV_HDMI_RX_H__
