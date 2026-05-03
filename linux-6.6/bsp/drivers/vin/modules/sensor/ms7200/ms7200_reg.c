/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for ms7200 HDMI to PARALLEL.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors: Gu Cheng <gucheng@allwinnertrch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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

#include "../camera.h"
#include "../sensor_helper.h"
#include "../../../vin-cci/cci_helper.h"
#include "ms7200_reg.h"


#define SENSOR_NAME "ms7200_parallel"

//DEFINE_MUTEX(ms_i2c_lock);

static u32 lowestBitSet(u32 x)
{
	u32 result = 0;

	/* x=0 is not properly handled by while-loop */
	/*if (x == 0)
		return 0;

	while ((x & 1) == 0) {
		x >>= 1;
		result++;
	}*/

	return result;
}

__maybe_unused static void ms_filp16(unsigned short *data)
{
	*data = ((*data & 0xff00) >> 8) | ((*data & 0xff) << 8);
}

__maybe_unused static void ms_filp32(unsigned int *data)
{
	*data = ((*data & 0xff00) << 8) | ((*data & 0xff) << 24)
	 | ((*data & 0xff0000) >> 8) | ((*data & 0xff000000) >> 24);
}

int ms_write8(struct v4l2_subdev *sd, unsigned short addr, unsigned char data)
{
	int ret = 0;
	if (!sd)
		return -1;
	ms_filp16(&addr);

//	mutex_lock(&ms_i2c_lock);
	ret = cci_write_a16_d8(sd, addr, data);
	if (ret < 0) {
		ms_filp16(&addr);
		pr_err("%s failed, addr:%04x value:0x%02x\n", __func__, addr, data);
	}
//	mutex_unlock(&ms_i2c_lock);

	return ret;
}

int ms_write16(struct v4l2_subdev *sd, unsigned short addr, unsigned int data)
{
	int ret = 0;

	ret = ms_write8(sd, addr, (unsigned char)data);
	ret = ms_write8(sd, addr + 1, (unsigned char)(data >> 8));

	return ret;
}

int ms_write8_regs(struct v4l2_subdev *sd, unsigned short addr, unsigned short regs_len, unsigned char *data)
{
	int ret = 0;
	unsigned short i;
	if (!sd)
		return -1;

//	mutex_lock(&ms_i2c_lock);
	for (i = 0; i < regs_len; i++) {
		ret = ms_write8(sd, addr + i, *(data + i));
		if (ret < 0)
			pr_err("%s failed, addr:0x%04x value:0x%02x\n", __func__, addr + i, *(data + i));
	}
//	mutex_unlock(&ms_i2c_lock);
	return ret;
}

int ms_write32(struct v4l2_subdev *sd, unsigned short addr, unsigned int data)
{
	int ret = 0;
	unsigned char buff[4];
	if (!sd)
		return -1;

	buff[0] = (unsigned char)(data);
	buff[1] = (unsigned char)(data >> 8);
	buff[2] = (unsigned char)(data >> 16);
	buff[3] = (unsigned char)(data >> 24);

	//ret = ms_write8_regs(sd, addr, 4, buff);
	ret = ms_write8(sd, addr, buff[0]);
	ret = ms_write8(sd, addr + 1, buff[1]);
	ret = ms_write8(sd, addr + 2, buff[2]);
	ret = ms_write8(sd, addr + 3, buff[3]);

	return ret;
}

int ms_write16_not_filp(struct v4l2_subdev *sd, unsigned short addr, unsigned int data)
{
	int ret = 0;

	if (!sd)
		return -1;

	ms_filp16(&addr);

//	mutex_lock(&ms_i2c_lock);
	ret = cci_write_a16_d16(sd, addr, data);
	if (ret < 0) {
		ms_filp16(&addr);
		pr_err("%s failed, addr:%04x value:0x%04x\n", __func__, addr, data);
	}
//	mutex_unlock(&ms_i2c_lock);

	return ret;
}

int ms_write32_not_filp(struct v4l2_subdev *sd, unsigned short addr, unsigned int data)
{
	int ret = 0;

	if (!sd)
		return -1;

	ms_filp16(&addr);

//	mutex_lock(&ms_i2c_lock);
	ret = cci_write_a16_d32(sd, addr, data);
	if (ret < 0) {
		ms_filp16(&addr);
		pr_err("%s failed, addr:%04x value:0x%08x\n", __func__, addr, data);
	}
//	mutex_unlock(&ms_i2c_lock);

	return ret;
}

unsigned char ms_read8(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int data = 0;
	int ret;

	ms_filp16(&addr);

//	mutex_lock(&ms_i2c_lock);
	ret = cci_read_a16_d8(sd, addr, (unsigned char *)&data);
	if (ret < 0) {
		ms_filp16(&addr);
		pr_err("%s failed, addr:%04x\n", __func__, addr);
	}
//	mutex_unlock(&ms_i2c_lock);

	return (unsigned char)data;
}

