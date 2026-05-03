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
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kfifo.h>
#include "../aw_hdmirx_define.h"
#include "aw_hdmirx_core_drv.h"

struct aw_hdmirx_core_s *g_aw_core;
u8 debug_log_current_port_id;
bool SigmaHDR10enabled = true;

const char *ColorFormatStrings[] = {
	"kColorFormat_YUV420_888",
	"kColorFormat_YUV420_1088",
	"kColorFormat_YUV420_101010",
	"kColorFormat_YUV420_121212",

	"kColorFormat_YUV422_888",
	"kColorFormat_YUV422_1088",
	"kColorFormat_YUV422_101010",
	"kColorFormat_YUV422_121212",

	"kColorFormat_YUV444_888",
	"kColorFormat_YUV444_101010",
	"kColorFormat_YUV444_121212",

	"kColorFormat_RGB_888",
	"kColorFormat_RGB_101010",
	"kColorFormat_RGB_121212",

	"kColorFormat_YUV420_101010_P010_Low",
	"kColorFormat_YUV420_101010_P010_High",

	"kColorFormat_DETN_ALPHA_1_BIT",
	"kColorFormat_Max",
};

const char *SourceIdStrings[] = {
	"kSourceId_Dummy",
	"kSourceId_VideoDec",
	"kSourceId_Image",
	"kSourceId_HDMI_1",
	"kSourceId_HDMI_2",
	"kSourceId_HDMI_3",
	"kSourceId_HDMI_4",
	"kSourceId_CVBS_1",
	"kSourceId_CVBS_2",
	"kSourceId_CVBS_3",
	"kSourceId_ATV",
	"kSourceId_Max",
};

const char *AudioTypeStrings[] = {
	"UNKOWN",
	"L-PCM",
	"DSD",
	"DST",
	"HBR",
};

U32 DataMap_SourceIdToTfd(TSourceId source_id)
{
	U32 uc;

	switch (source_id) {
	case kSourceId_HDMI_1:
		uc = AI_SIGNAL_CHANNEL_HDMI1;
		break;
	case kSourceId_HDMI_2:
		uc = AI_SIGNAL_CHANNEL_HDMI2;
		break;
	case kSourceId_HDMI_3:
		uc = AI_SIGNAL_CHANNEL_HDMI3;
		break;
	default:
		uc = 0xf;
		break;
	}
	return uc;
}

U8 HDMIChannelMapToPort(U32 dwChannel)
{
	U8 uc;

	switch (dwChannel) {
	case AI_SIGNAL_CHANNEL_HDMI1:
		uc = HDMI_PORT1;
		break;
	case AI_SIGNAL_CHANNEL_HDMI2:
		uc = HDMI_PORT2;
		break;
	case AI_SIGNAL_CHANNEL_HDMI3:
		uc = HDMI_PORT3;
		break;
	default:
		uc = 0xf;
		break;
	}
	return uc;
}

ssize_t aw_core_edid_parse(char *buf)
{
	ssize_t n = 0;
	U8 edid_data[EDID_DATA_LEN] = {0};
	struct edid_info_s edid_info;
	HDMIRx_Edid_Getedid(edid_data);

	memset(&edid_info, 0, sizeof(struct edid_info_s));
	HDMIRx_Edid_Dumpedid(edid_data, buf, &n);
	HDMIRx_Edid_Parse_Block0(edid_data, &edid_info, buf, &n);
	HDMIRx_Edid_Parse_Cea_Block(&(edid_data[128]), &edid_info,  buf, &n);
	HDMIRx_Edid_Parse_Print(&edid_info, buf, &n);

	return n;
}

