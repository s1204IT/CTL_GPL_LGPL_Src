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
#include "THDMIRx_Port.h"
#include "aw_hdmirx_core_drv.h"
#include "hardware/ThdmiRxIPReg.h"

const char *PortPathStateStrings[] = {
	"LinkDisconnect",
	"LinkDisconnecting",
	"LinkConnecting",
	"LinkConnected",
	"VideoOut"
};

const char *HdmiRxStateStrings[] = {
	"HDMIRX_STATE_NONE",
	"HDMIRX_STATE_INIT",
	"HDMIRX_STATE_SLEEP",
	"HDMIRX_STATE_IDLE",
	"HDMIRX_STATE_ACTIVE",
	"HDMIRX_STATE_RUNNING"
};

struct THDMIRx_Port *core_port[HDMI_PORT_NUM];

struct THDMIRx_Port *HDMIRx_get_port(U8 PortID)
{
	if (PortID > HDMI_PORT_NUM)
		return NULL;
	return core_port[PortID - 1];
}

ssize_t THDMIRx_Port_Dump_Status(char *buf)
{
	ssize_t n = 0;
	int port_id = 0;

	for (port_id = 0; port_id < HDMI_PORT_NUM; port_id++) {
		n += sprintf(buf + n, "port%d\n", port_id);
		n += sprintf(buf + n, "    HDCP_TaskActive              = %d\n", HDMIRx_Port_GetHDCPTaskIsActive(core_port[port_id]));
		n += sprintf(buf + n, "    StateMachine_TaskActive      = %d\n", HDMIRx_Port_GetSMTaskIsActive(core_port[port_id]));
		n += sprintf(buf + n, "    5VState                      = %d\n", HdmiRx_GetPort_5VState(port_id + 1));
		n += sprintf(buf + n, "    HpdState                     = %d\n", HdmiRx_GetPort_HpdState(port_id));
		n += sprintf(buf + n, "    SignalValid                  = %d\n", HdmiRx_GetPort_SignalValid());
		n += sprintf(buf + n, "    PhyCDRLock                   = %d\n", HdmiRx_GetPort_PhyCDRLock());
		n += sprintf(buf + n, "    PathState                    = %s\n", PortPathStateStrings[HDMIRx_Port_GetPortPathState(core_port[port_id])]);
		n += sprintf(buf + n, "    HdmiRxState                  = %s\n", HdmiRxStateStrings[HDMIRx_Port_GetHdmiRxState(core_port[port_id])]);
	}

	return n;
}

void THDMIRx_Port_Clear_SW_Context(struct THDMIRx_Port *pPort)
{
	// clear ctx
	HDMIRx_PortCtx_Contextclear(pPort->PortCtx.PortID);
}

struct HDMIRx_PortContext *THDMIRx_Port_GetPortContext(struct THDMIRx_Port *pActivePort)
{
	return &pActivePort->PortCtx;
}

PortPathState_e HDMIRx_Port_GetPortPathState(struct THDMIRx_Port *pActivePort)
{
	return pActivePort->PortCtx.PortPathState;
}

void HDMIRx_Port_SetPortPathState(struct THDMIRx_Port *pPort, PortPathState_e state)
{
	pPort->PortCtx.PortPathState = state;
}

bool HDMIRx_Port_GetIsActivePort(struct THDMIRx_Port *pPort)
{
	return pPort->PortCtx.isActivePort;
}

void HDMIRx_Port_SetIsActivePort(struct THDMIRx_Port *pPort, bool isActive)
{
	pPort->PortCtx.isActivePort = isActive;
}

bool HDMIRx_Port_GetHDCPTaskIsActive(struct THDMIRx_Port *pPort)
{
	return pPort->PortCtx.TaskCtx.HDCP_TaskActive;
}

void HDMIRx_Port_SetHDCPTaskIsActive(struct THDMIRx_Port *pPort, bool isActive)
{
	pPort->PortCtx.TaskCtx.HDCP_TaskActive = isActive;
}

void HDMIRx_Port_GetHDCP14Status(THdcp14Info *pHdcp14Info)
{
	HdmiRx_HDCP14GetStatus(pHdcp14Info);
}

bool HDMIRx_Port_GetSMTaskIsActive(struct THDMIRx_Port *pPort)
{
	return pPort->PortCtx.TaskCtx.StateMachine_TaskActive;
}

