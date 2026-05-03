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
#ifndef _SUNXI_HRC_DEFINE_H_
#define _SUNXI_HRC_DEFINE_H_

#include <media/v4l2-dv-timings.h>
#include <linux/types.h>

/* ----------- HRC --------------- */

#define HRC_DRV_NAME "sunxi-hrc"

enum hrc_data_src {
	HRC_DATA_SRC_HDMI_RX = 0,
	HRC_DATA_SRC_DDR,
	HRC_DATA_SRC_MAX,
};

enum hrc_field_mode {
	HRC_FIELD_MODE_FRAME = 0,
	HRC_FIELD_MODE_FIELD,
	HRC_FIELD_MODE_MAX,
};

enum hrc_field_order {
	HRC_FIELD_ORDER_BOTTOM = 0,
	HRC_FIELD_ORDER_TOP,
	HRC_FIELD_ORDER_MAX,
};

enum hrc_fmt {
	HRC_FMT_RGB = 0,
	HRC_FMT_YUV444,
	HRC_FMT_YUV422,
	HRC_FMT_YUV420,
	HRC_FMT_YUV444_1PLANE,
	HRC_FMT_MAX,
};

enum hrc_depth {
	HRC_DEPTH_8 = 0,
	HRC_DEPTH_10,
	HRC_DEPTH_12,
	HRC_DEPTH_16,
	HRC_DEPTH_MAX,
};

enum hrc_color_space {
	HRC_CS_BT601 = 0,
	HRC_CS_BT709,
	HRC_CS_BT2020,
	HRC_CS_MAX,
};

enum hrc_quantization {
	HRC_QUANTIZATION_FULL = 0,
	HRC_QUANTIZATION_LIMIT,
	HRC_QUANTIZATION_MAX,
};

struct hrc_input_param {
	enum hrc_data_src     data_src;
	enum hrc_field_mode   field_mode;
	u8                    field_inverse;
	enum hrc_field_order  field_order;
	u32                   width;
	u32                   height;
	enum hrc_fmt          format;
	enum hrc_depth        depth;
	enum hrc_color_space  cs;
	enum hrc_quantization quantization;
	u32                   ddr_addr[3];
};

struct hrc_output_param {
	u32                   width;
	u32                   height;
	enum hrc_fmt          format;
	enum hrc_color_space  cs;
	enum hrc_quantization quantization;
	u32                   out_addr[3][2];
};

enum hrc_debug_flag {
	DEBUG_INPUT_PARAM = BIT(0),
	DEBUG_OUTPUT_PARAM = BIT(1),
};

/* ----------- HDMIRX <-> HRC --------------- */
#define HDMI_MAX_INPUT_PORT 4

enum hrc_source_id {
	hrc_source_Dummy,
	hrc_source_VideoDec, //DTV/USB/OTT
	hrc_source_Image,
	hrc_source_HDMI_1,
	hrc_source_HDMI_2,
	hrc_source_HDMI_3,
	hrc_source_HDMI_4,
	hrc_source_CVBS_1,
	hrc_source_CVBS_2,
	hrc_source_CVBS_3,
	hrc_source_ATV,
	hrc_source_Max,
};

struct hrc_from_hdmirx_signal {
	enum hrc_source_id    source_id;
	enum hrc_fmt          format;
	enum hrc_depth        depth;
	enum hrc_color_space  csc;
	enum hrc_quantization quantization;
	struct v4l2_bt_timings timings;
};

struct hrc_from_hdmirx_5V {
	u8 PortID;
	bool is5V;
};

enum notify_msg_type {
	SIGNAL_CHANGE = 0x0,
	HPD_CHANGE,
	AUDIO_FSRATE_CHANGE,
};

/* ----------- HRC <-> V4L2 --------------- */

enum hrc_event_change {
	/* V4L2_EVENT_SRC_CH_RESOLUTION = 1, */
	V4L2_EVENT_SRC_CH_HPD_IN = 2,
	V4L2_EVENT_SRC_CH_HPD_OUT,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_22K,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_24K,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_32K,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_44K1,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_48K,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_88K,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_96K,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_176K4,
	V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_192K,
};

#endif  /* _SUNXI_HRC_DEFINE_H_ */
