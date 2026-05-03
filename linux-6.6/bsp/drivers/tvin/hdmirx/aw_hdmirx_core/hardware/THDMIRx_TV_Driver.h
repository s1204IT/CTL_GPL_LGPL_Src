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
#ifndef _THDMIRX_DRIVER_H_
#define _THDMIRX_DRIVER_H_

#include "../include/display_datatype.h"
#include "../THDMIRx_System.h"
#include "../THDMIRx_Packet.h"

// HDCP22 reauth
#define HDCP22_REAUTH_TIMEOUT_TICK_MS (1000)
// HDCP14 reauth
#define HDCP14_REAUTH_TIMEOUT_TICK_MS (1000)
#define KSVSIZE   5

// port ID
typedef enum _tagHDMIPortID {
	HDMI_PORT1 = 1,
	HDMI_PORT2,
	HDMI_PORT3,
} HDMIPortID_e;

typedef enum _tagMCUPortID {
	MCU_PORT1 = 0,
	MCU_PORT2,
	MCU_PORT3,
} MCUPortID_e;

typedef struct _tagHDMIPortMap {
	TSourceId display_source_id;
	HDMIPortID_e hdmi_port_id;
	MCUPortID_e mcu_port_id;
} HDMIPortMap_t;

/*
enum _tagHDMI_PORT {
	HDMI1_PORT,
	HDMI2_PORT,
	HDMI3_PORT,
	HDMI_PORT_NUM_MAX,
	HDMI_PORT_INVALID = HDMI_PORT_NUM_MAX,
} HDMI_PORT;
*/
//#define HDMI_PORT_NUM	3

#define HDMI_PORT_NUM HDMI_PORT2

typedef enum _tagLinkBaseID {
	LinkBaseID_0,
	LinkBaseID_1,
	LinkBaseID_2,
	LinkBaseID_3,
	LinkBaseID_Max,
	LinkBaseID_Invalid,
} LinkBaseID_e;

typedef enum _tagOutputChannelPath {
	OutputChannelMode_Default,
	OutputChannelMode_EDR_LL,
} OutputChannelPath_e;

// message type
typedef enum _eHdmiRxMsgID {
	MSG_INVALID = -1,

	// common message
	MSG_HDMIRX_COMMON_BEGIN = 0,
	MSG_HDMIRX_COMMON_SET_SIGNAL = 1,
	MSG_HDMIRX_COMMON_END,

	// hdmi related interrupts
	MSG_HDMIRX_INTR_BEGIN = 0x1000,
	MSG_HDMIRX_INTR_PHY_LOCKED = 0x1001,
	MSG_HDMIRX_INTR_PHY_UNLOCKED = 0x1002,
	MSG_HDMIRX_INTR_DVI_LOCKED = 0x1003,
	MSG_HDMIRX_INTR_HDMI_LOCKED = 0x1004,
	MSG_HDMIRX_INTR_PHY_INT_FREQ_DET = 0x1005,
	MSG_HDMIRX_INTR_VTOTAL_CHANGED = 0x1006,

	// packet related interrupts
	MSG_HDMIRX_INTR_PKT_LOCKED = 0x2000,
	MSG_HDMIRX_INTR_PKT_UNLOCKED = 0x2001,
	MSG_HDMIRX_INTR_PKT_DIFF_VSI_LLC = 0x2002,
	MSG_HDMIRX_INTR_PKT_DIFF_GCP = 0x2003,
	MSG_HDMIRX_INTR_PKT_DIFF_SPDI = 0x2004,
	MSG_HDMIRX_INTR_PKT_DIFF_AVI = 0x2005,
	MSG_HDMIRX_INTR_PKT_DIFF_AI = 0x2006,
	MSG_HDMIRX_INTR_PKT_DIFF_ACR = 0x2007,
	MSG_HDMIRX_INTR_PKT_DIFF_ACP = 0x2008,
	MSG_HDMIRX_INTR_PKT_DIFF_GP2 = 0x2009,

	// audio related interrupts
	MSG_HDMIRX_INTR_AUDIO_CS_DIFF = 0x3000,
	MSG_HDMIRX_INTR_AUDIO_FIFO_UNDERRUN = 0x3001,
	MSG_HDMIRX_INTR_AUDIO_FIFO_OVERRUN = 0x3002,
	MSG_HDMIRX_INTR_AUDIO_CHECKSUM_ERROR = 0x3003,
	MSG_HDMIRX_INTR_AUDIO_STATUS_BIT_ERROR = 0x3004,
	MSG_HDMIRX_INTR_AUDIO_PLL_LOCKED = 0x3005,
	MSG_HDMIRX_INTR_END,

	// HDCP related interrupts
	MSG_HDCPRX_INTR_BEGIN = 0x4000,
	MSG_HDCPRX_INTR_AKSV_UPDATED = 0x4001,
	MSG_HDCPRX_INTR_ENC_STOP = 0x4002,
	MSG_HDCPRX_INTR_HDCP22_BCH50_ERROR = 0x4003,
	MSG_HDCPRX_INTR_HDCP14_BCH_ERROR = 0x4004,
	MSG_HDCPRX_INTR_END,

	// MULTI-VIEW message
	MSG_HDMIRX_MULTIVIEW_BEGIN = 0x5000,
	MSG_HDMIRX_MULTIVIEW_END,

	MSG_MAX,
} eHdmiRxMsgID;

