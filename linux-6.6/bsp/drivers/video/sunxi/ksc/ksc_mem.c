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

#include "ksc_mem.h"

static LIST_HEAD(ksc_mem_list);


struct ksc_dma_info {
	void *vaddr;
	size_t size;
	struct dmabuf_item *p_item;

};

struct ksc_mem_list_node {
	struct list_head node;
	struct ksc_dma_info mem;
};

int ksc_dma_map(int fd, struct dmabuf_item *item)
{
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	if (fd < 0) {
		dev_err(item->p_device, "dma_buf_id %d is invalid\n", fd);
		goto exit;
	}
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		dev_err(item->p_device, "dma_buf_get failed, fd=%d\n", fd);
		goto exit;
	}

	attachment = dma_buf_attach(dmabuf, item->p_device);
	if (IS_ERR(attachment)) {
		dev_err(item->p_device, "dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(sgt)) {
		dev_err(item->p_device, "dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	item->fd = fd;
	item->buf = dmabuf;
	item->sgt = sgt;
	item->attachment = attachment;
	item->dma_addr = sg_dma_address(sgt->sgl);
	ret = 0;
	goto exit;

err_buf_detach:
	dma_buf_detach(dmabuf, attachment);
err_buf_put:
	dma_buf_put(dmabuf);
exit:
	return ret;
}

void ksc_dma_unmap(struct dmabuf_item *item)
{
	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(item->buf, item->attachment);
	dma_buf_put(item->buf);
	item->buf = NULL;
	item->fd = -1;
}

#ifdef CONFIG_AW_KSC_ENABLE_DMA_HEAP_MEM
static int __disp_dma_heap_alloc_coherent(struct ksc_dma_info *mem, struct device *p_dev)
{
	struct dma_buf *dmabuf;
	struct dma_heap *dmaheap;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct dma_buf_map map;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map;
#endif

#if IS_ENABLED(CONFIG_AW_IOMMU) || IS_ENABLED(CONFIG_ARM_SMMU_V3)
	dmaheap = dma_heap_find("system");
#elif IS_ENABLED(CONFIG_AW_DMABUF_HEAPS_CARVEOUT)
	dmaheap = dma_heap_find("carveout");
#else
	dmaheap = dma_heap_find("reserved");
#endif

	if (IS_ERR_OR_NULL(dmaheap)) {
		pr_err("failed, size=%u dmaheap=0x%p\n", (unsigned int)mem->size, dmaheap);
		return -2;
	}

	dmabuf = dma_heap_buffer_alloc(dmaheap, mem->size, O_RDWR, 0);

	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("failed, size=%u dmabuf=0x%p\n", (unsigned int)mem->size, dmabuf);
		return -2;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(dmabuf, &map);
	mem->vaddr = map.vaddr;
	if (ret) {
		pr_err("dma_buf_vmap failed!\n");
		goto err_map_kernel;
	}
#else
	mem->vaddr = dma_buf_vmap(dmabuf);
#endif

	if (IS_ERR_OR_NULL(mem->vaddr)) {
		pr_err("ion_map_kernel failed!\n");
		goto err_map_kernel;
	}

	mem->p_item = kmalloc(sizeof(struct dmabuf_item), GFP_KERNEL);

	attachment = dma_buf_attach(dmabuf, p_dev);
	if (IS_ERR(attachment)) {
		pr_err("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		/* FIXME, wait iommu ready */
		return -1;
		/* goto err_buf_detach; */
	}

	mem->p_item->buf = dmabuf;
	mem->p_item->sgt = sgt;
	mem->p_item->attachment = attachment;
	mem->p_item->dma_addr = sg_dma_address(sgt->sgl);

	return 0;
	/* unmap attachment sgt, not sgt_bak, cause it's not alloc yet! */
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
/* err_buf_detach: */
	dma_buf_detach(dmabuf, attachment);

err_buf_put:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	map.vaddr = mem->vaddr;
	dma_buf_vunmap(mem->p_item->buf, &map);
#else
	dma_buf_vunmap(mem->p_item->buf, mem->vaddr);
#endif
err_map_kernel:
	dma_heap_buffer_free(mem->p_item->buf);
	return -ENOMEM;
}
#endif


void *ksc_mem_alloc(struct device *p_dev, u32 num_bytes, dma_addr_t *p_phys_addr)
{
#ifndef CONFIG_AW_KSC_ENABLE_DMA_HEAP_MEM
	return  dma_alloc_coherent(p_dev, num_bytes, p_phys_addr, GFP_KERNEL);
#else
	struct ksc_mem_list_node *ion_node = NULL;
	struct ksc_dma_info *mem = NULL;
	u32 *paddr = NULL;
	int ret = -1;


	ion_node = kmalloc(sizeof(struct ksc_mem_list_node), GFP_KERNEL);
	if (ion_node == NULL) {
		dev_err(p_dev, "fail to alloc ion node, size=%u\n",
		      (unsigned int)sizeof(struct ksc_mem_list_node));
		return NULL;
	}
	mem = &ion_node->mem;
	mem->size = num_bytes;

	ret = __disp_dma_heap_alloc_coherent(mem, p_dev);

	if (ret != 0) {
		dev_err(p_dev, "fail to alloc ion, ret=%d\n", ret);
		goto err_hdl;
	}

	paddr = (u32 *)p_phys_addr;
	*paddr = (u32)mem->p_item->dma_addr;
	list_add_tail(&(ion_node->node), &ksc_mem_list);

	return mem->vaddr;

err_hdl:
	kfree(ion_node);
	return NULL;
#endif
}


#ifdef CONFIG_AW_KSC_ENABLE_DMA_HEAP_MEM
static void __disp_ion_free_coherent(struct ksc_dma_info *mem)
{
	struct dmabuf_item item;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct dma_buf_map map = DMA_BUF_MAP_INIT_VADDR(mem->vaddr);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(mem->vaddr);
#endif
	memcpy(&item, mem->p_item, sizeof(struct dmabuf_item));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	dma_buf_vunmap(item.buf, &map);
#else
	dma_buf_vunmap(item.buf, mem->vaddr);
#endif
	ksc_dma_unmap(mem->p_item);
	kfree(mem->p_item);
	return;
}
#endif


void ksc_mem_free(struct device *p_dev, u32 num_bytes, void *virt_addr, dma_addr_t phys_addr)
{
#ifndef CONFIG_AW_KSC_ENABLE_DMA_HEAP_MEM
	return  dma_free_coherent(p_dev, num_bytes, virt_addr, phys_addr);
#else
	struct ksc_mem_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct ksc_dma_info *mem = NULL;
	bool found = false;

	list_for_each_entry_safe(ion_node, tmp_ion_node, &ksc_mem_list,
				 node) {
		if (ion_node != NULL) {
			mem = &ion_node->mem;
			if ((((unsigned long)mem->p_item->dma_addr) ==
			     ((unsigned long)phys_addr)) &&
			    (((unsigned long)mem->vaddr) ==
			     ((unsigned long)virt_addr))) {
				__disp_ion_free_coherent(mem);
				__list_del_entry(&(ion_node->node));
				kfree(ion_node);
				found = true;
				break;
			}
		}
	}
	if (false == found) {
		dev_err(p_dev, "vaddr=0x%p, paddr=0x%llx is not found in ion\n", virt_addr,
			phys_addr);
	}
#endif

}

int ksc_mem_mmap(struct device *p_dev, void *lut_virt, dma_addr_t lut_phy_addr,
		 unsigned int lut_size, struct vm_area_struct *vma)
{
#ifndef CONFIG_AW_KSC_ENABLE_DMA_HEAP_MEM
	return dma_mmap_wc(p_dev, vma, lut_virt,
			   (dma_addr_t)lut_phy_addr,
			   (size_t)lut_size);

#else
	struct ksc_mem_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct ksc_dma_info *mem = NULL;

	list_for_each_entry_safe(ion_node, tmp_ion_node, &ksc_mem_list,
				 node) {
		if (ion_node != NULL) {
			mem = &ion_node->mem;
			if ((((unsigned long)mem->p_item->dma_addr) ==
			     ((unsigned long)lut_phy_addr)) &&
			    (((unsigned long)mem->vaddr) ==
			     ((unsigned long)lut_virt))) {
				return mem->p_item->buf->ops->mmap(mem->p_item->buf, vma);
				break;
			}
		}
	}

	dev_err(p_dev, "mmap fail! vaddr=0x%p, paddr=0x%llx is not found in mem node\n", lut_virt,
		lut_phy_addr);

	return -1;
#endif
}


int ksc_mem_sync(struct device *p_dev, void *virt, dma_addr_t phy_addr, unsigned int size)
{
#ifndef CONFIG_AW_KSC_ENABLE_DMA_HEAP_MEM
	dma_sync_single_range_for_device(p_dev, phy_addr, 0, size, DMA_TO_DEVICE);
	return 0;
#else
	struct ksc_mem_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct ksc_dma_info *mem = NULL;

	list_for_each_entry_safe(ion_node, tmp_ion_node, &ksc_mem_list,
				 node) {
		if (ion_node != NULL) {
			mem = &ion_node->mem;
			if ((((unsigned long)mem->p_item->dma_addr) ==
			     ((unsigned long)phy_addr)) &&
			    (((unsigned long)mem->vaddr) ==
			     ((unsigned long)virt))) {
				return dma_buf_end_cpu_access(mem->p_item->buf, DMA_TO_DEVICE);
				break;
			}
		}
	}
#endif
	return 0;

}


//End of File
