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
#ifndef _THDMIRX_PORT_H_
#define _THDMIRX_PORT_H_
#include "THDMIRx_System.h"
#include "THDMIRx_PortCtx.h"
#include "THDMIRx_Event.h"
#include "HDCP1.4/THDMIRx_HDCP14.h"

#include "hardware/THDMIRx_TV_Driver.h"

#include "THDMIRx_DisplayModuleCtx.h"

struct THDMIRx_Port {
	// HDCP
	struct THDMIRx_HDCP14 pHDMIRx_HDCP14;

	struct HDMIRx_PortContext PortCtx;
	struct THDMIRx_Event pHDMIRxEvent;
	// support from display module
	struct THDMIRx_DisplayModuleCtx *pDisplayModuleCtx;
};

/*
struct THDMIRx_Status {
	bool HDCP_TaskActive;
	bool StateMachine_TaskActive;
	bool Event_TaskActive;
	bool 5VState;
	bool SignalValid;
	bool PhyCDRLock;
	HdmiRxState_e HdmiRxState;
};
*/

ssize_t THDMIRx_Port_Dump_Status(char *buf);

PortPathState_e HDMIRx_Port_GetPortPathState(struct THDMIRx_Port *pActivePort);

void HDMIRx_Port_SetPortPathState(struct THDMIRx_Port *pPort, PortPathState_e state);

void THDMIRx_Port_Clear_SW_Context(struct THDMIRx_Port *pPort);

void HDMIRx_Port_SetIsActivePort(struct THDMIRx_Port *pPort, bool isActive);

bool HDMIRx_Port_GetHDCPTaskIsActive(struct THDMIRx_Port *pPort);

void HDMIRx_Port_SetHDCPTaskIsActive(struct THDMIRx_Port *pPort, bool isActive);

bool HDMIRx_Port_GetHDCPTaskIsActive(struct THDMIRx_Port *pPort);

void HDMIRx_Port_GetHDCP14Status(THdcp14Info *pHdcp14Info);

void HDMIRx_Port_SetSMTaskIsActive(struct THDMIRx_Port *pPort, bool isActive);

bool HDMIRx_Port_GetSMTaskIsActive(struct THDMIRx_Port *pPort);

HdmiRxState_e HDMIRx_Port_GetHdmiRxState(struct THDMIRx_Port *pPort);

void HDMIRx_Port_SetRemap(bool enable);

void HDMIRx_Port_SetHPDTimeInterval(U32 val);

void HDMIRx_Port_SendHPDEvent(struct THDMIRx_Port *pPort);

void HDMIRx_Port_SendSwitchSourceEvent(struct THDMIRx_Port *pPort, bool isActive);

struct HDMIRx_PortContext *THDMIRx_Port_GetPortContext(struct THDMIRx_Port *pActivePort);

void HDMIRx_Port_EventTask(U8 PortID);

void HDMIRx_Port_HDCPTask(U8 PortID);

void HDMIRx_Port_StateMachineTask(U8 PortID);

void THDMIRx_Portinit(U8 port_id, struct THDMIRx_Port *core, struct THDMIRx_DisplayModuleCtx *_pDisplayModuleCtx);

#endif /* _THDMIRX_PORT_H_ */
