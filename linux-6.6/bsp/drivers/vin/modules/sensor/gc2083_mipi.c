/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for GC2083 Raw cameras.
 *
 * Copyright (c) 2023 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("myf");
MODULE_DESCRIPTION("A low-level driver for GC2083 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR  0x2083

/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0x03f0
#define ID_REG_LOW		0x03f1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)
#define SENSOR_FRAME_RATE 20

/*
 * The GC2083 i2c address
 */
#define I2C_ADDR 0x6e

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc2083_mipi"
#define SENSOR_NAME_2 "gc2083_mipi_2"

#if IS_ENABLED(CONFIG_SENSOR_GC2083_ONE_LANE_MIPI) /* one lane */
#define GC2083_1LANE_1920X1080_10FPS
#define GC2083_1LANE_1920X1080_15FPS
#define GC2083_1LANE_1920X1080_20FPS
#define GC2083_1LANE_1920X1080_25FPS
#else /* two lane */
#define GC2083_1920X1088_10FPS
#define GC2083_1920X1088_15FPS
#define GC2083_1920X1088_20FPS
#define GC2083_1920X1088_25FPS
//#define GC2083_1920X360_120FPS
#endif

#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
#define A_I2C_ADDR 0x6e
#define B_I2C_ADDR 0x7e
#define SWITCH_SENSOR_A 0
#define SWITCH_SENSOR_B 1
static struct sensor_switch_cfg_t sensor_switch_cfg;
static DEFINE_MUTEX(device_mutex);
#endif
/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

/*
 * window_size=1920*1080 mipi@2lane
 * mclk=24mhz, mipi_clk=594Mbps
 * pixel_line_total=2640, line_frame_total=1125
 * row_time=35.55us, frame_rate=25fps
 */

#ifdef GC2083_1LANE_1920X1080_10FPS
static struct regval_list sensor_1080p10_1lane_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0xae},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x07},
	{0x0d06, 0x29},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	/* vts */
	{0x0d41, 0x0a},
	{0x0d42, 0x8c},
	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x05},
	{0x008d, 0xa8},
	{0x007a, 0x58},//global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},
	{0x0102, 0xa9},
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},
	{0x0428, 0x86},
	{0x0429, 0x86},
	{0x042a, 0x86},
	{0x042b, 0x68},
	{0x042c, 0x68},
	{0x042d, 0x68},
	{0x042e, 0x68},
	{0x042f, 0x68},
	{0x0430, 0x4f},
	{0x0431, 0x68},
	{0x0432, 0x67},
	{0x0433, 0x66},
	{0x0434, 0x66},
	{0x0435, 0x66},
	{0x0436, 0x66},
	{0x0437, 0x66},
	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x38},
	// {0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1LANE_1920X1080_15FPS
static struct regval_list sensor_1080p15_1lane_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0xae},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x07},
	{0x0d06, 0x29},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	/* vts */
	{0x0d41, 0x07},
	{0x0d42, 0x08},
	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x05},
	{0x008d, 0xa8},
	{0x007a, 0x58},//global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},
	{0x0102, 0xa9},
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},
	{0x0428, 0x86},
	{0x0429, 0x86},
	{0x042a, 0x86},
	{0x042b, 0x68},
	{0x042c, 0x68},
	{0x042d, 0x68},
	{0x042e, 0x68},
	{0x042f, 0x68},
	{0x0430, 0x4f},
	{0x0431, 0x68},
	{0x0432, 0x67},
	{0x0433, 0x66},
	{0x0434, 0x66},
	{0x0435, 0x66},
	{0x0436, 0x66},
	{0x0437, 0x66},
	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x38},
	// {0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1LANE_1920X1080_20FPS
static struct regval_list sensor_1080p20_1lane_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0xae},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x07},
	{0x0d06, 0x29},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	/* vts */
	{0x0d41, 0x05},
	{0x0d42, 0x46},
	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x05},
	{0x008d, 0xa8},
	{0x007a, 0x58},//global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},
	{0x0102, 0xa9},
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},
	{0x0428, 0x86},
	{0x0429, 0x86},
	{0x042a, 0x86},
	{0x042b, 0x68},
	{0x042c, 0x68},
	{0x042d, 0x68},
	{0x042e, 0x68},
	{0x042f, 0x68},
	{0x0430, 0x4f},
	{0x0431, 0x68},
	{0x0432, 0x67},
	{0x0433, 0x66},
	{0x0434, 0x66},
	{0x0435, 0x66},
	{0x0436, 0x66},
	{0x0437, 0x66},
	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x38},
	// {0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1LANE_1920X1080_25FPS
static struct regval_list sensor_1080p25_1lane_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0xae},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x07},
	{0x0d06, 0x29},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	/* vts */
	{0x0d41, 0x04},
	{0x0d42, 0x38},
	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x05},
	{0x008d, 0xa8},
	{0x007a, 0x58},//global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},
	{0x0102, 0xa9},
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},
	{0x0428, 0x86},
	{0x0429, 0x86},
	{0x042a, 0x86},
	{0x042b, 0x68},
	{0x042c, 0x68},
	{0x042d, 0x68},
	{0x042e, 0x68},
	{0x042f, 0x68},
	{0x0430, 0x4f},
	{0x0431, 0x68},
	{0x0432, 0x67},
	{0x0433, 0x66},
	{0x0434, 0x66},
	{0x0435, 0x66},
	{0x0436, 0x66},
	{0x0437, 0x66},
	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x38},
	// {0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1920X1088_20FPS
