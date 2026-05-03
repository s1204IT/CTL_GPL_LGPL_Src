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
#include "../aw_hdmirx_define.h"
#include "THDMIRx_PortCtx.h"
#include "THDMIRx_DataPath.h"

struct HDMIRx_PortContext *core_portCtx[HDMI_PORT_NUM];

struct HDMIRx_PortContext *HDMIRx_get_portCtx(U8 PortID)
{
	if (PortID > HDMI_PORT_NUM)
		return NULL;
	return core_portCtx[PortID - 1];
}

U32 ConvertTo_PC_SignalID(bool isDVI, U32 cur_signalID)
{
	U32 signalID = cur_signalID;
	if (isDVI) {
		switch (cur_signalID) {
		case AI_SIGNAL_MODE_480P:
			signalID = AI_SIGNAL_MODE_640_480;
			break;

		case AI_SIGNAL_MODE_576P:
			signalID = AI_SIGNAL_MODE_720_576;
			break;

		case AI_SIGNAL_MODE_720P:
			signalID = AI_SIGNAL_MODE_1280_720;
			break;

		case AI_SIGNAL_MODE_1080P:
			signalID = AI_SIGNAL_MODE_1920_1080;
			break;

		default:
			break;
		}
	}

	PORT_INF("%s: Conver signalID,isDVI:%d signal:0x%x\n", __func__, isDVI, signalID);
	return signalID;
}

