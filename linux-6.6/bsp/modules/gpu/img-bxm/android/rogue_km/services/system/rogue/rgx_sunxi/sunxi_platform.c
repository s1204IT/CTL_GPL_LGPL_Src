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
 * Author: zhengwanyu<zhengwanyu@allwinnertech.com>
 */

#include <asm/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>

#include <sunxi-sid.h>

#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#endif
#include <linux/thermal.h>
#include <linux/interrupt.h>
#include "rgxinit.h"

#include "rgxdevice.h"
#include "osdi_impl.h"
#include "di_common.h"

#include "sunxi_platform.h"

#if defined(SUNXI_DVFS_CTRL_ENABLE)
#include "sunxi_dvfs_ctrl.h"
#endif

#define GPU_PARENT_CLK_NAME "clk_parent"
#define GPU_CORE_CLK_NAME   "clk"
#define GPU_CLK_BUS_NAME    "clk_bus"
#define GPU_CLK_800_NAME    "clk_800"
#define GPU_CLK_600_NAME    "clk_600"
#define GPU_CLK_400_NAME    "clk_400"
#define GPU_CLK_300_NAME    "clk_300"
#define GPU_CLK_200_NAME    "clk_200"

#define GPU_RESET_BUS	     "reset_bus"
#define GPU0_RESET_BUS_SMMU  "reset_bus_smmu_gpu0"
#define GPU1_RESET_BUS_SMMU  "reset_bus_smmu_gpu1"

#define PPU_GPU_CORE 0x07066000

#define DEFAULT_GPU_RATE 600000000

#define GPU_THERMAL_ZONE       "gpu_thermal_zone"

struct sunxi_platform *sunxi_data;

#define USE_FPGA 1
#if defined(USE_FPGA)
static void sunxi_ppu_init_gpu(void)
{
	void __iomem *ioaddr;

	ioaddr = ioremap(PPU_GPU_CORE + 0x20, 4);

	writel(0x0, ioaddr);
	iounmap(ioaddr);
}


static void sunxi_ppu_enable_gpu(void)
{
	void __iomem *ioaddr;

	ioaddr = ioremap(PPU_GPU_CORE, 4);

	writel(0x8, ioaddr);
	iounmap(ioaddr);
}

static void sunxi_ppu_disable_gpu(void)
{
	void __iomem *ioaddr;

	ioaddr = ioremap(PPU_GPU_CORE, 4);

	writel(0x0, ioaddr);
	iounmap(ioaddr);
}

#endif

static int sunxi_get_clks_wrap(struct device *dev)
{
#if defined(CONFIG_OF)
	sunxi_data->clk_parent = of_clk_get_by_name(dev->of_node, GPU_PARENT_CLK_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_parent)) {
		dev_err(dev, "failed to get GPU clk_parent clock");
		return -1;
	}

	sunxi_data->clk_core = of_clk_get_by_name(dev->of_node, GPU_CORE_CLK_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_core)) {
		dev_err(dev, "failed to get GPU clk_core clock");
		return -1;
	}

	sunxi_data->clk_bus = of_clk_get_by_name(dev->of_node, GPU_CLK_BUS_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_core)) {
		dev_err(dev, "failed to get GPU clk_bus clock");
		return -1;
	}

	sunxi_data->clk_800 = of_clk_get_by_name(dev->of_node, GPU_CLK_800_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_800)) {
		dev_err(dev, "failed to get GPU clk_800 clock");
		return -1;
	}

	sunxi_data->clk_600 = of_clk_get_by_name(dev->of_node, GPU_CLK_600_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_600)) {
		dev_err(dev, "failed to get GPU clk_600 clock");
		return -1;
	}

	sunxi_data->clk_400 = of_clk_get_by_name(dev->of_node, GPU_CLK_400_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_400)) {
		dev_err(dev, "failed to get GPU clk_400 clock");
		return -1;
	}

	sunxi_data->clk_300 = of_clk_get_by_name(dev->of_node, GPU_CLK_300_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_300)) {
		dev_err(dev, "failed to get GPU clk_300 clock");
		return -1;
	}

	sunxi_data->clk_200 = of_clk_get_by_name(dev->of_node, GPU_CLK_200_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_200)) {
		dev_err(dev, "failed to get GPU clk_200 clock");
		return -1;
	}

	sunxi_data->rst_bus = devm_reset_control_get(dev, GPU_RESET_BUS);
	if (IS_ERR_OR_NULL(sunxi_data->rst_bus)) {
		dev_err(dev, "failed to get %s clock", GPU_RESET_BUS);
		return -1;
	}
