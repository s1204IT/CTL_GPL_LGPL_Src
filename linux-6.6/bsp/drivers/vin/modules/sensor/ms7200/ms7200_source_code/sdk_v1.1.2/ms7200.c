/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file	ms7200.c
* @author
* @version V1.0.0
* @date	24-Nov-2020
* @brief   ms7200 SDK Library interfaces source file
* @history
*
* Copyright (c) 2009-2020, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include "ms7200_comm.h"
#include "ms7200_drv_hdmi_rx.h"
#include "ms7200_drv_misc.h"
#include "ms7200_drv_dvout.h"
#include "ms7200_mpi.h"
#include "ms7200.h"
#include "autobuild.h"

/****************************************/
static __CODE UINT8 g_u8_chip_addr[] = { 0xB2, 0x56 };
static __CODE UINT8 g_u8_chip_addr_num = sizeof(g_u8_chip_addr) / sizeof(UINT8);
/****************************************/

BOOL ms7200_chip_connect_detect(UINT8 u8_chip_addr)
{
	UINT8 i;
	for (i = 0; i < (u8_chip_addr ? 1 : g_u8_chip_addr_num); i++) {
		HAL_SetChipAddr(u8_chip_addr ? u8_chip_addr : g_u8_chip_addr[i]);
		if (ms7200drv_misc_package_sel_get() == 0x01) {
			if (ms7200drv_misc_chipisvalid())
				return TRUE;
		}
	}
	return FALSE;
}

VOID ms7200_hdmirx_init(VOID)
{
	ms7200drv_hdmi_rx_controller_reset(HDMI_RX_CTRL_MAIN);
	ms7200drv_hdmi_rx_init();
	ms7200drv_hdmi_rx_pi_phy_init();
}

VOID ms7200_hdmirx_hdcp_init(UINT8 *u8_rx_ksv, UINT8 *u8_rx_key)
{
	ms7200drv_hdmi_rx_controller_hdcp_init(u8_rx_ksv, u8_rx_key);
	ms7200drv_hdmi_rx_controller_hdcp_encryption_enable(TRUE);
}

VOID ms7200_hdmirx_interrupt_pin_set(HDMIRX_INT_INDEX_E e_int_index, UINT32 u32_int_flag, BOOL b_set)
{
	switch (e_int_index) {
	case RX_INT_INDEX_PDEC:
	case RX_INT_INDEX_MDT:
	case RX_INT_INDEX_HDMI:
	case RX_INT_INDEX_AUDFIFO:
		ms7200drv_hdmi_rx_interrupt_to_pin_ctrl((UINT8)e_int_index, u32_int_flag, b_set);
		break;
	case RX_INT_INDEX_5V:
		break;
	}
}

