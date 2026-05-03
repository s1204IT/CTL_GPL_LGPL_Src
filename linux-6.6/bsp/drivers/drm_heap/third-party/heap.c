// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Framework for userspace DMA-BUF allocations
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2019 Linaro Ltd.
 */
//#define DEBUG
#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/xarray.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <uapi/security/sunxi-drm-heap.h>
#include <sunxi-log.h>
#include "heap.h"

#define DRM_HEAP_CLASS_NAME	"sunxi_heap"
#define NUM_HEAP_MINORS		128

extern struct sunxi_drm_info sunxi_drm_info;

/**
 * struct dma_heap - represents a dmabuf heap in the system
 * @name:		used for debugging/device-node name
 * @ops:		ops struct for this heap
 * @heap_devt		heap device node
 * @list		list head connecting to list of heaps
 * @heap_cdev		heap char device
 *
 * Represents a heap of memory from which buffers can be made.
 */
struct dma_heap {
	const char *name;
	const struct dma_heap_ops *ops;
	void *priv;
	dev_t heap_devt;
	struct list_head list;
	struct cdev heap_cdev;
};

static LIST_HEAD(heap_list);
static DEFINE_MUTEX(heap_list_lock);
static dev_t dma_heap_devt;
static struct class *dma_heap_class;
static DEFINE_XARRAY_ALLOC(dma_heap_minors);


static int dma_heap_open(struct inode *inode, struct file *file)
{
	struct dma_heap *heap;

	sunxi_debug(NULL, "\n");

	/* load @heap from xarray @dma_heap_minors */
	heap = xa_load(&dma_heap_minors, iminor(inode));  /* @dma_heap_minors is inited in dma_heap_add() */
	if (!heap) {
		sunxi_err(NULL, "dma_heap: minor %d unknown.\n", iminor(inode));
		return -ENODEV;
	}

	/* instance data as context */
	file->private_data = heap;
	nonseekable_open(inode, file);

	return 0;
}

static long dma_heap_ioctl_allocate(struct file *file, void *kdata)
{
	struct dma_heap_allocation_data *heap_allocation = kdata;
	struct dma_heap *heap = file->private_data;
	size_t len;
	int fd;

	sunxi_debug(NULL, "Begin. len = 0x%llx\n", heap_allocation->len);

	if (heap_allocation->fd) {
		sunxi_err(NULL, "heap_allocation->fd = %d, which is in use\n", heap_allocation->fd);
		return -EINVAL;
	}

	if (heap_allocation->fd_flags & ~DMA_HEAP_VALID_FD_FLAGS) {
		sunxi_err(NULL, "'heap_allocation->fd_flags' is invalid\n");
		return -EINVAL;
	}

	if (heap_allocation->heap_flags & ~DMA_HEAP_VALID_HEAP_FLAGS) {
		sunxi_err(NULL, "'heap_allocation->heap_flags' is invalid\n");
		return -EINVAL;
	}

	/*
	 * Allocations from all heaps have to begin
	 * and end on page boundaries.
	 */
	len = PAGE_ALIGN(heap_allocation->len);
	if (!len) {
		sunxi_err(NULL, "size 0x%lx is not page aligned\n", len);
		return -EINVAL;
	}

	/* heap->ops->allocate = sunxi_drm_heap_allocate */
	fd =  heap->ops->allocate(heap, len, heap_allocation->fd_flags, heap_allocation->heap_flags);
	if (fd < 0)
		return fd;
	heap_allocation->fd = fd;

	sunxi_debug(NULL, "Done. dmabuf_fd = %d\n", heap_allocation->fd);
	return 0;
}

/* Get tee_addr, phys_addr and size from current dma_buf_fd */
static long dma_heap_ioctl_phys(struct file *file, void *kdata)
{
	sunxi_drm_phys_data *phys_data = kdata;
	struct dma_heap *heap = file->private_data;  /* Inited in dma_heap_open() */
	int err;

	/* phys_data->handle = heap_allocation->fd = dmabuf_fd */
	sunxi_debug(NULL, "dmabuf_fd = %d\n", phys_data->handle);
	/* heap->ops->phys = sunxi_drm_heap_phys */
	err = heap->ops->phys(heap,
			       phys_data->handle,
			       &phys_data->tee_addr, &phys_data->phys_addr, &phys_data->size);
	sunxi_debug(NULL, "tee_addr = 0x%x, phys_addr = 0x%x, size = 0x%x\n",
			   phys_data->tee_addr, phys_data->phys_addr, phys_data->size);
	return err;
}

static long dma_heap_ioctl_info(struct file *file, void *kdata)
{
	struct sunxi_drm_info *drm_info = kdata;
	*drm_info = sunxi_drm_info;
	sunxi_debug(NULL, "drm_base = 0x%lx, drm_size = 0x%lx, tee_base = 0x%lx\n",
			   drm_info->drm_base, drm_info->drm_size, drm_info->tee_base);
	return 0;
}

unsigned int dma_heap_ioctl_cmds[] = {
	DMA_HEAP_IOC_ALLOC,
	DMA_HEAP_GET_ADDR,
	DMA_HEAP_GET_INFO,
};

