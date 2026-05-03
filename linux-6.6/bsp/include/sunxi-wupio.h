/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * header file for sunxi-wupio driver
 *
 * Copyright (C) 2024 Allwinner.
 *
 * luwinkey <luwinkey@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __SUNXI_WUPIO_H__
#define __SUNXI_WUPIO_H__

int sunxi_wupio_get_index(struct device *dev);
int sunxi_wupio_register_callback(int wupio_index, void *callback);
int sunxi_wupio_is_illegal(struct device *dev, struct gpio_desc *gpio);
int sunxi_share_wupio_enable(struct device *dev, struct gpio_desc *gpio);

#endif  /* __SUNXI_WUPIO_H__ */
