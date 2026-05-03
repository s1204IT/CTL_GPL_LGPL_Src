/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
 /*
  * Hawkview ISP - sun300iw1_vin_cfg.h module
  *
  * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
  *
  * Authors:  Liu chensheng <liuchensheng@allwinnertech.com>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */

#ifndef _SUN300IW1_VIN_CFG_H_
#define _SUN300IW1_VIN_CFG_H_

#define MEMRESERVE 0x86000000
#define MEMRESERVE_SIZE 0x1400000

#define CSI_CCU_REGS_BASE			0x45800000
#define CSI_TOP_REGS_BASE			0x45800800

#define CSI0_REGS_BASE				0x45820000
#define CSI1_REGS_BASE				0x45821000

#define ISP_REGS_BASE				0x45900000

#define VIPP0_REGS_BASE				0x45910000
#define VIPP1_REGS_BASE				0x45910400

#define CSI_DMA0_REG_BASE			0x45830000
#define CSI_DMA1_REG_BASE			0x45831000

#define GPIO_REGS_VBASE				0x42000000

#define VIN_MAX_DEV			6
#define VIN_MAX_CSI			2
#define VIN_MAX_TDM			1
#define VIN_MAX_CCI			2
#define VIN_MAX_MIPI		2
#define VIN_MAX_ISP			5
#define VIN_MAX_SCALER		6

#define MAX_CH_NUM			4

#endif /* _SUN300IW1_VIN_CFG_H_ */
