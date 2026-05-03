// SPDX-License-Identifier: GPL-2.0
//#define DEBUG
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/dma-map-ops.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <uapi/security/sunxi-drm-heap.h>
#include <sunxi-log.h>
#include "heap-helpers.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
typedef struct iosys_map	heap_map_t;
#define heap_map_set_vaddr	iosys_map_set_vaddr
#define heap_map_clear		iosys_map_clear
#else  /* LINUX_VERSION_CODE */
typedef struct dma_buf_map	heap_map_t;
#define heap_map_set_vaddr	dma_buf_map_set_vaddr
#define heap_map_clear		dma_buf_map_clear
#endif  /* LINUX_VERSION_CODE */

static void init_heap_helper_buffer(struct heap_helper_buffer *buffer,
				    void (*free)(struct heap_helper_buffer *))
{
	buffer->priv_virt = NULL;
	mutex_init(&buffer->lock);
	buffer->vmap_cnt = 0;
	buffer->vaddr = NULL;
	buffer->xaddr = NULL;
	buffer->pagecount = 0;
	buffer->pages = NULL;
	INIT_LIST_HEAD(&buffer->attachments);
	buffer->free = free;
}

static struct dma_buf *
heap_helper_export_dmabuf(struct heap_helper_buffer *buffer, int fd_flags)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.ops = &heap_helper_ops;
	exp_info.size = buffer->size;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;

	return dma_buf_export(&exp_info);
}

struct dma_heaps_attachment {
	struct device *dev;
	struct sg_table table;
	struct list_head list;
};

/* dma_buf callback: `heap_helper_ops.attach` */
static int dma_heap_attach(struct dma_buf *dmabuf,
			   struct dma_buf_attachment *attachment)
{
	struct dma_heaps_attachment *a;
	struct heap_helper_buffer *buffer = dmabuf->priv;
	int ret;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	ret = sg_alloc_table_from_pages(&a->table, buffer->pages,
					buffer->pagecount, 0,
					buffer->pagecount << PAGE_SHIFT,
					GFP_KERNEL);
	if (ret) {
		kfree(a);
		return ret;
	}

	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);

	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

/* dma_buf callback: `heap_helper_ops.detach` */
static void dma_heap_detach(struct dma_buf *dmabuf,
			    struct dma_buf_attachment *attachment)
{
	struct dma_heaps_attachment *a = attachment->priv;
	struct heap_helper_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(&a->table);
	kfree(a);
}

/* dma_buf callback: `heap_helper_ops.map_dma_buf` */
static struct sg_table *
dma_heap_map_dma_buf(struct dma_buf_attachment *attachment,
		     enum dma_data_direction direction)
{
	struct dma_heaps_attachment *a = attachment->priv;
	struct sg_table *table;

	table = &a->table;

	if (!dma_map_sg(attachment->dev, table->sgl, table->nents, direction))
		table = ERR_PTR(-ENOMEM);
	return table;
}

/* dma_buf callback: `heap_helper_ops.unmap_dma_buf` */
static void dma_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				   struct sg_table *table,
				   enum dma_data_direction direction)
{
	dma_unmap_sg(attachment->dev, table->sgl, table->nents, direction);
}

static vm_fault_t dma_heap_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct heap_helper_buffer *buffer = vma->vm_private_data;

	if (vmf->pgoff > buffer->pagecount)
		return VM_FAULT_SIGBUS;

	vmf->page = buffer->pages[vmf->pgoff];
	get_page(vmf->page);

	return 0;
}

static const struct vm_operations_struct dma_heap_vm_ops = {
	.fault = dma_heap_vm_fault,
};

/* dma_buf callback: `heap_helper_ops.mmap` */
static int dma_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct heap_helper_buffer *buffer = dmabuf->priv;

	if ((vma->vm_flags & (VM_SHARED | VM_MAYSHARE)) == 0)
		return -EINVAL;

	vma->vm_ops = &dma_heap_vm_ops;
	vma->vm_private_data = buffer;

	return 0;
}

