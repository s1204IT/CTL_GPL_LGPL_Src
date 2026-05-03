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
#include <linux/slab.h>
#include "../aw_hdmirx_define.h"
#include "THDMIRx_Event.h"
#include "aw_hdmirx_core_drv.h"

#if (HDMIRX_PORT_NEEDREMAP)
	static bool isRemap;
#endif

static U32 HPD_TimeInterval = HPD_VALID_INTERVAL;

struct THDMIRx_Event *core_event[HDMI_PORT_NUM];

struct THDMIRx_Event *HDMIRx_get_Event(U8 PortID)
{
	if (PortID > HDMI_PORT_NUM)
		return NULL;
	return core_event[PortID - 1];
}

void EventQueue_Init(U8 PortID)
{
	int ret;
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	mcore_event->pEventQueue = kzalloc(sizeof(struct kfifo), GFP_KERNEL);
	if (!mcore_event->pEventQueue) {
		hdmirx_err("%s:  alloc pEventQueue memory failed!!!\n", __func__);
	}

	ret = kfifo_alloc(mcore_event->pEventQueue, sizeof(HDMIRxEvent_t) * 128, GFP_KERNEL);
	if (ret != 0) {
		hdmirx_err("%s:  Failed to alloc pEventQueue.\n", __func__);
	}
}

void EventQueue_Deinit(U8 PortID)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	//kfifo_free(mcore_event->pEventQueue);
	kfree(mcore_event->pEventQueue);
}

bool EventQueue_Push(U8 PortID, HDMIRxEvent_t *pEvent)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return false;

	if (kfifo_is_full(mcore_event->pEventQueue) == 0) {
		if (kfifo_in(mcore_event->pEventQueue, pEvent, sizeof(HDMIRxEvent_t)) != sizeof(HDMIRxEvent_t)) {
			hdmirx_err("%s:  kfifo_in data is error.\n", __func__);
			return false;
		}
		return true;
	}
	EVENT_INF("%s:  pEventQueue is full.\n", __func__);
	return false;
}

bool EventQueue_Pop(U8 PortID, HDMIRxEvent_t *pEvent)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return false;

	if (kfifo_is_empty(mcore_event->pEventQueue) == 0) {
		if (kfifo_out(mcore_event->pEventQueue, pEvent, sizeof(HDMIRxEvent_t)) != sizeof(HDMIRxEvent_t)) {
			hdmirx_err("%s:  kfifo_out data is error.\n", __func__);
			return false;
		}
		return true;
	}
	//hdmirx_inf("%s:  pEventQueue is empty.\n", __func__);
	return false;
}

void EventQueue_Flush(U8 PortID)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	kfifo_reset(mcore_event->pEventQueue);
}

void HDMIRx_Event_SendEvent(HDMIRxEvent_t event)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(event.PortID);
	if (mcore_event == NULL)
		return;

	mcore_event->EventQueueLock = true;
	EventQueue_Push(event.PortID, &event);
	mcore_event->EventQueueLock = false;
}

void HDMIRx_Event_SetRemap(bool enable)
{
	isRemap = enable;
}

void HDMIRx_Event_SetHPDTimeInterval(U32 val)
{
	EVENT_INF("%s: SetHPDTimeInterval from %d to %d\n", __func__, HPD_TimeInterval, val);
	HPD_TimeInterval = val;
}

void HDMIRx_Event_HPDSwitchState(U8 PortID, HPDState_e new_state)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	if (HPDState_CMD_TimeInterval == new_state) {
		mcore_event->HPDHandle.tick = HDMIRx_GetCurrentTick();
	} else if (HPDState_Idle == new_state) {
		// clear ctx
		// memset(&HPDHandle,0,sizeof(HPDHandle_t));
	}

	mcore_event->HPDHandle.HPDState = new_state;
}