#endif /* defined(CONFIG_OF) */

	return 0;
}

#define GPU_CLK 0x02002b20
static void sunxi_enable_clks_wrap(struct device *dev)
{
	void __iomem *ioaddr;
	unsigned long value;

	if (sunxi_data->clk_parent)
		clk_prepare_enable(sunxi_data->clk_parent);
	else
		dev_err(dev, "%s No clk_parent\n", __func__);

	if (sunxi_data->clk_800)
		clk_prepare_enable(sunxi_data->clk_800);
	else
		dev_err(dev, "%s No clk_800\n", __func__);

	if (sunxi_data->clk_600)
		clk_prepare_enable(sunxi_data->clk_600);
	else
		dev_err(dev, "%s No clk_600\n", __func__);

	if (sunxi_data->clk_400)
		clk_prepare_enable(sunxi_data->clk_400);
	else
		dev_err(dev, "%s No clk_400\n", __func__);

	if (sunxi_data->clk_300)
		clk_prepare_enable(sunxi_data->clk_300);
	else
		dev_err(dev, "%s No clk_300\n", __func__);

	if (sunxi_data->clk_200)
		clk_prepare_enable(sunxi_data->clk_200);
	else
		dev_err(dev, "%s No clk_200\n", __func__);

	if (sunxi_data->rst_bus)
		reset_control_deassert(sunxi_data->rst_bus);
	else
		dev_err(dev, "%s No rst_bus\n", __func__);

	if (sunxi_data->clk_bus)
		clk_prepare_enable(sunxi_data->clk_bus);
	else
		dev_err(dev, "%s No clk_bus\n", __func__);

	if (sunxi_data->clk_core)
		clk_prepare_enable(sunxi_data->clk_core);
	else
		dev_err(dev, "%s No clk_core\n", __func__);

	ioaddr = ioremap(GPU_CLK, 4);
	value = readl(ioaddr);

	writel(value | 1 << 27, ioaddr);
	iounmap(ioaddr);

}

static void sunxi_disable_clks_wrap(struct device *dev)
{
	if (sunxi_data->rst_bus)
		clk_disable_unprepare(sunxi_data->clk_bus);
	else
		dev_err(dev, "%s No rst_bus\n", __func__);

	if (sunxi_data->clk_core)
		clk_disable_unprepare(sunxi_data->clk_core);
	else
		dev_err(dev, "%s No clk_core\n", __func__);

	if (sunxi_data->clk_200)
		clk_disable_unprepare(sunxi_data->clk_200);
	else
		dev_err(dev, "%s No clk_200\n", __func__);

	if (sunxi_data->clk_300)
		clk_disable_unprepare(sunxi_data->clk_300);
	else
		dev_err(dev, "%s No clk_300\n", __func__);

	if (sunxi_data->clk_400)
		clk_disable_unprepare(sunxi_data->clk_400);
	else
		dev_err(dev, "%s No clk_400\n", __func__);

	if (sunxi_data->clk_600)
		clk_disable_unprepare(sunxi_data->clk_600);
	else
		dev_err(dev, "%s No clk_600\n", __func__);

	if (sunxi_data->clk_800)
		clk_disable_unprepare(sunxi_data->clk_800);
	else
		dev_err(dev, "%s No clk_800\n", __func__);

	if (sunxi_data->clk_parent)
		clk_disable_unprepare(sunxi_data->clk_parent);
	else
		dev_err(dev, "%s No clk_parent\n", __func__);

	if (sunxi_data->rst_bus)
		reset_control_assert(sunxi_data->rst_bus);
	else
		dev_err(dev, "%s No clk_bus\n", __func__);
}

