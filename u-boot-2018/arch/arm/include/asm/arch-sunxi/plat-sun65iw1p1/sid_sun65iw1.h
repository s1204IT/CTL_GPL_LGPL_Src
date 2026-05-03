/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * wangwei <wangwei@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __SID_H__
#define __SID_H__

#include <linux/types.h>
#include <asm/arch/cpu.h>

/* Efuse power control */
#define EFUSE_HV_SWITCH			(SUNXI_RTC_BASE + 0x204)

/* SID registers */
#define SID_PRCTL			(SUNXI_SID_BASE + 0x40)
#define SID_OP_LOCK			(0xAC)  /* value of bits[15:8] in SID_PRCTL */
#define SID_PRKEY			(SUNXI_SID_BASE + 0x50)
#define SID_RDKEY			(SUNXI_SID_BASE + 0x60)
#define SJTAG_S				(SUNXI_SID_BASE + 0x88)   /* SJTAG_SELECT_REG */
#define SID_SECURE_MODE			(SUNXI_SID_BASE + 0xA0)
#define SID_ROTPK_VALUE(n)		(SUNXI_SID_BASE + 0x120 + (n * 4))
#define SID_ROTPK_CTRL			(SUNXI_SID_BASE + 0x140)
#define SID_ROTPK_EFUSED_BIT		(1)     /* Bit offset of 'ROTPK_EFUSED' */
#define SID_ROTPK_CMP_RET_BIT		(0)     /* Bit offset of 'ROTPK_COMP_RESULT' */

/* SID SRAM */
#define SID_EFUSE			(SUNXI_SID_BASE + 0x200)  /* SID_SRAM Base */
#define SID_SRAM_SIZE_BYTES		(2048 / 8)

/* Efuse Mapping Table */
#define EFUSE_CHIPID			(0x0)   /* CHIPID */
#define EFUSE_OEM_PROGRAM		(0x4c)  /* OEM PROG0 */
#define SID_OEM_PROGRAM_SIZE		(32)    /* Bits */
#define EFUSE_WRITE_PROTECT		(0x50)  /* WR PROT */
#define EFUSE_READ_PROTECT		(0x54)  /* RD PROT */
#define EFUSE_LCJS			(0x58)  /* SEC_CTRL_BIT. For JTAG security */
#define EFUSE_ROTPK			(0x80)  /* ROTPK */
#define SCC_ROTPK_BURNED_FLAG		(14)    /* 'rng*' index of ROTPK */

#endif  /* #ifndef __SID_H__ */
