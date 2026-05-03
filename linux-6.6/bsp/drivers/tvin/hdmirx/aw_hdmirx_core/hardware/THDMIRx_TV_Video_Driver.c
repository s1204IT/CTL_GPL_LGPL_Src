/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/math64.h>
#include "../../aw_hdmirx_define.h"
#include "THDMIRx_TV_Driver.h"
#include "../THDMIRx_System.h"
#include "ThdmiRxIPReg.h"
#include "ThdmiRxMvReg.h"
#include "../aw_hdmirx_core_drv.h"
#include "../THDMIRx_Event.h"
#include "../THDMIRx_PortCtx.h"

void HdmiRx_Video_set_regbase(uintptr_t reg_base)
{
	vs_base = reg_base;
	link_base = reg_base + 0x40000;
	phy_base = reg_base + 0x800;
	audio_pll_base = reg_base + 0x80000;
}

bool HdmiRx_Video_GetVRRMode(void)
{
	return HDMIRx_ReadRegU8(link_base + NEW_FORMAT_DET_30) & ALLOW_VRR_MODE;
}

void HdmiRx_Video_SetVRRMode(bool isEnterVRR)
{
	HDMIRx_WriteRegMaskU8(link_base + NEW_FORMAT_DET_30, ALLOW_VRR_MODE, isEnterVRR ? ALLOW_VRR_MODE : 0);
}

bool HdmiRx_Video_IsInterlace(void)
{
	U8 data;
	data = HDMIRx_ReadRegU8(link_base + NEW_FORMAT_DET_25);

	return (data & INTERLACED_NEWFD) ? true : false;
}

void HdmiRx_Video_SCDC_Detect(void *pSCDCContex)
{
	U8 data;
	U8 scrambling_enable = 0;
	U8 bit_clock_ratio = 0;
	ThdmiRxSCDCContext *pSDCDCtx = (ThdmiRxSCDCContext *)pSCDCContex;
	data = HDMIRx_ReadRegU8(link_base + SCDC_TMDS_CONFIG);
	bit_clock_ratio = (data & TMDS_BIT_CLOCK_RATIO) >> TMDS_BIT_CLOCK_RATIO_SHIFT;
	scrambling_enable = data & SCRAMBLINE_ENABLE;

	// detect bit clock ratio
	if (pSDCDCtx->ucTMDSBitClockRatio != bit_clock_ratio) {
		hdmirx_inf("%s: TMDS_Bit_Clock_Ratio is changed from %d to %d\n", __func__, pSDCDCtx->ucTMDSBitClockRatio, bit_clock_ratio);
		pSDCDCtx->ucTMDSBitClockRatio = bit_clock_ratio;

		// update Rx mode
		//HDMIRx_WriteRegMaskU8(link_base + CTRLA_PHY_RF3, RX_MODE, bit_clock_ratio ? RX_MODE : 0);

		// update AEC
		HdmiRx_AEC_Update(bit_clock_ratio ? true : false);
	}

	// detect scrambling_enable
	if (pSDCDCtx->ucScramblingEnable != scrambling_enable) {
		hdmirx_inf("%s: Scrambling_Enable is changed from %d to %d\n", __func__, pSDCDCtx->ucScramblingEnable, scrambling_enable);
		pSDCDCtx->ucScramblingEnable = scrambling_enable;

		if (scrambling_enable) {
			HDMIRx_WriteRegMaskU8(link_base + LFSR_CONTROL_1, EN_SCRAMBLER, EN_SCRAMBLER);
			HDMIRx_WriteRegMaskU8(link_base + SCDC_SCRAMBLE_STATUS, SCRAMBLING_STATUS, SCRAMBLING_STATUS);
		} else {
			HDMIRx_WriteRegMaskU8(link_base + LFSR_CONTROL_1, EN_SCRAMBLER, 0);
			HDMIRx_WriteRegMaskU8(link_base + SCDC_SCRAMBLE_STATUS, SCRAMBLING_STATUS, 0);
		}
	}
}

