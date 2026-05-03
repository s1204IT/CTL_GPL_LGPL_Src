/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
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

#ifndef _AW_HDMIRX_DEFINE_H_
#define _AW_HDMIRX_DEFINE_H_

#include "sunxi-log.h"
#include <asm/div64.h>
#include <linux/delay.h>
#include "aw_hdmirx_log.h"

/**
 * @desc: allwinner hdmirx log
 * @demo: hdmirx20: xxx
*/

#ifdef hdmirx_inf
#undef hdmirx_inf
#endif
#define hdmirx_inf(fmt, args...)                 \
	do {                                       \
		sunxi_info(NULL, "[ info] "fmt, ##args);        \
		hdmirx_log("hdmirx: [ info] "fmt, ##args); \
	} while (0)

#ifdef hdmirx_wrn
#undef hdmirx_wrn
#endif
#define hdmirx_wrn(fmt, args...)                  \
	do {                                        \
		sunxi_warn(NULL, "[ warn] "fmt, ##args);  \
		hdmirx_log("hdmirx: [ warn] "fmt, ##args);  \
	} while (0)

#ifdef hdmirx_err
#undef hdmirx_err
#endif
#define hdmirx_err(fmt, args...)                  \
	do {                                        \
		sunxi_err(NULL, "[error] "fmt, ##args);  \
		hdmirx_log("hdmirx: [error] "fmt, ##args);  \
	} while (0)

extern u32 gHdmirx_log_level;

#define VIDEO_INF(fmt, args...)                      \
	do {                                             \
		if ((gHdmirx_log_level == 1) || (gHdmirx_log_level == 4) \
				|| (gHdmirx_log_level > 9))                \
			sunxi_info(NULL, "[video] "fmt, ##args);  \
		hdmirx_log("hdmirx: [video] "fmt, ##args);  \
	} while (0)

#define EDID_INF(fmt, args...)                       \
	do {                                             \
		if ((gHdmirx_log_level == 2) || (gHdmirx_log_level == 4) \
				|| (gHdmirx_log_level > 9))                \
			sunxi_info(NULL, "[ edid] "fmt, ##args);   \
		hdmirx_log("hdmirx: [ edid] "fmt, ##args);   \
	} while (0)

#define AUDIO_INF(fmt, args...)                      \
	do {                                             \
		if ((gHdmirx_log_level == 3) || (gHdmirx_log_level == 4) \
				|| (gHdmirx_log_level > 9))                \
			sunxi_info(NULL, "[audio] "fmt, ##args);  \
		hdmirx_log("hdmirx: [audio] "fmt, ##args);  \
	} while (0)

#define CEC_INF(fmt, args...)                         \
	do {                                              \
		if ((gHdmirx_log_level > 9) || (gHdmirx_log_level == 5))  \
			sunxi_info(NULL, "[  cec] "fmt, ##args);     \
		hdmirx_log("hdmirx: [  cec] "fmt, ##args);     \
	} while (0)

#define HDCP_INF(fmt, args...)                      \
	do {                                            \
		if ((gHdmirx_log_level > 9) || (gHdmirx_log_level == 6))  \
			sunxi_info(NULL, "[ hdcp] "fmt, ##args);  \
		hdmirx_log("hdmirx: [ hdcp] "fmt, ##args);  \
	} while (0)

#define EVENT_INF(fmt, args...)                      \
	do {											\
		if ((gHdmirx_log_level > 9) || (gHdmirx_log_level == 7))  \
			sunxi_info(NULL, "[event] "fmt, ##args);  \
		hdmirx_log("hdmirx: [event] "fmt, ##args);	\
	} while (0)

#define PACK_INF(fmt, args...)                      \
	do {											\
		if ((gHdmirx_log_level > 9) || (gHdmirx_log_level == 8))  \
			sunxi_info(NULL, "[ pack] "fmt, ##args);  \
		hdmirx_log("hdmirx: [ pack] "fmt, ##args);	\
	} while (0)

#define PORT_INF(fmt, args...)                      \
	do {											\
		if ((gHdmirx_log_level > 9) || (gHdmirx_log_level == 9))  \
			sunxi_info(NULL, "[ port] "fmt, ##args);  \
		hdmirx_log("hdmirx: [ port] "fmt, ##args);	\
	} while (0)

#define LOG_TRACE()                                    \
	do {                                               \
		if (gHdmirx_log_level > 10)                           \
			sunxi_info(NULL, "[trace] %s\n", __func__); \
		hdmirx_log("hdmirx: [trace] %s\n", __func__); \
	} while (0)

#define LOG_TRACE1(a)                              \
	do {                                           \
		if (gHdmirx_log_level > 10)                       \
			sunxi_info(NULL, "[trace] %s: %d\n",    \
				__func__, a);                      \
		hdmirx_log("hdmirx: [trace] %s: %d\n",    \
				__func__, a);                      \
	} while (0)

#define LOG_TRACE2(a, b)                             \
	do {                                             \
		if (gHdmirx_log_level > 10)                         \
			sunxi_info(NULL, "[trace] %s: %d %d\n",   \
				__func__, a, b);                     \
		hdmirx_log("hdmirx: [trace] %s: %d %d\n",   \
				__func__, a, b);                     \
	} while (0)

#define LOG_TRACE3(a, b, c)                            \
	do {                                               \
		if (gHdmirx_log_level > 10)                           \
			sunxi_info(NULL, "[trace] %s: %d %d %d\n",  \
				__func__, a, b, c);                    \
		hdmirx_log("hdmirx: [trace] %s: %d %d %d\n",  \
				__func__, a, b, c);                    \
	} while (0)

#endif /* _AW_CONFIG_H_ */