bool CheckTimingDiff(U8 PortID, ThdmRxTimingParam *pOldTiming, ThdmRxTimingParam *pNewTiming)
{
	U16 diff;
	bool ret = false;
	struct HDMIRx_PortContext *mportCtx;

	mportCtx = HDMIRx_get_portCtx(PortID);
	if (mportCtx == NULL)
		return ret;

	// check interlaced
	if (IS_DIFF(pNewTiming->bInterlaced, pOldTiming->bInterlaced)) {
		PORT_INF("%s: bInterlaced is changed from %d to %d\n", __func__, pOldTiming->bInterlaced, pNewTiming->bInterlaced);
		ret = true;
	}

	// check pixel frequency
	if (IS_DIFF(pNewTiming->pixelFreq, pOldTiming->pixelFreq)) {
		diff = pOldTiming->pixelFreq > pNewTiming->pixelFreq ? (pOldTiming->pixelFreq - pNewTiming->pixelFreq) / 1000
														: (pNewTiming->pixelFreq - pOldTiming->pixelFreq) / 1000;
		if (diff > HDMIRX_PIXEL_FREQ_RANGE) {
			PORT_INF("%s: pixel frequency is changed from %d to %d, diff is %d\n", __func__, pOldTiming->pixelFreq, pNewTiming->pixelFreq, diff);
			ret = true;
		}
	}

	// check field frequency
	if (IS_DIFF(pNewTiming->fieldFreq, pOldTiming->fieldFreq)) {
		diff = pOldTiming->fieldFreq > pNewTiming->fieldFreq ? (U16)(pOldTiming->fieldFreq - pNewTiming->fieldFreq)
														: (U16)(pNewTiming->fieldFreq - pOldTiming->fieldFreq);
		if (diff > HDMIRX_FIELD_FREQ_RANGE) {
			PORT_INF("%s: field frequency is changed from %d to %d\n", __func__, pOldTiming->fieldFreq, pNewTiming->fieldFreq);
			ret = true;
		}
	}

	// check h back porch
	if (IS_DIFF(pNewTiming->offsetFirstActiveLine, pOldTiming->offsetFirstActiveLine)) {
		diff = pOldTiming->offsetFirstActiveLine > pNewTiming->offsetFirstActiveLine
					? (pOldTiming->offsetFirstActiveLine - pNewTiming->offsetFirstActiveLine)
					: (pNewTiming->offsetFirstActiveLine - pOldTiming->offsetFirstActiveLine);
		if (diff > HDMIRX_HBACK_PORCH_RANGE) {
			PORT_INF("%s: offsetFirstActiveLine is changed from %d to %d, diff is %d\n", __func__, pOldTiming->offsetFirstActiveLine,
				pNewTiming->offsetFirstActiveLine, diff);
			ret = true;
		}
	}

	// check v total
	if (IS_DIFF(pNewTiming->numTotalFrameLines, pOldTiming->numTotalFrameLines)) {
		diff = pOldTiming->numTotalFrameLines > pNewTiming->numTotalFrameLines
					? (pOldTiming->numTotalFrameLines - pNewTiming->numTotalFrameLines)
					: (pNewTiming->numTotalFrameLines - pOldTiming->numTotalFrameLines);
		if (diff > HDMIRX_VTOTAL_RANGE) {
			PORT_INF("%s: numTotalFrameLines is changed from %d to %d, diff is %d\n", __func__, pOldTiming->numTotalFrameLines,
				pNewTiming->numTotalFrameLines, diff);
			// if enter freesync mode, ignore vtotal diff
			if (!mportCtx->videoCtx.ctx.bVrrMode)
				ret = true;
		}
	}

	// check v active
	if (IS_DIFF(pNewTiming->numActiveLines, pOldTiming->numActiveLines)) {
		diff = pOldTiming->numActiveLines > pNewTiming->numActiveLines ? (pOldTiming->numActiveLines - pNewTiming->numActiveLines)
															: (pNewTiming->numActiveLines - pOldTiming->numActiveLines);
		if (diff > HDMIRX_VACTIVE_RANGE) {
			PORT_INF("%s: numActiveLines is changed from %d to %d, diff is %d\n", __func__, pOldTiming->numActiveLines, pNewTiming->numActiveLines, diff);
			ret = true;
		}
	}

	// check v back porch
	if (IS_DIFF(pNewTiming->offsetFirstActivePixel, pOldTiming->offsetFirstActivePixel)) {
		diff = pOldTiming->offsetFirstActivePixel > pNewTiming->offsetFirstActivePixel
					? (pOldTiming->offsetFirstActivePixel - pNewTiming->offsetFirstActivePixel)
					: (pNewTiming->offsetFirstActivePixel - pOldTiming->offsetFirstActivePixel);
		if (diff > HDMIRX_VBACK_PORCH_RANGE) {
			PORT_INF("%s: offsetFirstActivePixel is changed from %d to %d, diff is %d\n", __func__, pOldTiming->offsetFirstActivePixel,
					pNewTiming->offsetFirstActivePixel, diff);
			ret = true;
		}
	}

	// check h total
	if (IS_DIFF(pNewTiming->numTotalPixelsPerLine, pOldTiming->numTotalPixelsPerLine)) {
		diff = pOldTiming->numTotalPixelsPerLine > pNewTiming->numTotalPixelsPerLine
				? (pOldTiming->numTotalPixelsPerLine - pNewTiming->numTotalPixelsPerLine)
				: (pNewTiming->numTotalPixelsPerLine - pOldTiming->numTotalPixelsPerLine);
		if (diff > HDMIRX_HTOTAL_RANGE) {
			PORT_INF("%s: numTotalPixelsPerLine is changed from %d to %d, diff is %d\n", __func__, pOldTiming->numTotalPixelsPerLine,
				pNewTiming->numTotalPixelsPerLine, diff);
			ret = true;
		}
	}

	// check h active
	if (IS_DIFF(pNewTiming->numActivePixelsPerLine, pOldTiming->numActivePixelsPerLine)) {
		diff = pOldTiming->numActivePixelsPerLine > pNewTiming->numActivePixelsPerLine
					? (pOldTiming->numActivePixelsPerLine - pNewTiming->numActivePixelsPerLine)
					: (pNewTiming->numActivePixelsPerLine - pOldTiming->numActivePixelsPerLine);
		if (diff > HDMIRX_HACTIVE_RANGE) {
			PORT_INF("%s: numActivePixelsPerLine is changed from %d to %d, diff is %d\n", __func__, pOldTiming->numActivePixelsPerLine,
					pNewTiming->numActivePixelsPerLine, diff);
			ret = true;
		}
	}

	return ret;
}

