/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _AW_HDMIRX_DEV_H_
#define _AW_HDMIRX_DEV_H_

typedef struct {
	/* hdmirx devices */
	dev_t          hdmirx_devid;
	struct cdev   *hdmirx_cdev;
	struct class  *hdmirx_class;
	struct device *hdmirx_device;
} aw_device_rx_t;

struct hdmirx_reg_t {
	const char *name;
	int start;
	int end;
};

struct hdmirx_reg_t hdmirx_regs[] = {
	{"Help",                       0x00000, 0x00000},
	{"HDMI_MV_TOP",                0X01017, 0X01045 + 1},
	{"HDMI_PHY0_0",                0X000a7, 0x000fe + 1},
	{"HDMI_PHY0_1",                0x00183, 0x001Bf + 1},
	{"HDMI_PHY1_0",                0x008a7, 0x008fe + 1},
	{"HDMI_PHY1_1",                0x00983, 0x009bf + 1},
	{"HDMI_AUDIO_PLL",             0x80000, 0x8001B + 1},
	{"HDMI_LIB_MV_TOP",            0x41000, 0x41062 + 1},
	{"HDMI_RX_IP_00",              0x40000, 0x400f0 + 1},
	{"HDMI_RX_IP_01",              0x40103, 0x401FF + 1},
	{"HDMI_RX_IP_02",              0x40201, 0x402FF + 1},
	{"HDMI_RX_IP_03",              0x40300, 0x403FF + 1},
	{"HDMI_RX_IP_04",              0x40410, 0x4044C + 1},
	{"HDMI_RX_PHY_00",             0x400f1, 0x400ff + 1},
	{"HDMI_RX_PHY_01",             0x401A9, 0x401BF + 1},
};

#define HDMIRX_IOC_MAGIC 'H'
#define AW_IOCTL_HDMIRX_NULL _IO(HDMIRX_IOC_MAGIC, 0x0)
#define AW_IOCTL_HDMIRX_SET_SOURCE _IOW(HDMIRX_IOC_MAGIC, 0x01, unsigned int)
#define AW_IOCTL_HDMIRX_SET_HPDTIMEINTERVAL _IOW(HDMIRX_IOC_MAGIC, 0x02, unsigned int)
#define AW_IOCTL_HDMIRX_SET_PORTMAP _IOW(HDMIRX_IOC_MAGIC, 0x03, unsigned int)
#define AW_IOCTL_HDMIRX_SET_UPDATEEDID _IOW(HDMIRX_IOC_MAGIC, 0x04, unsigned int)
#define AW_IOCTL_HDMIRX_SET_ARCENABLE _IOW(HDMIRX_IOC_MAGIC, 0x05, unsigned int)
#define AW_IOCTL_HDMIRX_SET_ARCTXPATH _IOW(HDMIRX_IOC_MAGIC, 0x06, unsigned int)
#define AW_IOCTL_HDMIRX_SET_EDIDVERSION _IOW(HDMIRX_IOC_MAGIC, 0x07, unsigned int)
#define AW_IOCTL_HDMIRX_SET_PULLHOTPLUG _IOW(HDMIRX_IOC_MAGIC, 0x08, unsigned int)
#define AW_IOCTL_HDMIRX_GET_HDMINUM _IOR(HDMIRX_IOC_MAGIC, 0x09, unsigned int)
#define AW_IOCTL_HDMIRX_GET_5V _IOR(HDMIRX_IOC_MAGIC, 0x0a, unsigned int)

#endif /* _AW_HDMIRX_DEV_H_ */
