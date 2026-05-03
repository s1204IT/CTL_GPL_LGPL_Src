// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <fdt_support.h>
#include <sunxi_hwspinlock.h>

int do_hwspinlock_init(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 1)
		return -1;

	hwspinlock_init();
	return  0;
}

#ifdef CONFIG_SYS_LONGHELP
static char hwspinlock_init_help_text[] =
	"\n    - spinlock init\n";
#endif

U_BOOT_CMD(
	hwspinlock_init,	CONFIG_SYS_MAXARGS,	1,	do_hwspinlock_init,
	"cmd of spinlock init", hwspinlock_init_help_text
);

int do_hwspinlock_put(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	__attribute__((unused)) u32 spinlock_num = SPINLOCK_NUM;
	int ret;

	if (argc != 2)
		return -1;

	spinlock_num = simple_strtoul(argv[1], NULL, 16);
	if (spinlock_num >= SPINLOCK_NUM) {
		pr_err("spinlock num %d, value err, max value %d\n", spinlock_num, SPINLOCK_NUM);
		return -1;
	}

	ret = hwspinlock_put(spinlock_num);
	if (ret)
		pr_err("hwspinlock put err, spinlock num %d\n", spinlock_num);
	else
		pr_info("hwspinlock put success, spinlock num %d\n", spinlock_num);

	return  0;
}

#ifdef CONFIG_SYS_LONGHELP
static char hwspinlock_put_help_text[] =
	"[hwspinlock_put [arg ...]]\n    - hwspinlock_put\n"
	"\tpassing arguments 'arg ...';\n"
	"\t'arg[1]' spinlock num\n";
#endif

U_BOOT_CMD(
	hwspinlock_put,	CONFIG_SYS_MAXARGS,	1,	do_hwspinlock_put,
	"cmd of spinlock put", hwspinlock_put_help_text
);

#ifdef CONFIG_SYS_LONGHELP
static char hwspinlock_get_help_text[] =
	"[hwspinlock_get [arg ...]]\n    - hwspinlock_get\n"
	"\tpassing arguments 'arg ...';\n"
	"\t'arg[1]' spinlock num\n";
#endif

int do_hwspinlock_get(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	__attribute__((unused)) u32 spinlock_num = SPINLOCK_NUM;
	int ret;

	if (argc != 2)
		return -1;

	spinlock_num = simple_strtoul(argv[1], NULL, 16);
	if (spinlock_num >= SPINLOCK_NUM) {
		pr_err("spinlock num %d, value err, max value %d\n", spinlock_num, SPINLOCK_NUM);
		return -1;
	}

	ret = hwspinlock_get(spinlock_num);
	if (ret)
		pr_err("hwspinlock get err, spinlock num %d\n", spinlock_num);
	else
		pr_info("hwspinlock get success, spinlock num %d\n", spinlock_num);

	return  0;
}

U_BOOT_CMD(
	hwspinlock_get,	CONFIG_SYS_MAXARGS,	1,	do_hwspinlock_get,
	"cmd of spinlock get", hwspinlock_get_help_text
);