void HDMIRx_Port_SetSMTaskIsActive(struct THDMIRx_Port *pPort, bool isActive)
{
	pPort->PortCtx.TaskCtx.StateMachine_TaskActive = isActive;
	PORT_INF("pPort->PortCtx.TaskCtx.StateMachine_TaskActive = %d\n", pPort->PortCtx.TaskCtx.StateMachine_TaskActive);
}

HdmiRxState_e HDMIRx_Port_GetHdmiRxState(struct THDMIRx_Port *pPort)
{
	return pPort->PortCtx.StateMachineCtx.HdmiRxState;
}

void HDMIRx_Port_SetRemap(bool enable)
{
	HDMIRx_Event_SetRemap(enable);
}

void HDMIRx_Port_SetHPDTimeInterval(U32 val)
{
	HDMIRx_Event_SetHPDTimeInterval(val);
}

void HDMIRx_Port_SendHPDEvent(struct THDMIRx_Port *pPort)
{
	HDMIRxEvent_t event;
	memset(&event, 0, sizeof(HDMIRxEvent_t));
	event.type = HDMIRxEventType_Hotplug;
	event.PortID = pPort->PortCtx.PortID;
	HDMIRx_Event_SendEvent(event);
	hdmirx_inf("%s: port %d send HPD event\n", __func__, event.PortID);
	// osal_time_delay_milliseconds(5);
}

void HDMIRx_Port_SendSwitchSourceEvent(struct THDMIRx_Port *pPort, bool isActive)
{
	HDMIRxEvent_t event;
	event.type = HDMIRxEventType_SwitchSource;
	event.PortID = pPort->PortCtx.PortID;
	event.param = isActive;
	HDMIRx_Event_SendEvent(event);
	hdmirx_inf("%s: port %d send SwitchSource event isActive %d\n", __func__, event.PortID, isActive);
	// osal_time_delay_milliseconds(5);
}

bool HDMIRx_Port_TimingMonitorTask(struct HDMIRx_PortContext *pCtx, U8 *pSignalIndex, U32 *pSignalID)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(pCtx->PortID);
	if (mcore_port == NULL)
		return false;

	ThdmRxTimingParam newTiming;
	memset(&newTiming, 0, sizeof(ThdmRxTimingParam));
	HdmiRx_Video_GetTimingParam(&newTiming, pCtx);
	if (CheckTimingDiff(pCtx->PortID, &(pCtx->videoCtx.ctx.tTimingParam), &newTiming)) {
		memcpy(&(pCtx->videoCtx.ctx.tTimingParam), &newTiming, sizeof(ThdmRxTimingParam));
		pCtx->videoCtx.DetectTimingValidTick = HDMIRx_GetCurrentTick();
		//hdmirx_inf("DetectTimingValidTick = %d", pCtx->videoCtx.DetectTimingValidTick);
		HDMIRx_DisplayModuleCtx_SetMute(true, true);
	} else {
		if (HDMIRx_CompareTickCount(pCtx->videoCtx.DetectTimingValidTick, HDMIRX_DETECT_TIMING_TICK)) {
			// signal stable
			//hdmirx_inf("signal stable DetectTimingValidTick = %d", pCtx->videoCtx.DetectTimingValidTick);
			if (0 != pCtx->videoCtx.DetectTimingValidTick) {
				PORT_INF("port%d new timing HActive:0x%x VActive:0x%x bPixelRep:%d\n", pCtx->PortID,
						pCtx->videoCtx.ctx.tTimingParam.numActivePixelsPerLine, pCtx->videoCtx.ctx.tTimingParam.numActiveLines,
						pCtx->videoCtx.ctx.tTimingParam.bpixRep);
				// new timing,find signal ID
				if (DisplayModule_DetectSignal_ByDB(&pCtx->videoCtx.ctx.tTimingParam, pSignalIndex, pSignalID) ==
					false) {
					hdmirx_err("%s: This timing cann't match any timing in database!!!\n", __func__);
					pCtx->videoCtx.ctx.wCurVidFormat = NONE_VIDEO_IN_FORMAT;
					pCtx->videoCtx.DetectTimingValidTick = HDMIRx_GetCurrentTick();
					HdmiRx_Toggle_PD_IDCLK_Reset();

					if (++pCtx->videoCtx.DetectTiming_TimeOutCount > HDMIRX_DETECT_TIMING_TIMEOUT_COUNT) {
						pCtx->videoCtx.DetectTiming_TimeOutCount = 0;

						if (AI_SIGNAL_MODE_UNKNOW != HDMIRx_DisplayModuleCtx_GetSignal()) {
							hdmirx_err("port%d unkonw signal!!!\n", pCtx->PortID);
							HDMIRx_DisplayModuleCtx_SetSignal(AI_SIGNAL_MODE_UNKNOW, &(mcore_port->PortCtx));
						}

						hdmirx_err("%s: port%d detect timing failed!!\n", __func__, pCtx->PortID);
					}
					return false;
				} else {
					// Some CVTE device change resolution, it always in running mode
					// So need do audio reset when detect valid timing
					HdmiRx_Audio_PD_Reset();
				}

				if ((pCtx->videoCtx.ctx.CurVideoMode != VIDEO_MODE_DVI) &&
					(!HdmiRxPacket_AVI_Handle(&pCtx->PacketCtx.ctx))) {
					pCtx->videoCtx.ctx.wCurVidFormat = NONE_VIDEO_IN_FORMAT;
					pCtx->videoCtx.DetectTimingValidTick = HDMIRx_GetCurrentTick();
					hdmirx_err("%s: Can not get valid AVI infoframe!!!!!\n", __func__);
					return false;
				}

				pCtx->videoCtx.DetectTiming_TimeOutCount = 0;
				pCtx->videoCtx.ctx.ucCurSignalIndex = *pSignalIndex;
				PORT_INF("%s: port%d detect new timing,signal ID=0x%x index=%d\n", __func__, pCtx->PortID, *pSignalID, *pSignalIndex);
			} else {
				// still old timing
				*pSignalIndex = pCtx->videoCtx.ctx.ucCurSignalIndex;
				*pSignalID = DisplayModule_GetSignalIDFromDB(*pSignalIndex);
			}

			pCtx->videoCtx.DetectTimingValidTick = 0;
			return true;
		}
	}

	return false;
}