ssize_t aw_core_dump_SignalInfo(char *buf)
{
	TSignalInfo *psignal_info;
	ssize_t n = 0;

	psignal_info = HDMIRx_DisplayModuleCtx_GetSignalInfo();

	n += sprintf(buf + n, "video\n");
	n += sprintf(buf + n, "    source_id              = %s\n", SourceIdStrings[psignal_info->source_id]);
	n += sprintf(buf + n, "    signal_id              = 0x%x\n", psignal_info->signal_id);
	n += sprintf(buf + n, "    frame_rate             = 0x%x\n", psignal_info->frame_rate);
	n += sprintf(buf + n, "    b_interlace            = %d\n", psignal_info->b_interlace);
	n += sprintf(buf + n, "    color_format           = %s\n", ColorFormatStrings[psignal_info->color_format]);
	n += sprintf(buf + n, "    color_space            = 0x%x\n", psignal_info->color_space);
	n += sprintf(buf + n, "    resolution.h_size      = %d\n", psignal_info->ext.hdmi.timing.h_active);
	n += sprintf(buf + n, "    resolution.v_size      = %d\n", psignal_info->ext.hdmi.timing.v_active);
	n += sprintf(buf + n, "    hdr_mode               = %d\n", psignal_info->hdr_scheme);
	n += sprintf(buf + n, "    b_full_range           = %d\n", psignal_info->ext.hdmi.b_full_range);
	n += sprintf(buf + n, "    b_dvi_mode             = %d\n", psignal_info->ext.hdmi.b_dvi_mode);
	n += sprintf(buf + n, "    tmds_clock             = %d\n", psignal_info->ext.hdmi.tmds_clock);
	n += sprintf(buf + n, "    pixel_repetition       = %d\n", psignal_info->ext.hdmi.prmode);
	n += sprintf(buf + n, "    video_mute             = %d\n", g_aw_core->pDisplayModuleCtx->videomute);
	n += sprintf(buf + n, "audio\n");
	n += sprintf(buf + n, "    cts                    = %d\n", g_aw_core->pActivePort ?
														g_aw_core->pActivePort->PortCtx.audioCtx.ctx.CTS : 0);
	n += sprintf(buf + n, "    N                      = %d\n", g_aw_core->pActivePort ?
														g_aw_core->pActivePort->PortCtx.audioCtx.ctx.N : 0);
	n += sprintf(buf + n, "    audio_fs               = %d\n", g_aw_core->pActivePort ?
														g_aw_core->pActivePort->PortCtx.audioCtx.ctx.fs : 0);
	n += sprintf(buf + n, "    audio_PLLfs            = %d\n", g_aw_core->pActivePort ?
														g_aw_core->pActivePort->PortCtx.audioCtx.ctx.audioPLLfs : 0);
	n += sprintf(buf + n, "    audio_type             = %s\n", g_aw_core->pActivePort ?
														AudioTypeStrings[g_aw_core->pActivePort->PortCtx.audioCtx.ctx.audioType >> 4] : AudioTypeStrings[0]);
	n += sprintf(buf + n, "    fifo_errcnt            = %d\n", g_aw_core->pActivePort ?
														g_aw_core->pActivePort->PortCtx.audioCtx.ctx.fifoErrCnt : 0);
	n += sprintf(buf + n, "    audio_pkterrcnt        = %d\n", g_aw_core->pActivePort ?
														g_aw_core->pActivePort->PortCtx.audioCtx.ctx.audioPktErrCnt : 0);
	n += sprintf(buf + n, "    audio_mute             = %d\n", g_aw_core->pDisplayModuleCtx->audiomute);

	return n;
}

ssize_t aw_core_dump_status(char *buf)
{
	return THDMIRx_Port_Dump_Status(buf);
}

ssize_t aw_core_dump_hdcp14(char *buf)
{
	THdcp14Info hdcp14_info;
	ssize_t n = 0;
	memset(&hdcp14_info, 0, sizeof(THdcp14Info));
	HDMIRx_Port_GetHDCP14Status(&hdcp14_info);

	n += sprintf(buf + n, "aksv                   = 0x%x 0x%x 0x%x 0x%x 0x%x\n", \
				hdcp14_info.aksv[0], hdcp14_info.aksv[1], hdcp14_info.aksv[2], \
				hdcp14_info.aksv[3], hdcp14_info.aksv[4]);
	n += sprintf(buf + n, "bksv                   = 0x%x 0x%x 0x%x 0x%x 0x%x\n", \
				hdcp14_info.bksv[0], hdcp14_info.bksv[1], hdcp14_info.bksv[2], \
				hdcp14_info.bksv[3], hdcp14_info.bksv[4]);
	n += sprintf(buf + n, "keyload                = %d\n", hdcp14_info.hdcpkey);
	n += sprintf(buf + n, "authenticate           = %d\n", hdcp14_info.auth);

	return n;
}