void sunxi_set_device_clk_rate(unsigned long rate)
{
	//clk_set_rate(sunxi_data->clk_parent, rate);
	sunxiSetFrequency(rate);
}

IMG_UINT32 sunxi_get_device_clk_rate(IMG_HANDLE hSysData)
{
	return clk_get_rate(sunxi_data->clk_core);
}

static void writeReg(u32 addr, u32 val)
{
	void __iomem *ioaddr;

	ioaddr = ioremap(addr, 4);
	if (!ioaddr) {
		return;
	}
	writel(val, ioaddr);
	iounmap(ioaddr);

}

static void cleanInterrupt(void)
{
	//u32 val = 0;
	writeReg(0x018008b0, 0x1);
	writeReg(0x01800138, 0xffffffff);
	writeReg(0x01800be8, 0x1);
	writeReg(0x01810be8, 0x1);
	writeReg(0x01820be8, 0x1);
	writeReg(0x01830be8, 0x1);
	writeReg(0x01840be8, 0x1);
	writeReg(0x01850be8, 0x1);
	writeReg(0x01860be8, 0x1);
	writeReg(0x01870be8, 0x1);
	/*val = readl(0x03400188);
	writeReg(0x03400188, val | 0x80000000);
	val = readl(0x0340018c);
	writeReg(0x0340018c, val | 0x1);*/
}

PVRSRV_ERROR sunxiPrePowerState(IMG_HANDLE hSysData,
				PVRSRV_SYS_POWER_STATE eNewPowerState,
				PVRSRV_SYS_POWER_STATE eCurrentPowerState,
				PVRSRV_POWER_FLAGS ePwrFlags)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;
	int ret = 0;

	if (ePwrFlags != PVRSRV_POWER_FLAGS_NONE) {
		dev_info(platform->dev, "%s ePwrFlags:%d current power status:%d %d\n", __func__, ePwrFlags, platform->runtime_power_status, platform->suspend_resume_power_status);
	}

	mutex_lock(&platform->power_lock);

	if (ePwrFlags == PVRSRV_POWER_FLAGS_OSPM_RESUME_REQ && platform->suspend_resume_power_status == GPU_POWER_SUSPEND) {
		dev_info(platform->dev, "regulator_enable\n");
		platform->suspend_resume_power_status = GPU_POWER_RESUME;
		ret = regulator_enable(platform->regula);
		if (ret)
			dev_err(platform->dev, "regulator_enable failed, ret:%d\n", ret);

		dev_info(platform->dev, "clear interrupt\n");
		cleanInterrupt();
		dev_info(platform->dev, "enable irq =%d\n", platform->irq_num);
		enable_irq(platform->irq_num);
		dev_info(platform->dev, "start irq hander\n");
		StartIrqHander();
	}

	if ((eNewPowerState == PVRSRV_DEV_POWER_STATE_ON || ePwrFlags == PVRSRV_POWER_FLAGS_OSPM_RESUME_REQ)
			&& platform->runtime_power_status == GPU_POWER_RUNTIME_OFF) {
		platform->runtime_power_status = GPU_POWER_RUNTIME_ON;

		sunxi_enable_clks_wrap(platform->dev);
		pm_runtime_get_sync(platform->dev);
#if defined(USE_FPGA)
		sunxi_ppu_enable_gpu();
#endif
	}
	mutex_unlock(&platform->power_lock);

	return PVRSRV_OK;

}

