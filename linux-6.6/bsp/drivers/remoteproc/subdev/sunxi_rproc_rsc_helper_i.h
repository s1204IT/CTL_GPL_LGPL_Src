/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc rsc helper internal interface
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SUNXI_RPROC_RSC_HELPER_I_H__
#define __SUNXI_RPROC_RSC_HELPER_I_H__

#include <linux/remoteproc.h>

int load_from_file(const char *path, void *dst, size_t size);
int load_from_partition(const char *partition, void *dst, size_t size);
int copy_to_rproc_da(struct rproc *rproc, u64 da, const void *src, size_t size);

#endif /* __SUNXI_RPROC_RSC_HELPER_I_H__ */
