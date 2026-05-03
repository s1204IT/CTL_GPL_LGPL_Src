/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>

#include "amp_ts_core.h"

static uint64_t arm_arch_counter_read(const amp_counter_t *counter)
{
	uint32_t low, high;
	unsigned long high_addr, low_addr;

	low_addr = ((unsigned long)counter->reg_base_addr) + 0x8;
	high_addr = low_addr + 4;

	do {
		high = readl((void __iomem *)high_addr);
		low = readl((void __iomem *)low_addr);
	} while (high != readl((void __iomem *)high_addr));

	return ((uint64_t)high) << 32 | low;
}

static int arm_arch_counter_init(amp_counter_t *counter)
{
	return 0;
}

amp_counter_ops_t g_arm_arch_counter_ops = {
	.init = arm_arch_counter_init,
	.read_counter = arm_arch_counter_read,
};