U32 HdmiRx_Video_TMDSGetFreq(void)
{
	U8 div_num = 0;
	U32 threshold = 0;
	U32 cnt_val = 0;
	U32 ulFreq = 1;

	cnt_val = HdmiRx_GetRegu32_ByRegu8(link_base, PHY_CONTROL_28);
	if (!cnt_val) {
		return ulFreq;
}

	div_num = HDMIRx_ReadRegU8(link_base + PHY_CONTROL_27) & DIV_NUM_MASK;
	threshold = (1 << div_num) * CPU_FREQ_MHZ;
	//ulFreq = (U32)((U32)(threshold * 1000000ULL) / cnt_val);
	ulFreq = (U32)div_u64(threshold * 1000000ULL, cnt_val);

	//VIDEO_INF("cnt_val = %d div_num = %d threshold = %d ulFreqv = %d\n", cnt_val, div_num, threshold, ulFreq);
	return ulFreq;
}

void HdmiRx_Video_GetTimingParam(void *pTimingData, void *pCtx)
{
	U32 color_ratio; /* 1*100 */
	U8 data;
	U8 data_h, data_l;
	ThdmRxTimingParam *pTimingParam = (ThdmRxTimingParam *)pTimingData;
	struct HDMIRx_PortContext *pPortContext = (struct HDMIRx_PortContext *)pCtx;
	// get color depth from GCP
	data = HDMIRx_ReadRegU8(link_base + GENERAL_CONTROL_PACKET_2);
	switch ((data & GCP_CD_MASK) >> GCP_CD_SHIFT) {
	case GCP_CD_10BIT:
		pTimingParam->eColorDepth = COLOR_DEPTH_10BIT;
		color_ratio = 125;
		break;
	case GCP_CD_12BIT:
		pTimingParam->eColorDepth = COLOR_DEPTH_12BIT;
		color_ratio = 150;
		break;
	case GCP_CD_16BIT:
		pTimingParam->eColorDepth = COLOR_DEPTH_16BIT;
		color_ratio = 200;
		break;
	case GCP_CD_8BIT:
	default:
		color_ratio = 100;
		pTimingParam->eColorDepth = COLOR_DEPTH_8BIT;
		break;
	}

	// get TMDS frequency
	pTimingParam->pixelFreq = HdmiRx_Video_TMDSGetFreq();

	// get Pixel Repetition
	#if HDMIRX_PR_PATCH
		// for SX8B, has color cliping issue, can't enable PP mode
		pTimingParam->bpixRep = false;
	#else
		pTimingParam->bpixRep = HdmiRxPacket_GetPixelRepetition(&(pPortContext->PacketCtx.ctx));
	#endif

	// get H total
	data_h = HDMIRx_ReadRegU8(link_base + HTOTAL_NEWFD_HI) & HTOTAL_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + HTOTAL_NEWFD_LO);
	pTimingParam->numTotalPixelsPerLine = ((data_h << HTOTAL_NEWFD_HI_SHIFT) + data_l) / (pTimingParam->bpixRep + 1);
	//VIDEO_INF("%s: data_h = %d, data_l = %d, numTotalPixelsPerLine = %d\n", __func__, data_h, data_l, pTimingParam->numTotalPixelsPerLine);

	// get V total
	data_h = HDMIRx_ReadRegU8(link_base + VTOTAL_NEWFD_HI) & VTOTAL_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + VTOTAL_NEWFD_LO);
	pTimingParam->numTotalFrameLines = ((data_h << VTOTAL_NEWFD_HI_SHIFT) + data_l);
	//VIDEO_INF("%s: data_h = %d, data_l = %d, numTotalFrameLines = %d\n", __func__, data_h, data_l, pTimingParam->numTotalFrameLines);

	// get H active
	data_h = HDMIRx_ReadRegU8(link_base + HACTIVE_NEWFD_HI) & HACTIVE_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + HACTIVE_NEWFD_LO);
	pTimingParam->numActivePixelsPerLine = ((data_h << HACTIVE_NEWFD_HI_SHIFT) + data_l) / (pTimingParam->bpixRep + 1);

	// get V active
	data_h = HDMIRx_ReadRegU8(link_base + VACTIVE_NEWFD_HI) & VACTIVE_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + VACTIVE_NEWFD_LO);
	pTimingParam->numActiveLines = ((data_h << VACTIVE_NEWFD_HI_SHIFT) + data_l);

	// get H duration(H active width)
	data_h = HDMIRx_ReadRegU8(link_base + HSYNC_NEWFD_HI) & HSYNC_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + HSYNC_NEWFD_LO);
	pTimingParam->hsyncWidth = ((data_h << HSYNC_NEWFD_HI_SHIFT) + data_l) / (pTimingParam->bpixRep + 1);

	// get V duration
	data_h = HDMIRx_ReadRegU8(link_base + VSYNC_NEWFD_HI) & VSYNC_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + VSYNC_NEWFD_LO);
	pTimingParam->vsyncWidth = ((data_h << VSYNC_NEWFD_HI_SHIFT) + data_l);

	// get H blank
	data_h = HDMIRx_ReadRegU8(link_base + HBLANK_NEWFD_HI) & HBLANK_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + HBLANK_NEWFD_LO);
	pTimingParam->hblank = ((data_h << HBLANK_NEWFD_HI_SHIFT) + data_l) / (pTimingParam->bpixRep + 1);

	// get V blank
	data_h = HDMIRx_ReadRegU8(link_base + VBLANK_NEWFD_HI) & VBLANK_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + VBLANK_NEWFD_LO);
	pTimingParam->vblank = ((data_h << VBLANK_NEWFD_HI_SHIFT) + data_l);

	// get H delay
	data_h = HDMIRx_ReadRegU8(link_base + HFRONT_NEWFD_HI) & HFRONT_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + HFRONT_NEWFD_LO);
	pTimingParam->hdelay = ((data_h << HFRONT_NEWFD_HI_SHIFT) + data_l) / (pTimingParam->bpixRep + 1);

	// get V delay
	data_h = HDMIRx_ReadRegU8(link_base + VBACK_NEWFD_HI) & VBACK_NEWFD_HI_MASK;
	data_l = HDMIRx_ReadRegU8(link_base + VBACK_NEWFD_LO);
	pTimingParam->vdelay = ((data_h << VBACK_NEWFD_HI_SHIFT) + data_l) + pTimingParam->vsyncWidth;

	// get interlaced and polarity
	data = HDMIRx_ReadRegU8(link_base + NEW_FORMAT_DET_25);
	pTimingParam->bVsyncPolarity = (data & VPOLAR_NEWFD) ? true : false;
	pTimingParam->bHsyncPolarity = (data & HPOLAR_NEWFD) ? true : false;
	pTimingParam->bInterlaced = (data & INTERLACED_NEWFD) ? true : false;

	// save original H/V total for MV
	pTimingParam->rawHTotal = pTimingParam->numTotalPixelsPerLine;
	pTimingParam->rawVTotal = pTimingParam->numTotalFrameLines;

	// process interlaced signal
	if (pTimingParam->bInterlaced) {
		pTimingParam->numTotalFrameLines /= 2;
		pTimingParam->numActiveLines /= 2;
	}

	// process vfreq
	if (pTimingParam->numTotalFrameLines && pTimingParam->numTotalPixelsPerLine) {
	pTimingParam->fieldFreq = pTimingParam->pixelFreq / (color_ratio * pTimingParam->numTotalPixelsPerLine * (pTimingParam->bpixRep + 1) * \
							pTimingParam->numTotalFrameLines / 1000 / 100);
	} else {
		pTimingParam->fieldFreq = 1000;
	}

	// process YUV420 signal
	if (HDMI_IN_YUV420 == HdmiRxPacket_GetColorFormat(&(pPortContext->PacketCtx.ctx), (U16)(pPortContext->videoCtx.ctx.CurVideoMode))) {
		pTimingParam->numActivePixelsPerLine *= 2;
		pTimingParam->numTotalPixelsPerLine *= 2;
		pTimingParam->hblank *= 2;
		pTimingParam->hsyncWidth *= 2;
		pTimingParam->hdelay *= 2;
	}

	// V back porch
	pTimingParam->offsetFirstActiveLine = pTimingParam->vdelay - pTimingParam->vsyncWidth;

	// H back porch
	pTimingParam->offsetFirstActivePixel = pTimingParam->hblank - pTimingParam->hsyncWidth - pTimingParam->hdelay;

	//VIDEO_INF("%s: fieldFreq = %d, pixelFreq = %d, color_ratio = %d, numTotalPixelsPerLine = %d, bpixRep =%d numTotalFrameLines = %d\n", \
	//__func__, pTimingParam->fieldFreq, pTimingParam->pixelFreq, color_ratio, \
	pTimingParam->numTotalPixelsPerLine, pTimingParam->bpixRep, pTimingParam->numTotalFrameLines);

}

