/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * drivers/riscv/sun21iw1/riscv_reg.h
 *
 * Copyright (c) 2007-2025 Allwinnertech Co., Ltd.
 * Author: wujiayi <wujiayi@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 */

#ifndef __RISCV_REG_H
#define __RISCV_REG_H

/*
 * RISCV CFG BASE
 */
#define RISCV_CFG_BASE		(0x06010000)
#define RISCV_VER_REG		(0x0000)
#define RISCV_STA_ADD0_REG	(0x0004)
#define RISCV_STA_ADD1_REG	(0x0008)
#define RF1P_CFG_REG		(0x0010)
#define RISCV_STA_ADD0_REG	(0x0004)
#define TS_TMODE_SEL_REG	(0x0040)
/*
 * CCMU related
 */
#define CCMU_RISCV_CLK_REG		(0x0d00)
#define RISCV_CLK_MASK			(0x7 << 24)
#define RISCV_CLK_HOSC			(0x0 << 24)
#define RISCV_CLK_32K			(0x1 << 24)
#define RISCV_CLK_16M			(0x2 << 24)
#define RISCV_CLK_PERI_800M		(0x3 << 24)
#define RISCV_CLK_PERI_1X		(0x4 << 24)
#define RISCV_CLK_CPUPLL		(0x5 << 24)
#define RISCV_CLK_AUDIO1PLL_DIV2	(0x6 << 24)
/* x must be 1 - 4 */
#define RISCV_AXI_FACTOR_N(x)		(((x) - 1) << 0)
/* x must be 1 - 32 */
#define RISCV_CLK_FACTOR_M(x)		(((x) - 1) << 0)
#define RISCV_CLK_M_MASK		(0x1f << 0)

#define RISCV_GATING_RST_REG		(0x0d04)
#define RISCV_GATING_RST_FIELD		(0x16aa << 0)
#define RISCV_CLK_GATING		(0x1 << 31)

#define RISCV_CFG_BGR_REG		(0x0d0c)
#define RISCV_CFG_RST			(0x1 << 16)
#define RISCV_CFG_GATING		(0x1 << 0)

#define RISCV_RST_REG			(0x0f20)
#define RISCV_RST_FIELD			(0x16aa << 16)
#define RISCV_SOFT_RSTN			(0x1 << 0)

#define RISCV_STA_ADD0_REG		(0x0004)

#endif /* __RISCV_I_H */
