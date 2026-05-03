/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2023 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors: Liang WeiJie <liangweijie@allwinnertech.com>
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for gc1084 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR  0x1084

#define EXP_MID			0x0d03
#define EXP_LOW			0x0d04

#define ID_REG_HIGH		0x03f0
#define ID_REG_LOW		0x03f1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 20

#if IS_ENABLED(CONFIG_SENSOR_GC1084_8BIT_MIPI)
#define RAW8 1 /* raw8 select */
#else
#define RAW8 0 /* raw10 select */
#endif

/*
 * The gc1084 i2c address
 */
#define I2C_ADDR 0x6e
#if IS_ENABLED(CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI)
#define TARGET_I2C_ADDR 0x7e
#endif

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc1084_mipi"
#define SENSOR_NAME_2 "gc1084_mipi_2"

#define SENSOR_720p30 1
#define SENSOR_720p20 1
#define SENSOR_720p15 1
#define SENSOR_720p10 1
#define SENSOR_360p120 0

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

#if SENSOR_720p30
static struct regval_list sensor_720p30_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x02},
	{0x0d42, 0xee},
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	/* 20230308 */
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x023e, 0x98},
	{0x031e, 0x3e},
#endif
	{0x0040, 0x02},
};
#endif

#if SENSOR_720p20
static struct regval_list sensor_720p20_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x04},
	{0x0d42, 0x65},
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	/* 20230308 */
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x023e, 0x98},
	{0x031e, 0x3e},
#endif
	{0x0040, 0x02},
};
#endif

#if SENSOR_720p15
static struct regval_list sensor_720p15_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x05},
	{0x0d42, 0xdc}, /* vts 1500 */
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	/* 20230308 */
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x023e, 0x98},
	{0x031e, 0x3e},
#endif
	{0x0040, 0x02},
};
#endif

#if SENSOR_720p10
static struct regval_list sensor_720p10_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x2c},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x08},
	{0x0d06, 0x98},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd4},
	{0x000f, 0x05},
	{0x0010, 0x08},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x08},
	{0x0d42, 0xca}, /* vts 2250 */
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},

	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},

	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xd0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x02},
	{0x0194, 0x04},
	{0x0195, 0x02},
	{0x0196, 0xd0},
	{0x0197, 0x05},
	{0x0198, 0x00},
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
#if RAW8
	{0x0212, 0x05},
	{0x0213, 0x00},
#else
	{0x0212, 0x06},
	{0x0213, 0x40},
#endif
	/* 20230308 */
#if RAW8
	{0x0215, 0x12},
	{0x0229, 0x05},
	{0x023e, 0x9c},
	{0x031e, 0x3e},
#else
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x023e, 0x98},
	{0x031e, 0x3e},
#endif
	{0x0040, 0x02},
};
#endif

