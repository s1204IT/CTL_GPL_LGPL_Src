/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for MS7200.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors: Gu Cheng <gucheng@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-dv-timings.h>
#include <uapi/linux/v4l2-dv-timings.h>
#include <linux/hdmi.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/kernel.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "ms7200_reg.h"

#include "ms7200_source_code/sdk_v1.1.2/hdmi_rx/ms7200_drv_hdmi_rx_config.h"
#include "ms7200_source_code/sdk_v1.1.2/inc/ms7200_typedef.h"
#include "ms7200_source_code/sdk_v1.1.2/misc/ms7200_config.h"

/*********************************ms7200_drv_dvout.c*****************************/
/* DVOUT register address map */
#define MS7200_DVOUT_REG_BASE				(0x0000)
#define REG_RST_CTRL0						(MS7200_DVOUT_REG_BASE + 0x09)
#define REG_TEST_SEL						(MS7200_DVOUT_REG_BASE + 0x16)
#define REG_DIG_CLK_SEL0					(MS7200_DVOUT_REG_BASE + 0xC0)
#define REG_DIG_CLK_SEL1					(MS7200_DVOUT_REG_BASE + 0xC1)
#define REG_DVO_CTRL_0						(MS7200_DVOUT_REG_BASE + 0xE0)
#define REG_DVO_CTRL_1						(MS7200_DVOUT_REG_BASE + 0xE1)
#define REG_DVO_CTRL_2						(MS7200_DVOUT_REG_BASE + 0xE2)
#define REG_DVO_CTRL_3						(MS7200_DVOUT_REG_BASE + 0xE3)
#define REG_DVO_CTRL_4						(MS7200_DVOUT_REG_BASE + 0xE4)
#define REG_DVO_CTRL_5						(MS7200_DVOUT_REG_BASE + 0xE5)

/* PA register address map */
#define MS7200_PA_REG_BASE					(0x1280)
#define REG_PA_S							(MS7200_PA_REG_BASE + 0x01)
/********************************************************************************/

/*********************************ms7200_drv_misc.c*****************************/
/* TOP MISC register address map */
#define MS7200_MISC_REG_BASE				(0x0000)
#define REG_CHIPID0_REG						(MS7200_MISC_REG_BASE + 0x0000)
#define REG_CHIPID1_REG						(MS7200_MISC_REG_BASE + 0x0001)
#define REG_CHIPID2_REG						(MS7200_MISC_REG_BASE + 0x0002)
#define REG_BDOP_REG						(MS7200_MISC_REG_BASE + 0x00a3)
#define REG_AUDIO_INPUT_OUTPUT_CNTRL		(MS7200_MISC_REG_BASE + 0x00c2)
#define REG_PINMUX							(MS7200_MISC_REG_BASE + 0x00a5)
#define REG_MISC_DRIV						(MS7200_MISC_REG_BASE + 0x00a8)
#define REG_MISC_TSTPAT0					(MS7200_MISC_REG_BASE + 0x00b0)
#define REG_MISC_TSTPAT1					(MS7200_MISC_REG_BASE + 0x00b1)
#define REG_MISC_TSTPAT8					(MS7200_MISC_REG_BASE + 0x00b8)
#define REG_MISC_TSTPAT9					(MS7200_MISC_REG_BASE + 0x00b9)
#define REG_MISC_TSTPAT10					(MS7200_MISC_REG_BASE + 0x00ba)

/* CSC register address map */
#define MS7200_CSC_REG_BASE					(0x00f0)
#define REG_CSC_CTRL1						(MS7200_CSC_REG_BASE + 0x00)

#define MS7200_CHIP_ID0			(0xa2)
#define MS7200_CHIP_ID1			(0x20)
#define MS7200_CHIP_ID2			(0x0a)

#define MS7200_DVO_16BIT 0

/********************************************************************************/

/*********************************ms7200_to_ms7210.c*****************************/
static UINT16 g_u16_tmds_clk;
static bool g_b_rxphy_status = TRUE;
static bool g_b_input_valid = FALSE;
static UINT8 g_u8_rx_stable_timer_count;
#define RX_STABLE_TIMEOUT (2)
static VIDEOTIMING_T g_t_hdmirx_timing;
static HDMI_CONFIG_T g_t_hdmirx_infoframe;
//static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_RGB, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_HSVSDE };
//static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_EMBANDED };
__maybe_unused static DVOUT_CONFIG_T g_t_dvout_config_16bit = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_DDR, DVOUT_SY_MODE_HSVSDE };
__maybe_unused static DVOUT_CONFIG_T g_t_dvout_config_8bit = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_8_12BIT, DVOUT_DR_MODE_DDR, DVOUT_SY_MODE_HSVSDE };
//static DVOUT_CONFIG_T g_t_dvout_sdr_config = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_SDR, DVOUT_SY_MODE_HSVSDE };
//static DVOUT_CONFIG_T g_t_dvout_config = { DVOUT_CS_MODE_YUV422, DVOUT_BW_MODE_16_24BIT, DVOUT_DR_MODE_DDR, DVOUT_SY_MODE_EMBANDED };

//static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_RGB, DVIN_BW_MODE_16_20_24BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_SDR, DVIN_SY_MODE_HSVSDE };
//static DVIN_CONFIG_T g_t_dvin_config = { DVIN_CS_MODE_YUV422, DVIN_BW_MODE_16_20_24BIT, DVIN_SQ_MODE_NONSEQ, DVIN_DR_MODE_SDR, DVIN_SY_MODE_EMBANDED };
//static bool g_b_output_valid = FALSE;
//static VIDEOTIMING_T g_t_hdmitx_timing;
//static HDMI_CONFIG_T g_t_hdmitx_infoframe;
//static UINT8 g_u8_tx_stable_timer_count = 0;
#define TX_STABLE_TIMEOUT (3)

static UINT8 u8_sys_edid_default_buf[256] = {
#if !MS7200_DVO_16BIT
	//default
	// Explore Semiconductor, Inc. EDID Editor V2
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,   // address 0x00
	0x4C, 0x2D, 0xFF, 0x0D, 0x58, 0x4D, 0x51, 0x30,   //
	0x1C, 0x1C, 0x01, 0x03, 0x80, 0x3D, 0x23, 0x78,   // address 0x10
	0x2A, 0x5F, 0xB1, 0xA2, 0x57, 0x4F, 0xA2, 0x28,   //
	0x0F, 0x50, 0x54, 0xBF, 0xEF, 0x80, 0x71, 0x4F,   // address 0x20
	0x81, 0x00, 0x81, 0xC0, 0x81, 0x80, 0x95, 0x00,   //
	0xA9, 0xC0, 0xB3, 0x00, 0x01, 0x01, 0x04, 0x74,   // address 0x30
	0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,   //
	0x8A, 0x00, 0x60, 0x59, 0x21, 0x00, 0x00, 0x1E,   // address 0x40
	0x00, 0x00, 0x00, 0xFD, 0x00, 0x18, 0x4B, 0x1E,   //
	0x5A, 0x1E, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20,   // address 0x50
	0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x55,   //
	0x32, 0x38, 0x48, 0x37, 0x35, 0x78, 0x0A, 0x20,   // address 0x60
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFF,   //
	0x00, 0x48, 0x54, 0x50, 0x4B, 0x37, 0x30, 0x30,   // address 0x70
	0x30, 0x35, 0x31, 0x0A, 0x20, 0x20, 0x01, 0xF7,   //
	0x02, 0x03, 0x26, 0xF0, 0x4B, 0x5F, 0x10, 0x04,   // address 0x80
	0x1F, 0x13, 0x03, 0x12, 0x20, 0x22, 0x5E, 0x5D,   //
	0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00,   // address 0x90
	0x6D, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x80, 0x3C,   //
	0x20, 0x10, 0x60, 0x01, 0x02, 0x03, 0x02, 0x3A,   // address 0xA0
	0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,   //
	0x45, 0x00, 0x60, 0x59, 0x21, 0x00, 0x00, 0x1E,   // address 0xB0
	0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38, 0x2D, 0x40,   //
	0x10, 0x2C, 0x45, 0x80, 0x60, 0x59, 0x21, 0x00,   // address 0xC0
	0x00, 0x1E, 0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0,   //
	0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0x60, 0x59,   // address 0xD0
	0x21, 0x00, 0x00, 0x1E, 0x56, 0x5E, 0x00, 0xA0,   //
	0xA0, 0xA0, 0x29, 0x50, 0x30, 0x20, 0x35, 0x00,   // address 0xE0
	0x60, 0x59, 0x21, 0x00, 0x00, 0x1A, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0xF0
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8,   //
#endif

#if VIN_FALSE
	// 1080p 12bit color depth
	// Explore Semiconductor, Inc. EDID Editor V2
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,   // address 0x00
	0x21, 0x57, 0x36, 0x18, 0xbd, 0xe9, 0x02, 0x00,   //
	0x25, 0x1d, 0x01, 0x03, 0x80, 0x35, 0x1d, 0x78,   // address 0x01
	0x62, 0xee, 0x91, 0xa3, 0x54, 0x4c, 0x99, 0x26,   //
	0x0f, 0x50, 0x54, 0x21, 0x0f, 0x00, 0x81, 0x00,   // address 0x02
	0x81, 0x40, 0x81, 0x80, 0x90, 0x40, 0x95, 0x00,   //
	0x01, 0x01, 0xa9, 0x40, 0xb3, 0x00, 0x02, 0x3a,   // address 0x03
	0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,   //
	0x55, 0x00, 0xe0, 0x0e, 0x11, 0x00, 0x00, 0x5f,   // address 0x04
	0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20,   //
	0x6e, 0x28, 0x55, 0x00, 0x0f, 0x48, 0x42, 0x00,   // address 0x05
	0x00, 0x1c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17,   //
	0x55, 0x1e, 0x64, 0x1e, 0x00, 0x0a, 0x20, 0x20,   // address 0x06
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,   //
	0x00, 0x50, 0x61, 0x69, 0x43, 0x61, 0x73, 0x74,   // address 0x07
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x2d,   //
	0x02, 0x03, 0x1f, 0xf1, 0x4a, 0x90, 0x04, 0x1f,   // address 0x08
	0x02, 0x03, 0x11, 0x05, 0x14, 0x13, 0x12, 0x23,   //
	0x09, 0x1f, 0x07, 0x83, 0x01, 0x00, 0x00, 0x67,   // address 0x09
	0x03, 0x0c, 0x00, 0x10, 0x00, 0x38, 0x3c, 0x02,   //
	0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58,   // address 0x0a
	0x2c, 0x45, 0x00, 0xdd, 0x0c, 0x11, 0x00, 0x00,   //
	0x1e, 0x66, 0x21, 0x56, 0xaa, 0x51, 0x00, 0x1e,   // address 0x0b
	0x30, 0x46, 0x8f, 0x33, 0x00, 0x0f, 0x48, 0x42,   //
	0x00, 0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20,   // address 0x0c
	0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0x10,   //
	0x09, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,   // address 0x0d
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0e
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd8,   //
#endif

#if VIN_FALSE
	// 4k(3840) 12bit color depth
	// Explore Semiconductor, Inc. EDID Editor V2
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,   // address 0x00
	0x21, 0x57, 0x36, 0x18, 0xbd, 0xe9, 0x02, 0x00,   //
	0x25, 0x1d, 0x01, 0x03, 0x80, 0x35, 0x1d, 0x78,   // address 0x01
	0x62, 0xee, 0x91, 0xa3, 0x54, 0x4c, 0x99, 0x26,   //
	0x0f, 0x50, 0x54, 0x21, 0x0f, 0x00, 0x81, 0x00,   // address 0x02
	0x81, 0x40, 0x81, 0x80, 0x90, 0x40, 0x95, 0x00,   //
	0x01, 0x01, 0xa9, 0x40, 0xb3, 0x00, 0x02, 0x3a,   // address 0x03
	0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,   //
	0x55, 0x00, 0xe0, 0x0e, 0x11, 0x00, 0x00, 0x5f,   // address 0x04
	0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20,   //
	0x6e, 0x28, 0x55, 0x00, 0x0f, 0x48, 0x42, 0x00,   // address 0x05
	0x00, 0x1c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17,   //
	0x55, 0x1e, 0x64, 0x1e, 0x00, 0x0a, 0x20, 0x20,   // address 0x06
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,   //
	0x00, 0x50, 0x61, 0x69, 0x43, 0x61, 0x73, 0x74,   // address 0x07
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x2d,   //
	0x02, 0x03, 0x22, 0xf1, 0x4d, 0x90, 0x04, 0x1f,   // address 0x08
	0x02, 0x03, 0x11, 0x05, 0x14, 0x5d, 0x5e, 0x5f,   //
	0x13, 0x12, 0x23, 0x09, 0x1f, 0x07, 0x83, 0x01,   // address 0x09
	0x00, 0x00, 0x67, 0x03, 0x0c, 0x00, 0x10, 0x00,   //
	0x38, 0x3c, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38,   // address 0x0a
	0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0xdd, 0x0c,   //
	0x11, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x56, 0xaa,   // address 0x0b
	0x51, 0x00, 0x1e, 0x30, 0x46, 0x8f, 0x33, 0x00,   //
	0x0f, 0x48, 0x42, 0x00, 0x00, 0x1e, 0x8c, 0x0a,   // address 0x0c
	0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,   //
	0x96, 0x00, 0x10, 0x09, 0x00, 0x00, 0x00, 0x18,   // address 0x0d
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0e
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8,   //
#endif

#if MS7200_DVO_16BIT
	// 4k(4096) 12bit color depth
	// Explore Semiconductor, Inc. EDID Editor V2
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,   // address 0x00
	0x21, 0x57, 0x36, 0x18, 0xbd, 0xe9, 0x02, 0x00,   //
	0x25, 0x1d, 0x01, 0x03, 0x80, 0x35, 0x1d, 0x78,   // address 0x01
	0x62, 0xee, 0x91, 0xa3, 0x54, 0x4c, 0x99, 0x26,   //
	0x0f, 0x50, 0x54, 0x21, 0x0f, 0x00, 0x81, 0x00,   // address 0x02
	0x81, 0x40, 0x81, 0x80, 0x90, 0x40, 0x95, 0x00,   //
	0x01, 0x01, 0xa9, 0x40, 0xb3, 0x00, 0x02, 0x3a,   // address 0x03
	0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,   //
	0x55, 0x00, 0xe0, 0x0e, 0x11, 0x00, 0x00, 0x5f,   // address 0x04
	0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20,   //
	0x6e, 0x28, 0x55, 0x00, 0x0f, 0x48, 0x42, 0x00,   // address 0x05
	0x00, 0x1c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17,   //
	0x55, 0x1e, 0x64, 0x1e, 0x00, 0x0a, 0x20, 0x20,   // address 0x06
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,   //
	0x00, 0x50, 0x61, 0x69, 0x43, 0x61, 0x73, 0x74,   // address 0x07
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x2d,   //
	0x02, 0x03, 0x25, 0xf1, 0x50, 0x90, 0x04, 0x1f,   // address 0x08
	0x02, 0x03, 0x11, 0x05, 0x14, 0x5d, 0x5e, 0x5f,   //
	0x62, 0x63, 0x64, 0x13, 0x12, 0x23, 0x09, 0x1f,   // address 0x09
	0x07, 0x83, 0x01, 0x00, 0x00, 0x67, 0x03, 0x0c,   //
	0x00, 0x10, 0x00, 0x38, 0x3c, 0x02, 0x3a, 0x80,   // address 0x0a
	0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c, 0x45,   //
	0x00, 0xdd, 0x0c, 0x11, 0x00, 0x00, 0x1e, 0x66,   // address 0x0b
	0x21, 0x56, 0xaa, 0x51, 0x00, 0x1e, 0x30, 0x46,   //
	0x8f, 0x33, 0x00, 0x0f, 0x48, 0x42, 0x00, 0x00,   // address 0x0c
	0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d,   //
	0x10, 0x10, 0x3e, 0x96, 0x00, 0x10, 0x09, 0x00,   // address 0x0d
	0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0e
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89,   //
#endif
};
/********************************************************************************/

/*********************************ms7200_drv_hdmi_rx.c*****************************/
#define MDT_VALID_ISTS ((UINT16)(~(MDT_VTOT_CLK_ISTS | MDT_HTOT32_CLK_ISTS | MDT_VS_CLK_ISTS)))

typedef struct _T_HDMI_AUDIO_SAMPLE_PACKET_PARA_ {
	UINT8 u8_audio_mode;		// enum refer to HDMI_AUDIO_MODE_E
	UINT8 u8_audio_rate;		// enum refer to HDMI_AUDIO_RATE_E
	UINT8 u8_audio_bits;		// enum refer to HDMI_AUDIO_LENGTH_E
	UINT8 u8_audio_channels;	// enum refer to HDMI_AUDIO_CHN_E
} AUDIO_SAMPLE_PACKET_T;
/********************************************************************************/

#define RXPHY_DEBUG_LOG (0)

#define MCLK (27 * 1000 * 1000) /*ms7200 not need mclk*/

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The imx386 i2c address
 */
#define I2C_ADDR 0x56

#define SENSOR_NUM 0x1
#define SENSOR_NAME "ms7200"
#define SENSOR_NAME_2 "ms7200_2"

#define EDID_NUM_BLOCKS_MAX 8
#define EDID_BLOCK_SIZE 128

#define HVREF_POL_CHANGE_LOW 0
#define HVREF_POL_CHANGE_HIGH 1
#define CLK_POL_CHANGE 2

static unsigned int ms_debug = 1;
static unsigned int ms7200_power_on_first_flag = 1;
static unsigned int ms7200_hvref_clk_pol_change = HVREF_POL_CHANGE_LOW;
static unsigned int ms7200_hdmi_hot_plug_status;

/*
 * Adjust paraments according to user manual spec.
 */
#define PCLK_DLY 	0x06
#define HSYNC_DLY	0x03
#define VSYNC_DLY	0x03
#define FIELD_DLY	0x00

#define MS_INF(...) \
	do { \
		if (ms_debug) { \
			printk("[MS7200]"__VA_ARGS__); \
		} \
	} while (0)

#define MS_INF2(...) \
	do { \
		if (ms_debug >= 2) { \
			printk("[MS7200]"__VA_ARGS__); \
		} \
	} while (0)

#define MS_ERR(...) \
	do { \
		printk("[MS7200 ERROR]"__VA_ARGS__); \
	} while (0)

enum ms_ddc5v_delays {
	DDC5V_DELAY_0_MS,
	DDC5V_DELAY_50_MS,
	DDC5V_DELAY_100_MS,
	DDC5V_DELAY_200_MS,
};

enum ms_hdmi_detection_delay {
	HDMI_MODE_DELAY_0_MS,
	HDMI_MODE_DELAY_25_MS,
	HDMI_MODE_DELAY_50_MS,
	HDMI_MODE_DELAY_100_MS,
};

struct ms_audio_param {
	bool present; //if there is audio data received
	unsigned int fs; //sampling frequency
	unsigned short audio_fs; // for ms7200 audio fs
};

struct hdmi_rx_ms7200 {
	struct sensor_info info;
	struct i2c_client *client;
	struct v4l2_ctrl_handler hdl;
	struct task_struct   *ms_task;

	unsigned char status;
	unsigned char wait4get_timing; //wait user space to get timing
	unsigned char wait4timing_stable; //wait timing stable

	struct v4l2_ctrl *tmds_signal_ctrl;
	bool tmds_signal;
	bool signal_event_subscribe;

	struct delayed_work source_change_work;
	struct v4l2_dv_timings timing;
	struct ms_audio_param audio;
	bool ddc_5v_present;

	unsigned int refclk_hz;
	enum ms_ddc5v_delays ddc5v_delays;
	unsigned int hdcp_enable;

	unsigned char colorbar_enable;
	unsigned char edid[1024];
};

#define V4L2_EVENT_SRC_CH_HPD_IN	(1 << 1)
#define V4L2_EVENT_SRC_CH_HPD_OUT	(1 << 2)

struct hdmi_rx_ms7200 ms7200[SENSOR_NUM];

 const struct v4l2_dv_timings_cap ms7200_timings_cap = {
	.type = V4L2_DV_BT_656_1120,
	.reserved = { 0 },

	//min_width,max_width,min_height,max_height,min_pixelclock,max_pixelclock,
	//standards,capabilities
	V4L2_INIT_BT_TIMINGS(720, 4096, 480, 2160, 13000000, 297100000,
		V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
		V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT,
		V4L2_DV_BT_CAP_PROGRESSIVE |
		V4L2_DV_BT_CAP_INTERLACED |
		V4L2_DV_BT_CAP_REDUCED_BLANKING |
		V4L2_DV_BT_CAP_CUSTOM)
};

extern bool v4l2_match_dv_timings(const struct v4l2_dv_timings *t1,
			const struct v4l2_dv_timings *t2,
			unsigned pclock_delta, bool match_reduced_fps);
extern void v4l2_print_dv_timings(const char *dev_prefix, const char *prefix,
			const struct v4l2_dv_timings *t, bool detailed);

extern int v4l2_enum_dv_timings_cap(struct v4l2_enum_dv_timings *t,
				const struct v4l2_dv_timings_cap *cap,
				v4l2_check_dv_timings_fnc fnc,
				void *fnc_handle);

static int ms_get_detected_timings(struct v4l2_subdev *sd,
			struct v4l2_dv_timings *timings);

VOID ms7200_media_service(struct v4l2_subdev *sd);
static int ms_run_thread(void *parg);
void ms7200_hpd_event_subscribe(struct v4l2_subdev *sd);

__maybe_unused static void ms_msleep(unsigned int ms)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(ms));
}

void set_edid(struct v4l2_subdev *sd,
			unsigned char *edid_data,
			unsigned int blocks)
{
	/*int i = 0;

	ms_write8(sd, EDID_MODE, 0x02);
	ms_write8(sd, EDID_LEN1, 0x00);
	ms_write8(sd, EDID_LEN2, 0x01);

	for (i = 0; i < 0x100; i++) {
		ms_write8(sd, 0x8C00 + i, edid_data[i]);
	}*/
}

void colorbar_1024x768(struct v4l2_subdev *sd)
{
	/*REM Debug Control
	REM refclk=27MHz, mipi 594Mbps 4 lane
	REM color bar for 1024x768 YUV422*/
}

static VOID _drv_hdmi_rx_audio_init(struct v4l2_subdev *sd)
{
	//20190401, audio pll step adjust.
	//
	//HAL_WriteWord(REG_AUPLL_FIFO_TH1_L, 0x0c0);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH2_L, 0x180);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH3_L, 0x2c0);
	//
	//HAL_WriteWord(REG_AUPLL_FIFO_TH4_L, 0x300);
	//
	//HAL_WriteWord(REG_AUPLL_FIFO_TH5_L, 0x340);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH6_L, 0x480);
	//HAL_WriteWord(REG_AUPLL_FIFO_TH7_L, 0x540);
	//
	//20200521, use default value. maybe cause output cts value change +- 10.
	//but if use beflow value, maybe cause can't support input audio cts value quick change
	ms_write8(sd, REG_AUPLL_FREQ_INC_STEP1, 0x00);
	ms_write8(sd, REG_AUPLL_FREQ_INC_STEP2, 0x01);
	ms_write8(sd, REG_AUPLL_FREQ_INC_STEP3, 0x01);
	ms_write8(sd, REG_AUPLL_FREQ_INC_STEP4, 0x01);
	ms_write8(sd, REG_AUPLL_FREQ_DEC_STEP1, 0x00);
	ms_write8(sd, REG_AUPLL_FREQ_DEC_STEP2, 0x01);
	ms_write8(sd, REG_AUPLL_FREQ_DEC_STEP3, 0x01);
	ms_write8(sd, REG_AUPLL_FREQ_DEC_STEP4, 0x01);

	//20180709, aupll_update_timer, FOSC = 25MHZ(40ns),
	//10us = 10000ns / 40ns = 250
	//set to 30us
	ms_write16(sd, REG_AUPLL_UPD_TIMER_L, 0x0300);

	//20180320, enhance Audio PLL VCO stable
	ms_write8(sd, REG_AUPLL_CFG_COEF, 0x0A);

	//ms7200drv_hdmi_audio_clk_enable(TRUE);
	ms_write8_mask(sd, REG_MISC_CLK_CTRL1, MSRT_BIT0, MSRT_BIT0);
	//ms7200drv_hdmi_audio_dll_clk_reset_release(TRUE);
	ms_write8_mask(sd, REG_MISC_RST_CTRL0, MSRT_BIT7, MSRT_BIT7);
}