typedef enum {
	SPIDF_ARC0 = 0,  // AW choose this path for arc
	SPIDF_IN1,
} ARCTXPath_e;

typedef struct _Edid_state {
	bool bEdidDataReady; // 1: EDID data ready, 0: no EDID data, must download from ARM
	bool bEdidRamReady;  // 1: EDID SRAM ready, 0: EDID SRAM is empty, 1: EDID SRAM is filled with data
	U8 ucEdidHdmi20Setting; // Bit3: HDMI4 use HDMI2.0, Bit2: HDMI3, Bit1: HDMI2, Bit0: HDMI1.
} Edid_state;

typedef struct _THdcp14Info {
	U8 aksv[KSVSIZE];
	U8 bksv[KSVSIZE];
	bool hdcpkey;
	bool auth;
} THdcp14Info;

// base
extern void HdmiRx_set_regbase(uintptr_t reg_base);
extern void HdmiRx_Video_set_regbase(uintptr_t reg_base);
extern void HdmiRx_Audio_set_regbase(uintptr_t reg_base);
extern U32 HdmiRx_GetLinkBaseByID(LinkBaseID_e link_id);
extern U32 HdmiRx_SingleLink_GetHDCPBase(U8 port_id);
extern U32 HdmiRx_GetMVBase(void);
extern U32 HdmiRx_GetPHYBase(void);

//hpd
extern void HdmiRx_SendHPDCMD(U8 cmd, U8 PortID);
extern U8 HDMIPortMapToMCU(U8 PortID);
extern HDMIPortMap_t *HdmiRx_GetPortMap(void);
extern bool HdmiRx_GetPort_HpdState(U8 port_id);

// hdcp1.4
extern void HdmiRx_HDCP14_EnableAKSV_ISR(void);
extern bool HdmiRx_HDCP14_LoadKey(void);
extern void HdmiRx_HDCP14_ISR_Handle(U8 port_id, bool *pAuthResult);
extern void HdmiRx_HDCP14_ReAuthReq(bool isReAuth);
extern void HdmiRx_HDCP14_ReloadKey(void);
extern void HdmiRx_HDCP14_GetStatus(THdcp14Info *pHdcp14Info);

// data path(ddc/tmds)
extern void HdmiRx_MAC_Init(void);
extern void HdmiRx_PHY_Init(void);
extern void HdmiRx_Port_Select(U8 port_id);
extern void HdmiRx_PHY_SetOutputChannel(U8 mode);
extern void HdmiRx_SCDC_Reset(void);
extern void HdmiRx_PHY_Reset(void);
extern void HdmiRx_MV_SCDC_Reset(U32 scdc_bae);
extern void HdmiRx_MVMode_Enable(bool isEnable);  // set ddc path for dual link mode
extern void HdmiRx_MVMode_HDCP_PathEnable(U32 RegBase, bool isEnable);
extern void HdmiRx_MVMode_ConnectLink(U32 base, LinkBaseID_e linkID, U32 portID);
extern void HdmiRx_MVMode_DisconnectLink(U32 base, LinkBaseID_e linkID, U32 portID);
extern bool HdmiRx_MVMode_Link_IsConnect(U32 base, LinkBaseID_e linkID);
extern void HdmiRx_MVMode_EnableOutput(bool isEnable);
extern void HdmiRx_VideoEnableOutput(bool isEnable);
extern void HdmiRx_AudioEnableOutput(bool isEnable);
extern U8 HdmiRx_GetAVMuteState(U32 base);
extern void HdmiRx_SYNCEnableOutput(bool isEnable);
extern void HdmiRx_MVTOP_ICGEnable(U8 port_id, bool isEnable);
extern void HdmiRx_Toggle_PD_IDCLK_Reset(void);
extern void HdmiRx_SetARCEnable(bool bOn);
extern void HdmiRx_SetARCTXPath(ARCTXPath_e arc_tx_path);