void HPDStateHandle(U8 PortID)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	//log_i("HPDHandle.HPDState : %d tick : %d HPD_Req : %d HPD_CMD : %d is5v : %d\n", HPDHandle.HPDState, HPDHandle.tick, HPDHandle.HPD_Req, HPDHandle.HPD_CMD, HPDHandle.is5V);
	switch (mcore_event->HPDHandle.HPDState) {
		case HPDState_Send_PullDown_CMD: {
			mcore_event->HPDHandle.HPD_CMD = CMD_HOTPLUG_PULLDOWN;
			HdmiRx_SendHPDCMD(mcore_event->HPDHandle.HPD_CMD, PortID);
			if (mcore_event->HPDHandle.HPD_Req)
				HDMIRx_Event_HPDSwitchState(PortID, HPDState_CMD_TimeInterval);
			else
				HDMIRx_Event_HPDSwitchState(PortID, HPDState_Idle);

		} break;

		case HPDState_CMD_TimeInterval: {
			if (HDMIRx_CompareTickCount(mcore_event->HPDHandle.tick, HPD_TimeInterval)) {
				HDMIRx_Event_HPDSwitchState(PortID, HPDState_Send_PullUp_CMD);
			}
		} break;

		case HPDState_Send_PullUp_CMD: {
			mcore_event->HPDHandle.HPD_CMD = CMD_HOTPLUG_PULLUP;
			HdmiRx_SendHPDCMD(mcore_event->HPDHandle.HPD_CMD, PortID);
			HDMIRx_Event_HPDSwitchState(PortID, HPDState_Idle);
		} break;

		default: {
			HDMIRx_Event_HPDSwitchState(PortID, HPDState_Idle);
		} break;
	}
}

void SwitchSourceEventHandle(U8 PortID, U8 isActive, struct HDMIRx_PortContext *pCtx)
{
	if (!isActive) {
		// switch out

		// hotplug requirment from AW customer CVTE
		EVENT_INF("%s: port%d switch source 5V pull down", __func__, PortID);
		// HPD_CMD_Request(port_id, CMD_PULL_DOWN);
		pCtx->PortPathState = PortPathState_LinkConnected;
		pCtx->StateMachineCtx.HdmiRxState = HDMIRX_STATE_SLEEP;
		EventQueue_Flush(PortID);

		// for power saving, disable clk
		// THDMIRx_DataPath::DataPath_CallBack.DataPath_MultiLink_CallBack.MVTOP_ICGEnable(0, false);
	} else {
		// switch in

		// for power saving, enable clk at first when switch source in
		if (pCtx->is5V)
			HdmiRx_MVTOP_ICGEnable(0, true);
	}
}

bool ISREventPreHandle(struct HDMIRx_PortContext *pCtx, HDMIRxEvent_t *pEvent)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(pEvent->PortID);
	if (mcore_event == NULL)
		return false;

	bool ret = true;
	pEvent->param = pEvent->param & 0xffff;
	eHdmiRxMsgID IsrMsg = (eHdmiRxMsgID)pEvent->param;
	switch (IsrMsg) {
	case MSG_HDMIRX_INTR_PKT_DIFF_AI:
	case MSG_HDMIRX_INTR_PKT_DIFF_ACR:
	case MSG_HDMIRX_INTR_PKT_DIFF_ACP:
	case MSG_HDMIRX_INTR_AUDIO_CS_DIFF:
	case MSG_HDMIRX_INTR_AUDIO_PLL_LOCKED:
		// audio packet valid only if port active
		if (!pCtx->isActivePort)
			ret = false;
		break;

	case MSG_HDMIRX_INTR_AUDIO_FIFO_UNDERRUN:
	case MSG_HDMIRX_INTR_AUDIO_FIFO_OVERRUN:
	case MSG_HDMIRX_INTR_AUDIO_CHECKSUM_ERROR:
	case MSG_HDMIRX_INTR_AUDIO_STATUS_BIT_ERROR:
		// audio isr valid only if port active and signal valid
		if ((!pCtx->isActivePort) || (pCtx->StateMachineCtx.HdmiRxState != HDMIRX_STATE_RUNNING)) {
			ret = false;
		}
		break;

	case MSG_HDMIRX_INTR_DVI_LOCKED:
	case MSG_HDMIRX_INTR_HDMI_LOCKED:
		// this isr should handle after phycdr locked or is invalid
		if (pCtx->StateMachineCtx.HdmiRxState != HDMIRX_STATE_RUNNING) {
			ret = false;
		}
		break;

	default:
		break;
	}

	return ret;
}