PVRSRV_ERROR sunxiPostPowerState(IMG_HANDLE hSysData,
				 PVRSRV_SYS_POWER_STATE eNewPowerState,
				 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
				 PVRSRV_POWER_FLAGS ePwrFlags)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;
	int ret = 0;

	if (ePwrFlags != PVRSRV_POWER_FLAGS_NONE) {
		dev_info(platform->dev, "%s ePwrFlags:%d current power status:%d %d\n", __func__, ePwrFlags, platform->runtime_power_status, platform->suspend_resume_power_status);
	}

	mutex_lock(&platform->power_lock);
	if ((eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF || ePwrFlags == PVRSRV_POWER_FLAGS_OSPM_SUSPEND_REQ)
		&& platform->runtime_power_status == GPU_POWER_RUNTIME_ON) {
#if defined(USE_FPGA)
		sunxi_ppu_disable_gpu();
#endif
		pm_runtime_put_sync(platform->dev);
		sunxi_disable_clks_wrap(platform->dev);

		platform->runtime_power_status = GPU_POWER_RUNTIME_OFF;
	}

	if (ePwrFlags == PVRSRV_POWER_FLAGS_OSPM_SUSPEND_REQ && platform->suspend_resume_power_status == GPU_POWER_RESUME) {
		dev_info(platform->dev, "stop irq hander\n");
		StopIrqHander();
		dev_info(platform->dev, "disable irq =%d\n", platform->irq_num);
		disable_irq(platform->irq_num);
		dev_info(platform->dev, "clear interrupt\n");
		cleanInterrupt();
		dev_info(platform->dev, "regulator_disable\n");
		ret = regulator_disable(platform->regula);
		if (ret)
			dev_err(platform->dev, "regulator_disable failed, ret:%d\n", ret);

		platform->suspend_resume_power_status = GPU_POWER_SUSPEND;
	}

	mutex_unlock(&platform->power_lock);

	return PVRSRV_OK;
}

void sunxiSetFrequency(IMG_UINT32 ui32Frequency)
{
	IMG_UINT32 ui32FrequencyMHZ = ui32Frequency / 1000000;

	if (ui32FrequencyMHZ > 800) {
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_parent) < 0) {
			dev_err(sunxi_data->dev, "%s:clk_set_parent err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_parent, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:clk_set_rate err!", __func__);
		}

		return;
	}

	switch (ui32FrequencyMHZ) {
	case 800:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_800) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 800 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 800 err!", __func__);
		}
		break;
	case 600:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_600) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 600 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 600 err!", __func__);
		}
		break;
	case 400:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_400) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 400 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 400 err!", __func__);
		}
		break;
	case 300:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_300) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 300 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 300 err!", __func__);
		}
		break;
	case 200:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_200) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 200 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 200 err!", __func__);
		}
		break;
	default:
		dev_err(sunxi_data->dev, "%s: Unsupport freq:%u\n", __func__, ui32Frequency);
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_600) < 0) {
			dev_err(sunxi_data->dev, "%s:set default parent 600 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set default 600 err!", __func__);
		}
		break;
	}
}

void sunxiSetVoltage(IMG_UINT32 ui32Volt)
{
	if (sunxi_data->regula) {
		if (regulator_set_voltage(sunxi_data->regula,
					ui32Volt, INT_MAX) != 0) {
			dev_err(sunxi_data->dev,
				"%s:Failed to set gpu power voltage=%d!",
				__func__, ui32Volt);
		}
	}
}

#if defined(SUPPORT_PDVFS) || defined(SUPPORT_LINUX_DVFS)
#if defined(CONFIG_DEVFREQ_THERMAL)

#define FALLBACK_STATIC_TEMPERATURE 65000

/* Temperatures on power over-temp-and-voltage curve (C) */
static const int vt_temperatures[] = { 25, 45, 65, 85, 105 };

/* Voltages on power over-temp-and-voltage curve (mV) */
static const int vt_voltages[] = { 900, 1000, 1130 };

#define POWER_TABLE_NUM_TEMP ARRAY_SIZE(vt_temperatures)
#define POWER_TABLE_NUM_VOLT ARRAY_SIZE(vt_voltages)

static const unsigned int
power_table[POWER_TABLE_NUM_VOLT][POWER_TABLE_NUM_TEMP] = {
	/*   25     45      65      85     105 */
	{ 14540, 35490,  60420, 120690, 230000 },  /*  900 mV */
	{ 21570, 41910,  82380, 159140, 298620 },  /* 1000 mV */
	{ 32320, 72950, 111320, 209290, 382700 },  /* 1130 mV */
};

