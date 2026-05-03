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
#ifndef _THDMIRX_DATAPATH_H_
#define _THDMIRX_DATAPATH_H_
#include "THDMIRx_System.h"
#include "hardware/THDMIRx_TV_Driver.h"

struct THDMIRx_DataPath {
	bool m_bDetect5VValid;  // app will set it to control 5v detect
	//struct aw_hdmirx_core_port      pPortArray[HDMI_PORT_NUM];
	struct THDMIRx_Port *pPortArray[HDMI_PORT_NUM];
};

void HDMIRx_DataPath_SetARCEnable(bool bOn);

void HDMIRx_DataPath_SetARCTXPath(ARCTXPath_e arc_tx_path);

void HDMIRx_DataPath_SetPortArray(struct THDMIRx_Port *pPort);

void HDMIRx_DataPath_Connect_DDC_Path(struct THDMIRx_Port *pPort);

void HDMIRx_DataPath_Connect_Link_Path(struct THDMIRx_Port *pPort);

void HDMIRx_DataPath_Disconnect_Link_Path(struct THDMIRx_Port *pPort);

void HDMIRx_DataPath_Connect_VideoOut_Path(struct THDMIRx_Port *pPort);

void HDMIRx_DataPath_Disconnect_VideoOut_Path(struct THDMIRx_Port *pPort);

void HDMIRx_DataPath_Connect_AudioOut_Path(struct THDMIRx_Port *pPort);

void THDMIRx_DataPathinit(struct THDMIRx_DataPath *core);

#endif /* _THDMIRX_DATAPATH_H_ */
