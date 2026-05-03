/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Share Interrupt Platform Head File
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PLATFORM_SHARE_IRQ_H__
#define __PLATFORM_SHARE_IRQ_H__

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#include "sun55iw3-share-irq.h"
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
#include "sun55iw6-share-irq.h"
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
#include "sun300iw1-share-irq.h"
#endif

#endif /* __PLATFORM_SHARE_IRQ_H__ */
