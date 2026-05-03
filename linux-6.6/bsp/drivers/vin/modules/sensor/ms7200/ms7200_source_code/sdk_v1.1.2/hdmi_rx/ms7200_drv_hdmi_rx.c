/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file		ms7200_drv_hdmi_rx.c
* @author
* @version	V1.0.0
* @date		20-Nov-2017
* @brief	hdmi rx module driver source file
* @history
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include "ms7200_comm.h"
#include "ms7200_mpi.h"
#include "ms7200_drv_hdmi_rx_config.h"
#include "ms7200_drv_hdmi_rx.h"

#define AUDIO_DLL_64FS_TO_PLL_ENABLE	(1)

#define RXPHY_DEBUG_LOG (0)

typedef struct {
	UINT8 u8Eq0GainVal;
	UINT8 u8Eq1GainVal;
	UINT8 u8Eq2GainVal;
	UINT8 u8Eq3GainVal;
} RX_EQ_GAIN_T;

#if VIN_FALSE //14eq
static RX_EQ_GAIN_T __CODE ptEqGainVal[] = {
	{ 7, 7, 7, 7 }, { 6, 6, 7, 7 }, { 5, 5, 7, 7 }, { 4, 4, 7, 7 }, { 3, 3, 7, 7 }, { 2, 2, 7, 7 }, { 1, 1, 7, 7 },
	{ 0, 0, 7, 7 }, { 0, 0, 6, 6 }, { 0, 0, 5, 5 }, { 0, 0, 4, 4 }, { 0, 0, 3, 3 }, { 0, 0, 2, 2 }, { 0, 0, 1, 1 },
};
#else //28eq
static RX_EQ_GAIN_T __CODE ptEqGainVal[] = {
	{ 7, 7, 7, 7 }, { 6, 7, 7, 7 }, { 5, 7, 7, 7 }, { 4, 7, 7, 7 }, { 3, 7, 7, 7 }, { 2, 7, 7, 7 }, { 1, 7, 7, 7 },
	{ 0, 7, 7, 7 }, { 0, 6, 7, 7 }, { 0, 5, 7, 7 }, { 0, 4, 7, 7 }, { 0, 3, 7, 7 }, { 0, 2, 7, 7 }, { 0, 1, 7, 7 },
	{ 0, 0, 7, 7 }, { 0, 0, 6, 7 }, { 0, 0, 5, 7 }, { 0, 0, 4, 7 }, { 0, 0, 3, 7 }, { 0, 0, 2, 7 }, { 0, 0, 1, 7 },
	{ 0, 0, 0, 7 }, { 0, 0, 0, 6 }, { 0, 0, 0, 5 }, { 0, 0, 0, 4 }, { 0, 0, 0, 3 }, { 0, 0, 0, 2 }, { 0, 0, 0, 1 },
};
#endif

#define g_u8EqGainNum (sizeof(ptEqGainVal) / sizeof(RX_EQ_GAIN_T))

#define ms7200drv_hdmi_rx_pi_clk_ofst_sel(u8_val)				HAL_ModBits(REG_PI_RX_OFST_CTRL4, MSRT_BITS3_0, u8_val)

#define ms7200drv_hdmi_rx_pi_dcm_sel(u8_th)						HAL_ModBits(REG_PI_CTRL0, MSRT_BITS1_0, u8_th)
#define ms7200drv_hdmi_rx_pi_det_th(u8_th)						HAL_ModBits(REG_PI_CTRL1, MSRT_BITS2_0, u8_th)

#define ms7200drv_hdmi_rx_edid_reset_release(b_release)			HAL_ToggleBits(REG_MISC_RST_CTRL1, MSRT_BIT2, b_release)

#define ms7200drv_hdmi_rx_audio_mute_ctrl(u8_aud_mute_sel)		HAL_ModBits_Ex(REG_AUD_MUTE_CTRL, MSRT_BITS6_5,  u8_aud_mute_sel << 5)
#define ms7200drv_hdmi_rx_audio_i2s_output_enable()				HAL_ModBits_Ex(REG_AUD_SAO_CTRL, 0x7E0,  0x0)
#define ms7200drv_hdmi_rx_audio_spdif_output_enable()			HAL_ModBits_Ex(REG_AUD_SAO_CTRL, MSRT_BITS4_1,  0x0)
//#define ms7200drv_hdmi_rx_audio_output_enable()				HAL_ModBits_Ex(REG_AUD_SAO_CTRL, 0x7FE,  0x0)
#define ms7200drv_hdmi_rx_afif_th(u32_value)					HAL_WriteDWord_Ex(REG_AUD_FIFO_TH, u32_value)

#define ms7200drv_hdmi_rx_mdt_vt_line_ith(u8_value)				HAL_ModBits_Ex(REG_MD_VTH, MSRT_BITS7_6,  (u8_value - 1) << 6)

//0:PLL; 1:DLL+PLL; 2:DLL
#define ms7200drv_hdmi_audio_clk_sel(u8_val)					HAL_ModBits(REG_MISC_RX_PHY_SEL, MSRT_BITS3_2, u8_val << 2)
#define ms7200drv_hdmi_audio_clk_enable(b_enable)				HAL_ToggleBits(REG_MISC_CLK_CTRL1, MSRT_BIT0, b_enable)
#define ms7200drv_hdmi_audio_dll_clk_reset_release(b_release)	HAL_ToggleBits(REG_MISC_RST_CTRL0, MSRT_BIT7, b_release)
#define ms7200drv_hdmi_audio_pll_clk_reset_release(b_release)	HAL_ToggleBits(REG_MISC_RST_CTRL0, MSRT_BIT6, b_release)


//helper
static VOID _drv_errdet_read_enable(BOOL bEnable);
static VOID _drv_hdmi_rx_audio_init(VOID);


VOID ms7200drv_hdmi_rx_init(VOID)
{
	//20181218, for register program val check only.no effect.
	//HAL_WriteByte(REG_PI_RX_REGISTER_CHECK, REG_PI_RX_REGISTER_CHECK_VAL);
	//20200518, r_osc25m_driver change 4mA->2mA
	HAL_ClrBits(REG_MISC_IO_CTRL1, MSRT_BITS1_0);

	//RC_25M config
	HAL_WriteByte(REG_MISC_RC_CTRL1, MS7200_RC_CTRL1);
	HAL_WriteByte(REG_MISC_RC_CTRL2, MS7200_RC_CTRL2);

	// 20180404, PI PHY, tmds noise filter
	ms7200drv_hdmi_rx_pi_clk_ofst_sel(5);

	//PI phase detection threshold set to 0
	ms7200drv_hdmi_rx_pi_dcm_sel(1);
	ms7200drv_hdmi_rx_pi_det_th(0);

	_drv_hdmi_rx_audio_init();

	//tmds clk good detect, 1/16 = 6.25% margin
	//20200325, fixed BUG for BS5330A, clk_measure_debug = 1
	HAL_WriteDWord_Ex(REG_HDMI_CKM_EVLTM, 0x0112FFF0UL);
	//if osc = 24M, clk detect max: 380MHz, min: 12MHz
	HAL_WriteDWord_Ex(REG_HDMI_CKM_F, 0xFFFF0800UL);
	//20181218, mdt detection time set to 2 frame stable
	//0x1F00, (0x2) << 8
	//20201117, [12:8] change 2->0, resolve TEK_DTG534 source interlace mode(480i/576i/1080i) can't be work.
	HAL_WriteDWord_Ex(REG_HDMI_MODE_RECOVER, 0x30110000UL);

	//20190328, MDT stable, do not use vblank status.
	HAL_ModBits_Ex(REG_MD_IL_CTRL, 0xFF000000UL, (0x11UL) << 24);

	//20190329, mdt interrupt magin update.
	//add hs vs active time
	HAL_WriteDWord_Ex(REG_MD_HCTRL2, 0x223a);
	//VTOT_LIN_ITH = 3line, resolve for interlace timing input issue.
	//VACT_LIN_ITH = 3line
	HAL_WriteDWord_Ex(REG_MD_VTH, 0x0a92);

	//20191123, resolve sanxing DVD, speaker_locations is 0x0A, but chn number is 8 chn issue.
	//fixed subpacket_ovr_speaker_allocation_en = 1
	//HAL_SetBits_Ex(REG_AUD_FIFO_CTRL, 0x1UL << 17);

	//2020724, set audio cts interruput threshold, cts value change >= 32
	HAL_ModBits_Ex(REG_PDEC_ACRM_CTRL, 0x1c,  5 << 2);
	//20180710, set audio fifo start threshold
	//AFIF_TH_START = 0x300 / 4 = 0xc0
	//AFIF_TH_MAX,AFIF_TH_MIN = 0x180 / 4 = 0x60
	ms7200drv_hdmi_rx_afif_th((0xc0UL << 18) | (0x60UL << 9) | (0x60UL));

	//automatic mute enable for FIFO over- underflow
	//20200926, resolve audio maybe force mute because HW bug.
	ms7200drv_hdmi_rx_audio_mute_ctrl(0x0);
	//enable I2S output, enable SPDIF output
	ms7200drv_hdmi_rx_audio_output_enable(TRUE);

	//hdmi status contorl
	//deep color status clr when 2 frame no valild deep color information input
	HAL_ModBits_Ex(REG_HDMI_DCM_CTRL, 0x3c, 2 << 2);

	//pdec error filter
	HAL_SetBits_Ex(REG_PDEC_ERR_FILTER, 0xfff);

#if MS7200_HDMI_RX_INT_ENABLE
	//enable rx int to PIN.clk change
	HAL_WriteDWord_Ex(REG_HDMI_IEN_SET, 0x40);
	//enable interrupt PIN. when interrupt coming, PIN 1->0.
	//when interrupt register bit be cleared, PIN 0->1
	HAL_SetBits(REG_MISC_INT_PIN_CTRL, 0x30);
#endif
}

VOID ms7200drv_hdmi_rx_audio_output_enable(BOOL b_enable)
{
	HAL_ToggleBits_Ex(REG_AUD_SAO_CTRL, 0x7FE, !b_enable);
}

VOID ms7200drv_hdmi_rx_controller_hdcp_data_enable(BOOL b_enable)
{
	HAL_ModBits_Ex(REG_HDMI_SYNC_CTRL, 0x30000000UL, (UINT32)(b_enable ? 0x10UL : 0x20UL) << 24);
}

UINT8 ms7200drv_hdmi_rx_5v_det(VOID)
{
	return (HAL_ReadByte(REG_MISC_5V_DET) & MSRT_BIT0);
}

UINT8 ms7200drv_hdmi_rx_5v_interrupt_get_status(UINT8 u8_int_flag)
{
	UINT8 u8_int_status = HAL_ReadByte(REG_MISC_5V_DET) & MSRT_BITS3_2 & u8_int_flag;
	HAL_ClrBits(REG_MISC_5V_DET, MSRT_BIT1);
	return u8_int_status;
}

UINT8 ms7200drv_hdmi_rx_5v_interrupt_get_status_ext(UINT8 u8_int_flag)
{
	return HAL_ReadByte(REG_MISC_5V_DET) & MSRT_BITS3_2 & u8_int_flag;
}

UINT8 ms7200drv_hdmi_rx_get_tmds_overload(VOID)
{
	return HAL_ReadByte(REG_PI_RX_OVL_STS);
}

