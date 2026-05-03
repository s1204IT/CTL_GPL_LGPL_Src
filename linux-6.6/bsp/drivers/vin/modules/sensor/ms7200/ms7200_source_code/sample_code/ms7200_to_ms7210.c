/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @readme
*
* this file show an example for how to use ms7200 & ms7210 together on demo board
*
* if you only need to use ms7200,
* replace "ms7210_init()" and "ms7210_media_service()" related code with your own system.
*
* if you only need to use ms7210,
* replace "ms7200_init()" and "ms7200_media_service()" related code with your own system.
******************************************************************************/
#include "mculib_common.h"
#include "ms7200.h"
#include "ms7210.h"

static UINT16 g_u16_tmds_clk;
static BOOL g_b_rxphy_status = TRUE;
static BOOL g_b_input_valid = FALSE;
static UINT8 g_u8_rx_stable_timer_count;
#define RX_STABLE_TIMEOUT (2)
static VIDEOTIMING_T g_t_hdmirx_timing;
static HDMI_CONFIG_T g_t_hdmirx_infoframe;
//static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_RGB, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_HSVSDE };
static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_EMBANDED };

//static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_RGB, DVIN_BW_MODE_16_20_24BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_SDR, DVIN_SY_MODE_HSVSDE };
static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_YUV422, DVIN_BW_MODE_16_20_24BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_SDR, DVIN_SY_MODE_EMBANDED };
static BOOL g_b_output_valid = FALSE;
static VIDEOTIMING_T g_t_hdmitx_timing;
static HDMI_CONFIG_T g_t_hdmitx_infoframe;
static UINT8 g_u8_tx_stable_timer_count;
#define TX_STABLE_TIMEOUT (3)

static UINT8 u8_sys_edid_default_buf[256] = {
	// Explore Semiconductor, Inc. EDID Editor V2
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	// address 0x00
	0x4C, 0x2D, 0xFF, 0x0D, 0x58, 0x4D, 0x51, 0x30,	//
	0x1C, 0x1C, 0x01, 0x03, 0x80, 0x3D, 0x23, 0x78,	// address 0x10
	0x2A, 0x5F, 0xB1, 0xA2, 0x57, 0x4F, 0xA2, 0x28,	//
	0x0F, 0x50, 0x54, 0xBF, 0xEF, 0x80, 0x71, 0x4F,	// address 0x20
	0x81, 0x00, 0x81, 0xC0, 0x81, 0x80, 0x95, 0x00,	//
	0xA9, 0xC0, 0xB3, 0x00, 0x01, 0x01, 0x04, 0x74,	// address 0x30
	0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,	//
	0x8A, 0x00, 0x60, 0x59, 0x21, 0x00, 0x00, 0x1E,	// address 0x40
	0x00, 0x00, 0x00, 0xFD, 0x00, 0x18, 0x4B, 0x1E,	//
	0x5A, 0x1E, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20,	// address 0x50
	0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x55,	//
	0x32, 0x38, 0x48, 0x37, 0x35, 0x78, 0x0A, 0x20,	// address 0x60
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFF,	//
	0x00, 0x48, 0x54, 0x50, 0x4B, 0x37, 0x30, 0x30,	// address 0x70
	0x30, 0x35, 0x31, 0x0A, 0x20, 0x20, 0x01, 0xF7,	//
	0x02, 0x03, 0x26, 0xF0, 0x4B, 0x5F, 0x10, 0x04,	// address 0x80
	0x1F, 0x13, 0x03, 0x12, 0x20, 0x22, 0x5E, 0x5D,	//
	0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00,	// address 0x90
	0x6D, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x80, 0x3C,	//
	0x20, 0x10, 0x60, 0x01, 0x02, 0x03, 0x02, 0x3A,	// address 0xA0
	0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,	//
	0x45, 0x00, 0x60, 0x59, 0x21, 0x00, 0x00, 0x1E,	// address 0xB0
	0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38, 0x2D, 0x40,	//
	0x10, 0x2C, 0x45, 0x80, 0x60, 0x59, 0x21, 0x00,	// address 0xC0
	0x00, 0x1E, 0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0,	//
	0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0x60, 0x59,	// address 0xD0
	0x21, 0x00, 0x00, 0x1E, 0x56, 0x5E, 0x00, 0xA0,	//
	0xA0, 0xA0, 0x29, 0x50, 0x30, 0x20, 0x35, 0x00,	// address 0xE0
	0x60, 0x59, 0x21, 0x00, 0x00, 0x1A, 0x00, 0x00,	//
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// address 0xF0
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8,	//
};


VOID system_init(VOID)
{
	mculib_misc_init();
	mculib_uart_init(9600);
	mculib_timer1_config_ms(TIMER_BASE_10MS);
	mculib_i2c_init();
	mculib_chip_reset();
	mculib_enable_interrupt();
	LOG("mcu init done.");
}