#if SENSOR_360p120
static struct regval_list sensor_360p120_regs[] = {
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0xf0},
	{0x03fe, 0x00},
	{0x03f2, 0x00},
	{0x03f3, 0x00},
	{0x03f4, 0x36},
	{0x03f5, 0xc0},
	{0x03f6, 0x13},
	{0x03f7, 0x01},
	{0x03f8, 0x48},
	{0x03f9, 0x21},
	{0x03fc, 0xae},
	{0x0d05, 0x07},
	{0x0d06, 0x08},
	{0x0d08, 0x10},
	{0x0d0a, 0x02},
	{0x000c, 0x03},
	{0x0d0d, 0x02},
	{0x0d0e, 0xd0},
	{0x000f, 0x05},
	{0x0010, 0x00},
	{0x0017, 0x08},
	{0x0d73, 0x92},
	{0x0076, 0x00},
	{0x0d76, 0x00},
	{0x0d41, 0x02},
	{0x0d42, 0xee},
	{0x0d7a, 0x0a},
	{0x006b, 0x18},
	{0x0db0, 0x9d},
	{0x0db1, 0x00},
	{0x0db2, 0xac},
	{0x0db3, 0xd5},
	{0x0db4, 0x00},
	{0x0db5, 0x97},
	{0x0db6, 0x09},
	{0x00d2, 0xfc},
	{0x0d19, 0x31},
	{0x0d20, 0x40},
	{0x0d25, 0xcb},
	{0x0d27, 0x03},
	{0x0d29, 0x40},
	{0x0d43, 0x20},
	{0x0058, 0x60},
	{0x00d6, 0x66},
	{0x00d7, 0x19},
	{0x0093, 0x02},
	{0x00d9, 0x14},
	{0x00da, 0xc1},
	{0x0d2a, 0x00},
	{0x0d28, 0x04},
	{0x0dc2, 0x84},
	{0x0050, 0x30},
	{0x0080, 0x07},
	{0x008c, 0x05},
	{0x008d, 0xa8},
	{0x0077, 0x01},
	{0x0078, 0xee},
	{0x0079, 0x02},
	{0x0067, 0xc0},
	{0x0054, 0xff},
	{0x0055, 0x02},
	{0x0056, 0x00},
	{0x0057, 0x04},
	{0x005a, 0xff},
	{0x005b, 0x07},
	{0x00d5, 0x03},
	{0x0102, 0xa9},
	{0x0d03, 0x02},
	{0x0d04, 0xe0},
	{0x007a, 0x60},
	{0x04e0, 0xff},
	{0x0414, 0x75},
	{0x0415, 0x75},
	{0x0416, 0x75},
	{0x0417, 0x75},
	{0x0122, 0x00},
	{0x0121, 0x80},
	{0x0428, 0x10},
	{0x0429, 0x10},
	{0x042a, 0x10},
	{0x042b, 0x10},
	{0x042c, 0x14},
	{0x042d, 0x14},
	{0x042e, 0x18},
	{0x042f, 0x18},
	{0x0430, 0x05},
	{0x0431, 0x05},
	{0x0432, 0x05},
	{0x0433, 0x05},
	{0x0434, 0x05},
	{0x0435, 0x05},
	{0x0436, 0x05},
	{0x0437, 0x05},
	/* out */
	{0x0153, 0x00},
	{0x0190, 0x01},
	{0x0192, 0x00},
	{0x0194, 0x00},
	{0x0195, 0x01},
	{0x0196, 0x68},
	{0x0197, 0x02},
	{0x0198, 0x80},
	/* mipi */
	{0x0201, 0x23},
	{0x0202, 0x53},
	{0x0203, 0xce},
	{0x0208, 0x39},
	{0x0212, 0x03},
	{0x0213, 0x20},
	{0x0215, 0x10},
	{0x0229, 0x05},
	{0x023e, 0x98},
	{0x031e, 0x3e},
	/* binning */
	{0x0040, 0x0a},
	{0x0015, 0x04},
	{0x0d15, 0x04},
	{0x03fc, 0x8e},
};
#endif

static struct regval_list sensor_fmt_raw[] = {
};

#if IS_ENABLED(CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI)
static int sensor_i2c_addr_get(struct v4l2_subdev *sd, unsigned int *iic_addr)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (client == NULL) {
		sensor_err("client is NULL!!!\n");
		return -1;
	}
	/* i2c addr useful for 7bit */
	*iic_addr = client->addr << 1;

	return 0;
}

static int sensor_i2c_addr_set(struct v4l2_subdev *sd, unsigned int iic_addr)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (client == NULL) {
		sensor_err("client is NULL!!!\n");
		return -1;
	}
	/* i2c addr useful for 7bit */
	client->addr = iic_addr >> 1;

	return 0;
}
#endif

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
	unsigned char explow, expmid;
	struct sensor_info *info = to_state(sd);

	expmid = (unsigned char)((0x0ff000 & exp_val) >> 12);
	explow = (unsigned char)((0x000ff0 & exp_val) >> 4);

	sensor_write(sd, EXP_MID, expmid);
	sensor_write(sd, EXP_LOW, explow);

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