VOID ms7200drv_hdmi_rx_init(struct v4l2_subdev *sd)
{
	//20181218, for register program val check only.no effect.
	//HAL_WriteByte(REG_PI_RX_REGISTER_CHECK, REG_PI_RX_REGISTER_CHECK_VAL);
	//20200518, r_osc25m_driver change 4mA->2mA
	//HAL_ClrBits(REG_MISC_IO_CTRL1, MSRT_BITS1_0);
	ms_write8_mask(sd, REG_MISC_IO_CTRL1, MSRT_BITS1_0, 0);

	//RC_25M config
	//HAL_WriteByte(REG_MISC_RC_CTRL1, MS7200_RC_CTRL1);
	//HAL_WriteByte(REG_MISC_RC_CTRL2, MS7200_RC_CTRL2);
	ms_write8(sd, REG_MISC_RC_CTRL1, MS7200_RC_CTRL1);
	ms_write8(sd, REG_MISC_RC_CTRL2, MS7200_RC_CTRL2);

	// 20180404, PI PHY, tmds noise filter
	//ms7200drv_hdmi_rx_pi_clk_ofst_sel(5);
	ms_write8_mask(sd, REG_PI_RX_OFST_CTRL4, MSRT_BITS3_0, 5);

	//PI phase detection threshold set to 0
	//ms7200drv_hdmi_rx_pi_dcm_sel(1);
	//ms7200drv_hdmi_rx_pi_det_th(0);
	ms_write8_mask(sd, REG_PI_CTRL0, MSRT_BITS1_0, 1);
	ms_write8_mask(sd, REG_PI_CTRL1, MSRT_BITS2_0, 0);

	_drv_hdmi_rx_audio_init(sd);

	//tmds clk good detect, 1/16 = 6.25% margin
	//20200325, fixed BUG for BS5330A, clk_measure_debug = 1
	//HAL_WriteDWord_Ex(REG_HDMI_CKM_EVLTM, 0x0112FFF0UL);
	ms_write32(sd, REG_HDMI_CKM_EVLTM, 0x0112FFF0UL);
	//if osc = 24M, clk detect max: 380MHz, min: 12MHz
	//HAL_WriteDWord_Ex(REG_HDMI_CKM_F, 0xFFFF0800UL);
	ms_write32(sd, REG_HDMI_CKM_F, 0xFFFF0800UL);
	//20181218, mdt detection time set to 2 frame stable
	//0x1F00, (0x2) << 8
	//20201117, [12:8] change 2->0, resolve TEK_DTG534 source interlace mode(480i/576i/1080i) can't be work.
	//HAL_WriteDWord_Ex(REG_HDMI_MODE_RECOVER, 0x30110000UL);
	ms_write32(sd, REG_HDMI_MODE_RECOVER, 0x30110000UL);

	//20190328, MDT stable, do not use vblank status.
	//HAL_ModBits_Ex(REG_MD_IL_CTRL, 0xFF000000UL, (0x11UL) << 24);
	ms_write32_mask(sd, REG_MD_IL_CTRL, 0xFF000000UL, (0x11UL) << 24);

	//20190329, mdt interrupt magin update.
	//add hs vs active time
	//HAL_WriteDWord_Ex(REG_MD_HCTRL2, 0x223a);
	ms_write32(sd, REG_MD_HCTRL2, 0x223a);
	//VTOT_LIN_ITH = 3line, resolve for interlace timing input issue.
	//VACT_LIN_ITH = 3line
	//HAL_WriteDWord_Ex(REG_MD_VTH, 0x0a92);
	ms_write32(sd, REG_MD_VTH, 0x0a92);

	//20191123, resolve sanxing DVD, speaker_locations is 0x0A, but chn number is 8 chn issue.
	//fixed subpacket_ovr_speaker_allocation_en = 1
	//HAL_SetBits_Ex(REG_AUD_FIFO_CTRL, 0x1UL << 17);

	//2020724, set audio cts interruput threshold, cts value change >= 32
	//HAL_ModBits_Ex(REG_PDEC_ACRM_CTRL, 0x1c,  5 << 2);
	ms_write32_mask(sd, REG_PDEC_ACRM_CTRL, 0x1c,  5 << 2);
	//20180710, set audio fifo start threshold
	//AFIF_TH_START = 0x300 / 4 = 0xc0
	//AFIF_TH_MAX,AFIF_TH_MIN = 0x180 / 4 = 0x60
	//ms7200drv_hdmi_rx_afif_th((0xc0UL << 18) | (0x60UL << 9) | (0x60UL));
	ms_write32(sd, REG_AUD_FIFO_TH, (0xc0UL << 18) | (0x60UL << 9) | (0x60UL));

	//automatic mute enable for FIFO over- underflow
	//20200926, resolve audio maybe force mute because HW bug.
	//ms7200drv_hdmi_rx_audio_mute_ctrl(0x0);
	ms_write32_mask(sd, REG_AUD_MUTE_CTRL, MSRT_BITS6_5,  0x0 << 5);
	//enable I2S output, enable SPDIF output
	//ms7200drv_hdmi_rx_audio_output_enable(TRUE);
	ms_write32_mask(sd, REG_AUD_SAO_CTRL, 0x7FE, 0x0);

	//hdmi status contorl
	//deep color status clr when 2 frame no valild deep color information input
	//HAL_ModBits_Ex(REG_HDMI_DCM_CTRL, 0x3c, 2 << 2);
	ms_write32_mask(sd, REG_HDMI_DCM_CTRL, 0x3c, 2 << 2);

	//pdec error filter
	//HAL_SetBits_Ex(REG_PDEC_ERR_FILTER, 0xfff);
	ms_write32_mask(sd, REG_PDEC_ERR_FILTER, 0xfff, 0xfff);

#if MS7200_HDMI_RX_INT_ENABLE
	//enable rx int to PIN.clk change
	//HAL_WriteDWord_Ex(REG_HDMI_IEN_SET, 0x40);
	ms_write32(sd, REG_HDMI_IEN_SET, 0x40);

	//enable interrupt PIN. when interrupt coming, PIN 1->0.
	//when interrupt register bit be cleared, PIN 0->1
	//HAL_SetBits(REG_MISC_INT_PIN_CTRL, 0x30);
	ms_write8_mask(sd, REG_MISC_INT_PIN_CTRL, 0x30, 0x30);
#endif
}

static bool _drv_hdmi_rx_pi_term_config(struct v4l2_subdev *sd, e_RxPhyMode eMode, UINT8 u8Time)
{
	bool bTermDone = TRUE;
	//UINT8 u8TermVal = 0;
	//ms7200drv_hdmi_rx_tmds_overload_protect_disable(TRUE);
	ms_write8_mask(sd, REG_PI_RX_CTRL1, MSRT_BIT0, MSRT_BIT0);
	//HAL_SetBits(REG_PI_RX_TRM_CTRL, MSRT_BIT7);
	ms_write8_mask(sd, REG_PI_RX_TRM_CTRL, MSRT_BIT7, MSRT_BIT7);
	//HAL_SetBits(REG_PI_RX_CTRL0, MSRT_BIT0);
	ms_write8_mask(sd, REG_PI_RX_CTRL0, MSRT_BIT0, MSRT_BIT0);
#if (MS7200_HDMI_RX_TMDS_OVERLOAD_PROTECT_ENABLE)
	//ms7200drv_hdmi_rx_tmds_overload_protect_disable(FALSE);
	ms_write8_mask(sd, REG_PI_RX_CTRL1, MSRT_BIT0, 0x00);
#endif
	return bTermDone;
}

static bool _drv_hdmi_rx_pi_offset_config(struct v4l2_subdev *sd, UINT8 u8Time)
{
	bool bOffsetDone = FALSE;
	UINT8 u8Channel;
	UINT8 u8OffsetDec, u8OffsetInc;

	for (u8Channel = 0; u8Channel <= 2; u8Channel++) {
		bOffsetDone = FALSE;
		//ms7200drv_hdmi_rx_pi_offset_enable_set((e_RxChannel)u8Channel);
		ms_write8_mask(sd, REG_PI_RX_OFST_CTRL5, MSRT_BITS2_0, 1 << (e_RxChannel)u8Channel);
		u8OffsetDec = 0x10;
		u8OffsetInc = 0;
		while (!bOffsetDone) {
			if (u8OffsetDec == 0)
				break;
			//ms7200drv_hdmi_rx_pi_offset_dec_set((e_RxChannel)u8Channel, --u8OffsetDec);
			ms_write8_mask(sd, REG_PI_RX_OFST_CTRL1 + (e_RxChannel)u8Channel, MSRT_BITS7_4, (--u8OffsetDec) << 4);
			//Delay_ms(u8Time);
			mdelay(u8Time);
			//bOffsetDone = ms7200drv_hdmi_rx_pi_offset_cmpout_get();
			bOffsetDone = (ms_read8(sd, REG_PI_RX_OFST_CTRL5) & MSRT_BIT4) >> 4;
		}
		while (!bOffsetDone) {
			if (u8OffsetInc == 16) {
				--u8OffsetInc;
				break;
			}
			//ms7200drv_hdmi_rx_pi_offset_inc_set((e_RxChannel)u8Channel, u8OffsetInc++);
			ms_write8_mask(sd, REG_PI_RX_OFST_CTRL1 + (e_RxChannel)u8Channel, MSRT_BITS3_0, (u8OffsetInc++));
			//Delay_ms(u8Time);
			mdelay(u8Time);
			//bOffsetDone = ms7200drv_hdmi_rx_pi_offset_cmpout_get();
			bOffsetDone = (ms_read8(sd, REG_PI_RX_OFST_CTRL5) & MSRT_BIT4) >> 4;
		}
		if (!bOffsetDone) {
			u8OffsetInc = 0;
			//ms7200drv_hdmi_rx_pi_offset_inc_set((e_RxChannel)u8Channel, u8OffsetInc);
			ms_write8_mask(sd, REG_PI_RX_OFST_CTRL1 + (e_RxChannel)u8Channel, MSRT_BITS3_0, (u8OffsetInc++));
			//Delay_ms(u8Time);
			mdelay(u8Time);
			//bOffsetDone = ms7200drv_hdmi_rx_pi_offset_cmpout_get();
			bOffsetDone = (ms_read8(sd, REG_PI_RX_OFST_CTRL5) & MSRT_BIT4) >> 4;
		}

		/*MS_INF("u8Channel = %d\n", u8Channel);
		MS_INF("bOffsetDone = %d\n", bOffsetDone);
		MS_INF("u8OffsetDec = %d\n", u8OffsetDec);
		MS_INF("u8OffsetInc = %d\n", u8OffsetInc);*/
	}
	//HAL_ClrBits(REG_PI_RX_OFST_CTRL5, MSRT_BITS2_0);
	ms_write8_mask(sd, REG_PI_RX_OFST_CTRL5, MSRT_BITS2_0, 0x00);
	return bOffsetDone;
}

bool ms7200drv_hdmi_rx_pi_phy_init(struct v4l2_subdev *sd)
{
	bool bValid = TRUE;
	//_drv_hdmi_rx_pi_rcv_release(TRUE);
	ms_write8_mask(sd, REG_MISC_RST_CTRL0, MSRT_BIT2, MSRT_BIT2);
	bValid &= _drv_hdmi_rx_pi_term_config(sd, AUTO_MODE, 0x80);
	bValid &= _drv_hdmi_rx_pi_offset_config(sd, 0x1);
	//HAL_WriteByte(REG_PI_RXPLL_CTRL0, 0x20);
	ms_write8(sd, REG_PI_RXPLL_CTRL0, 0x20);
	//ms7200drv_hdmi_rx_pi_rxpll_mode_set(AUTO_MODE);
	ms_write8_mask(sd, REG_PI_RX_PLL_CFG_SEL, MSRT_BITS2_0, 0x00);
	//20180404, rx_bus_clk_en
	//HAL_SetBits(REG_MISC_CLK_CTRL0, MSRT_BIT6);
	ms_write8_mask(sd, REG_MISC_CLK_CTRL0, MSRT_BIT6, MSRT_BIT6);

	//20200427, add RXPLL new method for BS5330A...
#if MS7200_RXPLL_METHOD
	//HAL_ClrBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BIT7);
	ms_write8_mask(sd, REG_PI_RX_PLL_CFG_SEL, MSRT_BIT7, 0x00);
#endif

	//HAL_SetBits(REG_PI_RX_PHY_INTR, MSRT_BIT1);
	ms_write8_mask(sd, REG_PI_RX_PHY_INTR, MSRT_BIT1, MSRT_BIT1);

	//PI error det enable
	//_drv_errdet_read_enable(TRUE);
	ms_write32_mask(sd, REG_SCDC_CONFIG, MSRT_BIT0, MSRT_BIT0);
	ms_write32_mask(sd, REG_HDMI20_CONTROL, 0x1f << 8, 0x1f << 8);
	//_drv_errdet_clr_swsel(TRUE);
	ms_write32_mask(sd, REG_HDMI20_CONTROL, 1UL << 14, 1UL << 14);
	return bValid;

}

VOID ms7200_hdmirx_init(struct v4l2_subdev *sd)
{
	//ms7200drv_hdmi_rx_controller_reset(HDMI_RX_CTRL_MAIN);
	ms7200drv_hdmi_rx_init(sd);
	ms7200drv_hdmi_rx_pi_phy_init(sd);
}

VOID ms7200drv_dvout_mode_config(struct v4l2_subdev *sd, DVOUT_CONFIG_T *t_dvout_config)
{
	UINT8 u8_clk_ratio = 0;

	// dvo_yuv_out
	//HAL_ToggleBits(REG_DVO_CTRL_1, MSRT_BIT4, t_dvout_config->u8_cs_mode != DVOUT_CS_MODE_RGB);
	ms_write8_mask(sd, REG_DVO_CTRL_1, MSRT_BIT4, t_dvout_config->u8_cs_mode != DVOUT_CS_MODE_RGB ? MSRT_BIT4 : 0x00);
	// dvo_blk_clip_off
	//HAL_ClrBits(REG_DVO_CTRL_1, MSRT_BIT5);
	ms_write8_mask(sd, REG_DVO_CTRL_1, MSRT_BIT5, 0x00);

	// dvo_16b_en, dvo_8b_en
	switch (t_dvout_config->u8_cs_mode) {
	case DVOUT_CS_MODE_RGB:
	case DVOUT_CS_MODE_YUV444:
		//HAL_ClrBits(REG_DVO_CTRL_2, MSRT_BITS2_1);   //24 bit
		ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BITS2_1, 0x00);
		if (t_dvout_config->u8_bw_mode == DVOUT_BW_MODE_8_12BIT) {
			// dvin_12b444
			//HAL_ToggleBits(REG_DVO_CTRL_5, MSRT_BIT1, !(u8_clk_ratio & MSRT_BIT3));
			ms_write8_mask(sd, REG_DVO_CTRL_5, MSRT_BIT1, !(u8_clk_ratio & MSRT_BIT3) ? MSRT_BIT1 : 0x00);
			u8_clk_ratio = MSRT_BIT2;   //clk x2
		}
		break;
	case DVOUT_CS_MODE_YUV422:
		switch (t_dvout_config->u8_bw_mode) {
		case DVOUT_BW_MODE_16_24BIT:
			//HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS2_1, MSRT_BIT1);   //16 bit
			ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BITS2_1, MSRT_BIT1);
			//HAL_WriteByte(REG_DVO_CTRL_3, 0x26);
			ms_write8(sd, REG_DVO_CTRL_3, 0x26);
			break;
		case DVOUT_BW_MODE_8_12BIT:
			//HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS2_1, MSRT_BIT2);   //8 bit
			ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BITS2_1, MSRT_BIT2);
			//HAL_WriteByte(REG_DVO_CTRL_3, 0x2A);
			ms_write8(sd, REG_DVO_CTRL_3, 0x2A);
			//HAL_SetBits(REG_DVO_CTRL_4, MSRT_BIT4);   //dvo_yuv444to422_sel_div2
			ms_write8_mask(sd, REG_DVO_CTRL_4, MSRT_BIT4, MSRT_BIT4);
			u8_clk_ratio = MSRT_BIT2;   //clk x2
			break;
		}
		break;
	}

	// dvo_ddr_mode_sel
	switch (t_dvout_config->u8_dr_mode) {
	case DVOUT_DR_MODE_SDR:
		//HAL_ClrBits(REG_DVO_CTRL_4, MSRT_BIT5);   //sdr
		break;
	case DVOUT_DR_MODE_DDR:
		//HAL_SetBits(REG_DVO_CTRL_4, MSRT_BIT5);   //ddr
		u8_clk_ratio |= MSRT_BIT3;   //clk div2
		break;
	}
	//HAL_ModBits(REG_DIG_CLK_SEL0, MSRT_BITS3_2, u8_clk_ratio);
	ms_write8_mask(sd, REG_DIG_CLK_SEL0, MSRT_BITS3_2, u8_clk_ratio);

	// dvo_bt1120_en, dvo_bt656_en
	switch (t_dvout_config->u8_sy_mode) {
	case DVOUT_SY_MODE_HSVSDE:
	case DVOUT_SY_MODE_BTAT1004:
		//HAL_ClrBits(REG_DVO_CTRL_2, MSRT_BITS4_3);
		ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BITS4_3, 0x00);
		break;
	case DVOUT_SY_MODE_EMBANDED:
		//HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS4_3, MSRT_BIT3);
		ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BITS4_3, MSRT_BIT3);
		break;
	case DVOUT_SY_MODE_2XEMBANDED:
		//HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS4_3, MSRT_BIT4);
		ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BITS4_3, MSRT_BIT4);
		break;
	}
	// r_16b_1004_en
	//HAL_ToggleBits(REG_DVO_CTRL_5, MSRT_BIT0, t_dvout_config->u8_sy_mode == DVOUT_SY_MODE_BTAT1004);
	ms_write8_mask(sd, REG_DVO_CTRL_5, MSRT_BIT0, t_dvout_config->u8_sy_mode == DVOUT_SY_MODE_BTAT1004 ? MSRT_BIT0 : 0x00);

	//ms_write8(sd, REG_DVO_CTRL_2, 0x23);//addr:0xe2 must be val:0x23
}

VOID ms7200_dvout_init(struct v4l2_subdev *sd, DVOUT_CONFIG_T *t_dvout_config, bool b_spdif_out)
{
	//DVOUT_CS_MODE_E e_cs_mode = (DVOUT_CS_MODE_E)t_dvout_config->u8_cs_mode;
	/*if (e_cs_mode == DVOUT_CS_MODE_YUV422) {
		e_cs_mode = DVOUT_CS_MODE_YUV444;
	}*/
	//ms7200drv_csc_config_output(e_cs_mode);
	//ms_write8_mask(sd, REG_CSC_CTRL1, MSRT_BITS4_2, 0x10 | ((UINT8)e_cs_mode << 2)); //csc reg 0xf0 val=0x19
	//csc todo...
	MS_INF("%s(line=%d) csc reg addr:0xf0 set value=0x00\n", __func__, __LINE__);
	ms_write8(sd, REG_CSC_CTRL1, 0x00);

	ms7200drv_dvout_mode_config(sd, t_dvout_config);
	//ms7200drv_misc_audio_pad_output_spdif(b_spdif_out);
	ms_write8_mask(sd, REG_PINMUX, MSRT_BIT2 | MSRT_BIT0, b_spdif_out ? (MSRT_BIT2 | MSRT_BIT0) : 0x00);
	//ms7200drv_misc_audio_mclk_pad_enable(TRUE);
	ms_write8_mask(sd, REG_AUDIO_INPUT_OUTPUT_CNTRL, MSRT_BIT4, MSRT_BIT4);
	//ms7200_dvout_clk_driver_set(0);
	ms_write8_mask(sd, REG_MISC_DRIV, MSRT_BITS1_0, 0);
}

VOID ms7200drv_hdmi_rx_controller_hpd_set(struct v4l2_subdev *sd, bool bReady)
{
#if MS7200_HDMI_RX_EDID_ENABLE
	//hdmi rx EDID reset release
	//ms7200drv_hdmi_rx_edid_reset_release(bReady);
	ms_write8_mask(sd, REG_MISC_RST_CTRL1, MSRT_BIT2, bReady ? MSRT_BIT2 : 0x00);
#endif
	//HAL_ToggleBits_Ex(REG_HDMI_SETUP_CTRL, MSRT_BIT0, bReady);
	ms_write32_mask(sd, REG_HDMI_SETUP_CTRL, MSRT_BIT0, bReady ? MSRT_BIT0 : 0x00);
}

VOID ms7200drv_hdmi_rx_edid_config(struct v4l2_subdev *sd, UINT8 *u8Edid)
{
	//UINT16 u16Range = (u8Ext > 3) ? 0x200 : (0x80 * (u8Ext + 1));
	//HAL_SetBits(REG_MISC_RX_PHY_SEL, MSRT_BIT7);
	ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BIT7, MSRT_BIT7);
	//HAL_WriteBytes(REG_EDID_BLOCK0, 0x100, u8Edid);
	ms_write8_regs(sd, REG_EDID_BLOCK0, 0x100, u8Edid);
	//HAL_ClrBits(REG_MISC_RX_PHY_SEL, MSRT_BIT7);
	ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BIT7, 0x00);
}

bool ms7200drv_hdmi_rx_controller_hpd_get(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_HDMI_SETUP_CTRL) & MSRT_BIT0;
	return ms_read32(sd, REG_HDMI_SETUP_CTRL) & MSRT_BIT0;
}

VOID ms7200_hdmirx_hpd_config(struct v4l2_subdev *sd, bool b_config, UINT8 *u8_edid)
{
	if (u8_edid != NULL) {
		ms7200drv_hdmi_rx_controller_hpd_set(sd, FALSE);
		ms7200drv_hdmi_rx_edid_config(sd, u8_edid);
	}

	if (b_config != ms7200drv_hdmi_rx_controller_hpd_get(sd)) {
		if (b_config) {
			//Delay_ms(200);
			mdelay(200);
		}
		ms7200drv_hdmi_rx_controller_hpd_set(sd, b_config);
	}
}

bool ms7200drv_misc_chipisvalid(struct v4l2_subdev *sd)
{
	UINT8 u8_chipid1;

	//u8_chipid1 = HAL_ReadByte(REG_CHIPID1_REG);
	u8_chipid1 = ms_read8(sd, REG_CHIPID1_REG);

	if (u8_chipid1 == MS7200_CHIP_ID1) {
		return TRUE;
	}

	return FALSE;
}