VOID ms7200drv_hdmi_rx_tmds_overload_protect_disable(BOOL b_disable)
{
	HAL_ToggleBits(REG_PI_RX_CTRL1, MSRT_BIT0, b_disable);
}

VOID ms7200drv_hdmi_rx_edid_config(UINT8 *u8Edid)
{
	//UINT16 u16Range = (u8Ext > 3) ? 0x200 : (0x80 * (u8Ext + 1));
	HAL_SetBits(REG_MISC_RX_PHY_SEL, MSRT_BIT7);
	HAL_WriteBytes(REG_EDID_BLOCK0, 0x100, u8Edid);
	HAL_ClrBits(REG_MISC_RX_PHY_SEL, MSRT_BIT7);
}

static VOID _drv_hdmi_rx_pi_rcv_release(BOOL bEnable)
{
	HAL_ToggleBits(REG_MISC_RST_CTRL0, MSRT_BIT2, bEnable);
}

VOID ms7200drv_hdmi_rx_pi_term_resoveride_set(UINT8 u8ResValue)
{
	HAL_ModBits(REG_PI_RX_TRM_CTRL, MSRT_BITS5_0, u8ResValue);
}

VOID ms7200drv_hdmi_rx_pi_phy_term_enable(BOOL b_enable)
{
	HAL_ToggleBits(REG_PI_RX_TRM_CTRL, MSRT_BIT7, b_enable);
}

static BOOL _drv_hdmi_rx_pi_term_config(e_RxPhyMode eMode, UINT8 u8Time)
{
	BOOL bTermDone = TRUE;
	//UINT8 u8TermVal = 0;
	ms7200drv_hdmi_rx_tmds_overload_protect_disable(TRUE);
	HAL_SetBits(REG_PI_RX_TRM_CTRL, MSRT_BIT7);
	HAL_SetBits(REG_PI_RX_CTRL0, MSRT_BIT0);
#if (MS7200_HDMI_RX_TMDS_OVERLOAD_PROTECT_ENABLE)
	ms7200drv_hdmi_rx_tmds_overload_protect_disable(FALSE);
#endif
	return bTermDone;
}

VOID ms7200drv_hdmi_rx_pi_offset_enable_set(e_RxChannel eChannel)
{
	HAL_ModBits(REG_PI_RX_OFST_CTRL5, MSRT_BITS2_0, 1 << eChannel);
}

VOID ms7200drv_hdmi_rx_pi_offset_dec_set(e_RxChannel eChannel, UINT8 u8OffsetDec)
{
	HAL_ModBits(REG_PI_RX_OFST_CTRL1 + eChannel, MSRT_BITS7_4, u8OffsetDec << 4);
}

VOID ms7200drv_hdmi_rx_pi_offset_inc_set(e_RxChannel eChannel, UINT8 u8OffsetInc)
{
	HAL_ModBits(REG_PI_RX_OFST_CTRL1 + eChannel, MSRT_BITS3_0, u8OffsetInc);
}

BOOL ms7200drv_hdmi_rx_pi_offset_cmpout_get(VOID)
{
	return (HAL_ReadByte(REG_PI_RX_OFST_CTRL5) & MSRT_BIT4) >> 4;
}

static BOOL _drv_hdmi_rx_pi_offset_config(UINT8 u8Time)
{
	BOOL bOffsetDone = FALSE;
	UINT8 u8Channel;
	UINT8 u8OffsetDec, u8OffsetInc;
	for (u8Channel = 0; u8Channel <= 2; u8Channel++) {
		bOffsetDone = FALSE;
		ms7200drv_hdmi_rx_pi_offset_enable_set((e_RxChannel)u8Channel);
		u8OffsetDec = 0x10;
		u8OffsetInc = 0;
		while (!bOffsetDone) {
			if (u8OffsetDec == 0)
				break;
			ms7200drv_hdmi_rx_pi_offset_dec_set((e_RxChannel)u8Channel, --u8OffsetDec);
			Delay_ms(u8Time);
			bOffsetDone = ms7200drv_hdmi_rx_pi_offset_cmpout_get();
		}
		while (!bOffsetDone) {
			if (u8OffsetInc == 16) {
				--u8OffsetInc;
				break;
			}
			ms7200drv_hdmi_rx_pi_offset_inc_set((e_RxChannel)u8Channel, u8OffsetInc++);
			Delay_ms(u8Time);
			bOffsetDone = ms7200drv_hdmi_rx_pi_offset_cmpout_get();
		}
		if (!bOffsetDone) {
			u8OffsetInc = 0;
			ms7200drv_hdmi_rx_pi_offset_inc_set((e_RxChannel)u8Channel, u8OffsetInc);
			Delay_ms(u8Time);
			bOffsetDone = ms7200drv_hdmi_rx_pi_offset_cmpout_get();
		}
#if RXPHY_DEBUG_LOG
		MS7200_LOG2("u8Channel = ", u8Channel);
		MS7200_LOG2("bOffsetDone = ", bOffsetDone);
		MS7200_LOG2("u8OffsetDec = ", u8OffsetDec);
		MS7200_LOG2("u8OffsetInc = ", u8OffsetInc);
#endif
	}
	HAL_ClrBits(REG_PI_RX_OFST_CTRL5, MSRT_BITS2_0);
	return bOffsetDone;
}

static VOID _drv_errdet_read_enable(BOOL bEnable)
{
	HAL_ToggleBits_Ex(REG_SCDC_CONFIG, MSRT_BIT0, bEnable);
	HAL_ToggleBits_Ex(REG_HDMI20_CONTROL, 0x1f << 8, bEnable);
}

static VOID _drv_errdet_clr_swsel(BOOL bEnable)
{
	HAL_ToggleBits_Ex(REG_HDMI20_CONTROL, 1UL << 14, bEnable);
}

static VOID _drv_errdet_clr(VOID)
{
	HAL_ToggleBits_Ex(REG_HDMI20_CONTROL, 1UL << 15, TRUE);
	HAL_ToggleBits_Ex(REG_HDMI20_CONTROL, 1UL << 15, FALSE);
}

VOID ms7200drv_hdmi_rx_pi_phy_eq_gain_set(e_RxChannel eChannel, UINT8 u8EqGainVal)
{
	switch (eChannel) {
	case HDMI_RX_CH0:
		HAL_ModBits(REG_PI_RX_GAIN_CTRL1, MSRT_BITS2_0, u8EqGainVal);
		break;
	case HDMI_RX_CH1:
		HAL_ModBits(REG_PI_RX_GAIN_CTRL1, MSRT_BITS6_4, u8EqGainVal << 4);
		break;
	case HDMI_RX_CH2:
		HAL_ModBits(REG_PI_RX_GAIN_CTRL2, MSRT_BITS2_0, u8EqGainVal);
		break;
	default:
		break;
	}
}

VOID ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(UINT8 eChannel, UINT8 u8EqGainVal)
{
	switch (eChannel) {
	case HDMI_RX_CH0:
		HAL_WriteByte(REG_PI_RX_CTRL5, (ptEqGainVal[u8EqGainVal].u8Eq1GainVal << 4) | ptEqGainVal[u8EqGainVal].u8Eq0GainVal);
		HAL_WriteByte(REG_PI_RX_CTRL6, (ptEqGainVal[u8EqGainVal].u8Eq3GainVal << 4) | ptEqGainVal[u8EqGainVal].u8Eq2GainVal);
		break;
	case HDMI_RX_CH1:
		HAL_WriteByte(REG_PI_RX_CTRL7, (ptEqGainVal[u8EqGainVal].u8Eq1GainVal << 4) | ptEqGainVal[u8EqGainVal].u8Eq0GainVal);
		HAL_WriteByte(REG_PI_RX_CTRL8, (ptEqGainVal[u8EqGainVal].u8Eq3GainVal << 4) | ptEqGainVal[u8EqGainVal].u8Eq2GainVal);
		break;
	case HDMI_RX_CH2:
		HAL_WriteByte(REG_PI_RX_CTRL9, (ptEqGainVal[u8EqGainVal].u8Eq1GainVal << 4) | ptEqGainVal[u8EqGainVal].u8Eq0GainVal);
		HAL_WriteByte(REG_PI_RX_CTRL10, (ptEqGainVal[u8EqGainVal].u8Eq3GainVal << 4) | ptEqGainVal[u8EqGainVal].u8Eq2GainVal);
		break;
	default:
		break;
	}
}

static BOOL _drv_errdet_counter(UINT16 *u16ErrCnt)
{
	UINT8 u8Channel;
	UINT8 u8_flag = 0;
	u16ErrCnt[HDMI_RX_CH0] = HAL_ReadDWord_Ex(REG_SCDC_REGS1) >> 16;
	u16ErrCnt[HDMI_RX_CH1] = HAL_ReadDWord_Ex(REG_SCDC_REGS2);
	u16ErrCnt[HDMI_RX_CH2] = HAL_ReadDWord_Ex(REG_SCDC_REGS2) >> 16;
	for (u8Channel = HDMI_RX_CH0; u8Channel <= 2; u8Channel++) {
		u8_flag <<= 1;
		if (u16ErrCnt[u8Channel] >> 15)
			u16ErrCnt[u8Channel] &= 0x7fff;
		else {
			u16ErrCnt[u8Channel] = 0x7fff;
			u8_flag |= 1;
		}
	}

	//chip bug, if dvi input, return FALSE
	if (u8_flag == 0x7)
		return FALSE;
	else
		return TRUE;
}

BOOL ms7200drv_errdet_wrong_status(UINT16 u16_error_val)
{
	UINT8 b_valid;
	UINT8 u8Channel;
	UINT16 u16ErrCnt[3];
	b_valid = _drv_errdet_counter(u16ErrCnt);
	_drv_errdet_clr();

	if (!b_valid)
		return FALSE;

	for (u8Channel = HDMI_RX_CH0; u8Channel <= 2; u8Channel++) {
#if VIN_FALSE
		if (u16ErrCnt[u8Channel] >= 0x7fff) {
			return TRUE;
		}
#else
		if (u16ErrCnt[u8Channel] >= u16_error_val) {
			//MS7200_LOG1("u16ErrCnt = ", u16ErrCnt[u8Channel]);
			return TRUE;
		}
#endif
	}

	return FALSE;
}

BOOL ms7200drv_rxpll_lock_status(VOID)
{
	//lock or unlock interruput be got
	if (HAL_ReadDWord_Ex(REG_HDMI_ISTS) & 0x30) {
		HAL_WriteDWord_Ex(REG_HDMI_ICLR, 0x30);
		return FALSE;
	}

	return (HAL_ReadDWord_Ex(REG_HDMI_CKM_RESULT) >> 16) & MSRT_BIT0;
}

static VOID _drv_hdmi_rx_pi_cdr_reset(VOID)
{
	HAL_WriteByte(REG_PI_CDR_RST, 0x00);
	HAL_WriteByte(REG_PI_CDR_RST, 0x07);
}

static VOID _drv_hdmi_rx_pi_fifo_reset(VOID)
{
	HAL_WriteByte(REG_PI_FIFO_RST, MSRT_BIT0);
	HAL_WriteByte(REG_PI_FIFO_RST, 0x00);
}