unsigned char regValTable[25][6] = {
	{0x00, 0x00, 0x00, 0x80, 0x01, 0x00},
	{0x0a, 0x00, 0x00, 0x80, 0x01, 0x0b},
	{0x00, 0x01, 0x00, 0x80, 0x01, 0x19},
	{0x0a, 0x01, 0x00, 0x80, 0x01, 0x2a},
	{0x00, 0x02, 0x00, 0x80, 0x02, 0x00},
	{0x0a, 0x02, 0x00, 0x80, 0x02, 0x17},
	{0x00, 0x03, 0x00, 0x80, 0x02, 0x33},
	{0x0a, 0x03, 0x00, 0x80, 0x03, 0x14},
	{0x00, 0x04, 0x00, 0x90, 0x04, 0x00},
	{0x0a, 0x04, 0x00, 0x90, 0x04, 0x2f},
	{0x00, 0x05, 0x00, 0x90, 0x05, 0x26},
	{0x0a, 0x05, 0x00, 0x90, 0x06, 0x28},
	{0x00, 0x06, 0x00, 0xa0, 0x08, 0x00},
	{0x0a, 0x06, 0x00, 0xa0, 0x09, 0x1e},
	{0x12, 0x46, 0x00, 0xa0, 0x0b, 0x0c},
	{0x19, 0x66, 0x00, 0xa0, 0x0d, 0x10},
	{0x00, 0x04, 0x01, 0xa0, 0x10, 0x00},
	{0x0a, 0x04, 0x01, 0xa0, 0x12, 0x3d},
	{0x00, 0x05, 0x01, 0xb0, 0x16, 0x19},
	{0x0a, 0x05, 0x01, 0xc0, 0x1a, 0x23},
	{0x00, 0x06, 0x01, 0xc0, 0x20, 0x00},
	{0x0a, 0x06, 0x01, 0xc0, 0x25, 0x3b},
	{0x12, 0x46, 0x01, 0xc0, 0x2c, 0x30},
	{0x19, 0x66, 0x01, 0xd0, 0x35, 0x01},
	{0x20, 0x06, 0x01, 0xe0, 0x3f, 0x3f},
};

unsigned int gainLevelTable[25] = {
	64,  76,  90, 106, 128, 152, 179, 212, 256, 303,
	358, 425, 512, 607, 716, 848, 1024, 1214, 1434, 1699,
	2048, 2427, 2865, 3393, 4096,
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

	tol_dig_gain = gain * 64 / gainLevelTable[i];
	sensor_write(sd, 0x00d1, regValTable[i][0]);
	sensor_write(sd, 0x00d0, regValTable[i][1]);
	sensor_write(sd, 0x031d, 0x2e);
	sensor_write(sd, 0x0dc1, regValTable[i][2]);
	sensor_write(sd, 0x031d, 0x28);
	sensor_write(sd, 0x0155, regValTable[i][3]);
	sensor_write(sd, 0x00b8, regValTable[i][4]);
	sensor_write(sd, 0x00b9, regValTable[i][5]);
	sensor_write(sd, 0x00b1, (tol_dig_gain>>6));
	sensor_write(sd, 0x00b2, ((tol_dig_gain&0x3f)<<2));

	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);

	setSensorGain(sd, gain_val * 4);
	sensor_dbg("gain_val:%d\n", gain_val);

	info->gain = gain_val;

	return 0;
}