void HDMIRx_Port_AudioMonitorTask(struct HDMIRx_PortContext *pCtx)
{
	if (pCtx->isActivePort) {
		AudioMonitor(pCtx);
	}
}

void HDMIRx_Port_SignalMonitorTask(struct HDMIRx_PortContext *pCtx)
{
	U8 ucNewIndex = 0;
	U32 dwNewSignal = AI_SIGNAL_MODE_UNKNOW;
	bool isTimingStable = HDMIRx_Port_TimingMonitorTask(pCtx, &ucNewIndex, &dwNewSignal);
	if (isTimingStable) {
		// detect signal stable
		pCtx->videoCtx.DetectTiming_TimeOutCount = 0;
		SignalMonitor(pCtx->PortID, ucNewIndex, dwNewSignal, pCtx);
	} else {
		pCtx->videoCtx.ctx.eVideoState = HDMIRX_VIDEO_STATE_MEASURE;
		pCtx->videoCtx.DetectSignalValidTick = HDMIRx_GetCurrentTick();
	}
}

void StateMachine_Sleep_PreAction(U8 PortID)
{
	LOG_TRACE1(PortID);
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	// driver
	HdmiRx_StateMachine_Sleep_PreAction();
	if (mcore_port->PortCtx.isActivePort) {
		// audio power down
		HdmiRx_Audio_PowerDownAPLL();
	}
	// ctx
	// this->pHDMIRxEvent->EventQueueFlush();
	// active port send no signal when lost 5V
	if (HDMIRX_STATE_IDLE <= mcore_port->PortCtx.StateMachineCtx.HdmiRxState) {
		if (mcore_port->PortCtx.isActivePort) {
			// send no signal
			HDMIRx_DisplayModuleCtx_SetSignal(AI_SIGNAL_MODE_NO_SIGNAL, &(mcore_port->PortCtx));
		}
	}
}

void StateMachine_Idle_PreAction(U8 PortID)
{
	LOG_TRACE1(PortID);
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	// driver
	HdmiRx_StateMachine_Idle_PreAction();
	if (mcore_port->PortCtx.isActivePort) {
#if (HDMIRX_USE_APLL)
		HdmiRx_Audio_PowerUpAPLL();
		HdmiRx_Audio_ResetAPLL();
		HdmiRx_Audio_ResetAudioBlock(&(mcore_port->PortCtx.audioCtx));
#else
		HdmiRx_Audio_PD_Reset();
#endif
	}
	// ctx

	// init detect
	mcore_port->PortCtx.videoCtx.DetectSignalValidTick = HDMIRx_GetCurrentTick();
	mcore_port->PortCtx.videoCtx.DetectTimingValidTick = HDMIRx_GetCurrentTick();
}