UINT8 ms7200drv_misc_package_sel_get(struct v4l2_subdev *sd)
{
	//UINT8 u8_package_sel = (HAL_ReadByte(REG_BDOP_REG) & MSRT_BITS3_2) >> 2;
	UINT8 u8_package_sel = (ms_read8(sd, REG_BDOP_REG) & MSRT_BITS3_2) >> 2;
	return u8_package_sel;
}

bool ms7200_chip_connect_detect(struct v4l2_subdev *sd)
{
	//UINT8 i;

	if (ms7200drv_misc_package_sel_get(sd) == 0x01) {
		if (ms7200drv_misc_chipisvalid(sd))
			return TRUE;
	}
	return FALSE;
}

VOID ms7200drv_dvout_data_swap_all(struct v4l2_subdev *sd)
{
	//UINT8 u8_swap = HAL_ReadByte(REG_DVO_CTRL_2) & MSRT_BIT7;
	UINT8 u8_swap = ms_read8(sd, REG_DVO_CTRL_2) & MSRT_BIT7;
	//HAL_ToggleBits(REG_DVO_CTRL_2, MSRT_BIT7, !u8_swap);
	ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BIT7, !u8_swap ? MSRT_BIT7 : 0x00);  //addr:0xe2=0x8b
}

VOID ms7200drv_dvout_data_swap_rb_channel(struct v4l2_subdev *sd)
{
	//UINT8 u8_map = HAL_ReadByte(REG_DVO_CTRL_3) & MSRT_BITS5_0;
	UINT8 u8_map = ms_read8(sd, REG_DVO_CTRL_3) & MSRT_BITS5_0;
	//HAL_ModBits(REG_DVO_CTRL_3, MSRT_BITS5_0, ((u8_map & MSRT_BITS1_0) << 4)
	//			| (u8_map & MSRT_BITS3_2) | ((u8_map & MSRT_BITS5_4) >> 4));
	ms_write8_mask(sd, REG_DVO_CTRL_3, MSRT_BITS5_0, ((u8_map & MSRT_BITS1_0) << 4)
				   | (u8_map & MSRT_BITS3_2) | ((u8_map & MSRT_BITS5_4) >> 4));
}

VOID ms7200drv_dvout_data_swap_yc_channel(struct v4l2_subdev *sd)
{
	//UINT8 u8_map = HAL_ReadByte(REG_DVO_CTRL_3) & MSRT_BITS5_0;
	UINT8 u8_map = ms_read8(sd, REG_DVO_CTRL_3) & MSRT_BITS5_0;
	//HAL_ModBits(REG_DVO_CTRL_3, MSRT_BITS5_0, ((u8_map & MSRT_BITS3_2) << 2)
	//			| ((u8_map & MSRT_BITS1_0) << 2) | ((u8_map & MSRT_BITS3_2) >> 2));
	ms_write8_mask(sd, REG_DVO_CTRL_3, MSRT_BITS5_0, ((u8_map & MSRT_BITS3_2) << 2) \
					| ((u8_map & MSRT_BITS1_0) << 2) | ((u8_map & MSRT_BITS3_2) >> 2));//addr:0xe2=0x19
}

VOID ms7200_dvout_data_swap(struct v4l2_subdev *sd, UINT8 u8_swap_mode)
{
	switch (u8_swap_mode) {
	case 0:
		ms7200drv_dvout_data_swap_all(sd);
		break;
	case 1:
		ms7200drv_dvout_data_swap_rb_channel(sd);
		break;
	case 2:
		ms7200drv_dvout_data_swap_yc_channel(sd);
	}
}

VOID ms7200drv_dvout_pa_adjust(struct v4l2_subdev *sd, bool b_invert, UINT8 u8_delay)
{
	//HAL_ModBits(REG_PA_S, MSRT_BITS2_0, ((UINT8)b_invert << 2) | (u8_delay % 4));
	ms_write8_mask(sd, REG_PA_S, MSRT_BITS2_0, ((UINT8)b_invert << 2) | (u8_delay % 4));
}

VOID ms7200_dvout_phase_adjust(struct v4l2_subdev *sd, bool b_invert, UINT8 u8_delay)
{
	ms7200drv_dvout_pa_adjust(sd, b_invert, u8_delay);
}

VOID ms7200_init(struct v4l2_subdev *sd)
{
	MS_INF("ms7200 chip connect = %d\n", ms7200_chip_connect_detect(sd));
	ms7200_hdmirx_init(sd);

#if MS7200_DVO_16BIT
	ms7200_dvout_init(sd, &g_t_dvout_config_16bit, 0);
#else
	ms7200_dvout_init(sd, &g_t_dvout_config_8bit, 0);
#endif

	ms7200_hdmirx_hpd_config(sd, FALSE, u8_sys_edid_default_buf);
	/******modify phase******/
	//ms7200_dvout_phase_adjust(sd, 0, 1);//sdr mode  bit0~bit2=001
	ms7200_dvout_phase_adjust(sd, 1, 1);//sdr mode  bit0~bit2=101

	//ms7200_dvout_data_swap(sd, 2);
}

void ms7200_i2c_test(struct v4l2_subdev *sd)
{
	UINT8 ValueBurst[4];
	UINT8 ValueBurst1[4];
	UINT8 ValueBurst2[4];

	MS_INF("read   addr:0x%x = 0x%x\n", 0x0000, ms_read8(sd, 0x0000));
	MS_INF("read   addr:0x%x = 0x%x\n", 0x0001, ms_read8(sd, 0x0001));
	MS_INF("read   addr:0x%x = 0x%x\n", 0x0002, ms_read8(sd, 0x0002));
	MS_INF("read   addr:0x%x = 0x%x\n", 0x1000, ms_read8(sd, 0x1000));
	MS_INF("read   addr:0x%x = 0x%x\n", 0x1001, ms_read8(sd, 0x1001));

	ms_write8(sd, 0x0003, 0x5A);
	MS_INF("write   addr:0x%x = 0x%x\n", 0x0003, 0x5A);
	MS_INF("read   addr:0x%x = 0x%x\n", 0x0003, ms_read8(sd, 0x0003));
	ms_write8(sd, 0x1000, 0x5A);
	MS_INF("write	addr:0x%x = 0x%x\n", 0x1000, 0x5A);
	MS_INF("read   addr:0x%x = 0x%x\n", 0x1000, ms_read8(sd, 0x1000));

	ms_read8_regs(sd, 0x1000, 4, ValueBurst);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x1000, ValueBurst[0]);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x1001, ValueBurst[1]);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x1002, ValueBurst[2]);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x1003, ValueBurst[3]);

	ValueBurst1[0] = 0x5A;
	ValueBurst1[1] = 0xA5;
	ValueBurst1[2] = 0x69;
	ValueBurst1[3] = 0x96;
	ms_write8_regs(sd, 0x20B0, 4, ValueBurst1);
	ms_read8_regs(sd, 0x20B0, 4, ValueBurst2);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x20B0, ValueBurst2[0]);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x20B1, ValueBurst2[1]);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x20B2, ValueBurst2[2]);
	MS_INF("read	addr:0x%x = 0x%x\n", 0x20B3, ValueBurst2[3]);
}

int set_capture2(struct v4l2_subdev *sd,
			unsigned char *edid,
			unsigned int blocks)
{
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	if (ms7200_power_on_first_flag == 1) {
		ms7200_power_on_first_flag = 0;

		/*while (1) {
			ms7200_i2c_test(sd);
			ms_msleep(100);
		}*/

		ms->ms_task = kthread_create(ms_run_thread, (void *)sd, "ms7200_proc");
		if (IS_ERR(ms->ms_task)) {
				 MS_INF("Unable to start kernel thread ms7200_proc\n");
				ms->ms_task = NULL;
				 return -1;
		}
		wake_up_process(ms->ms_task);
	} else {
		MS_INF("ms7200_power_on_first_flag = %d\n", ms7200_power_on_first_flag);
	}
	return 0;
}

UINT32 ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status(struct v4l2_subdev *sd, UINT32 u32_mask)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_PDEC_ISTS);
	UINT32 u32_value = ms_read32(sd, REG_PDEC_ISTS);
	if (u32_value & u32_mask) {
		//clr bit
		//HAL_WriteDWord_Ex(REG_PDEC_ICLR, u32_value & u32_mask);
		ms_write32(sd, REG_PDEC_ICLR, u32_value & u32_mask);
	}

	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status_ext(struct v4l2_subdev *sd, UINT32 u32_mask)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_PDEC_ISTS);
	UINT32 u32_value = ms_read32(sd, REG_PDEC_ISTS);
	return (u32_value & u32_mask);
}

UINT16 ms7200drv_hdmi_rx_get_mdt_interrupt_status(struct v4l2_subdev *sd, UINT16 u16_mask)
{
	//UINT16 u16_value = HAL_ReadDWord_Ex(REG_MD_ISTS) & MDT_VALID_ISTS;
	UINT16 u16_value = ms_read32(sd, REG_MD_ISTS) & MDT_VALID_ISTS;
	if (u16_value & u16_mask) {
		//clr bit
		//HAL_WriteDWord_Ex(REG_MD_ICLR, u16_value & u16_mask);
		ms_write32(sd, REG_MD_ICLR, u16_value & u16_mask);
	}

	return (u16_value & u16_mask);
}

UINT16 ms7200drv_hdmi_rx_get_mdt_interrupt_status_ext(struct v4l2_subdev *sd, UINT16 u16_mask)
{
	//return HAL_ReadDWord_Ex(REG_MD_ISTS) & MDT_VALID_ISTS & u16_mask;
	return ms_read32(sd, REG_MD_ISTS) & MDT_VALID_ISTS & u16_mask;
}

UINT32 ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status(struct v4l2_subdev *sd, UINT32 u32_mask)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_HDMI_ISTS);
	UINT32 u32_value = ms_read32(sd, REG_HDMI_ISTS);
	if (u32_value & u32_mask) {
		//clr bit
		//HAL_WriteDWord_Ex(REG_HDMI_ICLR, u32_value & u32_mask);
		ms_write32(sd, REG_HDMI_ICLR, u32_value & u32_mask);
	}

	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status_ext(struct v4l2_subdev *sd, UINT32 u32_mask)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_HDMI_ISTS);
	UINT32 u32_value = ms_read32(sd, REG_HDMI_ISTS);
	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status(struct v4l2_subdev *sd, UINT32 u32_mask)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_AFIFO_ISTS);
	UINT32 u32_value = ms_read32(sd, REG_AFIFO_ISTS);
	if (u32_value & u32_mask) {
		//clr bit
		//HAL_WriteDWord_Ex(REG_AFIFO_ICLR, u32_value & u32_mask);
		ms_write32(sd, REG_AFIFO_ICLR, u32_value & u32_mask);
	}

	return (u32_value & u32_mask);
}

UINT32 ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status_ext(struct v4l2_subdev *sd, UINT32 u32_mask)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_AFIFO_ISTS);
	UINT32 u32_value = ms_read32(sd, REG_AFIFO_ISTS);
	return (u32_value & u32_mask);
}

UINT8 ms7200drv_hdmi_rx_5v_interrupt_get_status(struct v4l2_subdev *sd, UINT8 u8_int_flag)
{
	//UINT8 u8_int_status = HAL_ReadByte(REG_MISC_5V_DET) & MSRT_BITS3_2 & u8_int_flag;
	UINT8 u8_int_status = ms_read8(sd, REG_MISC_5V_DET) & MSRT_BITS3_2 & u8_int_flag;
	//HAL_ClrBits(REG_MISC_5V_DET, MSRT_BIT1);
	ms_write8_mask(sd, REG_MISC_5V_DET, MSRT_BIT1, 0x00);
	return u8_int_status;
}

UINT8 ms7200drv_hdmi_rx_5v_interrupt_get_status_ext(struct v4l2_subdev *sd, UINT8 u8_int_flag)
{
	//return HAL_ReadByte(REG_MISC_5V_DET) & MSRT_BITS3_2 & u8_int_flag;
	return ms_read8(sd, REG_MISC_5V_DET) & MSRT_BITS3_2 & u8_int_flag;
}