void aw_core_SetPortMap(TSourceId source_id, U8 soc_port_id)
{
	hdmirx_inf("%s: source_id %d soc_port_id %d\n", __func__, source_id, soc_port_id);
	U8 i;
	HDMIPortMap_t *pMap = HdmiRx_GetPortMap();
	for (i = 0; i < HDMI_PORT_NUM; i++) {
		if (pMap[i].display_source_id == source_id) {
			hdmirx_inf("%s: Change Port(%d) Map, from %d to %d\n", __func__, \
			pMap[i].hdmi_port_id, pMap[i].mcu_port_id, soc_port_id);
			pMap[i].mcu_port_id = (MCUPortID_e)soc_port_id;
			break;
		}
	}

#if (HDMIRX_PORT_NEEDREMAP)
	HDMIRx_Port_SetRemap(true);
#endif
}

void aw_core_SetUpdateEdid(U8 *edid_table, U8 edid_version)
{
	HDMIRx_Edid_SetUpdateEdid(edid_table, edid_version);
}

void aw_core_SetHPDTimeInterval(U32 val)
{
	HDMIRx_Port_SetHPDTimeInterval(val);
}

void aw_core_SetARCEnable(bool bOn)
{
	HDMIRx_DataPath_SetARCEnable(bOn);
}

void aw_core_SetARCTXPath(ARCTXPath_e arc_tx_path)
{
	HDMIRx_DataPath_SetARCTXPath(arc_tx_path);
}

void aw_core_SetEdidVersion(U8 setting_version)
{
	HDMIRx_Edid_SetEdidVersion(setting_version);
}

void aw_core_SetPullHotplug(U8 port_id, U8 type)
{
	HDMIRx_Edid_SetPullHotplug(port_id, type);
}

void aw_core_Get5vState(U32 *pb5vstate)
{
	HdmiRx_Edid_Get_5VState(pb5vstate);
}

void aw_core_SetBlueScreen(bool bON)
{
	return HDMIRx_DisplayModuleCtx_SetBlueScreen(bON);
}

// switch port, active<-->bg
void aw_core_SwitchPort(struct THDMIRx_Port *pPort, bool isActive)
{
	if (!pPort)
		return;
	hdmirx_inf("%s: port id=%d isActive=%d tick=%d\n", __func__, pPort->PortCtx.PortID, isActive, HDMIRx_GetCurrentTick());
	if (isActive) {
		// bg--->active
		HDMIRx_Port_SetIsActivePort(pPort, true);
		HDMIRx_DataPath_Connect_DDC_Path(pPort);
		HDMIRx_DataPath_Connect_Link_Path(pPort);
		HDMIRx_DataPath_Connect_VideoOut_Path(pPort);
		HDMIRx_DataPath_Connect_AudioOut_Path(pPort);
	} else {
		// active-->bg
		HDMIRx_Port_SetIsActivePort(pPort, false);
		HDMIRx_DataPath_Disconnect_Link_Path(pPort);
		HDMIRx_DataPath_Disconnect_VideoOut_Path(pPort);
	}
}

