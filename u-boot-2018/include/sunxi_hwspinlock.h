/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUNXI_HW_SPIN_LOCK__
#define __SUNXI_HW_SPIN_LOCK__

#define SPINLOCK_NUM			(32)	/* max lock num */

enum hwspinlock_err {
	HWSPINLOCK_OK = 0,
	HWSPINLOCK_ERR = -1,
	HWSPINLOCK_EXCEED_MAX = -2,
	HWSPINLOCK_PM_ERR = -3,
	HWSPINLOCK_TIMEOUT = -4,
};

void hwspinlock_init(void);
int hwspinlock_put(int num);
int hwspinlock_get(int num);
int hwspinlock_check_taken(int num);
int hwspin_lock(int num);
void hwspin_unlock(int num);

#endif