static long dma_heap_ioctl(struct file *file, unsigned int ucmd,
			   unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	unsigned int kcmd;
	unsigned int in_size, out_size, drv_size, ksize;
	int nr = _IOC_NR(ucmd);
	int ret = 0;

	if (nr >= ARRAY_SIZE(dma_heap_ioctl_cmds)) {
		sunxi_err(NULL, "Invalid nr = %d\n", nr);
		return -EINVAL;
	}

	/* Get the kernel ioctl cmd that matches */
	kcmd = dma_heap_ioctl_cmds[nr];

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(kcmd);
	out_size = _IOC_SIZE(ucmd);
	in_size = out_size;
	if ((ucmd & kcmd & IOC_IN) == 0)
		in_size = 0;
	if ((ucmd & kcmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata) {
			sunxi_err(NULL, "kmalloc(ksize) failed\n");
			return -ENOMEM;
		}
	}

	if (copy_from_user(kdata, (void __user *)arg, in_size) != 0) {
		sunxi_err(NULL, "copy_from_user() failed\n");
		ret = -EFAULT;
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (kcmd) {
	case DMA_HEAP_IOC_ALLOC:
		ret = dma_heap_ioctl_allocate(file, kdata);
		break;
	case DMA_HEAP_GET_ADDR:
		ret = dma_heap_ioctl_phys(file, kdata);
		break;
	case DMA_HEAP_GET_INFO:
		ret = dma_heap_ioctl_info(file, kdata);
		break;
	default:
		sunxi_err(NULL, "Unknow cmd 0x%x\n", kcmd);
		return -ENOTTY;
	}

	if (ret) {
		sunxi_err(NULL, "Failed for cmd 0x%x. ret = 0x%x\n", kcmd, ret);
		goto err;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0) {
		sunxi_err(NULL, "copy_to_user() failed\n");
		ret = -EFAULT;
	}
err:
	if (kdata != stack_kdata)
		kfree(kdata);
	return ret;
}

static const struct file_operations dma_heap_fops = {
	.owner = THIS_MODULE,
	.open = dma_heap_open,
	.unlocked_ioctl = dma_heap_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dma_heap_ioctl,
#endif
};

/**
 * dma_heap_get_drvdata() - get per-subdriver data for the heap
 * @heap: DMA-Heap to retrieve private data for
 *
 * Returns:
 * The per-subdriver data for the heap.
 */
//static void *dma_heap_get_drvdata(struct dma_heap *heap)
//{
//	return heap->priv;
//}

static struct dma_heap *
dma_heap_add(const struct dma_heap_export_info *exp_info)
{
	struct dma_heap *heap, *h, *err_ret;
	struct device *dev_ret;
	unsigned int minor;
	int ret;

	if (!exp_info->name || !strcmp(exp_info->name, "")) {
		sunxi_err(NULL, "dma_heap: Cannot add heap without a name\n");
		return ERR_PTR(-EINVAL);
	}

	if (!exp_info->ops || !exp_info->ops->allocate) {
		sunxi_err(NULL, "dma_heap: Cannot add heap with invalid ops struct\n");
		return ERR_PTR(-EINVAL);
	}

	/* check the name is unique */
	mutex_lock(&heap_list_lock);
	list_for_each_entry (h, &heap_list, list) {
		if (!strcmp(h->name, exp_info->name)) {
			mutex_unlock(&heap_list_lock);
			sunxi_err(NULL, "dma_heap: Already registered heap named %s\n",
			       exp_info->name);
			return ERR_PTR(-EINVAL);
		}
	}
	mutex_unlock(&heap_list_lock);

	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);

	heap->name = exp_info->name;
	heap->ops = exp_info->ops;
	heap->priv = exp_info->priv;

	/* Find unused minor number */
	ret = xa_alloc(&dma_heap_minors, &minor, heap,
		       XA_LIMIT(0, NUM_HEAP_MINORS - 1), GFP_KERNEL);
	if (ret < 0) {
		sunxi_err(NULL, "dma_heap: Unable to get minor number for heap\n");
		err_ret = ERR_PTR(ret);
		goto err0;
	}

	/* Create device "/dev/sunxi_drm_heap" */
	heap->heap_devt = MKDEV(MAJOR(dma_heap_devt), minor);

	cdev_init(&heap->heap_cdev, &dma_heap_fops);
	ret = cdev_add(&heap->heap_cdev, heap->heap_devt, 1);
	if (ret < 0) {
		sunxi_err(NULL, "dma_heap: Unable to add char device\n");
		err_ret = ERR_PTR(ret);
		goto err1;
	}

	/* Create "/sys/class/sunxi_heap/sunxi_drm_heap/" */
	dev_ret = device_create(dma_heap_class, NULL, heap->heap_devt, NULL,
				heap->name);
	if (IS_ERR(dev_ret)) {
		sunxi_err(NULL, "dma_heap: Unable to create device\n");
		err_ret = ERR_CAST(dev_ret);
		goto err2;
	}

	/* Add heap to the list */
	mutex_lock(&heap_list_lock);
	list_add(&heap->list, &heap_list);
	mutex_unlock(&heap_list_lock);

	return heap;

err2:
	cdev_del(&heap->heap_cdev);
err1:
	xa_erase(&dma_heap_minors, minor);
err0:
	kfree(heap);
	return err_ret;
}

static int dma_heap_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&dma_heap_devt, 0, NUM_HEAP_MINORS, DRM_HEAP_CLASS_NAME);
	if (ret)
		return ret;

	/* Create "/sys/class/sunxi_heap/" */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	dma_heap_class = class_create(DRM_HEAP_CLASS_NAME);
#else
	dma_heap_class = class_create(THIS_MODULE, DRM_HEAP_CLASS_NAME);
#endif
	if (IS_ERR(dma_heap_class)) {
		unregister_chrdev_region(dma_heap_devt, NUM_HEAP_MINORS);
		return PTR_ERR(dma_heap_class);
	}

	return 0;
}