void aw_core_SetActivePort(U8 PortID)
{
	U32 i;
	hdmirx_inf("%s: SetActivePort %d tick=%d\n", __func__, PortID, HDMIRx_GetCurrentTick());
	// switch port
	if (g_aw_core->pActivePort) {
		hdmirx_inf("%s: pActivePort is ture!!\n", __func__);
		if (g_aw_core->pActivePort->PortCtx.PortID != PortID) {
			aw_core_SwitchPort(g_aw_core->pActivePort, false);   //Cut away the original hdmi channel
			// find active port from BG
			for (i = 0; i < HDMI_PORT_NUM; i++) {
				struct THDMIRx_Port *pPort = g_aw_core->pPortArray[i];
				if (pPort->PortCtx.PortID == PortID) {
					g_aw_core->pActivePort = pPort;
					break;
				}
			}
			aw_core_SwitchPort(g_aw_core->pActivePort, true);  //Cut to current channel
		} else
			aw_core_SwitchPort(g_aw_core->pActivePort, true);  // from other source to active source

	} else {
		// find active port from BG
		//first select hdmi source
		hdmirx_inf("%s: pActivePort init!!\n", __func__);
		for (i = 0; i < HDMI_PORT_NUM; i++) {
			struct THDMIRx_Port *pPort = g_aw_core->pPortArray[i];
			hdmirx_inf("%s: pPort->PortCtx.PortID = %d PortID = %d\n", __func__, pPort->PortCtx.PortID, PortID);
			if (pPort->PortCtx.PortID == PortID) {
				g_aw_core->pActivePort = pPort;
				break;
			}
		}
		aw_core_SwitchPort(g_aw_core->pActivePort, true);
	}
	hdmirx_inf("1111pPort->PortCtx.TaskCtx.StateMachine_TaskActive = %d\n", g_aw_core->pActivePort->PortCtx.TaskCtx.StateMachine_TaskActive);
	hdmirx_inf("2222pPort->PortCtx.TaskCtx.StateMachine_TaskActive = %d\n",  g_aw_core->pPortArray[0]->PortCtx.TaskCtx.StateMachine_TaskActive);
	hdmirx_inf("2222pPort->PortCtx.TaskCtx.StateMachine_TaskActive = %d\n",  g_aw_core->pPortArray[1]->PortCtx.TaskCtx.StateMachine_TaskActive);
}

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
int aw_hdmirx_core_set_cecbase(void)
{
	HdmiRx_Cec_Hardware_set_regbase(g_aw_core->reg_edid_top);
	return 0;
}

int aw_core_Cec_Enable(void)
{
	HdmiRx_Cec_Hardware_assist_ctrl();
	udelay(200);
	HdmiRx_Cec_Hardware_enable();
	return 0;
}

int aw_core_Cec_Disable(void)
{
	HdmiRx_Cec_Hardware_disable();
	udelay(200);

	return 0;
}

s32 aw_core_Cec_Receive(unsigned char *msg, unsigned *size)
{
	return HdmiRx_Cec_Hardware_receive_frame(msg, size);
}

s32 aw_core_Cec_Send(unsigned char *msg, unsigned size, unsigned frame_type)
{
	u32 dw_frame_type = 0;

	switch (frame_type) {
	case AW_HDMI_CEC_FRAME_TYPE_RETRY:
		dw_frame_type = CEC_CTRL_FRAME_TYP_RETRY;
		break;
	case AW_HDMI_CEC_FRAME_TYPE_NORMAL:
	default:
		dw_frame_type = CEC_CTRL_FRAME_TYP_NORMAL;
		break;
	case AW_HDMI_CEC_FRAME_TYPE_IMMED:
		dw_frame_type = CEC_CTRL_FRAME_TYP_IMMED;
		break;
	}
	return HdmiRx_Cec_Hardware_send_frame(msg, size, dw_frame_type);
}

int aw_core_Cec_Set_Logical_Addr(unsigned int addr)
{
	return HdmiRx_Cec_Hardware_set_logical_addr(addr);
}

