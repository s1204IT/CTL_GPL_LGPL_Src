// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * ouyangkun <ouyangkun@allwinnertech.com>
 */

#include <common.h>
#include <asm/io.h>
#include <sunxi_nsi.h>

#define IAG_MODE(n) (SUNXI_NSI_BASE + 0x0010 + (0x200 * (n)))
#define IAG_PRI_CFG(n) (SUNXI_NSI_BASE + 0x0014 + (0x200 * (n)))
#define IAG_QOS_SEL_CFG(n) (SUNXI_NSI_BASE + 0x0018 + (0x200 * (n)))

void sunxi_nsi_set(int id, enum sunxi_nsi_ig_type_e type, u32 val)
{
	u32 val_tmp;
	switch (type) {
	case SUNXI_NSI_IG_MOD:
		writel(val, IAG_MODE(id));
		break;
	case SUNXI_NSI_IG_PRIORITY:
		val_tmp = val + (val << 2);
		writel(val_tmp, IAG_PRI_CFG(id));
		break;
	case SUNXI_NSI_IG_QOS_SEL:
		writel(val, IAG_MODE(id));
		break;
	}
}
