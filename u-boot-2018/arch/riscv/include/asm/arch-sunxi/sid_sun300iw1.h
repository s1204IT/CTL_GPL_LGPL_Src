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

#define SID_PRCTL               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x40)
#define SID_PRKEY               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x50)
#define SID_RDKEY               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x60)

#define SID_ROTPK_VALUE(n)      (IOMEM_ADDR(SUNXI_SID_BASE) + 0x120 + (n * 4))
#define SID_ROTPK_CTRL          (IOMEM_ADDR(SUNXI_SID_BASE) + 0x140)
#define SID_ROTPK_CMP_RET_BIT   (0)
#define SID_ROTPK_EFUSED_BIT    (1)
#define SUNXI_EFUSE_RAM_BASE_IN_DDR	(0x80FFFF00)
#define SID_EFUSE               ((volatile void *)SUNXI_EFUSE_RAM_BASE_IN_DDR)
#define SID_OP_LOCK  (0xAC)

/*efuse power ctl*/
#define EFUSE_HV_SWITCH         (IOMEM_ADDR(SUNXI_PMU_AON_BASE) + 0x84)

#define EFUSE_CHIPID            (0x0)
#define EFUSE_BROM              (0x4c)
#define EFUSE_LCJS		EFUSE_BROM
#define EFUSE_ANTI_BRUSH	EFUSE_BROM
#define EFUSE_ROTPK             (0x74)
#define ANTI_BRUSH_MODE		(SID_EFUSE + EFUSE_ANTI_BRUSH)
#define ANTI_BRUSH_BIT_OFFSET	(0)

#define EFUSE_WMAC                (0x3F)
#define EFUSE_WMAC_SEL_FLAG_MASK  (0x3)
#define EFUSE_WMAC_ADDR1          (0x40)
#define EFUSE_WMAC_ADDR2          (0x70)
#define SID_WMAC_SIZE             (0x30)

/* secure enbale bit */
#define SECURE_BIT_VAL 		(3)

/* write protect */
#define EFUSE_WRITE_PROTECT	(0x50)
#define EFUSE_READ_PROTECT	EFUSE_WRITE_PROTECT

/*write protect*/
#define SCC_ROTPK_BURNED_FLAG	(26)

/* The read/write protection bit occupies 32 bits */
#define LONG_EFUSE_RW_PROTECT_OFFSET 1
#define EFUSE_ACL_SET_BRUN_BIT (0x100000000)
#define EFUSE_ACL_SET_RD_FORBID_BIT (0x100000000)
#define EFUSE_BRUN_RD_OFFSET_MASK (0xFFFFFFFF)

#endif    /*  #ifndef __SID_H__  */