static int gc1084_sensor_vts = 1125;
static int sensor_s_exp_gain(struct v4l2_subdev *sd, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	int shutter = 0, frame_length = 0;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;

	shutter = exp_val >> 4;
	if (shutter > gc1084_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = gc1084_sensor_vts;
	sensor_dbg("frame_length = %d\n", frame_length);
	sensor_write(sd, 0x0d42, frame_length & 0xff);
	sensor_write(sd, 0x0d41, frame_length >> 8);

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

typedef struct {
	int blc_val;
	int temper;
} sensor_bcl_temper_entity;
sensor_bcl_temper_entity sensor_blc_temper_table[] = {
/* blc_val, temp */
	/* 1x idx = 0 */
	{0x1,    40},
	{0x2,    50},
	{0x5,    60},
	{0x6,    70},
	{0x7,    80},
	{0x9,    90},
	{0x21,   100},
	/* 8x idx = 7 */
	{0x5,    40},
	{0x8,    50},
	{0x9,    60},
	{0xb,    70},
	{0x13,	 80},
	{0x1b,   90},
	{0x21,   100},
	/* 16x */
	{0x6,    40},
	{0x8,    50},
	{0x9,    60},
	{0xb,    70},
	{0x13,	 80},
	{0x1b,   90},
	{0x21,   100},
	/* 32x */
	{0x6,    40},
	{0x8,    50},
	{0x9,    60},
	{0xb,    70},
	{0x13,	 80},
	{0x1b,   90},
	{0x21,   100},
	/* 64x */
	{0x6,    40},
	{0x9,    50},
	{0xb,    60},
	{0x13,   70},
	{0x1b,	 80},
	{0x21,   90},
	{0x25,   100},
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
#define GAIN_1X_TEMP_OFFSET 15
#define GAIN_OVER_1X_TEMP_OFFSET 15
#define AGAIN_MAX 64
static int sensor_get_temp(struct v4l2_subdev *sd,  struct sensor_temp *temp)
{
	gain_temper_entity gain_low, gain_high, curr_gain_reslut;
	data_type blc_val;
	int blc_val_sum;
	int blc_get_cnt;
	int temper_idx;
	int again_idx;
	int cur_again;
	int eRet;

	blc_val_sum = 0;
	blc_get_cnt = 0;
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
		if (cur_again >= AGAIN_MAX) {
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

	if (cur_again >= AGAIN_MAX) {
		sensor_dbg("special sulo\n");
		temp->temp = gain_low.temper_val + GAIN_OVER_1X_TEMP_OFFSET;
		return 0;
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

	if (cur_again <= 8) {
		/* 1x temper range is too short */
		temp->temp = temp->temp - GAIN_1X_TEMP_OFFSET;
	} else {
		temp->temp = temp->temp + GAIN_OVER_1X_TEMP_OFFSET;
	}
	sensor_dbg("cur_again = %d, blc_val = 0x%x, gain: low(%d, %d), high(%d, %d), numEntries = %d, temp = %d\n",
			cur_again, blc_val, gain_low.again_val, gain_low.temper_val, gain_high.again_val, gain_high.temper_val, numEntries, temp->temp);

	return 0;
}

/*
 *set && get sensor flip
 */
static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	struct sensor_info *info = to_state(sd);

	*code = info->fmt->mbus_code;

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
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
#if IS_ENABLED(CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI)
    unsigned int i2c_addr;
	sensor_i2c_addr_get(sd, &i2c_addr);
#endif

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
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(3000, 3100);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
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
		usleep_range(200000, 220000);
		times_out--;
	} while (eRet < 0 && times_out > 0);

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
#if IS_ENABLED(CONFIG_SENSOR_GC1084_MIPI_USE_SAME_TWI)
	unsigned int iic_addr;
	struct v4l2_subdev *sd_temp;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!info->preview_first_flag) {
		if (cci_drv[0].sd != NULL && cci_drv[1].sd != NULL) {
			if (!strcmp(sd->name, cci_drv[0].name))
				sd_temp = cci_drv[1].sd;
			else if (!strcmp(sd->name, cci_drv[1].name))
				sd_temp = cci_drv[0].sd;
			if (sd_temp->entity.use_count == 0 && sd->entity.use_count == 1) {
				iic_addr = TARGET_I2C_ADDR;
			} else if (sd_temp->entity.use_count != 0) {
				iic_addr = I2C_ADDR;
				sensor_i2c_addr_set(sd, I2C_ADDR);
			}
		} else {
			sensor_i2c_addr_get(sd, &iic_addr);
			sensor_print("[INFO]  ic2 addr client name = [%s] iic_addr = 0x%x\n", client->name, iic_addr);
		}
		sensor_dbg("sensor_init\n");

		if (iic_addr == I2C_ADDR) {
			ret = sensor_detect(sd);
			if (ret) {
				sensor_err("chip found is not an target chip.\n");
				return ret;
			}
		} else if (iic_addr == TARGET_I2C_ADDR) {
			int ret6e = 0;
			int ret7e = 0;
			sensor_print("[INFO] 1. detect 0x%x \n", I2C_ADDR);
			sensor_i2c_addr_set(sd, I2C_ADDR);

			ret6e = sensor_detect(sd);

			if (ret6e) {
				sensor_err("chip found is not an target chip, twi_addr:0x%x.\n", I2C_ADDR);
				return -ENODEV;
			}

			sensor_print("[INFO] 2 doesn't detect 7e, need to change 6e to 7e \n");
			sensor_write(sd, 0x03fb, (TARGET_I2C_ADDR & 0xFF));

			sensor_print("[INFO] 3. detect 0x%x \n", TARGET_I2C_ADDR);
			sensor_i2c_addr_set(sd, TARGET_I2C_ADDR);

			ret7e = sensor_detect(sd);
			if (ret7e) {
				sensor_err("chip found is not an target chip, twi_addr:0x%x.\n", TARGET_I2C_ADDR);
				return -ENODEV;
			}
		}
	} else {
		ret = sensor_detect(sd);
		if (ret) {
			sensor_err("chip found is not an target chip.\n");
			return ret;
		}
	}
#else
	sensor_dbg("sensor_init\n");
	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}
#endif
	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1280;
	info->height = 720;
	info->hflip = 0;
	info->vflip = 0;
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
		sensor_dbg("%s: GET_CURRENT_WIN_CFG, info->current_wins=%p\n", __func__, info->current_wins);

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
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_GET_SENSOR_CODE:
		sensor_get_fmt_mbus_core(sd, (int *)arg);
		break;
	case VIDIOC_VIN_SET_IR:
		sensor_set_ir(sd, (struct ir_switch *)arg);
		break;
	case VIDIOC_VIN_SENSOR_GET_TEMP:
		sensor_get_temp(sd, (struct sensor_temp *)arg);
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
		.desc = "Raw RGB Bayer",
		/* .mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10, */
#if RAW8
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
#else
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
#endif
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
#if SENSOR_720p30
	{
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 750,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (750 -16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_720p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p30_regs),
		.set_size   = NULL,
	},
#endif

#if SENSOR_720p20
	{
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 1125,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1125 -16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_720p20_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p20_regs),
		.set_size   = NULL,
	},
#endif

#if SENSOR_720p15
	{
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 1500,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 15,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1500 -16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_720p15_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p15_regs),
		.set_size   = NULL,
	},
