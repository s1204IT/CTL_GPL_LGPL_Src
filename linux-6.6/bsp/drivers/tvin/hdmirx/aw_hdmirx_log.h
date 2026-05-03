/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _AW_HDMIRX_LOG_H_
#define _AW_HDMIRX_LOG_H_

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_LOG_BUFFER)
#include <linux/of.h>

char *aw_hdmirx_log_get_address(void);
void aw_hdmirx_log_put_address(void);
unsigned int aw_hdmirx_log_get_start_index(void);
unsigned int aw_hdmirx_log_get_used_size(void);
unsigned int aw_hdmirx_log_get_max_size(void);
void aw_hdmirx_log_set_enable(bool enable);
bool aw_hdmirx_log_get_enable(void);

void hdmirx_log(const char *fmt, ...);
int aw_hdmirx_log_init(struct device_node *of_node);
void aw_hdmirx_log_exit(void);
#else
static inline void hdmirx_log(const char *fmt, ...)
{
}
#endif

#endif /* _AW_HDMI_LOG_H_ */