bool SignalMonitor(U8 PortID, U8 SignalIndex, U32 SignalID, void *pPortContex)
{
	// THDMI3DInfo tNew3DInfo;
	struct HDMIRx_PortContext *mportCtx;

	mportCtx = HDMIRx_get_portCtx(PortID);
	if (mportCtx == NULL)
		return false;

	U32 dwChangeStatus = 0;
	U8 ucNewIndex = SignalIndex;
	U32 dwNewSignal = SignalID;
	U32 dwNewFrameRate = AI_SIGNAL_FRAMERATE_60;
	U16 wVidColorFmt = NONE_VIDEO_IN_FORMAT;
	bool bVrr = false;
	bool bHDR10Plus = false;
	eHdmiRxEOTF HdrMode = hdmiRx_EOTF_SDR;
	// eHdmiRxColorMatrix wColorMatrix = hdmiRx_ColorMatrix_NotIndicated;
	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;
	struct HDMIRx_PacketContext *pPacketContext = &pPortCtx->PacketCtx;
	// get current signal format and signal index
	if (dwNewSignal != AI_SIGNAL_MODE_UNKNOW) {
		//log_i("dwNewSignal:0x%x ucNewIndex:%d", dwNewSignal, ucNewIndex);
		dwNewFrameRate = DisplayModule_GetFrameRateFromDB(ucNewIndex);

		// PATCH:copy this part of code from old hdmirx driver
		if ((mportCtx->videoCtx.ctx.CurVideoMode == VIDEO_MODE_DVI) &&
			(DisplayModule_CheckAttrGroupContains(AT_SIGNAL_MODE, AG_SIGNAL_MODE_HD, dwNewSignal) ||
			DisplayModule_CheckAttrGroupContains(AT_SIGNAL_MODE, AG_SIGNAL_MODE_UHD, dwNewSignal))) {
			if (dwNewSignal == AI_SIGNAL_MODE_720P) {
				dwNewSignal = AI_SIGNAL_MODE_1280_720;
			} else if (dwNewSignal == AI_SIGNAL_MODE_1080P) {
				dwNewSignal = AI_SIGNAL_MODE_1920_1080;
			} else if (dwNewSignal == AI_SIGNAL_MODE_2160P3840) {
				dwNewSignal = AI_SIGNAL_MODE_3840_2160;
			} else if (dwNewSignal == AI_SIGNAL_MODE_2160P4096) {
				dwNewSignal = AI_SIGNAL_MODE_4096_2160;
			}
		}
		// PATCH END.
	} else {
		dwNewSignal = AI_SIGNAL_MODE_UNKNOW;
		dwNewFrameRate = AI_SIGNAL_FRAMERATE_60;
		mportCtx->videoCtx.ctx.bCurEdrMode = false;
		mportCtx->videoCtx.ctx.bVrrMode = false;
		mportCtx->videoCtx.ctx.bHDR10Plus = false;
		hdmirx_err("Didn't find correct signal index from DB\n");

		// why not set signal here immediate? timing is stable but can not recognize by database.
		if ((AI_SIGNAL_MODE_UNKNOW != mportCtx->videoCtx.ctx.dwCurSignal) && (AI_SIGNAL_MODE_NO_SIGNAL != mportCtx->videoCtx.ctx.dwCurSignal)) {
			mportCtx->videoCtx.ctx.dwCurSignal = dwNewSignal;
			HDMIRx_DisplayModuleCtx_SetSignal(dwNewSignal, pPortCtx);
			return true;
		}
	}

	// get video color format
	wVidColorFmt = HdmiRxPacket_GetColorFormat(&pPacketContext->ctx, (U16)(mportCtx->videoCtx.ctx.CurVideoMode));
	if (wVidColorFmt == DVI_IN_RGB444) {
		wVidColorFmt = HDMI_IN_RGB444;
	}

	if (IS_DIFF(mportCtx->videoCtx.ctx.wCurVidFormat, wVidColorFmt)) {
		PORT_INF("%s: Video Format changed from %d to %d\n", __func__, mportCtx->videoCtx.ctx.wCurVidFormat, wVidColorFmt);
		dwChangeStatus |= COLOR_FORMAT_DIFF_MASK;
		mportCtx->videoCtx.ctx.wCurVidFormat = wVidColorFmt;
#if HDMRIX_422_PATCH
		if (HDMI_IN_YUV422 == wVidColorFmt) {
			if (!pPortCtx->isPatchRun_422) {
				pPortCtx->isPatchRun_422 = true;
				HdmiRx_Toggle_PD_IDCLK_Reset();
			}
		}
#endif
	}

	// check VRR mode change
	bVrr = (HdmiRx_Video_GetVRRMode()) ? true : false;
	if (IS_DIFF(mportCtx->videoCtx.ctx.bVrrMode, bVrr)) {
		PORT_INF("%s: VRR mode changed from 0x%x to 0x%x\n", __func__, mportCtx->videoCtx.ctx.bVrrMode, bVrr);
		dwChangeStatus |= VRR_MODE_DIFF_MASK;
		mportCtx->videoCtx.ctx.bVrrMode = bVrr;
	}

	// check HDR10+ mode change
	bHDR10Plus = HdmiRxPacket_GetHdr10Plus_Status(mportCtx->PacketCtx);
	if (IS_DIFF(mportCtx->videoCtx.ctx.bHDR10Plus, bHDR10Plus)) {
		PORT_INF("%s: HDR10+ mode changed from 0x%x to 0x%x\n", __func__, mportCtx->videoCtx.ctx.bHDR10Plus, bHDR10Plus);
		dwChangeStatus |= HDR10_PLUS_MODE_DIFF_MASK;
		mportCtx->videoCtx.ctx.bHDR10Plus = bHDR10Plus;
	}

	// check signal format change
	if (IS_DIFF(mportCtx->videoCtx.ctx.dwCurSignal, dwNewSignal)) {
		PORT_INF("%s: Signal changed from 0x%x to 0x%x\n", __func__, mportCtx->videoCtx.ctx.dwCurSignal, dwNewSignal);
		dwChangeStatus |= SIGNAL_DIFF_MASK;
		mportCtx->videoCtx.ctx.dwCurSignal = dwNewSignal;
	}

	// check frame rate change
	if (IS_DIFF(mportCtx->videoCtx.ctx.dwCurFrameRate, dwNewFrameRate)) {
		PORT_INF("%s: Frame rate changed from 0x%x to 0x%x\n", __func__, mportCtx->videoCtx.ctx.dwCurFrameRate, dwNewFrameRate);
		dwChangeStatus |= FRAME_RATE_DIFF_MASK;
		mportCtx->videoCtx.ctx.dwCurFrameRate = dwNewFrameRate;
	}

	// check EDR mode change
	if (IS_DIFF(mportCtx->videoCtx.ctx.bCurEdrMode, mportCtx->videoCtx.ctx.bNewEdrMode)) {
		PORT_INF("%s: EDR mode changed from 0x%x to 0x%x\n", __func__, mportCtx->videoCtx.ctx.bCurEdrMode, mportCtx->videoCtx.ctx.bNewEdrMode);
		dwChangeStatus |= EDR_MODE_DIFF_MASK;
		mportCtx->videoCtx.ctx.bCurEdrMode = mportCtx->videoCtx.ctx.bNewEdrMode;
	}

	// get HDR EOTF
	HdrMode = HdmiRxPacket_GetHdrEOTF();
	if (IS_DIFF(mportCtx->videoCtx.ctx.CurHdrMode, HdrMode)) {
		PORT_INF("%s: HDR Mode changed from %d to %d\n", __func__, mportCtx->videoCtx.ctx.CurHdrMode, HdrMode);
		dwChangeStatus |= HDR_MODE_DIFF_MASK;
		mportCtx->videoCtx.ctx.CurHdrMode = HdrMode;
		// tHdmiRxContext.tVideoCtx.dwLastHdrEventTime = osal_time_get_tick_count();
	}

	// check DbVision LowLatency
	if (IS_DIFF(mportCtx->videoCtx.ctx.bCurDbVisionLowLatencyMode, mportCtx->videoCtx.ctx.bNewDbVisionLowLatencyMode)) {
		PORT_INF("%s: DbVision LowLatency mode changed from 0x%x to 0x%x\n", __func__, mportCtx->videoCtx.ctx.bCurDbVisionLowLatencyMode, mportCtx->videoCtx.ctx.bNewDbVisionLowLatencyMode);
		dwChangeStatus |= DOBLY_LOWLATENCY_DIFF_MASK;
		mportCtx->videoCtx.ctx.bCurDbVisionLowLatencyMode = mportCtx->videoCtx.ctx.bNewDbVisionLowLatencyMode;
	}

	if (dwChangeStatus == HDMIRX_SIGNAL_NO_CHANGE) {
		if (HDMIRx_CompareTickCount(mportCtx->videoCtx.DetectSignalValidTick, HDMIRX_DETECT_SIGNAL_TICK)) {
			// signal stable
			//log_i("DetectSignalValidTick = %d\n", DetectSignalValidTick);
			if (mportCtx->videoCtx.DetectSignalValidTick) {
#if HDMRIX_422_PATCH
				pPortCtx->isPatchRun_422 = false;
#endif
				// set signal
				mportCtx->videoCtx.ctx.eVideoState = HDMIRX_VIDEO_STATE_STABLE;
				if (pPortCtx->isActivePort) {
					if ((AI_SIGNAL_MODE_UNKNOW != mportCtx->videoCtx.ctx.dwCurSignal) && (AI_SIGNAL_MODE_NO_SIGNAL != mportCtx->videoCtx.ctx.dwCurSignal)) {
						HdmiRx_SYNCEnableOutput(true);
						// Enable video output before set signal
						HDMIRx_DisplayModuleCtx_SetMute(false, true);
						HDMIRx_DisplayModuleCtx_SetSignal(
							ConvertTo_PC_SignalID((VIDEO_MODE_DVI == mportCtx->videoCtx.ctx.CurVideoMode) ? true : false,
												mportCtx->videoCtx.ctx.dwCurSignal),
						pPortCtx);
						pPortCtx->PortPathState = PortPathState_VideoOut;
						PORT_INF("%s: prot:%d set signal ID:0x%x framerate:0x%x tick=%d\n", __func__, PortID, mportCtx->videoCtx.ctx.dwCurSignal, mportCtx->videoCtx.ctx.dwCurFrameRate,
							HDMIRx_GetCurrentTick());
					}
				}
			}

			mportCtx->videoCtx.DetectSignalValidTick = 0;
		}
	} else {
		mportCtx->videoCtx.ctx.dwLastChangeStatus |= dwChangeStatus;
		mportCtx->videoCtx.ctx.eVideoState = HDMIRX_VIDEO_STATE_UNSTABLE;
		mportCtx->videoCtx.DetectSignalValidTick = HDMIRx_GetCurrentTick();
	}

	return true;
}


