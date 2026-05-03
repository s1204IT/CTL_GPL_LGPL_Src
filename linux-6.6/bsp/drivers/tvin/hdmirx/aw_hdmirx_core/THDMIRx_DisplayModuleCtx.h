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
#ifndef _AW_HDMIRX_CORE_DISPLAYMODULECTX_H_
#define _AW_HDMIRX_CORE_DISPLAYMODULECTX_H_
#include <linux/types.h>
#include <linux/notifier.h>

#include "include/display_datatype.h"
#include "THDMIRx_PortCtx.h"
#include "THDMIRx_Packet.h"
#include "THDMIRx_System.h"
#include "../../hrc/sunxi_hrc_define.h"

#define V_BACKPORCH_RANGE 0x05
#define V_WIDTH_RANGE 0x02
#define H_WIDTH_RANGE 0x0A
#define VGA_V_RANGE 0x08
#define HDTV_V_RANGE 0x20
#define VGA_H_RANGE 0x0A
#define HDTV_H_RANGE 0x10
#define H_RANGE 0x18
#define V_RANGE 0x10
#define HDMI_H_RANGE 0x05
#define HDMI_V_RANGE 0x03
#define HDMI_HW_RANGE 0x05

#define IS_BETWEEN(a, b, sh)      \
	(((a) > (b)) ? (((a) - (b)) <= (sh) ? true : false) : (((b) - (a)) <= (sh) ? true : false))
#define GET_MATCH_LEVEL_LOG_1(_s, _p1)       HDMILOG_1_WITH_CONDITION((index == 0xb), _s, _p1);
#define GET_MATCH_LEVEL_LOG_2(_s, _p1, _p2)  HDMILOG_2_WITH_CONDITION((index == 0xb), _s, _p1, _p2);

typedef U8 (*GetPolarity_CallBack)(void);
typedef U8 (*GetPhase_CallBack)(void);
typedef void (*SetPhase_CallBack)(U8 dwPhase);
typedef void (*SetPLL_CallBack)(U32 dwPLL);
typedef U8 (*GetAllPort_5VState_CallBack)(void);

typedef struct _tagDisplayModuleCallBack {
	GetPolarity_CallBack GetPolarity;
	GetPhase_CallBack GetPhase;
	SetPhase_CallBack SetPhase;
	SetPLL_CallBack SetPLL;
	GetAllPort_5VState_CallBack GetAllPort_5VState;
} DisplayModuleCallBack_t;

typedef struct tagStructTHDMI_ModeList {
	U32          dwSignalID;
	U32          dwRefreshRate;
	U16          wHFreq;
	U16          wVFreq;
	U16          wHActive;
	U16          wVActive;
	U16          wHSyncWidth;
	U16          wVSyncWidth;
	U8           NotCareSyncWidth;
	U8           NotCarePolarity;
	U8           HSyncIsPositive;
	U8           VSyncIsPositive;
	U8           ucPixClk;
	U64          fVFreq;//1hz*1000
	U16          wVSpace;
	U64          fHFreq;//1hz*1000
	U16          wVStart;
	U16          wVBackPorch;
	U8           NotCareVBackPorch;
	U8           NotCarefHFreq;
	U8           NotCarefVFreq;
	U8           Progressive;
	U8           VICInfor;
	U8           bUseActive;
	U8           bSignalEnable;
} StructTHDMI_ModeList;

typedef enum tagHDMI_QRANGE {
	LIMITED_RANGE,
	FULL_RANGE
} HDMI_QRANGE;

struct THDMIRx_DisplayModuleCtx {
	TDeviceId m_DeviceID;
	TSourceId m_source_id;
	TColorFormat dwSignalFormat;
	u32 dwSignal;
	u32 dwFrameRate;
	u32 dwQRange;
	u32 dwHdrWorkModeAttr;
	u32 dwHdmiRxOutputChannel;
	u8 m_ucLevelDetection;
	s32 m_blue_screen_handle;

	int HDMIOrDVI;
	bool bEQReset;
	bool m_bForceFullToLimit;

	bool bOutputEnabled[_OUTPUT_MAX_];
	bool m_bDetect5VValid;  // app will set it to control 5v detect
	bool videomute;
	bool audiomute;

	U8 ucModeCnt;
};

int sunxi_hrc_call_notifiers(unsigned long val, void *data);

U32 HDMIRx_DisplayModuleCtx_GetSignal(void);

bool DisplayModule_CheckAttrGroupContains(u16_t attrID, u32_t attrGroupID, u32_t attrItemID);

HRESULT HDMIRx_DisplayModuleCtx_SetSignal(U32 _dwSignal, void *pPortContex);

TSignalInfo *HDMIRx_DisplayModuleCtx_GetSignalInfo(void);

U32 DisplayModule_GetSignalIDFromDB(U8 index);

U32 DisplayModule_GetFrameRateFromDB(U8 index);

bool DisplayModule_DetectSignal_ByDB(void *pHDMIRxTiming, U8 *pIndex, U32 *pdwSignal);

bool HDMIRx_DisplayModuleCtx_CheckHDMIOrDVI(void *pPortContex);

void HDMIRx_DisplayModuleCtx_SetMute(bool bVideo, bool bAudio);

VOID HDMIRx_DisplayModuleCtx_SetBlueScreen(bool bON);

HRESULT HDMIRx_DisplayModuleCtx_Enable(EOutputType output, TSourceId source_id);

HRESULT HDMIRx_DisplayModuleCtx_Disable(EOutputType output);

HRESULT HDMIRx_DisplayModuleCtx_AfterEnable(void);

int THDMIRx_DisplayModuleCtxinit(struct THDMIRx_DisplayModuleCtx *core);

#endif /* _AW_HDMIRX_CORE_DISPLAYMODULECTX_H_ */