#endif

#if SENSOR_720p10
	{
		.width      = 1280,
		.height     = 720,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2200,
		.vts        = 2250,
		.pclk       = 49500000,
		.mipi_bps   = 396 * 1000 * 1000,
		.fps_fixed  = 10,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (2250 -16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_720p10_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p10_regs),
		.set_size   = NULL,
	},
#endif

#if SENSOR_360p120
	{
		.width      = 640,
		.height     = 360,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 1800,
		.vts        = 750,
		.pclk       = 1620000,
		.mipi_bps   = 648 * 1000 * 1000,
		.fps_fixed  = 120,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (750 -16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 64 << 4,
		.regs       = sensor_360p120_regs,
		.regs_size  = ARRAY_SIZE(sensor_360p120_regs),
		.set_size   = NULL,
	},
#endif
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

__maybe_unused static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif

	return 0;
}

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
#if VIN_FALSE
	struct v4l2_queryctrl qc;
	int ret;
#endif
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;
#if VIN_FALSE
	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);

	if (ret < 0)
		return ret;

	if (ctrl->val < qc.minimum || ctrl->val > qc.maximum)
		return -ERANGE;
#endif

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

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	__maybe_unused struct sensor_exp_gain exp_gain;
	sensor_dbg("sensor_reg_init, ARRAY_SIZE(sensor_default_regs)=%d\n", ARRAY_SIZE(sensor_default_regs));

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init, wsize=%p, wsize->regs=0x%x, wsize->regs_size=%d\n", wsize, wsize->regs, wsize->regs_size);

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

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
			exp_gain.exp_val = 11200;
			exp_gain.gain_val = 32;
		}
		sensor_s_exp_gain(sd, &exp_gain);
		//sensor_write(sd, 0x0100, 0x09); /* stream_on */
	}
#else
	if (wsize->regs) {
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	}
#endif

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	gc1084_sensor_vts = wsize->vts;
	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
			 wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable)
		return 0;

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

/* ----------------------------------------------------------------------- */
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
#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	// use CMB_PHYA_OFFSET2  also ok
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET3 | MIPI_NORMAL_MODE;
	//info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	info->preview_first_flag = 1;
#else
	info->preview_first_flag = 0;
#endif
	info->exp = 0;
	info->gain = 0;
	info->first_power_flag = 1;

	sensor_dbg("sensor 1084 probe\n");

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