void PacketContextclear(U8 PortID)
{
	struct HDMIRx_PortContext *mportCtx;

	mportCtx = HDMIRx_get_portCtx(PortID);
	if (mportCtx == NULL)
		return;

	memset(&(mportCtx->PacketCtx.ctx), 0, sizeof(ThdmiRxPacketContext));
}

/***********************************************
Audio Ctx
***********************************************/
void audioContextclear(U8 PortID)
{
	struct HDMIRx_PortContext *mportCtx;

	mportCtx = HDMIRx_get_portCtx(PortID);
	if (mportCtx == NULL)
		return;

	memset(&(mportCtx->audioCtx.ctx), 0, sizeof(ThdmiRxAudioContext));
	memset(&(mportCtx->audioCtx.AudioPllDividerParams), 0, sizeof(ThdmiRxAudioPllDividerParams));
	mportCtx->audioCtx.sui32PrevN = 0;
	mportCtx->audioCtx.sui32PrevFs = 0;
}

void HDMIRx_AudioContextinit(U8 PortID)
{
	audioContextclear(PortID);
	// get audio pll base
	//this->Audio_PLL_Base = this->Audio_DriverCallBack.Audio_GetAudioPLLBase();
}

bool AudioMonitor(void *pPortContex)
{
	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;
	U8 data;
	U32 RegBase = pPortCtx->LinkBase;
	U32 audio_pll_base = pPortCtx->audioCtx.Audio_PLL_Base;
	// check audio type
	data = HdmiRx_Audio_GetType();

	if (data != pPortCtx->audioCtx.ctx.audioType) {
		PORT_INF("%s: Audio type changed from 0x%x to 0x%x\n", __func__, pPortCtx->audioCtx.ctx.audioType, data);
		pPortCtx->audioCtx.ctx.audioType = data;
		HdmiRx_Audio_SetAPLL(&pPortCtx->audioCtx);

		// patch for HBR support
		HdmiRx_Audio_HBR_Patch(data);
	}

	// update audio fs from APLL
	HdmiRx_Audio_GetFsFromAPLL(&pPortCtx->audioCtx);

	return true;
}

