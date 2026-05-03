/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for GC08A3 Raw cameras.
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
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

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for gc08a3 sensors");
MODULE_LICENSE("GPL");

#define MCLK              	(24*1000*1000)

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE	(30)

/*
 * The gc08a3 i2c address
 */
/* confirm with modulehouse first! 0x62/0x20/0x22/0x24*/
#define I2C_ADDR 			(0x62)

#define SENSOR_NAME 		"gc08a3_mipi"

/**
  * @brief gc08a3 sensor private param definition
  */
static int gc08a3_sensor_vts;

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {
	/*system*/
	{0x031c, 0x60},
	{0x0337, 0x04},
	{0x0335, 0x51},
	{0x0336, 0x70},
	{0x0383, 0xbb},
	{0x031a, 0x00},
	{0x0321, 0x10},
	{0x0327, 0x03},
	{0x0325, 0x40},
	{0x0326, 0x23},
	{0x0314, 0x11},
	{0x0315, 0xd6},
	{0x0316, 0x01},
	{0x0334, 0x40},
	{0x0324, 0x42},
	{0x031c, 0x00},
	{0x031c, 0x9f},
	{0x039a, 0x13},
	{0x0084, 0x30},
	{0x02b3, 0x08},
	{0x0057, 0x0c},
	{0x05c3, 0x50},
	{0x0311, 0x90},
	{0x05a0, 0x02},
	{0x0074, 0x0a},
	{0x0059, 0x11},
	{0x0070, 0x05},
	{0x0101, 0x00},
	/*analog*/
	{0x0344, 0x00},
	{0x0345, 0x06},
	{0x0346, 0x00},
	{0x0347, 0x04},
	{0x0348, 0x0c},
	{0x0349, 0xd0},
	{0x034a, 0x09},
	{0x034b, 0x9c},
	{0x0202, 0x09},
	{0x0203, 0x04},
	{0x0340, 0x09},
	{0x0341, 0xf4},
	{0x0342, 0x07},
	{0x0343, 0x1c},
	{0x0219, 0x05},
	{0x0226, 0x00},
	{0x0227, 0x28},
	{0x0e0a, 0x00},
	{0x0e0b, 0x00},
	{0x0e24, 0x04},
	{0x0e25, 0x04},
	{0x0e26, 0x00},
	{0x0e27, 0x10},
	{0x0e01, 0x74},
	{0x0e03, 0x47},
	{0x0e04, 0x33},
	{0x0e05, 0x44},
	{0x0e06, 0x44},
	{0x0e0c, 0x1e},
	{0x0e17, 0x3a},
	{0x0e18, 0x3c},
	{0x0e19, 0x40},
	{0x0e1a, 0x42},
	{0x0e28, 0x21},
	{0x0e2b, 0x68},
	{0x0e2c, 0x0d},
	{0x0e2d, 0x08},
	{0x0e34, 0xf4},
	{0x0e35, 0x44},
	{0x0e36, 0x07},
	{0x0e38, 0x49},
	{0x0210, 0x13},
	{0x0218, 0x00},
	{0x0241, 0x88},
	{0x0e32, 0x00},
	{0x0e33, 0x18},
	{0x0e42, 0x03},
	{0x0e43, 0x80},
	{0x0e44, 0x04},
	{0x0e45, 0x00},
	{0x0e4f, 0x04},
	{0x057a, 0x20},
	{0x0381, 0x7c},
	{0x0382, 0x9b},
	{0x0384, 0xfb},
	{0x0389, 0x38},
	{0x038a, 0x03},
	{0x0390, 0x6a},
	{0x0391, 0x0b},
	{0x0392, 0x60},
	{0x0393, 0xc1},
	{0x0396, 0xff},
	{0x0398, 0x62},
	/*cisctl reset*/
	{0x031c, 0x80},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x031c, 0x9f},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x031c, 0x80},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x031c, 0x9f},
	{0x0360, 0x01},
	{0x0360, 0x00},
	{0x0316, 0x09},
	{0x0a67, 0x80},
	{0x0313, 0x00},
	{0x0a53, 0x0e},
	{0x0a65, 0x17},
	{0x0a68, 0xa1},
	{0x0a58, 0x00},
	{0x0ace, 0x0c},
	{0x00a4, 0x00},
	{0x00a5, 0x01},
	{0x00a7, 0x09},
	{0x00a8, 0x9c},
	{0x00a9, 0x0c},
	{0x00aa, 0xd0},
	{0x0a8a, 0x00},
	{0x0a8b, 0xe0},
	{0x0a8c, 0x13},
	{0x0a8d, 0xe8},
	{0x0a90, 0x0a},
	{0x0a91, 0x10},
	{0x0a92, 0xf8},
	{0x0a71, 0xf2},
	{0x0a72, 0x12},
	{0x0a73, 0x64},
	{0x0a75, 0x41},
	{0x0a70, 0x07},
	{0x0313, 0x80},
	/*ISP*/
	{0x00a0, 0x01},
	{0x0080, 0xd2},
	{0x0081, 0x3f},
	{0x0087, 0x51},
	{0x0089, 0x03},
	{0x009b, 0x40},
	{0x05a0, 0x82},
	{0x05ac, 0x00},
	{0x05ad, 0x01},
	{0x05ae, 0x00},
	{0x0800, 0x0a},
	{0x0801, 0x14},
	{0x0802, 0x28},
	{0x0803, 0x34},
	{0x0804, 0x0e},
	{0x0805, 0x33},
	{0x0806, 0x03},
	{0x0807, 0x8a},
	{0x0808, 0x50},
	{0x0809, 0x00},
	{0x080a, 0x34},
	{0x080b, 0x03},
	{0x080c, 0x26},
	{0x080d, 0x03},
	{0x080e, 0x18},
	{0x080f, 0x03},
	{0x0810, 0x10},
	{0x0811, 0x03},
	{0x0812, 0x00},
	{0x0813, 0x00},
	{0x0814, 0x01},
	{0x0815, 0x00},
	{0x0816, 0x01},
	{0x0817, 0x00},
	{0x0818, 0x00},
	{0x0819, 0x0a},
	{0x081a, 0x01},
	{0x081b, 0x6c},
	{0x081c, 0x00},
	{0x081d, 0x0b},
	{0x081e, 0x02},
	{0x081f, 0x00},
	{0x0820, 0x00},
	{0x0821, 0x0c},
	{0x0822, 0x02},
	{0x0823, 0xd9},
	{0x0824, 0x00},
	{0x0825, 0x0d},
	{0x0826, 0x03},
	{0x0827, 0xf0},
	{0x0828, 0x00},
	{0x0829, 0x0e},
	{0x082a, 0x05},
	{0x082b, 0x94},
	{0x082c, 0x09},
	{0x082d, 0x6e},
	{0x082e, 0x07},
	{0x082f, 0xe6},
	{0x0830, 0x10},
	{0x0831, 0x0e},
	{0x0832, 0x0b},
	{0x0833, 0x2c},
	{0x0834, 0x14},
	{0x0835, 0xae},
	{0x0836, 0x0f},
	{0x0837, 0xc4},
	{0x0838, 0x18},
	{0x0839, 0x0e},
	{0x05ac, 0x01},
	{0x059a, 0x00},
	{0x059b, 0x00},
	{0x059c, 0x01},
	{0x0598, 0x00},
	{0x0597, 0x14},
	{0x05ab, 0x09},
	{0x05a4, 0x02},
	{0x05a3, 0x05},
	{0x05a0, 0xc2},
	{0x0207, 0xc4},
	/*GAIN*/
	{0x0208, 0x01},
	{0x0209, 0x72},
	{0x0204, 0x04},
	{0x0205, 0x00},
	{0x0040, 0x22},
	{0x0041, 0x20},
	{0x0043, 0x10},
	{0x0044, 0x00},
	{0x0046, 0x08},
	{0x0047, 0xf0},
	{0x0048, 0x0f},
	{0x004b, 0x0f},
	{0x004c, 0x00},
	{0x0050, 0x5c},
	{0x0051, 0x44},
	{0x005b, 0x03},
	{0x00c0, 0x00},
	{0x00c1, 0x80},
	{0x00c2, 0x31},
	{0x00c3, 0x00},
	{0x0460, 0x04},
	{0x0462, 0x08},
	{0x0464, 0x0e},
	{0x0466, 0x0a},
	{0x0468, 0x12},
	{0x046a, 0x12},
	{0x046c, 0x10},
	{0x046e, 0x0c},
	{0x0461, 0x03},
	{0x0463, 0x03},
	{0x0465, 0x03},
	{0x0467, 0x03},
	{0x0469, 0x04},
	{0x046b, 0x04},
	{0x046d, 0x04},
	{0x046f, 0x04},
	{0x0470, 0x04},
	{0x0472, 0x10},
	{0x0474, 0x26},
	{0x0476, 0x38},
	{0x0478, 0x20},
	{0x047a, 0x30},
	{0x047c, 0x38},
	{0x047e, 0x60},
	{0x0471, 0x05},
	{0x0473, 0x05},
	{0x0475, 0x05},
	{0x0477, 0x05},
	{0x0479, 0x04},
	{0x047b, 0x04},
	{0x047d, 0x04},
	{0x047f, 0x04},
};