BOOL ms7200drv_hdmi_rx_pi_eq_config(UINT8 u8_eq_gain)
{
	BOOL b_flag = TRUE;
	UINT8 u8Channel;
	UINT8 u8EqGainVal;
	UINT16 u16ErrCnt[3][8];
	UINT16 u16ErrCntTemp[3];
	UINT8 u8ErrMinBand[9];
	UINT16 u16ErrMinGain;

	if (u8_eq_gain != 0xFF)
		goto EQ_MANUAL;

	for (u8EqGainVal = 2; u8EqGainVal < 8; u8EqGainVal++) {
		for (u8Channel = HDMI_RX_CH0; u8Channel <= HDMI_RX_CH2; u8Channel++)
			ms7200drv_hdmi_rx_pi_phy_eq_gain_set((e_RxChannel)u8Channel, u8EqGainVal);
		_drv_hdmi_rx_pi_fifo_reset();
		Delay_us(100);
		_drv_errdet_clr();
		Delay_ms(3);
		_drv_errdet_counter(u16ErrCntTemp);
		for (u8Channel = HDMI_RX_CH0; u8Channel <= 2; u8Channel++)
			u16ErrCnt[u8Channel][u8EqGainVal] = u16ErrCntTemp[u8Channel];
	}

	for (u8Channel = HDMI_RX_CH0; u8Channel <= HDMI_RX_CH2; u8Channel++) {
#if RXPHY_DEBUG_LOG
		MS7200_LOG2("u8Channel = ", u8Channel);
#endif
		u16ErrMinGain = 0xFFFF;
		u8ErrMinBand[0] = 0;   //min error count gain number
		for (u8EqGainVal = 2; u8EqGainVal < 8; u8EqGainVal++) {
#if RXPHY_DEBUG_LOG
			MS7200_LOG2("u8EqGainVal = ", u8EqGainVal);
			MS7200_LOG1("u16ErrCnt = ", u16ErrCnt[u8Channel][u8EqGainVal]);
#endif

			if (u16ErrCnt[u8Channel][u8EqGainVal] < u16ErrMinGain) {
				u16ErrMinGain = u16ErrCnt[u8Channel][u8EqGainVal];
				u8ErrMinBand[0] = 1;
				u8ErrMinBand[u8ErrMinBand[0]] = u8EqGainVal;
			} else if (u16ErrCnt[u8Channel][u8EqGainVal] == u16ErrMinGain) {
				++u8ErrMinBand[0];
				u8ErrMinBand[u8ErrMinBand[0]] = u8EqGainVal;
			}
		}

		if (u16ErrMinGain > 0x100) {
			u8EqGainVal = 4;
			b_flag = FALSE;
		} else if (u8ErrMinBand[0] % 2)
			u8EqGainVal = u8ErrMinBand[(u8ErrMinBand[0] + 1) / 2];
		else
			u8EqGainVal = u8ErrMinBand[u8ErrMinBand[0] / 2];

#if RXPHY_DEBUG_LOG
		MS7200_LOG2("min err = ", u16ErrMinGain);
		MS7200_LOG2("number of min err gain = ", u8ErrMinBand[0]);
		MS7200_LOG2("select eq gain = ", u8EqGainVal);
#endif
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set((e_RxChannel)u8Channel, u8EqGainVal);
	}

EQ_MANUAL:
	if (u8_eq_gain < 0xFE) {
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(HDMI_RX_CH0, u8_eq_gain);
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(HDMI_RX_CH1, u8_eq_gain);
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(HDMI_RX_CH2, u8_eq_gain);
	}
	//
	_drv_hdmi_rx_pi_fifo_reset();
	Delay_us(100);
	_drv_errdet_clr();

	return b_flag;
}

static VOID _drv_hdmi_rx_pi_28eq_set_and_get_error(UINT8 u8GainIndex, UINT16 *p_u16ErrCntTemp, UINT8 u8_delay_ms)
{
	ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(0, u8GainIndex);
	ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(1, u8GainIndex);
	ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(2, u8GainIndex);

	_drv_hdmi_rx_pi_cdr_reset();
	Delay_ms(2);
	_drv_hdmi_rx_pi_fifo_reset();
	Delay_us(100);

	_drv_errdet_clr();
	Delay_ms(u8_delay_ms);
	_drv_errdet_counter(p_u16ErrCntTemp);
}

static UINT8 _drv_hdmi_rx_pi_28eq_search(UINT8 u8_search_st, UINT8 u8_search_sp, UINT8 *u8_gain_buf)
{
	UINT8 u8_flag = 0x02;
	BOOL bChangeNumFlag = FALSE;
	UINT8 u8Channel;
	UINT8 u8GainIndex, u8GainNum = g_u8EqGainNum;
	UINT16 u16ErrCnt[3][28];
	UINT16 u16ErrCntTemp[3];
	UINT8 u8ErrMinBand[29];
	UINT16 u16ErrMinGain;

	//pre judge
	u8_search_st %= g_u8EqGainNum;
	u8_search_sp %= g_u8EqGainNum;

	//
	if (u8_search_st == 0 && u8_search_sp == (g_u8EqGainNum - 1)) {
		u8_search_st = 10;
		for (u8GainIndex = 0; u8GainIndex < u8_search_st; u8GainIndex++) {
			_drv_hdmi_rx_pi_28eq_set_and_get_error(u8GainIndex, u16ErrCntTemp, 10);

			if (u16ErrCntTemp[0] < 0x7fff || u16ErrCntTemp[1] < 0x7fff || u16ErrCntTemp[2] < 0x7fff) {
				u8_search_st = u8GainIndex;
				break;
			}
		}

		u8_search_sp = g_u8EqGainNum - 11;
		for (u8GainIndex = (g_u8EqGainNum - 1); u8GainIndex > u8_search_sp; u8GainIndex--) {
			_drv_hdmi_rx_pi_28eq_set_and_get_error(u8GainIndex, u16ErrCntTemp, 10);
			if (u16ErrCntTemp[0] < 0x7fff || u16ErrCntTemp[1] < 0x7fff || u16ErrCntTemp[2] < 0x7fff) {
				u8_search_sp = u8GainIndex;
				break;
			}
		}
	}

	//MS7200_LOG2("u8_search_st = ", u8_search_st);
	//MS7200_LOG2("u8_search_sp = ", u8_search_sp);

	u8GainNum = u8_search_sp;

	for (u8GainIndex = u8_search_st; u8GainIndex <= u8GainNum; u8GainIndex++) {
		_drv_hdmi_rx_pi_28eq_set_and_get_error(u8GainIndex, u16ErrCntTemp, 50);

		for (u8Channel = HDMI_RX_CH0; u8Channel <= 2; u8Channel++)
			u16ErrCnt[u8Channel][u8GainIndex] = u16ErrCntTemp[u8Channel];

		if (!bChangeNumFlag) {
			if (u16ErrCntTemp[0] < 0x20 \
				&& u16ErrCntTemp[1] < 0x20 \
				&& u16ErrCntTemp[2] < 0x20) {
				bChangeNumFlag = TRUE;
				u8GainNum = u8GainIndex + 9;
				u8GainNum = (u8GainNum > u8_search_sp) ? u8_search_sp : u8GainNum;
			}
		}
	}

	for (u8Channel = HDMI_RX_CH0; u8Channel <= HDMI_RX_CH2; u8Channel++) {
#if VIN_TRUE
		MS7200_LOG2("u8Channel = ", u8Channel);
#endif
		u16ErrMinGain = 0xFFFF;
		u8ErrMinBand[0] = 0;   //min error count gain number
		for (u8GainIndex = u8_search_st; u8GainIndex <= u8GainNum; u8GainIndex++) {
#if VIN_FALSE
			MS7200_LOG2("u8EqGainVal = ", u8GainIndex);
			MS7200_LOG1("u16ErrCnt = ", u16ErrCnt[u8Channel][u8GainIndex]);
#endif

			if (u16ErrCnt[u8Channel][u8GainIndex] < u16ErrMinGain) {
				u16ErrMinGain = u16ErrCnt[u8Channel][u8GainIndex];
				u8ErrMinBand[0] = 1;
				u8ErrMinBand[u8ErrMinBand[0]] = u8GainIndex;
			} else if (u16ErrCnt[u8Channel][u8GainIndex] == u16ErrMinGain) {
				++u8ErrMinBand[0];
				u8ErrMinBand[u8ErrMinBand[0]] = u8GainIndex;
			}
		}

		//remove default
		if (u16ErrMinGain > 0x500)
			u8_flag = 0;

		if (u8ErrMinBand[0] % 2)
			u8GainIndex = u8ErrMinBand[(u8ErrMinBand[0] + 1) / 2];
		else
			u8GainIndex = u8ErrMinBand[u8ErrMinBand[0] / 2];

		MS7200_LOG2("min err = ", u16ErrMinGain);
		//MS7200_LOG2("number of min err gain = ", u8ErrMinBand[0]);
		MS7200_LOG2("select eq gain = ", u8GainIndex);

		u8_gain_buf[u8Channel] = u8GainIndex;
	}

	MS7200_LOG("");

	return u8_flag;
}

static VOID _drv_hdmi_rx_pi_28eq_calc_next_search_range(UINT8 *p_u8_gain_buf, UINT8 *p_u8_search_st, UINT8 *p_u8_search_sp)
{
	UINT8 a;
	UINT8 b;
	UINT8 c;
	UINT8 max;
	UINT8 min;

	a = p_u8_gain_buf[0];
	b = p_u8_gain_buf[1];
	c = p_u8_gain_buf[2];

	min = a;
	max = a;

	if (min > b)
		min = b;

	if (min > c)
		min = c;

	if (max < b)
		max = b;

	if (max < c)
		max = c;

	min = (min > 4) ? (min - 4) : 0;
	max = (max < (g_u8EqGainNum - 5)) ? (max + 4) : (g_u8EqGainNum - 1);

	(*p_u8_search_st) = min;
	(*p_u8_search_sp) = max;
}

UINT8 ms7200drv_hdmi_rx_pi_28eq_config(UINT8 u8_eq_gain)
{
	UINT8 u8_flag = 0x01;
	UINT8 i;
	UINT8 u8_gain_buf[3][3];
	UINT8 u8_gain;
	UINT8 u8_search_st;
	UINT8 u8_search_sp;

	if (u8_eq_gain != 0xFF)
		goto EQ_MANUAL;

	u8_search_st = 0;
	u8_search_sp = g_u8EqGainNum - 1;

	for (i = 0; i < 3; i++) {
		u8_flag |= _drv_hdmi_rx_pi_28eq_search(u8_search_st, u8_search_sp, u8_gain_buf[i]);

		if (!ms7200drv_rxpll_lock_status()) {
			u8_flag = 0x00;
			ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(0, 13);
			ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(1, 13);
			ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(2, 13);
			goto EQ_MANUAL;
		}

		if (i == 0)
			_drv_hdmi_rx_pi_28eq_calc_next_search_range(u8_gain_buf[i], &u8_search_st, &u8_search_sp);
		else if (i == 1) {
			if (u8_gain_buf[0][0] == u8_gain_buf[1][0] &&
				u8_gain_buf[0][1] == u8_gain_buf[1][1] &&
				u8_gain_buf[0][2] == u8_gain_buf[1][2])
				break;
		}
	}

	for (i = 0; i < 3; i++) {
		if (u8_gain_buf[0][i] == u8_gain_buf[1][i])
			u8_gain = u8_gain_buf[0][i];
		else if (u8_gain_buf[0][i] == u8_gain_buf[2][i])
			u8_gain = u8_gain_buf[0][i];
		else if (u8_gain_buf[1][i] == u8_gain_buf[2][i])
			u8_gain = u8_gain_buf[1][i];
		else {
			u8_gain = u8_gain_buf[0][i] + u8_gain_buf[1][i] + u8_gain_buf[2][i];
			u8_gain /= 3;
		}

		MS7200_LOG2("chn = ", i);
		MS7200_LOG2("u8_gain = ", u8_gain);
		ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(i, u8_gain);
	}

	MS7200_LOG("");

EQ_MANUAL:
	if (u8_eq_gain != 0xFF) {
		u8_eq_gain %= g_u8EqGainNum;
		ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(0, u8_eq_gain);
		ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(1, u8_eq_gain);
		ms7200drv_hdmi_rx_pi_phy_28eq_gain_set(2, u8_eq_gain);
	}

	//
	_drv_hdmi_rx_pi_cdr_reset();
	Delay_ms(2);
	_drv_hdmi_rx_pi_fifo_reset();
	Delay_us(100);
	_drv_errdet_clr();

	if (u8_eq_gain != 0xFF)
		if (!ms7200drv_rxpll_lock_status())
			u8_flag = 0x00;

	return u8_flag;
}