UINT32 ms7200_hdmirx_interrupt_get_and_clear(struct v4l2_subdev *sd, HDMIRX_INT_INDEX_E e_int_index,
												UINT32 u32_int_flag, bool b_clr)
{
	UINT32 u32_interrupt_status = 0;

	switch (e_int_index) {
	case RX_INT_INDEX_PDEC:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status(sd, u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_pdec_interrupt_get_status_ext(sd, u32_int_flag);
		break;
	case RX_INT_INDEX_MDT:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_get_mdt_interrupt_status(sd, (UINT16)u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_get_mdt_interrupt_status_ext(sd, u32_int_flag);
		break;
	case RX_INT_INDEX_HDMI:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status(sd, u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_hdmi_interrupt_get_status_ext(sd, u32_int_flag);
		break;
	case RX_INT_INDEX_AUDFIFO:
		if (b_clr)
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status(sd, u32_int_flag);
		else
			u32_interrupt_status = ms7200drv_hdmi_rx_controller_audio_fifo_interrupt_get_status_ext(sd, u32_int_flag);
		break;
	case RX_INT_INDEX_5V:
		if (b_clr)
			ms7200drv_hdmi_rx_5v_interrupt_get_status(sd, u32_int_flag);
		else
			ms7200drv_hdmi_rx_5v_interrupt_get_status_ext(sd, u32_int_flag);
		break;
	}

	return u32_interrupt_status;
}

UINT16 ms7200drv_hdmi_rx_get_tmds_clk(struct v4l2_subdev *sd)
{
	//UINT32 u32_value = HAL_ReadDWord_Ex(REG_HDMI_CKM_RESULT);
	UINT32 u32_value = ms_read32(sd, REG_HDMI_CKM_RESULT);

	//clk stable detect. detect 200us once time
	if ((u32_value >> 17) & MSRT_BIT0) {
		u32_value &= 0xffff;
		return (UINT16)(u32_value * (MS7200_EXT_XTAL / 10000) / 0xfff);
	}

	return 0;
}

UINT16 ms7200_hdmirx_input_clk_get(struct v4l2_subdev *sd)
{
	return ms7200drv_hdmi_rx_get_tmds_clk(sd);
}

static int _abs(int i)
{ /* compute absolute value of int argument */
	return (i < 0 ? -i : i);
}

VOID ms7200drv_misc_audio_out_pad_enable(struct v4l2_subdev *sd, bool b_enable)
{
	//HAL_ToggleBits(REG_AUDIO_INPUT_OUTPUT_CNTRL, MSRT_BIT2, b_enable);
	ms_write8_mask(sd, REG_AUDIO_INPUT_OUTPUT_CNTRL, MSRT_BIT2, b_enable ? MSRT_BIT2 : 0x00);
}

VOID ms7200_dvout_audio_config(struct v4l2_subdev *sd, bool b_config)
{
	ms7200drv_misc_audio_out_pad_enable(sd, b_config);
}

VOID ms7200drv_dvout_clk_reset_release(struct v4l2_subdev *sd, bool b_release)
{
	if (b_release) {
		//HAL_SetBits(REG_TEST_SEL, MSRT_BIT1);
		ms_write8_mask(sd, REG_TEST_SEL, MSRT_BIT1, MSRT_BIT1);
		//HAL_SetBits(REG_RST_CTRL0, MSRT_BIT1);
		ms_write8_mask(sd, REG_RST_CTRL0, MSRT_BIT1, MSRT_BIT1);
	} else {
		//HAL_ClrBits(REG_RST_CTRL0, MSRT_BIT1);
		ms_write8_mask(sd, REG_RST_CTRL0, MSRT_BIT1, 0x00);
		//HAL_ClrBits(REG_TEST_SEL, MSRT_BIT1);
		ms_write8_mask(sd, REG_TEST_SEL, MSRT_BIT1, 0x00);
	}
	//HAL_ToggleBits(REG_DVO_CTRL_2, MSRT_BIT0, b_release);
	ms_write8_mask(sd, REG_DVO_CTRL_2, MSRT_BIT0, b_release ? MSRT_BIT0 : 0x00);
	//ms_write8(sd, REG_DVO_CTRL_2, 0x23);//addr:0xe2 must be val:0x23
}

VOID ms7200_dvout_video_config(struct v4l2_subdev *sd, bool b_config)
{
	ms7200drv_dvout_clk_reset_release(sd, b_config);
}

VOID ms7200drv_hdmi_rx_controller_hdcp_data_enable(struct v4l2_subdev *sd, bool b_enable)
{
	//HAL_ModBits_Ex(REG_HDMI_SYNC_CTRL, 0x30000000UL, (UINT32)(b_enable ? 0x10UL : 0x20UL) << 24);
	ms_write32_mask(sd, REG_HDMI_SYNC_CTRL, 0x30000000UL, (UINT32)(b_enable ? 0x10UL : 0x20UL) << 24);
}

VOID ms7200drv_hdmi_rx_controller_reset(struct v4l2_subdev *sd, UINT32 eModule)
{
	//HAL_WriteDWord_Ex(REG_SW_RST, eModule);
	ms_write32(sd, REG_SW_RST, eModule);
}

VOID ms7200drv_hdmi_rx_hdcp_detect_enable(struct v4l2_subdev *sd, bool bEnable)
{
	ms7200drv_hdmi_rx_controller_hdcp_data_enable(sd, bEnable);

	//20200831, new design for HDMI video reset only.
	// resolve HDMI RX error counter maybe working abnormity
	if (!bEnable)
		ms7200drv_hdmi_rx_controller_reset(sd, HDMI_RX_CTRL_HDCP);
}

VOID ms7200drv_hdmi_rx_pi_rxpll_trigger(struct v4l2_subdev *sd)
{
	//HAL_WriteByte(REG_PI_RX_PLL_CAL_TRIG, MSRT_BIT0);
	ms_write8(sd, REG_PI_RX_PLL_CAL_TRIG, MSRT_BIT0);
}

static VOID _drv_hdmi_rx_pi_pll_kband_enable(struct v4l2_subdev *sd, bool bEnable)
{
	if (bEnable) {
		//HAL_ToggleBits(REG_PI_RXPLL_CTRL0, MSRT_BIT0, !bEnable);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BIT0, !bEnable ? MSRT_BIT0 : 0x00);
	}
	//HAL_ToggleBits(REG_PI_RXPLL_CTRL4, MSRT_BIT2, bEnable);
	ms_write8_mask(sd, REG_PI_RXPLL_CTRL4, MSRT_BIT2, bEnable ? MSRT_BIT2 : 0x00);
	//HAL_ToggleBits(REG_PI_RXPLL_CTRL1, MSRT_BIT0, bEnable);
	ms_write8_mask(sd, REG_PI_RXPLL_CTRL1, MSRT_BIT0, bEnable ? MSRT_BIT0 : 0x00);
	//Delay_us(10);
	udelay(10);
	//HAL_ToggleBits(REG_PI_RXPLL_CTRL3, MSRT_BIT7, bEnable);
	ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BIT7, bEnable ? MSRT_BIT7 : 0x00);
}

static UINT16 _drv_hdmi_rx_pi_rxpll_cal_ref_freq_get(struct v4l2_subdev *sd)
{
	//return (HAL_ReadWord(REG_PI_CAL_REF_FREQ) & 0x3fff);
	return (ms_read16(sd, REG_PI_CAL_REF_FREQ) & 0x3fff);
}

static VOID _drv_hdmi_rx_pi_rxpll_lookup_table(struct v4l2_subdev *sd, UINT16 u16TmdsClk)
{
	UINT8 u8Gear = (u16TmdsClk > 50) ? ((u16TmdsClk > 100) ? 2 : 1) : 0;
	switch (u8Gear) {
	case 0:
		//HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x03);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x03);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		//HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x06 << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x06 << 4);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		break;
	case 1:
		//HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x01);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x01);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		//HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x02 << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x02 << 4);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		break;
	case 2:
		//HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x00);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, 0x00);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x02);
		//HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x06 << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, 0x06 << 4);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS6_4, 0x06 << 4);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS3_2, 0x01 << 2);
		break;
	default:
		break;
	}
}

static UINT16 _drv_hdmi_rx_pi_rxpll_iband(struct v4l2_subdev *sd, UINT8 u8Iband)
{
	//HAL_ModBits(REG_PI_RXPLL_CTRL2, MSRT_BITS5_0, u8Iband);
	ms_write8_mask(sd, REG_PI_RXPLL_CTRL2, MSRT_BITS5_0, u8Iband);
	//HAL_SetBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
	ms_write8_mask(sd, REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1, MSRT_BIT1);
	//HAL_ClrBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
	ms_write8_mask(sd, REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1, 0x00);
	//Delay_us(30);
	udelay(30);
	//return (HAL_ReadWord(REG_PI_CAL_FREQ) & 0x3fff);
	return (ms_read16(sd, REG_PI_CAL_FREQ) & 0x3fff);
}

static bool _drv_hdmi_rx_pi_rxpll_config(struct v4l2_subdev *sd, e_RxPhyMode eMode, UINT16 u16TmdsClk)
{
	//UINT16 u16TmdsFreq;
	//UINT16 u16TmdsClk;
	UINT16 u16CalRefFreq = 0;
	UINT16 u16CalFreq = 0;
	UINT8 u8Iband = 0;
	INT8 i;
	//UINT8 u8PreDiv;
	UINT8 u8FreqSet;
	UINT8 u8IchpSel;

	//u16TmdsClk /= 100;

	switch (eMode) {
	case AUTO_MODE:
		//RG_RXPLL_KBAND_DIV2_EN: to fix hardware bug
		//HAL_ToggleBits(REG_PI_RXPLL_CTRL5, MSRT_BIT2, u16TmdsClk > 5000);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL5, MSRT_BIT2, u16TmdsClk > 5000 ? MSRT_BIT2 : 0x00);

		//increase rxpll bandwidth, to resolve inphic display problem
#if VIN_TRUE
		//HAL_ClrBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BIT2);   //look up table auto mode
		ms_write8_mask(sd, REG_PI_RX_PLL_CFG_SEL, MSRT_BIT2, 0x00);   //look up table auto mode
		//lock flag will be clear after trigger
		ms7200drv_hdmi_rx_pi_rxpll_trigger(sd);
		//Delay_ms(1);
		mdelay(1);
#if VIN_FALSE
		u16TmdsFreq = _drv_hdmi_rx_pi_rxpll_tmdsfreq_get();
		MS7200_LOG1("u16TmdsFreq = ", u16TmdsFreq);
		u16TmdsClk = ((UINT32)u16TmdsFreq * (MS7200_EXT_XTAL / 1000) / 256) / 1000;
		MS7200_LOG2("u16TmdsClk = ", u16TmdsClk);
#endif
		//u8PreDiv = (HAL_ReadByte(REG_PI_PLL_CAL_RESULT0) & MSRT_BITS7_6) >> 6;
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, u8PreDiv);
		//HAL_WriteByte(REG_PI_RXPLL_CTRL3, 0x14);
		ms_write8(sd, REG_PI_RXPLL_CTRL3, 0x14);
		//u8FreqSet = (HAL_ReadByte(REG_PI_PLL_CAL_RESULT2) & MSRT_BITS2_1) >> 1;
		u8FreqSet = (ms_read8(sd, REG_PI_PLL_CAL_RESULT2) & MSRT_BITS2_1) >> 1;
		//<=50M: 4X clk   <=150M: 2X clk
		if (u16TmdsClk <= 5000)
			u8FreqSet = 0x3;
		else if (u16TmdsClk <= 15000)
			u8FreqSet = 0x01;
		//HAL_ModBits(REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, u8FreqSet);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL4, MSRT_BITS1_0, u8FreqSet);

		//u8IchpSel = HAL_ReadByte(REG_PI_PLL_CAL_RESULT1) & MSRT_BITS3_0;
		u8IchpSel = ms_read8(sd, REG_PI_PLL_CAL_RESULT1) & MSRT_BITS3_0;
		//u8IchpSel = u8IchpSel << ((u8PreDiv != 0) ? 0 : 1);   // >100M: move 1-bit to left
		//20190530, 50~100MHz, ICHP change from 4 to 8.
		//u8IchpSel = (u8IchpSel == 4) ? 8 : u8IchpSel;
		//HAL_ModBits(REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, u8IchpSel << 4);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BITS7_4, u8IchpSel << 4);
		//HAL_WriteByte(REG_PI_CTRL0, 0x02);
		ms_write8(sd, REG_PI_CTRL0, 0x02);
		//HAL_SetBits(REG_PI_RX_PLL_CFG_SEL, MSRT_BIT2);   //look up table manual mode
		ms_write8_mask(sd, REG_PI_RX_PLL_CFG_SEL, MSRT_BIT2, MSRT_BIT2);   //look up table manual mode
#endif
		ms7200drv_hdmi_rx_pi_rxpll_trigger(sd);
		break;

	case MANUAL_MODE:
		_drv_hdmi_rx_pi_pll_kband_enable(sd, TRUE);
		//HAL_SetBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
		ms_write8_mask(sd, REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1, MSRT_BIT1);
		//HAL_ClrBits(REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1);
		ms_write8_mask(sd, REG_PI_RX_PLL_FREQM_CTRL, MSRT_BIT1, 0x00);
		//Delay_ms(1);
		mdelay(1);
		u16CalRefFreq = _drv_hdmi_rx_pi_rxpll_cal_ref_freq_get(sd);
		MS_INF("u16CalRefFreq = %d\n", u16CalRefFreq);
		_drv_hdmi_rx_pi_rxpll_lookup_table(sd, u16TmdsClk / 100);
		//HAL_ModBits(REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x00);
		do {
			u8Iband += 0x10;
			if (u8Iband == 0x40)
				break;
			u16CalFreq = _drv_hdmi_rx_pi_rxpll_iband(sd, u8Iband);
#if RXPHY_DEBUG_LOG
			MS_INF("u8Iband = %d\n", u8Iband);
			MS_INF("  u16CalFreq = %d\n", u16CalFreq);
#endif
		} while (u16CalFreq < u16CalRefFreq);
		u8Iband -= 0x10;
		for (i = 3; i >= 0; i--) {
			u8Iband |= (1 << i);
			u16CalFreq = _drv_hdmi_rx_pi_rxpll_iband(sd, u8Iband);

#if RXPHY_DEBUG_LOG
			MS_INF("u8Iband = %d\n", u8Iband);
			MS_INF("  u16CalFreq = %d\n", u16CalFreq);
#endif
			if (u16CalFreq > u16CalRefFreq)
				u8Iband -= (1 << i);
		}
		u16CalFreq = _drv_hdmi_rx_pi_rxpll_iband(sd, u8Iband);
		MS_INF("Choose u8Iband = %d\n", u8Iband);
		MS_INF("  u16CalFreq = %d\n", u16CalFreq);

		if (u16TmdsClk > 10000)
			ms_write8_mask(sd, REG_PI_RXPLL_CTRL3, MSRT_BITS1_0, 0x02);
		_drv_hdmi_rx_pi_pll_kband_enable(sd, FALSE);
		break;

	default:
		break;
	}
	//delay for PLL stable
	//Delay_ms(1);
	mdelay(1);
	return TRUE;
}

static VOID _drv_hdmi_rx_pi_mixer_config(struct v4l2_subdev *sd, UINT16 u16TmdsClk)
{
	u16TmdsClk /= 100;
	//HAL_WriteByte(REG_RX_PI_CTRL5, 0x00);
	ms_write8(sd, REG_RX_PI_CTRL5, 0x00);
	//HAL_ClrBits(REG_PI_CDR_RST, MSRT_BITS2_0);
	ms_write8_mask(sd, REG_PI_CDR_RST, MSRT_BITS2_0, 0x00);
	if (u16TmdsClk <= 75) {
		//HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x2 << 4);
		//HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		//HAL_ModBits(REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x2);
		//HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x1 << 4);
		//HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x4);
		//HAL_ModBits(REG_RX_PI_CTRL4, MSRT_BITS3_0, 0x4);
		ms_write8_mask(sd, REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x2 << 4);
		ms_write8_mask(sd, REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		ms_write8_mask(sd, REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x2);
		ms_write8_mask(sd, REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x1 << 4);
		ms_write8_mask(sd, REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x4);
		ms_write8_mask(sd, REG_RX_PI_CTRL4, MSRT_BITS3_0, 0x4);
	} else if (u16TmdsClk <= 150) {
		//HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x1 << 4);
		//HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		//HAL_ModBits(REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x4);
		//HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x0 << 4);
		//HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x4);
		//HAL_ModBits(REG_RX_PI_CTRL4, MSRT_BITS3_0, 0xc);
		ms_write8_mask(sd, REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x1 << 4);
		ms_write8_mask(sd, REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		ms_write8_mask(sd, REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x4);
		ms_write8_mask(sd, REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x0 << 4);
		ms_write8_mask(sd, REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x4);
		ms_write8_mask(sd, REG_RX_PI_CTRL4, MSRT_BITS3_0, 0xc);
	} else {
		//HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x0 << 4);
		//HAL_ModBits(REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		//HAL_ModBits(REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x9);
		//HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x0 << 4);
		//HAL_ModBits(REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x0);
		//HAL_ModBits(REG_RX_PI_CTRL4, MSRT_BITS3_0, 0xc);
		ms_write8_mask(sd, REG_RX_PI_CTRL1, MSRT_BITS5_4, 0x0 << 4);
		ms_write8_mask(sd, REG_RX_PI_CTRL1, MSRT_BITS2_0, 0x3);
		ms_write8_mask(sd, REG_RX_PI_CTRL2, MSRT_BITS3_0, 0x9);
		ms_write8_mask(sd, REG_RX_PI_CTRL3, MSRT_BITS5_4, 0x0 << 4);
		ms_write8_mask(sd, REG_RX_PI_CTRL3, MSRT_BITS2_0, 0x0);
		ms_write8_mask(sd, REG_RX_PI_CTRL4, MSRT_BITS3_0, 0xc);
	}
	//HAL_WriteByte(REG_RX_PI_CTRL5, 0x70);
	ms_write8(sd, REG_RX_PI_CTRL5, 0x70);
	//HAL_SetBits(REG_PI_CDR_RST, MSRT_BITS2_0);
	ms_write8_mask(sd, REG_PI_CDR_RST, MSRT_BITS2_0, MSRT_BITS2_0);
}

VOID ms7200drv_hdmi_rx_pi_phy_eq_gain_set(struct v4l2_subdev *sd, e_RxChannel eChannel, UINT8 u8EqGainVal)
{
	switch (eChannel) {
	case HDMI_RX_CH0:
		//HAL_ModBits(REG_PI_RX_GAIN_CTRL1, MSRT_BITS2_0, u8EqGainVal);
		ms_write8_mask(sd, REG_PI_RX_GAIN_CTRL1, MSRT_BITS2_0, u8EqGainVal);
		break;
	case HDMI_RX_CH1:
		//HAL_ModBits(REG_PI_RX_GAIN_CTRL1, MSRT_BITS6_4, u8EqGainVal << 4);
		ms_write8_mask(sd, REG_PI_RX_GAIN_CTRL1, MSRT_BITS6_4, u8EqGainVal << 4);
		break;
	case HDMI_RX_CH2:
		//HAL_ModBits(REG_PI_RX_GAIN_CTRL2, MSRT_BITS2_0, u8EqGainVal);
		ms_write8_mask(sd, REG_PI_RX_GAIN_CTRL2, MSRT_BITS2_0, u8EqGainVal);
		break;
	default:
		break;
	}
}

__maybe_unused static VOID _drv_hdmi_rx_pi_fifo_reset(struct v4l2_subdev *sd)
{
	//HAL_WriteByte(REG_PI_FIFO_RST, MSRT_BIT0);
	ms_write8(sd, REG_PI_FIFO_RST, MSRT_BIT0);
	//HAL_WriteByte(REG_PI_FIFO_RST, 0x00);
	ms_write8(sd, REG_PI_FIFO_RST, 0x00);
}

static VOID _drv_errdet_clr(struct v4l2_subdev *sd)
{
	//HAL_ToggleBits_Ex(REG_HDMI20_CONTROL, 1UL << 15, TRUE);
	ms_write32_mask(sd, REG_HDMI20_CONTROL, 1U << 15, 1U << 15);
	//HAL_ToggleBits_Ex(REG_HDMI20_CONTROL, 1UL << 15, FALSE);
	ms_write32_mask(sd, REG_HDMI20_CONTROL, 1U << 15, 0x00);
}

static bool _drv_errdet_counter(struct v4l2_subdev *sd, UINT16 *u16ErrCnt)
{
	UINT8 u8Channel;
	UINT8 u8_flag = 0;
	//u16ErrCnt[HDMI_RX_CH0] = HAL_ReadDWord_Ex(REG_SCDC_REGS1) >> 16;
	//u16ErrCnt[HDMI_RX_CH1] = HAL_ReadDWord_Ex(REG_SCDC_REGS2);
	//u16ErrCnt[HDMI_RX_CH2] = HAL_ReadDWord_Ex(REG_SCDC_REGS2) >> 16;
	u16ErrCnt[HDMI_RX_CH0] = ms_read32(sd, REG_SCDC_REGS1) >> 16;
	u16ErrCnt[HDMI_RX_CH1] = ms_read32(sd, REG_SCDC_REGS2);
	u16ErrCnt[HDMI_RX_CH2] = ms_read32(sd, REG_SCDC_REGS2) >> 16;
	for (u8Channel = HDMI_RX_CH0; u8Channel <= 2; u8Channel++) {
		u8_flag <<= 1;
		if (u16ErrCnt[u8Channel] >> 15) {
			u16ErrCnt[u8Channel] &= 0x7fff;
		} else {
			u16ErrCnt[u8Channel] = 0x7fff;
			u8_flag |= 1;
		}
	}

	//chip bug, if dvi input, return FALSE
	return u8_flag == 0x7 ? FALSE : TRUE;
}

bool ms7200drv_hdmi_rx_pi_eq_config(struct v4l2_subdev *sd, UINT8 u8_eq_gain)
{
	bool b_flag = TRUE;
	UINT8 u8Channel;
	UINT8 u8EqGainVal;
	UINT16 u16ErrCnt[3][8];
	UINT16 u16ErrCntTemp[3];
	UINT8 u8ErrMinBand[9];
	UINT16 u16ErrMinGain;

	if (u8_eq_gain != 0xFF)
		goto EQ_MANUAL;

	for (u8EqGainVal = 2; u8EqGainVal < 8; u8EqGainVal++) {
		for (u8Channel = HDMI_RX_CH0; u8Channel <= HDMI_RX_CH2; u8Channel++)
			ms7200drv_hdmi_rx_pi_phy_eq_gain_set(sd, (e_RxChannel)u8Channel, u8EqGainVal);
		//_drv_hdmi_rx_pi_fifo_reset(sd);
		ms_write8(sd, REG_PI_FIFO_RST, MSRT_BIT0);
		ms_write8(sd, REG_PI_FIFO_RST, 0x00);
		//Delay_us(100);
		udelay(100);
		_drv_errdet_clr(sd);
		//Delay_ms(3);
		mdelay(3);
		_drv_errdet_counter(sd, u16ErrCntTemp);
		for (u8Channel = HDMI_RX_CH0; u8Channel <= 2; u8Channel++) {
			//MS_INF("%s:%d  u8Channel=%d u8EqGainVal=%d u16ErrCntTemp[%d]=%d\n", __func__, __LINE__,
			//	u8Channel, u8EqGainVal, u8Channel, u16ErrCntTemp[u8Channel]);
			u16ErrCnt[u8Channel][u8EqGainVal] = u16ErrCntTemp[u8Channel];
		}
	}

	for (u8Channel = HDMI_RX_CH0; u8Channel <= HDMI_RX_CH2; u8Channel++) {
#if RXPHY_DEBUG_LOG
		MS_INF("u8Channel = %d\n", u8Channel);
#endif
		u16ErrMinGain = 0xFFFF;
		u8ErrMinBand[0] = 0;   //min error count gain number
		for (u8EqGainVal = 2; u8EqGainVal < 8; u8EqGainVal++) {
#if RXPHY_DEBUG_LOG
			MS_INF("  u8EqGainVal = %d\n", u8EqGainVal);
			MS_INF("	  u16ErrCnt = %d\n", u16ErrCnt[u8Channel][u8EqGainVal]);
#endif
			if (u16ErrCnt[u8Channel][u8EqGainVal] < u16ErrMinGain) {
				u16ErrMinGain = u16ErrCnt[u8Channel][u8EqGainVal];
				u8ErrMinBand[0] = 1;
				u8ErrMinBand[u8ErrMinBand[0]] = u8EqGainVal;
			} else if (u16ErrCnt[u8Channel][u8EqGainVal] == u16ErrMinGain) {
				++u8ErrMinBand[0];
				u8ErrMinBand[u8ErrMinBand[0]] = u8EqGainVal;
			}
		}

		if (u16ErrMinGain > 0x100) {
			u8EqGainVal = 4;
			b_flag = FALSE;
		} else if (u8ErrMinBand[0] % 2) {
			u8EqGainVal = u8ErrMinBand[(u8ErrMinBand[0] + 1) / 2];
		} else {
			u8EqGainVal = u8ErrMinBand[u8ErrMinBand[0] / 2];
		}

#if RXPHY_DEBUG_LOG
		MS_INF("  min err = %d\n", u16ErrMinGain);
		MS_INF("  number of min err gain = %d\n", u8ErrMinBand[0]);
		MS_INF("  select eq gain = %d\n", u8EqGainVal);
#endif
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(sd, (e_RxChannel)u8Channel, u8EqGainVal);
	}
	//return b_flag;

EQ_MANUAL:
	if (u8_eq_gain < 0xFE) {
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(sd, HDMI_RX_CH0, u8_eq_gain);
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(sd, HDMI_RX_CH1, u8_eq_gain);
		ms7200drv_hdmi_rx_pi_phy_eq_gain_set(sd, HDMI_RX_CH2, u8_eq_gain);
	}
	//
	//_drv_hdmi_rx_pi_fifo_reset(sd);
	ms_write8(sd, REG_PI_FIFO_RST, MSRT_BIT0);
	ms_write8(sd, REG_PI_FIFO_RST, 0x00);
	//Delay_us(100);
	udelay(100);
	_drv_errdet_clr(sd);

	return b_flag;
}

bool ms7200drv_rxpll_lock_status(struct v4l2_subdev *sd)
{
	//lock or unlock interruput be got
	/*if (HAL_ReadDWord_Ex(REG_HDMI_ISTS) & 0x30) {
		HAL_WriteDWord_Ex(REG_HDMI_ICLR, 0x30);
		return FALSE;
	}

	return (HAL_ReadDWord_Ex(REG_HDMI_CKM_RESULT) >> 16) & MSRT_BIT0;*/
	if (ms_read32(sd, REG_HDMI_ISTS) & 0x30) {
		ms_write32(sd, REG_HDMI_ICLR, 0x30);
		return FALSE;
	}

	return (ms_read32(sd, REG_HDMI_CKM_RESULT) >> 16) & MSRT_BIT0;
}

UINT8 ms7200drv_hdmi_rx_pi_phy_config(struct v4l2_subdev *sd, UINT16 u16TmdsClk, UINT8 u8_eq_gain)
{
	UINT8 u8_flag = 0;

	if (u8_eq_gain == 0xFE) {
		goto RX_FIFO_RESET;
	}

	//HAL_ClrBits(REG_PI_RXPLL_CTRL0, MSRT_BITS2_0);   //power on rxpll
	ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BITS2_0, 0x00);   //power on rxpll

	//20200526, rx_tmds_clk_en = 0.
	//disable rxpll out clk to rx_core when rxpll config.
	//HAL_ModBits(REG_MISC_CLK_CTRL0, 0x83, 0x82);
	ms_write8_mask(sd, REG_MISC_CLK_CTRL0, 0x83, 0x82);
	//HAL_SetBits(REG_MISC_RST_CTRL0, MSRT_BIT5);
	ms_write8_mask(sd, REG_MISC_RST_CTRL0, MSRT_BIT5, MSRT_BIT5);

	//Delay_us(1);
	udelay(1);

	//optimize 1080p config with long line
	if (u16TmdsClk < 20000) {
		//HAL_ModBits(REG_PI_RX_GAIN_CTRL2, MSRT_BITS5_4, 0x3 << 4);
		ms_write8_mask(sd, REG_PI_RX_GAIN_CTRL2, MSRT_BITS5_4, 0x3 << 4);
		//HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS3_2, 0x1 << 2);
		ms_write8_mask(sd, REG_PI_RX_CTRL0, MSRT_BITS3_2, 0x1 << 2);
	} else {
		//HAL_ModBits(REG_PI_RX_GAIN_CTRL2, MSRT_BITS5_4, 0x2 << 4);
		ms_write8_mask(sd, REG_PI_RX_GAIN_CTRL2, MSRT_BITS5_4, 0x2 << 4);
		//HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS3_2, 0x0 << 2);
		ms_write8_mask(sd, REG_PI_RX_CTRL0, MSRT_BITS3_2, 0x0 << 2);
	}
	//HAL_ModBits(REG_PI_RX_CTRL0, MSRT_BITS7_6, 0x3 << 6);
	ms_write8_mask(sd, REG_PI_RX_CTRL0, MSRT_BITS7_6, 0x3 << 6);

	_drv_hdmi_rx_pi_rxpll_config(sd, AUTO_MODE, u16TmdsClk);

	_drv_hdmi_rx_pi_mixer_config(sd, u16TmdsClk);

	//20200526, rx_tmds_clk_en = 1
	//HAL_SetBits(REG_MISC_CLK_CTRL0, 0x01);
	ms_write8_mask(sd, REG_MISC_CLK_CTRL0, 0x01, 0x01);

	//Delay_ms(1);
	mdelay(1);
	//clr rxpll unlock.
	//HAL_WriteDWord_Ex(REG_HDMI_ICLR, 0x30);
	ms_write32(sd, REG_HDMI_ICLR, 0x30);

RX_FIFO_RESET:
	if (ms7200drv_hdmi_rx_pi_eq_config(sd, u8_eq_gain))
		u8_flag |= 0x02;

	//get rxpll lock again
	if (ms7200drv_rxpll_lock_status(sd))
		u8_flag |= 0x01;

	return u8_flag;
}

//b_config_phy_all, TRUE: config rxpll and rxphy EQ; FALSE: config rxpll
static UINT8 _hdmi_rx_phy_config(struct v4l2_subdev *sd, UINT16 *p_u16_tmds_clk, bool b_config_phy_all)
{
	UINT8 u8_times;
	UINT8 u8_flag = 0;
	UINT16 u16_clk1;
	UINT16 u16_clk2;

	u16_clk2 = (*p_u16_tmds_clk);

	//if rxpll unlock when rx config, do again
	for (u8_times = 0; u8_times < 3; u8_times++) {
		u8_flag &= 0xFC;
		u8_flag |= ms7200drv_hdmi_rx_pi_phy_config(sd, u16_clk2, b_config_phy_all ? 0xFF : 0xFE);

		u16_clk1 = ms7200drv_hdmi_rx_get_tmds_clk(sd);
		b_config_phy_all = TRUE;

		//20200520, if detect input clk invalid, return
		if (u16_clk1 < 500) {
			u8_flag |= 0x08;
			(*p_u16_tmds_clk) = u16_clk1;
			break;
		}

		// 5MHz
		if (u16_clk1 >= 500 && _abs(u16_clk1 - u16_clk2) >= 500) {
			u16_clk2 = u16_clk1;
			(*p_u16_tmds_clk) = u16_clk1;
			u8_flag |= 0x04;
		} else {
			if (u8_flag & 0x01)
				break;
		}
	}

	return u8_flag | (u8_times << 4);
}

VOID ms7200drv_hdmi_rx_phy_power_down(struct v4l2_subdev *sd)
{
	/*//rxpll power down
	HAL_SetBits(REG_PI_RXPLL_CTRL0, MSRT_BITS2_0);
	//pi power down
	HAL_SetBits(REG_RX_PI_CTRL5, MSRT_BITS2_0);
	//audio pll power down
	HAL_ClrBits(REG_AUPLL_PWR, MSRT_BIT1);
	HAL_SetBits(REG_AUPLL_PWR, MSRT_BIT0);*/
	//rxpll power down
	ms_write8_mask(sd, REG_PI_RXPLL_CTRL0, MSRT_BITS2_0, MSRT_BITS2_0);
	//pi power down
	ms_write8_mask(sd, REG_RX_PI_CTRL5, MSRT_BITS2_0, MSRT_BITS2_0);
	//audio pll power down
	ms_write8_mask(sd, REG_AUPLL_PWR, MSRT_BIT1, 0x00);
	ms_write8_mask(sd, REG_AUPLL_PWR, MSRT_BIT0, MSRT_BIT0);
}

VOID ms7200drv_hdmi_rx_pi_pll_release(struct v4l2_subdev *sd, bool bEnable)
{
	//HAL_ToggleBits(REG_MISC_CLK_CTRL0, 0x83, bEnable);
	ms_write8_mask(sd, REG_MISC_CLK_CTRL0, 0x83, bEnable ? 0x83 : 0x00);
	//HAL_ToggleBits(REG_MISC_RST_CTRL0, MSRT_BIT5, bEnable);
	ms_write8_mask(sd, REG_MISC_RST_CTRL0, MSRT_BIT5, bEnable ? MSRT_BIT5 : 0x00);
}

UINT8 ms7200_hdmirx_rxphy_config(struct v4l2_subdev *sd, UINT16 *u16_tmds_clk)
{
	UINT8 u8_rxphy_flag = 0;

	*u16_tmds_clk = ms7200drv_hdmi_rx_get_tmds_clk(sd);
	if (*u16_tmds_clk > 500) {
		//20190219, diable rx data
		ms7200drv_hdmi_rx_hdcp_detect_enable(sd, FALSE);
		//config hdmi rx PLL
		u8_rxphy_flag = _hdmi_rx_phy_config(sd, u16_tmds_clk, TRUE) & 0x01;
		u8_rxphy_flag |= 0x80;
		//enable rx data
		ms7200drv_hdmi_rx_hdcp_detect_enable(sd, TRUE);
	} else {
		//20190116, enhace, reset HDMI before rxpll clk disable.
		ms7200drv_hdmi_rx_controller_reset(sd, HDMI_RX_CTRL_HDMI);
		ms7200drv_hdmi_rx_phy_power_down(sd);
		ms7200drv_hdmi_rx_pi_pll_release(sd, FALSE);
	}

	return u8_rxphy_flag;
}


VOID ms7200drv_hdmi_rx_audio_fifo_reset(struct v4l2_subdev *sd, bool b_reset)
{
	//HAL_ToggleBits_Ex(REG_AUD_FIFO_CTRL, MSRT_BIT0, b_reset);
	ms_write32_mask(sd, REG_AUD_FIFO_CTRL, MSRT_BIT0, b_reset ? MSRT_BIT0 : 0x00);

	// toggle aupll_ato_cfg_en, I want to reset pll ACR
	//HAL_ToggleBits(REG_AUPLL_CFG_CTRL, MSRT_BIT0, !b_reset);
	ms_write8_mask(sd, REG_AUPLL_CFG_CTRL, MSRT_BIT0, !b_reset ? MSRT_BIT0 : 0x00);
}

//0:PLL; 1:DLL+PLL; 2:DLL
#define ms7200drv_hdmi_audio_clk_sel(sd, u8_val) \
	ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BITS3_2, u8_val << 2)
#define ms7200drv_hdmi_audio_clk_enable(sd, b_enable) \
	ms_write8_mask(sd, REG_MISC_CLK_CTRL1, MSRT_BIT0, b_enable ? MSRT_BIT0 : 0x00)
#define ms7200drv_hdmi_audio_dll_clk_reset_release(sd, b_release) \
	ms_write8_mask(sd, REG_MISC_RST_CTRL0, MSRT_BIT7, b_release ? MSRT_BIT7 : 0x00)
#define ms7200drv_hdmi_audio_pll_clk_reset_release(sd, b_release) \
	ms_write8_mask(sd, REG_MISC_RST_CTRL0, MSRT_BIT6, b_release ? MSRT_BIT6 : 0x00)

static VOID _drv_hdmi_rx_audio_clk_sel(struct v4l2_subdev *sd, UINT8 u8_audio_clk_mode)
{
	//MS7200_LOG1("u8_audio_clk_mode = ", u8_audio_clk_mode);

	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		ms7200drv_hdmi_audio_clk_sel(sd, 0);
		//aupll_ref_ck_sel = 0
		//HAL_ClrBits(REG_AUPLL_CTRL2, MSRT_BIT4);
		ms_write8_mask(sd, REG_AUPLL_CTRL2, MSRT_BIT4, 0x00);
		ms7200drv_hdmi_audio_pll_clk_reset_release(sd, TRUE);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_2XPLL_MODE) {
		ms7200drv_hdmi_audio_clk_sel(sd, 1);
		//aupll_ref_ck_sel = 1
		//HAL_SetBits(REG_AUPLL_CTRL2, MSRT_BIT4);
		ms_write8_mask(sd, REG_AUPLL_CTRL2, MSRT_BIT4, MSRT_BIT4);
		ms7200drv_hdmi_audio_pll_clk_reset_release(sd, FALSE);

		//rx_audll_div2_en = 1
		//HAL_SetBits(REG_MISC_RX_PHY_SEL, MSRT_BIT5);
		ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BIT5, MSRT_BIT5);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_PLL_MODE) {
		ms7200drv_hdmi_audio_clk_sel(sd, 1);
		//aupll_ref_ck_sel = 1
		//HAL_SetBits(REG_AUPLL_CTRL2, MSRT_BIT4);
		ms_write8_mask(sd, REG_AUPLL_CTRL2, MSRT_BIT4, MSRT_BIT4);
		ms7200drv_hdmi_audio_pll_clk_reset_release(sd, FALSE);

		//rx_audll_div2_en = 0
		//HAL_ClrBits(REG_MISC_RX_PHY_SEL, MSRT_BIT5);
		ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BIT5, 0x00);
	} else {	// DLL
		ms7200drv_hdmi_audio_clk_sel(sd, 2);

		//rx_audll_div2_en = 0
		//HAL_ClrBits(REG_MISC_RX_PHY_SEL, MSRT_BIT5);
		ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BIT5, 0x00);
	}
}