int aw_core_Cec_interrupt_get_state(void)
{
	u32 dw_state = HdmiRx_Cec_Hardware_interrupt_get_state();
	u32 state = 0;

	if (dw_state & IH_CEC_STAT0_WAKEUP_MASK)
		state |= AW_HDMI_CEC_STAT_WAKEUP;
	if (dw_state & IH_CEC_STAT0_DONE_MASK)
		state |= AW_HDMI_CEC_STAT_DONE;
	if (dw_state & IH_CEC_STAT0_EOM_MASK)
		state |= AW_HDMI_CEC_STAT_EOM;
	if (dw_state & IH_CEC_STAT0_NACK_MASK)
		state |= AW_HDMI_CEC_STAT_NACK;
	if (dw_state & IH_CEC_STAT0_ARB_LOST_MASK)
		state |= AW_HDMI_CEC_STAT_ARBLOST;
	if (dw_state & IH_CEC_STAT0_ERROR_INITIATOR_MASK)
		state |= AW_HDMI_CEC_STAT_ERROR_INIT;
	if (dw_state & IH_CEC_STAT0_ERROR_FOLLOW_MASK)
		state |= AW_HDMI_CEC_STAT_ERROR_FOLL;

	return state;
}

void aw_core_Cec_interrupt_clear_state(unsigned state)
{
	u32 dw_state = 0;

	if (state & AW_HDMI_CEC_STAT_WAKEUP)
		dw_state |= IH_CEC_STAT0_WAKEUP_MASK;
	if (state & AW_HDMI_CEC_STAT_DONE)
		dw_state |= IH_CEC_STAT0_DONE_MASK;
	if (state & AW_HDMI_CEC_STAT_EOM)
		dw_state |= IH_CEC_STAT0_EOM_MASK;
	if (state & AW_HDMI_CEC_STAT_NACK)
		dw_state |= IH_CEC_STAT0_NACK_MASK;
	if (state & AW_HDMI_CEC_STAT_ARBLOST)
		dw_state |= IH_CEC_STAT0_ARB_LOST_MASK;
	if (state & AW_HDMI_CEC_STAT_ERROR_INIT)
		dw_state |= IH_CEC_STAT0_ERROR_INITIATOR_MASK;
	if (state & AW_HDMI_CEC_STAT_ERROR_FOLL)
		dw_state |= IH_CEC_STAT0_ERROR_FOLLOW_MASK;

	HdmiRx_Cec_Hardware_interrupt_clear_state(dw_state);
}

#endif

bool aw_core_Enable(TSourceId source_id)
{
	hdmirx_inf("%s: source_id %d HDMIRx_Enable\n", __func__, source_id);
	HRESULT ret = HDMIRx_DisplayModuleCtx_Enable(_OUTPUT_MP_, source_id);

	if (SUCCEEDED(ret)) {
		return true;
	}

	return false;
}

bool aw_core_AfterEnable(TSourceId source_id)
{
	hdmirx_inf("%s: source_id %d AfterEnable\n", __func__, source_id);
	U32 channel_id = DataMap_SourceIdToTfd(source_id);
	aw_core_SetActivePort(HDMIChannelMapToPort(channel_id));
	CHECK_POINTER(g_aw_core->pActivePort, E_FAIL);
	HDMIRx_DisplayModuleCtx_AfterEnable();
	if (g_aw_core->pActivePort->PortCtx.is5V) {
		aw_core_SetBlueScreen(true);
	} else {
		HDMIRx_DisplayModuleCtx_SetSignal(AI_SIGNAL_MODE_NO_SIGNAL, &g_aw_core->pActivePort->PortCtx);
		return true;
	}

	return true;
}

bool aw_core_Disable(void)
{
	hdmirx_inf("%s: HDMIRx_Disable\n", __func__);
	HRESULT ret = HDMIRx_DisplayModuleCtx_Disable(_OUTPUT_MP_);

	// for power saving, disable clk
	HdmiRx_MVTOP_ICGEnable(0, false);

	if (SUCCEEDED(ret)) {
		return true;
	}

	return false;
}


static int aw_core_ScanTask(void *parg)
{
	hdmirx_inf("throop HDMIRx_ScanTask\n");

	U8 i;
	while (1) {
		if (kthread_should_stop())
			break;

		for (i = 0; i < SizeOfArray(g_aw_core->pPortArray); i++) {
			HDMIRx_Port_EventTask(i + 1);
		}

		msleep(10);
	}
	return 0;
}