/** Frequency and Power in Khz and mW respectively */
static const int f_range[] = {253500, 299000, 396500, 455000, 494000, 598000};
static const IMG_UINT32 max_dynamic_power[] = {612, 722, 957, 1100, 1194, 1445};

static u32 interpolate(int value, const int *x, const unsigned int *y, int len)
{
	u64 tmp64;
	u32 dx;
	u32 dy;
	int i, ret;

	if (value <= x[0])
		return y[0];
	if (value >= x[len - 1])
		return y[len - 1];

	for (i = 1; i < len - 1; i++) {
		/* If value is identical, no need to interpolate */
		if (value == x[i])
			return y[i];
		if (value < x[i])
			break;
	}

	/* Linear interpolation between the two (x,y) points */
	dy = y[i] - y[i - 1];
	dx = x[i] - x[i - 1];

	tmp64 = value - x[i - 1];
	tmp64 *= dy;
	do_div(tmp64, dx);
	ret = y[i - 1] + tmp64;

	return ret;
}

unsigned long sunxi_get_static_power(unsigned long voltage)
{
	struct thermal_zone_device *tz = sunxi_data->tz;
	unsigned long power;
	int temperature = FALLBACK_STATIC_TEMPERATURE;
	int low_idx = 0, high_idx = POWER_TABLE_NUM_VOLT - 1;
	int i;

	if (!tz)
		return 0;

	if (tz->ops->get_temp(tz, &temperature))
		dev_err(sunxi_data->dev, "Failed to read temperature\n");
	do_div(temperature, 1000);

	for (i = 0; i < POWER_TABLE_NUM_VOLT; i++) {
		if (voltage <= vt_voltages[POWER_TABLE_NUM_VOLT - 1 - i])
			high_idx = POWER_TABLE_NUM_VOLT - 1 - i;

		if (voltage >= vt_voltages[i])
			low_idx = i;
	}

	if (low_idx == high_idx) {
		power = interpolate(temperature,
				    vt_temperatures,
				    &power_table[low_idx][0],
				    POWER_TABLE_NUM_TEMP);
	} else {
		unsigned long dvt =
				vt_voltages[high_idx] - vt_voltages[low_idx];
		unsigned long power1, power2;

		power1 = interpolate(temperature,
				     vt_temperatures,
				     &power_table[high_idx][0],
				     POWER_TABLE_NUM_TEMP);

		power2 = interpolate(temperature,
				     vt_temperatures,
				     &power_table[low_idx][0],
				     POWER_TABLE_NUM_TEMP);

		power = (power1 - power2) * (voltage - vt_voltages[low_idx]);
		do_div(power, dvt);
		power += power2;
	}

	/* convert to mw */
	do_div(power, 1000);

	dev_err(sunxi_data->dev, "%s:%lu at Temperature %d\n", __func__,
			  power, temperature);
	return power;
}

unsigned long sunxi_get_dynamic_power(unsigned long freq, unsigned long voltage)
{
	#define NUM_RANGE  ARRAY_SIZE(f_range)
	/** Frequency and Power in Khz and mW respectively */
	IMG_INT32 i, low_idx = 0, high_idx = NUM_RANGE - 1;
	IMG_UINT32 power;

	for (i = 0; i < NUM_RANGE; i++) {
		if (freq <= f_range[NUM_RANGE - 1 - i])
			high_idx = NUM_RANGE - 1 - i;

		if (freq >= f_range[i])
			low_idx = i;
	}

	if (low_idx == high_idx) {
		power = max_dynamic_power[low_idx];
	} else {
		IMG_UINT32 f_interval = f_range[high_idx] - f_range[low_idx];
		IMG_UINT32 p_interval = max_dynamic_power[high_idx] -
				max_dynamic_power[low_idx];

		power = p_interval * (freq - f_range[low_idx]);
		do_div(power, f_interval);
		power += max_dynamic_power[low_idx];
	}

	power = (IMG_UINT32)div_u64((IMG_UINT64)power * voltage * voltage,
				    1000000UL);

	dev_err(sunxi_data->dev, "%s:%u at voltage %lu\n", __func__,
			  power, voltage);
	return power;
	#undef NUM_RANGE
}
#endif //~THERMAL
#endif //~DVFS


