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
#ifndef _SUNXI_HRC_H_
#define _SUNXI_HRC_H_

#include "sunxi_hrc_log.h"
#include "hardware/sunxi_hrc_hardware.h"

int sunxi_hrc_call_notifiers(unsigned long val, void *data);

#endif  /* _SUNXI_HRC_H_ */