VOID ms7200_init(VOID)
{
	LOG2("ms7200 chip connect = ", ms7200_chip_connect_detect(0));
	ms7200_hdmirx_init();
	//ms7200_hdmirx_hdcp_init(u8_rx_ksv_buf, u8_rx_key_buf);
	ms7200_dvout_init(&g_t_dvout_config, 0);
	//ms7200_dvout_clk_driver_set(3);
	ms7200_hdmirx_hpd_config(FALSE, u8_sys_edid_default_buf);
}

void sys_default_hdmi_video_config(void)
{
	//g_t_hdmi_infoframe.u8_hdmi_flag = TRUE;
	g_t_hdmirx_infoframe.u8_vic = 0;
	//g_t_hdmi_infoframe.u16_video_clk = 7425;
	g_t_hdmirx_infoframe.u8_clk_rpt = HDMI_X1CLK;
	g_t_hdmirx_infoframe.u8_scan_info = HDMI_OVERSCAN;
	g_t_hdmirx_infoframe.u8_aspect_ratio = HDMI_16X9;
	g_t_hdmirx_infoframe.u8_color_space = HDMI_RGB;
	g_t_hdmirx_infoframe.u8_color_depth = HDMI_COLOR_DEPTH_8BIT;
	g_t_hdmirx_infoframe.u8_colorimetry = HDMI_COLORIMETRY_709;
}

void sys_default_hdmi_vendor_specific_config(void)
{
	g_t_hdmirx_infoframe.u8_video_format = HDMI_NO_ADD_FORMAT;
	g_t_hdmirx_infoframe.u8_4Kx2K_vic = HDMI_4Kx2K_30HZ;
	g_t_hdmirx_infoframe.u8_3D_structure = HDMI_FRAME_PACKING;
}

void sys_default_hdmi_audio_config(void)
{
	g_t_hdmirx_infoframe.u8_audio_mode = HDMI_AUD_MODE_AUDIO_SAMPLE;
	g_t_hdmirx_infoframe.u8_audio_rate = HDMI_AUD_RATE_48K;
	g_t_hdmirx_infoframe.u8_audio_bits = HDMI_AUD_LENGTH_16BITS;
	g_t_hdmirx_infoframe.u8_audio_channels = HDMI_AUD_2CH;
	g_t_hdmirx_infoframe.u8_audio_speaker_locations = 0;
}

static int _abs(int i)
{
	/* compute absolute value of int argument */
	return (i < 0 ? -i : i);
}

VOID ms7200_media_service(VOID)
{
	UINT8 u8_rxphy_status = 0;
	UINT32 u32_int_status = 0;
	HDMI_CONFIG_T t_hdmi_infoframe;
	//detect 5v to pull up/down hpd
	ms7200_hdmirx_hpd_config(ms7200_hdmirx_source_connect_detect(), NULL);

	//reconfig rxphy when input clk change
	u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_HDMI, FREQ_LOCK_ISTS | FREQ_UNLOCK_ISTS | CLK_CHANGE_ISTS, TRUE);
	if (u32_int_status) {
		if (u32_int_status & CLK_CHANGE_ISTS) {
			if (_abs(ms7200_hdmirx_input_clk_get() - g_u16_tmds_clk) > 1000)
				g_b_rxphy_status = FALSE;
		}
		if ((u32_int_status & (FREQ_LOCK_ISTS | FREQ_UNLOCK_ISTS)) == FREQ_LOCK_ISTS)
			g_b_rxphy_status = TRUE;
		if (!g_b_rxphy_status) {
			ms7200_dvout_audio_config(FALSE);
			ms7200_dvout_video_config(FALSE);
			u8_rxphy_status = ms7200_hdmirx_rxphy_config(&g_u16_tmds_clk);
			g_b_rxphy_status = u8_rxphy_status ? (u8_rxphy_status & 0x01) : TRUE;
			g_b_input_valid = FALSE;
			g_u8_rx_stable_timer_count = 0;
			LOG2("g_u16_tmds_clk = ", g_u16_tmds_clk);
			LOG2("g_b_rxphy_status = ", g_b_rxphy_status);
		}
	}
	if (g_u16_tmds_clk < 500)
		return;

	//reset pdec when mdt change
	u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_MDT, MDT_STB_ISTS | MDT_USTB_ISTS, TRUE);
	if (u32_int_status) {
		if (!(g_b_input_valid && (u32_int_status == MDT_STB_ISTS))) {
			ms7200_dvout_audio_config(FALSE);
			ms7200_dvout_video_config(FALSE);
			ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
			ms7200_hdmirx_core_reset(HDMI_RX_CTRL_PDEC | HDMI_RX_CTRL_AUD);
			ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
			g_u8_rx_stable_timer_count = 0;
			if (g_b_input_valid) {
				g_b_input_valid = FALSE;
				LOG("timing unstable");
			}
		}
	}

	//get input timing status when mdt stable
	if (!g_b_input_valid) {
		if (g_u8_rx_stable_timer_count < RX_STABLE_TIMEOUT) {
			g_u8_rx_stable_timer_count++;
			return;
		}
		g_b_input_valid = ms7200_hdmirx_input_timing_get(&g_t_hdmirx_timing);
		if (g_b_input_valid) {
			u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
			ms7200_hdmirx_input_infoframe_get(&g_t_hdmirx_infoframe);
			g_t_hdmirx_infoframe.u16_video_clk = g_u16_tmds_clk;
			if (!(u32_int_status & AVI_RCV_ISTS))
				sys_default_hdmi_video_config();
			if (!(u32_int_status & AIF_RCV_ISTS))
				sys_default_hdmi_vendor_specific_config();
			if (!(u32_int_status & VSI_RCV_ISTS))
				sys_default_hdmi_vendor_specific_config();

			ms7200_hdmirx_video_config(&g_t_hdmirx_infoframe);
			ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
			ms7200_dvout_video_config(TRUE);
			ms7200_dvout_audio_config(TRUE);
			LOG("timing stable");
		} else
			LOG("timing error");
		return;
	}

	//reconfig system when input packet change
	u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
	if (u32_int_status & (AVI_CKS_CHG_ISTS | AIF_CKS_CHG_ISTS | VSI_CKS_CHG_ISTS)) {
		ms7200_hdmirx_input_infoframe_get(&t_hdmi_infoframe);
		t_hdmi_infoframe.u16_video_clk = g_u16_tmds_clk;
		if (!(u32_int_status & AVI_RCV_ISTS))
			sys_default_hdmi_video_config();
		if (!(u32_int_status & AIF_RCV_ISTS))
			sys_default_hdmi_vendor_specific_config();
		if (!(u32_int_status & VSI_RCV_ISTS))
			sys_default_hdmi_vendor_specific_config();
		if (memcmp(&t_hdmi_infoframe, &g_t_hdmirx_infoframe, sizeof(HDMI_CONFIG_T)) != 0) {
			ms7200_dvout_audio_config(FALSE);
			ms7200_dvout_video_config(FALSE);
			ms7200_hdmirx_video_config(&t_hdmi_infoframe);
			ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
			ms7200_dvout_video_config(TRUE);
			ms7200_dvout_audio_config(TRUE);
			g_t_hdmirx_infoframe = t_hdmi_infoframe;
			LOG("infoframe change");
		}
	}

	//reconfig audio when acr change or audio fifo error
	if (u32_int_status & (ACR_CTS_CHG_ISTS | ACR_N_CHG_ISTS)) {
		ms7200_hdmirx_audio_config(TRUE, g_u16_tmds_clk);
		LOG("acr change");
	} else {
		u32_int_status = ms7200_hdmirx_audio_fifo_status_get();
		if (u32_int_status & AFIF_THS_PASS_STS) {
			if (u32_int_status & (AFIF_UNDERFL_STS | AFIF_OVERFL_STS)) {
				ms7200_hdmirx_audio_config(FALSE, g_u16_tmds_clk);
				LOG("audio fifo reset");
			}
		}
	}
}

