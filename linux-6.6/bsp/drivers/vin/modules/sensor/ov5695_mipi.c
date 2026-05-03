/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ov5695 Raw cameras.
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
MODULE_DESCRIPTION("A low-level driver for OV5695 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.1");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x5695

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The GC0310 i2c address
 */
#define I2C_ADDR 0x6c

#define SENSOR_NAME "ov5695_mipi"


/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {
	/*2lane initial*/
	/*Slave_ID=0x6c*/

	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x0300, 0x04},
	{0x0301, 0x00},
	{0x0302, 0x69},
	{0x0303, 0x00},
	{0x0304, 0x00},
	{0x0305, 0x01},
	{0x0307, 0x00},
	{0x030b, 0x00},
	{0x030c, 0x00},
	{0x030d, 0x1e},
	{0x030e, 0x04},
	{0x030f, 0x03},
	{0x0312, 0x01},
	{0x3000, 0x00},
	{0x3002, 0x21},
	{0x3016, 0x32},
	{0x3022, 0x51},
	{0x3106, 0x15},
	{0x3107, 0x01},
	{0x3108, 0x05},
	{0x3500, 0x00},
	{0x3501, 0x45},
	{0x3502, 0x00},
	{0x3503, 0x08},
	{0x3504, 0x03},
	{0x3505, 0x8c},
	{0x3507, 0x03},
	{0x3508, 0x00},
	{0x3509, 0x10},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3601, 0x55},
	{0x3602, 0x58},
	{0x3614, 0x30},
	{0x3611, 0x54},
	{0x3615, 0x77},
	{0x3621, 0x08},
	{0x3624, 0x40},
	{0x3633, 0x0c},
	{0x3634, 0x0c},
	{0x3635, 0x0c},
	{0x3636, 0x0c},
	{0x3638, 0x00},
	{0x3639, 0x00},
	{0x363a, 0x00},
	{0x363b, 0x00},
	{0x363c, 0xff},
	{0x363d, 0xfa},
	{0x3650, 0x44},
	{0x3651, 0x44},
	{0x3652, 0x44},
	{0x3653, 0x44},
	{0x3654, 0x44},
	{0x3655, 0x44},
	{0x3656, 0x44},
	{0x3657, 0x44},
	{0x3660, 0x00},
	{0x3661, 0x00},
	{0x3662, 0x00},
	{0x366a, 0x00},
	{0x366e, 0x0c},
	{0x3673, 0x04},
	{0x3700, 0x14},
	{0x3703, 0x0c},
	{0x3706, 0x31},
	{0x3714, 0x27},
	{0x3715, 0x01},
	{0x3716, 0x00},
	{0x3717, 0x02},
	{0x3733, 0x10},
	{0x3734, 0x40},
	{0x373f, 0xa0},
	{0x3765, 0x20},
	{0x37a1, 0x1d},
	{0x37a8, 0x26},
	{0x37ab, 0x14},
	{0x37c2, 0x04},
	{0x37c3, 0xf0},
	{0x37cb, 0x09},
	{0x37cc, 0x13},
	{0x37cd, 0x1f},
	{0x37ce, 0x1f},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0xaf},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x02},
	{0x380d, 0xe4},
	{0x380e, 0x07},
	{0x380f, 0xe8},
	{0x3810, 0x00},
	{0x3811, 0x00},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3814, 0x03},
	{0x3815, 0x01},
	{0x3816, 0x03},
	{0x3817, 0x01},
	{0x3818, 0x00},
	{0x3819, 0x00},
	{0x381a, 0x00},
	{0x381b, 0x01},
	{0x3820, 0x88},
	{0x3821, 0x88},
	{0x3c80, 0x08},
	{0x3c82, 0x00},
	{0x3c83, 0x00},
	{0x3c88, 0x00},
	{0x3d85, 0x14},
	{0x3f02, 0x08},
	{0x3f03, 0x10},
	{0x4008, 0x02},
	{0x4009, 0x09},
	{0x404e, 0x20},
	{0x4501, 0x00},
	{0x4502, 0x10},
	{0x4800, 0x00},
	{0x481f, 0x2a},
	{0x4837, 0x13},
	{0x5000, 0x13},
	{0x5780, 0x3e},
	{0x5781, 0x0f},
	{0x5782, 0x44},
	{0x5783, 0x02},
	{0x5784, 0x01},
	{0x5785, 0x01},
	{0x5786, 0x00},
	{0x5787, 0x04},
	{0x5788, 0x02},
	{0x5789, 0x0f},
	{0x578a, 0xfd},
	{0x578b, 0xf5},
	{0x578c, 0xf5},
	{0x578d, 0x03},
	{0x578e, 0x08},
	{0x578f, 0x0c},
	{0x5790, 0x08},
	{0x5791, 0x06},
	{0x5792, 0x00},
	{0x5793, 0x52},
	{0x5794, 0xa3},
	{0x5b00, 0x00},
	{0x5b01, 0x1c},
	{0x5b02, 0x00},
	{0x5b03, 0x7f},
	{0x5b05, 0x6c},
	{0x5e10, 0xfc},
	{0x4010, 0xf1},
	{0x3503, 0x08},
	{0x3505, 0x8c},
	{0x3507, 0x03},
	{0x3508, 0x00},
	{0x3509, 0xf8},
	{0x0100, 0x01},

};
static struct regval_list sensor_qsxga_regs[] = {
	/*2592x1944 15fps 2 lane MIPI 420Mbps/lane */
	{0x0100, 0x00},
	{0x3501, 0x7e},
	{0x366e, 0x18},
	{0x3714, 0x23},
	{0x37c2, 0x04},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0xab},
	{0x3808, 0x0a},
	{0x3809, 0x20},
	{0x380a, 0x07},
	{0x380b, 0x98},
	{0x380c, 0x02},
	{0x380d, 0xe4},
	{0x380e, 0x07},
	{0x380f, 0xe8},
	{0x3811, 0x06},
	{0x3813, 0x08},
	{0x3814, 0x01},
	{0x3816, 0x01},
	{0x3817, 0x01},
	{0x3820, 0x88},
	{0x3821, 0x88},
	{0x4501, 0x00},
	{0x4008, 0x04},
	{0x4009, 0x13},
	{0x0100, 0x01},

};