// statemachine
extern bool HdmiRx_GetPort_5VState(U8 PortID);
extern bool HdmiRx_GetPort_SignalValid(void);
extern bool HdmiRx_GetPort_PhyCDRLock(void);
extern void HdmiRx_StateMachine_Sleep_PreAction(void);
extern void HdmiRx_StateMachine_Idle_PreAction(void);
extern void HdmiRx_StateMachine_Active_PreAction(void);
extern void HdmiRx_StateMachine_Running_PreAction(void);
extern void HdmiRx_ISR_Enable(bool isEnable);

// video
extern bool HdmiRx_Video_GetVRRMode(void);
extern void HdmiRx_Video_GetTimingParam(void *pTimingData, void *pCtx);
extern bool HdmiRx_Video_TMDS_FreqDetect(void *pVideoCtx);
extern void HdmiRx_Video_UpdateHdmiMode(U8 video_mode, void *pVideoCtx);
extern void HdmiRx_Video_SetVRRMode(bool isEnterVRR);
extern bool HdmiRx_Video_IsInterlace(void);
extern void HdmiRx_Video_UpdatePixelRepetition(U8 PixelRepetition);
extern void HdmiRx_Video_UpdateColorSpaceModel(eHdmiRxColorFmt color_format);
extern void HdmiRx_Video_SCDC_Detect(void *pSCDCContex);
extern bool HdmiRx_Video_ScrambleState(U32 scdc_base);
extern void HdmiRx_Video_MVSpecialCase_Cfg(U32 base, bool above3G);

// audio
extern U32 HdmiRx_Audio_GetAudioPLLBase(void);
extern void HdmiRx_Audio_PowerDownAPLL(void);
extern void HdmiRx_Audio_PowerUpAPLL(void);
extern bool HdmiRx_Audio_CalculateDividerSettings(void *pAudioCtx, void *pSTRetParams);
extern bool HdmiRx_Audio_InitAPLL(void *pAudioContex);
extern bool HdmiRx_Audio_UpdateAPLL(void *pAudioCtx);
extern bool HdmiRx_Audio_ResetAPLL(void);
extern bool HdmiRx_Audio_SetAPLL(void *pAudioContext);
extern bool HdmiRx_Audio_GetFsFromAPLL(void *pAudioContext);
extern bool HdmiRx_Audio_Get_N_CTS(void *pAudioContext);
extern bool HdmiRx_Audio_GetFsFromCS(void *pAudioContext);
extern bool HdmiRx_Audio_ResetAudioBlock(void *pAudioContext);
extern void HdmiRx_Audio_HBR_Patch(U8 audio_type);
extern U8 HdmiRx_Audio_GetType(void);
extern void HdmiRx_Audio_I2S_EnableOutput(bool isEnable);
extern void HdmiRx_Audio_PD_Reset(void);

// packet
extern bool HdmiRx_Packet_Read(eHdmiRxPacketType type, ThdmiRxPacket *pPacket);
extern bool HdmiRx_Packet_ReadGen1(eHdmiRxPacketType type, ThdmiRxPacket *pPacket);
extern bool HdmiRx_Packet_ReadGen2(eHdmiRxPacketType type, ThdmiRxPacket *pPacket);

// ISR
extern void HdmiRx_ScanISR(U8 PortID);

//hdcp
extern void HdmiRx_Reset_HDCP_DDC(void);

// EQ
extern void HdmiRx_MV_LIB_EQ_Init(void);
extern void HdmiRx_EQ_Init(void);
extern void HdmiRx_AEC_Update(bool b6G);
extern void HdmiRx_AEC_Enable(bool bEnable);
extern void HdmiRxFixedEQ_Update(U16 wEQ);

// display module
// ----------------  SVP Pages ------------------------------
// TODO: should be removed. use in hdmi
#define  _PAGE_DATA_SYNC_   0x06940400
#define _PAGE_ADC_ 0x193de800
extern U8 DisplayModule_GetPolarity(void);
extern U8 DisplayModule_GetPhase(void);
extern void DisplayModule_SetPhase(U8 dwPhase);
extern void DisplayModule_SetPLL(U32 dwPLL);
extern U8 DisplayModule_GetAllPort_5VState(void);

#endif /* _THDMIRX_DRIVER_H_ */
