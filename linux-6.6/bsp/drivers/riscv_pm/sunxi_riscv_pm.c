/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/suspend.h>
#include <asm/sbi.h>
#include <sunxi-log.h>

#define NormalMode              0
#define LightSleepMode          1
#define DeepSleepMode           2
#define CpuHotplugDeepSleepMode 3

#define SET_HIB_POWERON(para)  ((1 << 30) | (para))
#define SET_ULTRA_POWERON(para)  ((2 << 30) | (para))

static int sunxi_riscv_enter(suspend_state_t state)
{
	sbi_andes_set_suspend_mode(DeepSleepMode);
	sbi_andes_enter_suspend_mode(true, 0);

	return 0;
}

static struct platform_suspend_ops sunxi_riscv_pm = {
	.valid = suspend_valid_only_mem,
	.enter = sunxi_riscv_enter,
};

static void sunxi_hib_and_ultra_poweron_source_send(void)
{
	struct device_node *node;
	u32 poweron_source;

	node = of_find_node_by_path("/poweron-source");
	if (node) {
		if (!of_property_read_u32 (node, "hib_poweron_source", &poweron_source)) {
			sunxi_info(NULL, "hib poweron_source is 0x%x\n", poweron_source);
			sbi_andes_set_wakeup_source(SET_HIB_POWERON(poweron_source), 1);
		}
		if (!of_property_read_u32 (node, "ultra_poweron_source", &poweron_source)) {
			sunxi_info(NULL, "hib ultra_poweron_source is 0x%x\n", poweron_source);
			sbi_andes_set_wakeup_source(SET_ULTRA_POWERON(poweron_source), 1);
		}
	}
}

static int sunxi_hib_and_ultra_notify_sys(struct notifier_block *this,
			   unsigned long mode, void *cmd)
{
	if (mode == SYS_POWER_OFF) {
		sunxi_hib_and_ultra_poweron_source_send();
	}
	return NOTIFY_DONE;
}

static struct notifier_block sunxi_hib_and_ultra_nb = {
	.notifier_call = sunxi_hib_and_ultra_notify_sys,
};

static int __init sunxi_riscv_pm_register(void)
{
	register_reboot_notifier(&sunxi_hib_and_ultra_nb);
	suspend_set_ops(&sunxi_riscv_pm);
	return 0;
}

late_initcall_sync(sunxi_riscv_pm_register);
void sunxi_riscv_pm_unregister(void)
{
	unregister_reboot_notifier(&sunxi_hib_and_ultra_nb);
}

module_exit(sunxi_riscv_pm_unregister);
