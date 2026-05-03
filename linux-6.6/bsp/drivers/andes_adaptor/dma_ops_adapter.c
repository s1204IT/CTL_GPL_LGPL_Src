// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * port dma ops to andes platform for memory access
 */
#include <linux/module.h>
#include <linux/cma.h>
#include <linux/dma-contiguous.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/mmzone.h>
#include <asm/dma-contiguous.h>
#include <asm/cacheflush.h>
#include <linux/dma-direct.h>
#include <linux/dma-noncoherent.h>

#if IS_ENABLED(CONFIG_AW_ANDES_NO_DIRECT_DMA)
void *uncached_kernel_address(void *ptr)
{
	static phys_addr_t base;
	static size_t size;
	struct cma *default_cma = dev_get_cma_area(NULL);

	/* oneshot iniliztion */
	if (default_cma) {
		size = cma_get_size(default_cma);
		base = cma_get_base(default_cma);
	}
	/* ptr validation */
	if (!size) {
		if (base < (phys_addr_t)ptr && (phys_addr_t)ptr < base + size) {
			goto ptr_is_uncached;
		}
	}
	pr_err("ptr %pad not non-cached!", &ptr);
	dump_stack();
ptr_is_uncached:
	return ptr;
}

void *cached_kernel_address(void *ptr)
{
	return ptr;
}

void arch_dma_prep_coherent(struct page *page, size_t size)
{
	arch_sync_dma_for_device(NULL, page_to_phys(page), size, DMA_TO_DEVICE);
}

long arch_dma_coherent_to_pfn(struct device *dev, void *cpu_addr,
			      dma_addr_t dma_addr)
{
	return __phys_to_pfn(dma_to_phys(dev, dma_addr));
}
#endif

#if IS_ENABLED(CONFIG_AW_ANDES_DMA_OPS)

void *non_cache_virt_base;
phys_addr_t non_cache_vir_offset;

/*
 * presumption:
 * 1. no-iommu involved so all memory we are dealing with
 * is contiguous here
 * 2. we are working on an platform works with limited dram
 * so highmem no need to worry about, lowmem address space
 * covers all the dram we have
 */
static void *nommu_dma_alloc(struct device *dev, size_t size,
			     dma_addr_t *dma_handle, gfp_t gfp,
			     unsigned long attrs)

{
	unsigned long order = get_order(size);
	size_t count = ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT;
	struct page *page;
	void *ptr = NULL;
	int allowblock = gfpflags_allow_blocking(gfp);

	/* CMA can be used only in the context which permits sleeping */
	if (allowblock) {
		page = dma_alloc_from_contiguous(dev, count, order,
						 gfp & __GFP_NOWARN);
	} else {
		page = alloc_pages(gfp, order);
	}
	if (!page)
		return NULL;

	*dma_handle = page_to_phys(page);
	if (pa_msb) {
		if (!allowblock) {
			ptr = page_to_virt(page);
			if (!(attrs & DMA_ATTR_WRITE_COMBINE))
				ptr = (void *)((
					uint32_t)(ptr + non_cache_vir_offset));
		} else {
			if (attrs & DMA_ATTR_WRITE_COMBINE)
				ptr = ioremap(*dma_handle, size);
			else
				ptr = ioremap_nocache(*dma_handle, size);
		}
	} else
		ptr = page_address(page);

	return ptr;
}

static void nommu_dma_free(struct device *dev, size_t size, void *cpu_addr,
			   dma_addr_t dma_addr, unsigned long attrs)
{
	if (!dma_release_from_contiguous(dev, phys_to_page(dma_addr),
					 PAGE_ALIGN(size) >> PAGE_SHIFT)) {
		/* cma realease fail, atomic alloc ,no unmap required */
		__free_pages(phys_to_page(dma_addr), get_order(size));
	} else {
		/* cma realease fail, non atomic alloc, unmap required */
		if (pa_msb)
			iounmap(cpu_addr);
	}
}