void StateMachine_Active_PreAction(U8 PortID)
{
	LOG_TRACE1(PortID);
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	// driver
	HdmiRx_StateMachine_Active_PreAction();
	// ctx

	// init detect
	mcore_port->PortCtx.videoCtx.DetectSignalValidTick = HDMIRx_GetCurrentTick();
	mcore_port->PortCtx.videoCtx.DetectTimingValidTick = HDMIRx_GetCurrentTick();
}

void StateMachine_Running_PreAction(U8 PortID)
{
	LOG_TRACE1(PortID);
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	// driver
	HdmiRx_StateMachine_Running_PreAction();
	// set AV mute before signal stable
	HDMIRx_DisplayModuleCtx_SetMute(true, true);
	// ctx
	if (mcore_port->PortCtx.videoCtx.TmdsCtx.bFreqDetStart) {
		mcore_port->PortCtx.videoCtx.TmdsCtx.bFreqDetStart = false;
		HdmiRx_Video_TMDS_FreqDetect(&(mcore_port->PortCtx.videoCtx));
	}

	mcore_port->PortCtx.videoCtx.DetectTiming_TimeOutCount = 0;
}

void SwitchState_PreAction(U8 PortID, HdmiRxState_e new_state)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	switch (new_state) {
	case HDMIRX_STATE_INIT:
		break;

	case HDMIRX_STATE_SLEEP:
		StateMachine_Sleep_PreAction(PortID);
		break;

	case HDMIRX_STATE_IDLE:
		StateMachine_Idle_PreAction(PortID);
		break;

	case HDMIRX_STATE_ACTIVE:
		StateMachine_Active_PreAction(PortID);
		break;

	case HDMIRX_STATE_RUNNING:
		StateMachine_Running_PreAction(PortID);
		break;

	default:
		break;
	}
}

void SwitchState(U8 PortID, HdmiRxState_e new_state)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	if (mcore_port->PortCtx.StateMachineCtx.HdmiRxState == new_state)
		return;
	//pre action
	SwitchState_PreAction(PortID, new_state);
	PORT_INF("%s port%d SwitchState from %d to %d\n", __func__, mcore_port->PortCtx.PortID, mcore_port->PortCtx.StateMachineCtx.HdmiRxState, new_state);
	mcore_port->PortCtx.StateMachineCtx.HdmiRxState = new_state;
}

void StateMachine_Init(U8 PortID)
{
	SwitchState(PortID, HDMIRX_STATE_SLEEP);
}

void StateMachine_Sleep(U8 PortID)
{
	if (HdmiRx_GetPort_5VState(PortID)) {
		SwitchState(PortID, HDMIRX_STATE_IDLE);
	}
}

void StateMachine_Idle(U8 PortID)
{
	if (!HdmiRx_GetPort_5VState(PortID)) {
		SwitchState(PortID, HDMIRX_STATE_SLEEP);
	} else if (HdmiRx_GetPort_SignalValid()) {
		SwitchState(PortID, HDMIRX_STATE_ACTIVE);
	}
}

void StateMachine_Active(U8 PortID)
{
	if (!HdmiRx_GetPort_5VState(PortID)) {
		SwitchState(PortID, HDMIRX_STATE_SLEEP);
	} else if (!HdmiRx_GetPort_SignalValid()) {
		SwitchState(PortID, HDMIRX_STATE_IDLE);
	} else if (HdmiRx_GetPort_PhyCDRLock()) {
		SwitchState(PortID, HDMIRX_STATE_RUNNING);
	}
}