// isr handle func
bool HdmiRx_Video_TMDS_FreqDetect(void *pVideoCtx)
{
	U32 cnt_val, threshold;
	U8 div_num, iFreqVal;
	struct HDMIRx_VideoContext *pCtx = (struct HDMIRx_VideoContext *)pVideoCtx;
	// check count value status
	VIDEO_INF("MSG_HDMIRX_INTR_PHY_INT_FREQ_DET received!\n");
	cnt_val = HdmiRx_GetRegu32_ByRegu8(link_base, PHY_CONTROL_28);
	if ((cnt_val & 0xFFFFFF00) != (pCtx->TmdsCtx.dwCntVal & 0xFFFFFF00)) {
		// calculate threshold
		div_num = HDMIRx_ReadRegU8(link_base + PHY_CONTROL_27) & DIV_NUM_MASK;
		threshold = (1 << div_num) * CPU_FREQ_MHZ;

		// calculate I_FREQ
		if (cnt_val < (threshold / 200)) {
			iFreqVal = I_FREQ_ABOVE_200;
		} else if (cnt_val < (threshold / 100)) {
			iFreqVal = I_FREQ_100_200;
		} else if (cnt_val < (threshold / 50)) {
			iFreqVal = I_FREQ_50_100;
		} else {
			iFreqVal = I_FREQ_BELOW_50;
		}

		HDMIRx_WriteRegMaskU8(link_base + PHY_CONTROL_32, I_FREQ_MASK, iFreqVal);
		VIDEO_INF("HdmiRxTMDS_FreqDetect success, cnt_val=0x%x div_num=0x%x, tmds=0x%x!\n", cnt_val, div_num,
		(U32)div_u64(threshold * 1000000ULL, cnt_val));
		return true;
	}

	// store new counter value
	pCtx->TmdsCtx.dwCntVal = cnt_val;
	return false;
}

