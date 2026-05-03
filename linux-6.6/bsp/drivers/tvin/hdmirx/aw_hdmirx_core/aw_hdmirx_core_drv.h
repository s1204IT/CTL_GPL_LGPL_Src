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
#ifndef _AW_HDMIRX_CORE_DRV_H_
#define _AW_HDMIRX_CORE_DRV_H_

#include "THDMIRx_Port.h"
#include "THDMIRx_Edid.h"
#include "THDMIRx_DataPath.h"
#include "THDMIRx_DisplayModuleCtx.h"
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
#include "hardware/THDMIRx_TV_Cec_Driver.h"
#endif

/**
 * aw hdmirx core
 */
struct aw_hdmirx_core_s {
	/* HDMI RX Controller */
	uintptr_t          reg_base_vs;
	uintptr_t          reg_edid_top;
	struct task_struct	 *hdmirx_statemachine_task;
	struct task_struct	 *hdmirx_hdcp_task;
	struct task_struct	 *hdmirx_scan_task;

	// HDMI
	struct THDMIRx_Port *pActivePort;
	struct THDMIRx_Port *pPortArray[HDMI_PORT_NUM];
	struct THDMIRx_DataPath *pDataPath;

	// Display
	struct THDMIRx_DisplayModuleCtx *pDisplayModuleCtx;
};

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
enum {
	AW_HDMI_CEC_STAT_WAKEUP 	= BIT(0),
	AW_HDMI_CEC_STAT_DONE		= BIT(1),
	AW_HDMI_CEC_STAT_EOM		= BIT(2),
	AW_HDMI_CEC_STAT_NACK		= BIT(3),
	AW_HDMI_CEC_STAT_ARBLOST	= BIT(4),
	AW_HDMI_CEC_STAT_ERROR_INIT	= BIT(5),
	AW_HDMI_CEC_STAT_ERROR_FOLL	= BIT(6),
};

enum {
	AW_HDMI_CEC_FRAME_TYPE_RETRY    = 0,
	AW_HDMI_CEC_FRAME_TYPE_NORMAL   = 1,
	AW_HDMI_CEC_FRAME_TYPE_IMMED    = 2,
};

int aw_hdmirx_core_set_cecbase(void);

int aw_core_Cec_Enable(void);

int aw_core_Cec_Disable(void);

s32 aw_core_Cec_Send(unsigned char *msg, unsigned size, unsigned frame_type);

s32 aw_core_Cec_Receive(unsigned char *msg, unsigned *size);

int aw_core_Cec_Set_Logical_Addr(unsigned int addr);

int aw_core_Cec_interrupt_get_state(void);

void aw_core_Cec_interrupt_clear_state(unsigned state);


#endif

ssize_t aw_core_edid_parse(char *buf);

ssize_t aw_core_dump_SignalInfo(char *buf);

ssize_t aw_core_dump_status(char *buf);

ssize_t aw_core_dump_hdcp14(char *buf);

bool aw_core_Enable(TSourceId source_id);

bool aw_core_AfterEnable(TSourceId source_id);

bool aw_core_Disable(void);

void aw_core_SetPortMap(TSourceId source_id, U8 soc_port_id);

void aw_core_SetHPDTimeInterval(U32 val);

void aw_core_SetARCEnable(bool bOn);

void aw_core_SetARCTXPath(ARCTXPath_e arc_tx_path);

void aw_core_SetUpdateEdid(U8 *edid_table, U8 edid_version);

void aw_core_SetEdidVersion(U8 setting_version);

void aw_core_SetPullHotplug(U8 port_id, U8 type);

void aw_core_Get5vState(U32 *pb5vstate);

void aw_core_scan_thread_exit(void);

void aw_core_hdcp_thread_exit(void);

void aw_core_statemachine_thread_exit(void);

int aw_hdmirx_core_init_thread(void);

int aw_hdmirx_core_init(struct aw_hdmirx_core_s *core);

void aw_hdmirx_core_exit(struct aw_hdmirx_core_s *core);

#endif /* _AW_HDMIRX_CORE_DRV_H_ */
