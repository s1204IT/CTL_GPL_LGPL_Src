/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2023 - 2030 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023-2030 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: zhengwanyu  <zhengwanyu@allwinnertech.com>
 */



#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#include "img_types.h"
#include "pvrsrv_error.h"
#include "pvrsrv_device.h"
#include "servicesext.h"
#include <linux/version.h>

#if defined(SUPPORT_PDVFS) || defined(SUPPORT_LINUX_DVFS)
#include "pvr_dvfs.h"
#endif

#define OPP_MAX_NUM 20

enum sunxi_gpu_power_runtime_status {
	GPU_POWER_NONE = -1,
	GPU_POWER_RUNTIME_OFF = 0,
	GPU_POWER_RUNTIME_ON,
};

enum sunxi_gpu_power_suspend_resume_status {
	GPU_POWER_SUSPEND_RESUME_NONE = -1,
	GPU_POWER_SUSPEND = 0,
	GPU_POWER_RESUME,
};

struct sunxi_platform {
	/*Linux devices driver related informations*/
	struct device *dev;

	bool initialized;

	/*register related informations*/
	u32 reg_base;
	void __iomem *gpu_reg;
	u32 reg_size;

	/*interruption related*/
	u32 irq_num;
#if defined(SUPPORT_PDVFS) || defined(SUPPORT_LINUX_DVFS)
	u32 dvfs_irq_num;

	IMG_OPP asOPPTable[OPP_MAX_NUM];
	u32     ui32OPPTableSize;
#endif
	struct mutex power_lock;
	/*power related*/
	struct regulator *regula;
	enum sunxi_gpu_power_runtime_status runtime_power_status;
	enum sunxi_gpu_power_suspend_resume_status suspend_resume_power_status;

	/*clk related*/
	struct clk *clk_parent;
	struct clk *clk_core;
	struct clk *clk_bus;
	struct clk *clk_800;
	struct clk *clk_600;
	struct clk *clk_400;
	struct clk *clk_300;
	struct clk *clk_200;
	u32 clk_rate;

	struct reset_control *rst_bus;
	struct reset_control *rst_bus_smmu_gpu0;
	struct reset_control *rst_bus_smmu_gpu1;

	struct thermal_zone_device *tz;

	DI_GROUP *psGroup;
	DI_ENTRY *psOpp;
};

IMG_UINT32 sunxi_get_device_clk_rate(IMG_HANDLE hSysData);
void sunxi_set_device_clk_rate(unsigned long rate);
int sunxi_platform_init(struct device *dev);
void sunxi_platform_term(void);
void sunxiSetFrequency(IMG_UINT32 ui32Frequency);
void sunxiSetVoltage(IMG_UINT32 ui32Volt);
PVRSRV_ERROR sunxiPrePowerState(IMG_HANDLE hSysData,
					 PVRSRV_SYS_POWER_STATE eNewPowerState,
					 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
					 PVRSRV_POWER_FLAGS ePwrFlags);
PVRSRV_ERROR sunxiPostPowerState(IMG_HANDLE hSysData,
					  PVRSRV_SYS_POWER_STATE eNewPowerState,
					  PVRSRV_SYS_POWER_STATE eCurrentPowerState,
					  PVRSRV_POWER_FLAGS ePwrFlags);

#if defined(SUPPORT_PDVFS) || defined(SUPPORT_LINUX_DVFS)
#if defined(CONFIG_DEVFREQ_THERMAL)
unsigned long sunxi_get_static_power(unsigned long voltage);

unsigned long sunxi_get_dynamic_power(unsigned long freq,
					       unsigned long voltage);
#endif //~CONFIG_DEVFREQ_THERMAL
#endif //~DVFS

#endif /* _PLATFORM_H_ */
