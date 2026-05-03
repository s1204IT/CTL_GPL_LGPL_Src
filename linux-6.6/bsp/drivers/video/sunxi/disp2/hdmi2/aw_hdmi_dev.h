/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _AW_HDMI_DEV_H_
#define _AW_HDMI_DEV_H_

typedef struct {
	/* hdmi devices */
	dev_t          hdmi_devid;
	struct cdev   *hdmi_cdev;
	struct class  *hdmi_class;
	struct device *hdmi_device;
	/* cec devices */
	dev_t          cec_devid;
	struct cdev   *cec_cdev;
	struct class  *cec_class;
	struct device *cec_device;
} aw_device_t;

#define AW_IOCTL_HDMI_HDCP22_LOAD_FW         _IOW('h', 1, unsigned int)
#define AW_IOCTL_HDMI_HDCP_ENABLE            _IOW('h', 2, unsigned int)
#define AW_IOCTL_HDMI_HDCP_DISABLE           _IOW('h', 3, unsigned int)
#define AW_IOCTL_HDMI_HDCP_INFO              _IOR('h', 4, unsigned int)
#define AW_IOCTL_HDMI_GET_LOG_SIZE           _IOR('h', 5, unsigned int)
#define AW_IOCTL_HDMI_GET_LOG                _IOR('h', 6, unsigned int)

enum aw_hdcp_debug_t {
	AW_HDCP_ENABLE_NORMAL   = 0,
	AW_HDCP_ENABLE_FORCE_14 = 1,
	AW_HDCP_ENABLE_FORCE_22 = 2,
};

struct debug_msg_t {
	const char *msg;
	int code;
};

#define __DEF_CODE(_code) \
{                       \
	.code  =  _code,      \
	.msg = #_code,      \
}

enum reg_type_e {
	HDMI2_NONE_REG = 0,
	HDMI2_BASIC_REG,
	HDMI2_PHY_REG,
	HDMI2_SCDC_REG,
	HDMI2_HPI_REG,
};

enum pattern_type_e {
	HDMI2_NONE_PATTERN = 0,
	HDMI2_RED_PATTERN,
	HDMI2_GREEN_PATTERN,
	HDMI2_BLUE_PATTERN,
};

struct debug_msg_t reg_bank_msg[] = {
	__DEF_CODE(HDMI2_NONE_REG),
	__DEF_CODE(HDMI2_BASIC_REG),
	__DEF_CODE(HDMI2_PHY_REG),
	__DEF_CODE(HDMI2_SCDC_REG),
	__DEF_CODE(HDMI2_HPI_REG),
};

struct debug_msg_t pattern_msg[] = {
	__DEF_CODE(HDMI2_NONE_PATTERN),
	__DEF_CODE(HDMI2_RED_PATTERN),
	__DEF_CODE(HDMI2_GREEN_PATTERN),
	__DEF_CODE(HDMI2_BLUE_PATTERN),
};

#endif /* _AW_HDMI_DEV_H_ */