static int check_dma_addr(struct device *dev, dma_addr_t dma_addr, size_t size,
			  unsigned long attrs)
{
	struct cma *cma_area;
	phys_addr_t dma_start, dma_end;
	phys_addr_t cma_start, cma_end;

	if (pa_msb)
		return 0; /* MSB no addr limit */

	cma_area = dev_get_cma_area(dev);
	cma_start = cma_get_base(cma_area);
	cma_end = cma_start + cma_get_size(cma_area);
	dma_start = dma_addr;
	dma_end = dma_addr + size;
	if ((dma_addr < cma_start) || (dma_end > cma_end)) {
		/* only cma is non-cached, non-cached not possible*/
		if (!(attrs & DMA_ATTR_WRITE_COMBINE)) {
			WARN(true,
			     "dma addr %pad ~ %pad not in cma area %pad ~ %pad, non-cached not supported",
			     &dma_addr, &dma_end, &cma_start, &cma_end);
			return -1;
		}
	} else {
		/* cma is non-cached, write_combine not possible*/
		if (attrs & DMA_ATTR_WRITE_COMBINE) {
			WARN(true,
			     "dma addr %pad ~ %pad in cma area %pad ~ %pad, cached not supported",
			     &dma_addr, &dma_end, &cma_start, &cma_end);
			return -1;
		}
	}
	return 0;
}

static int nommu_dma_mmap(struct device *dev, struct vm_area_struct *vma,
			  void *cpu_addr, dma_addr_t dma_addr, size_t size,
			  unsigned long attrs)
{
	int ret;
	unsigned long user_count = vma_pages(vma);
	unsigned long count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned long off = vma->vm_pgoff;
	/* param sanity check */
	check_dma_addr(dev, dma_addr, size, attrs);

	vma->vm_page_prot = dma_pgprot(dev, vma->vm_page_prot, attrs);

	if (off >= count || user_count > count - off) {
		dev_err(dev, "out of range");
		return -ENXIO;
	}

	ret = remap_pfn_range(vma, vma->vm_start,
			      __phys_to_pfn(dma_to_phys(dev, dma_addr)) +
				      vma->vm_pgoff,
			      vma_pages(vma) << PAGE_SHIFT, vma->vm_page_prot);
	return ret;
}

int sunxi_andes_dma_mmap(struct device *dev, struct vm_area_struct *vma,
			 void *cpu_addr, dma_addr_t dma_addr, size_t size,
			 unsigned long attrs)
{
	return nommu_dma_mmap(dev, vma, cpu_addr, dma_addr, size, attrs);
}
EXPORT_SYMBOL_GPL(sunxi_andes_dma_mmap);

__maybe_unused static void __dma_page_cpu_to_dev(phys_addr_t paddr, size_t size,
				  enum dma_data_direction dir)
{
	arch_sync_dma_for_device(NULL, paddr, size, dir);
}

__maybe_unused static void __dma_page_dev_to_cpu(phys_addr_t paddr, size_t size,
				  enum dma_data_direction dir)
{
	arch_sync_dma_for_cpu(NULL, paddr, size, dir);
}

__maybe_unused static dma_addr_t nommu_dma_map_page(struct device *dev, struct page *page,
				     unsigned long offset, size_t size,
				     enum dma_data_direction dir,
				     unsigned long attrs)
{
	dma_addr_t handle = page_to_phys(page) + offset;
	/* param sanity check */
	check_dma_addr(dev, handle, size, attrs);

	__dma_page_cpu_to_dev(handle, size, dir);

	return handle;
}

__maybe_unused static void nommu_dma_unmap_page(struct device *dev, dma_addr_t handle,
				 size_t size, enum dma_data_direction dir,
				 unsigned long attrs)
{
	__dma_page_dev_to_cpu(handle, size, dir);
}

__maybe_unused static int nommu_dma_map_sg(struct device *dev, struct scatterlist *sgl,
			    int nents, enum dma_data_direction dir,
			    unsigned long attrs)
{
	int i;
	struct scatterlist *sg;

	for_each_sg (sgl, sg, nents, i) {
		sg_dma_address(sg) = sg_phys(sg);
		sg_dma_len(sg) = sg->length;
		check_dma_addr(dev, sg_phys(sg), sg->length, attrs);
		__dma_page_cpu_to_dev(sg_dma_address(sg), sg_dma_len(sg), dir);
	}

	return nents;
}