void HdmiRx_Video_UpdateHdmiMode(U8 video_mode, void *pVideoCtx)
{
	struct HDMIRx_VideoContext *pCtx = (struct HDMIRx_VideoContext *)pVideoCtx;
	VideoMode_e mode = (VideoMode_e)video_mode;
	if (mode == pCtx->ctx.CurVideoMode) {
		return;
	}
	VIDEO_INF("Update HDMI mode to %d\n", mode);
	pCtx->ctx.CurVideoMode = mode;

	if (mode == VIDEO_MODE_HDMI) {
	} else if (mode == VIDEO_MODE_DVI) {
	// we output 444, RGB and 8 bit by default in DVI Mode
#if HDMIRX_PR_PATCH
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, AUTO_PR_EN, AUTO_PR_EN);
		HDMIRx_WriteRegMaskU8(link_base + MISC_VIDEO_CONTROL, USE_IDCLK2X_PR, 0);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, MAN_PR, 0);
#endif
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_FORMAT, COLOR_SPACE, 0);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, EN_420_422_CONV, 0);
	}
}

void HdmiRx_Video_UpdatePixelRepetition(U8 PixelRepetition)
{
#if HDMIRX_PR_PATCH
	if (PixelRepetition) {
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, AUTO_PR_EN, 0);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, MAN_PR, PixelRepetition << 4);
		HDMIRx_WriteRegMaskU8(link_base + MISC_VIDEO_CONTROL, USE_IDCLK2X_PR, 0);
	} else {
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, AUTO_PR_EN, AUTO_PR_EN);
		HDMIRx_WriteRegMaskU8(link_base + MISC_VIDEO_CONTROL, USE_IDCLK2X_PR, 0);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, MAN_PR, 0);
	}
