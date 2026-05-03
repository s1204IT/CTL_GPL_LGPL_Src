/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __HWSPINLOCK_SUN50IW11_H__
#define __HWSPINLOCK_SUN50IW11_H__

/* config for DSP */
#if defined(CONFIG_CORE_DSP0)
#include <hal_prcm.h>

#define SPIN_LOCK_BASE		(0x03004000)

/* for prcm and ccmu compatibility */
#define HAL_CLK_PERIPH_SPINLOCK	CCU_MOD_CLK_SPINLOCK
#endif /* CONFIG_CORE_DSP0 */

#endif /*__HWSPINLOCK_SUN50IW11_H__  */