int aw_core_scan_thread_init(void)
{
	g_aw_core->hdmirx_scan_task = kthread_create(aw_core_ScanTask, (void *)0, "hdmirx scan");
	if (IS_ERR(g_aw_core->hdmirx_scan_task)) {
		hdmirx_err("%s: Unable to start kernel thread %s.\n",
				__func__, "hdmirx scan");
		g_aw_core->hdmirx_scan_task = NULL;
		return -1;
	}
	wake_up_process(g_aw_core->hdmirx_scan_task);
	return 0;
}

void aw_core_scan_thread_exit(void)
{
	if (g_aw_core->hdmirx_scan_task) {
		kthread_stop(g_aw_core->hdmirx_scan_task);
		g_aw_core->hdmirx_scan_task = NULL;
	}
}

static int aw_core_hdcp_task(void *parg)
{
	hdmirx_inf("throop HDMIRx_HdcpTask\n");

	while (1) {
		if (kthread_should_stop())
			break;

		if (g_aw_core->pActivePort) {
			debug_log_current_port_id = g_aw_core->pActivePort->PortCtx.PortID;
			//hdmirx_inf("1debug_log_current_port_id = %d\n", debug_log_current_port_id);
			HDMIRx_Port_HDCPTask(debug_log_current_port_id);
		}
		msleep(1);
	}
	return 0;
}

int aw_core_hdcp_thread_init(void)
{
	hdmirx_inf("%s: start.\n", __func__);

	g_aw_core->hdmirx_hdcp_task = kthread_create(aw_core_hdcp_task, (void *)0, "hdmirx hdcp");
	if (IS_ERR(g_aw_core->hdmirx_hdcp_task)) {
		hdmirx_err("%s: Unable to start kernel thread %s.\n",
				__func__, "hdmirx hdcp");
		g_aw_core->hdmirx_hdcp_task = NULL;
		return -1;
	}
	wake_up_process(g_aw_core->hdmirx_hdcp_task);
	return 0;
}

void aw_core_hdcp_thread_exit(void)
{
	if (g_aw_core->hdmirx_hdcp_task) {
		kthread_stop(g_aw_core->hdmirx_hdcp_task);
		g_aw_core->hdmirx_hdcp_task = NULL;
	}
}

static int aw_core_StateMachineTask(void *parg)
{
	hdmirx_inf("throop HDMIRx_StateMachineTask\n");

	while (1) {
		if (kthread_should_stop())
			break;

		if (g_aw_core->pActivePort) {
			debug_log_current_port_id = g_aw_core->pActivePort->PortCtx.PortID;
			//hdmirx_inf("1debug_log_current_port_id = %d\n", debug_log_current_port_id);
			HDMIRx_Port_StateMachineTask(debug_log_current_port_id);
		}
		msleep(10);
	}
	return 0;
}

int aw_core_statemachine_thread_init(void)
{
	hdmirx_inf("%s: start.\n", __func__);
	g_aw_core->hdmirx_statemachine_task = kthread_create(aw_core_StateMachineTask, (void *)0, "hdmirx statemachine");
	if (IS_ERR(g_aw_core->hdmirx_statemachine_task)) {
		hdmirx_err("%s: Unable to start kernel thread %s.\n",
				__func__, "hdmirx statemachine");
		g_aw_core->hdmirx_statemachine_task = NULL;
		return -1;
	}
	wake_up_process(g_aw_core->hdmirx_statemachine_task);
	return 0;
}

void aw_core_statemachine_thread_exit(void)
{
	if (g_aw_core->hdmirx_statemachine_task) {
		kthread_stop(g_aw_core->hdmirx_statemachine_task);
		g_aw_core->hdmirx_statemachine_task = NULL;
	}
}

int aw_hdmirx_core_init_thread(void)
{
	if (aw_core_statemachine_thread_init() != 0) {
		hdmirx_err("%s: hdmirx statemachine thread init failed!!!\n", __func__);
		return -1;
	}

	if (aw_core_hdcp_thread_init() != 0) {
		hdmirx_err("%s: hdmirx hdcp thread init failed!!!\n", __func__);
		return -1;
	}

	if (aw_core_scan_thread_init() != 0) {
		hdmirx_err("%s: hdmirx scan thread init failed!!!\n", __func__);
		return -1;
	}

	return 0;
}

