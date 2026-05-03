/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Share Interrupt Head File
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <dt-bindings/gpio/sun300iw1-share-irq-dt.h>
#include "sunxi_share_interrupt.h"

struct share_irq_res_table share_irq_table[] = {
	/* major  host coreq hwirq  remore core irq */
	/* table for e907 */
	{1,    A27_PA_IRQ_NUM},
	{3,    A27_PC_IRQ_NUM},
	{4,    A27_PD_IRQ_NUM},
	{12,   A27_PL_IRQ_NUM},
};
