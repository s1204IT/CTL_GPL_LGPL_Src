// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * DMABUF System heap exporter
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2019 Linaro Ltd.
 */
//#define DEBUG
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>
#include <linux/genalloc.h>
#include <asm/page.h>
#include <sunxi-smc.h>
#include <sunxi-log.h>
#include <uapi/security/sunxi-drm-heap.h>

/*
 * google gonna use these source in GKI 2.0 (suppose
 * to lunch in 2021), so these source may encounter
 * namespace conflict in less than one year, just
 * include them with all function static, so no conflict
 * with other sources
 * 							--ouyangkun 2020.09.12
 */
#include "third-party/heap.c"
#include "third-party/heap-helpers.c"

#define DRM_HEAP_DEV_NAME	"sunxi_drm_heap"

MODULE_IMPORT_NS(DMA_BUF);

struct sunxi_drm_info sunxi_drm_info;
EXPORT_SYMBOL_GPL(sunxi_drm_info);

static struct gen_pool *drm_pool;

static void sunxi_drm_heap_free(struct heap_helper_buffer *buffer)
{
	sunxi_debug(NULL, "len = 0x%lx, xaddr = 0x%llx\n",
		    buffer->size, (unsigned long long)buffer->xaddr);
	gen_pool_free(drm_pool, (unsigned long)buffer->xaddr, buffer->size);
	kfree(buffer->pages);
	kfree(buffer);
}

static int sunxi_drm_heap_allocate(struct dma_heap *heap, unsigned long len,
				   unsigned long fd_flags,
				   unsigned long heap_flags)
{
	struct heap_helper_buffer *buffer;
	struct dma_buf *dmabuf;
	int ret = -ENOMEM;
	pgoff_t pg;
	unsigned long xaddr;
	struct page *allocated_page;

	sunxi_debug(NULL, "len = 0x%lx\n", len);

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer) {
		sunxi_err(NULL, "kzalloc() failed\n");
		return -ENOMEM;
	}

	init_heap_helper_buffer(buffer, sunxi_drm_heap_free);
	buffer->heap = heap;
	buffer->size = len;

	buffer->pagecount = len / PAGE_SIZE;

	/* @xaddr is the virt start addr on TA's view  */
	xaddr = gen_pool_alloc(drm_pool, buffer->pagecount * PAGE_SIZE);
	if (!xaddr) {
		sunxi_err(NULL, "gen_pool_alloc() failed\n");
		ret = -ENOMEM;
		goto err0;
	}
	buffer->xaddr = (void *)xaddr;
	sunxi_debug(NULL, "allocated buffer at 0x%llx (phy 0x%llx) with size 0x%lx\n",
	       (unsigned long long)buffer->xaddr,
	       gen_pool_virt_to_phys(drm_pool, xaddr), buffer->size);

	buffer->pages = kmalloc_array(buffer->pagecount,
			       sizeof(*buffer->pages), GFP_KERNEL);
	if (!buffer->pages) {
		sunxi_err(NULL, "kmalloc_array() failed: pagecount = 0x%lx\n", buffer->pagecount);
		ret = -ENOMEM;
		goto err0;
	}

	allocated_page = phys_to_page(gen_pool_virt_to_phys(drm_pool, xaddr));
	for (pg = 0; pg < buffer->pagecount; pg++)
		buffer->pages[pg] = &allocated_page[pg];

	/* create the dmabuf */
	dmabuf = heap_helper_export_dmabuf(buffer, fd_flags);
	if (IS_ERR(dmabuf)) {
		sunxi_err(NULL, "heap_helper_export_dmabuf() failed\n");
		ret = PTR_ERR(dmabuf);
		goto err1;
	}

	buffer->dmabuf = dmabuf;

	ret = dma_buf_fd(dmabuf, fd_flags);
	if (ret < 0) {
		sunxi_err(NULL, "dma_buf_fd() failed\n");
		dma_buf_put(dmabuf);
		/* just return, as put will call release and that will free */
		return ret;
	}

	sunxi_debug(NULL, "done\n");
	return ret;

err1:
	while (pg > 0)
		__free_page(buffer->pages[--pg]);
	kfree(buffer->pages);
err0:
	kfree(buffer);

	return ret;
}

int sunxi_drm_heap_phys(struct dma_heap *heap, int dmabuf_fd,
			unsigned int *tee_addr, unsigned int *phy_addr, unsigned int *len)
{
	struct dma_buf *dma_buf;
	struct heap_helper_buffer *helper;

	(void)heap;
	dma_buf = dma_buf_get(dmabuf_fd);
	if (IS_ERR_OR_NULL(dma_buf)) {
		sunxi_err(NULL, "Invalid dmabuf_fd = 0x%x\n", dmabuf_fd);
		return PTR_ERR(dma_buf);
	}

	helper = (struct heap_helper_buffer *)dma_buf->priv;
	*phy_addr = page_to_phys(helper->pages[0]);
	*tee_addr = *phy_addr - sunxi_drm_info.drm_base + sunxi_drm_info.tee_base;
	*len = helper->size;

	dma_buf_put(dma_buf);
	return 0;
}

static const struct dma_heap_ops sunxi_drm_heap_ops = {
	.allocate = sunxi_drm_heap_allocate,
	.phys = sunxi_drm_heap_phys,
};

static int sunxi_drm_heap_init(void)
{
	struct dma_heap_export_info exp_info;
	struct dma_heap *heap;
	int ret = 0;

	ret = optee_probe_drm_configure(&sunxi_drm_info.drm_base,
					&sunxi_drm_info.drm_size,
					&sunxi_drm_info.tee_base);
	if (ret)
		return ret;

	drm_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!drm_pool)
		return -ENOMEM;

	gen_pool_set_algo(drm_pool, gen_pool_best_fit, NULL);
	ret = gen_pool_add_virt(drm_pool,  /* Add the **whole** DRM region to @drm_pool */
				sunxi_drm_info.tee_base,  /* Virt start addr on TA's view. Allocated by `heap_helper_buffer.xaddr` */
				sunxi_drm_info.drm_base,  /* Phys start addr */
				sunxi_drm_info.drm_size,  /* Size */
				-1);
	if (ret)
		goto exit;

	ret = dma_heap_init();
	if (ret)
		goto exit;

	exp_info.name = DRM_HEAP_DEV_NAME;
	exp_info.ops = &sunxi_drm_heap_ops;
	exp_info.priv = NULL;

	heap = dma_heap_add(&exp_info);
	if (IS_ERR(heap)) {
		ret = PTR_ERR(heap);
		goto exit;
	}

	return 0;

exit:
	gen_pool_destroy(drm_pool);
	return ret;
}

module_init(sunxi_drm_heap_init);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("weidonghui <weidonghui@allwinnertech.com>");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_VERSION("V1.4.1");
MODULE_DESCRIPTION("sunxi DRM heap driver");
