/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs HRC Driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _SUNXI_HRC_HARDWARE_H_
#define _SUNXI_HRC_HARDWARE_H_

#include <linux/types.h>
#include <linux/platform_device.h>
#include "sunxi_hrc_define.h"

enum hrc_apply_dirty {
	HRC_APPLY_NO_DIRTY    = BIT(0),
	HRC_APPLY_PARAM_DIRTY = BIT(1),
	HRC_APPLY_ADDR_DIRTY  = BIT(2),
	HRC_APPLY_ALL_DIRTY   = BIT(3),
};

struct hrc_apply_data {
	struct hrc_input_param input;
	struct hrc_output_param output;
	enum hrc_apply_dirty dirty;
};

struct hrc_size_range {
	u32 min_width;
	u32 min_height;
	u32 max_width;
	u32 max_height;
	u32 align_width;
	u32 align_height;
};

struct hrc_hw_create_info {
	struct platform_device *pdev;
	u32 version;
};

struct hrc_hw_handle {
	struct hrc_hw_create_info info;
	struct hrc_private_data *private;
};

enum hrc_irq {
	HRC_IRQ_FRAME_VSYNC = BIT(0),
	HRC_IRQ_CFG_FINISH  = BIT(1),
	HRC_IRQ_WB_FINISH   = BIT(2),
	HRC_IRQ_OVERFLOW    = BIT(3),
	HRC_IRQ_TIMEOUT     = BIT(4),
	HRC_IRQ_UNUSUAL     = BIT(5),
	HRC_IRQ_ALL         = BIT(6),
};

int sunxi_hrc_hardware_enable(struct hrc_hw_handle *hrc_hdl);
int sunxi_hrc_hardware_disable(struct hrc_hw_handle *hrc_hdl);
int sunxi_hrc_hardware_get_size_range(struct hrc_hw_handle *hrc_hdl, struct hrc_size_range *size);
int sunxi_hrc_hardware_get_format(struct hrc_hw_handle *hrc_hdl,
				  enum hrc_fmt input_format,
				  enum hrc_fmt *output_format);
int sunxi_hrc_hardware_get_color_space(struct hrc_hw_handle *hrc_hdl,
				       enum hrc_color_space input_cs,
				       enum hrc_color_space *output_cs);
int sunxi_hrc_hardware_get_quantization(struct hrc_hw_handle *hrc_hdl,
					enum hrc_quantization input_quan,
					enum hrc_quantization *output_quan);
int sunxi_hrc_hardware_check(struct hrc_hw_handle *hrc_hdl, struct hrc_apply_data *data);
int sunxi_hrc_hardware_apply(struct hrc_hw_handle *hrc_hdl, struct hrc_apply_data *data);
int sunxi_hrc_hardware_commit(struct hrc_hw_handle *hrc_hdl);
int sunxi_hrc_hardware_get_irq_state(struct hrc_hw_handle *hrc_hdl, u32 *state);
int sunxi_hrc_hardware_clr_irq_state(struct hrc_hw_handle *hrc_hdl, u32 state);
int sunxi_hrc_hardware_dump(struct hrc_hw_handle *hrc_hdl, char *buf, int n);
struct hrc_hw_handle *sunxi_hrc_handle_create(struct hrc_hw_create_info *info);
void sunxi_hrc_handle_destory(struct hrc_hw_handle *hrc_hdl);

u32 hrc_read(struct hrc_hw_handle *hrc_hdl, u32 reg);
void hrc_write(struct hrc_hw_handle *hrc_hdl, u32 reg, u32 val);
u32 hrc_read_mask(struct hrc_hw_handle *hrc_hdl, u32 reg, u32 mask);
void hrc_write_mask(struct hrc_hw_handle *hrc_hdl, u32 reg, u32 mask, u32 val);
int hrc_reg_dump(struct hrc_hw_handle *hrc_hdl, char *buf, int size);

#endif  /* _SUNXI_HRC_HARDWARE_H_ */