void StateMachine_Running(U8 PortID)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	if (!HdmiRx_GetPort_5VState(PortID)) {
		HDMIRx_PortCtx_Contextclear(PortID);
		SwitchState(PortID, HDMIRX_STATE_SLEEP);
	} else if (!HdmiRx_GetPort_SignalValid()) {
		HDMIRx_PortCtx_Contextclear(PortID);
		SwitchState(PortID, HDMIRX_STATE_IDLE);

		hdmirx_wrn("port%d invalid signal!!!\n", PortID);
		HDMIRx_DisplayModuleCtx_SetSignal(AI_SIGNAL_MODE_NO_SIGNAL, &(mcore_port->PortCtx));

	} else if (!HdmiRx_GetPort_PhyCDRLock()) {
		HDMIRx_PortCtx_Contextclear(PortID);
		SwitchState(PortID, HDMIRX_STATE_ACTIVE);
	}

	if (mcore_port->PortCtx.StateMachineCtx.HdmiRxState == HDMIRX_STATE_RUNNING) {
		HDMIRx_Port_SignalMonitorTask(&(mcore_port->PortCtx));
		HDMIRx_Port_AudioMonitorTask(&(mcore_port->PortCtx));
		// HDMIRx_Port_AVMuteDetectTask(&PortCtx);
	} else {
		HDMIRx_DisplayModuleCtx_SetMute(true, true);
	}
}

void HDMIRx_Port_StateMachineTask(U8 PortID)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	if (!mcore_port->PortCtx.TaskCtx.StateMachine_TaskActive)
		return;

	switch (mcore_port->PortCtx.StateMachineCtx.HdmiRxState) {
	case HDMIRX_STATE_INIT:
		StateMachine_Init(PortID);
		break;

	case HDMIRX_STATE_SLEEP:
		StateMachine_Sleep(PortID);
		break;

	case HDMIRX_STATE_IDLE:
		StateMachine_Idle(PortID);
		break;

	case HDMIRX_STATE_ACTIVE:
		StateMachine_Active(PortID);
		break;

	case HDMIRX_STATE_RUNNING:
		StateMachine_Running(PortID);
		break;

	default:
		break;
	}
}

void HDMIRx_Port_HDCPTask(U8 PortID)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	if (!mcore_port->PortCtx.TaskCtx.HDCP_TaskActive)
		return;
	HDMIRx_HDCP14Task(PortID);
}

void HDMIRx_Port_EventTask(U8 PortID)
{
	struct THDMIRx_Port *mcore_port;

	mcore_port = HDMIRx_get_port(PortID);
	if (mcore_port == NULL)
		return ;

	HDMIRx_HandleEventTask(PortID, &(mcore_port->PortCtx));  // always active,solve pEventQueue

	if (!mcore_port->PortCtx.TaskCtx.Event_TaskActive)
		return;

	//if(this->pDisplayModuleCtx->DisplayModule_Get5VDetect_isEnable())
	mcore_port->PortCtx.is5V = HdmiRx_GetPort_5VState(PortID);
	//solve interrupt
	HDMIRx_ScanISRTask(&(mcore_port->PortCtx));
	HDMIRx_ScanPortTask(&(mcore_port->PortCtx));
}

void THDMIRx_Portinit(U8 port_id, struct THDMIRx_Port *core, struct THDMIRx_DisplayModuleCtx *_pDisplayModuleCtx)
{
	core_port[port_id] = core;
	core_port[port_id]->pDisplayModuleCtx = _pDisplayModuleCtx;

	HDMIRx_PortContextinit(port_id, &core_port[port_id]->PortCtx);
	// init port_ctx
	core_port[port_id]->PortCtx.PortID = port_id + 1;
	core_port[port_id]->PortCtx.LinkID = LinkBaseID_0;
	core_port[port_id]->PortCtx.isActivePort = false;
	core_port[port_id]->PortCtx.isHDCPRunning = true;
	core_port[port_id]->PortCtx.TaskCtx.Event_TaskActive = true;
	core_port[port_id]->PortCtx.StateMachineCtx.HdmiRxState = HDMIRX_STATE_INIT;
	core_port[port_id]->PortCtx.TaskCtx.StateMachine_TaskActive = false;
	core_port[port_id]->PortCtx.PortPathState = PortPathState_LinkDisconnect;

	THDMIRx_Eventinit(port_id, &core_port[port_id]->pHDMIRxEvent);
	THDMIRx_HDCP14init(port_id, &core_port[port_id]->pHDMIRx_HDCP14, &core_port[port_id]->PortCtx);
	core_port[port_id]->PortCtx.PortPathState = PortPathState_LinkConnected;
	hdmirx_inf("THDMIRx_Port start!!\n");
}