void videoContextclear(U8 PortID)
{
	struct HDMIRx_PortContext *mportCtx;

	mportCtx = HDMIRx_get_portCtx(PortID);
	if (mportCtx == NULL)
		return;

	memset(&(mportCtx->videoCtx.ctx), 0, sizeof(ThdmiRxVideoContext));
	memset(&(mportCtx->videoCtx.TmdsCtx), 0, sizeof(ThdmiRxTmdsCtx));
	memset(&(mportCtx->videoCtx.SCDCContext), 0, sizeof(ThdmiRxSCDCContext));
	mportCtx->videoCtx.DetectTimingValidTick = 0;
	mportCtx->videoCtx.DetectSignalValidTick = 0;
}

void HDMIRx_PortCtx_Contextclear(U8 PortID)
{
	videoContextclear(PortID);
	audioContextclear(PortID);
	PacketContextclear(PortID);
}

void HDMIRx_PortContextinit(U8 port_id, struct HDMIRx_PortContext *core)
{
	core_portCtx[port_id] = core;
	memset(core_portCtx[port_id], 0, sizeof(struct HDMIRx_PortContext));

	HDMIRx_PortCtx_Contextclear(port_id + 1);
#if HDMRIX_422_PATCH
	core_portCtx[port_id]->isPatchRun_422 = false;
#endif
}