VOID ms7200drv_hdmi_rx_pi_pll_release(BOOL bEnable)
{
	HAL_ToggleBits(REG_MISC_CLK_CTRL0, 0x83, bEnable);
	HAL_ToggleBits(REG_MISC_RST_CTRL0, MSRT_BIT5, bEnable);
}

static VOID _drv_hdmi_rx_pi_pll_kband_enable(BOOL bEnable)
{
	if (bEnable)
		HAL_ToggleBits(REG_PI_RXPLL_CTRL0, MSRT_BIT0, !bEnable);
	HAL_ToggleBits(REG_PI_RXPLL_CTRL4, MSRT_BIT2, bEnable);
	HAL_ToggleBits(REG_PI_RXPLL_CTRL1, MSRT_BIT0, bEnable);
	Delay_us(10);
	HAL_ToggleBits(REG_PI_RXPLL_CTRL3, MSRT_BIT7, bEnable);
}

VOID ms7200drv_hdmi_rx_pi_rxpll_mode_set(e_RxPhyMode eMode)
{
	HAL_ToggleBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BITS2_0, eMode);
}

VOID ms7200drv_hdmi_rx_pi_rxpll_trigger(VOID)
{
	HAL_WriteByte(REG_PI_RX_PLL_CAL_TRIG, MSRT_BIT0);
}

BOOL ms7200drv_hdmi_rx_pi_rxpll_done_flag(VOID)
{
	return (HAL_ReadByte(REG_PI_RXPLL_STS) & MSRT_BIT0);
}

BOOL ms7200drv_hdmi_rx_pi_rxpll_lock_flag(VOID)
{
	return ((HAL_ReadByte(REG_PI_RXPLL_STS) & MSRT_BIT1) >> 1);
}

#if VIN_FALSE
static UINT16 _drv_hdmi_rx_pi_rxpll_tmdsfreq_get(VOID)
{
	return (HAL_ReadWord(REG_PI_TMDS_FREQ) & 0x3fff);
}
#endif

static VOID _drv_hdmi_rx_pi_rxpll_lookup_table(UINT16 u16TmdsClk)
{
	UINT8 u8Gear = (u16TmdsClk > 50) ? ((u16TmdsClk > 100) ? 2 : 1) : 0;
	switch (u8Gear) {
	case 0:
		HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x03);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x06 << 4);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		break;
	case 1:
		HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x01);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x02 << 4);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		break;
	case 2:
		HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x00);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x02);
		HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x06 << 4);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		break;
	default:
		break;
	}
}

static UINT16 _drv_hdmi_rx_pi_rxpll_cal_ref_freq_get(VOID)
{
	return (HAL_ReadWord(REG_PI_CAL_REF_FREQ) & 0x3fff);
}

static UINT16 _drv_hdmi_rx_pi_rxpll_iband(UINT8 u8Iband)
{
	HAL_ModBits(REG_PI_RXPLL_CTRL2, MSRT_BITS5_0, u8Iband);
	HAL_SetBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
	HAL_ClrBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
	Delay_us(30);
	return (HAL_ReadWord(REG_PI_CAL_FREQ) & 0x3fff);
}

static BOOL _drv_hdmi_rx_pi_rxpll_config(e_RxPhyMode eMode, UINT16 u16TmdsClk)
{
	//UINT16 u16TmdsFreq;
	//UINT16 u16TmdsClk;
	UINT16 u16CalRefFreq = 0;
	UINT16 u16CalFreq = 0;
	UINT8 u8Iband = 0;
	INT8 i;
	//UINT8 u8PreDiv;
	UINT8 u8FreqSet;
	UINT8 u8IchpSel;

	//u16TmdsClk /= 100;

	switch (eMode) {
	case AUTO_MODE:
		//RG_RXPLL_KBAND_DIV2_EN: to fix hardware bug
		HAL_ToggleBits(REG_PI_RXPLL_CTRL5, MSRT_BIT2, u16TmdsClk > 5000);

		//increase rxpll bandwidth, to resolve inphic display problem
#if VIN_TRUE
		HAL_ClrBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BIT2);   //look up table auto mode
		//lock flag will be clear after trigger
		ms7200drv_hdmi_rx_pi_rxpll_trigger();
		Delay_ms(1);
#if VIN_FALSE
		u16TmdsFreq = _drv_hdmi_rx_pi_rxpll_tmdsfreq_get();
		MS7200_LOG1("u16TmdsFreq = ", u16TmdsFreq);
		u16TmdsClk = ((UINT32)u16TmdsFreq * (MS7200_EXT_XTAL / 1000) / 256) / 1000;
		MS7200_LOG2("u16TmdsClk = ", u16TmdsClk);
#endif

		//u8PreDiv = (HAL_ReadByte(REG_PI_PLL_CAL_RESULT0) & MSRT_BITS7_6) >> 6;
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, u8PreDiv);
		HAL_WriteByte(REG_PI_RXPLL_CTRL3, 0x14);
		u8FreqSet = (HAL_ReadByte(REG_PI_PLL_CAL_RESULT2) & MSRT_BITS2_1) >> 1;
		//<=50M: 4X clk   <=150M: 2X clk
		if (u16TmdsClk <= 5000)
			u8FreqSet = 0x3;
		else if (u16TmdsClk <= 15000)
			u8FreqSet = 0x01;

		HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, u8FreqSet);

		u8IchpSel = HAL_ReadByte(REG_PI_PLL_CAL_RESULT1) & MSRT_BITS3_0;
		//u8IchpSel = u8IchpSel << ((u8PreDiv != 0) ? 0 : 1);   // >100M: move 1-bit to left
		//20190530, 50~100MHz, ICHP change from 4 to 8.
		//u8IchpSel = (u8IchpSel == 4) ? 8 : u8IchpSel;
		HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, u8IchpSel << 4);
		HAL_WriteByte(REG_PI_CTRL0, 0x02);

		HAL_SetBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BIT2);   //look up table manual mode
#endif

		ms7200drv_hdmi_rx_pi_rxpll_trigger();
		break;

	case MANUAL_MODE:
		_drv_hdmi_rx_pi_pll_kband_enable(TRUE);
		HAL_SetBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
		HAL_ClrBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
		Delay_ms(1);
		u16CalRefFreq = _drv_hdmi_rx_pi_rxpll_cal_ref_freq_get();
		MS7200_LOG1("u16CalRefFreq = ", u16CalRefFreq);
		_drv_hdmi_rx_pi_rxpll_lookup_table(u16TmdsClk / 100);
		HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		do {
			u8Iband += 0x10;
			if (u8Iband == 0x40)
				break;
			u16CalFreq = _drv_hdmi_rx_pi_rxpll_iband(u8Iband);

#if RXPHY_DEBUG_LOG
			MS7200_LOG1("u8Iband = ", u8Iband);
			MS7200_LOG1("u16CalFreq = ", u16CalFreq);
#endif
		} while (u16CalFreq < u16CalRefFreq);
		u8Iband -= 0x10;
		for (i = 3; i >= 0; i--) {
			u8Iband |= (1 << i);
			u16CalFreq = _drv_hdmi_rx_pi_rxpll_iband(u8Iband);

#if RXPHY_DEBUG_LOG
			MS7200_LOG1("u8Iband = ", u8Iband);
			MS7200_LOG1("u16CalFreq = ", u16CalFreq);
#endif

			if (u16CalFreq > u16CalRefFreq)
				u8Iband -= (1 << i);
		}
		u16CalFreq = _drv_hdmi_rx_pi_rxpll_iband(u8Iband);
		MS7200_LOG1("Choose u8Iband = ", u8Iband);
		MS7200_LOG1("u16CalFreq = ", u16CalFreq);

		if (u16TmdsClk > 10000)
			HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x02);
		_drv_hdmi_rx_pi_pll_kband_enable(FALSE);
		break;

	default:
		break;
	}

	//delay for PLL stable
	Delay_ms(1);
	return TRUE;
}

static VOID _drv_hdmi_rx_pi_mixer_config(UINT16 u16TmdsClk)
{
	u16TmdsClk /= 100;
	HAL_WriteByte(REG_RX_PI_CTRL5, 0x00);
	HAL_ClrBits(REG_PI_CDR_RST, MSRT_BITS2_0);
	if (u16TmdsClk <= 75) {
		HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x2 << 4);
		HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		HAL_ModBits(REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x2);
		HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x1 << 4);
		HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x4);
		HAL_ModBits(REG_RX_PI_CTRL4, MSRT_BITS3_0, 0x4);
	} else if (u16TmdsClk <= 150) {
		HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x1 << 4);
		HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		HAL_ModBits(REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x4);
		HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x0 << 4);
		HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x4);
		HAL_ModBits(REG_RX_PI_CTRL4, MSRT_BITS3_0, 0xc);
	} else {
		HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x0 << 4);
		HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		HAL_ModBits(REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x9);
		HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x0 << 4);
		HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x0);
		HAL_ModBits(REG_RX_PI_CTRL4, MSRT_BITS3_0, 0xc);
	}
	HAL_WriteByte(REG_RX_PI_CTRL5, 0x70);
	HAL_SetBits(REG_PI_CDR_RST, MSRT_BITS2_0);
}

BOOL ms7200drv_hdmi_rx_pi_phy_init(VOID)
{
	BOOL bValid = TRUE;
	_drv_hdmi_rx_pi_rcv_release(TRUE);
	bValid &= _drv_hdmi_rx_pi_term_config(AUTO_MODE, 0x80);
	bValid &= _drv_hdmi_rx_pi_offset_config(0x1);
	HAL_WriteByte(REG_PI_RXPLL_CTRL0, 0x20);
	ms7200drv_hdmi_rx_pi_rxpll_mode_set(AUTO_MODE);
	//20180404, rx_bus_clk_en
	HAL_SetBits(REG_MISC_CLK_CTRL0, MSRT_BIT6);

	//20200427, add RXPLL new method for BS5330A...
#if MS7200_RXPLL_METHOD
	HAL_ClrBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BIT7);
#endif

	HAL_SetBits(REG_PI_RX_PHY_INTR, MSRT_BIT1);

	//PI error det enable
	_drv_errdet_read_enable(TRUE);
	_drv_errdet_clr_swsel(TRUE);
	return bValid;
}