static void dma_heap_buffer_destroy(struct heap_helper_buffer *buffer)
{
	if (buffer->vmap_cnt > 0) {
		sunxi_err(NULL, "buffer still mapped in the kernel\n");
		vunmap(buffer->vaddr);
		buffer->vmap_cnt = 0;
	}

	buffer->free(buffer);  /* sunxi_drm_heap_free() */
}

/* dma_buf callback: `heap_helper_ops.release` */
/* Called by `close(dma_buf_fd);` */
static void dma_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct heap_helper_buffer *buffer = dmabuf->priv;
	sunxi_debug(NULL, "len = 0x%lx, xaddr = 0x%llx\n",
		    buffer->size, (unsigned long long)buffer->xaddr);
	dma_heap_buffer_destroy(buffer);
	sunxi_debug(NULL, "done\n");
}

/* dma_buf callback: `heap_helper_ops.begin_cpu_access` */
static int dma_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					     enum dma_data_direction direction)
{
	struct heap_helper_buffer *buffer = dmabuf->priv;
	struct dma_heaps_attachment *a;
	int ret = 0;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->size);

	list_for_each_entry(a, &buffer->attachments, list) {
		dma_sync_sg_for_cpu(a->dev, a->table.sgl, a->table.nents,
				    direction);
	}
	mutex_unlock(&buffer->lock);

	return ret;
}

/* dma_buf callback: `heap_helper_ops.end_cpu_access` */
static int dma_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					   enum dma_data_direction direction)
{
	struct heap_helper_buffer *buffer = dmabuf->priv;
	struct dma_heaps_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->size);

	list_for_each_entry(a, &buffer->attachments, list) {
		dma_sync_sg_for_device(a->dev, a->table.sgl, a->table.nents,
				       direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

/* vmap() the buffer, update @buffer and @map  */
static int dma_heap_buffer_vmap_get(struct heap_helper_buffer *buffer, heap_map_t *map)
{
	void *vaddr;

	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		heap_map_set_vaddr(map, buffer->vaddr);
		return 0;
	}

	vaddr = vmap(buffer->pages, buffer->pagecount, VM_MAP, PAGE_KERNEL);
	if (!vaddr)
		return -ENOMEM;

	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;
	heap_map_set_vaddr(map, buffer->vaddr);
	return 0;
}

static void dma_heap_buffer_vmap_put(struct heap_helper_buffer *buffer)
{
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
}

/* dma_buf callback: `heap_helper_ops.vmap` */
static int dma_heap_dma_buf_vmap(struct dma_buf *dmabuf, heap_map_t *map)
{
	struct heap_helper_buffer *buffer = dmabuf->priv;
	int ret = 0;

	mutex_lock(&buffer->lock);
	ret = dma_heap_buffer_vmap_get(buffer, map);
	mutex_unlock(&buffer->lock);

	return ret;
}

/* dma_buf callback: `heap_helper_ops.vunmap` */
static void dma_heap_dma_buf_vunmap(struct dma_buf *dmabuf, heap_map_t *map)
{
	struct heap_helper_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	dma_heap_buffer_vmap_put(buffer);
	mutex_unlock(&buffer->lock);
	heap_map_clear(map);
}

const struct dma_buf_ops heap_helper_ops = {
	.map_dma_buf = dma_heap_map_dma_buf,
	.unmap_dma_buf = dma_heap_unmap_dma_buf,
	.mmap = dma_heap_mmap,
	.release = dma_heap_dma_buf_release,
	.attach = dma_heap_attach,
	.detach = dma_heap_detach,
	.begin_cpu_access = dma_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = dma_heap_dma_buf_end_cpu_access,
	.vmap = dma_heap_dma_buf_vmap,
	.vunmap = dma_heap_dma_buf_vunmap,
};
