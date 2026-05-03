/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Private header file for sunxi-rtc driver
 *
 * Copyright (C) 2018 Allwinner.
 *
 * Martin <wuyan@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __SUNXI_RTC_H__
#define __SUNXI_RTC_H__

int sunxi_rtc_get_gpr_val(u8 index, u32 *val);
int sunxi_rtc_set_gpr_val(u8 index, u32 val);

#endif  /* __SUNXI_RTC_H__ */
