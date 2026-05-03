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
#include "THDMIRx_DisplayModuleCtx.h"
#include "hardware/THDMIRx_TV_Driver.h"
#include "hardware/THDMIRx_TV_Edid_Driver.h"

extern bool SigmaHDR10enabled;
static BLOCKING_NOTIFIER_HEAD(hdmirx_notifier_list);

DisplayModuleCallBack_t DisplayModuleCallBack = {
	DisplayModule_GetPolarity, DisplayModule_GetPhase, DisplayModule_SetPhase, DisplayModule_SetPLL, DisplayModule_GetAllPort_5VState,
};

static StructTHDMI_ModeList pModeList[] = {
	{AI_SIGNAL_MODE_720_480I,  AI_SIGNAL_FRAMERATE_60,    0x35A, 0x106, 0x2D0, 0xF0,  0X3E, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  59940, 0X0,  15734, 0, 0,  1, 1, 0, 0, 0, 1, 1}, //0
	{AI_SIGNAL_MODE_480P,      AI_SIGNAL_FRAMERATE_60,    0x35A, 0x20D, 0x2D0, 0x1E0, 0X3E, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  59940, 0X0,  34469, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //2
	{AI_SIGNAL_MODE_576I,	   AI_SIGNAL_FRAMERATE_50,    0x360, 0x138, 0x2D0, 0x120, 0x3F, 0x2, 0x1,	0x1,   0x0, 0x0, 0x0,  50000, 0x0,  15625, 0, 0,  1, 1, 0, 0, 0, 1, 1}, //4
	{AI_SIGNAL_MODE_576P,	   AI_SIGNAL_FRAMERATE_50,    0x360, 0x271, 0x2D0, 0x240, 0x40, 0x4, 0x1,	0x1,   0x0, 0x0, 0x0,  50000, 0x0,  31250, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //6
	{AI_SIGNAL_MODE_720P,	   AI_SIGNAL_FRAMERATE_50,    0x7BC, 0x2EE, 0x500, 0x2D0, 0X28, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  50000, 0X0,  37500, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //7
	{AI_SIGNAL_MODE_720P,	   AI_SIGNAL_FRAMERATE_60,    0x672, 0x2EE, 0x500, 0x2D0, 0X28, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  60000, 0X0,  45000, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //8
	{AI_SIGNAL_MODE_1080I,	   AI_SIGNAL_FRAMERATE_50,    0xA50, 0x233, 0x780, 0x21C, 0X42, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  50000, 0X0,  28125, 0, 0,  1, 1, 0, 0, 0, 1, 1}, //9
	{AI_SIGNAL_MODE_1080I,	   AI_SIGNAL_FRAMERATE_60,    0x898, 0x233, 0x780, 0x21C, 0X2C, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  60000, 0X0,  33750, 0, 0,  1, 1, 0, 0, 0, 1, 1}, //10
	{AI_SIGNAL_MODE_1080P,	   AI_SIGNAL_FRAMERATE_24,    0xABE, 0x465, 0x780, 0x438, 0X2C, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  24000, 0X0,  27000, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //11
	{AI_SIGNAL_MODE_1080P,	   AI_SIGNAL_FRAMERATE_50,    0xA50, 0x465, 0x780, 0x438, 0X2C, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  50000, 0X0,  28125, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //12
	{AI_SIGNAL_MODE_1080P,	   AI_SIGNAL_FRAMERATE_60,    0x898, 0x465, 0x780, 0x438, 0X2C, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  60000, 0X0,  67500, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //14
	{AI_SIGNAL_MODE_1080P,	   AI_SIGNAL_FRAMERATE_25,    0xA50, 0x465, 0x780, 0x438, 0X2C, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  25000, 0X0,  56250, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //16
	{AI_SIGNAL_MODE_1080P,	   AI_SIGNAL_FRAMERATE_30,    0x898, 0x465, 0x780, 0x438, 0X2C, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  30000, 0X0,  33750, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //17
	{AI_SIGNAL_MODE_640_480,   AI_SIGNAL_FRAMERATE_60,    0x320, 0x20D, 0x280, 0x1E0, 0X60, 0X2, 0X1,	0X1,   0X0, 0X0, 0X0,  59940, 0X0,  31469, 0, 0x21, 0, 1, 0, 1, 0, 1, 1}, //27
	{AI_SIGNAL_MODE_800_600,   AI_SIGNAL_FRAMERATE_60,    0x420, 0x274, 0x320, 0x258, 0X80, 0X4, 0X1,	0X1,   0X1, 0X1, 0X0,  60320, 0X0,  37880, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //36
	{AI_SIGNAL_MODE_1280_720,  AI_SIGNAL_FRAMERATE_60,    0x680, 0x2EC, 0x500, 0x2D0, 0X80, 0X5, 0X1,	0X1,   0X0, 0X1, 0X0,  60000, 0X0,      0, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //58
	{AI_SIGNAL_MODE_1280_768,  AI_SIGNAL_FRAMERATE_60,    0x680, 0x31E, 0x500, 0x300, 0X20, 0X7, 0X1,	0X1,   0X0, 0X1, 0X0,  59870, 0X0,  47776, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //62
	{AI_SIGNAL_MODE_1280_800,  AI_SIGNAL_FRAMERATE_60,    0x690, 0x33F, 0x500, 0x320, 0X3E, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  59810, 0X0,  49702, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //65
	{AI_SIGNAL_MODE_1280_960,  AI_SIGNAL_FRAMERATE_60,    0x708, 0x3E8, 0x500, 0x3C0, 0X70, 0X3, 0X1,	0X1,   0X1, 0X1, 0X0,  60000, 0X0,      0, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //69
	{AI_SIGNAL_MODE_1280_1024, AI_SIGNAL_FRAMERATE_60,    0x698, 0x42A, 0x500, 0x400, 0X70, 0X3, 0X1,	0X1,   0X1, 0X1, 0X0,  60020, 0X0,  63980, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //73
	{AI_SIGNAL_MODE_1360_765,  AI_SIGNAL_FRAMERATE_60,    0x700, 0x31B, 0x550, 0x2FD, 0X70, 0X6, 0X1,	0X1,   0X1, 0X1, 0X0,  60000, 0X0,      0, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //77
	{AI_SIGNAL_MODE_1360_768,  AI_SIGNAL_FRAMERATE_60,    0x700, 0x31E, 0x550, 0x300, 0X8F, 0X3, 0X1,	0X1,   0X1, 0X1, 0X0,  60015, 0X0,  47712, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //78
	{AI_SIGNAL_MODE_1366_768,  AI_SIGNAL_FRAMERATE_59_94, 0x714, 0x331, 0x556, 0x300, 0X90, 0X5, 0X1,	0X1,   0X1, 0X1, 0X0,  59940, 0X0,      0, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //79
	{AI_SIGNAL_MODE_1440_900,  AI_SIGNAL_FRAMERATE_60,    0x770, 0x3A6, 0x5A0, 0x384, 0X3E, 0X4, 0X1,	0X1,   0X0, 0X1, 0X0,  59887, 0X0,  55935, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //80
	{AI_SIGNAL_MODE_1440_900,  AI_SIGNAL_FRAMERATE_59_94, 0x640, 0x39E, 0x5A0, 0x384, 0X3E, 0X4, 0X1,	0X1,   0X1, 0X0, 0X0,  59901, 0X0,  55469, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //81
	{AI_SIGNAL_MODE_1680_1050, AI_SIGNAL_FRAMERATE_60,    0x8C0, 0x441, 0x690, 0x41A, 0XB0, 0X6, 0X1,	0X1,   0X1, 0X1, 0X0,  59954, 0X0,  65290, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //84
	{AI_SIGNAL_MODE_1680_1050, AI_SIGNAL_FRAMERATE_59_94, 0x730, 0x438, 0x690, 0x41A, 0X3E, 0X4, 0X1,	0X1,   0X0, 0X0, 0X0,  59954, 0X0,  15734, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //85
	{AI_SIGNAL_MODE_1600_1200, AI_SIGNAL_FRAMERATE_60,    0x870, 0x4E2, 0x640, 0x4B0, 0XC0, 0X3, 0X1,	0X1,   0X1, 0X1, 0X0,  60000, 0X0,  75000, 0, 0,  1, 1, 0, 1, 0, 1, 0}, //86
	{AI_SIGNAL_MODE_1920_1080, AI_SIGNAL_FRAMERATE_60,    2200,  1125,  1920,  1080,  0X2C, 0X5, 0X1,  0X1,   0X1, 0X0, 0X0,  60000, 0X0,  67500, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //87
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_24,    0XABE, 0X8CA, 0,     0,     0X2C,  0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  24000, 0X2D, 54000, 0, 0,  0, 1, 0, 1, 0, 0, 1}, //88
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_30,    0X898, 0X8CA, 0,     0,     0X2C,  0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  30000, 0X2D, 67500, 0, 0,  1, 1, 0, 0, 0, 1, 1}, //89
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_50,    0X7BC, 0X5DC, 0x500, 0x2D0, 0X28, 0X5, 0X1,  0X1,   0X0, 0X0, 0X0,  50000, 0X1E, 75000, 0, 0,  0, 1, 0, 1, 0, 0, 1}, //90
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_60,    0X672, 0X5DC, 0x500, 0x2D0, 0X28, 0X5, 0X1,  0X1,   0X0, 0X0, 0X0,  60000, 0X1E, 90000, 0, 0,  0, 1, 0, 1, 0, 0, 1}, //91
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_24,    0X157C, 0X465, 0, 0, 0X2C, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  24000, 0X0,  27000, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //92
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_30,    0X1130, 0X465, 0, 0, 0X2C, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  30000, 0X0,  33750, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //93
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_50,    0XF78, 0X2EE, 0, 0, 0X28, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  50000, 0X0,  33750, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //94
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_60,    0XCE4, 0X2EE, 0, 0, 0X28, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  60000, 0X0,  45000, 0, 0,  1, 1, 0, 0, 0, 0, 1}, //95
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_60,    0X672, 0X2EA, 0, 0, 0X28, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  60000, 0X0,  45000, 0, 0,  1, 1, 0, 1, 0, 0, 1}, //96
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_25,    0XA50, 0X8CA, 0, 0, 0X3E, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  25000, 0X44, 56250, 0x461, 0xD,  0, 1, 0, 1, 0, 0, 1}, //97
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_25,    0XA50, 0X8CA, 0, 0, 0X3E, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  25000, 0X2D, 56250, 0x461, 0x23, 0, 1, 0, 1, 0, 0, 1}, //98
	{AI_SIGNAL_MODE_1080P,     AI_SIGNAL_FRAMERATE_30,    0X898, 0X8CA, 0, 0, 0X3E, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  30000, 0X44, 67500, 0x461, 0xD, 0, 1, 0, 1, 0, 0, 1}, //99
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_24,    0XCE4, 0X2EE, 0X500, 0X2D0, 0X28, 0X5, 0X1,  0X1,   0X0, 0X0, 0X0,  24000, 0X1E, 18000, 0x2E9, 0x14, 0, 1, 0, 1, 0, 0, 1}, //100
	{AI_SIGNAL_MODE_720P,      AI_SIGNAL_FRAMERATE_30,    0XCE4, 0X2EE, 0X500, 0X2D0, 0X28, 0X5, 0X1,  0X1,   0X0, 0X0, 0X0,  30000, 0X1E, 22500, 0x2E9, 0x14, 0, 1, 0, 1, 0, 0, 1}, //101
	{AI_SIGNAL_MODE_1920_1200, AI_SIGNAL_FRAMERATE_59_94, 0X820, 0X4D3, 0x780, 0x4B0, 0X3E, 0X4, 0X1,  0X1,   0X1, 0X0, 0X0,  59900, 0X0,  64674, 0, 0,  1, 1, 0, 1, 0, 1, 0}, //106
	{AI_SIGNAL_MODE_1366_768,  AI_SIGNAL_FRAMERATE_60,    0X5DC, 0X320, 0x556, 0x300, 0X38, 0X3, 0X1,  0X1,   0X1, 0X1, 0X0,  59950, 0X0,  74038, 0, 0,  1, 1, 0, 1, 0, 1, 1}, //107
	{AI_SIGNAL_MODE_2160P3840, AI_SIGNAL_FRAMERATE_25,    0x14A0, 0x8CA, 0xF00, 0x870, 0X58, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  25000, 0X0,  54000, 0, 0,  1, 0, 0, 1, 0, 1, 1}, //125
	{AI_SIGNAL_MODE_2160P3840, AI_SIGNAL_FRAMERATE_30,    0x1130, 0x8CA, 0xF00, 0x870, 0X58, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  30000, 0X0,  54000, 0, 0,  1, 0, 0, 1, 0, 1, 1}, //126
	{AI_SIGNAL_MODE_2160P4096, AI_SIGNAL_FRAMERATE_24,    0x157C, 0x8CA, 0x1000, 0x870, 0X58, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  24000, 0X0,  54000, 0, 0,  1, 0, 0, 1, 0, 1, 1}, //129
	{AI_SIGNAL_MODE_2160P4096, AI_SIGNAL_FRAMERATE_25,    0x14A0, 0x8CA, 0x1000, 0x870, 0X58, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  25000, 0X0,  54000, 0, 0,  1, 0, 0, 1, 0, 1, 1}, //130
	{AI_SIGNAL_MODE_2160P4096, AI_SIGNAL_FRAMERATE_30,    0x1130, 0x8CA, 0x1000, 0x870, 0X58, 0X4, 0X1,  0X1,   0X0, 0X0, 0X0,  30000, 0X0,  54000, 0, 0,  1, 0, 0, 1, 0, 1, 1}, //131
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

struct THDMIRx_DisplayModuleCtx *core_displaymodulectx;

struct THDMIRx_DisplayModuleCtx *HDMIRx_get_DisplayModuleCtx(void)
{
	return core_displaymodulectx;
}

U32 GetDefaultFrameRate(void)
{
	return AI_SIGNAL_FRAMERATE_60;
}

U32 GetDefaultSignal(void)
{
	return AI_SIGNAL_MODE_NO_SIGNAL;
}

bool IsOutputEnable(U8 ucOutput)
{
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return false;

	return mDisplaymodulectx->bOutputEnabled[ucOutput];
}

U8 GetEnableCount(const bool bOutputEnabled[])
{
	U8 ucTmp = 0;
	int i;

	for (i = 0; i < _OUTPUT_MAX_; i++) {
		if (bOutputEnabled[i] == true)
			ucTmp++;
	}

	return ucTmp;
}

U16 HDMIRx_DisplayModuleCtx_GetColorDepth(void *pPortContex)
{
	CHECK_POINTER(pPortContex, false);
	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;

	return pPortCtx->videoCtx.ctx.tTimingParam.eColorDepth;
}

HRESULT HDMIRx_DisplayModuleCtx_SetSignalFormat(U16 vidFormat, void *pPortContex)
{
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return S_FALSE;

	U16 wColorDepth = HDMIRx_DisplayModuleCtx_GetColorDepth(pPortContex);

	// For hdmi path, no YUV420 for database, always convert to YUV422.
	if (vidFormat == HDMI_IN_YUV420)
		vidFormat = HDMI_IN_YUV422;

	// color depth: 1--8 bit, 2--10bit, 3--12bit, 4--16bit
	// currently, we don't support 16 bit
	if (vidFormat == HDMI_IN_YUV420) {
		if (wColorDepth == 1) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV420_888;
		} else if (wColorDepth == 2) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV420_101010;
		} else {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV420_121212;
		}
	} else if (vidFormat == HDMI_IN_YUV422) {
		if (wColorDepth == 1) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV422_888;
		} else if (wColorDepth == 2) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV422_101010;
		} else {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV422_121212;
	}
	} else if (vidFormat == HDMI_IN_YUV444) {
		if (wColorDepth == 1) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV444_888;
		} else if (wColorDepth == 2) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV444_101010;
		} else {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_YUV444_121212;
		}
	} else {
		if (wColorDepth == 1) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_RGB_888;
		} else if (wColorDepth == 2) {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_RGB_101010;
		} else {
			mDisplaymodulectx->dwSignalFormat = kColorFormat_RGB_121212;
		}
	}

	return S_OK;
}

U32 HDMIRx_DisplayModuleCtx_GetSignal(void)
{
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return AI_SIGNAL_MODE_UNKNOW ;

	return mDisplaymodulectx->dwSignal;
}

THdrScheme HDMIRx_DisplayModuleCtx_GetHdrWorkMode(void *pPortContex)
{
	eHdmiRxEOTF HdrEOTF = hdmiRx_EOTF_SDR;
	CHECK_POINTER(pPortContex, kHdrScheme_Sdr);
	struct HDMIRx_PortContext *pCtx = (struct HDMIRx_PortContext *)pPortContex;
	// EDR mode
	if (pCtx->videoCtx.ctx.bCurEdrMode) {
		return kHdrScheme_DbVision;
	}

	// HDR mode
	HdrEOTF = HdmiRxPacket_GetHdrEOTF();
	switch (HdrEOTF) {
	case hdmiRx_EOTF_HLG:
		return SigmaHDR10enabled ? kHdrScheme_Hlg : kHdrScheme_Hdr10;
	case hdmiRx_EOTF_SMPTE_ST:
		if (HdmiRxPacket_GetHdr10Plus_Status(pCtx->PacketCtx)) {
			VIDEO_INF("%s: HDR10 Plus = TRUE!\n", __func__);
			return kHdrScheme_Hdr10Plus;
		}
		//log_i("HDR10 = TRUE!");
		return SigmaHDR10enabled ? kHdrScheme_Hdr10Sgmip : kHdrScheme_Hdr10;
	default:
		break;
	}

	// Low latency mode
	if (pCtx->videoCtx.ctx.bCurDbVisionLowLatencyMode) {
		return kHdrScheme_DbVisionLowLatency;
	} else {
		return kHdrScheme_Sdr;
	}
}

bool HDMIRx_DisplayModuleCtx_IsPCSignal(void)
{
	return false;
}

bool CheckAttrGroupContains(U16 attr_id, U32 attr_group_id, U32 attr_item_id)
{
	return true;
}

U32 HDMIRx_DisplayModuleCtx_GetSignalColorSpace(void *pPortContex)
{
	CHECK_POINTER(pPortContex, false);
	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;
	U32 item = 0;
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return item;

	eHdmiRxColorMatrix matrix = hdmiRx_ColorMatrix_NotIndicated;
	matrix = HdmiRxPacket_GetColorMatrix(pPortCtx->PacketCtx);

	switch (matrix) {
	case hdmiRx_ColorMatrix_BT601:
		item = AI_SIGNAL_COLORSPACE_BT601;
		break;
	case hdmiRx_ColorMatrix_BT709:
		item = AI_SIGNAL_COLORSPACE_BT709;
		break;
	case hdmiRx_ColorMatrix_BT2020_C:
		item = AI_SIGNAL_COLORSPACE_BT2020_CLYCC;
		break;
	case hdmiRx_ColorMatrix_BT2020_NC:
		item = AI_SIGNAL_COLORSPACE_BT2020_NCLYCC;
		break;
	case hdmiRx_ColorMatrix_NotIndicated:
	default:
		if (HDMIRx_DisplayModuleCtx_GetHdrWorkMode(pPortCtx) != kHdrScheme_Sdr) {
			item = AI_SIGNAL_COLORSPACE_BT2020_NCLYCC;
		} else if (HDMIRx_DisplayModuleCtx_IsPCSignal()) {
			item = AI_SIGNAL_COLORSPACE_BT709;
		} else {
			if (CheckAttrGroupContains(AT_SIGNAL_MODE, AG_SIGNAL_MODE_HSD_VSD, mDisplaymodulectx->dwSignal) ||
				CheckAttrGroupContains(AT_SIGNAL_MODE, AG_SIGNAL_MODE_HMD_VSD, mDisplaymodulectx->dwSignal)) {
				item = AI_SIGNAL_COLORSPACE_BT601;
			} else {
				item = AI_SIGNAL_COLORSPACE_BT709;
			}
		}
	}

	return item;
}

THdrScheme HDMIRx_DisplayModuleCtx_GetTFDHdrEdrWorkMode(void *pPortContex)
{
	THdrScheme workmode = HDMIRx_DisplayModuleCtx_GetHdrWorkMode(pPortContex);

	VIDEO_INF("%s: get tfd hdr/edr mode is %d\n", __func__, workmode);

	return workmode;
}


U16 HDMIRx_DisplayModuleCtx_GetQuantRange(bool *bDefault, void *pPortContex)
{
	CHECK_POINTER(pPortContex, false);
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return false;

	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;
	CHECK_POINTER(bDefault, 0);

	// HDMI mode
	if (VIDEO_MODE_HDMI == pPortCtx->videoCtx.ctx.CurVideoMode) {
		switch (pPortCtx->PacketCtx.ctx.tAVIInfoFrame.ColorSpace.Range) {
		case hdmiRx_LIMITED_RANGE:
			*bDefault = false;
			return LIMITED_RANGE;
		case hdmiRx_FULL_RANGE:
			*bDefault = false;
			return FULL_RANGE;
		default: {
			if (HdmiRxPacket_GetColorFormat(&pPortCtx->PacketCtx.ctx, &pPortCtx->videoCtx) == HDMI_IN_RGB444) {
				if (HDMIRx_DisplayModuleCtx_IsPCSignal()) {
					mDisplaymodulectx->m_bForceFullToLimit = true;
					return FULL_RANGE;
				}
			}
			*bDefault = true;
			return LIMITED_RANGE;
		}
		}
	} else {
		*bDefault = true;
		if (HDMIRx_DisplayModuleCtx_IsPCSignal()) {  // PC signal
			mDisplaymodulectx->m_bForceFullToLimit = true;
			return FULL_RANGE;
		}
	}

	return LIMITED_RANGE;
}

enum hrc_fmt HDMIRx_DisplayModuleCtx_GetHrcFmt(TSignalInfo new_signal_info)
{
	switch (new_signal_info.color_format) {
	case kColorFormat_YUV420_888:
	case kColorFormat_YUV420_1088:
	case kColorFormat_YUV420_101010:
	case kColorFormat_YUV420_121212:
	case kColorFormat_YUV420_101010_P010_Low:
	case kColorFormat_YUV420_101010_P010_High:
		return HRC_FMT_YUV420;

	case kColorFormat_YUV422_888:
	case kColorFormat_YUV422_1088:
	case kColorFormat_YUV422_101010:
	case kColorFormat_YUV422_121212:
		return HRC_FMT_YUV422;

	case kColorFormat_YUV444_888:
	case kColorFormat_YUV444_101010:
	case kColorFormat_YUV444_121212:
		return HRC_FMT_YUV444;

	case kColorFormat_RGB_888:
	case kColorFormat_RGB_101010:
	case kColorFormat_RGB_121212:
		return HRC_FMT_RGB;

	default:
		return HRC_FMT_RGB;
	}
}

enum hrc_depth HDMIRx_DisplayModuleCtx_GetHrcDepth(TSignalInfo new_signal_info)
{
	switch (new_signal_info.color_format) {
	case kColorFormat_YUV420_888:
	case kColorFormat_YUV422_888:
	case kColorFormat_YUV444_888:
	case kColorFormat_RGB_888:
		return HRC_DEPTH_8;

	case kColorFormat_YUV420_101010:
	case kColorFormat_YUV422_101010:
	case kColorFormat_YUV444_101010:
	case kColorFormat_RGB_101010:
	case kColorFormat_YUV420_101010_P010_Low:
	case kColorFormat_YUV420_101010_P010_High:
		return HRC_DEPTH_10;

	case kColorFormat_YUV420_121212:
	case kColorFormat_YUV422_121212:
	case kColorFormat_YUV444_121212:
	case kColorFormat_RGB_121212:
		return HRC_DEPTH_12;

	default:
		return HRC_DEPTH_8;
	}
}

enum hrc_color_space HDMIRx_DisplayModuleCtx_GetHrcColorSpace(TSignalInfo new_signal_info)
{
	switch (new_signal_info.color_space) {
	case AI_SIGNAL_COLORSPACE_BT601:
		return HRC_CS_BT601;

	case AI_SIGNAL_COLORSPACE_BT709:
		return HRC_CS_BT709;

	case AI_SIGNAL_COLORSPACE_BT2020_NCLYCC:
	case AI_SIGNAL_COLORSPACE_BT2020_CLYCC:
		return HRC_CS_BT2020;

	default:
		return HRC_CS_BT601;
	}
}

enum hrc_quantization HDMIRx_DisplayModuleCtx_GetHrcQuantization(TSignalInfo new_signal_info)
{
	if (new_signal_info.ext.hdmi.b_full_range)
		return HRC_QUANTIZATION_FULL;
	else
		return HRC_QUANTIZATION_LIMIT;
}


TSignalInfo msignal_info;

HRESULT HDMIRx_DisplayModuleCtx_SetSignal(U32 _dwSignal, void *pPortContex)
{
	CHECK_POINTER(pPortContex, E_FAIL);
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;
	struct hrc_from_hdmirx_signal hrc_signal_info;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return S_FALSE;

	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;
	struct HDMIRx_PortContext *pCtx = (struct HDMIRx_PortContext *)pPortContex;
	struct HDMIRx_VideoContext *pVideoCtx = &pCtx->videoCtx;
	mDisplaymodulectx->dwSignal = _dwSignal;
	bool bQBit_Default;
	int i;
	EOutputType output;

	VIDEO_INF("%s: SetSignal dwSignal = 0x%x tick=%d\n", __func__, _dwSignal, HDMIRx_GetCurrentTick());

	mDisplaymodulectx->dwHdmiRxOutputChannel = AI_HDMIRX_OUTPUT_CHANNEL_MAIN;

	U16 color_format = HdmiRxPacket_GetColorFormat(&pPortCtx->PacketCtx.ctx, &pPortCtx->videoCtx);
	U32 frame_rate = pPortCtx->videoCtx.ctx.dwCurFrameRate;
	bool bVrr = pPortCtx->videoCtx.ctx.bVrrMode;
	VIDEO_INF("%s: Vrr mode is %d\n", __func__, bVrr);
#if HDMIRX_PR_PATCH
	bool bPRMode = HdmiRxPacket_GetPixelRepetition(&pPortCtx->PacketCtx.ctx);
#endif
	// U32 bInterlace = pHdmiRxActiveLink->HdmiRxVideo_IsInterlace();
	mDisplaymodulectx->dwFrameRate = frame_rate;
	HDMIRx_DisplayModuleCtx_SetSignalFormat(color_format, pPortContex);

	if (pCtx->PortID == 1) {
		mDisplaymodulectx->m_source_id = kSourceId_HDMI_1;
	} else if (pCtx->PortID == 2) {
		mDisplaymodulectx->m_source_id = kSourceId_HDMI_2;
	} else if (pCtx->PortID == 3) {
		mDisplaymodulectx->m_source_id = kSourceId_HDMI_3;
	} else if (pCtx->PortID == 4) {
		mDisplaymodulectx->m_source_id = kSourceId_HDMI_4;
	}

	if (mDisplaymodulectx->dwSignal != AI_SIGNAL_MODE_UNKNOW && mDisplaymodulectx->dwSignal != AI_SIGNAL_MODE_NO_SIGNAL) {
		for (i = 0; i < _OUTPUT_MAX_; i++) {
			output = (EOutputType)i;

			VIDEO_INF("%s: bOutputEnabled[%d]=%x, index=%d dwSignal = 0x%x\n", __func__, output, mDisplaymodulectx->bOutputEnabled[output], i, mDisplaymodulectx->dwSignal);
			if (output == _OUTPUT_PIP_) {
				continue;
			}

			if (!mDisplaymodulectx->bOutputEnabled[output] /*|| bBindImgSlotFull*/) {
				continue;
			}

			U16 vidQrange = HDMIRx_DisplayModuleCtx_GetQuantRange(&bQBit_Default, pPortCtx);
			if (vidQrange == LIMITED_RANGE) {
				mDisplaymodulectx->dwQRange = AI_SIGNAL_RANGE_LIMIT_RANGE;
			} else {
				mDisplaymodulectx->dwQRange = AI_SIGNAL_RANGE_FULL_RANGE;
			}
			// patch for EDR_LL
			U8 ucOutputChannel = OutputChannelMode_Default;
			if ((mDisplaymodulectx->dwHdrWorkModeAttr == AI_ENHANCEDVIDEOMODE_EDR_LL) && (color_format == HDMI_IN_RGB444)) {
				ucOutputChannel = OutputChannelMode_EDR_LL;
			}

			HdmiRx_PHY_SetOutputChannel(ucOutputChannel);
			VIDEO_INF("%s: Set output channel map to 0x%08x\n", __func__, ucOutputChannel);

			// patch end
			/*
			log_i(
			"dwHdrWorkModeAttr=0x%x dwHdmiRxOutputChannel=0x%x dwChannel=0x%x dwSignal=0x%x dwFrameRate=0x%x
			dwSignalFormat=0x%x\n",
			dwHdrWorkModeAttr, dwHdmiRxOutputChannel, dwChannel, dwSignal, dwFrameRate, dwSignalFormat);
			*/
		}
	}
	VIDEO_INF("%s: Set Valid Signal dwSignal:0x%x dwFrameRate:0x%x signal_format:%d color_space:0x%x\n", __func__, \
		mDisplaymodulectx->dwSignal, mDisplaymodulectx->dwFrameRate, mDisplaymodulectx->dwSignalFormat, HDMIRx_DisplayModuleCtx_GetSignalColorSpace(pPortContex));

	TSignalInfo new_signal_info = {
		.source_id = mDisplaymodulectx->m_source_id,
		.signal_id = mDisplaymodulectx->dwSignal,
		.frame_rate = mDisplaymodulectx->dwFrameRate,
		.b_interlace = HdmiRx_Video_IsInterlace(),
		.color_format = mDisplaymodulectx->dwSignalFormat,
		.color_space = HDMIRx_DisplayModuleCtx_GetSignalColorSpace(pPortContex),
		.hdr_scheme = HDMIRx_DisplayModuleCtx_GetTFDHdrEdrWorkMode(pPortContex),
		.p_hdr_static_metadata = NULL,
		.p_hdr_dynamic_metadata = NULL,

		.ext = {
			.hdmi = {.b_full_range = (mDisplaymodulectx->dwQRange == AI_SIGNAL_RANGE_FULL_RANGE) ? true : false,
					.b_timing_valid = true,
					.timing = {
						.h_total = (U32)(pVideoCtx->ctx.tTimingParam.numTotalPixelsPerLine),
						.v_total = pVideoCtx->ctx.tTimingParam.numTotalFrameLines,
						.h_active = (U32)(pVideoCtx->ctx.tTimingParam.numActivePixelsPerLine),
						.v_active = pVideoCtx->ctx.tTimingParam.numActiveLines,
						.hde_start = (U32)(pVideoCtx->ctx.tTimingParam.offsetFirstActivePixel + pVideoCtx->ctx.tTimingParam.hsyncWidth),
						.vde_start = (U32)(pVideoCtx->ctx.tTimingParam.offsetFirstActiveLine + pVideoCtx->ctx.tTimingParam.vsyncWidth), },
						.b_dvi_mode = !((bool)(HDMIRx_DisplayModuleCtx_CheckHDMIOrDVI(pPortCtx))),
						.tmds_clock = pVideoCtx->ctx.tTimingParam.pixelFreq,
						.prmode = HdmiRxPacket_GetPixelRepetition(&pPortCtx->PacketCtx.ctx),
						.audio_fs = pPortCtx->audioCtx.ctx.fs,

					}
				}
			};

	int i1;
	for (i1 = 0; i1 < _OUTPUT_MAX_; i1++) {
		EOutputType output = (EOutputType)i1;
		if (!mDisplaymodulectx->bOutputEnabled[output] /*|| bBindImgSlotFull*/) {
			continue;
		}

		memcpy(&msignal_info, &new_signal_info, sizeof(TSignalInfo));

		memset(&hrc_signal_info, 0, sizeof(hrc_signal_info));
		hrc_signal_info.source_id = new_signal_info.source_id;
		hrc_signal_info.format = HDMIRx_DisplayModuleCtx_GetHrcFmt(new_signal_info);
		hrc_signal_info.depth = HDMIRx_DisplayModuleCtx_GetHrcDepth(new_signal_info);
		hrc_signal_info.csc = HDMIRx_DisplayModuleCtx_GetHrcColorSpace(new_signal_info);
		hrc_signal_info.quantization = HDMIRx_DisplayModuleCtx_GetHrcQuantization(new_signal_info);
		hrc_signal_info.timings.width = new_signal_info.ext.hdmi.timing.h_active;
		hrc_signal_info.timings.height = new_signal_info.ext.hdmi.timing.v_active;
		hrc_signal_info.timings.interlaced = new_signal_info.b_interlace;
		hrc_signal_info.timings.polarities = pVideoCtx->ctx.tTimingParam.bVsyncPolarity;
		hrc_signal_info.timings.pixelclock = pVideoCtx->ctx.tTimingParam.pixelFreq;
		hrc_signal_info.timings.hfrontporch = pVideoCtx->ctx.tTimingParam.hblank - pVideoCtx->ctx.tTimingParam.offsetFirstActivePixel - pVideoCtx->ctx.tTimingParam.hsyncWidth;
		hrc_signal_info.timings.hsync = pVideoCtx->ctx.tTimingParam.hsyncWidth;
		hrc_signal_info.timings.hbackporch = pVideoCtx->ctx.tTimingParam.offsetFirstActivePixel;
		hrc_signal_info.timings.vfrontporch = pVideoCtx->ctx.tTimingParam.vblank - pVideoCtx->ctx.tTimingParam.offsetFirstActiveLine - pVideoCtx->ctx.tTimingParam.vsyncWidth;
		hrc_signal_info.timings.vsync = pVideoCtx->ctx.tTimingParam.vsyncWidth;
		hrc_signal_info.timings.vbackporch = pVideoCtx->ctx.tTimingParam.offsetFirstActiveLine;

		sunxi_hrc_call_notifiers(SIGNAL_CHANGE, &hrc_signal_info);
		if ((AI_SIGNAL_MODE_UNKNOW != new_signal_info.signal_id) && (AI_SIGNAL_MODE_NO_SIGNAL != new_signal_info.signal_id)) {
			HDMIRx_DisplayModuleCtx_SetMute(false, false);
		}
	}

	return S_OK;
}

TSignalInfo *HDMIRx_DisplayModuleCtx_GetSignalInfo(void)
{
	return &msignal_info;
}

bool DisplayModule_CheckAttrGroupContains(u16_t attrID, u32_t attrGroupID, u32_t attrItemID)
{
	return 1;
}

U32 DisplayModule_GetSignalIDFromDB(U8 index)
{
	return pModeList[index].dwSignalID;
}

// get signal format from database by indicated index
U32 DisplayModule_GetFrameRateFromDB(U8 index)
{
	return pModeList[index].dwRefreshRate;
}

U8 HDMIRx_DisplayModuleCtx_GetMatchLevel(void *pHDMIRxTiming, U8 index)
{
	VIDEO_INF("%s: GetMatchLevel index = %d\n", __func__, index);
	ThdmRxTimingParam *pTiming = (ThdmRxTimingParam *)pHDMIRxTiming;
	U8 ucMatchFlag = 0;
	U8 ucLevelIndex = 0;

	U16 vTotal = 0;
	bool bInterlaced = false;
	bool bBackPorchMatch = false;
	U64 VFreqRange;

	// check signal available
	if (false == pModeList[index].bSignalEnable) {
		return _MAX_RGBMATCH_LEVEL_;
	}

	// check signal scan mode
	bInterlaced = (pModeList[index].Progressive) == 0 ? true : false;
	if (bInterlaced != pTiming->bInterlaced) {
		GET_MATCH_LEVEL_LOG_2("bInterlaced mismatch, database:%d current:%d!", bInterlaced, pTiming->bInterlaced);
		return _MAX_RGBMATCH_LEVEL_;
	} else {
		VIDEO_INF("%s: bInterlaced matched!\n", __func__);
	}

	// check H/V count
	if (pModeList[index].bUseActive) {
		// check H active
		if (IS_BETWEEN(pTiming->numActivePixelsPerLine, (pModeList[index].wHActive), HDMI_H_RANGE)) {
			VIDEO_INF("%s: hActive matched!", __func__);
			ucMatchFlag |= _MATCH_HFREQ_;
		} else {
			GET_MATCH_LEVEL_LOG_2("hActive mismatch, database:%d current:%d!", (pModeList[index].wHActive),
									pTiming->numActivePixelsPerLine);
			return _MAX_RGBMATCH_LEVEL_;
		}
		// check V active
		if (IS_BETWEEN(pTiming->numActiveLines, (pModeList[index].wVActive), HDMI_V_RANGE)) {
			VIDEO_INF("%s: VActive matched!\n", __func__);
			ucMatchFlag |= _MATCH_VFREQ_;
		} else {
			GET_MATCH_LEVEL_LOG_2("vActive mismatch, database:%d current:%d!\n", (pModeList[index].wVActive), pTiming->numActiveLines);
			return _MAX_RGBMATCH_LEVEL_;
		}
	} else {
		// check H total
		if (IS_BETWEEN(pTiming->numTotalPixelsPerLine, (pModeList[index].wHFreq), HDMI_H_RANGE)) {
			VIDEO_INF("%s: hTotal matched!\n", __func__);
			ucMatchFlag |= _MATCH_HFREQ_;
		} else {
			GET_MATCH_LEVEL_LOG_2("hTotal mismatch, database:%d current:%d!\n", (pModeList[index].wHFreq), pTiming->numTotalPixelsPerLine);
			return _MAX_RGBMATCH_LEVEL_;
		}

		// check V total
		vTotal = pTiming->bInterlaced ? (pTiming->numTotalFrameLines / 2) : pTiming->numTotalFrameLines;
		if (IS_BETWEEN(vTotal, (pModeList[index].wVFreq), HDMI_V_RANGE)) {
			VIDEO_INF("%s: vTotal matched!", __func__);
			ucMatchFlag |= _MATCH_VFREQ_;
		} else {
			GET_MATCH_LEVEL_LOG_2("vTotal mismatch, database:%d current:%d!\n", (pModeList[index].wVFreq), pTiming->numTotalFrameLines);
			return _MAX_RGBMATCH_LEVEL_;
		}
	}

	// check V frequency
	if (pModeList[index].NotCarefVFreq) {
		ucMatchFlag |= _MATCH_VFREQENCY_;
	} else {
		VFreqRange = pTiming->fieldFreq >= 30000 ? HDMI_V_RANGE_FREQUENCY_ABOVE_30HZ_ : HDMI_V_RANGE_FREQUENCY_;
		if (IS_BETWEEN(pTiming->fieldFreq, pModeList[index].fVFreq, VFreqRange)) {
			ucMatchFlag |= _MATCH_VFREQENCY_;
			VIDEO_INF("%s: fieldFreq matched!, pTiming->fieldFreq = %d, fVFreq = %d\n", __func__, pTiming->fieldFreq, \
			pModeList[index].fVFreq);
		} else {
			hdmirx_inf("%s: fieldFreq mismatch, database:%d current:%d!\n", __func__, pModeList[index].fVFreq, \
			pTiming->fieldFreq);
			return _MAX_RGBMATCH_LEVEL_;
		}
	}

	// skip to check H frequency
	ucMatchFlag |= _MATCH_HFREQENCY_;

	// check polarity
	if (pModeList[index].NotCarePolarity) {
		ucMatchFlag |= _MATCH_HPOL_;
		ucMatchFlag |= _MATCH_VPOL_;
	} else {
		if (pModeList[index].VSyncIsPositive == pTiming->bVsyncPolarity) {
			ucMatchFlag |= _MATCH_VPOL_;
			VIDEO_INF("%s: bVsyncPolarity matched!", __func__);
	} else {
			GET_MATCH_LEVEL_LOG_2("bVsyncPolarity mismatch, database:%d current:%d!\n", pModeList[index].VSyncIsPositive,
						pTiming->bVsyncPolarity);
		}

		if (pModeList[index].HSyncIsPositive == pTiming->bHsyncPolarity) {
			ucMatchFlag |= _MATCH_HPOL_;
			VIDEO_INF("%s: bHsyncPolarity matched!\n", __func__);
		}
	}

	// check sync width
	if (pModeList[index].NotCareSyncWidth) {
		ucMatchFlag |= _MATCH_HWIDTH_;
		ucMatchFlag |= _MATCH_VWIDTH_;
	} else {
		// check H sync width
		if (IS_BETWEEN(pTiming->hsyncWidth, (pModeList[index].wHSyncWidth), H_WIDTH_RANGE)) {
			ucMatchFlag |= _MATCH_HWIDTH_;
			VIDEO_INF("%s: wHSyncWidth matched!", __func__);
		} else {
			GET_MATCH_LEVEL_LOG_2("wHSyncWidth mismatch, database:%d current:%d!\n", (pModeList[index].wHSyncWidth), pTiming->hsyncWidth);
		}

		// skip to check V sync width
		ucMatchFlag |= _MATCH_VWIDTH_;
	}

	// check back porch
	if (pModeList[index].NotCareVBackPorch) {
		bBackPorchMatch = true;
	} else {
		if (IS_BETWEEN(pTiming->offsetFirstActiveLine, (pModeList[index].wVBackPorch), V_BACKPORCH_RANGE)) {
			bBackPorchMatch = true;
			VIDEO_INF("%s: wVBackPorch matched!", __func__);
		} else {
			GET_MATCH_LEVEL_LOG_2("wVBackPorch mismatch, database:%d current:%d!\n", pModeList[index].wVBackPorch, pTiming->hsyncWidth);
			return _MAX_RGBMATCH_LEVEL_;
		}
	}

	// check match level table
	hdmirx_inf("%s: ucMatchFlag is %d bBackPorchMatch : %d\n", __func__, ucMatchFlag, bBackPorchMatch);
	for (ucLevelIndex = 0; ucLevelIndex < _MAX_RGBMATCH_LEVEL_; ucLevelIndex++) {
		if (bBackPorchMatch && (MatchLevelTable[ucLevelIndex] == ucMatchFlag)) {
			hdmirx_inf("%s: Fine match level is %d\n", __func__, ucLevelIndex);
			return ucLevelIndex;
		}
	}
	return _MAX_RGBMATCH_LEVEL_;
}

HRESULT HDMIRx_DisplayModuleCtx_GetSignalIndex(void *pHDMIRxTiming, U8 *index)
{
	CHECK_POINTER(index, E_FAIL);

	U32 i = 0;
	U8 ucBestIndex = 0;
	U8 ucLevel = _MAX_RGBMATCH_LEVEL_;
	U8 ucBestLevel = _MAX_RGBMATCH_LEVEL_;
	for (i = 0; pModeList[i].dwSignalID != 0; i++) {
		ucLevel = HDMIRx_DisplayModuleCtx_GetMatchLevel(pHDMIRxTiming, i);
		// best match level
		if (ucLevel == BEST_MATCH_LEVEL) {
			*index = i;
			hdmirx_inf("%s: Matched best level = %d\n", __func__, ucBestIndex);
			return S_OK;
		}

		// sort a best match level,find least level
		if (ucBestLevel < ucLevel) {
			ucBestLevel = ucLevel;
			ucBestIndex = i;
		}
	}

	// match signal failed
	if (ucBestLevel == _MAX_RGBMATCH_LEVEL_) {
		hdmirx_err("%s: Match signal failed!\n", __func__);
		return E_FAIL;
	} else {
		*index = ucBestIndex;
		hdmirx_inf("%s: Matched index = %d level = %d\n", __func__, ucBestIndex, ucBestLevel);
		return S_OK;
	}
}

// get current signal format and signal index
bool DisplayModule_DetectSignal_ByDB(void *pHDMIRxTiming, U8 *pIndex, U32 *pdwSignal)
{
	CHECK_POINTER(pdwSignal, false);

	// U8 index = 0;
	U32 signal = AI_SIGNAL_MODE_UNKNOW;
	ThdmRxTimingParam *pTiming = (ThdmRxTimingParam *)pHDMIRxTiming;
	if (S_OK == HDMIRx_DisplayModuleCtx_GetSignalIndex(pTiming, pIndex)) {
		// get signal format from database
		signal = DisplayModule_GetSignalIDFromDB(*pIndex);

		// store current signal format and index
		*pdwSignal = signal;

		if (signal != AI_SIGNAL_MODE_NO_SIGNAL) {
			VIDEO_INF("%s: Signal 0x%x detected index=0x%x\n", __func__, signal, *pIndex);
			return true;
		}
	} else {
		*pdwSignal = AI_SIGNAL_MODE_UNKNOW;
	}
	return false;
}

bool HDMIRx_DisplayModuleCtx_CheckHDMIOrDVI(void *pPortContex)
{
	CHECK_POINTER(pPortContex, false);
	struct HDMIRx_PortContext *pPortCtx = (struct HDMIRx_PortContext *)pPortContex;
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return false;

	bool mode = (VIDEO_MODE_DVI == pPortCtx->videoCtx.ctx.CurVideoMode) ? false : true;

	if (mDisplaymodulectx->HDMIOrDVI != mode) {
		//call to app
		//if (DisplayModuleCallBack.pHDMIDVIChangeHandler) DisplayModuleCallBack.pHDMIDVIChangeHandler((mode));

		mDisplaymodulectx->HDMIOrDVI = mode;
	}

	return mDisplaymodulectx->HDMIOrDVI;
}

void HDMIRx_DisplayModuleCtx_SetMute(bool bVideo, bool bAudio)
{
	VIDEO_INF("%s: hdmirx set AV mute:%d %d\n", __func__, bVideo, bAudio);
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return;

	mDisplaymodulectx->videomute = bVideo;
	mDisplaymodulectx->audiomute = bAudio;
	HdmiRx_VideoEnableOutput(!bVideo);
	HdmiRx_AudioEnableOutput(!bAudio);
}

VOID HDMIRx_DisplayModuleCtx_SetBlueScreen(bool bON)
{
	hdmirx_inf("%s: HDMIRX Send BlueScreen, bON=%d\n", __func__, bON);
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;
	EOutputType output;
	int i;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return;

	for (i = 0; i < _OUTPUT_MAX_; i++) {
		output = (EOutputType)i;

		if (!mDisplaymodulectx->bOutputEnabled[output] /*|| bBindImgSlotFull*/)
			continue;
		if (mDisplaymodulectx->m_blue_screen_handle < 0) {
			// m_blue_screen_handle = GetBlueScreen()->ObtainBlueScreenHandle("HDMIRx");
		}
		if (bON) {
			// GetBlueScreen()->EnableBlueScreen(m_blue_screen_handle, "HDMIRx");
		} else {
			// GetBlueScreen()->DisableBlueScreen(m_blue_screen_handle, "HDMIRx");
		}
	}
	return;
}

HRESULT HDMIRx_DisplayModuleCtx_AfterEnable(void)
{
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return S_FALSE;

	mDisplaymodulectx->dwHdmiRxOutputChannel = AI_HDMIRX_OUTPUT_CHANNEL_MAIN;
	return S_OK;
}

HRESULT HDMIRx_DisplayModuleCtx_Enable(EOutputType output, TSourceId source_id)
{
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return S_FALSE;
	mDisplaymodulectx->dwSignal = GetDefaultSignal();
	mDisplaymodulectx->dwFrameRate = GetDefaultFrameRate();
	mDisplaymodulectx->dwSignalFormat = kColorFormat_RGB_888;
	mDisplaymodulectx->dwQRange = AI_SIGNAL_RANGE_FULL_RANGE;
	mDisplaymodulectx->m_source_id = source_id;

	mDisplaymodulectx->dwHdmiRxOutputChannel = AI_HDMIRX_OUTPUT_CHANNEL_MAIN;
	mDisplaymodulectx->bOutputEnabled[output] = true;  // Should be before SetActivePort.

	if (GetEnableCount(mDisplaymodulectx->bOutputEnabled) > 0) {
	} else {
		// set default window
		mDisplaymodulectx->dwSignal = GetDefaultSignal();
		mDisplaymodulectx->dwFrameRate = GetDefaultFrameRate();
		mDisplaymodulectx->dwSignalFormat = kColorFormat_RGB_888;
		mDisplaymodulectx->dwQRange = AI_SIGNAL_RANGE_FULL_RANGE;
		mDisplaymodulectx->dwSignal = AI_SIGNAL_MODE_NO_SIGNAL;
	}
	hdmirx_inf("%s():bOutputEnabled[%d]=%x, dwSignal = 0x%x\n", __FUNCTION__, output, \
		mDisplaymodulectx->bOutputEnabled[output], mDisplaymodulectx->dwSignal);
	return S_OK;
}

HRESULT HDMIRx_DisplayModuleCtx_Disable(EOutputType output)
{
	struct THDMIRx_DisplayModuleCtx *mDisplaymodulectx;

	mDisplaymodulectx = HDMIRx_get_DisplayModuleCtx();
	if (mDisplaymodulectx == NULL)
		return S_FALSE;

	mDisplaymodulectx->bOutputEnabled[output] = false;

	if (GetEnableCount(mDisplaymodulectx->bOutputEnabled) == 0) {
		mDisplaymodulectx->m_source_id = kSourceId_Dummy;
		mDisplaymodulectx->dwSignal = AI_SIGNAL_MODE_NO_SIGNAL;
		//m_3DDevSigType = E_2DSigType;
		mDisplaymodulectx->HDMIOrDVI = 0xFF;
	}

	hdmirx_wrn("%s():bOutputEnabled[%d]=%x, dwSignal = 0x%x\n", __FUNCTION__, output, \
		mDisplaymodulectx->bOutputEnabled[output], mDisplaymodulectx->dwSignal);

	return S_OK;
}

int THDMIRx_DisplayModuleCtxinit(struct THDMIRx_DisplayModuleCtx *core)
{
	int i;
	core_displaymodulectx = core;

	core_displaymodulectx->m_DeviceID = kDeviceId_HDMI1;
	core_displaymodulectx->dwSignal = GetDefaultSignal();
	core_displaymodulectx->m_source_id = kSourceId_Dummy;
	core_displaymodulectx->dwHdmiRxOutputChannel = AI_HDMIRX_OUTPUT_CHANNEL_MAIN;
	core_displaymodulectx->dwSignalFormat = kColorFormat_RGB_888;
	core_displaymodulectx->dwQRange = AI_SIGNAL_RANGE_FULL_RANGE;
	core_displaymodulectx->m_ucLevelDetection = 0;
	//dwCapFormat = AI_V_INCAP_MP_FORMAT_YUV422_101010;
	core_displaymodulectx->dwHdrWorkModeAttr = AI_ENHANCEDVIDEOMODE_SDR;
	core_displaymodulectx->bEQReset = false;
	core_displaymodulectx->dwFrameRate = GetDefaultFrameRate();
	core_displaymodulectx->m_bForceFullToLimit = false;
	core_displaymodulectx->m_blue_screen_handle = -1;
	for (i = 0; i < _OUTPUT_MAX_; i++) {
		core_displaymodulectx->bOutputEnabled[i] = false;
	}

	core_displaymodulectx->HDMIOrDVI = 0xFF;
	core_displaymodulectx->m_bDetect5VValid = false;
	core_displaymodulectx->videomute = true;
	core_displaymodulectx->audiomute = true;

	hdmirx_inf("THDMIRx_DisplayModuleCtx start!!!\n");
	return 0;
}