static struct regval_list sensor_3264x2448p30_regs[] = {
	{0x031c, 0x60},
	{0x0337, 0x04},
	{0x0335, 0x51},
	{0x0336, 0x70},
	{0x0383, 0xbb},
	{0x031a, 0x00},
	{0x0321, 0x10},
	{0x0327, 0x03},
	{0x0325, 0x40},
	{0x0326, 0x23},
	{0x0314, 0x11},
	{0x0315, 0xd6},
	{0x0316, 0x01},
	{0x0334, 0x40},
	{0x0324, 0x42},
	{0x031c, 0x00},
	{0x031c, 0x9f},
	{0x039a, 0x13},
	{0x0084, 0x30},
	{0x02b3, 0x08},
	{0x0057, 0x0c},
	{0x05c3, 0x50},
	{0x0311, 0x90},
	{0x05a0, 0x02},
	{0x0074, 0x0a},
	{0x0059, 0x11},
	{0x0070, 0x05},
	{0x0101, 0x00},
	/*analog*/
	{0x0344, 0x00},
	{0x0345, 0x06},
	{0x0346, 0x00},
	{0x0347, 0x04},
	{0x0348, 0x0c},
	{0x0349, 0xd0},
	{0x034a, 0x09},
	{0x034b, 0x9c},
	{0x0202, 0x09},
	{0x0203, 0x04},
	{0x0340, 0x09},
	{0x0341, 0xf4},
	{0x0342, 0x07},
	{0x0343, 0x1c},
	{0x0219, 0x05},
	{0x0226, 0x00},
	{0x0227, 0x28},
	{0x0e0a, 0x00},
	{0x0e0b, 0x00},
	{0x0e24, 0x04},
	{0x0e25, 0x04},
	{0x0e26, 0x00},
	{0x0e27, 0x10},
	{0x0e01, 0x74},
	{0x0e03, 0x47},
	{0x0e04, 0x33},
	{0x0e05, 0x44},
	{0x0e06, 0x44},
	{0x0e0c, 0x1e},
	{0x0e17, 0x3a},
	{0x0e18, 0x3c},
	{0x0e19, 0x40},
	{0x0e1a, 0x42},
	{0x0e28, 0x21},
	{0x0e2b, 0x68},
	{0x0e2c, 0x0d},
	{0x0e2d, 0x08},
	{0x0e34, 0xf4},
	{0x0e35, 0x44},
	{0x0e36, 0x07},
	{0x0e38, 0x49},
	{0x0210, 0x13},
	{0x0218, 0x00},
	{0x0241, 0x88},
	{0x0e32, 0x00},
	{0x0e33, 0x18},
	{0x0e42, 0x23},
	{0x0e43, 0x80},
	{0x0e44, 0x04},
	{0x0e45, 0x00},
	{0x0e4f, 0x04},
	{0x057a, 0x20},
	{0x0381, 0x7c},
	{0x0382, 0x9b},
	{0x0384, 0xfb},
	{0x0389, 0x38},
	{0x038a, 0x03},
	{0x0390, 0x6a},
	{0x0391, 0x0b},
	{0x0392, 0x60},
	{0x0393, 0xc1},
	{0x0396, 0xff},
	{0x0398, 0x62},
	{0x031c, 0x80},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x031c, 0x9f},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x03fe, 0x00},
	{0x031c, 0x80},
	{0x03fe, 0x10},
	{0x03fe, 0x00},
	{0x031c, 0x9f},
	{0x0360, 0x01},
	{0x0360, 0x00},
	{0x0316, 0x09},
	{0x0a67, 0x80},
	{0x0313, 0x00},
	{0x0a53, 0x0e},
	{0x0a65, 0x17},
	{0x0a68, 0xa1},
	{0x0a58, 0x00},
	{0x0ace, 0x0c},
	{0x00a4, 0x00},
	{0x00a5, 0x01},
	{0x00a2, 0x00},
	{0x00a3, 0x00},
	{0x00ab, 0x00},
	{0x00ac, 0x00},
	{0x00a7, 0x09},
	{0x00a8, 0x9c},
	{0x00a9, 0x0c},
	{0x00aa, 0xd0},
	{0x0a8a, 0x00},
	{0x0a8b, 0xe0},
	{0x0a8c, 0x13},
	{0x0a8d, 0xe8},
	{0x0a90, 0x0a},
	{0x0a91, 0x10},
	{0x0a92, 0xf8},
	{0x0a71, 0xf2},
	{0x0a72, 0x12},
	{0x0a73, 0x64},
	{0x0a75, 0x41},
	{0x0a70, 0x07},
	{0x0313, 0x80},
	/*ISP*/
	{0x00a0, 0x01},
	{0x0080, 0xd2},
	{0x0081, 0x3f},
	{0x0087, 0x51},
	{0x0089, 0x03},
	{0x009b, 0x40},
	{0x05a0, 0x82},
	{0x05ac, 0x00},
	{0x05ad, 0x01},
	{0x05ae, 0x00},
	{0x0800, 0x0a},
	{0x0801, 0x14},
	{0x0802, 0x28},
	{0x0803, 0x34},
	{0x0804, 0x0e},
	{0x0805, 0x33},
	{0x0806, 0x03},
	{0x0807, 0x8a},
	{0x0808, 0x4c},
	{0x0809, 0x00},
	{0x080a, 0x32},
	{0x080b, 0x03},
	{0x080c, 0x24},
	{0x080d, 0x03},
	{0x080e, 0x14},
	{0x080f, 0x03},
	{0x0810, 0x0c},
	{0x0811, 0x03},
	{0x0812, 0x00},
	{0x0813, 0x00},
	{0x0814, 0x01},
	{0x0815, 0x00},
	{0x0816, 0x01},
	{0x0817, 0x00},
	{0x0818, 0x00},
	{0x0819, 0x0a},
	{0x081a, 0x01},
	{0x081b, 0x6c},
	{0x081c, 0x00},
	{0x081d, 0x0b},
	{0x081e, 0x02},
	{0x081f, 0x00},
	{0x0820, 0x00},
	{0x0821, 0x0c},
	{0x0822, 0x02},
	{0x0823, 0xd9},
	{0x0824, 0x00},
	{0x0825, 0x0d},
	{0x0826, 0x03},
	{0x0827, 0xf0},
	{0x0828, 0x00},
	{0x0829, 0x0e},
	{0x082a, 0x05},
	{0x082b, 0x94},
	{0x082c, 0x09},
	{0x082d, 0x6e},
	{0x082e, 0x07},
	{0x082f, 0xe6},
	{0x0830, 0x10},
	{0x0831, 0x0e},
	{0x0832, 0x0b},
	{0x0833, 0x2c},
	{0x0834, 0x14},
	{0x0835, 0xae},
	{0x0836, 0x0f},
	{0x0837, 0xc4},
	{0x0838, 0x18},
	{0x0839, 0x0e},
	{0x05ac, 0x01},
	{0x059a, 0x00},
	{0x059b, 0x00},
	{0x059c, 0x01},
	{0x0598, 0x00},
	{0x0597, 0x14},
	{0x05ab, 0x09},
	{0x05a4, 0x02},
	{0x05a3, 0x05},
	{0x05a0, 0xc2},
	{0x0207, 0xc4},
	/*GAIN*/
	{0x0208, 0x01},
	{0x0209, 0x72},
	{0x0204, 0x04},
	{0x0205, 0x00},
	{0x0040, 0x22},
	{0x0041, 0x20},
	{0x0043, 0x10},
	{0x0044, 0x00},
	{0x0046, 0x08},
	{0x0047, 0xf0},
	{0x0048, 0x0f},
	{0x004b, 0x0f},
	{0x004c, 0x00},
	{0x0050, 0x5c},
	{0x0051, 0x44},
	{0x005b, 0x03},
	{0x00c0, 0x00},
	{0x00c1, 0x80},
	{0x00c2, 0x31},
	{0x00c3, 0x00},
	{0x0460, 0x04},
	{0x0462, 0x08},
	{0x0464, 0x0e},
	{0x0466, 0x0a},
	{0x0468, 0x12},
	{0x046a, 0x12},
	{0x046c, 0x10},
	{0x046e, 0x0c},
	{0x0461, 0x03},
	{0x0463, 0x03},
	{0x0465, 0x03},
	{0x0467, 0x03},
	{0x0469, 0x04},
	{0x046b, 0x04},
	{0x046d, 0x04},
	{0x046f, 0x04},
	{0x0470, 0x04},
	{0x0472, 0x10},
	{0x0474, 0x26},
	{0x0476, 0x38},
	{0x0478, 0x20},
	{0x047a, 0x30},
	{0x047c, 0x38},
	{0x047e, 0x60},
	{0x0471, 0x05},
	{0x0473, 0x05},
	{0x0475, 0x05},
	{0x0477, 0x05},
	{0x0479, 0x04},
	{0x047b, 0x04},
	{0x047d, 0x04},
	{0x047f, 0x04},
	/*out window*/
	{0x009a, 0x00},
	{0x0351, 0x00},
	{0x0352, 0x06},
	{0x0353, 0x00},
	{0x0354, 0x08},
	{0x034c, 0x0c},
	{0x034d, 0xc0},
	{0x034e, 0x09},
	{0x034f, 0x90},
	/*MIPI*/
	{0x0114, 0x03},
	{0x0180, 0x67},
	{0x0181, 0xf0},
	{0x0185, 0x01},
	{0x0115, 0x32},
	{0x011b, 0x12},
	{0x011c, 0x12},
	{0x0121, 0x06},
	{0x0122, 0x06},
	{0x0123, 0x15},
	{0x0124, 0x01},
	{0x0125, 0x0b},
	{0x0126, 0x08},
	{0x0129, 0x06},
	{0x012a, 0x08},
	{0x012b, 0x08},
	{0x0a73, 0x60},
	{0x0a70, 0x11},
	{0x0313, 0x80},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0aff, 0x00},
	{0x0a70, 0x00},
	{0x00a4, 0x80},
	{0x0316, 0x01},
	{0x0a67, 0x00},
	{0x0084, 0x10},
	{0x0102, 0x09},
	{0x0100, 0x01},
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */
static struct regval_list sensor_fmt_raw[] = {
};

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */
static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);

	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, exphigh;
	unsigned int all_exp;
	struct sensor_info *info = to_state(sd);

	all_exp = exp_val >> 4;
	if (all_exp > 0xffee)
		all_exp = 0xffee;
	if (all_exp < 2)
		all_exp = 2;

	exphigh  = (unsigned char) ((all_exp >> 8) & 0xff);
	explow  = (unsigned char) (all_exp & 0xff);
	sensor_write(sd, 0x0202, exphigh);
	sensor_write(sd, 0x0203, explow);
	sensor_dbg("sensor_set_exp = %d, Done!\n", exp_val);
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

