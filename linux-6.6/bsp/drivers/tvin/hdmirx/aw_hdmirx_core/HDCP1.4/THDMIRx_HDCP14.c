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
#include "../../aw_hdmirx_define.h"
#include "THDMIRx_HDCP14.h"

struct THDMIRx_HDCP14 *core_hdcp14[HDMI_PORT_NUM];

struct THDMIRx_HDCP14 *HDMIRx_get_pHDCP14(U8 PortID)
{
	if (PortID > HDMI_PORT_NUM - 1)
		return NULL;
	return core_hdcp14[PortID - 1];
}

void HDMIRx_HDCP14Task(U8 PortID)
{
	struct THDMIRx_HDCP14 *mcore_hdcp14;

	mcore_hdcp14 = HDMIRx_get_pHDCP14(PortID);
	if (mcore_hdcp14 == NULL)
		return;

	HdmiRx_HDCP14_ISR_Handle(PortID, &(mcore_hdcp14->pPortCtx->isHDCPRunning));
}

void HdmiRx_HDCP14GetStatus(THdcp14Info *pHdcp14Info)
{
	HdmiRx_HDCP14_GetStatus(pHdcp14Info);
}

void THDMIRx_HDCP14init(U8 port_id, struct THDMIRx_HDCP14 *pHDCP14, struct HDMIRx_PortContext *pCtx)
{
	core_hdcp14[port_id] = pHDCP14;
	core_hdcp14[port_id]->pPortCtx = pCtx;
	if (!HdmiRx_HDCP14_LoadKey()) {
		hdmirx_err("%s: load hdcp1.4 key fail!!!!", __func__);
	}

	HdmiRx_HDCP14_EnableAKSV_ISR();
}
