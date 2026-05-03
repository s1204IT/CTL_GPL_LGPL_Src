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
#ifndef _THDMIRX_HDCP14_H_
#define _THDMIRX_HDCP14_H_
#include "../THDMIRx_System.h"
#include "../hardware/THDMIRx_TV_Driver.h"
#include "../THDMIRx_PortCtx.h"

struct THDMIRx_HDCP14 {
	struct HDMIRx_PortContext *pPortCtx;
};

void HDMIRx_HDCP14Task(U8 PortID);

void HdmiRx_HDCP14GetStatus(THdcp14Info *pHdcp14Info);

void THDMIRx_HDCP14init(U8 port_id, struct THDMIRx_HDCP14 *pHDCP14, struct HDMIRx_PortContext *pCtx);

#endif /* _THDMIRX_HDCP14_H_ */