#if defined(SUPPORT_LINUX_DVFS)
#define DVFS_EFUSE_OFF         (0x4c)

static int sunxi_match_vf_table(struct device *dev, u32 combi, u32 *value)
{
	struct device_node *np = NULL;
	int nsels, ret, i;
	u32 tmp;

	np = of_find_node_by_name(NULL, "vf_mapping_table");
	if (!np) {
		dev_err(dev, "Unable to find node 'note'\n");
		return -EINVAL;
	}

	if (!of_get_property(np, "table", &nsels)) {
		dev_err(dev, "can NOT get 'table' property under node of 'vf_mapping_table'\n");
		return -EINVAL;
	}

	nsels /= sizeof(u32);
	if (!nsels) {
		dev_err(dev, "invalid table property size\n");
		return -EINVAL;
	}

	for (i = 0; i < nsels / 2; i++) {
		ret = of_property_read_u32_index(np, "table", i * 2, &tmp);
		if (ret) {
			dev_err(dev, "could not retrieve table property: %d\n", ret);
			return ret;
		}

		if (tmp == combi) {
			ret = of_property_read_u32_index(np, "table", i * 2 + 1, &tmp);
			if (ret) {
				dev_err(dev, "could not retrieve table property: %d\n", ret);
				return ret;
			}

			*value = tmp;
			break;
		} else
			continue;
	}

	if (i == nsels/2) {
		dev_err(dev, "%s %d, could not match vf table, i:%d", __func__, __LINE__, i);
		return -1;
	}

	return 0;
}

static int sunxi_get_opp(struct device *dev, struct sunxi_platform *sunxi_data)
{
	int match = 0;
	unsigned int dvfs_code;
	unsigned int dvfs_value;

	int opp_count = 0;
	int index;
	bool order_reverse = false;
	struct device_node *node;
	struct device_node *opp_node = of_parse_phandle(dev->of_node,
							"operating-points-v2", 0);
	if (!opp_node) {
		dev_err(dev, "of_parse_phandle failed");
		return -1;
	}

	sunxi_get_soc_dvfs(&dvfs_code);
	dev_info(dev, "get dvfs_code:0x%x\n", dvfs_code);
	match = sunxi_match_vf_table(dev, dvfs_code, &dvfs_value);
	dev_info(dev, "get dvfs_value:0x%04x\n", dvfs_value);

	index = 0;
	for_each_available_child_of_node(opp_node, node) {
		int err;
		u64 opp_freq;
		u32 opp_uvolt;
		err = of_property_read_u64(node, "opp-hz", &opp_freq);
		char opp_microvolt_name[40] = { 0 };

		if (err) {
			dev_err(dev, "Failed to read opp-hz property with error %d\n", err);
			continue;
		}

		if (match < 0)
			sprintf(opp_microvolt_name, "opp-microvolt-vfdefault");
		else
			sprintf(opp_microvolt_name, "opp-microvolt-vf%04x", dvfs_value);
		err = of_property_read_u32(node, opp_microvolt_name, &opp_uvolt);
		if (err) {
			dev_err(dev, "Failed to read %s property with error %d\n", opp_microvolt_name, err);
			continue;
		}

		sunxi_data->asOPPTable[index].ui32Freq = (IMG_UINT32)opp_freq;
		sunxi_data->asOPPTable[index].ui32Volt = (IMG_UINT32)opp_uvolt;
		dev_info(dev, "get opp:(%u hz, %u uv)\n", sunxi_data->asOPPTable[index].ui32Freq, sunxi_data->asOPPTable[index].ui32Volt);

		index++;
	}

	opp_count = index;
	dev_info(dev, "opp count:%d\n", opp_count);
	if (opp_count > OPP_MAX_NUM) {
		dev_err(dev, "opp count is too big, has exceed OPP_MAX_NUM:%d\n", OPP_MAX_NUM);
		return -1;
	}
	sunxi_data->ui32OPPTableSize = opp_count;

	if (sunxi_data->asOPPTable[0].ui32Freq > sunxi_data->asOPPTable[1].ui32Freq)
		order_reverse = true;
	sunxi_data->clk_rate = order_reverse ? sunxi_data->asOPPTable[0].ui32Freq
					: sunxi_data->asOPPTable[opp_count - 1].ui32Freq;
	dev_info(dev, "set default clk_rate to max opp freq:%u\n", sunxi_data->clk_rate);

	return 0;
}
#endif