unsigned char ms_read8_a8(struct v4l2_subdev *sd, unsigned char addr)
{
	unsigned int data = 0;
	int ret;

//mutex_lock(&ms_i2c_lock);
	ret = cci_read_a8_d8(sd, addr, (unsigned char *)&data);
	if (ret < 0)
		pr_err("%s failed, addr:%04x\n", __func__, addr);
//	mutex_unlock(&ms_i2c_lock);

	return (unsigned char)data;
}

unsigned short ms_read16(struct v4l2_subdev *sd, unsigned short addr)
{
	return (ms_read8(sd, addr) + ((unsigned short)ms_read8(sd, addr + 1) << 8));
}

int ms_read8_regs(struct v4l2_subdev *sd, unsigned short addr, unsigned short regs_len, unsigned char *data)
{
	int ret = 0;
	unsigned short i = 0;
	if (!sd)
		return -1;

//	mutex_lock(&ms_i2c_lock);
	for (i = 0; i < regs_len; i++) {
		*(data + i) = ms_read8(sd, addr + i);
		//pr_err("%s:%d failed, i=%d  ret=%d addr:%04x value:0x%02x\n", __func__, __LINE__, i, ret, addr + i, *(data + i));
		if (ret < 0) {
			pr_err("%s failed, addr:%04x value:0x%02x\n", __func__, addr + i, *(data + i));
		}
	}
//	mutex_unlock(&ms_i2c_lock);

	return ret;
}

unsigned int ms_read32(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int temp;
	unsigned char buff[4];
	ms_read8_regs(sd, addr, 4, buff);
	temp = buff[3];
	temp = temp << 8 | buff[2];
	temp = temp << 8 | buff[1];
	temp = temp << 8 | buff[0];
	return temp;
}

unsigned short ms_read16_not_filp(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int data = 0;
	int ret;

	ms_filp16(&addr);

//	mutex_lock(&ms_i2c_lock);
	ret = cci_read_a16_d16(sd, addr, (unsigned short *)&data);
	if (ret < 0) {
		ms_filp16(&addr);
		pr_err("%s failed, addr:%04x\n", __func__, addr);
	}
//	mutex_unlock(&ms_i2c_lock);

	return (unsigned short)data;
}

unsigned int ms_read32_not_filp(struct v4l2_subdev *sd, unsigned short addr)
{
	unsigned int data = 0;
	int ret;

	ms_filp16(&addr);

//	mutex_lock(&ms_i2c_lock);
	ret = cci_read_a16_d32(sd, addr, &data);
	if (ret < 0) {
		ms_filp16(&addr);
		pr_err("%s failed, addr:%04x\n", __func__, addr);
	}
//	mutex_unlock(&ms_i2c_lock);

	return data;
}


void ms_write8_mask(struct v4l2_subdev *sd, u16 reg,
		u8 mask, u8 val)
{
	u8 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = ms_read8(sd, reg);
	//pr_err("%s:%d,ms_read8 addr:%04x temp:0x%02x\n", __func__, __LINE__, reg, temp);
	temp &= ~(mask);
	//pr_err("%s:%d,ms_write8 addr:%04x temp:0x%02x shift=0x%x mask=0x%x val=0x%x\n", __func__, __LINE__, reg, temp, shift, mask, val);
	temp |= (mask & (val << shift));
	//pr_err("%s:%d,ms_write8 addr:%04x value:0x%02x shift=0x%x mask=0x%x val=0x%x\n\n", __func__, __LINE__, reg, temp, shift, mask, val);
	ms_write8(sd, reg, temp);
}

void ms_write16_mask(struct v4l2_subdev *sd, u16 reg,
		u16 mask, u16 val)
{
	u16 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = ms_read16(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	ms_write16(sd, reg, temp);
}

void ms_write16_mask_not_filp(struct v4l2_subdev *sd, u16 reg,
		u16 mask, u16 val)
{
	u16 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = ms_read16_not_filp(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	ms_write16_not_filp(sd, reg, temp);
}


void ms_write32_mask(struct v4l2_subdev *sd, u16 reg,
		u32 mask, u32 val)
{
	u32 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = ms_read32(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	ms_write32(sd, reg, temp);
}

void ms_write32_mask_not_filp(struct v4l2_subdev *sd, u16 reg,
		u32 mask, u32 val)
{
	u32 temp = 0;
	u32 shift = lowestBitSet(mask);

	temp = ms_read32_not_filp(sd, reg);
	temp &= ~(mask);
	temp |= (mask & (val << shift));
	ms_write32_not_filp(sd, reg, temp);
}
