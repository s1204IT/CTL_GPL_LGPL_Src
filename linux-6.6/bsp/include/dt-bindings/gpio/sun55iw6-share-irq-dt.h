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

#ifndef _SUN55IW6_SHARE_INTERRUPT_DT_H
#define _SUN55IW6_SHARE_INTERRUPT_DT_H

#define ARM_IRQn(hw_irq_id) ((hw_irq_id) - 32)

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

#define ARM_PA_IRQ_NUM    ARM_IRQn(99)
#define ARM_PB_IRQ_NUM    ARM_IRQn(101)
#define ARM_PC_IRQ_NUM    ARM_IRQn(103)
#define ARM_PD_IRQ_NUM    ARM_IRQn(105)
#define ARM_PE_IRQ_NUM    ARM_IRQn(107)
#define ARM_PF_IRQ_NUM    ARM_IRQn(109)
#define ARM_PG_IRQ_NUM    ARM_IRQn(111)
#define ARM_PH_IRQ_NUM    ARM_IRQn(113)
#define ARM_PI_IRQ_NUM    ARM_IRQn(115)
#define ARM_PJ_IRQ_NUM    ARM_IRQn(117)
#define ARM_PK_IRQ_NUM    ARM_IRQn(172)
#define ARM_PL_IRQ_NUM    ARM_IRQn(234)
#define ARM_PM_IRQ_NUM    ARM_IRQn(236)


#define E907_PA_IRQ_NUM    E907_IRQn(83, 0)
#define E907_PB_IRQ_NUM    E907_IRQn(85, 0)
#define E907_PC_IRQ_NUM    E907_IRQn(87, 0)
#define E907_PD_IRQ_NUM    E907_IRQn(89, 0)
#define E907_PE_IRQ_NUM    E907_IRQn(91, 0)
#define E907_PF_IRQ_NUM    E907_IRQn(93, 0)
#define E907_PG_IRQ_NUM    E907_IRQn(95, 0)
#define E907_PH_IRQ_NUM    E907_IRQn(97, 0)
#define E907_PI_IRQ_NUM    E907_IRQn(99, 0)
#define E907_PJ_IRQ_NUM    E907_IRQn(101, 0)
#define E907_PK_IRQ_NUM    E907_IRQn(156, 0)
#define E907_PL_IRQ_NUM    E907_IRQn(218, 0)
#define E907_PM_IRQ_NUM    E907_IRQn(220, 0)

#endif