extern void DIPrintf(const OSDI_IMPL_ENTRY *psEntry, const IMG_CHAR *pszFmt, ...);
static int _DebugDumpOppInfoDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	int i = 0;
	struct sunxi_platform *sunxi_data;

	sunxi_data = (struct sunxi_platform *)psEntry->pvPrivData;
	if (!sunxi_data) {
		DIPrintf(psEntry, "Null sunxi_data\n");
		return -1;
	}

	if (!psEntry) {
		DIPrintf(psEntry, "Null psEntry\n");
		return -1;
	}

	for (i = 0; i < sunxi_data->ui32OPPTableSize; i++) {
		DIPrintf(psEntry, "opp%d is %luHz---%lumV\n",
				  i, sunxi_data->asOPPTable[i].ui32Freq,
				  sunxi_data->asOPPTable[i].ui32Volt / 1000);
	}

	return 0;
}

static int sunxi_parse_dts(struct device *dev, struct sunxi_platform *sunxi_data)
{
#ifdef CONFIG_OF
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *reg_res;

	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (reg_res == NULL) {
		dev_err(dev, "failed to get register data from device tree");
		return -1;
	}
	sunxi_data->reg_base = reg_res->start;
	sunxi_data->reg_size = reg_res->end - reg_res->start + 1;
	sunxi_data->dev = dev;

	sunxi_data->irq_num = platform_get_irq_byname(pdev, "IRQGPU");
	if (sunxi_data->irq_num < 0) {
		dev_err(dev, "failed to get irq number from device tree");
		return -1;
	}

#if defined(SUNXI_DVFS_CTRL_ENABLE)
	sunxi_data->dvfs_irq_num = platform_get_irq_byname(pdev, "IRQGPUDVFS");
	if (sunxi_data->dvfs_irq_num < 0) {
		dev_err(dev, "failed to get dvfs irq number from device tree");
		return -1;
	}
#endif
	ret = of_property_read_u32(dev->of_node, "clk_rate", &sunxi_data->clk_rate);
	if (ret < 0) {
		dev_info(dev, "warning: default clk_rate is NOT set in DTS, set it to default:%d\n", DEFAULT_GPU_RATE);
		sunxi_data->clk_rate = DEFAULT_GPU_RATE;
	}
	dev_info(dev, "%s clk_rate:%d\n", __func__, sunxi_data->clk_rate);

	ret = sunxi_get_clks_wrap(dev);
	if (ret < 0) {
		dev_err(dev, "sunxi_get_clks_wrap failed");
		return ret;
	}

	sunxi_data->regula = regulator_get_optional(dev, "gpu");
	if (!sunxi_data->regula) {
		dev_err(dev, "regulator_get_optional for gpu-supply failed\n");
	}
	dev_info(dev, "succeed to get gpu regulator, default volt value:%u",
				regulator_get_voltage(sunxi_data->regula));
#endif /* CONFIG_OF */

#if defined(SUPPORT_LINUX_DVFS)
	sunxi_get_opp(dev, sunxi_data);
#endif
	dev_info(dev, "%s finished\n", __func__);
	return 0;
}

extern PVRSRV_ERROR DICreateGroup(const IMG_CHAR *pszName,
				  DI_GROUP *psParent,
				  DI_GROUP **ppsGroup);
extern void DIDestroyGroup(DI_GROUP *psGroup);
extern PVRSRV_ERROR DICreateEntry(const IMG_CHAR *pszName,
				  DI_GROUP *psGroup,
				  const DI_ITERATOR_CB *psIterCb,
				  void *pvPriv,
				  DI_ENTRY_TYPE eType,
				  DI_ENTRY **ppsEntry);
