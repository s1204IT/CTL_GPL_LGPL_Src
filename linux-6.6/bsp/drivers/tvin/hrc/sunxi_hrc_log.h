/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs HRC Driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _SUNXI_HRC_LOG_H_
#define _SUNXI_HRC_LOG_H_

#include <sunxi-log.h>

extern u32 loglevel;

#ifdef hrc_inf
#undef hrc_inf
#endif  /* hrc_inf */
#define hrc_inf(fmt, args...) sunxi_info(NULL, fmt, ##args)

#ifdef hrc_wrn
#undef hrc_wrn
#endif  /* hrc_wrn */
#define hrc_wrn(fmt, args...) sunxi_warn(NULL, fmt, ##args)

#ifdef hrc_err
#undef hrc_err
#endif  /* hrc_err */
#define hrc_err(fmt, args...) sunxi_err(NULL, "[%s:%d] " fmt, __func__, __LINE__, ##args)

#ifdef hrc_dbg
#undef hrc_dbg
#endif  /* hrc_dbg */
#define hrc_dbg(fmt, args...) \
	do { \
		if (loglevel > 7) \
			sunxi_info(NULL, "[%s:%d] " fmt, __func__, __LINE__, ##args); \
		else \
			sunxi_debug(NULL, "[%s:%d] " fmt, __func__, __LINE__, ##args); \
	} while (0)

#endif  /* _SUNXI_HRC_LOG_H_ */