/*
UINT8 float2char(float value, UINT8 *array)
{
	float DecimalPart;
	UINT32 IntegerPart;
	UINT8 i = 0, j = 0, temp;
	//separate integers and decimals
	if (value< 0) {
		value = -value ;
		array[i++] = '-';
	} else
		array[i++] = '+';
	IntegerPart = (UINT32)value;
	DecimalPart =  value - IntegerPart;
	// Processing integers
	if (0 == IntegerPart)//the integer part is 0
		array[i++] = '0';
	else {
		while (0 != IntegerPart) {
			array[i++] = IntegerPart%10 + '0';
			IntegerPart/=10;
		}
		i --;
		for (j = 1; j < (i+1)/2; j++) {
			temp = array[j];
			array[j] = array[i -j+1];
			array[i-j+1] = temp;
		}
		i++;
	}


	array[i++] = '.';
	// Do handle decimals
	array[i++] = (UINT32)(DecimalPart * 10)%10 + '0';
	array[i++] = (UINT32)(DecimalPart * 100)%10 + '0';
	array[i++] = (UINT32)(DecimalPart * 1000)%10 + '0';
	array[i++] = (UINT32)(DecimalPart * 10000)%10 + '0';
	array[i]   = '\0';

	return i;
}*/

//u16_fs, uint: 100Hz; u16_tmds_clk, uint:10000Hz
VOID ms7200drv_hdmi_audio_pll_config(struct v4l2_subdev *sd, UINT8 u8_audio_clk_mode,
										 UINT16 u16_tmds_clk, UINT32 N, UINT32 CTS)
{
	//float f_val;
	UINT8 f_val;
	UINT8 u8_pre_div;
	UINT8 u8_post_div;
	UINT16 u16_fs;
	bool b_hbr_input;
	//UINT8 float2char_array[42];
	//power down
	//HAL_ClrBits(REG_AUPLL_PWR, MSRT_BIT1);
	ms_write8_mask(sd, REG_AUPLL_PWR, MSRT_BIT1, 0x00);
	//HAL_SetBits(REG_AUPLL_PWR, MSRT_BIT0);
	ms_write8_mask(sd, REG_AUPLL_PWR, MSRT_BIT0, MSRT_BIT0);
	//HAL_SetBits(REG_AUPLL_FN_H, MSRT_BIT4);
	ms_write8_mask(sd, REG_AUPLL_FN_H, MSRT_BIT4, MSRT_BIT4);

	_drv_hdmi_rx_audio_clk_sel(sd, u8_audio_clk_mode);
	//PRE_DIV
	if (u16_tmds_clk <= 5000)
		u8_pre_div = 0x00;
	else if (u16_tmds_clk <= 10000)
		u8_pre_div = 0x01;
	else if (u16_tmds_clk <= 20000)
		u8_pre_div = 0x02;
	else
		u8_pre_div = 0x03;
	//MS7200_LOG1("u8_pre_div = ", u8_pre_div);
	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		//HAL_ModBits(REG_AUPLL_CTRL2, MSRT_BITS1_0, u8_pre_div);
		ms_write8_mask(sd, REG_AUPLL_CTRL2, MSRT_BITS1_0, u8_pre_div);
	}

	u16_fs = 100UL * u16_tmds_clk / 128 * N / CTS;

	if (u16_fs == 0)
		u16_fs = 480;
	//b_hbr_input = (HAL_ReadDWord_Ex(REG_PDEC_AUD_STS) & MSRT_BIT3) >> 3;
	b_hbr_input = (ms_read32(sd, REG_PDEC_AUD_STS) & MSRT_BIT3) >> 3;

	//post_div
	u8_post_div = (UINT8)(7500000UL / 2 / (128UL * u16_fs));
	if (b_hbr_input)
		u8_post_div /= 4;
	else
		u8_post_div /= 2;   //256fs output

	//MS7200_LOG1("u8_post_div = ", u8_post_div);
	//HAL_WriteByte(REG_AUPLL_POS_DVI, u8_post_div);
	ms_write8(sd, REG_AUPLL_POS_DVI, u8_post_div);
	//MS_INF("%s:%d before calculation  u8_post_div=%d, (UINT8)f_val=%d\n", __func__, __LINE__, u8_post_div, (UINT8)f_val);

	//fbdiv
	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		u8_pre_div = (1 << u8_pre_div);
		//f_val = (float)u8_post_div * 2 * u8_pre_div * N / CTS;
		f_val = u8_post_div * 2 * u8_pre_div * N / CTS;
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_PLL_MODE) {
		f_val = u8_post_div * 2 * 1;
	} else { //RX_AUDIO_DLL_TO_2XPLL_MODE
		f_val = u8_post_div * 2 * 2;
	}

	if (b_hbr_input)
		f_val *= 4;
	else
		f_val *= 2;
	//HAL_ToggleBits(REG_MISC_RX_PHY_SEL, MSRT_BIT1, !b_hbr_input);
	ms_write8_mask(sd, REG_MISC_RX_PHY_SEL, MSRT_BIT1, !b_hbr_input ? MSRT_BIT1 : 0x00);

	//MS7200_LOG1("f_val = ", (UINT8)f_val);
	//MS_INF("%s:%d   (float)f_val = %f\n", __func__, __LINE__, f_val);
	//MS_INF("%s:%d   (UINT8)f_val = %d\n", __func__, __LINE__, (UINT8)f_val);
	//float2char(f_val, float2char_array);
	//MS_INF("%s:%d   (float)f_val = %s\n", __func__, __LINE__, float2char_array);
	//HAL_WriteByte(REG_AUPLL_M, (UINT8)f_val);
	//MS_INF("%s:%d after calculation  u8_post_div=%d, (UINT8)f_val=%d\n", __func__, __LINE__, u8_post_div, (UINT8)f_val);
	ms_write8(sd, REG_AUPLL_M, (UINT8)f_val);

	f_val = f_val - (UINT8)f_val;
	//float2char(f_val, float2char_array);
	//MS_INF("%s:%d   (float)f_val = %s\n", __func__, __LINE__, float2char_array);
	f_val = f_val * (1UL << 20);
	//float2char(f_val, float2char_array);
	//MS_INF("%s:%d   (float)f_val = %s\n", __func__, __LINE__, float2char_array);
	//HAL_WriteByte(REG_AUPLL_FN_L, ((UINT32)f_val) >> 0);
	ms_write8(sd, REG_AUPLL_FN_L, ((UINT32)f_val) >> 0);
	//HAL_WriteByte(REG_AUPLL_FN_M, ((UINT32)f_val) >> 8);
	ms_write8(sd, REG_AUPLL_FN_M, ((UINT32)f_val) >> 8);
	//HAL_ModBits(REG_AUPLL_FN_H, MSRT_BITS3_0, ((UINT32)f_val) >> 16);
	ms_write8_mask(sd, REG_AUPLL_FN_H, MSRT_BITS3_0, ((UINT32)f_val) >> 16);

	//aupll_div2_sel = 1
	//HAL_SetBits(REG_AUPLL_CTRL1, MSRT_BIT7);
	ms_write8_mask(sd, REG_AUPLL_CTRL1, MSRT_BIT7, MSRT_BIT7);

	//power on
	//HAL_ClrBits(REG_AUPLL_PWR, MSRT_BIT0);
	ms_write8_mask(sd, REG_AUPLL_PWR, MSRT_BIT0, 0x00);
	//Delay_us(100);
	udelay(100);
	//HAL_SetBits(REG_AUPLL_PWR, MSRT_BIT1);
	ms_write8_mask(sd, REG_AUPLL_PWR, MSRT_BIT1, MSRT_BIT1);
}

//input:  u16_tmds_clk, uint:10000Hz;
//return: u16_fs, uint: 100Hz;
UINT16 ms7200drv_hdmi_rx_audio_config(struct v4l2_subdev *sd, UINT8 u8_audio_clk_mode, UINT16 u16_tmds_clk)
{
	UINT32 N;
	UINT32 CTS;
	UINT16 u16_fs = 0;
	bool b_dll_128fs = FALSE;

	//CTS = HAL_ReadDWord_Ex(REG_PDEC_ACR_CTS) & 0xfffffUL;
	CTS = ms_read32(sd, REG_PDEC_ACR_CTS) & 0xfffffUL;
	//N = HAL_ReadDWord_Ex(REG_PDEC_ACR_N) & 0xfffffUL;
	N = ms_read32(sd, REG_PDEC_ACR_N) & 0xfffffUL;
	//MS_INF("CTS_H = 0x%x\n", CTS >> 16);
	//MS_INF("CTS_L = 0x%x\n", CTS);
	//MS_INF("N_H = 0x%x\n", N >> 16);
	//MS_INF("N_L = 0x%x\n", N);
	if ((CTS == 0) || (N  == 0) || (u8_audio_clk_mode == RX_AUDIO_PLL_FREERUN_MODE)) {
		//MS7200_LOG("Auido PLL.");
		N = 0x1800UL;
		//fixed to 96KHz
		CTS = 100UL * u16_tmds_clk / 128 * N / 960;
		if (CTS == 0)
			CTS = 0x1220AUL;
		ms7200drv_hdmi_audio_pll_config(sd, RX_AUDIO_PLL_MODE, u16_tmds_clk, N, CTS);
		return u16_fs;
	}

	u16_fs = 100UL * u16_tmds_clk / 128 * N / CTS;
	//MS7200_LOG2("u16_audio_fs = ", u16_fs);

	//if tmds_clk > 256fs
	if (CTS > (2 * (N + N / 100))) {
		b_dll_128fs = TRUE;
	}

	if (u8_audio_clk_mode == RX_AUDIO_PLL_MODE) {
		//MS7200_LOG("Auido PLL.");
		ms7200drv_hdmi_audio_pll_config(sd, RX_AUDIO_PLL_MODE, u16_tmds_clk, N, CTS);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_2XPLL_MODE) {
		//MS7200_LOG("Auido DLL+2xPLL.");
		ms7200drv_hdmi_audio_pll_config(sd, RX_AUDIO_DLL_TO_2XPLL_MODE, u16_tmds_clk, N, CTS);
	} else if (u8_audio_clk_mode == RX_AUDIO_DLL_TO_PLL_MODE) {
		if (b_dll_128fs) {
			//MS7200_LOG("Auido DLL+PLL.");
			ms7200drv_hdmi_audio_pll_config(sd, RX_AUDIO_DLL_TO_PLL_MODE, u16_tmds_clk, N, CTS);
		} else { //fixed couvert
			//MS7200_LOG("Auido DLL+2xPLL.");
			ms7200drv_hdmi_audio_pll_config(sd, RX_AUDIO_DLL_TO_2XPLL_MODE, u16_tmds_clk, N, CTS);
		}
	} else { // DLL
		if (b_dll_128fs) {
			//MS7200_LOG("Auido DLL.");
			_drv_hdmi_rx_audio_clk_sel(sd, RX_AUDIO_DLL_MODE);
		} else { //fixed couvert
			//MS7200_LOG("Auido DLL+2xPLL.");
			ms7200drv_hdmi_audio_pll_config(sd, RX_AUDIO_DLL_TO_2XPLL_MODE, u16_tmds_clk, N, CTS);
		}
	}

	return u16_fs;
}

UINT16 ms7200_hdmirx_audio_config(struct v4l2_subdev *sd, bool b_clk_change, UINT16 u16_tmds_clk)
{
	UINT16 u16_fs = 0;

	ms7200drv_hdmi_rx_audio_fifo_reset(sd, TRUE);

	if (b_clk_change) {
		u16_fs = ms7200drv_hdmi_rx_audio_config(sd, RX_AUDIO_DLL_TO_PLL_MODE, u16_tmds_clk);
#if VIN_FALSE
		g_st_hdmi_timing.u8_audio_rate = _hdmi_rx_audio_fs_type_get(u16_fs);
#endif
		//LOG2("u16_audio_fs = ", u16_fs);
	}

	//Delay_ms(1);
	mdelay(1);
	ms7200drv_hdmi_rx_audio_fifo_reset(sd, FALSE);

	return u16_fs;
}

VOID ms7200_hdmirx_core_reset(struct v4l2_subdev *sd, UINT32 u32_reset_ctrl)
{
	ms7200drv_hdmi_rx_controller_reset(sd, u32_reset_ctrl);
}

bool ms7200drv_hdmi_rx_controller_mdt_syncvalid(struct v4l2_subdev *sd)
{
	//return (HAL_ReadDWord_Ex(REG_MD_STS) & 0x1107) == 0x1107;
	return (ms_read32(sd, REG_MD_STS) & 0x1107) == 0x1107;
}

static e_RxMdtPol _drv_mdt_get_sync_polarity(struct v4l2_subdev *sd)
{
	UINT8 Polarity;
	//UINT8 temp = (HAL_ReadDWord_Ex(REG_HDMI_STS) >> 7) & MSRT_BITS2_1;
	UINT8 temp = (ms_read32(sd, REG_HDMI_STS) >> 7) & MSRT_BITS2_1;
	//Polarity = temp | (( (~ HAL_ReadDWord_Ex(REG_MD_STS)) >> 3) & MSRT_BIT0);
	Polarity = temp | (((~ms_read32(sd, REG_MD_STS)) >> 3) & MSRT_BIT0);
	return (e_RxMdtPol)Polarity;
}

static UINT16 _drv_mdt_get_htotal(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_MD_HT1) >> 16;
	return ms_read32(sd, REG_MD_HT1) >> 16;
}

static UINT16 _drv_mdt_get_vtotal(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_MD_VTL) & 0xffff;
	return ms_read32(sd, REG_MD_VTL) & 0xffff;
}

static UINT16 _drv_mdt_get_hactive(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_MD_HACT_PX) & 0xffff;
	return ms_read32(sd, REG_MD_HACT_PX) & 0xffff;
}

static UINT16 _drv_mdt_get_vactive(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_MD_VAL) & 0xffff;
	return ms_read32(sd, REG_MD_VAL) & 0xffff;
}

static UINT32 _drv_mdt_get_hfreq(struct v4l2_subdev *sd)
{
	//UINT16 u16_ht = HAL_ReadDWord_Ex(REG_MD_HT0) >> 16;
	UINT16 u16_ht = ms_read32(sd, REG_MD_HT0) >> 16;

	return u16_ht ? (16 * MS7200_EXT_XTAL / u16_ht) : 0;
}

static UINT16 _drv_mdt_get_hoffset(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_MD_HT1) & 0xffff;
	return ms_read32(sd, REG_MD_HT1) & 0xffff;
}

static UINT16 _drv_mdt_get_vback(struct v4l2_subdev *sd)
{
	//use top misc mdt
	//return HAL_ReadWord(REG_MISC_MD_VBACK_L);
	return ms_read16(sd, REG_MISC_MD_VBACK_L);
}

static UINT16 _drv_mdt_get_hsw(struct v4l2_subdev *sd)
{
	//use top misc mdt
	//return HAL_ReadWord(REG_MISC_MD_HSW_L);
	return ms_read16(sd, REG_MISC_MD_HSW_L);
}

static UINT16 _drv_mdt_get_vsw(struct v4l2_subdev *sd)
{
	//use top misc mdt
	//return HAL_ReadWord(REG_MISC_MD_VSW_L);
	return ms_read16(sd, REG_MISC_MD_VSW_L);
}

bool ms7200drv_hdmi_rx_get_input_timing(struct v4l2_subdev *sd, VIDEOTIMING_T *ptTiming)
{
	bool bValid = FALSE;

	if (ms7200drv_hdmi_rx_controller_mdt_syncvalid(sd)) {
		ptTiming->u8_polarity = _drv_mdt_get_sync_polarity(sd);
		ptTiming->u16_htotal = _drv_mdt_get_htotal(sd);
		ptTiming->u16_vtotal = _drv_mdt_get_vtotal(sd);
		ptTiming->u16_hactive = _drv_mdt_get_hactive(sd);
		ptTiming->u16_vactive = _drv_mdt_get_vactive(sd);
		ptTiming->u16_pixclk = ms7200drv_hdmi_rx_get_tmds_clk(sd);
		// 20141029 Domain judge
		if (ptTiming->u16_htotal < 50 \
			|| ptTiming->u16_vtotal < 50 \
			|| ptTiming->u16_hactive < 50 \
			|| ptTiming->u16_vactive < 50 \
			|| ptTiming->u16_htotal < ptTiming->u16_hactive \
			|| ptTiming->u16_vtotal < ptTiming->u16_vactive) {
			MS_INF("mdt error.\n");
			return FALSE;
		}

		if (!(ptTiming->u8_polarity & MSRT_BIT0)) {
			if (ptTiming->u16_vtotal % 2) {
				if (ptTiming->u16_vtotal == 625) // 1920x1080i (1250 Total) @ 50 Hz (Format 39)
					ptTiming->u16_vtotal = 2 * ptTiming->u16_vtotal;
				else
					ptTiming->u16_vtotal = 2 * ptTiming->u16_vtotal - 1;
			} else
				ptTiming->u16_vtotal = 2 * ptTiming->u16_vtotal + 1;
		}

		ptTiming->u16_vfreq = 100 * _drv_mdt_get_hfreq(sd) / ptTiming->u16_vtotal;
		if (!(ptTiming->u8_polarity & MSRT_BIT0)) {
			ptTiming->u16_vfreq <<= 1;
			ptTiming->u16_vactive <<= 1;
		}

		if (ptTiming->u16_vfreq < 500) {
			MS_INF("mdt error1.\n");
			return FALSE;
		}

		ptTiming->u16_hoffset = _drv_mdt_get_hoffset(sd);
		ptTiming->u16_voffset = _drv_mdt_get_vback(sd);
		ptTiming->u16_hsyncwidth = _drv_mdt_get_hsw(sd);
		ptTiming->u16_vsyncwidth = _drv_mdt_get_vsw(sd);
		//
		ptTiming->u16_voffset += ptTiming->u16_vsyncwidth;

		bValid = TRUE;
	}

	return bValid;
}

bool ms7200_hdmirx_input_timing_get(struct v4l2_subdev *sd, VIDEOTIMING_T *ptTiming)
{
	return ms7200drv_hdmi_rx_get_input_timing(sd, ptTiming);
}

bool ms7200drv_hdmi_rx_controller_dvidet(struct v4l2_subdev *sd)
{
	//return (HAL_ReadDWord_Ex(REG_PDEC_STS) >> 28) & MSRT_BIT0;
	return (ms_read32(sd, REG_PDEC_STS) >> 28) & MSRT_BIT0;
}

static UINT8 _drv_pdec_get_vic(struct v4l2_subdev *sd)
{
	//return (HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 24) & MSRT_BITS6_0;
	return (ms_read32(sd, REG_PDEC_AVI_PB) >> 24) & MSRT_BITS6_0;
}

static HDMI_CLK_RPT_E _drv_pdec_get_clk_rpt(struct v4l2_subdev *sd)
{
	//return (HDMI_CLK_RPT_E)((HAL_ReadDWord_Ex(REG_PDEC_AVI_HB) >> 24) & MSRT_BITS3_0);
	return (HDMI_CLK_RPT_E)((ms_read32(sd, REG_PDEC_AVI_HB) >> 24) & MSRT_BITS3_0);
}

static HDMI_SCAN_INFO_E _drv_pdec_get_scan_info(struct v4l2_subdev *sd)
{
	//UINT8 u8ScanInfo = HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) & MSRT_BITS1_0;
	UINT8 u8ScanInfo = ms_read32(sd, REG_PDEC_AVI_PB) & MSRT_BITS1_0;
	return (HDMI_SCAN_INFO_E)(u8ScanInfo);
}

static HDMI_ASPECT_RATIO_E _drv_pdec_get_aspect_ratio(struct v4l2_subdev *sd)
{
	//UINT8 u8AspectRatio = (HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 12) & MSRT_BITS1_0;
	UINT8 u8AspectRatio = (ms_read32(sd, REG_PDEC_AVI_PB) >> 12) & MSRT_BITS1_0;
	return (HDMI_ASPECT_RATIO_E)(u8AspectRatio);
}