VOID ms7200drv_hdmi_rx_phy_power_down(VOID)
{
	//rxpll power down
	HAL_SetBits(REG_PI_RXPLL_CTRL0, MSRT_BITS2_0);
	//pi power down
	HAL_SetBits(REG_RX_PI_CTRL5, MSRT_BITS2_0);
	//audio pll power down
	HAL_ClrBits(REG_AUPLL_PWR, MSRT_BIT1);
	HAL_SetBits(REG_AUPLL_PWR, MSRT_BIT0);
}

UINT8 ms7200drv_hdmi_rx_pi_phy_config(UINT16 u16TmdsClk, UINT8 u8_eq_gain)
{
	UINT8 u8_flag = 0;

	if (u8_eq_gain == 0xFE)
		goto RX_FIFO_RESET;

	HAL_ClrBits(REG_PI_RXPLL_CTRL0, MSRT_BITS2_0);   //power on rxpll

	//20200526, rx_tmds_clk_en = 0.
	//disable rxpll out clk to rx_core when rxpll config.
	HAL_ModBits(REG_MISC_CLK_CTRL0, 0x83, 0x82);
	HAL_SetBits(REG_MISC_RST_CTRL0, MSRT_BIT5);

	Delay_us(1);

	//optimize 1080p config with long line
	if (u16TmdsClk < 20000) {
		HAL_ModBits(REG_PI_RX_GAIN_CTRL2, MSRT_BITS5_4, 0x3 << 4);
		HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS3_2, 0x1 << 2);
	} else {
		HAL_ModBits(REG_PI_RX_GAIN_CTRL2, MSRT_BITS5_4, 0x2 << 4);
		HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS3_2, 0x0 << 2);
	}
	HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS7_6, 0x3 << 6);

	_drv_hdmi_rx_pi_rxpll_config(AUTO_MODE, u16TmdsClk);

	_drv_hdmi_rx_pi_mixer_config(u16TmdsClk);

	//20200526, rx_tmds_clk_en = 1
	HAL_SetBits(REG_MISC_CLK_CTRL0, 0x01);

	Delay_ms(1);
	//clr rxpll unlock.
	HAL_WriteDWord_Ex(REG_HDMI_ICLR, 0x30);

RX_FIFO_RESET:
	if (ms7200drv_hdmi_rx_pi_eq_config(u8_eq_gain))
		u8_flag |= 0x02;

	//get rxpll lock again
	if (ms7200drv_rxpll_lock_status())
		u8_flag |= 0x01;

	return u8_flag;
}

UINT8 ms7200drv_hdmi_rx_pi_phy_config_with_28eq(UINT16 u16TmdsClk, UINT8 u8_eq_gain)
{
	UINT8 u8_flag = 0;

	//20200303,  eq new method for as5332a/as5338 only
	HAL_ClrBits(REG_PI_RX_EQ_SEL, MSRT_BIT0);

	HAL_ClrBits(REG_PI_RXPLL_CTRL0, MSRT_BITS2_0);   //power on rxpll

	//20200526, rx_tmds_clk_en = 0.
	//disable rxpll out clk to rx_core when rxpll config.
	HAL_ModBits(REG_MISC_CLK_CTRL0, 0x83, 0x82);
	HAL_SetBits(REG_MISC_RST_CTRL0, MSRT_BIT5);

	Delay_us(1);

	HAL_SetBits(0x1007, 0x80);
	HAL_WriteByte(0x1008, 0x63);

	//optimize 1080p config with long line
	if (u16TmdsClk < 20000) {
		HAL_ModBits(REG_PI_RX_CTRL11, MSRT_BITS5_4, 0x2 << 4);
		HAL_ModBits(REG_PI_RX_CTRL11, MSRT_BITS3_0, 0x0);
		HAL_WriteByte(REG_PI_RX_CTRL3, 0x55);
		HAL_WriteByte(REG_PI_RX_CTRL4, 0x55);
	} else {
		HAL_ModBits(REG_PI_RX_CTRL11, MSRT_BITS5_4, 0x0 << 4);
		HAL_ModBits(REG_PI_RX_CTRL11, MSRT_BITS3_0, 0x0);
		HAL_WriteByte(REG_PI_RX_CTRL3, 0x11);
		HAL_WriteByte(REG_PI_RX_CTRL4, 0x11);
	}
	HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS7_6, 0x3 << 6);

	_drv_hdmi_rx_pi_rxpll_config(AUTO_MODE, u16TmdsClk);

	HAL_ModBits(REG_PI_CTRL1, MSRT_BITS6_4, 0x1 << 4);
	HAL_SetBits(REG_PI_S2P_SKEW_CTRL, 0x80);

	_drv_hdmi_rx_pi_mixer_config(u16TmdsClk);

	//20200526, rx_tmds_clk_en = 1
	HAL_SetBits(REG_MISC_CLK_CTRL0, 0x01);

	Delay_ms(1);
	//clr rxpll unlock.
	HAL_WriteDWord_Ex(REG_HDMI_ICLR, 0x30);

	u8_flag = ms7200drv_hdmi_rx_pi_28eq_config(u8_eq_gain);

	return u8_flag;
}


VOID ms7200drv_hdmi_rx_controller_hpd_set(BOOL bReady)
{
#if MS7200_HDMI_RX_EDID_ENABLE
	//hdmi rx EDID reset release
	ms7200drv_hdmi_rx_edid_reset_release(bReady);
#endif
	HAL_ToggleBits_Ex(REG_HDMI_SETUP_CTRL, MSRT_BIT0, bReady);
}

BOOL ms7200drv_hdmi_rx_controller_hpd_get(VOID)
{
	return HAL_ReadDWord_Ex(REG_HDMI_SETUP_CTRL) & MSRT_BIT0;
}

VOID ms7200drv_hdmi_rx_controller_hdcp_encryption_enable(BOOL bEnable)
{
	//20181127, force OESS. resolve RX HDCP working status can't be get issue.
	//HAL_ModBits_Ex(REG_HDMI_MODE_RECOVER, MSRT_BITS3_2, 0x01 << 2);
	HAL_ToggleBits_Ex(REG_HDCP_CTRL, MSRT_BIT0, bEnable);
}


VOID ms7200drv_hdmi_rx_hdcp_detect_enable(BOOL bEnable)
{
	ms7200drv_hdmi_rx_controller_hdcp_data_enable(bEnable);

	//20200831, new design for HDMI video reset only. resolve HDMI RX error counter maybe working abnormity
	if (!bEnable)
		ms7200drv_hdmi_rx_controller_reset(HDMI_RX_CTRL_HDCP);
}


VOID ms7200drv_hdmi_rx_controller_hdcp_hdmi_mode_set(UINT8 bHdmiMode)
{
	//HAL_ModBits_Ex(REG_HDCP_CTRL, MSRT_BIT2, bHdmiMode << 2);
	//HAL_ModBits_Ex(REG_HDCP_CTRL, 0xc00, 0x400);
}

VOID ms7200drv_hdmi_rx_controller_hdcp_bksv_set(UINT8 *u8RxKsv)
{
	HAL_WriteDWord_Ex(REG_HDCP_BKSV1, (UINT32)u8RxKsv[0]);
	HAL_WriteDWord_Ex(REG_HDCP_BKSV0, ((UINT32)u8RxKsv[1] << 24)
		| ((UINT32)u8RxKsv[2] << 16) | ((UINT32)u8RxKsv[3] << 8) | (UINT32)u8RxKsv[4]);
}

VOID ms7200drv_hdmi_rx_controller_hdcp_key_set(UINT8 *u8RxKey)
{
	UINT8 i;
	UINT16 u16KeyNum;
	for (i = 0; i < 40; i++) {
		u16KeyNum = 7 * i;
		HAL_WriteDWord_Ex(REG_HDCP_KEY1, ((UINT32)u8RxKey[u16KeyNum + 0] << 16)
			| ((UINT32)u8RxKey[u16KeyNum + 1] << 8) | (UINT32)u8RxKey[u16KeyNum + 2]);
		HAL_WriteDWord_Ex(REG_HDCP_KEY0, ((UINT32)u8RxKey[u16KeyNum + 3] << 24)
			| ((UINT32)u8RxKey[u16KeyNum + 4] << 16) | ((UINT32)u8RxKey[u16KeyNum + 5] << 8)
			| (UINT32)u8RxKey[u16KeyNum + 6]);
	}
}

VOID ms7200drv_hdmi_rx_controller_hdcp_init(UINT8 *u8RxKsv, UINT8 *u8RxKey)
{
	ms7200drv_hdmi_rx_controller_hdcp_bksv_set(u8RxKsv);
	ms7200drv_hdmi_rx_controller_hdcp_key_set(u8RxKey);
}

UINT8 ms7200drv_hdmi_rx_controller_hdcp_get_status(VOID)
{
	return ((HAL_ReadDWord_Ex(REG_HDCP_STS) >> 9) & 0x01);
}

UINT16 ms7200drv_hdmi_rx_controller_hdcp_get_Ri(VOID)
{
	return (HAL_ReadDWord_Ex(REG_HDCP_DBG) & 0xffff);
}


BOOL ms7200drv_hdmi_rx_controller_mdt_syncvalid(VOID)
{
	return (HAL_ReadDWord_Ex(REG_MD_STS) & 0x1107) == 0x1107;
}

static e_RxMdtPol _drv_mdt_get_sync_polarity(VOID)
{
	UINT8 Polarity;
	UINT8 temp = (HAL_ReadDWord_Ex(REG_HDMI_STS) >> 7) & MSRT_BITS2_1;
	Polarity = temp | (((~HAL_ReadDWord_Ex(REG_MD_STS)) >> 3) & MSRT_BIT0);
	return (e_RxMdtPol)Polarity;
}

static UINT16 _drv_mdt_get_htotal(VOID)
{
	return HAL_ReadDWord_Ex(REG_MD_HT1) >> 16;
}

static UINT16 _drv_mdt_get_vtotal(VOID)
{
	return HAL_ReadDWord_Ex(REG_MD_VTL) & 0xffff;
}

static UINT16 _drv_mdt_get_hactive(VOID)
{
	return HAL_ReadDWord_Ex(REG_MD_HACT_PX) & 0xffff;
}

static UINT16 _drv_mdt_get_vactive(VOID)
{
	return HAL_ReadDWord_Ex(REG_MD_VAL) & 0xffff;
}

static UINT32 _drv_mdt_get_hfreq(VOID)
{
	UINT16 u16_ht = HAL_ReadDWord_Ex(REG_MD_HT0) >> 16;

	return u16_ht ? (16 * MS7200_EXT_XTAL / u16_ht) : 0;
}

UINT16 ms7200drv_hdmi_rx_get_tmds_clk(VOID)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_HDMI_CKM_RESULT);

	//clk stable detect. detect 200us once time
	if ((u32_value >> 17) & MSRT_BIT0) {
		u32_value &= 0xffff;
		return (UINT16)(u32_value * (MS7200_EXT_XTAL / 10000) / 0xfff);
	}

	return 0;
}

static UINT16 _drv_mdt_get_hoffset(VOID)
{
	return HAL_ReadDWord_Ex(REG_MD_HT1) & 0xffff;
}

static UINT16 _drv_mdt_get_vback(VOID)
{
	//use top misc mdt
	return HAL_ReadWord(REG_MISC_MD_VBACK_L);
}