void ISREventHandle(struct HDMIRx_PortContext *pCtx, HDMIRxEvent_t *pEvent)
{
	eHdmiRxMsgID IsrMsg = (eHdmiRxMsgID)pEvent->param;
	EVENT_INF("%s: port%d is5v:%d handle isr event:0x%x\n", __func__, pCtx->PortID, pCtx->is5V, IsrMsg);
	switch (IsrMsg) {
	case MSG_HDMIRX_INTR_PHY_INT_FREQ_DET:
		// pCtx->videoCtx.Video_DriverCallBack.Video_TMDS_FreqDetect(pCtx->LinkBase,&(pCtx->videoCtx));
		pCtx->videoCtx.TmdsCtx.bFreqDetStart = true;
		break;

	case MSG_HDMIRX_INTR_PHY_UNLOCKED:
		break;

	case MSG_HDMIRX_INTR_DVI_LOCKED:
		HdmiRx_Video_UpdateHdmiMode(VIDEO_MODE_DVI, &(pCtx->videoCtx));
		break;

	case MSG_HDMIRX_INTR_HDMI_LOCKED:
	case MSG_HDMIRX_INTR_PKT_LOCKED:
		HdmiRx_Video_UpdateHdmiMode(VIDEO_MODE_HDMI, &(pCtx->videoCtx));
		// After hdmi_locked interrupt comes, always toggle bch_err_cnt_rst to clear bch_cnt to 0. (Natsuko Ibuki solution for BCH50
		// since SX7B)
		//HdmiRx_HDCP22_BCH_ErrorCount_Reset(pCtx->LinkBase);
		break;

	case MSG_HDMIRX_INTR_VTOTAL_CHANGED:
		// enter VRR mode
		// pCtx->videoCtx.Video_DriverCallBack.Video_SetVRRMode(pCtx->LinkBase,TRUE);
		EVENT_INF("%s: Enter VRR mode!\n", __func__);
		break;

	case MSG_HDMIRX_INTR_PKT_DIFF_GCP:
		if (HdmiRxPacket_GCP_Hanlde(&(pCtx->PacketCtx.ctx))) {
			// callback to app
			/*
			if ((pCtx->isActivePort) && (THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewGCPacketHandler)) {
				THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewGCPacketHandler(
					(U8*)&pCtx->PacketCtx.ctx.packet[PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_GCP)]);
			}
			*/
		}
		break;

	case MSG_HDMIRX_INTR_PKT_DIFF_VSI_LLC:
	case MSG_HDMIRX_INTR_PKT_DIFF_GP2:
		if (HdmiRxPacket_VSI_LLC_Handle(&(pCtx->PacketCtx.ctx))) {
			pCtx->videoCtx.ctx.bNewDbVisionLowLatencyMode = pCtx->PacketCtx.ctx.tVSIInfoFrame.DbVision_LowLatency;
			// callback to app
			/*
			if ((pCtx->isActivePort) && (THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewVSIPacketHandler)) {
				THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewVSIPacketHandler(
					(U8*)&pCtx->PacketCtx.ctx.packet[PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_VSI)]);
			}
			*/
		}
		break;

	case MSG_HDMIRX_INTR_PKT_DIFF_SPDI:
		if (HdmiRxPacket_SPDI_Handle(&(pCtx->PacketCtx.ctx))) {
			if (pCtx->PacketCtx.ctx.tFreeSyncSPDI.bFreeSyncActive || pCtx->PacketCtx.ctx.tFreeSyncSPDI.bFreeSyncEnabled) {
				HdmiRx_Video_SetVRRMode(true);
				EVENT_INF("%s: Enter VRR mode!\n", __func__);
			} else {
				HdmiRx_Video_SetVRRMode(false);
				EVENT_INF("%s: Leave VRR mode!\n", __func__);
			}

			// callback to app
			/*
			if ((pCtx->isActivePort) && (THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewSPDPacketHandler)) {
				THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewSPDPacketHandler(
					(U8*)&pCtx->PacketCtx.ctx.packet[PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_SPDI)]);
			}
			*/
		}
		break;

	case MSG_HDMIRX_INTR_PKT_DIFF_AVI: {
		// U8 PixelRepetition = pCtx->PacketCtx.ctx.tAVIInfoFrame.PixelRepetition;
		eHdmiRxColorFmt color_format = pCtx->PacketCtx.ctx.tAVIInfoFrame.ColorSpace.Model;
		if (HdmiRxPacket_AVI_Handle(&(pCtx->PacketCtx.ctx))) {
			// if (PixelRepetition != pCtx->PacketCtx.ctx.tAVIInfoFrame.PixelRepetition)
			HdmiRx_Video_UpdatePixelRepetition(pCtx->PacketCtx.ctx.tAVIInfoFrame.PixelRepetition);

			if (color_format != pCtx->PacketCtx.ctx.tAVIInfoFrame.ColorSpace.Model)
				HdmiRx_Video_UpdateColorSpaceModel(pCtx->PacketCtx.ctx.tAVIInfoFrame.ColorSpace.Model);

			// callback to app
			/*
			if ((pCtx->isActivePort) && (THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewAVIPacketHandler)) {
				THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewAVIPacketHandler(
					(U8*)&pCtx->PacketCtx.ctx.packet[PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_AVI)]);
			}
			*/
		}
	} break;

	case MSG_HDMIRX_INTR_PKT_DIFF_AI:
		if (HdmiRxPacket_AI_Handle(&(pCtx->PacketCtx.ctx))) {
			// callback to app
			/*
			if ((pCtx->isActivePort) && (THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewAudioInfoPacketHandler)) {
				THDMIRx_DisplayModuleCtx::DisplayModuleCallBack.pHDMINewAudioInfoPacketHandler(
					(U8*)&pCtx->PacketCtx.ctx.packet[PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_AI)]);
			}
			*/
		}
		break;

	case MSG_HDMIRX_INTR_PKT_DIFF_ACR:
		break;

	case MSG_HDMIRX_INTR_AUDIO_CS_DIFF: {
		HdmiRx_Audio_Get_N_CTS(&pCtx->audioCtx);
		HdmiRx_Audio_GetFsFromCS(&pCtx->audioCtx);
		EVENT_INF("%s: audio param:CTS %d N %d fsrate %d audioPLLfs %d audioType 0x%x\n", __func__, pCtx->audioCtx.ctx.CTS, pCtx->audioCtx.ctx.N, pCtx->audioCtx.ctx.fs,
			pCtx->audioCtx.ctx.audioPLLfs, pCtx->audioCtx.ctx.audioType);
		pCtx->audioCtx.sui32PrevFs = 0;
		pCtx->audioCtx.sui32PrevN = 0;
		sunxi_hrc_call_notifiers(AUDIO_FSRATE_CHANGE, &pCtx->audioCtx.ctx.fs);
#if (HDMIRX_USE_APLL)
		HdmiRx_Audio_SetAPLL(&pCtx->audioCtx);
#endif
		}
		break;

	case MSG_HDMIRX_INTR_AUDIO_FIFO_UNDERRUN:
	case MSG_HDMIRX_INTR_AUDIO_FIFO_OVERRUN: {
		if (++pCtx->audioCtx.ctx.fifoErrCnt > AUDIO_FIFO_ERR_THRESHOLD) {
			// reset audio processing block
			EVENT_INF("%s: audio error(0x%x) !!!!\n", __func__, IsrMsg);
			audioContextclear(pCtx->PortID);
#if (HDMIRX_USE_APLL)
			HdmiRx_Audio_ResetAudioBlock(&pCtx->audioCtx);
#else
			HdmiRx_Audio_PD_Reset();
#endif
			pCtx->audioCtx.ctx.fifoErrCnt = 0;
		}

	} break;

	case MSG_HDMIRX_INTR_AUDIO_PLL_LOCKED:
		// enable i2s out
		HdmiRx_Audio_I2S_EnableOutput(true);
		break;

	case MSG_HDMIRX_INTR_AUDIO_CHECKSUM_ERROR:
	case MSG_HDMIRX_INTR_AUDIO_STATUS_BIT_ERROR: {
		if (++pCtx->audioCtx.ctx.audioPktErrCnt > AUDIO_PKT_ERR_THRESHOLD) {
			// reset audio processing block
			EVENT_INF("%s: audio error(0x%x) !!!!\n", __func__, IsrMsg);
			audioContextclear(pCtx->PortID);
#if (HDMIRX_USE_APLL)
			HdmiRx_Audio_ResetAudioBlock(&pCtx->audioCtx);
#else
			HdmiRx_Audio_PD_Reset();

			// patch for tv303 projection, some hdmi cable EQ issue
			HdmiRxFixedEQ_Update(0x04);
#endif
			pCtx->audioCtx.ctx.audioPktErrCnt = 0;
		}
	} break;

	/*
	case MSG_HDCPRX_INTR_HDCP22_BCH50_ERROR:
		if (!pCtx->HDCP22_ReAuthTimeOutTick) {
			// reauth req
			THDMIRx_HDCP22::HDCP22_MsgHandle.HDCP22_ReAuthReq(pCtx->LinkBase, TRUE);
			pCtx->HDCP22_ReAuthTimeOutTick = HDMIRx_GetCurrentTick();
		}
		break;
	*/

	case MSG_HDCPRX_INTR_HDCP14_BCH_ERROR:
		if (!pCtx->HDCP14_ReAuthTimeOutTick) {
			// reauth req
			HdmiRx_HDCP14_ReAuthReq(true);
			pCtx->HDCP14_ReAuthTimeOutTick = HDMIRx_GetCurrentTick();
		}
		break;

	default:
			EVENT_INF("%s: Not support ISR event:%d\n", __func__, IsrMsg);
		break;
	}
}