static struct regval_list sensor_1080p20_regs[] = {
//version 1.0
//mclk 27Mhz
//mipiclk 594Mbps/lane
//framelength 1688
//framrate 30fps
//rowtime 29.616162us
//pattern rggb
/****system****/
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0x8e},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	/****CISCTL & ANALOG****/
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x05},
	{0x0d06, 0xba},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x06},
	{0x0d42, 0x98},

	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x07},
	{0x008d, 0xff},

	/*gain*/
	{0x007a, 0x50}, //global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},

	/*isp*/
	{0x0102, 0xa9}, //89
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},

	{0x0428, 0x86}, //84 edge_th
	{0x0429, 0x86}, //84
	{0x042a, 0x86}, //84
	{0x042b, 0x68}, //84
	{0x042c, 0x68}, //84
	{0x042d, 0x68}, //84
	{0x042e, 0x68}, //83
	{0x042f, 0x68}, //82

	{0x0430, 0x4f}, //82 noise_th
	{0x0431, 0x68}, //82
	{0x0432, 0x67}, //82
	{0x0433, 0x66}, //82
	{0x0434, 0x66}, //82
	{0x0435, 0x66}, //82
	{0x0436, 0x66}, //64
	{0x0437, 0x66}, //68

	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},

	/*dark sun*/
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	/*blk*/
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},

	/*window*/
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},

	/****DVP & MIPI****/
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1920X1088_25FPS
static struct regval_list sensor_1080p25_regs[] = {
//version 1.0
//mclk 27Mhz
//mipiclk 594Mbps/lane
//framelength 1688
//framrate 30fps
//rowtime 29.616162us
//pattern rggb
/****system****/
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0x8e},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	/****CISCTL & ANALOG****/
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x05},
	{0x0d06, 0xba},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x05},
	{0x0d42, 0x46},

	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x07},
	{0x008d, 0xff},

	/*gain*/
	{0x007a, 0x50}, //global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},

	/*isp*/
	{0x0102, 0xa9}, //89
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},

	{0x0428, 0x86}, //84 edge_th
	{0x0429, 0x86}, //84
	{0x042a, 0x86}, //84
	{0x042b, 0x68}, //84
	{0x042c, 0x68}, //84
	{0x042d, 0x68}, //84
	{0x042e, 0x68}, //83
	{0x042f, 0x68}, //82

	{0x0430, 0x4f}, //82 noise_th
	{0x0431, 0x68}, //82
	{0x0432, 0x67}, //82
	{0x0433, 0x66}, //82
	{0x0434, 0x66}, //82
	{0x0435, 0x66}, //82
	{0x0436, 0x66}, //64
	{0x0437, 0x66}, //68

	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},

	/*dark sun*/
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	/*blk*/
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},

	/*window*/
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},

	/****DVP & MIPI****/
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1920X1088_15FPS
static struct regval_list sensor_1080p15_regs[] = {
//version 1.0
//mclk 27Mhz
//mipiclk 594Mbps/lane
//framelength 1688
//framrate 30fps
//rowtime 29.616162us
//pattern rggb
/****system****/
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0x8e},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	/****CISCTL & ANALOG****/
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x05},
	{0x0d06, 0xba},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x08},
	{0x0d42, 0xca},

	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x07},
	{0x008d, 0xff},

	/*gain*/
	{0x007a, 0x50}, //global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},

	/*isp*/
	{0x0102, 0xa9}, //89
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},

	{0x0428, 0x86}, //84 edge_th
	{0x0429, 0x86}, //84
	{0x042a, 0x86}, //84
	{0x042b, 0x68}, //84
	{0x042c, 0x68}, //84
	{0x042d, 0x68}, //84
	{0x042e, 0x68}, //83
	{0x042f, 0x68}, //82

	{0x0430, 0x4f}, //82 noise_th
	{0x0431, 0x68}, //82
	{0x0432, 0x67}, //82
	{0x0433, 0x66}, //82
	{0x0434, 0x66}, //82
	{0x0435, 0x66}, //82
	{0x0436, 0x66}, //64
	{0x0437, 0x66}, //68

	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},

	/*dark sun*/
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	/*blk*/
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},

	/*window*/
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},

	/****DVP & MIPI****/
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1920X1088_10FPS
static struct regval_list sensor_1080p10_regs[] = {
//version 1.0
//mclk 27Mhz
//mipiclk 594Mbps/lane
//framelength 1688
//framrate 30fps
//rowtime 29.616162us
//pattern rggb
/****system****/
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x24},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x43},
	{0x03fc, 0x8e},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	/****CISCTL & ANALOG****/
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x02},
	{0x0d04, 0x02},
	{0x0d05, 0x05},
	{0x0d06, 0xba},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x00},
	{0x0d0a, 0x02},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x04},
	{0x0d0e, 0x40},
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x0d},
	{0x0d42, 0x2f},

	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x07},
	{0x008d, 0xff},

	/*gain*/
	{0x007a, 0x50}, //global gain
	{0x00d0, 0x00},
	{0x0dc1, 0x00},

	/*isp*/
	{0x0102, 0xa9}, //89
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},

	{0x0428, 0x86}, //84 edge_th
	{0x0429, 0x86}, //84
	{0x042a, 0x86}, //84
	{0x042b, 0x68}, //84
	{0x042c, 0x68}, //84
	{0x042d, 0x68}, //84
	{0x042e, 0x68}, //83
	{0x042f, 0x68}, //82

	{0x0430, 0x4f}, //82 noise_th
	{0x0431, 0x68}, //82
	{0x0432, 0x67}, //82
	{0x0433, 0x66}, //82
	{0x0434, 0x66}, //82
	{0x0435, 0x66}, //82
	{0x0436, 0x66}, //64
	{0x0437, 0x66}, //68

	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},

	/*dark sun*/
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	/*blk*/
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},

	/*window*/
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x04},
	{0x0196, 0x40},
	{0x0197, 0x07},
	{0x0198, 0x80},

	/****DVP & MIPI****/
	{0x0201, 0x27},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#ifdef GC2083_1920X360_120FPS