static UINT16 _drv_mdt_get_hsw(VOID)
{
	//use top misc mdt
	return HAL_ReadWord(REG_MISC_MD_HSW_L);
}

static UINT16 _drv_mdt_get_vsw(VOID)
{
	//use top misc mdt
	return HAL_ReadWord(REG_MISC_MD_VSW_L);
}

#define MDT_VALID_ISTS ((UINT16)(~(MDT_VTOT_CLK_ISTS | MDT_HTOT32_CLK_ISTS | MDT_VS_CLK_ISTS)))

UINT16 ms7200drv_hdmi_rx_get_mdt_interrupt_status(UINT16 u16_mask)
{
	UINT16 u16_value = HAL_ReadDWord_Ex(REG_MD_ISTS) & MDT_VALID_ISTS;

	//clr bit
	if (u16_value & u16_mask)
		HAL_WriteDWord_Ex(REG_MD_ICLR, u16_value & u16_mask);

	return (u16_value & u16_mask);
}

UINT16 ms7200drv_hdmi_rx_get_mdt_interrupt_status_ext(UINT16 u16_mask)
{
	return HAL_ReadDWord_Ex(REG_MD_ISTS) & MDT_VALID_ISTS & u16_mask;
}

BOOL ms7200drv_hdmi_rx_get_input_timing(VIDEOTIMING_T *ptTiming)
{
	BOOL bValid = FALSE;

	if (ms7200drv_hdmi_rx_controller_mdt_syncvalid()) {
		ptTiming->u8_polarity = _drv_mdt_get_sync_polarity();
		ptTiming->u16_htotal = _drv_mdt_get_htotal();
		ptTiming->u16_vtotal = _drv_mdt_get_vtotal();
		ptTiming->u16_hactive = _drv_mdt_get_hactive();
		ptTiming->u16_vactive = _drv_mdt_get_vactive();
		ptTiming->u16_pixclk = ms7200drv_hdmi_rx_get_tmds_clk();
		// 20141029 Domain judge
		if (ptTiming->u16_htotal < 50 \
			|| ptTiming->u16_vtotal < 50 \
			|| ptTiming->u16_hactive < 50 \
			|| ptTiming->u16_vactive < 50 \
			|| ptTiming->u16_htotal < ptTiming->u16_hactive \
			|| ptTiming->u16_vtotal < ptTiming->u16_vactive) {
			MS7200_LOG("mdt error.");
			return FALSE;
		}

		if (!(ptTiming->u8_polarity & MSRT_BIT0)) {
			if (ptTiming->u16_vtotal % 2) {
				if (ptTiming->u16_vtotal == 625)	//1920x1080i (1250 Total) @ 50 Hz (Format 39)
					ptTiming->u16_vtotal = 2 * ptTiming->u16_vtotal;
				else
					ptTiming->u16_vtotal = 2 * ptTiming->u16_vtotal - 1;
			} else
				ptTiming->u16_vtotal = 2 * ptTiming->u16_vtotal + 1;
		}

		ptTiming->u16_vfreq = 100 * _drv_mdt_get_hfreq() / ptTiming->u16_vtotal;
		if (!(ptTiming->u8_polarity & MSRT_BIT0)) {
			ptTiming->u16_vfreq <<= 1;
			ptTiming->u16_vactive <<= 1;
		}

		if (ptTiming->u16_vfreq < 500) {
			MS7200_LOG("mdt error1.");
			return FALSE;
		}

		ptTiming->u16_hoffset = _drv_mdt_get_hoffset();
		ptTiming->u16_voffset = _drv_mdt_get_vback();
		ptTiming->u16_hsyncwidth = _drv_mdt_get_hsw();
		ptTiming->u16_vsyncwidth = _drv_mdt_get_vsw();
		//
		ptTiming->u16_voffset += ptTiming->u16_vsyncwidth;

		bValid = TRUE;
	}

	return bValid;
}

BOOL ms7200drv_hdmi_rx_controller_dvidet(VOID)
{
	return (HAL_ReadDWord_Ex(REG_PDEC_STS) >> 28) & MSRT_BIT0;
}

VOID ms7200drv_hdmi_rx_controller_reset(UINT32 eModule)
{
	HAL_WriteDWord_Ex(REG_SW_RST, eModule);
}

VOID ms7200drv_hdmi_rx_controller_enable(UINT32 eModule, BOOL bEnable)
{
	HAL_ToggleBits_Ex(REG_DMI_DISABLE_IF, eModule, bEnable);
}

UINT8 ms7200drv_hdmi_rx_get_audio_fifo_status(VOID)
{
	return HAL_ReadDWord_Ex(REG_AUD_FIFO_STS) & 0x1F;
}

VOID ms7200drv_hdmi_rx_audio_fifo_reset(BOOL b_reset)
{
	HAL_ToggleBits_Ex(REG_AUD_FIFO_CTRL, MSRT_BIT0, b_reset);

	// toggle aupll_ato_cfg_en, I want to reset pll ACR
	HAL_ToggleBits(REG_AUPLL_CFG_CTRL, MSRT_BIT0, !b_reset);
}

VOID ms7200drv_hdmi_rx_video_fifo_reset(BOOL b_reset)
{
	HAL_ToggleBits_Ex(MS7200_HDMI_RX_CEA_VIDEO_REG, 0x80000000UL, b_reset);
}

static UINT8 _drv_pdec_get_vic(VOID)
{
	return (HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 24) & MSRT_BITS6_0;
}

static HDMI_CLK_RPT_E _drv_pdec_get_clk_rpt(VOID)
{
	return (HDMI_CLK_RPT_E)((HAL_ReadDWord_Ex(REG_PDEC_AVI_HB) >> 24) & MSRT_BITS3_0);
}

static HDMI_SCAN_INFO_E _drv_pdec_get_scan_info(VOID)
{
	UINT8 u8ScanInfo = HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) & MSRT_BITS1_0;
	return (HDMI_SCAN_INFO_E)(u8ScanInfo);
}

static HDMI_ASPECT_RATIO_E _drv_pdec_get_aspect_ratio(VOID)
{
	UINT8 u8AspectRatio = (HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 12) & MSRT_BITS1_0;
	return (HDMI_ASPECT_RATIO_E)(u8AspectRatio);
}

static HDMI_CS_E _drv_pdec_get_color_space(VOID)
{
	return (HDMI_CS_E)((HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 5) & MSRT_BITS1_0);
}

#if VIN_FALSE
static HDMI_COLOR_DEPTH_E _drv_pdec_get_color_depth(VOID)
{
	UINT8 u8ColorDepth = ((HAL_ReadDWord_Ex(REG_PDEC_GCP_AVMUTE) >> 4) & MSRT_BITS3_0);
	if (u8ColorDepth == 4)
		u8ColorDepth = HDMI_COLOR_DEPTH_8BIT;
	else if (u8ColorDepth == 5)
		u8ColorDepth = HDMI_COLOR_DEPTH_10BIT;
	else if (u8ColorDepth == 6)
		u8ColorDepth = HDMI_COLOR_DEPTH_12BIT;
	else if (u8ColorDepth == 7)
		u8ColorDepth = HDMI_COLOR_DEPTH_16BIT;
	else
		u8ColorDepth = HDMI_COLOR_DEPTH_8BIT;
	return (HDMI_COLOR_DEPTH_E)u8ColorDepth;
}
#endif

BOOL ms7200drv_hdmi_rx_controller_get_gcp_avmute(VOID)
{
	return (BOOL)(HAL_ReadDWord_Ex(REG_PDEC_GCP_AVMUTE) & MSRT_BIT1);
}

VOID ms7200drv_hdmi_rx_controller_set_avmute_black_color(UINT8 u8_cs)
{
	UINT16 ub = 0;
	UINT16 yg = 0;
	UINT16 vr = 0;

	if (u8_cs == HDMI_YCBCR422) {
		ub = 0;
		yg = 0;
		vr = 0x8000;
	} else if (u8_cs == HDMI_YCBCR444) { //yuv444
		ub = 0x8000;
		yg = 0;
		vr = 0x8000;
	} else if (u8_cs == HDMI_YUV420) { //yuv420
		ub = 0x8000;
		yg = 0;
		vr = 0;
	}

	HAL_WriteDWord_Ex(REG_HDMI_VM_CFG_CH_0_1, ((UINT32)yg << 16) | ub);
	HAL_ModBits_Ex(REG_HDMI_VM_CFG_CH_2, 0xffff, vr);
}


VOID ms7200drv_hdmi_rx_controller_set_vmute_enable(BOOL b_vmute)
{
	HAL_ToggleBits_Ex(REG_HDMI_VM_CFG_CH_2, 0x10000ul, b_vmute);
}


static UINT8 _drv_hdmi_status_get_color_depth(VOID)
{
	//[31:28]
	UINT8 u8ColorDepth = HAL_ReadDWord_Ex(REG_HDMI_STS) >> 28;
	u8ColorDepth -= (u8ColorDepth >= 4) ? 4 : u8ColorDepth;
	return u8ColorDepth;
}


static HDMI_COLORIMETRY_E _drv_pdec_get_colorimetry(VOID)
{
	UINT8 u8Colorimetry = (HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 14) & MSRT_BITS1_0;
	switch (u8Colorimetry) {
	case 1:
		u8Colorimetry = HDMI_COLORIMETRY_601;
		break;
	case 2:
		u8Colorimetry = HDMI_COLORIMETRY_709;
		break;
	case 3:
		if (((HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 20) & MSRT_BITS2_0) == 0)
			u8Colorimetry = HDMI_COLORIMETRY_XVYCC601;
		else
			u8Colorimetry = HDMI_COLORIMETRY_XVYCC709;
		break;
	}
	return (HDMI_COLORIMETRY_E)u8Colorimetry;
}

static HDMI_VIDEO_FORMAT_E _drv_pdec_get_video_format(VOID)
{
	UINT8 u8VideoFormat = (HAL_ReadDWord_Ex(REG_PDEC_VSI_PAYLOAD0) >> 5) & MSRT_BITS2_0;
	switch (u8VideoFormat) {
	case 1:
		u8VideoFormat = HDMI_4Kx2K_FORMAT;
		break;
	case 2:
		u8VideoFormat = HDMI_3D_FORMAT;
		break;
	default:
		u8VideoFormat = HDMI_NO_ADD_FORMAT;
		break;
	}
	return (HDMI_VIDEO_FORMAT_E)u8VideoFormat;
}

static HDMI_4Kx2K_VIC_E _drv_pdec_get_4Kx2K_vic(VOID)
{
	UINT8 u84Kx2KVic = (HAL_ReadDWord_Ex(REG_PDEC_VSI_PAYLOAD0) >> 8) & MSRT_BITS7_0;
	switch (u84Kx2KVic) {
	case 1:
		u84Kx2KVic = HDMI_4Kx2K_30HZ;
		break;
	case 2:
		u84Kx2KVic = HDMI_4Kx2K_25HZ;
		break;
	case 3:
		u84Kx2KVic = HDMI_4Kx2K_24HZ;
		break;
	case 4:
		u84Kx2KVic = HDMI_4Kx2K_24HZ_SMPTE;
		break;
	default:
		u84Kx2KVic = HDMI_4Kx2K_30HZ;
		break;
	}
	return (HDMI_4Kx2K_VIC_E)u84Kx2KVic;
}

