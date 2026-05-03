// SPDX-License-Identifier: GPL-2.0-or-later
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
#include <linux/kernel.h>
#include "sunxi_hrc_platform.h"

/*
 * Note:
 *     The current desc table indicates the common capabilities of hardware and software.
 *     It is possible that the hardware supports it, but the software does not.
 **/

struct hrc_hw_desc hrc110 = {
	.name = "hrc110",
	.version = 0x110,
	.csc = HRC_CSC_RGB_TO_YUV,
	.scaler = HRC_SCALER_HOR,
	.sample = HRC_SAMPLE_VER,

	/* -------------- SIZE -----------------*/
	.min_width = 16,
	.min_height = 16,
	.max_width = 4096,
	.max_height = 4096,
	.align_width = 1,
	.align_height = 1,

	/* -------------- FORMAT -----------------*/
	.fmt_size = 4,
	.fmt = {
		{
			.input_format = HRC_FMT_RGB,
			.output_format_size = 5,
			.output_format = {
				HRC_FMT_RGB,
				HRC_FMT_YUV444,
				HRC_FMT_YUV444_1PLANE,
				HRC_FMT_YUV422,
				HRC_FMT_YUV420,
			},
		}, {
			.input_format = HRC_FMT_YUV444,
			.output_format_size = 4,
			.output_format = {
				HRC_FMT_YUV444,
				HRC_FMT_YUV444_1PLANE,
				HRC_FMT_YUV422,
				HRC_FMT_YUV420,
			},
		}, {
			.input_format = HRC_FMT_YUV422,
			.output_format_size = 2,
			.output_format = {
				HRC_FMT_YUV422,
				HRC_FMT_YUV420,
			},
		}, {
			.input_format = HRC_FMT_YUV420,
			.output_format_size = 1,
			.output_format = {
				HRC_FMT_YUV420,
			},
		},
	},

	/* -------------- DEPTH -----------------*/
	.depth_size = 3,
	.depth = {
		{
			.input_depth = HRC_DEPTH_8,
			.output_depth_size = 1,
			.output_depth = {
				HRC_DEPTH_8,
			},
		},
		{
			.input_depth = HRC_DEPTH_10,
			.output_depth_size = 1,
			.output_depth = {
				HRC_DEPTH_10,
			},
		},
		{
			.input_depth = HRC_DEPTH_12,
			.output_depth_size = 1,
			.output_depth = {
				HRC_DEPTH_12,
			},
		},
	},

	/* -------------- CS -----------------*/
	.cs_size = 3,
	.cs = {
		{
			.input_cs = HRC_CS_BT601,
			.output_cs_size = 1,
			.output_cs = {
				HRC_CS_BT601,
			},
		},
		{
			.input_cs = HRC_CS_BT709,
			.output_cs_size = 1,
			.output_cs = {
				HRC_CS_BT709,
			},
		},
		{
			.input_cs = HRC_CS_BT2020,
			.output_cs_size = 1,
			.output_cs = {
				HRC_CS_BT2020,
			},
		},
	},

	/* -------------- QUAN -----------------*/
	.quan_size = 2,
	.quan = {
		{
			.input_quan = HRC_QUANTIZATION_FULL,
			.output_quan_size = 2,
			.output_quan = {
				HRC_QUANTIZATION_FULL,
				HRC_QUANTIZATION_LIMIT,
			},
		},
		{
			.input_quan = HRC_QUANTIZATION_LIMIT,
			.output_quan_size = 1,
			.output_quan = {
				HRC_QUANTIZATION_LIMIT,
			},
		},
	},

	/* -------------- REG_RANGE -----------------*/
	.reg_range = {
		{ 0x0,   0x100 },
		{ 0x100, 0x180 },
		{ 0x200, 0x280 },
		{ 0x400, 0x480 },
		{ 0x500, 0x580 },
	},
};

struct hrc_hw_desc *hrc_descs[] = {
	&hrc110
};

struct hrc_hw_desc *get_hrc_desc(u32 version)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_descs); i++) {
		if (version == hrc_descs[i]->version)
			return hrc_descs[i];
	}
	return NULL;
}
