/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Share Interrupt Platform Head File
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SUN300IW1_SHARE_INTERRUPT_DT_H
#define _SUN300IW1_SHARE_INTERRUPT_DT_H

#define A27_IRQn(hw_irq_id) (hw_irq_id)
#define E907_IRQn(major, sub) ((major) * 100 + (sub))

/* sunxi share-interrupt format
 * major type remote_core_irq flags
 *
 * Generic Type Define
 * type = 0x0 --- normal irq
 *
 * GPIO Type Define
 * type = 0x1 --- GPIOA
 * type = 0x2 --- GPIOB
 * type = 0x3 --- GPIOC
 * type = 0x4 --- GPIOD
 * type = 0x5 --- GPIOE
 * type = 0x6 --- GPIOF
 * type = 0x7 --- GPIOG
 * type = 0x8 --- GPIOH
 * type = 0x9 --- GPIOI
 * type = 0xa --- GPIOJ
 * type = 0xb --- GPIOK
 * type = 0xc --- GPIOL
 * type = 0xd --- GPIOM
 * type = 0xe --- GPION
 */

#define A27_PA_IRQ_NUM    A27_IRQn(68)
#define A27_PC_IRQ_NUM    A27_IRQn(72)
#define A27_PD_IRQ_NUM    A27_IRQn(74)
#define A27_PL_IRQ_NUM    A27_IRQn(84)

#define E907_PA_IRQ_NUM    E907_IRQn(83, 0)
#define E907_PC_IRQ_NUM    E907_IRQn(87, 0)
#define E907_PD_IRQ_NUM    E907_IRQn(89, 0)
#define E907_PL_IRQ_NUM    E907_IRQn(99, 0)

#endif