static HDMI_3D_STRUCTURE_E _drv_pdec_get_3D_structure(VOID)
{
	UINT8 u83DStructure = (HAL_ReadDWord_Ex(REG_PDEC_VSI_PAYLOAD0) >> 20) & MSRT_BITS3_0;
	switch (u83DStructure) {
	case 0:
		u83DStructure = HDMI_FRAME_PACKING;
		break;
	case 1:
		u83DStructure = HDMI_FIELD_ALTERNATIVE;
		break;
	case 2:
		u83DStructure = HDMI_LINE_ALTERNATIVE;
		break;
	case 3:
		u83DStructure = HDMI_SIDE_BY_SIDE_FULL;
		break;
	case 4:
		u83DStructure = L_DEPTH;
		break;
	case 5:
		u83DStructure = L_DEPTH_GRAPHICS;
		break;
	case 8:
		u83DStructure = SIDE_BY_SIDE_HALF;
		break;
	default:
		u83DStructure = HDMI_FRAME_PACKING;
		break;
	}
	return (HDMI_3D_STRUCTURE_E)u83DStructure;
}

#if VIN_FALSE
static HDMI_AUDIO_MODE_E _drv_pdec_get_audio_mode(VOID)
{
	UINT8 u8AudioMode = HAL_ReadDWord_Ex(REG_PDEC_AUD_STS) & MSRT_BITS3_0;
	switch (u8AudioMode) {
	case 0x01:
		u8AudioMode = HDMI_AUD_MODE_AUDIO_SAMPLE;
		break;
	case 0x02:
		u8AudioMode = HDMI_AUD_MODE_DSD;
		break;
	case 0x04:
		u8AudioMode = HDMI_AUD_MODE_DST;
		break;
	case 0x08:
		u8AudioMode = HDMI_AUD_MODE_HBR;
		break;
	default:
		u8AudioMode = HDMI_AUD_MODE_AUDIO_SAMPLE;
		break;
	}
	return (HDMI_AUDIO_MODE_E)u8AudioMode;
}
#endif

static HDMI_AUDIO_RATE_E _drv_pdec_get_audio_rate(VOID)
{
	UINT8 u8AudioRate = (HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) >> 10) & MSRT_BITS2_0;
	switch (u8AudioRate) {
	case 1:
		u8AudioRate = HDMI_AUD_RATE_32K;
		break;
	case 2:
		u8AudioRate = HDMI_AUD_RATE_44K1;
		break;
	case 3:
		u8AudioRate = HDMI_AUD_RATE_48K;
		break;
	case 4:
		u8AudioRate = HDMI_AUD_RATE_88K2;
		break;
	case 5:
		u8AudioRate = HDMI_AUD_RATE_96K;
		break;
	case 6:
		u8AudioRate = HDMI_AUD_RATE_176K4;
		break;
	case 7:
		u8AudioRate = HDMI_AUD_RATE_192K;
		break;
	default:
		u8AudioRate = HDMI_AUD_RATE_48K;
		break;
	}
	return (HDMI_AUDIO_RATE_E)u8AudioRate;
}

static HDMI_AUDIO_LENGTH_E _drv_pdec_get_audio_bits(VOID)
{
	UINT8 u8AudioBits = (HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) >> 8) & MSRT_BITS1_0;
	switch (u8AudioBits) {
	case 1:
		u8AudioBits = HDMI_AUD_LENGTH_16BITS;
		break;
	case 2:
		u8AudioBits = HDMI_AUD_LENGTH_20BITS;
		break;
	case 3:
		u8AudioBits = HDMI_AUD_LENGTH_24BITS;
		break;
	default:
		u8AudioBits = HDMI_AUD_LENGTH_16BITS;
		break;
	}
	return (HDMI_AUDIO_LENGTH_E)u8AudioBits;
}

static HDMI_AUDIO_CHN_E _drv_pdec_get_audio_channels(VOID)
{
	return (HDMI_AUDIO_CHN_E)(HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) & MSRT_BITS2_0);
}

static UINT8 _drv_pdec_get_audio_speaker_locations(VOID)
{
	return (UINT8)(HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) >> 24);
}

typedef struct _T_HDMI_AUDIO_SAMPLE_PACKET_PARA_ {
	UINT8  u8_audio_mode;		 // enum refer to HDMI_AUDIO_MODE_E
	UINT8  u8_audio_rate;		 // enum refer to HDMI_AUDIO_RATE_E
	UINT8  u8_audio_bits;		 // enum refer to HDMI_AUDIO_LENGTH_E
	UINT8  u8_audio_channels;	 // enum refer to HDMI_AUDIO_CHN_E
} AUDIO_SAMPLE_PACKET_T;

//refer to IEC-60958-3-Digital-Audio
static VOID _drv_pdec_get_audio_sample_packet_infomation(AUDIO_SAMPLE_PACKET_T *pt_packet)
{
	UINT8 u8_byte;

#if VIN_FALSE
	u8_byte = HAL_ReadByte(REG_MISC_C_BYTE0) & MSRT_BIT1;
	pt_packet->u8_audio_mode = u8_byte ? HDMI_AUD_MODE_HBR : HDMI_AUD_MODE_AUDIO_SAMPLE;
#endif

	u8_byte = HAL_ReadByte(REG_MISC_C_BYTE3);
	pt_packet->u8_audio_rate = u8_byte & 0xf;

	u8_byte = HAL_ReadByte(REG_MISC_C_BYTE4);
	u8_byte = u8_byte & 0xf;
	if (u8_byte == 0x03 || u8_byte == 0x0A)
		pt_packet->u8_audio_bits = HDMI_AUD_LENGTH_20BITS;
	else if (u8_byte == 0x0B)
		pt_packet->u8_audio_bits = HDMI_AUD_LENGTH_24BITS;
	else
		pt_packet->u8_audio_bits = HDMI_AUD_LENGTH_16BITS;

	u8_byte = HAL_ReadByte(REG_MISC_C_BYTE2);
	pt_packet->u8_audio_channels = u8_byte & 0xf;
}

UINT32 ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status(UINT32 u32_mask)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_PDEC_ISTS);
	if (u32_value & u32_mask) {
		//clr bit
		HAL_WriteDWord_Ex(REG_PDEC_ICLR, u32_value & u32_mask);
	}

	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status_ext(UINT32 u32_mask)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_PDEC_ISTS);
	return (u32_value & u32_mask);
}


UINT32 ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status(UINT32 u32_mask)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_HDMI_ISTS);
	if (u32_value & u32_mask) {
		//clr bit
		HAL_WriteDWord_Ex(REG_HDMI_ICLR, u32_value & u32_mask);
	}

	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status_ext(UINT32 u32_mask)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_HDMI_ISTS);
	return (u32_value & u32_mask);
}


UINT32 ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status(UINT32 u32_mask)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_AFIFO_ISTS);
	if (u32_value & u32_mask) {
		//clr bit
		HAL_WriteDWord_Ex(REG_AFIFO_ICLR, u32_value & u32_mask);
	}

	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status_ext(UINT32 u32_mask)
{
	UINT32 u32_value = HAL_ReadDWord_Ex(REG_AFIFO_ISTS);
	return (u32_value & u32_mask);
}

VOID ms7200drv_hdmi_rx_interrupt_to_pin_ctrl(UINT8 u8_module, UINT32 u32_mask, BOOL b_enable_to_pin)
{
	UINT16 u16_address;

	if (u8_module == 0) //HDMI_RX_CTRL_PDEC
		u16_address = REG_PDEC_IEN_SET;
	else if (u8_module == 1) //HDMI_RX_CTRL_MODET
		u16_address = REG_MD_IEN_SET;
	else if (u8_module == 2) //HDMI_RX_CTRL_HDMI
		u16_address = REG_HDMI_IEN_SET;
	else if (u8_module == 3) //HDMI_RX_CTRL_AUD
		u16_address = REG_AFIFO_IEN_SET;

	if (u8_module <= 3)
		HAL_WriteDWord_Ex(b_enable_to_pin ? u16_address : (u16_address - 4), u32_mask);
}

VOID ms7200drv_hdmi_rx_controller_get_input_config(HDMI_CONFIG_T *pt_hdmi_rx)
{
	AUDIO_SAMPLE_PACKET_T t_audio_sample_packet;

	pt_hdmi_rx->u8_hdmi_flag = !ms7200drv_hdmi_rx_controller_dvidet();
	pt_hdmi_rx->u8_vic = _drv_pdec_get_vic();
	pt_hdmi_rx->u16_video_clk = ms7200drv_hdmi_rx_get_tmds_clk();
	pt_hdmi_rx->u8_clk_rpt = _drv_pdec_get_clk_rpt();
	pt_hdmi_rx->u8_scan_info = _drv_pdec_get_scan_info();
	pt_hdmi_rx->u8_aspect_ratio = _drv_pdec_get_aspect_ratio();
	pt_hdmi_rx->u8_color_space = _drv_pdec_get_color_space();
#if VIN_FALSE
	//20180911, buf because, no packet input, the register keep pre stauts
	pt_hdmi_rx->u8_color_depth = _drv_pdec_get_color_depth();
#else
	pt_hdmi_rx->u8_color_depth = _drv_hdmi_status_get_color_depth();
#endif
	pt_hdmi_rx->u8_colorimetry = _drv_pdec_get_colorimetry();

	pt_hdmi_rx->u8_video_format = _drv_pdec_get_video_format();
	pt_hdmi_rx->u8_4Kx2K_vic = _drv_pdec_get_4Kx2K_vic();
	pt_hdmi_rx->u8_3D_structure = _drv_pdec_get_3D_structure();

	_drv_pdec_get_audio_sample_packet_infomation(&t_audio_sample_packet);
#if VIN_FALSE
	pt_hdmi_rx->u8_audio_mode = ms7200drv_pdec_get_audio_mode();
#else //20191231, fixed to audio sample mode. bacause others case never be tested.
	pt_hdmi_rx->u8_audio_mode = 0;
#endif
	pt_hdmi_rx->u8_audio_rate = _drv_pdec_get_audio_rate();
	pt_hdmi_rx->u8_audio_bits = _drv_pdec_get_audio_bits();
	pt_hdmi_rx->u8_audio_channels = _drv_pdec_get_audio_channels();

	//refer to header
#if VIN_FALSE
	//20181023, xiaomi bofangbo detect error.
	//if (pt_hdmi_rx->u8_audio_mode == 0) pt_hdmi_rx->u8_audio_mode = t_audio_sample_packet.u8_audio_mode;
#endif
	if (pt_hdmi_rx->u8_audio_rate == 0)
		pt_hdmi_rx->u8_audio_rate = t_audio_sample_packet.u8_audio_rate;
	if (pt_hdmi_rx->u8_audio_bits == 0)
		pt_hdmi_rx->u8_audio_bits = t_audio_sample_packet.u8_audio_bits;
	if (pt_hdmi_rx->u8_audio_channels == 0)
		pt_hdmi_rx->u8_audio_channels = t_audio_sample_packet.u8_audio_channels;

	pt_hdmi_rx->u8_audio_speaker_locations = _drv_pdec_get_audio_speaker_locations();
}

BOOL ms7200drv_hdmi_rx_controller_audio_channel_config(UINT8 u8_chn, UINT8 u8_speaker_locations)
{
	UINT8 u8_aud_sample_present;

	u8_aud_sample_present = HAL_ReadDWord_Ex(REG_AUD_SPARE) & 0x0F;

	if (u8_chn != 0x07 && u8_aud_sample_present == 0x0F && u8_speaker_locations != 0) {
		HAL_ModBits_Ex(REG_AUD_CHEXTR_CTRL, 0xFC, 0x80 | (0x13 << 2));
		return FALSE;
	} else {
		HAL_ModBits_Ex(REG_AUD_CHEXTR_CTRL, 0xFC, 0);
		return TRUE;
	}
}