UINT32 ms7200_hdmirx_interrupt_get_and_clear(HDMIRX_INT_INDEX_E e_int_index, UINT32 u32_int_flag, BOOL b_clr)
{
	UINT32 u32_interrupt_status = 0;
	switch (e_int_index) {
	case RX_INT_INDEX_PDEC:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status(u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status_ext(u32_int_flag);
		break;
	case RX_INT_INDEX_MDT:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_get_mdt_interrupt_status((UINT16)u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_get_mdt_interrupt_status_ext(u32_int_flag);
		break;
	case RX_INT_INDEX_HDMI:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status(u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status_ext(u32_int_flag);
		break;
	case RX_INT_INDEX_AUDFIFO:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status(u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status_ext(u32_int_flag);
		break;
	case RX_INT_INDEX_5V:
		if (b_clr)
			ms7200drv_hdmi_rx_5v_interrupt_get_status(u32_int_flag);
		else
			ms7200drv_hdmi_rx_5v_interrupt_get_status_ext(u32_int_flag);
		break;
	}

	return u32_interrupt_status;
}

BOOL ms7200_hdmirx_source_connect_detect(VOID)
{
	return ms7200drv_hdmi_rx_5v_det();
}

UINT16 ms7200_hdmirx_input_clk_get(VOID)
{
	return ms7200drv_hdmi_rx_get_tmds_clk();
}

BOOL ms7200_hdmirx_input_timing_get(VIDEOTIMING_T *ptTiming)
{
	return ms7200drv_hdmi_rx_get_input_timing(ptTiming);
}

VOID ms7200_hdmirx_input_infoframe_get(HDMI_CONFIG_T *pt_hdmi_rx)
{
	ms7200drv_hdmi_rx_controller_get_input_config(pt_hdmi_rx);
}

UINT8 ms7200_hdmirx_audio_fifo_status_get(VOID)
{
	return ms7200drv_hdmi_rx_get_audio_fifo_status();
}

VOID ms7200_hdmirx_hpd_config(BOOL b_config, UINT8 *u8_edid)
{
	if (u8_edid != NULL) {
		ms7200drv_hdmi_rx_controller_hpd_set(FALSE);
		ms7200drv_hdmi_rx_edid_config(u8_edid);
	}
	if (b_config != ms7200drv_hdmi_rx_controller_hpd_get()) {
		if (b_config)
			Delay_ms(200);
		ms7200drv_hdmi_rx_controller_hpd_set(b_config);
	}
}

/* compute absolute value of int argument */
static int _abs(int i)
{
	return (i < 0 ? -i : i);
}

// b_config_phy_all, TRUE: config rxpll and rxphy EQ; FALSE: config rxpll
static UINT8 _hdmi_rx_phy_config(UINT16 *p_u16_tmds_clk, BOOL b_config_phy_all)
{
	UINT8 u8_times;
	UINT8 u8_flag = 0;
	UINT16 u16_clk1;
	UINT16 u16_clk2;

	u16_clk2 = (*p_u16_tmds_clk);

	//if rxpll unlock when rx config, do again
	for (u8_times = 0; u8_times < 3; u8_times++) {
		u8_flag &= 0xFC;
		u8_flag |= ms7200drv_hdmi_rx_pi_phy_config(u16_clk2, b_config_phy_all ? 0xFF : 0xFE);

		u16_clk1 = ms7200drv_hdmi_rx_get_tmds_clk();
		b_config_phy_all = TRUE;

		//20200520, if detect input clk invalid, return
		if (u16_clk1 < 500) {
			u8_flag |= 0x08;
			(*p_u16_tmds_clk) = u16_clk1;
			break;
		}

		// 5MHz
		if (u16_clk1 >= 500 && _abs(u16_clk1 - u16_clk2) >= 500) {
			u16_clk2 = u16_clk1;
			(*p_u16_tmds_clk) = u16_clk1;
			u8_flag |= 0x04;
		} else {
			if (u8_flag & 0x01)
				break;
		}
	}

	return u8_flag | (u8_times << 4);
}

UINT8 ms7200_hdmirx_rxphy_config(UINT16 *u16_tmds_clk)
{
	UINT8 u8_rxphy_flag = 0;

	*u16_tmds_clk = ms7200drv_hdmi_rx_get_tmds_clk();
	if (*u16_tmds_clk > 500) {
		//20190219, diable rx data
		ms7200drv_hdmi_rx_hdcp_detect_enable(FALSE);
		//config hdmi rx PLL
		u8_rxphy_flag = _hdmi_rx_phy_config(u16_tmds_clk, TRUE) & 0x01;
		u8_rxphy_flag |= 0x80;
		//enable rx data
		ms7200drv_hdmi_rx_hdcp_detect_enable(TRUE);
	} else {
		//20190116, enhace, reset HDMI before rxpll clk disable.
		ms7200drv_hdmi_rx_controller_reset(HDMI_RX_CTRL_HDMI);
		ms7200drv_hdmi_rx_phy_power_down();
		ms7200drv_hdmi_rx_pi_pll_release(FALSE);
	}

	return u8_rxphy_flag;
}

VOID ms7200_hdmirx_core_reset(UINT32 u32_reset_ctrl)
{
	ms7200drv_hdmi_rx_controller_reset(u32_reset_ctrl);
}

VOID ms7200_hdmirx_vmute_enable(BOOL b_vmute)
{
	ms7200drv_hdmi_rx_controller_set_vmute_enable(b_vmute);
}

VOID ms7200_hdmirx_video_config(HDMI_CONFIG_T *pt_hdmi_rx)
{
	ms7200drv_hdmi_rx_controller_pixel_clk_config(pt_hdmi_rx->u8_color_depth, pt_hdmi_rx->u8_clk_rpt);
	//20181221, config rx auto avmute color.
	ms7200drv_hdmi_rx_controller_set_avmute_black_color(pt_hdmi_rx->u8_color_space);

	//20190515, reset video fifo after pixel config done.
	ms7200drv_hdmi_rx_video_fifo_reset(TRUE);
	Delay_us(100);
	ms7200drv_hdmi_rx_video_fifo_reset(FALSE);

	ms7200drv_csc_config_input((HDMI_CS_E)pt_hdmi_rx->u8_color_space);
	ms7200drv_dvout_clk_sel(pt_hdmi_rx->u8_clk_rpt, pt_hdmi_rx->u16_video_clk);
}

UINT16 ms7200_hdmirx_audio_config(BOOL b_clk_change, UINT16 u16_tmds_clk)
{
	UINT16 u16_fs = 0;

	ms7200drv_hdmi_rx_audio_fifo_reset(TRUE);

	if (b_clk_change) {
		u16_fs = ms7200drv_hdmi_rx_audio_config(RX_AUDIO_DLL_TO_PLL_MODE, u16_tmds_clk);
#if VIN_FALSE
		g_st_hdmi_timing.u8_audio_rate = _hdmi_rx_audio_fs_type_get(u16_fs);
#endif
		//LOG2("u16_audio_fs = ", u16_fs);
	}

	Delay_ms(1);
	ms7200drv_hdmi_rx_audio_fifo_reset(FALSE);

	return u16_fs;
}

VOID ms7200_dvout_init(DVOUT_CONFIG_T *t_dvout_config, BOOL b_spdif_out)
{
	DVOUT_CS_MODE_E e_cs_mode = (DVOUT_CS_MODE_E)t_dvout_config->u8_cs_mode;
	if (e_cs_mode == DVOUT_CS_MODE_YUV422)
		e_cs_mode = DVOUT_CS_MODE_YUV444;

	ms7200drv_csc_config_output(e_cs_mode);
	ms7200drv_dvout_mode_config(t_dvout_config);
	ms7200drv_misc_audio_pad_output_spdif(b_spdif_out);
	ms7200drv_misc_audio_mclk_pad_enable(TRUE);
	ms7200_dvout_clk_driver_set(0);
}

VOID ms7200_dvout_clk_driver_set(UINT8 u8_drv_sel)
{
	ms7200drv_misc_vclk_driver_set(u8_drv_sel);
}

VOID ms7200_dvout_data_swap(UINT8 u8_swap_mode)
{
	switch (u8_swap_mode) {
	case 0:
		ms7200drv_dvout_data_swap_all();
		break;
	case 1:
		ms7200drv_dvout_data_swap_rb_channel();
		break;
	case 2:
		ms7200drv_dvout_data_swap_yc_channel();
	}
}

VOID ms7200_dvout_data_swap_for_16bit_patch(VOID)
{
	ms7200drv_dvout_data_swap_16bit_all_y();
}

VOID ms7200_dvout_phase_adjust(BOOL b_invert, UINT8 u8_delay)
{
	ms7200drv_dvout_pa_adjust(b_invert, u8_delay);
}

VOID ms7200_dvout_video_config(BOOL b_config)
{
	ms7200drv_dvout_clk_reset_release(b_config);
}

VOID ms7200_dvout_audio_config(BOOL b_config)
{
	ms7200drv_misc_audio_out_pad_enable(b_config);
}

VOID ms7200_tp_pure_color_set(UINT8 u8_vr, UINT8 u8_yg, UINT8 u8_ub)
{
	ms7200drv_tp_pure_color_set(u8_vr, u8_yg, u8_ub);
}

VOID ms7200_tp_output_enable(BOOL b_enable, HDMI_CS_E e_cs)
{
	ms7200drv_csc_config_input(e_cs);
	ms7200drv_tp_output_enable(b_enable);
}
