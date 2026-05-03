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
#define EFUSE_HV_SWITCH			(IOMEM_ADDR(SUNXI_RTC_BASE) + 0x204)

/* SID registers */
#define SID_PRCTL               	(IOMEM_ADDR(SUNXI_SID_BASE) + 0x40)
#define SID_OP_LOCK			(0xAC)
#define SID_PRKEY               	(IOMEM_ADDR(SUNXI_SID_BASE) + 0x50)
#define SID_RDKEY               	(IOMEM_ADDR(SUNXI_SID_BASE) + 0x60)
#define SID_ROTPK_VALUE(n)              (IOMEM_ADDR(SUNXI_SID_BASE) + 0x120 + (n * 4))
#define SID_ROTPK_CTRL                  (IOMEM_ADDR(SUNXI_SID_BASE) + 0x140)
#define SID_ROTPK_EFUSED_BIT            (1)     /* Bit offset of 'ROTPK_EFUSED' */
#define SID_ROTPK_CMP_RET_BIT           (0)     /* Bit offset of 'ROTPK_COMP_RESULT' */


/* SID SRAM */
#define SID_EFUSE               	(IOMEM_ADDR(SUNXI_SID_BASE) + 0x200)

/* Efuse Mapping Table */
#define EFUSE_CHIPID            	(0x0)
#define EFUSE_WRITE_PROTECT		(0x40)
#define EFUSE_READ_PROTECT		(0x44)
#define EFUSE_LCJS                      (0x48)
#define EFUSE_ROTPK			(0x70)
#define SCC_ROTPK_BURNED_FLAG		(12)
#define EFUSE_OEM_PROGRAM		(0xE4)
#define SID_OEM_PROGRAM_SIZE		(224)

#endif    /*  #ifndef __SID_H__  */
