/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc user resource handle
 * operation interface of user resource handle for rproc.
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SUNXI_RPROC_USER_RESOURE_H__
#define __SUNXI_RPROC_USER_RESOURE_H__

struct fw_rsc_user_resource {
	//u32 type; // useless for helper
	u32 da;
	u32 len;
	u32 reserved;
	u32 src_type;
	u8 src_name[32];
} __packed;

extern const struct rsc_ops user_resource_ops;

#endif /* __SUNXI_RPROC_USER_RESOURE_H__ */
