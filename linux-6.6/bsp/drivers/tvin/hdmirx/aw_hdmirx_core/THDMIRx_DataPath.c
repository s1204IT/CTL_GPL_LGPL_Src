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
#include "THDMIRx_DataPath.h"
#include "aw_hdmirx_core_drv.h"

struct THDMIRx_DataPath *core_datapath;

void HDMIRx_DataPath_SetARCEnable(bool bOn)
{
	HdmiRx_SetARCEnable(bOn);
}

void HDMIRx_DataPath_SetARCTXPath(ARCTXPath_e arc_tx_path)
{
	HdmiRx_SetARCTXPath(arc_tx_path);
}

void HDMIRx_DataPath_Connect_DDC_Path(struct THDMIRx_Port *pPort)
{
	if (PortPathState_VideoOut == HDMIRx_Port_GetPortPathState(pPort))
		return;

	// clear ctx
	THDMIRx_Port_Clear_SW_Context(pPort);
}

void HDMIRx_DataPath_Connect_Link_Path(struct THDMIRx_Port *pPort)
{
	hdmirx_inf("PortCtx.PortPathState %d\n", HDMIRx_Port_GetPortPathState(pPort));

	if (PortPathState_VideoOut == HDMIRx_Port_GetPortPathState(pPort))
		return;

	HDMIRx_Port_SendSwitchSourceEvent(pPort, true);
	// set ddc tmds port select
	hdmirx_inf("%s: DDC and PHY select prot %d\n", __func__, pPort->PortCtx.PortID);
	HdmiRx_Port_Select(pPort->PortCtx.PortID);
	// scdc reset
	HdmiRx_SCDC_Reset();
	HdmiRx_PHY_Reset();
	// hdcp ddc reset
	HdmiRx_Reset_HDCP_DDC();

	HdmiRx_Toggle_PD_IDCLK_Reset();

	HDMIRx_Port_SetHDCPTaskIsActive(pPort, true);
	HDMIRx_Port_SetSMTaskIsActive(pPort, true);
	// send HPD
	HDMIRx_Port_SendHPDEvent(pPort);
}

void HDMIRx_DataPath_Disconnect_Link_Path(struct THDMIRx_Port *pPort)
{
	HDMIRx_Port_SetHDCPTaskIsActive(pPort, false);
	HDMIRx_Port_SetSMTaskIsActive(pPort, false);

	HDMIRx_Port_SendSwitchSourceEvent(pPort, false);
}

void HDMIRx_DataPath_Connect_VideoOut_Path(struct THDMIRx_Port *pPort)
{
	HdmiRx_SYNCEnableOutput(true);
	HdmiRx_VideoEnableOutput(true);
	if (PortPathState_VideoOut == HDMIRx_Port_GetPortPathState(pPort)) {
		// init detect
		pPort->PortCtx.videoCtx.DetectTimingValidTick = HDMIRx_GetCurrentTick();
		pPort->PortCtx.videoCtx.DetectSignalValidTick = HDMIRx_GetCurrentTick();
	}
}

void HDMIRx_DataPath_Disconnect_VideoOut_Path(struct THDMIRx_Port *pPort)
{
	HDMIRx_Port_SetPortPathState(pPort, PortPathState_LinkConnected);
}

void HDMIRx_DataPath_Connect_AudioOut_Path(struct THDMIRx_Port *pPort)
{
	// only one audio pll for four link,so should do audio reset when switch port
	HdmiRx_Audio_ResetAudioBlock(&pPort->PortCtx.audioCtx);
}

void HDMIRx_DataPath_SetPortArray(struct THDMIRx_Port *pPort)
{
	static U8 port_index;
	if (port_index >= HDMI_PORT_NUM)
		return;
	core_datapath->pPortArray[port_index] = pPort;
	port_index++;
}

void HDMIRx_DataPath_Path_Init(void)
{
	// MV_LIB init
	HdmiRx_MV_LIB_EQ_Init();

	// init all link
	HdmiRx_MAC_Init();
	HdmiRx_PHY_Init();
	HdmiRx_EQ_Init();
}

void THDMIRx_DataPathinit(struct THDMIRx_DataPath *core)
{
	core_datapath = core;
	HDMIRx_DataPath_Path_Init();
	hdmirx_inf("THDMIRx_DataPath start!!\n");
}