/* 1280x720 30fps 2 lane MIPI 420Mbps/lane */
/*
static struct regval_list sensor_720p_regs[] = {
	{0x0100, 0x00},
	{0x3501, 0x2d},
	{0x3502, 0xc0},
	{0x3708, 0x66},
	{0x3709, 0x52},
	{0x370c, 0xcf},
	{0x3800, 0x00},
	{0x3801, 0x10},
	{0x3802, 0x00},
	{0x3803, 0xfe},
	{0x3804, 0x0a},
	{0x3805, 0x2f},
	{0x3806, 0x06},
	{0x3807, 0xa5},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x02},
	{0x380b, 0xd0},
	{0x380c, 0x0e},
	{0x380d, 0xc4},
	{0x380e, 0x02},
	{0x380f, 0xe6},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3817, 0x00},
	{0x3820, 0x08},
	{0x3821, 0x07},
	{0x4004, 0x02},
	{0x4005, 0x18},
	{0x3b0b, 0x80},
	{0x4837, 0x17},
	{0x0100, 0x01},
};
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
	unsigned char explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;
	if (exp_val < 7)
		exp_val = 7;

	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)(0x0000ff & exp_val);

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
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	struct sensor_info *info = to_state(sd);
	unsigned char gainlow = 0;
	unsigned char gainhigh = 0;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x3);

	sensor_write(sd, 0x3509, gainlow);
	sensor_write(sd, 0x3508, gainhigh);

	info->gain = gain_val;

	return 0;
}

static int OV5695_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
		int exp_val, gain_val, shutter, frame_length;
  unsigned char explow = 0, expmid = 0, exphigh = 0;
  unsigned char gainlow = 0, gainhigh = 0;
  struct sensor_info *info = to_state(sd);

  exp_val = exp_gain->exp_val;
  gain_val = exp_gain->gain_val;
  if (gain_val < 1*16)
	  gain_val = 16;
  if (gain_val > 64*16-1)
	  gain_val = 64*16-1;

  if (exp_val > 0xfffff)
	  exp_val = 0xfffff;

  gainlow = (unsigned char)(gain_val&0xff);
  gainhigh = (unsigned char)((gain_val>>8)&0x3);

  exphigh	= (unsigned char)((0x0f0000&exp_val)>>16);
  expmid	= (unsigned char)((0x00ff00&exp_val)>>8);
  explow	= (unsigned char)((0x0000ff&exp_val));

  shutter = exp_val/16;
  if (shutter  > OV5695_sensor_vts - 4)
		frame_length = shutter + 4;
  else
		frame_length = OV5695_sensor_vts;

  sensor_write(sd, 0x3208, 0x00);//enter group write
  sensor_write(sd, 0x3503, 0x08);
  sensor_write(sd, 0x380f, (frame_length & 0xff));
  sensor_write(sd, 0x380e, (frame_length >> 8));
  sensor_write(sd, 0x3509, gainlow);
  sensor_write(sd, 0x3508, gainhigh);

  sensor_write(sd, 0x3502, explow);
  sensor_write(sd, 0x3501, expmid);
  sensor_write(sd, 0x3500, exphigh);
  sensor_write(sd, 0x3208, 0x10);//end group write
  sensor_write(sd, 0x3208, 0xa0);//init group write
  info->exp = exp_val;
  info->gain = gain_val;
  return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
		sensor_dbg("[ACE]5695 sensor_power\n");
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		sensor_s_sw_stby(sd, STBY_ON);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		sensor_s_sw_stby(sd, STBY_OFF);
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!100\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(100, 120);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(200, 220);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(2000, 2200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(25000, 26000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(100, 120);
		//vin_gpio_set_status(sd, RESET, 0);
		//vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		printk("[ACE] ov5645 PWR_OFF OK\n");
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
		usleep_range(1000, 1200);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
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
		int ret = 0;

		ret = sensor_read(sd, 0x300b, &val);
		if (ret < 0)
		return -ENODEV;
		sensor_print("zzz detect 0x300b value = %x\n", val);
		SENSOR_ID |= (val << 8);
		ret = sensor_read(sd, 0x300c, &val);
		if (ret < 0)
		return -ENODEV;
		sensor_print("zzz detect 0x300c value = %x\n", val);
		SENSOR_ID |= (val);
		sensor_print("zzz detect V4L2_IDENT_SENSOR = %x\n", SENSOR_ID);

		while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
				ret = sensor_read(sd, 0x300b, &val);
				if (ret < 0)
				return -ENODEV;
				SENSOR_ID |= (val << 8);
				ret = sensor_read(sd, 0x300c, &val);
				if (ret < 0)
				return -ENODEV;
				SENSOR_ID |= (val);
				sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt, SENSOR_ID);
				cnt++;
		}

//        if (SENSOR_ID != V4L2_IDENT_SENSOR)
//                return -ENODEV;

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
	info->width = 2592;
	info->height = 1944;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

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

    {
      .width      = 2592,
      .height     = 1944,
      .hoffset    = 0,
      .voffset    = 0,
      .hts        = 740,
      .vts        = 2024,
      .pclk       = 45*1000*1000,
      .mipi_bps	  = 420*1000*1000,
	  .fps_fixed  = 30,
	  .bin_factor = 1,
	  .intg_min   = 1<<4,
	  .intg_max   = 1984<<4,
	  .gain_min   = 1<<4,
	  .gain_max   = 64<<4,
	  .regs       = sensor_qsxga_regs,
	  .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
		.set_size   = NULL,
    },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	struct sensor_exp_gain exp_gain;

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
	OV5695_sensor_vts = wsize->vts;
	exp_gain.exp_val = info->exp;
	exp_gain.gain_val = info->gain;
	if (exp_gain.exp_val == 0 || exp_gain.gain_val == 0) {
		exp_gain.exp_val = 16000;
		exp_gain.gain_val = 64;
	}
	sensor_s_exp_gain(sd, &exp_gain);
	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
				wsize->height);

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
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
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

static const struct v4l2_ctrl_config sensor_custom_ctrls[] = {
	{
		.ops = &sensor_ctrl_ops,
		.id = V4L2_CID_FRAME_RATE,
		.name = "frame rate",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 15,
		.max = 120,
		.step = 1,
		.def = 120,
	},
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int i;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2 + ARRAY_SIZE(sensor_custom_ctrls));

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
				256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
				65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	for (i = 0; i < ARRAY_SIZE(sensor_custom_ctrls); i++)
		v4l2_ctrl_new_custom(handler, &sensor_custom_ctrls[i], NULL);

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
	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
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
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 15616;
	info->gain = 64;

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