__maybe_unused static void nommu_dma_unmap_sg(struct device *dev, struct scatterlist *sgl,
			       int nents, enum dma_data_direction dir,
			       unsigned long attrs)
{
	struct scatterlist *sg;
	int i;

	for_each_sg (sgl, sg, nents, i)
		__dma_page_dev_to_cpu(sg_dma_address(sg), sg_dma_len(sg), dir);
}

__maybe_unused static void nommu_dma_sync_single_for_device(struct device *dev,
					     dma_addr_t handle, size_t size,
					     enum dma_data_direction dir)
{
	__dma_page_cpu_to_dev(handle, size, dir);
}

__maybe_unused static void nommu_dma_sync_single_for_cpu(struct device *dev, dma_addr_t handle,
					  size_t size,
					  enum dma_data_direction dir)
{
	__dma_page_cpu_to_dev(handle, size, dir);
}

__maybe_unused static void nommu_dma_sync_sg_for_device(struct device *dev,
					 struct scatterlist *sgl, int nents,
					 enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg (sgl, sg, nents, i)
		__dma_page_cpu_to_dev(sg_dma_address(sg), sg_dma_len(sg), dir);
}

__maybe_unused static void nommu_dma_sync_sg_for_cpu(struct device *dev,
				      struct scatterlist *sgl, int nents,
				      enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg (sgl, sg, nents, i)
		__dma_page_dev_to_cpu(sg_dma_address(sg), sg_dma_len(sg), dir);
}

const struct dma_map_ops aw_andes_dma_ops = {
	.alloc = nommu_dma_alloc,
	.free = nommu_dma_free,
	.mmap = nommu_dma_mmap,
	.map_page = dma_direct_map_page,
	.unmap_page = dma_direct_unmap_page,
	.map_sg = dma_direct_map_sg,
	.unmap_sg = dma_direct_unmap_sg,
	.sync_single_for_device = dma_direct_sync_single_for_device,
	.sync_single_for_cpu = dma_direct_sync_single_for_cpu,
	.sync_sg_for_device = dma_direct_sync_sg_for_device,
	.sync_sg_for_cpu = dma_direct_sync_sg_for_cpu,
};
EXPORT_SYMBOL(aw_andes_dma_ops);

static int dma_ops_init(void)
{
	int nid;
	int i = 0;

	for_each_online_node (nid) {
		i++;
		if (i > 1)
			continue;
		non_cache_virt_base =
			ioremap_nocache(pfn_to_phys(node_start_pfn(nid)),
					node_present_pages(nid) * PAGE_SIZE);
		non_cache_vir_offset =
			non_cache_virt_base - pfn_to_virt(node_start_pfn(nid));

		pr_debug("non-cache vir:%x offset:%x low\n",
		       (uint32_t)non_cache_virt_base,
		       (uint32_t)non_cache_vir_offset);
		pr_debug("phys:%x low vir:%x non-cache:%x~%x\n", 0x80000000,
		       (uint32_t)phys_to_virt(0x80000000),
		       (uint32_t)(phys_to_virt(0x80000000) +
				  non_cache_vir_offset),
		       (uint32_t)(phys_to_virt(0x80000000) +
				  non_cache_vir_offset +
				  PAGE_SIZE * node_present_pages(nid)));
	}
	if (i > 1)
		panic("support one 1 node only");
	return 0;
}