static HDMI_CS_E _drv_pdec_get_color_space(struct v4l2_subdev *sd)
{
	//return (HDMI_CS_E)((HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 5) & MSRT_BITS1_0);
	return (HDMI_CS_E)((ms_read32(sd, REG_PDEC_AVI_PB) >> 5) & MSRT_BITS1_0);
}

static UINT8 _drv_hdmi_status_get_color_depth(struct v4l2_subdev *sd)
{
	//[31:28]
	//UINT8 u8ColorDepth = HAL_ReadDWord_Ex(REG_HDMI_STS) >> 28;
	UINT8 u8ColorDepth = ms_read32(sd, REG_HDMI_STS) >> 28;
	u8ColorDepth -= (u8ColorDepth >= 4) ? 4 : u8ColorDepth;
	return u8ColorDepth;
}

static HDMI_COLORIMETRY_E _drv_pdec_get_colorimetry(struct v4l2_subdev *sd)
{
	//UINT8 u8Colorimetry = (HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 14) & MSRT_BITS1_0;
	UINT8 u8Colorimetry = (ms_read32(sd, REG_PDEC_AVI_PB) >> 14) & MSRT_BITS1_0;
	switch (u8Colorimetry) {
	case 1:
		u8Colorimetry = HDMI_COLORIMETRY_601;
		break;
	case 2:
		u8Colorimetry = HDMI_COLORIMETRY_709;
		break;
	case 3:
		//if (((HAL_ReadDWord_Ex(REG_PDEC_AVI_PB) >> 20) & MSRT_BITS2_0) == 0)
		if (((ms_read32(sd, REG_PDEC_AVI_PB) >> 20) & MSRT_BITS2_0) == 0)
			u8Colorimetry = HDMI_COLORIMETRY_XVYCC601;
		else
			u8Colorimetry = HDMI_COLORIMETRY_XVYCC709;
		break;
	}
	return (HDMI_COLORIMETRY_E)u8Colorimetry;
}

static HDMI_VIDEO_FORMAT_E _drv_pdec_get_video_format(struct v4l2_subdev *sd)
{
	//UINT8 u8VideoFormat = (HAL_ReadDWord_Ex(REG_PDEC_VSI_PAYLOAD0) >> 5) & MSRT_BITS2_0;
	UINT8 u8VideoFormat = (ms_read32(sd, REG_PDEC_VSI_PAYLOAD0) >> 5) & MSRT_BITS2_0;
	switch (u8VideoFormat) {
	case 1:
		u8VideoFormat = HDMI_4Kx2K_FORMAT;
		break;
	case 2:
		u8VideoFormat = HDMI_3D_FORMAT;
		break;
	default:
		u8VideoFormat = HDMI_NO_ADD_FORMAT;
		break;
	}
	return (HDMI_VIDEO_FORMAT_E)u8VideoFormat;
}

static HDMI_4Kx2K_VIC_E _drv_pdec_get_4Kx2K_vic(struct v4l2_subdev *sd)
{
	//UINT8 u84Kx2KVic = (HAL_ReadDWord_Ex(REG_PDEC_VSI_PAYLOAD0) >> 8) & MSRT_BITS7_0;
	UINT8 u84Kx2KVic = (ms_read32(sd, REG_PDEC_VSI_PAYLOAD0) >> 8) & MSRT_BITS7_0;
	switch (u84Kx2KVic) {
	case 1:
		u84Kx2KVic = HDMI_4Kx2K_30HZ;
		break;
	case 2:
		u84Kx2KVic = HDMI_4Kx2K_25HZ;
		break;
	case 3:
		u84Kx2KVic = HDMI_4Kx2K_24HZ;
		break;
	case 4:
		u84Kx2KVic = HDMI_4Kx2K_24HZ_SMPTE;
		break;
	default:
		u84Kx2KVic = HDMI_4Kx2K_30HZ;
		break;
	}
	return (HDMI_4Kx2K_VIC_E)u84Kx2KVic;
}

static HDMI_3D_STRUCTURE_E _drv_pdec_get_3D_structure(struct v4l2_subdev *sd)
{
	//UINT8 u83DStructure = (HAL_ReadDWord_Ex(REG_PDEC_VSI_PAYLOAD0) >> 20) & MSRT_BITS3_0;
	UINT8 u83DStructure = (ms_read32(sd, REG_PDEC_VSI_PAYLOAD0) >> 20) & MSRT_BITS3_0;
	switch (u83DStructure) {
	case 0:
		u83DStructure = HDMI_FRAME_PACKING;
		break;
	case 1:
		u83DStructure = HDMI_FIELD_ALTERNATIVE;
		break;
	case 2:
		u83DStructure = HDMI_LINE_ALTERNATIVE;
		break;
	case 3:
		u83DStructure = HDMI_SIDE_BY_SIDE_FULL;
		break;
	case 4:
		u83DStructure = L_DEPTH;
		break;
	case 5:
		u83DStructure = L_DEPTH_GRAPHICS;
		break;
	case 8:
		u83DStructure = SIDE_BY_SIDE_HALF;
		break;
	default:
		u83DStructure = HDMI_FRAME_PACKING;
		break;
	}
	return (HDMI_3D_STRUCTURE_E)u83DStructure;
}

//refer to IEC-60958-3-Digital-Audio
static VOID _drv_pdec_get_audio_sample_packet_infomation(struct v4l2_subdev *sd, AUDIO_SAMPLE_PACKET_T *pt_packet)
{
	UINT8 u8_byte;

#if VIN_FALSE
	u8_byte = HAL_ReadByte(REG_MISC_C_BYTE0) & MSRT_BIT1;
	pt_packet->u8_audio_mode = u8_byte ? HDMI_AUD_MODE_HBR : HDMI_AUD_MODE_AUDIO_SAMPLE;
#endif

	//u8_byte = HAL_ReadByte(REG_MISC_C_BYTE3);
	u8_byte = ms_read8(sd, REG_MISC_C_BYTE3);
	pt_packet->u8_audio_rate = u8_byte & 0xf;

	//u8_byte = HAL_ReadByte(REG_MISC_C_BYTE4);
	u8_byte = ms_read8(sd, REG_MISC_C_BYTE4);
	u8_byte = u8_byte & 0xf;
	if (u8_byte == 0x03 || u8_byte == 0x0A)
		pt_packet->u8_audio_bits = HDMI_AUD_LENGTH_20BITS;
	else if (u8_byte == 0x0B)
		pt_packet->u8_audio_bits = HDMI_AUD_LENGTH_24BITS;
	else
		pt_packet->u8_audio_bits = HDMI_AUD_LENGTH_16BITS;

	//u8_byte = HAL_ReadByte(REG_MISC_C_BYTE2);
	u8_byte = ms_read8(sd, REG_MISC_C_BYTE2);
	pt_packet->u8_audio_channels = u8_byte & 0xf;
}

static HDMI_AUDIO_RATE_E _drv_pdec_get_audio_rate(struct v4l2_subdev *sd)
{
	//UINT8 u8AudioRate = (HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) >> 10) & MSRT_BITS2_0;
	UINT8 u8AudioRate = (ms_read32(sd, REG_PDEC_AIF_PB0) >> 10) & MSRT_BITS2_0;
	switch (u8AudioRate) {
	case 1:
		u8AudioRate = HDMI_AUD_RATE_32K;
		break;
	case 2:
		u8AudioRate = HDMI_AUD_RATE_44K1;
		break;
	case 3:
		u8AudioRate = HDMI_AUD_RATE_48K;
		break;
	case 4:
		u8AudioRate = HDMI_AUD_RATE_88K2;
		break;
	case 5:
		u8AudioRate = HDMI_AUD_RATE_96K;
		break;
	case 6:
		u8AudioRate = HDMI_AUD_RATE_176K4;
		break;
	case 7:
		u8AudioRate = HDMI_AUD_RATE_192K;
		break;
	default:
		u8AudioRate = HDMI_AUD_RATE_48K;
		break;
	}
	return (HDMI_AUDIO_RATE_E)u8AudioRate;
}

static HDMI_AUDIO_LENGTH_E _drv_pdec_get_audio_bits(struct v4l2_subdev *sd)
{
	//UINT8 u8AudioBits = (HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) >> 8) & MSRT_BITS1_0;
	UINT8 u8AudioBits = (ms_read32(sd, REG_PDEC_AIF_PB0) >> 8) & MSRT_BITS1_0;
	switch (u8AudioBits) {
	case 1:
		u8AudioBits = HDMI_AUD_LENGTH_16BITS;
		break;
	case 2:
		u8AudioBits = HDMI_AUD_LENGTH_20BITS;
		break;
	case 3:
		u8AudioBits = HDMI_AUD_LENGTH_24BITS;
		break;
	default:
		u8AudioBits = HDMI_AUD_LENGTH_16BITS;
		break;
	}
	return (HDMI_AUDIO_LENGTH_E)u8AudioBits;
}

static HDMI_AUDIO_CHN_E _drv_pdec_get_audio_channels(struct v4l2_subdev *sd)
{
	//return (HDMI_AUDIO_CHN_E)(HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) & MSRT_BITS2_0);
	return (HDMI_AUDIO_CHN_E)(ms_read32(sd, REG_PDEC_AIF_PB0) & MSRT_BITS2_0);
}

static UINT8 _drv_pdec_get_audio_speaker_locations(struct v4l2_subdev *sd)
{
	//return (UINT8)(HAL_ReadDWord_Ex(REG_PDEC_AIF_PB0) >> 24);
	return (UINT8)(ms_read32(sd, REG_PDEC_AIF_PB0) >> 24);
}

VOID ms7200drv_hdmi_rx_controller_get_input_config(struct v4l2_subdev *sd, HDMI_CONFIG_T *pt_hdmi_rx)
{
	AUDIO_SAMPLE_PACKET_T t_audio_sample_packet;

	pt_hdmi_rx->u8_hdmi_flag = !ms7200drv_hdmi_rx_controller_dvidet(sd);
	pt_hdmi_rx->u8_vic = _drv_pdec_get_vic(sd);
	pt_hdmi_rx->u16_video_clk = ms7200drv_hdmi_rx_get_tmds_clk(sd);
	pt_hdmi_rx->u8_clk_rpt = _drv_pdec_get_clk_rpt(sd);
	pt_hdmi_rx->u8_scan_info = _drv_pdec_get_scan_info(sd);
	pt_hdmi_rx->u8_aspect_ratio = _drv_pdec_get_aspect_ratio(sd);
	pt_hdmi_rx->u8_color_space = _drv_pdec_get_color_space(sd);
#if VIN_FALSE
	//20180911, buf because, no packet input, the register keep pre stauts
	pt_hdmi_rx->u8_color_depth = _drv_pdec_get_color_depth();
#else
	pt_hdmi_rx->u8_color_depth = _drv_hdmi_status_get_color_depth(sd);
#endif
	pt_hdmi_rx->u8_colorimetry = _drv_pdec_get_colorimetry(sd);

	pt_hdmi_rx->u8_video_format = _drv_pdec_get_video_format(sd);
	pt_hdmi_rx->u8_4Kx2K_vic = _drv_pdec_get_4Kx2K_vic(sd);
	pt_hdmi_rx->u8_3D_structure = _drv_pdec_get_3D_structure(sd);

	_drv_pdec_get_audio_sample_packet_infomation(sd, &t_audio_sample_packet);
#if VIN_FALSE
	pt_hdmi_rx->u8_audio_mode = ms7200drv_pdec_get_audio_mode();
#else //20191231, fixed to audio sample mode. bacause others case never be tested.
	pt_hdmi_rx->u8_audio_mode = 0;
#endif
	pt_hdmi_rx->u8_audio_rate = _drv_pdec_get_audio_rate(sd);
	pt_hdmi_rx->u8_audio_bits = _drv_pdec_get_audio_bits(sd);
	pt_hdmi_rx->u8_audio_channels = _drv_pdec_get_audio_channels(sd);

	//refer to header
#if VIN_FALSE
	//20181023, xiaomi bofangbo detect error.
	//if (pt_hdmi_rx->u8_audio_mode == 0) pt_hdmi_rx->u8_audio_mode = t_audio_sample_packet.u8_audio_mode;
#endif
	if (pt_hdmi_rx->u8_audio_rate == 0)
		pt_hdmi_rx->u8_audio_rate = t_audio_sample_packet.u8_audio_rate;
	if (pt_hdmi_rx->u8_audio_bits == 0)
		pt_hdmi_rx->u8_audio_bits = t_audio_sample_packet.u8_audio_bits;
	if (pt_hdmi_rx->u8_audio_channels == 0)
		pt_hdmi_rx->u8_audio_channels = t_audio_sample_packet.u8_audio_channels;

	pt_hdmi_rx->u8_audio_speaker_locations = _drv_pdec_get_audio_speaker_locations(sd);
}

VOID ms7200_hdmirx_input_infoframe_get(struct v4l2_subdev *sd, HDMI_CONFIG_T *pt_hdmi_rx)
{
	ms7200drv_hdmi_rx_controller_get_input_config(sd, pt_hdmi_rx);
}

void sys_default_hdmi_video_config(void)
{
	//g_t_hdmi_infoframe.u8_hdmi_flag = TRUE;
	g_t_hdmirx_infoframe.u8_vic = 0;
	//g_t_hdmi_infoframe.u16_video_clk = 7425;
	g_t_hdmirx_infoframe.u8_clk_rpt = HDMI_X1CLK;
	g_t_hdmirx_infoframe.u8_scan_info = HDMI_OVERSCAN;
	g_t_hdmirx_infoframe.u8_aspect_ratio = HDMI_16X9;
	g_t_hdmirx_infoframe.u8_color_space = HDMI_RGB;
	g_t_hdmirx_infoframe.u8_color_depth = HDMI_COLOR_DEPTH_8BIT;
	g_t_hdmirx_infoframe.u8_colorimetry = HDMI_COLORIMETRY_709;
}

void sys_default_hdmi_vendor_specific_config(void)
{
	g_t_hdmirx_infoframe.u8_video_format = HDMI_NO_ADD_FORMAT;
	g_t_hdmirx_infoframe.u8_4Kx2K_vic = HDMI_4Kx2K_30HZ;
	g_t_hdmirx_infoframe.u8_3D_structure = HDMI_FRAME_PACKING;
}

VOID ms7200drv_hdmi_rx_controller_pixel_clk_config(struct v4l2_subdev *sd, UINT8 u8ColorDepth, UINT8 u8ClkRepeat)
{
	//rx pll deep color clk enable
	//HAL_ToggleBits(REG_PI_RX_CTRL0, MSRT_BIT4, u8ColorDepth > 0);
	ms_write8_mask(sd, REG_PI_RX_CTRL0, MSRT_BIT4, u8ColorDepth > 0 ? MSRT_BIT4 : 0x00);
	//HAL_ModBits(REG_MISC_CLK_CTRL0, MSRT_BITS5_4, u8ColorDepth << 4);
	ms_write8_mask(sd, REG_MISC_CLK_CTRL0, MSRT_BITS5_4, u8ColorDepth << 4);

	//
	//HAL_ModBits(REG_MISC_AV_CTRL2, MSRT_BITS3_0, u8ClkRepeat);
}

VOID ms7200drv_hdmi_rx_controller_set_avmute_black_color(struct v4l2_subdev *sd, UINT8 u8_cs)
{
	UINT16 ub = 0;
	UINT16 yg = 0;
	UINT16 vr = 0;

	if (u8_cs == HDMI_YCBCR422) {
		ub = 0;
		yg = 0;
		vr = 0x8000;
	} else if (u8_cs == HDMI_YCBCR444) { //yuv444
		ub = 0x8000;
		yg = 0;
		vr = 0x8000;
	} else if (u8_cs == HDMI_YUV420) { //yuv420
		ub = 0x8000;
		yg = 0;
		vr = 0;
	}

	//HAL_WriteDWord_Ex(REG_HDMI_VM_CFG_CH_0_1, ((UINT32)yg << 16) | ub);
	ms_write32(sd, REG_HDMI_VM_CFG_CH_0_1, ((UINT32)yg << 16) | ub);
	//HAL_ModBits_Ex(REG_HDMI_VM_CFG_CH_2, 0xffff, vr);
	ms_write32_mask(sd, REG_HDMI_VM_CFG_CH_2, 0xffff, vr);
}

VOID ms7200drv_hdmi_rx_video_fifo_reset(struct v4l2_subdev *sd, bool b_reset)
{
	//HAL_ToggleBits_Ex(MS7200_HDMI_RX_CEA_VIDEO_REG, 0x80000000UL, b_reset);
	ms_write32_mask(sd, MS7200_HDMI_RX_CEA_VIDEO_REG, 0x80000000UL, b_reset ? 0x80000000UL : 0x00);
}

VOID ms7200drv_csc_config_input(struct v4l2_subdev *sd, HDMI_CS_E e_cs)
{
	UINT8 u8_cs = 3;   //0:RGB; 1:YUV444; 2:YUV422; 3:bypass

	switch (e_cs) {
	case HDMI_RGB:
		u8_cs = 4;
		break;
	case HDMI_YCBCR444:
		u8_cs = 0;
		break;
	case HDMI_YCBCR422:
		u8_cs = 6;
		break;
	default:
		break;
	}
	//HAL_ModBits(REG_CSC_CTRL1, MSRT_BITS1_0, u8_cs);
	//ms_write8_mask(sd, REG_CSC_CTRL1, MSRT_BITS1_0, u8_cs);
	MS_INF("%s(line=%d) csc reg addr:0xf0 set value=%d\n", __func__, __LINE__, u8_cs);
	ms_write8(sd, REG_CSC_CTRL1, u8_cs);
}

VOID ms7200drv_dvout_clk_sel(struct v4l2_subdev *sd, UINT8 u8_clk_rpt, UINT16 u16_video_clk)
{
	//HAL_ModBits(REG_DIG_CLK_SEL0, MSRT_BITS7_4, u8_clk_rpt << 4);
	ms_write8_mask(sd, REG_DIG_CLK_SEL0, MSRT_BITS7_4, u8_clk_rpt << 4);
	//HAL_ModBits(REG_DIG_CLK_SEL1, MSRT_BITS3_0, ((u16_video_clk <= 5000) ? 2 : 1) * (u8_clk_rpt + 1) - 1);
	ms_write8_mask(sd, REG_DIG_CLK_SEL1, MSRT_BITS3_0, ((u16_video_clk <= 5000) ? 2 : 1) * (u8_clk_rpt + 1) - 1);
}

VOID ms7200_hdmirx_video_config(struct v4l2_subdev *sd, HDMI_CONFIG_T *pt_hdmi_rx)
{
	ms7200drv_hdmi_rx_controller_pixel_clk_config(sd, pt_hdmi_rx->u8_color_depth, pt_hdmi_rx->u8_clk_rpt);
	//20181221, config rx auto avmute color.
	ms7200drv_hdmi_rx_controller_set_avmute_black_color(sd, pt_hdmi_rx->u8_color_space);

	//20190515, reset video fifo after pixel config done.
	ms7200drv_hdmi_rx_video_fifo_reset(sd, TRUE);
	//Delay_us(100);
	udelay(100);
	ms7200drv_hdmi_rx_video_fifo_reset(sd, FALSE);

	ms7200drv_csc_config_input(sd, (HDMI_CS_E)pt_hdmi_rx->u8_color_space);
	ms7200drv_dvout_clk_sel(sd, pt_hdmi_rx->u8_clk_rpt, pt_hdmi_rx->u16_video_clk);
}

UINT8 ms7200drv_hdmi_rx_get_audio_fifo_status(struct v4l2_subdev *sd)
{
	//return HAL_ReadDWord_Ex(REG_AUD_FIFO_STS) & 0x1F;
	return ms_read32(sd, REG_AUD_FIFO_STS) & 0x1F;
}

UINT8 ms7200_hdmirx_audio_fifo_status_get(struct v4l2_subdev *sd)
{
	return ms7200drv_hdmi_rx_get_audio_fifo_status(sd);
}

UINT8 ms7200drv_hdmi_rx_5v_det(struct v4l2_subdev *sd)
{
	//return (HAL_ReadByte(REG_MISC_5V_DET) & MSRT_BIT0);
	return (ms_read8(sd, REG_MISC_5V_DET) & MSRT_BIT0);
}

bool ms7200_hdmirx_source_connect_detect(struct v4l2_subdev *sd)
{
	return ms7200drv_hdmi_rx_5v_det(sd);
}

void ms7200_hdmirx_hot_plug_status_set(struct v4l2_subdev *sd)
{
	//struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	if (ms7200drv_hdmi_rx_controller_hpd_get(sd)) {
		ms7200_hdmi_hot_plug_status = 1;
	} else {
		ms7200_hdmi_hot_plug_status = 0;
	}
}

void ms7200_hpd_event_subscribe(struct v4l2_subdev *sd)
{
	struct v4l2_event ms_ev_fmt;
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	if (v4l2_ctrl_s_ctrl(ms->tmds_signal_ctrl,
		ms->tmds_signal) < 0) {
		MS_ERR("v4l2_ctrl_s_ctrl for tmds signal failed!\n");
	}

	memset(&ms_ev_fmt, 0, sizeof(ms_ev_fmt));
	ms_ev_fmt.type = V4L2_EVENT_SOURCE_CHANGE;
	ms_ev_fmt.u.src_change.changes = V4L2_EVENT_SRC_CH_HPD_OUT;
	if (ms->signal_event_subscribe)
		v4l2_subdev_notify_event(sd, &ms_ev_fmt);

	memset(&ms->timing, 0, sizeof(struct v4l2_dv_timings));
}

void ms7200_detected_resolution_change(struct v4l2_subdev *sd)
{
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	if (!ms7200_hdmirx_input_clk_get(sd)) {
		if (ms->wait4timing_stable == 1) {
			MS_INF("++++++tmds signal 0\n");
			ms7200_dvout_video_config(sd, FALSE);
			ms7200_hpd_event_subscribe(sd);
			ms->wait4timing_stable = 0;
		}
	} else {
		//MS_INF("++++++tmds signal 1\n");
		ms->wait4timing_stable = 1;
	}

}

#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_48K			(1 << 3)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_44K1		(1 << 4)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_32K			(1 << 5)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_88K2		(1 << 6)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_96K			(1 << 7)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_176K4		(1 << 8)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_192K		(1 << 9)
//add from tc358743
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_22K			(1 << 10)
#define V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_24K			(1 << 11)
//add from tc358743

void ms7200_audio_event_subscribe(struct v4l2_subdev *sd, UINT16 u16_fs)
{
	struct v4l2_event ms_ev_fmt;
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	unsigned int change = 0;

	if (u16_fs > 470 && u16_fs < 490) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_48K;
	} else if (u16_fs > 431 && u16_fs < 451) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_44K1;
	} else if (u16_fs > 310 && u16_fs < 330) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_32K;
	} else if (u16_fs > 872 && u16_fs < 892) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_88K2;
	} else if (u16_fs > 950 && u16_fs < 970) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_96K;
	} else if (u16_fs > 1754 && u16_fs < 1774) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_176K4;
	} else if (u16_fs > 1910 && u16_fs < 1930) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_192K;
	} else if (u16_fs > 219 && u16_fs < 222) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_22K;
	} else if (u16_fs > 230 && u16_fs < 250) {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_24K;
	} else {
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_48K;
	}
	MS_INF("audio sampling rate: %d, change=0x%x\n", u16_fs, change);

	if (v4l2_ctrl_s_ctrl(ms->tmds_signal_ctrl,
		ms->tmds_signal) < 0) {
		MS_ERR("v4l2_ctrl_s_ctrl for tmds signal failed!\n");
	}

	memset(&ms_ev_fmt, 0, sizeof(ms_ev_fmt));
	ms_ev_fmt.type = V4L2_EVENT_SOURCE_CHANGE;
	ms_ev_fmt.u.src_change.changes = change;
	if (ms->signal_event_subscribe)
		v4l2_subdev_notify_event(sd, &ms_ev_fmt);
}