extern void DIDestroyEntry(DI_ENTRY *psEntry);

int sunxi_platform_init(struct device *dev)
{
	PVRSRV_ERROR error;
	DI_ITERATOR_CB sIterator;
#if defined(SUNXI_DVFS_CTRL_ENABLE)
	struct sunxi_dvfs_init_params dvfs_init_para;
#endif

	dev_info(dev, "%s start\n", __func__);
	sunxi_data = (struct sunxi_platform *)kzalloc(sizeof(struct sunxi_platform), GFP_KERNEL);
	if (!sunxi_data) {
		dev_err(dev, "failed to get kzalloc sunxi_platform");
		return -1;
	}
	dev->platform_data = sunxi_data;
	mutex_init(&sunxi_data->power_lock);

	if (sunxi_parse_dts(dev, sunxi_data) < 0)
		return -1;

	sunxi_enable_clks_wrap(dev);
	sunxi_data->runtime_power_status = GPU_POWER_RUNTIME_ON;

	sunxi_set_device_clk_rate(sunxi_data->clk_rate);
	dev_info(dev, "sunxi_set_device_clk_rate:%d\n", sunxi_data->clk_rate);

#if defined(SUNXI_DVFS_CTRL_ENABLE)
	dvfs_init_para.reg_base = sunxi_data->reg_base + SUNXI_DVFS_CTRL_OFFSET;
	dvfs_init_para.clk_rate = sunxi_data->clk_rate;
	dvfs_init_para.irq_no = sunxi_data->dvfs_irq_num;
	if (sunxi_dvfs_ctrl_init(dev, &dvfs_init_para) < 0) {
		dev_err(dev, "sunxi_dvfs_ctrl_init failed\n");
		return -1;
	}
	dev_info(dev, "sunxi_dvfs_ctrl_init");

	if (sunxi_dvfs_ctrl_get_opp_table(&sunxi_data->asOPPTable,
					&sunxi_data->ui32OPPTableSize) < 0) {
		dev_err(dev, "No any valid opp got!\n");
		return -1;
	}
#endif
	if (regulator_enable(sunxi_data->regula) < 0)
		dev_err(dev, "regulator_enable in probe failed\n");
	sunxi_data->suspend_resume_power_status = GPU_POWER_RESUME;

#if defined(CONFIG_DEVFREQ_THERMAL)
	sunxi_data->tz = thermal_zone_get_zone_by_name(GPU_THERMAL_ZONE);
	if (IS_ERR(sunxi_data->tz)) {
		dev_err(dev, "Failed to get gpu thermal zone\n");
	}
#endif

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

#if defined(USE_FPGA)
	sunxi_ppu_init_gpu();
	sunxi_ppu_enable_gpu();
#endif

	/*create sunxi debugfs or proc_fs*/
	error = DICreateGroup("sunxi_gpu", NULL, &sunxi_data->psGroup);
	if (unlikely(error != PVRSRV_OK)) {
		dev_info(dev, "%s DICreateGroup failed\n", __func__);
		return -1;
	}

	memset(&sIterator, 0, sizeof(sIterator));
	sIterator.pfnShow = _DebugDumpOppInfoDIShow;
	error = DICreateEntry("gpu_opp_ops", sunxi_data->psGroup, &sIterator,
					sunxi_data, DI_ENTRY_TYPE_GENERIC,
					&sunxi_data->psOpp);
	if (unlikely(error != PVRSRV_OK)) {
		dev_info(dev, "%s DICreateEntry for gpu_opp_ops failed\n", __func__);
		return -1;
	}

	dev_info(dev, "%s end\n", __func__);
	return 0;
}

void sunxi_platform_term(void)
{
	DIDestroyEntry(sunxi_data->psOpp);
	DIDestroyGroup(sunxi_data->psGroup);
	sunxi_disable_clks_wrap(sunxi_data->dev);
	pm_runtime_disable(sunxi_data->dev);
	kfree(sunxi_data);

	sunxi_data = NULL;
}
