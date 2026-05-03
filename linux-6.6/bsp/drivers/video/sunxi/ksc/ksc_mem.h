// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _KSC_MEM_H
#define _KSC_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/version.h>

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	struct device *p_device;
};



int ksc_dma_map(int fd, struct dmabuf_item *item);

void ksc_dma_unmap(struct dmabuf_item *item);

void *ksc_mem_alloc(struct device *p_dev, u32 num_bytes, dma_addr_t *p_phys_addr);

void ksc_mem_free(struct device *p_dev, u32 num_bytes, void *virt_addr, dma_addr_t phys_addr);

int ksc_mem_mmap(struct device *p_dev, void *lut_virt, dma_addr_t lut_phy_addr,
		 unsigned int lut_size, struct vm_area_struct *vma);

int ksc_mem_sync(struct device *p_dev, void *virt, dma_addr_t phy_addr, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