void aw_hdmirx_core_set_regbase(void)
{
	HdmiRx_set_regbase(g_aw_core->reg_base_vs);
	HdmiRx_Audio_set_regbase(g_aw_core->reg_base_vs);
	HdmiRx_Video_set_regbase(g_aw_core->reg_base_vs);
	HdmiRx_Edid_Hardware_set_regbase(g_aw_core->reg_edid_top);
}

int aw_hdmirx_core_init(struct aw_hdmirx_core_s *core)
{
	U32 i;
	int ret = 0;
	g_aw_core = core;
	hdmirx_inf("%s: start.\n", __func__);

	aw_hdmirx_core_set_regbase();

	HdmiRx_Edid_Hardware_set_top_cfg();
	HdmiRx_Edid_Hardware_set_5v_cfg();

	g_aw_core->pActivePort = NULL;

	g_aw_core->pDisplayModuleCtx = kzalloc(sizeof(struct THDMIRx_DisplayModuleCtx), GFP_KERNEL);
	if (!g_aw_core->pDisplayModuleCtx) {
		hdmirx_err("%s: could not allocated pDisplayModuleCtx!\n", __func__);
		return -1;
	}

	g_aw_core->pDataPath = kzalloc(sizeof(struct THDMIRx_DataPath), GFP_KERNEL);
	if (!g_aw_core->pDataPath) {
		hdmirx_err("%s: could not allocated pDataPath!\n", __func__);
		return -1;
	}

	memset(g_aw_core->pPortArray, 0, sizeof(struct THDMIRx_Port *) * HDMI_PORT_NUM);
	THDMIRx_DisplayModuleCtxinit(g_aw_core->pDisplayModuleCtx);
	THDMIRx_DataPathinit(g_aw_core->pDataPath);
	THDMIRx_Edidinit();

	for (i = 0; i < HDMI_PORT_NUM; i++) {
		g_aw_core->pPortArray[i] = kzalloc(sizeof(struct THDMIRx_Port), GFP_KERNEL);
		if (!g_aw_core->pPortArray[i]) {
			hdmirx_err("%s: could not allocated pPortArray!\n", __func__);
			return -1;
		}
		THDMIRx_Portinit(i, g_aw_core->pPortArray[i], g_aw_core->pDisplayModuleCtx);
		HDMIRx_DataPath_SetPortArray(g_aw_core->pPortArray[i]);
	}
	hdmirx_inf("g_aw_core->pPortArray[0]->PortCtx.PortID = %d\n", g_aw_core->pPortArray[0]->PortCtx.PortID);
	hdmirx_inf("g_aw_core->pPortArray[1]->PortCtx.PortID = %d\n", g_aw_core->pPortArray[1]->PortCtx.PortID);

	ret = aw_hdmirx_core_init_thread();
	if (ret != 0) {
		hdmirx_err("%s: aw hdmi initial thread failed!!!\n", __func__);
		return -1;
	}

	return 0;
}

void aw_hdmirx_core_exit(struct aw_hdmirx_core_s *g_aw_core)
{
	int i;
	kfree(g_aw_core->pDisplayModuleCtx);
	for (i = 0; i < HDMI_PORT_NUM; i++) {
		if (g_aw_core->pPortArray[i]->pHDMIRxEvent.pEventQueue) {
			kfifo_free(g_aw_core->pPortArray[i]->pHDMIRxEvent.pEventQueue);
			kfree(g_aw_core->pPortArray[i]->pHDMIRxEvent.pEventQueue);
			hdmirx_inf("kfifo_free\n");
		}

		if (g_aw_core->pPortArray[i]) {
			kfree(g_aw_core->pPortArray[i]);
			hdmirx_inf("kfree pPortArray\n");
		}

		HDMIRx_Edid_DeinitHpdtimer(i);
	}
	kfree(g_aw_core->pDataPath);
	kfree(g_aw_core);
}
