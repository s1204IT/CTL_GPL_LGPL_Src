/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Private header file for sunxi-wupio driver
 *
 * Copyright (C) 2024 Allwinner.
 *
 * luwinkey <luwinkey@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _WUPIO_SUNXI_H_
#define _WUPIO_SUNXI_H_

enum {
	WUPIO_NEGATIVE_EDGE = 0,
	WUPIO_POSITIVE_EDGE = 1
} wupio_edge_mode;

enum {
	WUPIO_HOLD_DISABLE = 0,
	WUPIO_HOLD_ENABLE  = 1
} wupio_hold_status;

enum {
	WUPIO_DETECTION_DISABLE = 0,
	WUPIO_DETECTION_ENABLE  = 1
} wupio_detection_status;

enum {
	WUPIO_INTERRUPT_DISABLE = 0,
	WUPIO_INTERRUPT_ENABLE  = 1
} wupio_interrupt_status;

enum {
	WAKEIO_DEB_CLK_0 =  0,
	WAKEIO_DEB_CLK_1 =  1
} wupio_dbc_clksrc;

struct wakeup_pin {
	struct gpio_desc *gpio;
	const char *edge_mode;
	unsigned char share_io;
	unsigned int clk_src;
	unsigned int debounce_us;
	int ionum;
	char name[32];
};

struct sunxi_wupio_dev {
	struct device *dev;
	void __iomem *gprcm_base;
	int irq;
	struct wakeup_pin *pins;
	int num_pins;
};

#endif /* end of _WUPIO_SUNXI_H_ */