void HPD_Request(U8 PortID)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	if (false == mcore_event->HPDHandle.is5V)
		return;
	mcore_event->HPDHandle.HPD_Req = true;
	mcore_event->HPDHandle.HPDState = HPDState_Send_PullDown_CMD;
	mcore_event->HPDHandle.tick = HDMIRx_GetCurrentTick();
	EVENT_INF("%s: port id=%d HDP_Req=%d HPDState=%d tick=%d\n", __func__, PortID, mcore_event->HPDHandle.HPD_Req, mcore_event->HPDHandle.HPDState, mcore_event->HPDHandle.tick);
}

/*
void HPD_CMD_Request(U8 PortID, U8 cmd)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	mcore_event->HPDHandle.HPD_Req = false;
	mcore_event->HPDHandle.HPDState = (cmd == CMD_PULL_UP) ? HPDState_Send_PullUp_CMD : HPDState_Send_PullDown_CMD;
	mcore_event->HPDHandle.HPD_CMD = cmd;
	mcore_event->HPDHandle.tick = HDMIRx_GetCurrentTick();
}
*/

void ScanPortPathState(struct HDMIRx_PortContext *pCtx)
{
	if (pCtx->PortPathState == PortPathState_VideoOut) {
		if (pCtx->StateMachineCtx.HdmiRxState != HDMIRX_STATE_RUNNING)
			pCtx->PortPathState = PortPathState_LinkConnected;

		if (pCtx->videoCtx.ctx.eVideoState != HDMIRX_VIDEO_STATE_STABLE)
			pCtx->PortPathState = PortPathState_LinkConnected;
	}
}

