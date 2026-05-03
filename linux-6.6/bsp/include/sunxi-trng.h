/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * allwinner sunxi soc chip version and chip id manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _SUNXI_TRNG_H_
#define _SUNXI_TRNG_H_

#include <linux/hw_random.h>

struct sunxi_trng {
	struct platform_device	*pdev;
	struct device		*dev;
	struct resource		*res;
	struct clk		*clk;
	struct reset_control	*reset;
	struct hwrng		*hwrng;
};

int sunxi_trng_exstract_random(u8 *trng_buf, u32 trng_len);

#endif /* end of _SUNXI_TRNG_H_ */