VOID ms7200_media_service(struct v4l2_subdev *sd)
{
	UINT8 u8_rxphy_status = 0;
	UINT32 u32_int_status = 0;
	HDMI_CONFIG_T t_hdmi_infoframe;
	bool signal = 0;
	struct v4l2_event ms_ev_fmt;
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;
	struct v4l2_dv_timings timing;
	unsigned int change = 0;
	//int i = 0;
	//UINT8 data[4];
	static UINT16 u16_fs;

	//detect 5v to pull up/down hpd
	ms7200_hdmirx_hpd_config(sd, ms7200_hdmirx_source_connect_detect(sd), NULL);

	//reconfig rxphy when input clk change
	u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(sd, RX_INT_INDEX_HDMI,
								 FREQ_LOCK_ISTS | FREQ_UNLOCK_ISTS | CLK_CHANGE_ISTS, TRUE);

	if (u32_int_status) {
		if (u32_int_status & CLK_CHANGE_ISTS) {
			if (_abs(ms7200_hdmirx_input_clk_get(sd) - g_u16_tmds_clk) > 1000)
				g_b_rxphy_status = FALSE;
		} else if ((u32_int_status & (FREQ_LOCK_ISTS | FREQ_UNLOCK_ISTS)) == FREQ_LOCK_ISTS)
			g_b_rxphy_status = TRUE;

		if (!g_b_rxphy_status) {
			ms7200_dvout_audio_config(sd, FALSE);
			ms7200_dvout_video_config(sd, FALSE);
			u8_rxphy_status = ms7200_hdmirx_rxphy_config(sd, &g_u16_tmds_clk);
			g_b_rxphy_status = u8_rxphy_status ? (u8_rxphy_status & 0x01) : TRUE;
			g_b_input_valid = FALSE;
			g_u8_rx_stable_timer_count = 0;

			if (g_u16_tmds_clk > 14800 && g_u16_tmds_clk < 14900) {
				g_u16_tmds_clk = 14850;
			} else if (g_u16_tmds_clk > 7375 && g_u16_tmds_clk < 7475) {
				g_u16_tmds_clk = 7425;
			} else if (g_u16_tmds_clk > 29600 && g_u16_tmds_clk < 29800) {
				g_u16_tmds_clk = 29700;
			} else if (g_u16_tmds_clk > 2600 && g_u16_tmds_clk < 2800) {
				g_u16_tmds_clk = 2700;
			} else if (g_u16_tmds_clk > 1250 && g_u16_tmds_clk < 1450) {
				g_u16_tmds_clk = 1350;
			} else if (g_u16_tmds_clk > 37025 && g_u16_tmds_clk < 37225) {
				g_u16_tmds_clk = 37125;
			}

			MS_INF("g_u16_tmds_clk = %d\n", g_u16_tmds_clk);
			MS_INF("g_b_rxphy_status = %d\n", g_b_rxphy_status);

			//=====================
			if (g_u16_tmds_clk == 0) {
				ms7200_hpd_event_subscribe(sd);
			}
			//=====================
		}
	}
	if (g_u16_tmds_clk < 500) {
		u16_fs = 0;
		ms->audio.audio_fs = 0;
		ms->audio.present = 0;
		return;
	}
	//reset pdec when mdt change
	u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(sd, RX_INT_INDEX_MDT, MDT_STB_ISTS | MDT_USTB_ISTS, TRUE);
	if (u32_int_status) {
		if (!(g_b_input_valid && (u32_int_status == MDT_STB_ISTS))) {
			ms7200_dvout_audio_config(sd, FALSE);
			ms7200_dvout_video_config(sd, FALSE);
			ms7200_hdmirx_audio_config(sd, TRUE, g_u16_tmds_clk);
			ms7200_hdmirx_core_reset(sd, HDMI_RX_CTRL_PDEC | HDMI_RX_CTRL_AUD);
			ms7200_hdmirx_interrupt_get_and_clear(sd, RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
			g_u8_rx_stable_timer_count = 0;
			if (g_b_input_valid) {
				g_b_input_valid = FALSE;
				MS_INF("timing unstable\n");
			}
		}
	}
	//get input timing status when mdt stable
	if (!g_b_input_valid) {
		u16_fs = 0;
		ms->audio.audio_fs = 0;
		ms->audio.present = 0;
		if (g_u8_rx_stable_timer_count < RX_STABLE_TIMEOUT) {
			g_u8_rx_stable_timer_count++;
			return;
		}
		g_b_input_valid = ms7200_hdmirx_input_timing_get(sd, &g_t_hdmirx_timing);
		MS_INF("g_t_hdmirx_timing.u16_hactive=%u, g_t_hdmirx_timing.u16_vactive=%u,\n \
				g_t_hdmirx_timing.u8_polarity=%u, g_t_hdmirx_timing.u16_vfreq=%u,\n \
				g_t_hdmirx_timing.u16_pixclk=%u,g_t_hdmirx_timing.u16_hoffset=%u,\n \
				g_t_hdmirx_timing.u16_voffset=%u, g_t_hdmirx_timing.u16_htotal=%u,\n \
				g_t_hdmirx_timing.u16_vtotal=%u, g_t_hdmirx_timing.u16_hsyncwidth=%u,\n \
				g_t_hdmirx_timing.u16_vsyncwidth=%u\n",
				g_t_hdmirx_timing.u16_hactive, g_t_hdmirx_timing.u16_vactive,
				g_t_hdmirx_timing.u8_polarity, g_t_hdmirx_timing.u16_vfreq,
				g_t_hdmirx_timing.u16_pixclk, g_t_hdmirx_timing.u16_hoffset,
				g_t_hdmirx_timing.u16_voffset, g_t_hdmirx_timing.u16_htotal,
				g_t_hdmirx_timing.u16_vtotal, g_t_hdmirx_timing.u16_hsyncwidth,
				g_t_hdmirx_timing.u16_vsyncwidth);
		if (g_b_input_valid) {
			u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(sd, RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
			ms7200_hdmirx_input_infoframe_get(sd, &g_t_hdmirx_infoframe);
			g_t_hdmirx_infoframe.u16_video_clk = g_u16_tmds_clk;
			if (!(u32_int_status & AVI_RCV_ISTS))
				sys_default_hdmi_video_config();
			if (!(u32_int_status & AIF_RCV_ISTS))
				sys_default_hdmi_vendor_specific_config();
			if (!(u32_int_status & VSI_RCV_ISTS))
				sys_default_hdmi_vendor_specific_config();

			ms7200_hdmirx_video_config(sd, &g_t_hdmirx_infoframe);
			u16_fs = ms7200_hdmirx_audio_config(sd, TRUE, g_u16_tmds_clk);
			ms7200_dvout_video_config(sd, TRUE);
			ms7200_dvout_audio_config(sd, TRUE);
			MS_INF("timing stable\n");

			change = 0;
			//signal = ms_signal(sd);
			signal = g_b_input_valid;
			if (ms->tmds_signal != signal) {
				/*for anti signal-shake*/
				//ms_msleep(100);
				//if (signal != ms_signal(sd))
				//	continue;
				MS_INF("tmds signal:%d\n", signal);
				ms->tmds_signal = signal;

				change |= ms->tmds_signal ?
					V4L2_EVENT_SRC_CH_HPD_IN :
					V4L2_EVENT_SRC_CH_HPD_OUT;
			}

			memset(&timing, 0, sizeof(struct v4l2_dv_timings));
			timing.type = V4L2_DV_BT_656_1120;

			MS_INF("hdmi in source infoframe: u8_color_space=%u, u8_color_depth=%u\n",
				g_t_hdmirx_infoframe.u8_color_space, g_t_hdmirx_infoframe.u8_color_depth);
			if (g_t_hdmirx_infoframe.u8_color_space == HDMI_RGB
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_8BIT) {
				MS_INF("hdmi in source input RGB 8bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_RGB
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_10BIT) {
				MS_INF("hdmi in source input RGB 10bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_RGB
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_12BIT) {
				MS_INF("hdmi in source input RGB 12bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_RGB
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_16BIT) {
				MS_INF("hdmi in source input RGB 16bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR422
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_8BIT) {
				MS_INF("hdmi in source input YCBCR422 8bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR422
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_10BIT) {
				MS_INF("hdmi in source input YCBCR422 10bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR422
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_12BIT) {
				MS_INF("hdmi in source input YCBCR422 12bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR422
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_16BIT) {
				MS_INF("hdmi in source input YCBCR422 16bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR444
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_8BIT) {
				MS_INF("hdmi in source input YCBCR444 8bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR444
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_10BIT) {
				MS_INF("hdmi in source input YCBCR444 10bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR444
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_12BIT) {
				MS_INF("hdmi in source input YCBCR444 12bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YCBCR444
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_16BIT) {
				MS_INF("hdmi in source input YCBCR444 16bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YUV420
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_8BIT) {
				MS_INF("hdmi in source input YUV420 8bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YUV420
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_10BIT) {
				MS_INF("hdmi in source input YUV420 10bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YUV420
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_12BIT) {
				MS_INF("hdmi in source input YUV420 12bit\n");
			} else if (g_t_hdmirx_infoframe.u8_color_space == HDMI_YUV420
				&& g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_16BIT) {
				MS_INF("hdmi in source input YUV420 16bit\n");
			}

			//when 10bit, the corresponding clock and horizontal direction timing
			//must be divided by 1.25, and 12bit must be divided by 1.5
			if (g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_10BIT) {
				//g_u16_tmds_clk = (g_u16_tmds_clk * 100)/125;
				g_t_hdmirx_timing.u16_hactive = (g_t_hdmirx_timing.u16_hactive * 100)/125;
			} else if (g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_12BIT) {
				//g_u16_tmds_clk = (g_u16_tmds_clk * 100)/150;
				g_t_hdmirx_timing.u16_hactive = (g_t_hdmirx_timing.u16_hactive * 100)/150;
			} else if (g_t_hdmirx_infoframe.u8_color_depth == HDMI_COLOR_DEPTH_16BIT) {
				//wait to check later...
				//g_u16_tmds_clk = (g_u16_tmds_clk * 100)/175;
				//g_t_hdmirx_timing.u16_hactive = (g_t_hdmirx_timing.u16_hactive * 100)/175;
			}

			timing.bt.width = g_t_hdmirx_timing.u16_hactive;
			timing.bt.height = g_t_hdmirx_timing.u16_vactive;
			timing.bt.hsync = g_t_hdmirx_timing.u16_htotal - g_t_hdmirx_timing.u16_hactive;
			timing.bt.vsync = g_t_hdmirx_timing.u16_vtotal - g_t_hdmirx_timing.u16_vactive;
			timing.bt.pixelclock = g_u16_tmds_clk * 10000;  //g_u16_tmds_clk=14854

			//1080i use single edge sampling, others use double edge sampling.
			//Use the REG_DIG_CLK_SEL0(0xc0) register to set SDR and DDR mode of ms7200.
			//Use ms7200_hvref_clk_pol_change for sensor_g_mbus_config interface
			//to set the csi single edge and double edge sampling of H700.
			if (g_t_hdmirx_timing.u16_hactive == 1920 && g_t_hdmirx_timing.u16_vactive == 1080 &&
				(g_t_hdmirx_timing.u8_polarity == 0 || g_t_hdmirx_timing.u8_polarity == 2 ||
				g_t_hdmirx_timing.u8_polarity == 4 || g_t_hdmirx_timing.u8_polarity == 6)) {
				timing.bt.interlaced = V4L2_DV_INTERLACED;
				//ms7200_dvout_init(sd, &g_t_dvout_sdr_config, 0);
				//ms7200drv_dvout_mode_config(sd, &g_t_dvout_sdr_config);
				//ms_write8_mask(sd, REG_DIG_CLK_SEL0, MSRT_BITS3_2, 0x0);
				ms_write8(sd, REG_DIG_CLK_SEL0, 0x0); //single edge sampling
				ms7200_hvref_clk_pol_change = CLK_POL_CHANGE; //for sensor_g_mbus_config
			} else if ((timing.bt.width == 720 && timing.bt.height == 480) ||
				(timing.bt.width == 720 && timing.bt.height == 576)) {
				//ms_write8_mask(sd, REG_DIG_CLK_SEL0, MSRT_BITS3_2, MSRT_BIT3);
				ms_write8(sd, REG_DIG_CLK_SEL0, MSRT_BIT3); //double edge sampling
				ms7200_hvref_clk_pol_change = HVREF_POL_CHANGE_HIGH; //for sensor_g_mbus_config
			} else {
				//ms_write8_mask(sd, REG_DIG_CLK_SEL0, MSRT_BITS3_2, MSRT_BIT3);
				//ms_write8(sd, REG_DIG_CLK_SEL0, MSRT_BIT3); //double edge sampling
				ms7200_hvref_clk_pol_change = HVREF_POL_CHANGE_LOW; //for sensor_g_mbus_config
			}

			MS_INF("hdmi in source infoframe: u8_hdmi_flag=%u, u8_audio_mode=%u, u8_audio_rate=%u,\n \
					u8_audio_bits=%u, u8_audio_channels=%u, u8_audio_speaker_locations=%u\n",
					g_t_hdmirx_infoframe.u8_hdmi_flag, g_t_hdmirx_infoframe.u8_audio_mode,
					g_t_hdmirx_infoframe.u8_audio_rate, g_t_hdmirx_infoframe.u8_audio_bits,
					g_t_hdmirx_infoframe.u8_audio_channels, g_t_hdmirx_infoframe.u8_audio_speaker_locations);
			MS_INF("g_t_hdmirx_timing.u16_hactive=%u, g_t_hdmirx_timing.u16_vactive=%u,\n \
					g_t_hdmirx_timing.u8_polarity=%u, g_t_hdmirx_timing.u16_vfreq=%u,\n \
					g_t_hdmirx_timing.u16_pixclk=%u, g_t_hdmirx_timing.u16_hoffset=%u,\n \
					g_t_hdmirx_timing.u16_voffset=%u, g_t_hdmirx_timing.u16_htotal=%u,\n \
					g_t_hdmirx_timing.u16_vtotal=%u, g_t_hdmirx_timing.u16_hsyncwidth=%u,\n \
					g_t_hdmirx_timing.u16_vsyncwidth=%u\n",
					g_t_hdmirx_timing.u16_hactive, g_t_hdmirx_timing.u16_vactive,
					g_t_hdmirx_timing.u8_polarity, g_t_hdmirx_timing.u16_vfreq,
					g_t_hdmirx_timing.u16_pixclk, g_t_hdmirx_timing.u16_hoffset,
					g_t_hdmirx_timing.u16_voffset, g_t_hdmirx_timing.u16_htotal,
					g_t_hdmirx_timing.u16_vtotal, g_t_hdmirx_timing.u16_hsyncwidth,
					g_t_hdmirx_timing.u16_vsyncwidth);
			if (!v4l2_match_dv_timings(&ms->timing, &timing, 0, false)) {
				MS_INF("ms resolution change!\n");

				ms->wait4get_timing = 1;
				//change |= V4L2_EVENT_SRC_CH_HPD_IN;
				change |= V4L2_EVENT_SRC_CH_RESOLUTION;
			}

			//Notify user-space the resolution_change event
			if (change) {
				if (v4l2_ctrl_s_ctrl(ms->tmds_signal_ctrl,
					ms->tmds_signal) < 0) {
					MS_ERR("v4l2_ctrl_s_ctrl for tmds signal failed!\n");
				}

				memset(&ms_ev_fmt, 0, sizeof(ms_ev_fmt));
				ms_ev_fmt.type = V4L2_EVENT_SOURCE_CHANGE;
				ms_ev_fmt.u.src_change.changes = change;
				if (ms->signal_event_subscribe)
					v4l2_subdev_notify_event(sd, &ms_ev_fmt);
			}
		} else {
			MS_INF("timing error\n");
		}
		return;
	}
	//reconfig system when input packet change
	u32_int_status = ms7200_hdmirx_interrupt_get_and_clear(sd, RX_INT_INDEX_PDEC, PDEC_ALL_ISTS, TRUE);
	if (u32_int_status & (AVI_CKS_CHG_ISTS | AIF_CKS_CHG_ISTS | VSI_CKS_CHG_ISTS)) {
		ms7200_hdmirx_input_infoframe_get(sd, &t_hdmi_infoframe);
		t_hdmi_infoframe.u16_video_clk = g_u16_tmds_clk;
		if (!(u32_int_status & AVI_RCV_ISTS))
			sys_default_hdmi_video_config();
		if (!(u32_int_status & AIF_RCV_ISTS))
			sys_default_hdmi_vendor_specific_config();
		if (!(u32_int_status & VSI_RCV_ISTS))
			sys_default_hdmi_vendor_specific_config();
		if (memcmp(&t_hdmi_infoframe, &g_t_hdmirx_infoframe, sizeof(HDMI_CONFIG_T)) != 0) {
			ms7200_dvout_audio_config(sd, FALSE);
			ms7200_dvout_video_config(sd, FALSE);
			ms7200_hdmirx_video_config(sd, &t_hdmi_infoframe);
			u16_fs = ms7200_hdmirx_audio_config(sd, TRUE, g_u16_tmds_clk);
			ms7200_dvout_video_config(sd, TRUE);
			ms7200_dvout_audio_config(sd, TRUE);
			g_t_hdmirx_infoframe = t_hdmi_infoframe;
			MS_INF("infoframe change\n");
		}
	}
	//reconfig audio when acr change or audio fifo error
	if (u32_int_status & ACR_RCV_ISTS) {
		//input audio clk valid
		if (u32_int_status & (ACR_CTS_CHG_ISTS | ACR_N_CHG_ISTS)) {
			u16_fs = ms7200_hdmirx_audio_config(sd, TRUE, g_u16_tmds_clk);
			ms->audio.audio_fs = 0;
			ms->audio.present = 0;
			MS_INF("acr change\n");
		} else {
			u32_int_status = ms7200_hdmirx_audio_fifo_status_get(sd);
			if (u32_int_status & AFIF_THS_PASS_STS) {
				if (u32_int_status & (AFIF_UNDERFL_STS | AFIF_OVERFL_STS)) {
					ms7200_hdmirx_audio_config(sd, FALSE, g_u16_tmds_clk);
					ms->audio.audio_fs = 0;
					ms->audio.present = 0;
					MS_INF("audio fifo reset\n");
				} else {
					if (ms->audio.present == 0 && ms->audio.audio_fs == 0) {
						MS_INF("%s: ms7200_audio_event_subscribe\n", __func__);
						u16_fs = ms7200_hdmirx_audio_config(sd, TRUE, g_u16_tmds_clk);
						ms->audio.audio_fs = u16_fs;
						ms7200_audio_event_subscribe(sd, u16_fs);
						ms->audio.present = 1;
					}
				}
			} else {
				MS_INF("abnormal audio data\n");
				ms->audio.audio_fs = 0;
				ms->audio.present = 0;
			}
		}
	} else {
		//input audio clk invalid
		MS_INF("input audio clk invalid\n");
		u16_fs = 0;
		ms->audio.audio_fs = 0;
		ms->audio.present = 0;
	}
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	MS_INF("%s\n", __func__);
	switch (on) {
	case STBY_ON:
		MS_INF("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			MS_ERR("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		MS_INF("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			MS_ERR("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_print("PWR_ON! do nothing\n");
		// cci_lock(sd);
		// vin_set_pmu_channel(sd, IOVDD, ON);
		// usleep_range(1000, 1200);
		// cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF! do nothing\n");
		// vin_set_pmu_channel(sd, IOVDD, OFF);
		// usleep_range(1000, 1200);
		// vin_gpio_set_status(sd, RESET, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	MS_INF("%s %d\n", __func__, val);
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
	int i = 0;
	int rdval = 0;

	rdval = ms_read8(sd, REG_CHIPID1_REG);	// 0x20
	while ((MS7200_CHIP_ID1 != rdval)) {
		i++;
		rdval = ms_read8(sd, REG_CHIPID1_REG);
		if (i > 4) {
				MS_INF("warning:CHIPID1(0x%x) is NOT equal to 0x%x\n",
					rdval, MS7200_CHIP_ID1);
				break;
		}
	}
	if (MS7200_CHIP_ID1 != rdval)
		return -ENODEV;

	sensor_print("----------- [sensor id info] -----------\n");
	rdval = ms_read8(sd, REG_CHIPID0_REG);	// 0xa2
	sensor_print("CHIPID0: 0x%02x\n", rdval);
	rdval = ms_read8(sd, REG_CHIPID1_REG);	// 0x20
	sensor_print("CHIPID0: 0x%02x\n", rdval);
	rdval = ms_read8(sd, REG_CHIPID2_REG);	// 0x0a
	sensor_print("CHIPID0: 0x%02x\n", rdval);

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	struct sensor_info *info = to_state(sd);
	int ret;

	MS_INF("sensor_init\n");

	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1920;
	info->height = 1080;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	//info->tpf.denominator = 30;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	MS_INF("%s cmd:%d\n", __func__, cmd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(struct sensor_win_size));
			MS_INF("%s cmd:%d  info->width=%d, info->height=%d\n", __func__, cmd, info->width, info->height);
			ret = 0;
		} else {
			MS_ERR("empty wins!\n");
			ret = -1;
		}
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
		.desc = "mipi",
#if MS7200_DVO_16BIT
		//.mbus_code	= MEDIA_BUS_FMT_UYVY8_1X16,
		//.mbus_code	= MEDIA_BUS_FMT_VYUY8_1X16,
		//.mbus_code	= MEDIA_BUS_FMT_YUYV8_1X16,
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_1X16,
#else
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		//.mbus_code = MEDIA_BUS_FMT_VYUY8_2X8,
		//.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
		// .mbus_code = MEDIA_BUS_FMT_YVYU8_2X8,
#endif
		.regs = NULL,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)


/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	{
	.width			= 720,  //480p 60fps
	.height			= 480,
	.hoffset		= 60,
	.voffset		= 30,
	.fps_fixed		= 60,
	},
	{
	.width			= 720,  //576p 50fps
	.height			= 576,
	.hoffset		= 68,
	.voffset		= 39,
	.fps_fixed		= 50,
	},
	{
	.width 			= 1280,  //720p 50fps 60fps
	.height			= 720,
	.hoffset		= 220,
	.voffset		= 20,
	.fps_fixed		= 60,
	},
	{
	.width			= 1920,  //1080p 50fps 60fps
	.height			= 1080,
	.hoffset		= 148,
	.voffset		= 36,
	.fps_fixed		= 60,
	},
	{
	.width 			= 1920,  //1080i 50fps 60fps
	.height			= 1080,
	.hoffset		= 148,
	.voffset		= 15,
	.fps_fixed		= 50,
	},
#if MS7200_DVO_16BIT
	{
	.width			= 3840,  //4k(3840) 24fps 25fps 30fps
	.height			= 2160,
	.hoffset		= 296,
	.voffset		= 72,
	.fps_fixed		= 30,
	},
	{
	.width			= 4096,  //4096 24fps
	.height			= 2160,
	.hoffset		= 296,
	.voffset		= 72,
	.fps_fixed		= 24,
	},
	{
	.width			= 4096,  //4k(4096) 25fps 30fps
	.height			= 2160,
	.hoffset		= 128,
	.voffset		= 72,
	.fps_fixed		= 30,
	},
#endif
};

#define N_WIN_SIZES			(ARRAY_SIZE(sensor_win_sizes))
#define VREF_POL_LOW		V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL_LOW		V4L2_MBUS_HSYNC_ACTIVE_LOW
#define VREF_POL_HIGH		V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL_HIGH		V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL				V4L2_MBUS_PCLK_SAMPLE_RISING
#define CLK_DPOL			(V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_PCLK_SAMPLE_FALLING)

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (ms7200_hvref_clk_pol_change == HVREF_POL_CHANGE_LOW) {
		cfg->bus.parallel.flags = V4L2_MBUS_MASTER | VREF_POL_LOW | HREF_POL_LOW | CLK_DPOL;   //720p 1080p 4k
	} else if (ms7200_hvref_clk_pol_change ==  HVREF_POL_CHANGE_HIGH) {
		cfg->bus.parallel.flags = V4L2_MBUS_MASTER | VREF_POL_HIGH | HREF_POL_HIGH | CLK_DPOL;  //480p 576p
	} else if (ms7200_hvref_clk_pol_change == CLK_POL_CHANGE) {
		cfg->bus.parallel.flags = V4L2_MBUS_MASTER | VREF_POL_LOW | HREF_POL_LOW | CLK_POL;   //1080i
	}
#else
	if (ms7200_hvref_clk_pol_change ==  HVREF_POL_CHANGE_LOW) {
		cfg->flags = V4L2_MBUS_MASTER | VREF_POL_LOW | HREF_POL_LOW | CLK_DPOL;   //720p 1080p 4k
	} else if (ms7200_hvref_clk_pol_change ==  HVREF_POL_CHANGE_HIGH) {
		cfg->flags = V4L2_MBUS_MASTER | VREF_POL_HIGH | HREF_POL_HIGH | CLK_DPOL;  //480p 576p
	} else if (ms7200_hvref_clk_pol_change == CLK_POL_CHANGE) {
		cfg->flags = V4L2_MBUS_MASTER | VREF_POL_LOW | HREF_POL_LOW | CLK_POL;   //1080i
	}
#endif

	return 0;
}

static int ms_enum_dv_timings(struct v4l2_subdev *sd,
			struct v4l2_enum_dv_timings *timings)
{
	printk("subdev feekback function:%s\n", __func__);
	if (timings->pad != 0)
		return -EINVAL;

	return v4l2_enum_dv_timings_cap(timings,
		&ms7200_timings_cap, NULL, NULL);
}

static int ms_query_dv_timings(struct v4l2_subdev *sd, struct v4l2_dv_timings *timings)
{
	int ret;

	printk("subdev feekback function:%s\n", __func__);
	ret = ms_get_detected_timings(sd, timings);
	if (ret)
		return ret;

	if (!v4l2_valid_dv_timings(timings,
			&ms7200_timings_cap, NULL, NULL)) {
		MS_ERR("%s: timings out of range\n", __func__);
		return -ERANGE;
	}


	v4l2_print_dv_timings(sd->name, "ms7200 format detected: ",
				timings, true);

	return 0;
}

static int ms_get_dv_timings(struct v4l2_subdev *sd, struct v4l2_dv_timings *timings)
{
	int ret, cnt = 0;
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	//read the timings until to get a valid timing,
	//e.g. until the ms7200 to get the hdmi signal stably
	while (cnt < 15) { //over 3s, then timeout!
		ret = ms_get_detected_timings(sd, timings);
		if (ret)
			return ret;

		if (v4l2_valid_dv_timings(timings,
			&ms7200_timings_cap, NULL, NULL)) {
			ms->wait4get_timing = 0;

			//store the valid timings
			memcpy(&ms->timing, timings, sizeof(*timings));

			v4l2_print_dv_timings(sd->name,
				"ms7200 format detected: ",
				timings, true);
			break;
		}

		msleep(200);
		cnt++;

		MS_INF2("%s: invalid timings\n", __func__);
	}

	if (ms7200_hdmirx_source_connect_detect(sd) && ms->audio.audio_fs && ms->audio.present) {
		MS_INF("%s: ms7200_audio_event_subscribe\n", __func__);
		ms7200_audio_event_subscribe(sd, ms->audio.audio_fs);
	}

	return 0;
}

static int ms_dv_timings_cap(struct v4l2_subdev *sd,
		struct v4l2_dv_timings_cap *cap)
{
	printk("subdev feekback function:%s\n", __func__);
	if (cap->pad != 0)
		return -EINVAL;

	*cap = ms7200_timings_cap;

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	//struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	printk("subdev feekback function:%s  enable:%d\n", __func__,  enable);
	MS_INF("%s on = %d, %d*%d fps: %d code: %x sensor_field: %d\n", __func__, enable,
		info->current_wins->width, info->current_wins->height,
		info->current_wins->fps_fixed, info->fmt->mbus_code, info->sensor_field);

	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

static int ms_set_edid(struct v4l2_subdev *sd, struct v4l2_subdev_edid *edid)
{
	sensor_print("subdev feekback function:%s\n", __func__);
	sensor_dbg("%s, pad %d, start block %d, blocks %d\n",
		__func__, edid->pad, edid->start_block, edid->blocks);

	if (edid->edid && edid->blocks) {
		ms_write8_regs(sd, REG_EDID_BLOCK0, 0x100, edid->edid);
	} else {
		sensor_err("%s, edid->edid or edid->blocks is NULL!!!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int ms_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				struct v4l2_event_subscription *sub)
{
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;
	printk("subdev feekback function:%s\n", __func__);

	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		printk("V4L2_EVENT_SOURCE_CHANGE\n");
		ms->signal_event_subscribe = 1;
		return v4l2_src_change_event_subdev_subscribe(sd, fh, sub);
	case V4L2_EVENT_CTRL:
		printk("V4L2_EVENT_CTRL\n");
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	default:
		return -EINVAL;
	}
}

static int ms_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
					struct v4l2_event_subscription *sub)
{
	   struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	   switch (sub->type) {
	   case V4L2_EVENT_SOURCE_CHANGE:
			   ms->signal_event_subscribe = 0;
			   break;
	   case V4L2_EVENT_CTRL:
			   break;
	   default:
			   return -EINVAL;
	   }
	   return v4l2_event_subdev_unsubscribe(sd, fh, sub);
}

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.subscribe_event = ms_subscribe_event,
	.unsubscribe_event = ms_unsubscribe_event,
	.ioctl = sensor_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	//.s_parm = sensor_s_parm,
	//.g_parm = sensor_g_parm,
	.g_dv_timings = ms_get_dv_timings,
	.query_dv_timings = ms_query_dv_timings,
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_dv_timings = ms_enum_dv_timings,
	.dv_timings_cap = ms_dv_timings_cap,
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.set_edid = ms_set_edid,
	.get_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

#if VIN_FALSE
static ssize_t ms_reg_dump_all_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	unsigned short i = 0;

	n += sprintf(buf + n, "\nGlobal Control Register:\n");
	for (i = 0; i < 0x0100; i += 2) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%04x ", ms_read16(sd, i));
	}

	n += sprintf(buf + n, "\nCSI2 TX PHY/PPI  Register:\n");
	for (i = 0x0100; i < 0x023c; i += 4) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%08x ", ms_read32(sd, i));
	}

	n += sprintf(buf + n, "\nCSI2 TX Control  Register:\n");
	for (i = 0x040c; i < 0x051c; i += 4) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%08x ", ms_read32(sd, i));
	}

	n += sprintf(buf + n, "\nHDMIRX  Register:\n");
	for (i = 0x8500; i < 0x8844; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", ms_read8(sd, i));
	}
	n += sprintf(buf + n, "\n");

	return n;
}
#endif

static ssize_t ms_reg_dump_all_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	unsigned short i = 0;

	n += sprintf(buf + n, "\nMISC/DVOUT/HDMI RX MISC  Register:\n");
	for (i = 0; i < 0x0100; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", ms_read8(sd, i));
	}

	n += sprintf(buf + n, "\nHDMI RX RCV  Register:\n");
	for (i = 0x0200; i < 0x0215; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", ms_read8(sd, i));
	}

	n += sprintf(buf + n, "\nHDMI RX PHY  Register:\n");
	for (i = 0x1000; i < 0x103b; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", ms_read8(sd, i));
	}

	n += sprintf(buf + n, "\nPhase  Register:\n");
	for (i = 0x1281; i < 0x1282; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", ms_read8(sd, i));
	}

	n += sprintf(buf + n, "\nHDMI RX Controller  Register:\n");
	for (i = 0x2000; i < 0x2403; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", ms_read8(sd, i));
	}
	n += sprintf(buf + n, "\n");

	return n;
}

static u32 ms_reg_read_start, ms_reg_read_end;
/*static ssize_t ms_read_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	u32 reg_index;

	if ((ms_reg_read_start > ms_reg_read_end)
		|| (ms_reg_read_end - ms_reg_read_start > 0x4ff)
		|| (ms_reg_read_end > 0xffff)) {
		n += sprintf(buf + n, "invalid reg addr input:(0x%x, 0x%x)\n",
				ms_reg_read_start, ms_reg_read_end);
		return n;
	}

	for (reg_index = ms_reg_read_start; reg_index <= ms_reg_read_end;) {
		if (reg_index < 0x0100) { //Global Control Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%04x ", ms_read16(sd, reg_index));
			reg_index += 2;
		} else if (reg_index < 0x051c) { //CSI2 TX PHY/PPI  Register and CSI2 TX Control  Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%08x ", ms_read32(sd, reg_index));
			reg_index += 4;
		} else if ((reg_index >= 0x8500) && (reg_index < 0xffff)) { // HDMI RX Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%02x ", ms_read8(sd, reg_index));
			reg_index++;
		}
	}

	n += sprintf(buf + n, "\n");

	return n;
}*/

static ssize_t ms_read_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	u32 reg_index;

	if ((ms_reg_read_start > ms_reg_read_end)
		|| (ms_reg_read_end - ms_reg_read_start > 0x4ff)
		|| (ms_reg_read_end > 0xffff)) {
		n += sprintf(buf + n, "invalid reg addr input:(0x%x, 0x%x)\n",
				ms_reg_read_start, ms_reg_read_end);
		return n;
	}

	for (reg_index = ms_reg_read_start; reg_index <= ms_reg_read_end;) {
		if (reg_index < 0xffff) {
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%02x ", ms_read8(sd, reg_index));
			reg_index++;
		}
	}

	n += sprintf(buf + n, "\n");

	return n;
}


static ssize_t ms_read_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	u8 *separator;

	separator = strchr(buf, ',');
	if (separator != NULL) {
		ms_reg_read_start = simple_strtoul(buf, NULL, 0);
		ms_reg_read_end = simple_strtoul(separator + 1, NULL, 0);
	} else {
		MS_ERR("Invalid input!must add a comma as separator\n");
	}

	return count;
}

static u32 ms_reg_write_addr,  ms_reg_write_value;
static ssize_t ms_write_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n,  "echo [addr] [value] > ms_write\n");
	return n;
}

static ssize_t ms_write_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;

	u8 *separator;

	separator = strchr(buf, ' ');
	if (separator != NULL) {
		ms_reg_write_addr = simple_strtoul(buf, NULL, 0);
		ms_reg_write_value = simple_strtoul(separator + 1, NULL, 0);
	} else {
		MS_ERR("Invalid input!must add a space as separator\n");
	}

	/*if (ms_reg_write_addr < 0x0100)
		ms_write16(sd, ms_reg_write_addr, ms_reg_write_value);
	else if (ms_reg_write_addr >= 0x0100 && ms_reg_write_addr < 0x051c)
		ms_write32(sd, ms_reg_write_addr, ms_reg_write_value);
	else if (ms_reg_write_addr >= 0x051c && ms_reg_write_addr <= 0xffff)
		ms_write8(sd, ms_reg_write_addr, ms_reg_write_value);*/

	if (ms_reg_write_addr <= 0x2081)
		ms_write8(sd, ms_reg_write_addr, ms_reg_write_value);
	return count;
}

static ssize_t ms_debug_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n, "ms_debug:%d\n", ms_debug);
	return n;
}

static ssize_t ms_debug_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	if (strncmp(buf, "0", 1) == 0)
		ms_debug = 0;
	else
		ms_debug = 1;

	return count;
}

static ssize_t ms_colorbar_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	n += sprintf(buf + n, "colorbar enable:%u\n", ms->colorbar_enable);
	return n;
}

static ssize_t ms_colorbar_enable_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	struct hdmi_rx_ms7200 *ms = (struct hdmi_rx_ms7200 *)sd;

	if (strncmp(buf, "0", 1) == 0) {
		colorbar_1024x768(sd);
		ms->colorbar_enable = 1;
	} else {
		ms->colorbar_enable = 0;
		set_capture2(sd, 0, 0);
	}

	return count;
}

static ssize_t ms_hdmi_hot_plug_status_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n = sprintf(buf, "%d\n", ms7200_hdmi_hot_plug_status);
	return n;
}

static struct device_attribute ms_device_attrs[] = {
	__ATTR(ms_read, S_IWUSR | S_IRUGO, ms_read_show, ms_read_store),
	__ATTR(ms_write, S_IWUSR | S_IRUGO, ms_write_show, ms_write_store),
	__ATTR(ms_debug, S_IWUSR | S_IRUGO, ms_debug_show, ms_debug_store),
	__ATTR(colorbar_enable, S_IWUSR | S_IRUGO, ms_colorbar_enable_show,
				ms_colorbar_enable_store),
	__ATTR(reg_dump_all, S_IRUGO, ms_reg_dump_all_show, NULL),
	__ATTR(hot_plug_status, S_IRUGO, ms_hdmi_hot_plug_status_show, NULL),
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_16,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_16,
	}
};

__maybe_unused static bool ms_signal(struct v4l2_subdev *sd)
{
	//return (ms_read8(sd, SYS_STATUS) & MASK_S_TMDS);
	return ms_read8(sd, REG_MISC_5V_DET) & MSRT_BIT0;
}

static int ms_get_detected_timings(struct v4l2_subdev *sd,
			struct v4l2_dv_timings *timings)
{
	struct v4l2_bt_timings *bt = &timings->bt;
	struct sensor_info *info = to_state(sd);
	//unsigned width, height, frame_width, frame_height, frame_interval, fps;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));

	/*if (!ms_signal(sd)) {
				MS_INF2("%s: no valid signal\n", __func__);
		return -ENOLINK;
	}*/

	timings->type = V4L2_DV_BT_656_1120;

	bt->width = g_t_hdmirx_timing.u16_hactive;
	bt->height = g_t_hdmirx_timing.u16_vactive;
	bt->hsync = g_t_hdmirx_timing.u16_htotal - g_t_hdmirx_timing.u16_hactive;
	bt->vsync = g_t_hdmirx_timing.u16_vtotal - g_t_hdmirx_timing.u16_vactive;
	bt->pixelclock = g_u16_tmds_clk * 10000;  //g_u16_tmds_clk=14854

	if (g_t_hdmirx_timing.u16_hactive == 1920 \
			&& g_t_hdmirx_timing.u16_vactive == 1080 \
			&& (g_t_hdmirx_timing.u8_polarity == 1 \
			|| g_t_hdmirx_timing.u8_polarity == 3 \
			|| g_t_hdmirx_timing.u8_polarity == 5 \
			|| g_t_hdmirx_timing.u8_polarity == 7))
		info->tpf.denominator = 60;
	else if (g_t_hdmirx_timing.u16_hactive == 1920 \
			&& g_t_hdmirx_timing.u16_vactive == 1080 \
			&& (g_t_hdmirx_timing.u8_polarity == 0 \
			|| g_t_hdmirx_timing.u8_polarity == 2 \
			|| g_t_hdmirx_timing.u8_polarity == 4 \
			|| g_t_hdmirx_timing.u8_polarity == 6)) {
		bt->interlaced = V4L2_DV_INTERLACED;
		info->tpf.denominator = 50;
	} else if (g_t_hdmirx_timing.u16_hactive == 4096 \
			&& g_t_hdmirx_timing.u16_vactive == 2160 \
			&& (g_t_hdmirx_timing.u16_hoffset - g_t_hdmirx_timing.u16_hsyncwidth == 296))
		info->tpf.denominator = 24;
	else if (g_t_hdmirx_timing.u16_hactive == 4096 \
			&& g_t_hdmirx_timing.u16_vactive == 2160 \
			&& (g_t_hdmirx_timing.u16_hoffset - g_t_hdmirx_timing.u16_hsyncwidth == 128))
		info->tpf.denominator = 30;
	else
		info->tpf.denominator = 0;

	if (bt->interlaced == V4L2_DV_INTERLACED) {
		info->sensor_field = V4L2_FIELD_INTERLACED;
	} else {
		info->sensor_field = V4L2_FIELD_NONE;
	}

	return 0;
}

static int ms_run_thread(void *parg)
{
	struct v4l2_subdev *sd = (struct v4l2_subdev *)parg;
	int count = 0;

	msleep(1000);
	sensor_print("ms_run_thread begin...\n");

	while (1) {
		if (kthread_should_stop())
			break;

		if (!ms7200_chip_connect_detect(sd)) {
			MS_INF("ms7200 not connect!\n");
			ms_msleep(100);
			continue;
		}

		ms7200_detected_resolution_change(sd);

		if (count == 10) {
			ms7200_hdmirx_hot_plug_status_set(sd);
			ms7200_media_service(sd);
			count = 0;
		}

		count++;
		ms_msleep(10);
	}

	return 0;
}

#define TC358743_CID_TMDS_SIGNAL   (V4L2_CID_USER_TC358743_BASE + 3)
static const struct v4l2_ctrl_config tc358743_ctrl_tmds_signal_present = {
	.id = TC358743_CID_TMDS_SIGNAL,
	.name = "tmds signal",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
	.flags = V4L2_CTRL_FLAG_READ_ONLY,
};

static int sensor_det_init(struct v4l2_subdev *sd)
{
	int ret;
	struct device_node *np = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	enum of_gpio_flags gc;
#endif
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;
	char *node_name = "sensor_detect";
	char *reset_gpio_name = "reset_gpios";

	np = of_find_node_by_name(NULL, node_name);
	if (np == NULL) {
		sensor_err("can not find the %s node\n", node_name);
		return -EINVAL;
	}

	/* reset sensor */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	sensor_indet->reset_gpio.gpio = of_get_named_gpio_flags(np, reset_gpio_name, 0, &gc);
#else
	sensor_indet->reset_gpio.gpio = of_get_named_gpio(np, reset_gpio_name, 0);
#endif
	sensor_dbg("get form %s gpio is %d\n", reset_gpio_name, sensor_indet->reset_gpio.gpio);
	if (!gpio_is_valid(sensor_indet->reset_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", reset_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(sensor_indet->reset_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", reset_gpio_name);
			return -1;
		}
		gpio_direction_output(sensor_indet->reset_gpio.gpio, 1);
		usleep_range(5000, 5100);
		gpio_direction_output(sensor_indet->reset_gpio.gpio, 0);
		usleep_range(5000, 5100);
		gpio_direction_output(sensor_indet->reset_gpio.gpio, 1);
	}

	sensor_dbg("%s seccuss\n", __func__);
	return 0;
}

