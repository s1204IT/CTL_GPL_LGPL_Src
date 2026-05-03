// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * ouyangkun <ouyangkun@allwinnertech.com>
 */

enum sunxi_nsi_ig_type_e {
	SUNXI_NSI_IG_MOD,
	SUNXI_NSI_IG_PRIORITY,
	SUNXI_NSI_IG_QOS_SEL,
};

void sunxi_nsi_set(int id, enum sunxi_nsi_ig_type_e type, u32 val);