static struct regval_list sensor_1920x360p120_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x14},
	{0x03f7, 0x01},
	{0x03f8, 0x30},
	{0x03f9, 0x43},
	{0x03fc, 0x8e},
	{0x0381, 0x07},
	{0x00d7, 0x29},
	{0x0d6d, 0x18},
	{0x00d5, 0x03},
	{0x0082, 0x01},
	{0x0db3, 0xd4},
	{0x0db0, 0x0d},
	{0x0db5, 0x96},
	{0x0d03, 0x01},
	{0x0d04, 0x51},
	{0x0d05, 0x04},
	{0x0d06, 0xd0},
	{0x0d07, 0x00},
	{0x0d08, 0x11},
	{0x0d09, 0x01},
	{0x0d0a, 0x68},
	{0x000b, 0x00},
	{0x000c, 0x02},
	{0x0d0d, 0x01},
	{0x0d0e, 0x70},//368
	{0x000f, 0x07},
	{0x0010, 0x90},
	{0x0017, 0x0c},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x01},
	{0x0d42, 0x85},//780
	{0x0d7a, 0x10},
	{0x0d19, 0x31},
	{0x0d25, 0xcb},
	{0x0d20, 0x60},
	{0x0d27, 0x03},
	{0x0d29, 0x60},
	{0x0d43, 0x10},
	{0x0d49, 0x10},
	{0x0d55, 0x18},
	{0x0dc2, 0x44},
	{0x0058, 0x3c},
	{0x00d8, 0x68},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0050, 0x18},
	{0x0db6, 0x3d},
	{0x00d2, 0xbc},
	{0x0d66, 0x42},
	{0x008c, 0x03},//05
	{0x008d, 0xd8},//ff
	{0x007a, 0x68},
	{0x00d0, 0x00},
	{0x0dc1, 0x00},
	{0x0102, 0xa9},
	{0x0158, 0x00},
	{0x0107, 0xa6},
	{0x0108, 0xa9},
	{0x0109, 0xa8},
	{0x010a, 0xa7},
	{0x010b, 0xff},
	{0x010c, 0xff},
	{0x0428, 0x86},
	{0x0429, 0x86},
	{0x042a, 0x86},
	{0x042b, 0x68},
	{0x042c, 0x68},
	{0x042d, 0x68},
	{0x042e, 0x68},
	{0x042f, 0x68},
	{0x0430, 0x4f},
	{0x0431, 0x68},
	{0x0432, 0x67},
	{0x0433, 0x66},
	{0x0434, 0x66},
	{0x0435, 0x66},
	{0x0436, 0x66},
	{0x0437, 0x66},
	{0x0438, 0x62},
	{0x0439, 0x62},
	{0x043a, 0x62},
	{0x043b, 0x62},
	{0x043c, 0x62},
	{0x043d, 0x62},
	{0x043e, 0x62},
	{0x043f, 0x62},
	{0x0077, 0x01},
	{0x0078, 0x65},
	{0x0079, 0x04},
	{0x0067, 0xa0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x0026, 0x01},
	{0x0152, 0x02},
	{0x0153, 0x50},
	{0x0155, 0x93},
	{0x0410, 0x16},
	{0x0411, 0x16},
	{0x0412, 0x16},
	{0x0413, 0x16},
	{0x0414, 0x6f},
	{0x0415, 0x6f},
	{0x0416, 0x6f},
	{0x0417, 0x6f},
	{0x04e0, 0x18},
	{0x0192, 0x04},
	{0x0193, 0x00},
	{0x0194, 0x04},
	{0x0195, 0x01},
	{0x0196, 0x68},
	{0x0197, 0x07},
	{0x0198, 0x80},
	{0x0201, 0x27},
	{0x0202, 0x53}, //0x50
	{0x0203, 0xce}, //0xb6//0x8e
	{0x0204, 0x40},
	{0x0212, 0x07},
	{0x0213, 0x80},
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x0237, 0x03},
};
#endif

#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
static struct regval_list sensor_master_mode_regs[] = {
	{0x0068, 0x95},
	{0x0069, 0x00},
	{0x0d69, 0x7f},
	{0x0d6a, 0x80},
	{0x006a, 0xff},
};

static struct regval_list sensor_slave_mode_regs[] = {
	{0x0068, 0x93},
	{0x0069, 0x00},
	{0x0d67, 0x00},
	{0x0d69, 0x90},
	{0x0d6a, 0x80},
	{0x0d6c, 0x02},
	{0x0d6d, 0x89},
	{0x0d6e, 0x00},
	{0x0d6f, 0x04},
	{0x0d70, 0x00},
	{0x0d71, 0x30},
	{0x0d73, 0x92},
	{0x0d6b, 0x51},
	{0x006a, 0xff},
};

static void sensor_i2c_switch_lock_init(void)
{
	mutex_init(&device_mutex);
}

static void sensor_i2c_switch_lock_lock(void)
{
	mutex_lock(&device_mutex);
}

static void sensor_i2c_switch_lock_unlock(void)
{
	mutex_unlock(&device_mutex);
}
#endif
/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function , retrun -EINVAL
 */

