/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Based on arch/arm64/kernel/chipid-sunxi.c
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/io.h>
#include <linux/kernel.h>
#include <asm/sbi.h>
#include <sunxi-smc.h>

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#define SBI_EXT_SUNXI			0x54535251
#define SBI_EXT_SUNXI_EFUSE_WRITE	0

int sbi_efuse_write(phys_addr_t key_buf)
{
	struct sbiret ret;
	ret = sbi_ecall(SBI_EXT_SUNXI, SBI_EXT_SUNXI_EFUSE_WRITE, (unsigned long)key_buf, 0, 0, 0, 0, 0);
	if (ret.error) {
		pr_err("sbi error is %lu, value is %lu\n", ret.error, ret.value);
		return ret.error;
	}
	return ret.value;
}
EXPORT_SYMBOL_GPL(sbi_efuse_write);

static int __init sunxi_sbi_init(void)
{
	pr_info("sunxi sbi init success\n");
	return 0;
}

static void __exit sunxi_sbi_exit(void)
{
}

module_init(sunxi_sbi_init);
module_exit(sunxi_sbi_exit);
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("shaosidi<shaosidi@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi sbi.");