static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	struct sensor_info *info = to_state(sd);
	unsigned int reg_gain;

	reg_gain = gain_val < 16 ? 0x400 : gain_val << 6;
	sensor_write(sd, 0x0204, reg_gain >> 8);
	sensor_write(sd, 0x0205, reg_gain & 0xff);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter = 0, frame_length = 0;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 16)
		gain_val = 16;
	if (gain_val > 64*16-1)
		gain_val = 64*16-1;
	if (exp_val > 0xffee)
		exp_val = 0xffee;
	if (exp_val < 2)
		exp_val = 2;

	shutter = exp_val >> 4;

	if (shutter > gc08a3_sensor_vts - 4) {
		frame_length = shutter + 4;
	} else {
		frame_length = gc08a3_sensor_vts;
	}

	//Write vts
	sensor_write(sd, 0x0340, frame_length >> 8);
	sensor_write(sd, 0x0341, frame_length & 0xff);

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_dbg("sensor_set_gain exp = %d, gain = %d Done!\n", exp_val, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;

	return 0;
}

static void sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_print("STBY_ON!\n");
		cci_lock(sd);
		sensor_s_sw_stby(sd, STBY_ON);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_print("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		sensor_s_sw_stby(sd, STBY_OFF);
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(200, 220);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(300, 310);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);/*AFVCC ON*/
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(100, 120);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, OFF);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);/*AFVCC ON*/
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	unsigned int SENSOR_ID = 0;
	data_type val;
	int cnt = 0;

	sensor_read(sd, 0x03f0, &val);
	SENSOR_ID |= (val << 8);
	sensor_read(sd, 0x03f1, &val);
	SENSOR_ID |= (val);
	sensor_print("gc08a3 detect V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);
	return 0;
	while ((SENSOR_ID != 0x08a3) && (cnt < 5)) {
		sensor_read(sd, 0x03f0, &val);
		SENSOR_ID |= (val << 8);
		sensor_read(sd, 0x03f1, &val);
		SENSOR_ID |= (val);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt, SENSOR_ID);
		cnt++;
	}

	if (SENSOR_ID != 0x08a3)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_print("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 3264;
	info->height = 2448;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30; /* 30fps */

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
		ret = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_ACT_INIT:
		ret = actuator_init(sd, (struct actuator_para *)arg);
		break;
	case VIDIOC_VIN_ACT_SET_CODE:
		ret = actuator_set_code(sd, (struct actuator_ctrl *)arg);
		break;
	case VIDIOC_VIN_FLASH_EN:
		ret = flash_en(sd, (struct flash_para *)arg);
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
		//.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
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
	{
		.width      = 3264,
		.height     = 2448,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3640,
		.vts        = 2548,
		.pclk       = 280*1000*1000,
		.mipi_bps   = 672*1000*1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 2<<4,
		.intg_max   = 2400<<4,
		.gain_min   = 1<<4,
		.gain_max   = 32<<4,
		.regs       = sensor_3264x2448p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_3264x2448p30_regs),
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				unsigned int pad, struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
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
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
				ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_print("sensor_reg_init\n");
	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	gc08a3_sensor_vts = wsize->vts;

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
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
	//.s_parm = sensor_s_parm,
	//.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
				256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
				65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	mutex_init(&info->lock);
	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);

#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->af_first_flag = 1;
	info->exp = 10000;
	info->gain = 1024;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sensor_remove(struct i2c_client *client)
#else
static void sensor_remove(struct i2c_client *client)
#endif
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);

	kfree(to_state(sd));
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = SENSOR_NAME,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	int ret = 0;

	ret = cci_dev_init_helper(&sensor_driver);

	return ret;
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
