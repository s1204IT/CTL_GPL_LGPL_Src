/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright(c) 2023 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __AMP_TIMESTAMP_CORE_H__
#define __AMP_TIMESTAMP_CORE_H__

#include <linux/types.h>

struct amp_counter_ops;

typedef struct amp_counter {
	uint32_t type;
	struct device_node *of_node;
	struct resource *io_res;
	uint32_t count_freq;
	struct amp_counter_ops *ops;
	void __iomem *reg_base_addr;
	void *priv;
} amp_counter_t;

typedef struct amp_counter_ops {
	int (*init)(amp_counter_t *);
	uint64_t (*read_counter)(const amp_counter_t *);
} amp_counter_ops_t;

extern amp_counter_ops_t g_arm_arch_counter_ops;
extern amp_counter_ops_t g_thead_riscv_arch_counter_ops;

#endif /* __AMP_TIMESTAMP_CORE_H__ */