static void sensor_det_exit(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	/* free gpio resources */
	if (sensor_indet->reset_gpio.gpio > 0)
		gpio_free(sensor_indet->reset_gpio.gpio);

	return;
}

static int sensor_dev_id;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	struct hdmi_rx_ms7200 *ms;
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	struct cci_driver *ms_cci_drv;
	int i, ret;

	MS_INF("sensor_probe start!\n");

	if (!client) {
		MS_ERR("i2c_client is NULL, do NOT find any i2c master driver for ms7200 sensor!\n");
		return -1;
	}

	for (i = 0; i < SENSOR_NUM; i++) {
		if (!strcmp(cci_drv[i].name, client->name))
			break;
	}

	if (i >= SENSOR_NUM) {
		MS_ERR("NOT find any matched cci driver for client(%s)\n", client->name);
		return -1;
	}
	MS_INF("find the matched cci driver for client(%s)\n", client->name);

	ms = &ms7200[i];
	info = &ms->info;
	sd = &info->sd;
	ms_cci_drv =  &cci_drv[i];

	ms->refclk_hz = 27000000;
	ms->ddc5v_delays = DDC5V_DELAY_100_MS;

	ms->client = client;

	cci_dev_probe_helper(sd, client, &sensor_ops, ms_cci_drv);
	for (i = 0; i < ARRAY_SIZE(ms_device_attrs); i++) {
		ret = device_create_file(&ms_cci_drv->cci_device,
					&ms_device_attrs[i]);
		if (ret) {
			MS_ERR("device_create_file failed, index:%d\n", i);
			continue;
		}
	}

	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;

	// ret = v4l2_async_register_subdev(sd);
	// if (ret < 0) {
	// 	MS_ERR("v4l2_async_register_subdev failed\n");
	// }

	v4l2_ctrl_handler_init(&ms->hdl, 1);
	ms->tmds_signal_ctrl = v4l2_ctrl_new_custom(&ms->hdl,
			&tc358743_ctrl_tmds_signal_present, NULL);
	ret = v4l2_ctrl_handler_setup(&ms->hdl);
	if (ret) {
		MS_ERR("v4l2_ctrl_handler_setup!\n");
		return -1;
	}
	sd->ctrl_handler = &ms->hdl;

	pr_info("v4l2_dev:%p\n", sd->v4l2_dev);

	mutex_init(&info->lock);
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	/*Phase delay*/
	// info->pclk_dly = PCLK_DLY;
	// info->field_dly = FIELD_DLY;
	// info->vsync_dly = VSYNC_DLY;
	// info->hsync_dly = HSYNC_DLY;

	/* identify interlace only by vsync */
	// info->sensor_field_dt = VSYNC_ONLY;

	sensor_det_init(sd);
	ms7200_init(sd);

	//Create hdmi thread
	set_capture2(sd, 0, 0);
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
	struct hdmi_rx_ms7200 *ms;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}

		ms = &ms7200[i];
		sd = (struct v4l2_subdev *)ms;

		kthread_stop(ms->ms_task);
		msleep(10);

		sensor_det_exit(&ms->info.sd);
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
		//v4l2_async_unregister_subdev(sd);
	} else {
		sensor_det_exit(cci_drv[sensor_dev_id].sd);
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

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

	MS_INF("init_sensor!\n");
	for (i = 0; i < SENSOR_NUM; i++) {
		ret = cci_dev_init_helper(&sensor_driver[i]);
		if (ret < 0)
			MS_ERR("cci_dev_init_helper failed, sen\n");
	}

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

#if IS_ENABLED(CONFIG_VIDEO_SUNXI_VIN_SPECIAL)
subsys_initcall_sync(init_sensor);
#else
module_init(init_sensor);
#endif
module_exit(exit_sensor);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("gucheng@allwinnertech.com");
MODULE_DESCRIPTION("MS7200 HDMI RX module driver");
MODULE_VERSION("1.0");