void HDMIRx_HandleEventTask(U8 PortID, struct HDMIRx_PortContext *pCtx)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(PortID);
	if (mcore_event == NULL)
		return;

	if (!mcore_event->EventQueueLock) {
		HDMIRxEvent_t event;
		HDMIRxEvent_t *pEvent = &event;
		if (!EventQueue_Pop(PortID, pEvent))
			return;

		EVENT_INF("%s: Handle event, port:%d event:%d\n", __func__, pEvent->PortID, pEvent->type);
		switch (pEvent->type) {
			//select port
		case HDMIRxEventType_Hotplug:
			//switch port
			HPD_Request(pEvent->PortID);
			break;

		case HDMIRxEventType_ISREvent:
			if (ISREventPreHandle(pCtx, pEvent))  // handle isr event only if event valid
				ISREventHandle(pCtx, pEvent);
			break;

			//select port
		case HDMIRxEventType_SwitchSource:
			SwitchSourceEventHandle(pEvent->PortID, pEvent->param, pCtx);
			break;

			//not surport
		case HDMIRxEventType_MV_ConnectLink:
			pCtx->LinkID = (LinkBaseID_e)(pEvent->param & 0xf);
			pCtx->PortPathState = PortPathState_LinkConnecting;  // start connect MV link
			EVENT_INF("%s: pCtx->LinkID : %d, pEvent->param : %d\n", __func__, pCtx->LinkID, pEvent->param);
			break;

			//not surport
		case HDMIRxEventType_MV_DisconnectLink:
			pCtx->LinkID = (LinkBaseID_e)(pEvent->param & 0xf);
			// THDMIRx_DataPath::DataPath_CallBack.DataPath_MultiLink_CallBack.MVMode_DisconnectLink(pCtx->HDCPBase
			// ,pCtx->LinkID,pCtx->PortID);
			pCtx->PortPathState = PortPathState_LinkDisconnecting;  // start disconnect MV link
			hdmirx_inf("%s: pCtx->LinkID : %d, pEvent->param : %d\n", __func__, pCtx->LinkID, pEvent->param);
			break;

		case HDMIRxEventType_MV_BgPortScan:
			break;

		default:
			break;
		}
	}
}