#else
	switch (PixelRepetition) {
	case 1:
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, DIV2_IDCLK | DIV4_IDCLK, DIV2_IDCLK);
		break;
	case 3:
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, DIV2_IDCLK | DIV4_IDCLK, DIV4_IDCLK);
		break;
	default:
		if (PixelRepetition > 1) {
		HDMIRx_WriteRegMaskU8(link_base + MISC_VIDEO_CONTROL, SW_IDCLK2X | USE_IDCLK2X_PR, SW_IDCLK2X | USE_IDCLK2X_PR);
	}
		break;
	}
#endif
}

void HdmiRx_Video_UpdateColorSpaceModel(eHdmiRxColorFmt color_format)
{
	HDMIRx_WriteRegMaskU8(link_base + RAM_POWER_DOWN, RAM_PD_UP_CONV, RAM_PD_UP_CONV);
	HDMIRx_WriteRegMaskU8(link_base + RAM_POWER_DOWN, CE_OFF_UP_CONV, CE_OFF_UP_CONV);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_420_PATH, SYSTEM_PD_420_PATH);

	switch (color_format) {
	case hdmiRx_ColorFmt_YUV444:
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_FORMAT, PIXEL_ENC | COLOR_SPACE, COLOR_SPACE);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, EN_420_422_CONV, 0);
		break;
	case hdmiRx_ColorFmt_YUV422:
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_FORMAT, PIXEL_ENC | COLOR_SPACE, COLOR_SPACE);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, EN_420_422_CONV, 0);
		// HDMIRx_WriteRegMaskU8(base + VIDEO_OUTPUT_PIN_CONTROL_2, YCC422_2CH, YCC422_2CH);
		break;
	case hdmiRx_ColorFmt_YUV420:
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_FORMAT, PIXEL_ENC | COLOR_SPACE, COLOR_SPACE);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, EN_420_422_CONV, EN_420_422_CONV);
		HDMIRx_WriteRegMaskU8(link_base + RAM_POWER_DOWN, RAM_PD_UP_CONV, 0);

		HDMIRx_WriteRegMaskU8(link_base + RAM_POWER_DOWN, CE_OFF_UP_CONV, 0);
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_420_PATH, 0);
		break;
	case hdmiRx_ColorFmt_RGB:
		HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_FORMAT, PIXEL_ENC | COLOR_SPACE, 0);
		HDMIRx_WriteRegMaskU8(link_base + ITU656_CONTROL, EN_420_422_CONV, 0);
		break;
	default:
		break;
	}
}

// toggle SYSTEM_PD_IDCLK reset
void HdmiRx_Toggle_PD_IDCLK_Reset(void)
{
	LOG_TRACE();
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_1, SYSTEM_PD_IDCLK, SYSTEM_PD_IDCLK);
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_1, SYSTEM_PD_IDCLK, 0);
}