core_initcall(dma_ops_init)
#include <asm/pgtable.h>
__maybe_unused int __cache_compare_test(void)
{
	size_t size = 4096 * 16;
	struct page *test_target = alloc_pages(GFP_KERNEL, get_order(size));
#define ROUNDS (32)
	uint64_t result[ROUNDS];
	int round = 0;
	void *cached_addr, *non_cached_addr;
	phys_addr_t phy;
	if (!test_target) {
		return -ENOMEM;
	}
	pr_err("start %s", __func__);
	pr_err("pa_msb:%llx", pa_msb);

	phy = page_to_phys(test_target);
	cached_addr = ioremap(page_to_phys(test_target), size);
	non_cached_addr = ioremap_nocache(page_to_phys(test_target), size);
	pr_err("testing phys:%pad caced:%px non-cached:%px", &phy, cached_addr,
	       non_cached_addr);

	while (round < ROUNDS) {
		uint64_t start, end;
		int i;
		void *ptr = round >= ROUNDS / 2 ? cached_addr : non_cached_addr;
		if (round == 0 || round == ROUNDS / 2)
			pr_err("test ptr:%px", ptr);
		start = ktime_get_ns();
		for (i = 0; i < 12; i++) {
			memset(ptr, 4, size);
		}

		end = ktime_get_ns();
		result[round] = end - start;
		pr_err("round[%d]: %llu", round % (ROUNDS / 2), end - start);
		round++;
	}
	pr_err("pa_msb:%llx", pa_msb);
	iounmap(cached_addr);
	iounmap(non_cached_addr);
	free_pages((unsigned long)test_target, get_order(size));
	pr_err("test resut: noncached %lluns %lluns cached %lluns %lluns",
	       result[0], result[1], result[2], result[3]);
	return 0;
}

int __self_test(struct device *pdev)
{
	size_t size = 4096 * 16;
#define ROUNDS (32)
	uint64_t result[ROUNDS];
	int round = 0;
	void *cached_addr, *non_cached_addr;
	dma_addr_t cached_dma, non_cached_dma;
	phys_addr_t phy;
	pr_err("start %s", __func__);
	pr_err("pa_msb:%llx", pa_msb);

	cached_addr = dma_alloc_attrs(pdev, size, &cached_dma, GFP_KERNEL,
				      DMA_ATTR_WRITE_COMBINE);
	non_cached_addr =
		dma_alloc_attrs(pdev, size, &non_cached_dma, GFP_KERNEL, 0);
	pr_err("testing phys:%pad caced:%px non-cached:%px", &phy, cached_addr,
	       non_cached_addr);

	while (round < ROUNDS) {
		uint64_t start, end;
		int i;
		void *ptr = round >= ROUNDS / 2 ? cached_addr : non_cached_addr;
		if (round == 0 || round == ROUNDS / 2)
			pr_err("test ptr:%px", ptr);
		start = ktime_get_ns();
		for (i = 0; i < 12; i++) {
			memset(ptr, 4, size);
		}

		end = ktime_get_ns();
		result[round] = end - start;
		pr_err("round[%d]: %llu", round % (ROUNDS / 2), end - start);
		round++;
	}
	pr_err("pa_msb:%llx", pa_msb);
	dma_free_coherent(pdev, size, non_cached_addr, non_cached_dma);
	dma_free_attrs(pdev, size, cached_addr, cached_dma,
		       DMA_ATTR_WRITE_COMBINE);
	pr_err("test resut: noncached %lluns %lluns cached %lluns %lluns",
	       result[0], result[1], result[2], result[3]);
	return 0;
}

__maybe_unused static int __init dmaops_self_test(void)
{
	int ret;
	struct class *nsi_pmu_class;
	struct device *dev;
	u64 mask;

	nsi_pmu_class = class_create(THIS_MODULE, "nsi-pmu");
	if (IS_ERR(nsi_pmu_class)) {
		ret = PTR_ERR(nsi_pmu_class);
		goto out_err;
	}

	dev = device_create(nsi_pmu_class, NULL, MKDEV(257, 1), NULL, "nsi");
	if (IS_ERR(dev)) {
		pr_err("device_create failed!\n");
		goto out_err;
	}

	dev->dma_mask = &mask;
	dev->coherent_dma_mask = DMA_BIT_MASK(32);
	__self_test(dev);

	device_destroy(nsi_pmu_class, MKDEV(257, 1));
	class_destroy(nsi_pmu_class);

out_err:
	return ret;
}

#ifdef SELF_TEST_ENABLED
device_initcall(dmaops_self_test);
#endif
#endif

MODULE_DESCRIPTION("dma ops adapter");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ouyangkun <ouyangkun@allwinnertech.com>");
MODULE_VERSION("1.0.0");