VOID ms7210_init(VOID)
{
	LOG2("ms7210 chip connect = ", ms7210_chip_connect_detect(0));
	ms7210_dvin_init(&g_t_dvin_config, 0);
	//ms7210_dvin_phase_adjust(FALSE, 1);
	//ms7210_dvin_data_swap(0);
}

VOID ms7210_media_service(VOID)
{
	BOOL b_outpit_valid = g_b_output_valid;
	if (g_b_input_valid && b_outpit_valid) {
		if (g_u8_tx_stable_timer_count < TX_STABLE_TIMEOUT) {
			g_u8_tx_stable_timer_count++;
			return;
		}
		if (!ms7210_hdmitx_input_timing_stable_get()) {
			b_outpit_valid = FALSE;
			g_u8_tx_stable_timer_count = 0;
			LOG("output unstable");
		}
	}
	if (g_b_input_valid != b_outpit_valid) {
		if (g_b_input_valid) {
			if (!g_b_output_valid) {
				g_t_hdmitx_timing = g_t_hdmirx_timing;
				g_t_hdmitx_infoframe = g_t_hdmirx_infoframe;
				ms7210_dvin_timing_config(&g_t_dvin_config, &g_t_hdmitx_timing, &g_t_hdmitx_infoframe);
				ms7210_dvin_video_config(TRUE);
				g_t_hdmitx_infoframe.u8_color_space = HDMI_RGB;
				g_t_hdmitx_infoframe.u8_color_depth = 0;
			}
			ms7210_hdmitx_output_config(&g_t_hdmitx_infoframe);
			g_u8_tx_stable_timer_count = 0;
			LOG("config output");
		} else {
			ms7210_hdmitx_shutdown_output();
			ms7210_dvin_video_config(FALSE);
			LOG("shutdown output");
		}
		g_b_output_valid = g_b_input_valid;
	}
}

VOID main(VOID)
{
	system_init();
	ms7200_init();
	ms7210_init();

	while (1) {
		mculib_uart_service();

		if (g_u16_sys_timer >= SYS_TIMEOUT_100MS) {
			g_u16_sys_timer = 0;
			ms7200_media_service();
			ms7210_media_service();
		}

		//if (g_u16_key_timer >= SYS_TIMEOUT_50MS) {
		//	g_u16_key_timer = 0;
		//	key_service();
		//}
	}
}
