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
#ifndef _SUNXI_HRC_PLATFORM_H_
#define _SUNXI_HRC_PLATFORM_H_

#include "sunxi_hrc_define.h"

struct hrc_hw_fmt {
	enum hrc_fmt input_format;
	enum hrc_fmt output_format_size;
	enum hrc_fmt output_format[HRC_FMT_MAX];
};

struct hrc_hw_depth {
	enum hrc_depth input_depth;
	enum hrc_depth output_depth_size;
	enum hrc_depth output_depth[HRC_DEPTH_MAX];
};

struct hrc_hw_color_space {
	enum hrc_color_space input_cs;
	enum hrc_color_space output_cs_size;
	enum hrc_color_space output_cs[HRC_CS_MAX];
};

struct hrc_hw_quantization {
	enum hrc_quantization input_quan;
	enum hrc_quantization output_quan_size;
	enum hrc_quantization output_quan[HRC_QUANTIZATION_MAX];
};

enum hrc_hw_csc_flag {
	HRC_CSC_NOT_SUPPORTED = BIT(0),
	HRC_CSC_RGB_TO_YUV    = BIT(1),
	HRC_CSC_YUV_TO_RGB    = BIT(2),
};

enum hrc_hw_scaler_flag {
	HRC_SCALER_NOT_SUPPORTED = BIT(0),
	HRC_SCALER_HOR           = BIT(1),
	HRC_SCALER_VER           = BIT(2),
};

enum hrc_hw_sample_flag {
	HRC_SAMPLE_NOT_SUPPORTED = BIT(0),
	HRC_SAMPLE_HOR           = BIT(1),
	HRC_SAMPLE_VER           = BIT(2),
};

#define HRC_HW_REG_RANGE_MAX_SIZE	32
struct hrc_hw_reg_range {
	u32 start;
	u32 end;
};

struct hrc_hw_desc {
	char name[32];
	u32 version;
	u32 csc;
	u32 scaler;
	u32 sample;
	u32 min_width;
	u32 min_height;
	u32 max_width;
	u32 max_height;
	u32 align_width;
	u32 align_height;
	u32 fmt_size;
	struct hrc_hw_fmt fmt[HRC_FMT_MAX];
	u32 depth_size;
	struct hrc_hw_depth depth[HRC_DEPTH_MAX];
	u32 cs_size;
	struct hrc_hw_color_space cs[HRC_CS_MAX];
	u32 quan_size;
	struct hrc_hw_quantization quan[HRC_CS_MAX];
	struct hrc_hw_reg_range reg_range[HRC_HW_REG_RANGE_MAX_SIZE];
};

struct hrc_hw_desc *get_hrc_desc(u32 version);

#endif  /* _SUNXI_HRC_PLATFORM_H_ */
