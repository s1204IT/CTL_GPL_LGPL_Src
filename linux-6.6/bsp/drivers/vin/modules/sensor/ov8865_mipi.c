/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ov8865 Raw cameras.
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Yang Feng <yangfeng@allwinnertech.com>
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
MODULE_DESCRIPTION("A low-level driver for OV8865 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.1");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x8865
int ov8865_sensor_vts;

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 15

/*
 * The ov8865 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x20
#define SENSOR_NAME "ov8865_mipi"

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
	{0x0103, 0x01},// ; software reset
	{REG_DLY, 10},
	{0x0100, 0x00},// ; software standby
	{0x0100, 0x00},//
	{0x0100, 0x00},//
	{0x0100, 0x00},//
	{0x3638, 0xff},// ; analog control
	{0x0302, 0x1f},// ; PLL
	{0x0303, 0x00},// ; PLL
	{0x0304, 0x03},// ; PLL
	{0x030d, 0x1f},// ; PLL	  ;1e
	{0x030e, 0x00},// ; PLL
	{0x030f, 0x09},// ; PLL
	{0x0312, 0x01},// ; PLL
	{0x031e, 0x0c},// ; PLL
	{0x3015, 0x01},// ; clock Div

	//{0x3018,0x72},// ; MIPI 4 lane
	{0x3018, 0x32},// ; MIPI 2 lane
	{0x3020, 0x93},// ; clock normal, pclk/1
	{0x3022, 0x01},// ; pd_mini enable when rst_sync
	{0x3031, 0x0a},// ; 10-bit
	{0x3106, 0x01},// ; PLL
	{0x3305, 0xf1},//
	{0x3308, 0x00},//
	{0x3309, 0x28},//
	{0x330a, 0x00},//
	{0x330b, 0x20},//
	{0x330c, 0x00},//
	{0x330d, 0x00},//
	{0x330e, 0x00},//
	{0x330f, 0x40},//
	{0x3307, 0x04},//
	{0x3604, 0x04},// ; analog control
	{0x3602, 0x30},//
	{0x3605, 0x00},//
	{0x3607, 0x20},//
	{0x3608, 0x11},//
	{0x3609, 0x68},//
	{0x360a, 0x40},//
	{0x360c, 0xdd},//
	{0x360e, 0x0c},//
	{0x3610, 0x07},//
	{0x3612, 0x89},//
	{0x3614, 0x28},//
	{0x3617, 0x40},//
	{0x3618, 0x5a},//
	{0x3619, 0x9b},//
	{0x361c, 0x00},//
	{0x361d, 0x60},//
	{0x3631, 0x60},//
	{0x3633, 0x10},//
	{0x3634, 0x10},//
	{0x3635, 0x10},//
	{0x3636, 0x0a},//
	{0x3641, 0x55},// ; MIPI settings
	{0x3646, 0x86},// ; MIPI settings
	{0x3647, 0x27},// ; MIPI settings
	{0x364a, 0x1b},// ; MIPI settings
	{0x3500, 0x00},// ; exposurre HH
	{0x3501, 0x4c},// ; expouere H
	{0x3502, 0x00},// ; exposure L
	{0x3503, 0x00},// ; gain no delay, exposure no delay
	{0x3508, 0x02},// ; gain H
	{0x3509, 0x00},// ; gain L
	{0x3700, 0x18},// ; sensor control
	{0x3701, 0x0c},//
	{0x3702, 0x28},//
	{0x3703, 0x19},//
	{0x3704, 0x14},//
	{0x3705, 0x00},//
	{0x3706, 0x28},//
	{0x3707, 0x04},//
	{0x3708, 0x24},//
	{0x3709, 0x40},//
	{0x370a, 0x00},//
	{0x370b, 0xa8},//
	{0x370c, 0x04},//
	{0x3718, 0x12},//
	{0x3719, 0x31},//
	{0x3712, 0x42},//
	{0x3714, 0x12},//
	{0x371e, 0x19},//
	{0x371f, 0x40},//
	{0x3720, 0x05},//
	{0x3721, 0x05},//
	{0x3724, 0x02},//
	{0x3725, 0x02},//
	{0x3726, 0x06},//
	{0x3728, 0x05},//
	{0x3729, 0x02},//
	{0x372a, 0x03},//
	{0x372b, 0x53},//
	{0x372c, 0xa3},//
	{0x372d, 0x53},//
	{0x372e, 0x06},//
	{0x372f, 0x10},//
	{0x3730, 0x01},//
	{0x3731, 0x06},//
	{0x3732, 0x14},//
	{0x3733, 0x10},//
	{0x3734, 0x40},//
	{0x3736, 0x20},//
	{0x373a, 0x02},//
	{0x373b, 0x0c},//
	{0x373c, 0x0a},//
	{0x373e, 0x03},//
	{0x3755, 0x40},//
	{0x3758, 0x00},//
	{0x3759, 0x4c},//
	{0x375a, 0x06},//
	{0x375b, 0x13},//
	{0x375c, 0x20},//
	{0x375d, 0x02},//
	{0x375e, 0x00},//
	{0x375f, 0x14},//
	{0x3767, 0x04},//
	{0x3768, 0x04},//
	{0x3769, 0x20},//
	{0x376c, 0x00},//
	{0x376d, 0x00},//
	{0x376a, 0x08},//
	{0x3761, 0x00},//
	{0x3762, 0x00},//
	{0x3763, 0x00},//
	{0x3766, 0xff},//
	{0x376b, 0x42},//
	{0x3772, 0x23},//
	{0x3773, 0x02},//
	{0x3774, 0x16},//
	{0x3775, 0x12},//
	{0x3776, 0x08},//
	{0x37a0, 0x44},//
	{0x37a1, 0x3d},//
	{0x37a2, 0x3d},//
	{0x37a3, 0x01},//
	{0x37a4, 0x00},//
	{0x37a5, 0x08},//
	{0x37a6, 0x00},//
	{0x37a7, 0x44},//
	{0x37a8, 0x4c},//
	{0x37a9, 0x4c},//
	{0x3760, 0x00},//
	{0x376f, 0x01},//
	{0x37aa, 0x44},//
	{0x37ab, 0x2e},//
	{0x37ac, 0x2e},//
	{0x37ad, 0x33},//
	{0x37ae, 0x0d},//
	{0x37af, 0x0d},//
	{0x37b0, 0x00},//
	{0x37b1, 0x00},//
	{0x37b2, 0x00},//
	{0x37b3, 0x42},//
	{0x37b4, 0x42},//
	{0x37b5, 0x33},//
	{0x37b6, 0x00},//
	{0x37b7, 0x00},//
	{0x37b8, 0x00},//
	{0x37b9, 0xff},// ; sensor control
	{0x3800, 0x00},// ; X start H
	{0x3801, 0x0c},// ; X start L
	{0x3802, 0x00},// ; Y start H
	{0x3803, 0x0c},// ; Y start L
	{0x3804, 0x0c},// ; X end H
	{0x3805, 0xd3},// ; X end L
	{0x3806, 0x09},// ; Y end H
	{0x3807, 0xa3},// ; Y end L
	{0x3808, 0x06},// ; X output size H
	{0x3809, 0x60},// ; X output size L
	{0x380a, 0x04},// ; Y output size H
	{0x380b, 0xc8},// ; Y output size L
	{0x380c, 0x07},// ; HTS H
	{0x380d, 0x83},// ; HTS L
	{0x380e, 0x04},// ; VTS H
	{0x380f, 0xe0},// ; VTS L
	{0x3810, 0x00},// ; ISP X win H
	{0x3811, 0x04},// ; ISP X win L
	{0x3813, 0x04},// ; ISP Y win L
	{0x3814, 0x03},// ; X inc odd
	{0x3815, 0x01},// ; X inc even
	{0x3820, 0x00},// ; flip off
	{0x3821, 0x67},// ; hsync_en_o, fst_vbin, mirror on
	{0x382a, 0x03},// ; Y inc odd
	{0x382b, 0x01},// ; Y inc even
	{0x3830, 0x08},// ; ablc_use_num[5:1]
	{0x3836, 0x02},// ; zline_use_num[5:1]
	{0x3837, 0x18},// ; vts_add_dis, cexp_gt_vts_offs=8
	{0x3841, 0xff},// ; auto size
	{0x3846, 0x88},// ; Y/X boundary pixel numbber for auto size mode
	{0x3f08, 0x0b},//

	{0x4000, 0xf1},// ; our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001, 0x14},// ; left 32 column, final BLC offset limitation enable
	{0x4005, 0x10},// ; BLC target

	{0x4006, 0x04},// ;revise for ZSD ON/OFF unstable,MTK
	{0x4007, 0x04},// ;

	{0x400b, 0x0c},// ; start line =0, offset limitation en, cut range function en
	{0x400d, 0x10},// ; offset trigger threshold
	{0x401b, 0x00},//
	{0x401d, 0x00},//
	{0x4020, 0x01},// ; anchor left start H
	{0x4021, 0x20},// ; anchor left start L
	{0x4022, 0x01},// ; anchor left end H
	{0x4023, 0x9f},// ; anchor left end L
	{0x4024, 0x03},// ; anchor right start H
	{0x4025, 0xe0},// ; anchor right start L
	{0x4026, 0x04},// ; anchor right end H
	{0x4027, 0x5f},// ; anchor right end L
	{0x4028, 0x00},// ; top zero line start
	{0x4029, 0x02},// ; top zero line number
	{0x402a, 0x04},// ; top black line start
	{0x402b, 0x04},// ; top black line number
	{0x402c, 0x02},// ; bottom zero line start
	{0x402d, 0x02},// ; bottom zero line number
	{0x402e, 0x08},// ; bottom black line start
	{0x402f, 0x02},// ; bottom black line number
	{0x401f, 0x00},// ; anchor one disable
	{0x4034, 0x3f},// ; limitation BLC offset
	{0x4300, 0xff},// ; clip max H
	{0x4301, 0x00},// ; clip min H
	{0x4302, 0x0f},// ; clip min L/clip max L
	{0x4500, 0x40},// ; ADC sync control
	{0x4503, 0x10},//
	{0x4601, 0x74},// ; V FIFO control
	{0x481f, 0x32},// ; clk_prepare_min
	{0x4837, 0x16},// ; clock period
	{0x4850, 0x10},// ; lane select
	{0x4851, 0x32},// ; lane select
	{0x4b00, 0x2a},// ; LVDS settings
	{0x4b0d, 0x00},// ; LVDS settings
	{0x4d00, 0x04},// ; temperature sensor
	{0x4d01, 0x18},// ; temperature sensor
	{0x4d02, 0xc3},// ; temperature sensor
	{0x4d03, 0xff},// ; temperature sensor
	{0x4d04, 0xff},// ; temperature sensor
	{0x4d05, 0xff},// ; temperature sensor
	{0x5000, 0x96},// ; LENC on, MWB on, BPC on, WPC on
	{0x5001, 0x01},// ; BLC on
	{0x5002, 0x08},// ; vario pixel off
	{0x5901, 0x00},//
	{0x5e00, 0x00},// ; test pattern off
	{0x5e01, 0x41},// ; window cut enable
	{0x0100, 0x01},// ; wake up, streaming
	{0x5800, 0x1d},// ; lens correction
	{0x5801, 0x0e},//
	{0x5802, 0x0c},//
	{0x5803, 0x0c},//
	{0x5804, 0x0f},//
	{0x5805, 0x22},//
	{0x5806, 0x0a},//
	{0x5807, 0x06},//
	{0x5808, 0x05},//
	{0x5809, 0x05},//
	{0x580a, 0x07},//
	{0x580b, 0x0a},//
	{0x580c, 0x06},//
	{0x580d, 0x02},//
	{0x580e, 0x00},//
	{0x580f, 0x00},//
	{0x5810, 0x03},//
	{0x5811, 0x07},//
	{0x5812, 0x06},//
	{0x5813, 0x02},//
	{0x5814, 0x00},//
	{0x5815, 0x00},//
	{0x5816, 0x03},//
	{0x5817, 0x07},//
	{0x5818, 0x09},//
	{0x5819, 0x06},//
	{0x581a, 0x04},//
	{0x581b, 0x04},//
	{0x581c, 0x06},//
	{0x581d, 0x0a},//
	{0x581e, 0x19},//
	{0x581f, 0x0d},//
	{0x5820, 0x0b},//
	{0x5821, 0x0b},//
	{0x5822, 0x0e},//
	{0x5823, 0x22},//
	{0x5824, 0x23},//
	{0x5825, 0x28},//
	{0x5826, 0x29},//
	{0x5827, 0x27},//
	{0x5828, 0x13},//
	{0x5829, 0x26},//
	{0x582a, 0x33},//
	{0x582b, 0x32},//
	{0x582c, 0x33},//
	{0x582d, 0x16},//
	{0x582e, 0x14},//
	{0x582f, 0x30},//
	{0x5830, 0x31},//
	{0x5831, 0x30},//
	{0x5832, 0x15},//
	{0x5833, 0x26},//
	{0x5834, 0x23},//
	{0x5835, 0x21},//
	{0x5836, 0x23},//
	{0x5837, 0x05},//
	{0x5838, 0x36},//
	{0x5839, 0x27},//
	{0x583a, 0x28},//
	{0x583b, 0x26},//
	{0x583c, 0x24},//
	{0x583d, 0xdf},// ; lens correction
	//{0x4800,0x5c},// ; mipi gate:lens start/end
};

static struct regval_list sensor_quxga_regs[] = {
	{0x0100, 0x00}, // software standby
	{0x030f, 0x04}, // PLL
	{0x3018, 0x32}, //
	{0x3106, 0x21}, //
	{0x3501, 0x98}, // expouere H
	{0x3502, 0x60}, // exposure L
	{0x3700, 0x24}, // sensor control
	{0x3701, 0x0c}, //
	{0x3702, 0x28}, //
	{0x3703, 0x19}, //
	{0x3704, 0x14}, //
	{0x3706, 0x38}, //
	{0x3707, 0x04}, //
	{0x3708, 0x24}, //
	{0x3709, 0x40}, //
	{0x370a, 0x00}, //
	{0x370b, 0xb8}, //
	{0x370c, 0x04}, //
	{0x3718, 0x12}, //
	{0x3712, 0x42}, //
	{0x371e, 0x19}, //
	{0x371f, 0x40}, //
	{0x3720, 0x05}, //
	{0x3721, 0x05}, //
	{0x3724, 0x02}, //
	{0x3725, 0x02}, //
	{0x3726, 0x06}, //
	{0x3728, 0x05}, //
	{0x3729, 0x02}, //
	{0x372a, 0x03}, //
	{0x372b, 0x53}, //
	{0x372c, 0xa3}, //
	{0x372d, 0x53}, //
	{0x372e, 0x06}, //
	{0x372f, 0x10}, //
	{0x3730, 0x01}, //
	{0x3731, 0x06}, //
	{0x3732, 0x14}, //
	{0x3736, 0x20}, //
	{0x373a, 0x02}, //
	{0x373b, 0x0c}, //
	{0x373c, 0x0a}, //
	{0x373e, 0x03}, //
	{0x375a, 0x06}, //
	{0x375b, 0x13}, //
	{0x375d, 0x02}, //
	{0x375f, 0x14}, //
	{0x3767, 0x1e}, //
	{0x3772, 0x23}, //
	{0x3773, 0x02}, //
	{0x3774, 0x16}, //
	{0x3775, 0x12}, //
	{0x3776, 0x08}, //
	{0x37a0, 0x44}, //
	{0x37a1, 0x3d}, //
	{0x37a2, 0x3d}, //
	{0x37a3, 0x02}, //
	{0x37a5, 0x09}, //
	{0x37a7, 0x44}, //
	{0x37a8, 0x58}, //
	{0x37a9, 0x58}, //
	{0x37aa, 0x44}, //
	{0x37ab, 0x2e}, //
	{0x37ac, 0x2e}, //
	{0x37ad, 0x33}, //
	{0x37ae, 0x0d}, //
	{0x37af, 0x0d}, //
	{0x37b3, 0x42}, //
	{0x37b4, 0x42}, //
	{0x37b5, 0x33}, //
	{0x3808, 0x0c}, // X output size H
	{0x3809, 0xc0}, // X output size L
	{0x380a, 0x09}, // Y output size H
	{0x380b, 0x90}, // Y output size L
	{0x380c, 0x07}, // HTS H
	{0x380d, 0x98}, // HTS L
	{0x380e, 0x09}, // VTS H
	{0x380f, 0xa6}, // VTS L
	{0x3813, 0x02}, // ISP Y win L
	{0x3814, 0x01}, // X inc odd
	{0x3821, 0x46}, // hsync_en_o, fst_vbin, mirror on
	{0x382a, 0x01}, // Y inc odd
	{0x382b, 0x01}, // Y inc even
	{0x3830, 0x04}, // ablc_use_num[5:1]
	{0x3836, 0x01}, // zline_use_num[5:1]
	{0x3846, 0x48}, // Y/X boundary pixel numbber for auto size mode
	{0x3f08, 0x0b}, //
	{0x4000, 0xf1}, // our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001, 0x04}, // left 32 column, final BLC offset limitation enable
	{0x4020, 0x02}, // anchor left start H
	{0x4021, 0x40}, // anchor left start L
	{0x4022, 0x03}, // anchor left end H
	{0x4023, 0x3f}, // anchor left end L
	{0x4024, 0x07}, // anchor right start H
	{0x4025, 0xc0}, // anchor right start L
	{0x4026, 0x08}, // anchor right end H
	{0x4027, 0xbf}, // anchor right end L
	{0x402a, 0x04}, // top black line start
	{0x402b, 0x04}, // top black line number
	{0x402c, 0x02}, // bottom zero line start
	{0x402d, 0x02}, // bottom zero line number
	{0x402e, 0x08}, // bottom black line start
	{0x4500, 0x68}, // ADC sync control
	{0x4601, 0x10}, // V FIFO control
	{0x5002, 0x08}, // vario pixel off
	{0x5901, 0x00}, //
	//{0x0100, 0x01}, // wake up, streaming
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)((0x0000ff & exp_val));

	sensor_write(sd, 0x3502, explow);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3500, exphigh);

	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	unsigned char gainlow = 0;
	unsigned char gainhigh = 0;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	/*
	if (gain_val < 2 * 16 * 8) {
		gainhigh = 0;
		gainlow = gain_val;
	} else if (2 * 16 * 8 <= gain_val && gain_val < 4 * 16 * 8) {
		gainhigh = 1;
		gainlow = gain_val / 2 - 8;
	} else if (4 * 16 * 8 <= gain_val && gain_val < 8 * 16 * 8) {
		gainhigh = 3;
		gainlow = gain_val / 4 - 12;
	} else {
		gainhigh = 7;
		gainlow = gain_val / 8 - 8;
	}
	*/

	gain_val *= 8;
	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x7);

	sensor_write(sd, 0x3509, gainlow);
	sensor_write(sd, 0x3508, gainhigh);

	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, frame_length, shutter;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val>>4;
	if (shutter > ov8865_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = ov8865_sensor_vts;

	sensor_write(sd, 0x3208, 0x00);
	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);
	info->exp = exp_val;
	info->gain = gain_val;

	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == CSI_GPIO_LOW) {
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
	} else {
		ret = sensor_write(sd, 0x0100, rdval | 0x01);
	}
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;
	switch (on) {
	case STBY_ON:
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_HIGH);
		cci_unlock(sd);
		break;
	case PWR_ON:
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, DVDD, ON);

		usleep_range(11000, 13000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);

		vin_set_pmu_channel(sd, DVDD, OFF);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		// vin_gpio_set_status(sd, RESET, 0);
		// vin_gpio_set_status(sd, PWDN, 0);
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
	data_type rdval;
	int ret = 0;

	ret = sensor_read(sd, 0x300b, &rdval);
	sensor_print("read reg 0x300b = 0x%x \n", rdval);
	if (ret < 0 || rdval != (V4L2_IDENT_SENSOR >> 8)) {
		sensor_err("sensor read reg 0x300b err! ret = %d, rdval = %d\n", ret, rdval);
		return -ENODEV;
	}

	ret = sensor_read(sd, 0x300c, &rdval);
	sensor_print("read reg 0x300c = 0x%x \n", rdval);
	if (ret < 0 || rdval != (V4L2_IDENT_SENSOR & 0xff)) {
		sensor_err("sensor read reg 0x300c err! ret = %d, rdval = %d\n", ret, rdval);
		return -ENODEV;
	}

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);


	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = QUXGA_WIDTH;
	info->height = QUXGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 15;	/* 30fps */

	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
				   info->current_wins,
				   sizeof(*info->current_wins));
			ret = 0;
		} else {
			ret = -1;
		}
		break;
	case SET_FPS:
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
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
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
	/* quxga: 3264*2448 */
	{
	 .width = QUXGA_WIDTH,
	 .height = QUXGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1940,
	 .vts = 2474,
	 .pclk = 144 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 15,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (2474 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 15 << 4,
	 .regs = sensor_quxga_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_regs),
	 .set_size = NULL,
	 },

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{

	int ret = 0;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(sd, sensor_default_regs,
			       ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		return ret;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	ov8865_sensor_vts = wsize->vts;
	exp_gain.exp_val = info->exp;
	exp_gain.gain_val = info->gain;
	if (exp_gain.exp_val == 0 || exp_gain.gain_val == 0) {
		exp_gain.exp_val = 16000;
		exp_gain.gain_val = 64;
	}
	sensor_s_exp_gain(sd, &exp_gain);
	sensor_write(sd, 0x0100, 0x01);
	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif

	return 0;
}

static int __attribute__((unused)) sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 64 * 16 - 1, 1, 1 * 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535 * 16, 1, 0);
	case V4L2_CID_FRAME_RATE:
		return v4l2_ctrl_query_fill(qc, 15, 120, 1, 120);
	}
	return -EINVAL;
}
static int __attribute__((unused))  sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int __attribute__((unused)) sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		return -ERANGE;
	}

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	//.g_ctrl = sensor_g_ctrl,
	//.s_ctrl = sensor_s_ctrl,
	//.queryctrl = sensor_queryctrl,
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
//	.g_mbus_config = sensor_g_mbus_config,
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
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
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;

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
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
