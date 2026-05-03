/* SPDX-License-Identifier: GPL-2.0+ */

#include <asm/io.h>
#include <common.h>
#include <sys_config.h>
#include <sunxi_hwspinlock.h>
#include "hwspinlock.h"

/* Possible values of SPINLOCK_LOCK_REG */
#define SPINLOCK_NOTTAKEN               (0)     /* free */
#define SPINLOCK_TAKEN                  (1)     /* locked */

void hwspinlock_init(void)
{
	u32 reg_val;
	/* clock gating */
	reg_val = readl(SUNXI_CCM_BASE + SPIN_LOCK_BGR_REG);
	reg_val |= (1 << BIT_SPIN_LOCK_GATING);
	writel(reg_val, SUNXI_CCM_BASE + SPIN_LOCK_BGR_REG);

	/* reset */
	reg_val = readl(SUNXI_CCM_BASE + SPIN_LOCK_BGR_REG);
	reg_val |= (1 << BIT_SPIN_LOCK_RST);
	writel(reg_val, SUNXI_CCM_BASE + SPIN_LOCK_BGR_REG);
}

int hwspinlock_check_taken(int num)
{
	return !!(readl(SPINLOCK_STATUS_REG) & (1 << num));
}

int hwspinlock_get(int num)
{
	unsigned long addr = SPINLOCK_LOCK_REG(num);
	int status;

	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_EXCEED_MAX;

	status = readl(addr);

	if (status == SPINLOCK_NOTTAKEN)
		return HWSPINLOCK_OK;

	return HWSPINLOCK_ERR;
}

int hwspinlock_put(int num)
{
	unsigned long addr = SPINLOCK_LOCK_REG(num);

	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_EXCEED_MAX;

	writel(SPINLOCK_NOTTAKEN, addr);

	return HWSPINLOCK_OK;
}

int hwspin_lock(int num)
{
	if (num > SPINLOCK_NUM)
		return HWSPINLOCK_ERR;

	while (1) {
		if (hwspinlock_get(num) == HWSPINLOCK_OK)
			break;
	}

	return HWSPINLOCK_OK;
}

void hwspin_unlock(int num)
{
	hwspinlock_put(num);
}