static int sensor_g_fps(struct v4l2_subdev *sd, struct sensor_fps *fps)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;
	data_type frame_length = 0, act_vts = 0;

	sensor_read(sd, 0x0d41, &frame_length);
	act_vts = frame_length << 8;
	sensor_read(sd, 0x0d42, &frame_length);
	act_vts |= frame_length;
	fps->fps = wsize->pclk / (wsize->hts * act_vts);
	sensor_dbg("fps = %d\n", fps->fps);

	return 0;
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	struct sensor_info *info = to_state(sd);
	int tmp_exp_val = exp_val / 16;

	sensor_write(sd, 0x0d03, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(sd, 0x0d04, (tmp_exp_val & 0xFF));
	sensor_dbg("exp_val:%d\n", exp_val);
	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

unsigned char regValTable[29][13] = {
	//0x00d0 0x0155 0x0410 0x0411 0x0412 0x0413 0x0414 0x0415 0x0416 0x0417 0x00b8 0x00b9 0x0dc1
	{0x00, 0x00, 0x01, 0x00, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x10, 0x00, 0x01, 0x0c, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x01, 0x00, 0x01, 0x1a, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x11, 0x00, 0x01, 0x2b, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x02, 0x00, 0x02, 0x00, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x12, 0x00, 0x02, 0x18, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x03, 0x00, 0x02, 0x33, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x13, 0x00, 0x03, 0x15, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x04, 0x00, 0x04, 0x00, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x14, 0x00, 0x04, 0xe0, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x05, 0x00, 0x05, 0x26, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x15, 0x00, 0x06, 0x2b, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x44, 0x00, 0x08, 0x00, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x54, 0x00, 0x09, 0x22, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x45, 0x00, 0x0b, 0x0d, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x55, 0x00, 0x0d, 0x16, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x04, 0x01, 0x10, 0x00, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x14, 0x01, 0x13, 0x04, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x24, 0x01, 0x16, 0x1a, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x34, 0x01, 0x1a, 0x2b, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x44, 0x01, 0x20, 0x00, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x54, 0x01, 0x26, 0x07, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x64, 0x01, 0x2c, 0x33, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x74, 0x01, 0x35, 0x17, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f},
	{0x84, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72},
	{0x94, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72},
	{0x85, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72},
	{0x95, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72},
	{0xa5, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72},
};

unsigned int gainLevelTable[29] = {
	 64,   77,   92,   110,  128,        154,
	186,   223,  269,  323,  381,        457,
	544,   653,  762,  914, 1078,       1293,
	1541, 1849, 2177, 2612, 3136,       3764,
	4710, 5652, 6656, 7988, 9474
};

static int total = sizeof(gainLevelTable) / sizeof(unsigned int);
static int setSensorGain(struct v4l2_subdev *sd, int gain)
{
	int i;
	unsigned int tol_dig_gain = 0;

	for (i = 0; (total > 1) && (i < (total - 1)); i++) {
		if ((gainLevelTable[i] <= gain) && (gain < gainLevelTable[i+1]))
			break;
	}

	tol_dig_gain = (gain * 64) / gainLevelTable[i];

	//if(tol_dig_gain>15*64) tol_dig_gain=15;

	sensor_write(sd, 0x00d0, regValTable[i][0]);
	sensor_write(sd, 0x031d, 0x2e);
	sensor_write(sd, 0x0dc1, regValTable[i][1]);
	sensor_write(sd, 0x031d, 0x28);
	sensor_write(sd, 0x00b8, regValTable[i][2]);
	sensor_write(sd, 0x00b9, regValTable[i][3]);
	sensor_write(sd, 0x0155, regValTable[i][4]);
	sensor_write(sd, 0x0410, regValTable[i][5]);
	sensor_write(sd, 0x0411, regValTable[i][6]);
	sensor_write(sd, 0x0412, regValTable[i][7]);
	sensor_write(sd, 0x0413, regValTable[i][8]);
	sensor_write(sd, 0x0414, regValTable[i][9]);
	sensor_write(sd, 0x0415, regValTable[i][10]);
	sensor_write(sd, 0x0416, regValTable[i][11]);

	sensor_write(sd, 0x0417, regValTable[i][12]);

	sensor_write(sd, 0x00b1, (tol_dig_gain >> 6));
	sensor_write(sd, 0x00b2, ((tol_dig_gain & 0x3f) << 2));

	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("gain_val:%d\n", gain_val);
	setSensorGain(sd, gain_val * 4);
	info->gain = gain_val;

	return 0;
}

static int gc2083_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	int shutter = 0, frame_length = 0;
	struct sensor_info *info = to_state(sd);
	__maybe_unused struct i2c_client *client = v4l2_get_subdevdata(sd);
	__maybe_unused struct sensor_switch_cfg_t *sensor_switch_config;
	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < (1 * 16)) {
		gain_val = 16;
	}

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val >> 4;
	if (shutter > gc2083_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = gc2083_sensor_vts;
	sensor_dbg("frame_length = %d\n", frame_length);
#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
	sensor_switch_config = &sensor_switch_cfg;
	if (client->addr == (A_I2C_ADDR >> 1)) {
		sensor_switch_config->exp_val_save[SWITCH_SENSOR_A] = exp_val;
		sensor_switch_config->gain_val_save[SWITCH_SENSOR_A] = gain_val;
		sensor_switch_config->set_expgain_flags[SWITCH_SENSOR_A] = true;
		sensor_switch_config->frame_length_save[SWITCH_SENSOR_A] = frame_length;
	} else if (client->addr == (B_I2C_ADDR >> 1)) {
		sensor_switch_config->exp_val_save[SWITCH_SENSOR_B] = exp_val;
		sensor_switch_config->gain_val_save[SWITCH_SENSOR_B] = gain_val;
		sensor_switch_config->set_expgain_flags[SWITCH_SENSOR_B] = true;
		sensor_switch_config->frame_length_save[SWITCH_SENSOR_B] = frame_length;
	}
#else
	sensor_write(sd, 0x0d42, frame_length & 0xff);
	sensor_write(sd, 0x0d41, frame_length >> 8);

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
#endif
	sensor_dbg("sensor_set_gain exp = %d, gain = %d Done!\n", exp_val, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

typedef struct {
    int blc_val;
    int temper;
} sensor_bcl_temper_entity;
sensor_bcl_temper_entity sensor_blc_temper_table[] = {
// blc_val, temp
	// 1x idx = 0
	{1,    20},
	{2,    30},
	{3,    40},
	{4,    50},
	{5,    60},
	{9,    70},
	{0xFF, 80},
	// 8x idx = 7
	{3,    20},
	{4,    30},
	{5,    40},
	{7,    50},
	{0x0C, 60},
	{0x2D, 70},
	{0xFF, 80},
	// 16x
	{3,    20},
	{5,    30},
	{6,    40},
	{0x0B, 50},
	{0x17, 60},
	{0x5C, 70},
	{0xFF, 80},
	// 32x
	{3,    20},
	{7,    30},
	{0x0A, 40},
	{0x12, 50},
	{0x2A, 60},
	{0xAD, 70},
	{0xFF, 80},
	// 64x
	{3,    20},
	{0x0B, 30},
	{0x10, 40},
	{0x1E, 50},
	{0x53, 60},
	{0xF9, 70},
	{0xFF, 80},
	// 124x
	{3,    20},
	{0x10, 30},
	{0x19, 40},
	{0x2D, 50},
	{0x98, 60},
	{0xFF, 70},
	{0xFF, 80}
};
static int numEntries = sizeof(sensor_blc_temper_table) / sizeof(sensor_bcl_temper_entity);
typedef struct {
    int again_val;
    int temper_idx;
} sensor_again_temper_idx_entity;
sensor_again_temper_idx_entity sensor_again_temper_idx_table[] = {
	{1,    0},
	{8,    7},
	{16,  14},
	{32,  21},
	{64,  28},
	{124, 35}
};
static int again_idx_total = sizeof(sensor_again_temper_idx_table) / sizeof(sensor_again_temper_idx_entity);
typedef struct {
    int again_val;
    int temper_val;
} gain_temper_entity;

static int ValueInterp(int curr, int x_low, int x_high, int y_low, int y_high)
{
	if ((x_high - x_low) == 0)
		return y_low;

	if (curr <= x_low)
		return y_low;
	else if (curr >= x_high)
		return y_high;

	return (y_low + (y_high - y_low)*(curr - x_low) / (x_high - x_low));
}

static int blc_valueinterp_for_temper(int cur_blc_val, sensor_bcl_temper_entity blc_low, sensor_bcl_temper_entity blc_high)
{
	int temper_reslut;

	temper_reslut = ValueInterp(cur_blc_val, blc_low.blc_val, blc_high.blc_val, blc_low.temper, blc_high.temper);

	return temper_reslut;
}

static int gain_valueinterp_for_temper(gain_temper_entity cur_gain, gain_temper_entity gain_low, gain_temper_entity gain_high)
{
	int temper_reslut;

	temper_reslut = ValueInterp(cur_gain.again_val, gain_low.again_val, gain_high.again_val, gain_low.temper_val, gain_high.temper_val);

	return temper_reslut;
}

#define SENSOR_TEMPER_REGS 0x040c
#define BLC_CUMU_CNT 5
#define GAIN_1X_TEMP_OFFSET 12
#define GAIN_OVER_1X_TEMP_OFFSET 12

static int sensor_get_temp(struct v4l2_subdev *sd,  struct sensor_temp *temp)
{
	gain_temper_entity gain_low, gain_high, curr_gain_reslut;
	data_type blc_val = 0;
	int blc_val_sum = 0;
	int blc_get_cnt = 0;
	int temper_idx = 0;
	int again_idx = 0;
	int cur_again = 0;
	int eRet;

	memset(&gain_low, 0, sizeof(gain_temper_entity));
	memset(&gain_high, 0, sizeof(gain_temper_entity));
	memset(&curr_gain_reslut, 0, sizeof(gain_temper_entity));

	while (blc_get_cnt < BLC_CUMU_CNT) {
		eRet = sensor_read(sd, SENSOR_TEMPER_REGS, &blc_val);
		if (eRet < 0) {
			sensor_err("read SENSOR_TEMPER_REGS:0x040c failed!!\n");
			return -1;
		} else {
			blc_val_sum += blc_val;
			blc_get_cnt++;
		}
	}

	blc_val = blc_val_sum / BLC_CUMU_CNT;
	sensor_g_gain(sd, &cur_again);
	if (cur_again <= 16) {
		cur_again = 16;
	}
	cur_again = cur_again >> 4;

	for (again_idx = 0; again_idx < again_idx_total; again_idx++) {
		if (cur_again >= 124) {
			gain_low.again_val = sensor_again_temper_idx_table[again_idx_total - 1].again_val;
			temper_idx = sensor_again_temper_idx_table[again_idx_total - 1].temper_idx;
			break;
		}
		if ((sensor_again_temper_idx_table[again_idx].again_val <= cur_again) && (cur_again < sensor_again_temper_idx_table[again_idx + 1].again_val)) {
			temper_idx = sensor_again_temper_idx_table[again_idx].temper_idx;
			gain_low.again_val = sensor_again_temper_idx_table[again_idx].again_val;
			gain_high.again_val = sensor_again_temper_idx_table[again_idx + 1].again_val;
			break;
		}
	}

	for (; temper_idx < numEntries; temper_idx++) {
			if (blc_val >= sensor_blc_temper_table[temper_idx].blc_val && blc_val < sensor_blc_temper_table[temper_idx + 1].blc_val) {
				gain_low.temper_val = blc_valueinterp_for_temper(blc_val, sensor_blc_temper_table[temper_idx], sensor_blc_temper_table[temper_idx + 1]);
				break;
			}
	}

	if (cur_again >= 124) {
		sensor_print("special sulo\n");
		temp->temp = gain_low.temper_val;
	} else {
		/* search next_gain's blc_temper */
		temper_idx = sensor_again_temper_idx_table[again_idx + 1].temper_idx;
		for (; temper_idx < numEntries; temper_idx++) {
			if (blc_val >= sensor_blc_temper_table[temper_idx].blc_val && blc_val < sensor_blc_temper_table[temper_idx + 1].blc_val) {
				gain_high.temper_val = blc_valueinterp_for_temper(blc_val, sensor_blc_temper_table[temper_idx], sensor_blc_temper_table[temper_idx + 1]);
				break;
			}
		}
		curr_gain_reslut.temper_val = gain_valueinterp_for_temper(curr_gain_reslut, gain_low, gain_high);
		temp->temp = curr_gain_reslut.temper_val;
	}

	if (cur_again < 8) {
		/* 1x temper range is too short */
		temp->temp = temp->temp - GAIN_1X_TEMP_OFFSET;
	} else {
		temp->temp = temp->temp + GAIN_OVER_1X_TEMP_OFFSET;
	}
	//sensor_print("cur_again = %d, blc_val = 0x%x, gain: low(%d, %d), high(%d, %d), numEntries = %d, temp = %d\n",
	//		cur_again, blc_val, gain_low.again_val, gain_low.temper_val, gain_high.again_val, gain_high.temper_val, numEntries, temp->temp);

	return 0;
}

/*
 *set && get sensor flip
 */

static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	*code = MEDIA_BUS_FMT_SRGGB10_1X10;
	sensor_print("gc2083 will change bayer format by itself!\n");

	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;
	data_type value_0015;
	data_type value_0d15;
	int times_out = 5;
	int eRet;

	if (!(enable == 0 || enable == 1)) {
		sensor_err("Invalid parameter!!!\n");
		return -1;
	}

	eRet = sensor_read(sd, 0x0015, &value_0015);
	eRet = sensor_read(sd, 0x0d15, &value_0d15);
	get_value = value_0015 & 0x3;
	sensor_print("[H] read regs : value_0015 = 0x%x, value_0d15 = 0x%x\n", value_0015, value_0d15);

	if (enable)
		set_value = get_value | 0x01;
	else
		set_value = get_value & 0xFE;

	do {
		/* write repeatly */
		sensor_write(sd, 0x0015, set_value);
		sensor_write(sd, 0x0d15, set_value);
		eRet = sensor_read(sd, 0x0015, &value_0015);
		eRet = sensor_read(sd, 0x0d15, &value_0d15);
		sensor_print("[H] eRet:%d, value_0015 = 0x%x, value_0d15 = 0x%x, times_out:%d\n", eRet, value_0015, value_0d15, times_out);
		usleep_range(5000, 10000);
		times_out--;
	} while ((value_0015 != set_value) && (value_0d15 != set_value) && (times_out >= 0));

	if ((times_out < 0) && ((value_0d15 != set_value) || (value_0015 != set_value))) {
		sensor_err("set hflip failed, please set more times!!!\n");
		return -1;
	} else {
		sensor_print("hflip finish, set_value : 0x%x, value_0015 = 0x%x, value_0d15 = 0x%x\n",
			set_value, value_0015, value_0d15);
	}

	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;
	data_type value_0015;
	data_type value_0d15;
	int times_out = 5;
	int eRet;

	if (!(enable == 0 || enable == 1)) {
		sensor_err("Invalid parameter!!!\n");
		return -1;
	}

	eRet = sensor_read(sd, 0x0015, &value_0015);
	eRet = sensor_read(sd, 0x0d15, &value_0d15);
	get_value = value_0015 & 0x3;
	sensor_print("[V] read regs : value_0015 = 0x%x, value_0d15 = 0x%x\n", value_0015, value_0d15);

	if (enable)
		set_value = get_value | 0x02;
	else
		set_value = get_value & 0xFD;

	do {
		/* write repeatly */
		sensor_write(sd, 0x0015, set_value);
		sensor_write(sd, 0x0d15, set_value);
		eRet = sensor_read(sd, 0x0015, &value_0015);
		eRet = sensor_read(sd, 0x0d15, &value_0d15);
		sensor_print("[V] eRet:%d, value_0015 = 0x%x, value_0d15 = 0x%x, times_out:%d\n", eRet, value_0015, value_0d15, times_out);
		usleep_range(5000, 10000);
		times_out--;
	} while ((value_0015 != set_value) && (value_0d15 != set_value) && (times_out >= 0));

	if ((times_out < 0) && ((value_0d15 != set_value) || (value_0015 != set_value))) {
		sensor_err("set vflip failed, please set more times!!!\n");
		return -1;
	} else {
		sensor_print("vflip finish, set_value : 0x%x, value_0015 = 0x%x, value_0d15 = 0x%x\n",
			set_value, value_0015, value_0d15);
	}

	return 0;
}

/*
* Stuff that knows about the sensor.
*/
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		break;

	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		usleep_range(10000, 12000);
		break;

	case PWR_ON:
		sensor_print("PWR_ON!!!\n");
		cci_lock(sd);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(1000, 1200);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		//vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		//vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		//vin_set_pmu_channel(sd, CMBCSI, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;

	case PWR_OFF:
		sensor_print("PWR_OFF!!do nothing\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		//vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		//vin_set_pmu_channel(sd, CMBCSI, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		cci_unlock(sd);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{

	sensor_dbg("%s: val=%d\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;

	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
#if !IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	data_type rdval;
	int eRet;
	int times_out = 3;
	do {
		eRet = sensor_read(sd, ID_REG_HIGH, &rdval);
		sensor_dbg("eRet:%d, ID_VAL_HIGH:0x%x, times_out:%d\n", eRet, rdval, times_out);
		usleep_range(200, 220);
		times_out--;
	} while (eRet < 0  &&  times_out > 0);

	sensor_read(sd, ID_REG_HIGH, &rdval);
	sensor_dbg("ID_VAL_HIGH = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;

	sensor_read(sd, ID_REG_LOW, &rdval);
	sensor_dbg("ID_VAL_LOW = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_LOW)
		return -ENODEV;

	sensor_dbg("Done!\n");
#endif
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed    = 0;
	info->width        = 1920;
	info->height       = 1080;
	info->hflip        = 0;
	info->vflip        = 0;
	info->tpf.numerator      = 1;
	info->tpf.denominator    = 20;	/* 30fps */
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_GET_FPS:
		ret = sensor_g_fps(sd, (struct sensor_fps *)arg);
		break;
//	case VIDIOC_VIN_SENSOR_SET_FPS:
//		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
//		break;
	case VIDIOC_VIN_SENSOR_GET_TEMP:
		sensor_get_temp(sd, (struct sensor_temp *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_GET_SENSOR_CODE:
		sensor_get_fmt_mbus_core(sd, (int *)arg);
		break;
	case VIDIOC_VIN_SET_IR:
		sensor_set_ir(sd, (struct ir_switch *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc      = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
		.regs      = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp       = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
#if IS_ENABLED(CONFIG_SENSOR_GC2083_ONE_LANE_MIPI)
#ifdef GC2083_1LANE_1920X1080_10FPS
	{
		.width = 1920,
		.height = 1080,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3666,
		.vts = 2700,
		.pclk = 98982000,
		.mipi_bps   = 593 * 1000 * 1000,
		.fps_fixed = 10,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (2700 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p10_1lane_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p10_1lane_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1LANE_1920X1080_15FPS
	{
		.width = 1920,
		.height = 1080,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3666,
		.vts = 1800,
		.pclk = 98982000,
		.mipi_bps   = 593 * 1000 * 1000,
		.fps_fixed = 15,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (1800 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p15_1lane_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p15_1lane_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1LANE_1920X1080_20FPS
	{
		.width = 1920,
		.height = 1080,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3666,
		.vts = 1350,
		.pclk = 98982000,
		.mipi_bps   = 593 * 1000 * 1000,
		.fps_fixed = 20,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (1350 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p20_1lane_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p20_1lane_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1LANE_1920X1080_25FPS
	{
		.width = 1920,
		.height = 1080,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3666,
		.vts = 1080,
		.pclk = 98982000,
		.mipi_bps   = 593 * 1000 * 1000,
		.fps_fixed = 25,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (1080 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p25_1lane_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p25_1lane_regs),
		.set_size = NULL,
	},
#endif

#else	/* two lane */
#ifdef GC2083_1920X1088_10FPS
	{
		.width = 1920,
		.height = 1088,
		.hoffset = 0,
		.voffset = 0,
		.hts = 2932,
		.vts = 3375,
		.pclk = 98955000,
		.mipi_bps   = 594 * 1000 * 1000,
		.fps_fixed = 10,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (3375 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p10_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p10_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1920X1088_15FPS
	{
		.width = 1920,
		.height = 1088,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3140,
		.vts = 2250,
		.pclk = 63600000,
		.mipi_bps   = 318 * 1000 * 1000,
		.fps_fixed = 15,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (2250 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p15_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p15_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1920X1088_25FPS
	{
		.width = 1920,
		.height = 1088,
		.hoffset = 0,
		.voffset = 0,
		.hts = 2932,
		.vts = 1350,
		.pclk = 98955000,
		.mipi_bps   = 594 * 1000 * 1000,
		.fps_fixed = 25,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (1350 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p25_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p25_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1920X1088_20FPS
	{
		.width = 1920,
		.height = 1088,
		.hoffset = 0,
		.voffset = 0,
		.hts = 2932,
		.vts = 1688,
		.pclk = 98955000,
		.mipi_bps   = 594 * 1000 * 1000,
		.fps_fixed = 20,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (1688 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1080p20_regs,
		.regs_size = ARRAY_SIZE(sensor_1080p20_regs),
		.set_size = NULL,
	},
#endif

#ifdef GC2083_1920X360_120FPS
	{
		.width = 1920,
		.height = 360,
		.hoffset = 0,
		.voffset = 0,
		.hts = 2464,
		.vts = 389,
		.pclk = 115019520,
		.mipi_bps   = 576 * 1000 * 1000,
		.fps_fixed = 120,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max   = (389 - 16) << 4,
		.gain_min = 1 << 4,
		.gain_max   = 110 << 4,
		.regs = sensor_1920x360p120_regs,
		.regs_size = ARRAY_SIZE(sensor_1920x360p120_regs),
		.set_size = NULL,
	},
#endif
#endif

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
__maybe_unused static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
#if IS_ENABLED(CONFIG_SENSOR_GC2083_ONE_LANE_MIPI)
	cfg->type  = V4L2_MBUS_CSI2_DPHY;
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else	/* two lane */
	cfg->type  = V4L2_MBUS_CSI2_DPHY;
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif
	return 0;
}
#else
__maybe_unused static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
#if IS_ENABLED(CONFIG_SENSOR_GC2083_ONE_LANE_MIPI)
	cfg->type  = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else	/* two lane */
	cfg->type  = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif
	return 0;
}
#endif

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->val);
	}
	return -EINVAL;
}

#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
static void sensor_switch_change(struct v4l2_subdev *sd)
{
	struct v4l2_subdev *sd_temp;
	unsigned int max_frame_length;
	struct sensor_switch_cfg_t *sensor_switch_config;
	unsigned int stream_count;

	sensor_switch_config = &sensor_switch_cfg;

	if (!strcmp(sd->name, cci_drv[0].name))
		sd_temp = cci_drv[1].sd;
	else if (!strcmp(sd->name, cci_drv[1].name))
		sd_temp = cci_drv[0].sd;

	max_frame_length = max(sensor_switch_config->frame_length_save[SWITCH_SENSOR_A], sensor_switch_config->frame_length_save[SWITCH_SENSOR_B]);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	struct sensor_info *info = to_state(sd);
	stream_count = info->stream_count;
#else
	stream_count = sd->entity.stream_count;
#endif
	if (stream_count == 0) {
		sensor_write(sd_temp, 0x0d42, max_frame_length & 0xff);
		sensor_write(sd_temp, 0x0d41, max_frame_length >> 8);
		if (sensor_switch_config->set_expgain_flags[SWITCH_SENSOR_B]) {
			sensor_s_exp(sd_temp, sensor_switch_config->exp_val_save[SWITCH_SENSOR_B]);
			sensor_s_gain(sd_temp, sensor_switch_config->gain_val_save[SWITCH_SENSOR_B]);

			sensor_switch_config->set_expgain_flags[SWITCH_SENSOR_B] = false;
		}
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	stream_count = info->stream_count;
#else
	stream_count = sd->entity.stream_count;
#endif
	if (stream_count == 0) {
		sensor_write(sd, 0x0d42, max_frame_length & 0xff);
		sensor_write(sd, 0x0d41, max_frame_length >> 8);

		if (sensor_switch_config->set_expgain_flags[SWITCH_SENSOR_A]) {

			sensor_s_exp(sd, sensor_switch_config->exp_val_save[SWITCH_SENSOR_A]);
			sensor_s_gain(sd, sensor_switch_config->gain_val_save[SWITCH_SENSOR_A]);

			sensor_switch_config->set_expgain_flags[SWITCH_SENSOR_A] = false;
		}
	}
}

static void __s_sensor_switch_change_handle(struct work_struct *work)
{
	struct sensor_vysnc_config *sensor_vysnc_cfg = container_of(work, struct sensor_vysnc_config, s_sensor_switch_change_task);
	struct sensor_info *info = container_of(sensor_vysnc_cfg, struct sensor_info, sensor_vysnc_cfg);
	struct v4l2_subdev *sd = &info->sd;

	if (!sd || !sd->entity.use_count)
		return;

	sensor_i2c_switch_lock_lock();
	sensor_switch_change(sd);
	sensor_i2c_switch_lock_unlock();
}

static irqreturn_t sensor_vsync_irq_func(int irq, void *priv)
{
	struct v4l2_subdev *sd = priv;
	struct sensor_info *info = to_state(sd);
	struct sensor_vysnc_config *sensor_vysnc_cfg = &info->sensor_vysnc_cfg;
	struct sensor_switch_cfg_t *sensor_switch_config;
	unsigned long flags;

	sensor_switch_config = &sensor_switch_cfg;

	spin_lock_irqsave(&info->slock, flags);
	schedule_work(&sensor_vysnc_cfg->s_sensor_switch_change_task);
	spin_unlock_irqrestore(&info->slock, flags);

	return IRQ_HANDLED;
}

static int sensor_vysnc_init(struct v4l2_subdev *sd)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct sensor_vysnc_config *sensor_vysnc_cfg = &info->sensor_vysnc_cfg;

	ret = vin_gpio_to_irq(sd, VSYNC, &sensor_vysnc_cfg->gpio_irq);
	if (ret < 0)
		return -1;

	ret = request_irq(sensor_vysnc_cfg->gpio_irq, sensor_vsync_irq_func,
			IRQF_TRIGGER_HIGH, //| IRQF_TRIGGER_RISING,
			sd->name, sd);

	disable_irq(sensor_vysnc_cfg->gpio_irq);

	INIT_WORK(&sensor_vysnc_cfg->s_sensor_switch_change_task, __s_sensor_switch_change_handle);
	return 0;
}

static void sensor_vysnc_exit(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_vysnc_config *sensor_vysnc_cfg = &info->sensor_vysnc_cfg;

	cancel_work_sync(&sensor_vysnc_cfg->s_sensor_switch_change_task);
	if (sensor_vysnc_cfg->gpio_irq > 0) {
		disable_irq(sensor_vysnc_cfg->gpio_irq);
		free_irq(sensor_vysnc_cfg->gpio_irq, sd);
	}
}
#endif

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	__maybe_unused struct sensor_exp_gain exp_gain;
	__maybe_unused struct v4l2_subdev *sd_temp;
	__maybe_unused struct sensor_switch_cfg_t *sensor_switch_config;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
	if (info->vsync_flag) {
		if (!sensor_vysnc_init(sd))
			sensor_i2c_switch_lock_init();
		info->vsync_flag = 0;
	}
#endif

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	if (info->preview_first_flag) {
		info->preview_first_flag = 0;
	} else {
		if (wsize->regs)
			sensor_write_array(sd, wsize->regs, wsize->regs_size);
		if (info->exp && info->gain) {
			exp_gain.exp_val = info->exp;
			exp_gain.gain_val = info->gain;
		} else {
			exp_gain.exp_val = 6000;
			exp_gain.gain_val = 32;
		}
		sensor_s_exp_gain(sd, &exp_gain);
	}
#else
	if (wsize->regs) {
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	}
#endif

	if (wsize->set_size)
		wsize->set_size(sd);
#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
	if (cci_drv[0].sd != NULL && cci_drv[1].sd != NULL) {
		if (!strcmp(sd->name, cci_drv[0].name))
				sd_temp = cci_drv[1].sd;
			else if (!strcmp(sd->name, cci_drv[1].name))
				sd_temp = cci_drv[0].sd;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		unsigned int stream_count;
		stream_count = info->stream_count;
		if (stream_count == 0 && stream_count != 0) {
					sensor_write_array(sd, sensor_master_mode_regs,  ARRAY_SIZE(sensor_master_mode_regs));
		} else if (stream_count != 0) {
			sensor_write_array(sd, sensor_slave_mode_regs,  ARRAY_SIZE(sensor_slave_mode_regs));
#else
		if (sd_temp->entity.stream_count == 0 && sd->entity.stream_count != 0) {
					sensor_write_array(sd, sensor_master_mode_regs,  ARRAY_SIZE(sensor_master_mode_regs));
		} else if (sd->entity.stream_count != 0) {
			sensor_write_array(sd, sensor_slave_mode_regs,  ARRAY_SIZE(sensor_slave_mode_regs));
#endif

#ifdef CONFIG_SENSOR_GC2083_ONE_LANE_MIPI
			sensor_write(sd, 0x023e, 0x98);
			sensor_write(sd_temp, 0x023e, 0x98);
#else
			sensor_write(sd, 0x023e, 0x99);
			sensor_write(sd_temp, 0x023e, 0x99);
#endif
		}
	}
#else
#ifdef CONFIG_SENSOR_GC2083_ONE_LANE_MIPI
	sensor_write(sd, 0x023e, 0x98);
#else
	sensor_write(sd, 0x023e, 0x99);
#endif
#endif
#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
	if (info->sensor_vysnc_cfg.gpio_irq > 0) {
		sensor_switch_config = &sensor_switch_cfg;
		sensor_switch_config->vsync_cnt = 0;
		enable_irq(info->sensor_vysnc_cfg.gpio_irq);
	}
#endif
	info->width = wsize->width;
	info->height = wsize->height;
	gc2083_sensor_vts = wsize->vts;

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
			 wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	__maybe_unused struct sensor_vysnc_config *sensor_vysnc_cfg = &info->sensor_vysnc_cfg;

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
			 info->current_wins->width, info->current_wins->height,
			 info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable) {
		if (sensor_vysnc_cfg->gpio_irq > 0) {
			disable_irq(sensor_vysnc_cfg->gpio_irq);
			cancel_work_sync(&sensor_vysnc_cfg->s_sensor_switch_change_task);
		}
		return 0;
	}

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */
static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.try_ctrl = sensor_try_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {

	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	.g_mbus_config = sensor_g_mbus_config,
#endif
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 4);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
				  256 * 1600, 1, 1 * 1600);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
				  65536 * 16, 1, 1);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}
static int sensor_dev_id;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	sensor_init_controls(sd, &sensor_ctrl_ops);

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE;
	//info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->preview_first_flag = 1;
	info->exp = 0;
	info->gain = 0;
	info->first_power_flag = 1;
	info->vsync_flag = 1;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sensor_remove(struct i2c_client *client)
#else
static void sensor_remove(struct i2c_client *client)
#endif
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}
#if IS_ENABLED(CONFIG_SENSOR_GC2083_VSYNC_MODE)
	if (!to_state(sd)->vsync_flag)
		sensor_vysnc_exit(sd);
#endif
	kfree(to_state(sd));
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