void HDMIRx_ScanISRTask(struct HDMIRx_PortContext *pCtx)
{
	if (PortPathState_LinkConnected <= pCtx->PortPathState) {
		if (pCtx->isActivePort)
			HdmiRx_ScanISR(pCtx->PortID);
	}
}

void HDMIRx_ScanPortTask(struct HDMIRx_PortContext *pCtx)
{
	struct THDMIRx_Event *mcore_event;

	mcore_event = HDMIRx_get_Event(pCtx->PortID);
	if (mcore_event == NULL)
		return;

	struct hrc_from_hdmirx_5V hrc_5V_info;
	memset(&hrc_5V_info, 0, sizeof(hrc_5V_info));

	// scan 5v
	if (mcore_event->HPDHandle.is5V != pCtx->is5V) {
		hrc_5V_info.PortID = pCtx->PortID;
		hrc_5V_info.is5V = pCtx->is5V;
#if (HDMIRX_PORT_NEEDREMAP)
		if (isRemap) {
			mcore_event->HPDHandle.is5V = pCtx->is5V;
			EVENT_INF("%s: port%d 5V detect ret:%d !!!!!\n", __func__, pCtx->PortID, mcore_event->HPDHandle.is5V);
			if (mcore_event->HPDHandle.is5V == true) {
				if (pCtx->isActivePort)
					HdmiRx_MVTOP_ICGEnable(0, true);

				// just let MCU handle this HPD, because some CEC issue.
				HdmiRx_SendHPDCMD(CMD_HOTPLUG_RESET, pCtx->PortID);
			} else {
				if (pCtx->isActivePort)
					HdmiRx_MVTOP_ICGEnable(0, false);

				//edid modle reset to slove dvi problem
				HdmiRx_Edid_Hardware_Edidreset();
				HdmiRx_Edid_Hardware_EdidUpdate(HDMI_ALL_PORT_MASK);

				// cpus need update GPIO state when lost 5V
				HdmiRx_SendHPDCMD(CMD_HOTPLUG_PULLDOWN, pCtx->PortID);
			}
			sunxi_hrc_call_notifiers(HPD_CHANGE, &hrc_5V_info); //call 5V to hrc
		}
#else
		mcore_event->HPDHandle.is5V = pCtx->is5V;
		EVENT_INF("port%d 5V detect ret:%d !!!!!\n", pCtx->PortID, mcore_event->HPDHandle.is5V);
		if (mcore_event->HPDHandle.is5V == true) {
			// just let MCU handle this HPD, because some CEC issue.
			HdmiRx_SendHPDCMD(CMD_HOTPLUG_RESET, pCtx->PortID);
		} else {
			// cpus need update GPIO state when lost 5V
			HdmiRx_SendHPDCMD(CMD_HOTPLUG_PULLDOWN, pCtx->PortID);
		}
		sunxi_hrc_call_notifiers(HPD_CHANGE, &hrc_5V_info);
#endif

		if ((pCtx->isActivePort) && (!pCtx->is5V)) {
			// scdc reset
			HdmiRx_SCDC_Reset();
			EventQueue_Flush(pCtx->PortID);
		}
	}
	// scan HPD event
	HPDStateHandle(pCtx->PortID);

	// scan scdc state
	if (pCtx->isActivePort)
		HdmiRx_Video_SCDC_Detect(&pCtx->videoCtx.SCDCContext);

	// scan port pathstate
	ScanPortPathState(pCtx);

	// scan hdcp14 reauth req
	if (pCtx->HDCP14_ReAuthTimeOutTick) {
		if (HDMIRx_CompareTickCount(pCtx->HDCP14_ReAuthTimeOutTick, HDCP14_REAUTH_TIMEOUT_TICK_MS)) {// reauth timeout
			pCtx->HDCP14_ReAuthTimeOutTick = 0;
			HdmiRx_HDCP14_ReAuthReq(false);
		}
	}
}

void THDMIRx_Eventinit(U8 port_id, struct THDMIRx_Event *core)
{
	hdmirx_inf("THDMIRx_Event start!!!\n");
	core_event[port_id] = core;
	memset(&core_event[port_id]->HPDHandle, 0, sizeof(HPDHandle_t));
	EventQueue_Init(port_id + 1);
	//HDMIRx_Initdelaywork(port_id);
}

void THDMIRx_Eventdeinit(U8 port_id)
{
	EventQueue_Deinit(port_id);
	hdmirx_inf("THDMIRx_Event end!!!\n");
}
