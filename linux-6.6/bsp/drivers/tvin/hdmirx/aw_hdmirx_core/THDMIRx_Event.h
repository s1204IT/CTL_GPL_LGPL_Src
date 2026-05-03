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
#ifndef _THDMIRX_EVENT_H_
#define _THDMIRX_EVENT_H_
#include <linux/init.h>
#include <linux/kfifo.h>
#include "THDMIRx_System.h"
#include "THDMIRx_PortCtx.h"
#include "hardware/THDMIRx_TV_Driver.h"
#include "hardware/THDMIRx_TV_Edid_Driver.h"
#include "../../hrc/sunxi_hrc_define.h"

typedef enum _tagHPDState {
	HPDState_Idle,
	HPDState_Send_PullUp_CMD,
	HPDState_CMD_TimeInterval,
	HPDState_Send_PullDown_CMD,
} HPDState_e;

typedef enum _HDMIRxEventType {
	HDMIRxEventType_Invalid = 0,
	HDMIRxEventType_Hotplug,
	HDMIRxEventType_ISREvent,
	HDMIRxEventType_SwitchSource,
	HDMIRxEventType_MV_ConnectLink,
	HDMIRxEventType_MV_DisconnectLink,
	HDMIRxEventType_MV_BgPortScan,
	HDMIRxEventType_Max
} HDMIRxEventType_e;

typedef struct _tagHDMIRxEvent {
	HDMIRxEventType_e type;
	U8 PortID; /* 1 2*/
	U32 param;
} HDMIRxEvent_t;

typedef struct _tagHPDHandle {
	HPDState_e HPDState;
	U32 tick;
	bool HPD_Req;
	U8 HPD_CMD;
	bool is5V;
} HPDHandle_t;

struct THDMIRx_Event {
	struct kfifo *pEventQueue;
	HPDHandle_t HPDHandle;
	bool EventQueueLock;
};

//void HPD_CMD_Request(U8 PortID, U8 cmd);



void HDMIRx_Event_SetRemap(bool enable);

void HDMIRx_Event_SetHPDTimeInterval(U32 val);

void HDMIRx_Event_SendEvent(HDMIRxEvent_t event);

void HDMIRx_ScanISRTask(struct HDMIRx_PortContext *pCtx);

void HDMIRx_ScanPortTask(struct HDMIRx_PortContext *pCtx);

void HDMIRx_HandleEventTask(U8 PortID, struct HDMIRx_PortContext *pCtx);

void THDMIRx_Eventinit(U8 port_id, struct THDMIRx_Event *core);

void THDMIRx_Eventdeinit(U8 port_id);

#endif /* _THDMIRX_EVENT_H_ */