VOID ms7200drv_hdmi_rx_controller_pixel_clk_config(UINT8 u8ColorDepth, UINT8 u8ClkRepeat)
{
	//rx pll deep color clk enable
	HAL_ToggleBits(REG_PI_RX_CTRL0, MSRT_BIT4, u8ColorDepth > 0);
	HAL_ModBits(REG_MISC_CLK_CTRL0, MSRT_BITS5_4, u8ColorDepth << 4);

	//
	//HAL_ModBits(REG_MISC_AV_CTRL2, MSRT_BITS3_0, u8ClkRepeat);

}

VOID ms7200drv_hdmi_rx_controller_scdc_config(BOOL bPowerBySource)
{
	HAL_ToggleBits_Ex(REG_SCDC_CONFIG, MSRT_BIT0, bPowerBySource);
}

static VOID _drv_hdmi_rx_audio_clk_sel(UINT8 u8_audio_clk_mode)
{
	//MS7200_LOG1("u8_audio_clk_mode = ", u8_audio_clk_mode);

	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		ms7200drv_hdmi_audio_clk_sel(0);

		//aupll_ref_ck_sel = 0
		HAL_ClrBits(REG_AUPLL_CTRL2, MSRT_BIT4);
		ms7200drv_hdmi_audio_pll_clk_reset_release(TRUE);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_2XPLL_MODE) {
		ms7200drv_hdmi_audio_clk_sel(1);

		//aupll_ref_ck_sel = 1
		HAL_SetBits(REG_AUPLL_CTRL2, MSRT_BIT4);
		ms7200drv_hdmi_audio_pll_clk_reset_release(FALSE);

		//rx_audll_div2_en = 1
		HAL_SetBits(REG_MISC_RX_PHY_SEL, MSRT_BIT5);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_PLL_MODE) {
		ms7200drv_hdmi_audio_clk_sel(1);

		//aupll_ref_ck_sel = 1
		HAL_SetBits(REG_AUPLL_CTRL2, MSRT_BIT4);
		ms7200drv_hdmi_audio_pll_clk_reset_release(FALSE);

		//rx_audll_div2_en = 0
		HAL_ClrBits(REG_MISC_RX_PHY_SEL, MSRT_BIT5);
	} else { // DLL
		ms7200drv_hdmi_audio_clk_sel(2);
		//rx_audll_div2_en = 0
		HAL_ClrBits(REG_MISC_RX_PHY_SEL, MSRT_BIT5);
	}
}

static VOID _drv_hdmi_rx_audio_init(VOID)
{
	//20190401, audio pll step adjust.
	//
	//HAL_WriteWord(REG_AUPLL_FIFO_TH1_L, 0x0c0);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH2_L, 0x180);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH3_L, 0x2c0);
	//
	//HAL_WriteWord(REG_AUPLL_FIFO_TH4_L, 0x300);
	//
	//HAL_WriteWord(REG_AUPLL_FIFO_TH5_L, 0x340);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH6_L, 0x480);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH7_L, 0x540);
	//
	//20200521, use default value. maybe cause output cts value change +- 10.
	//but if use beflow value, maybe cause can't support input audio cts value quick change
	HAL_WriteByte(REG_AUPLL_FREQ_INC_STEP1, 0x00);
	HAL_WriteByte(REG_AUPLL_FREQ_INC_STEP2, 0x01);
	HAL_WriteByte(REG_AUPLL_FREQ_INC_STEP3, 0x01);
	HAL_WriteByte(REG_AUPLL_FREQ_INC_STEP4, 0x01);
	HAL_WriteByte(REG_AUPLL_FREQ_DEC_STEP1, 0x00);
	HAL_WriteByte(REG_AUPLL_FREQ_DEC_STEP2, 0x01);
	HAL_WriteByte(REG_AUPLL_FREQ_DEC_STEP3, 0x01);
	HAL_WriteByte(REG_AUPLL_FREQ_DEC_STEP4, 0x01);

	//20180709, aupll_update_timer, FOSC = 25MHZ(40ns),
	//10us = 10000ns / 40ns = 250
	//set to 30us
	HAL_WriteWord(REG_AUPLL_UPD_TIMER_L, 0x0300);

	//20180320, enhance Audio PLL VCO stable
	HAL_WriteByte(REG_AUPLL_CFG_COEF, 0x0A);

	ms7200drv_hdmi_audio_clk_enable(TRUE);
	ms7200drv_hdmi_audio_dll_clk_reset_release(TRUE);
}

//u16_fs, uint: 100Hz; u16_tmds_clk, uint:10000Hz
VOID ms7200drv_hdmi_audio_pll_config(UINT8 u8_audio_clk_mode, UINT16 u16_tmds_clk, UINT32 N, UINT32 CTS)
{
	float f_val;
	UINT8 u8_pre_div;
	UINT8 u8_post_div;
	UINT16 u16_fs;
	BOOL b_hbr_input;

	//power down
	HAL_ClrBits(REG_AUPLL_PWR, MSRT_BIT1);
	HAL_SetBits(REG_AUPLL_PWR, MSRT_BIT0);

	HAL_SetBits(REG_AUPLL_FN_H, MSRT_BIT4);

	_drv_hdmi_rx_audio_clk_sel(u8_audio_clk_mode);

	//PRE_DIV
	if (u16_tmds_clk <= 5000)
		u8_pre_div = 0x00;
	else if (u16_tmds_clk <= 10000)
		u8_pre_div = 0x01;
	else if (u16_tmds_clk <= 20000)
		u8_pre_div = 0x02;
	else
		u8_pre_div = 0x03;

	//MS7200_LOG1("u8_pre_div = ", u8_pre_div);
	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE)
		HAL_ModBits(REG_AUPLL_CTRL2, MSRT_BITS1_0, u8_pre_div);

	u16_fs = 100UL * u16_tmds_clk / 128 * N / CTS;

	if (u16_fs == 0)
		u16_fs = 480;
	b_hbr_input = (HAL_ReadDWord_Ex(REG_PDEC_AUD_STS) & MSRT_BIT3) >> 3;

	//post_div
	u8_post_div = (UINT8)(7500000UL / 2 / (128UL * u16_fs));
	if (b_hbr_input)
		u8_post_div /= 4;
	else
		u8_post_div /= 2;   //256fs output
	//MS7200_LOG1("u8_post_div = ", u8_post_div);
	HAL_WriteByte(REG_AUPLL_POS_DVI, u8_post_div);

	//fbdiv
	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		u8_pre_div = (1 << u8_pre_div);
		f_val = (float)u8_post_div * 2 * u8_pre_div * N / CTS;
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_PLL_MODE)
		f_val = u8_post_div * 2 * 1;
	else //RX_AUDIO_DLL_TO_2XPLL_MODE
		f_val = u8_post_div * 2 * 2;

	if (b_hbr_input)
		f_val *= 4;
	else
		f_val *= 2;

	HAL_ToggleBits(REG_MISC_RX_PHY_SEL, MSRT_BIT1, !b_hbr_input);

	//MS7200_LOG1("f_val = ", (UINT8)f_val);
	HAL_WriteByte(REG_AUPLL_M, (UINT8)f_val);

	f_val = f_val - (UINT8)f_val;
	f_val = f_val * (1UL << 20);
	HAL_WriteByte(REG_AUPLL_FN_L, ((UINT32)f_val) >> 0);
	HAL_WriteByte(REG_AUPLL_FN_M, ((UINT32)f_val) >> 8);
	HAL_ModBits(REG_AUPLL_FN_H, MSRT_BITS3_0, ((UINT32)f_val) >> 16);

	//aupll_div2_sel = 1
	HAL_SetBits(REG_AUPLL_CTRL1, MSRT_BIT7);

	//power on
	HAL_ClrBits(REG_AUPLL_PWR, MSRT_BIT0);
	Delay_us(100);
	HAL_SetBits(REG_AUPLL_PWR, MSRT_BIT1);
}

//input:  u16_tmds_clk, uint:10000Hz;
//return: u16_fs, uint: 100Hz;
UINT16 ms7200drv_hdmi_rx_audio_config(UINT8 u8_audio_clk_mode, UINT16 u16_tmds_clk)
{
	UINT32 N;
	UINT32 CTS;
	UINT16 u16_fs = 0;
	BOOL b_dll_128fs = FALSE;

	CTS = HAL_ReadDWord_Ex(REG_PDEC_ACR_CTS) & 0xfffffUL;
	N = HAL_ReadDWord_Ex(REG_PDEC_ACR_N) & 0xfffffUL;
	//MS7200_LOG1("CTS_H = ", CTS >> 16);
	//MS7200_LOG1("CTS_L = ", CTS);
	//MS7200_LOG1("N_H = ", N >> 16);
	//MS7200_LOG1("N_L = ", N);
	if ((CTS == 0) || (N  == 0) || (u8_audio_clk_mode == RX_AUDIO_PLL_FREERUN_MODE)) {
		//MS7200_LOG("Auido PLL.");
		N = 0x1800UL;
		//fixed to 96KHz
		CTS = 100UL * u16_tmds_clk / 128 * N / 960;
		if (CTS == 0)
			CTS = 0x1220AUL;
		ms7200drv_hdmi_audio_pll_config(RX_AUDIO_PLL_MODE, u16_tmds_clk, N, CTS);
		return u16_fs;
	}

	u16_fs = 100UL * u16_tmds_clk / 128 * N / CTS;
	//MS7200_LOG2("u16_audio_fs = ", u16_fs);

	//if tmds_clk > 256fs
	if (CTS > (2 * (N + N / 100)))
		b_dll_128fs = TRUE;

	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		//MS7200_LOG("Auido PLL.");
		ms7200drv_hdmi_audio_pll_config(RX_AUDIO_PLL_MODE, u16_tmds_clk, N, CTS);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_2XPLL_MODE) {
		//MS7200_LOG("Auido DLL+2xPLL.");
		ms7200drv_hdmi_audio_pll_config(RX_AUDIO_DLL_TO_2XPLL_MODE, u16_tmds_clk, N, CTS);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_PLL_MODE) {
		if (b_dll_128fs) {
			//MS7200_LOG("Auido DLL+PLL.");
			ms7200drv_hdmi_audio_pll_config(RX_AUDIO_DLL_TO_PLL_MODE, u16_tmds_clk, N, CTS);
		} else { //fixed couvert
			//MS7200_LOG("Auido DLL+2xPLL.");
			ms7200drv_hdmi_audio_pll_config(RX_AUDIO_DLL_TO_2XPLL_MODE, u16_tmds_clk, N, CTS);
		}
	} else { // DLL
		if (b_dll_128fs) {
			//MS7200_LOG("Auido DLL.");
			_drv_hdmi_rx_audio_clk_sel(RX_AUDIO_DLL_MODE);
		} else { //fixed couvert
			//MS7200_LOG("Auido DLL+2xPLL.");
			ms7200drv_hdmi_audio_pll_config(RX_AUDIO_DLL_TO_2XPLL_MODE, u16_tmds_clk, N, CTS);
		}
	}

	return u16_fs;
}
