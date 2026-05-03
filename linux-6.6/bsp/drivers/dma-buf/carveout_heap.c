// SPDX-License-Identifier: GPL-2.0

#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/io.h>
#include <linux/genalloc.h>
#include <linux/of_reserved_mem.h>
#include <linux/debugfs.h>

struct carveout_heap_priv {
	struct dma_heap *heap;
	struct gen_pool *pool;
};

struct carveout_heap_buffer_priv {
	struct mutex lock;
	struct list_head attachments;

	unsigned long len;
	unsigned long num_pages;
	struct carveout_heap_priv *heap;
	void *vaddr;
	dma_addr_t dma_addr;
	int vmap_cnt;
};

struct carveout_heap_attachment {
	struct list_head head;
	struct sg_table table;

	struct device *dev;
	bool mapped;
};

static int carveout_heap_attach(struct dma_buf *buf,
				struct dma_buf_attachment *attachment)
{
	struct carveout_heap_buffer_priv *priv = buf->priv;
	struct carveout_heap_attachment *a;
	struct sg_table *table;
	int ret;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;
	INIT_LIST_HEAD(&a->head);
	a->dev = attachment->dev;
	attachment->priv = a;

	table = &a->table;
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_cleanup_attach;

	sg_set_page(table->sgl, phys_to_page(priv->dma_addr), priv->len, 0);
	mutex_lock(&priv->lock);
	list_add(&a->head, &priv->attachments);
	mutex_unlock(&priv->lock);

	return 0;

err_cleanup_attach:
	kfree(a);
	return ret;
}

static void carveout_heap_detach(struct dma_buf *dmabuf,
				 struct dma_buf_attachment *attachment)
{
	struct carveout_heap_buffer_priv *priv = dmabuf->priv;
	struct carveout_heap_attachment *a = attachment->priv;

	mutex_lock(&priv->lock);
	list_del(&a->head);
	mutex_unlock(&priv->lock);

	sg_free_table(&a->table);
	kfree(a);
}

static struct sg_table *
carveout_heap_map_dma_buf(struct dma_buf_attachment *attachment,
			  enum dma_data_direction direction)
{
	struct carveout_heap_attachment *a = attachment->priv;
	struct sg_table *table = &a->table;
	int ret;

	ret = dma_map_sgtable(a->dev, table, direction, 0);
	if (ret)
		return ERR_PTR(-ENOMEM);

	a->mapped = true;

	return table;
}

static void carveout_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
					struct sg_table *table,
					enum dma_data_direction direction)
{
	struct carveout_heap_attachment *a = attachment->priv;

	a->mapped = false;
	dma_unmap_sgtable(a->dev, table, direction, 0);
}

static int
carveout_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
				       enum dma_data_direction direction)
{
	struct carveout_heap_buffer_priv *priv = dmabuf->priv;
	struct carveout_heap_attachment *a;

	mutex_lock(&priv->lock);

	list_for_each_entry(a, &priv->attachments, head) {
		if (!a->mapped)
			continue;

		dma_sync_sgtable_for_cpu(a->dev, &a->table, direction);
	}

	mutex_unlock(&priv->lock);

	return 0;
}

static int
carveout_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
				     enum dma_data_direction direction)
{
	struct carveout_heap_buffer_priv *priv = dmabuf->priv;
	struct carveout_heap_attachment *a;

	mutex_lock(&priv->lock);

	list_for_each_entry(a, &priv->attachments, head) {
		if (!a->mapped)
			continue;

		dma_sync_sgtable_for_device(a->dev, &a->table, direction);
	}

	mutex_unlock(&priv->lock);

	return 0;
}

static int carveout_heap_mmap(struct dma_buf *dmabuf,
			      struct vm_area_struct *vma)
{
	struct carveout_heap_buffer_priv *priv = dmabuf->priv;
	struct page *page = phys_to_page(priv->dma_addr);

	return remap_pfn_range(vma, vma->vm_start, page_to_pfn(page),
			       priv->num_pages * PAGE_SIZE, vma->vm_page_prot);
}

static int carveout_heap_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct carveout_heap_buffer_priv *priv = dmabuf->priv;
	int ret = 0;

	mutex_lock(&priv->lock);
	if (priv->vmap_cnt) {
		priv->vmap_cnt++;
		iosys_map_set_vaddr(map, priv->vaddr);
		goto out;
	}

	priv->vmap_cnt++;
	iosys_map_set_vaddr(map, priv->vaddr);
out:
	mutex_unlock(&priv->lock);

	return ret;
}

static void carveout_heap_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	iosys_map_clear(map);
}

static void carveout_heap_dma_buf_release(struct dma_buf *buf)
{
	struct carveout_heap_buffer_priv *buffer_priv = buf->priv;
	struct carveout_heap_priv *heap_priv = buffer_priv->heap;

	gen_pool_free(heap_priv->pool, (unsigned long)buffer_priv->vaddr,
		      buffer_priv->len);

	mutex_lock(&buffer_priv->lock);
	if (!--buffer_priv->vmap_cnt) {
		buffer_priv->vaddr = NULL;
	}
	mutex_unlock(&buffer_priv->lock);

	kfree(buffer_priv);
}

static const struct dma_buf_ops carveout_heap_buf_ops = {
	.attach		= carveout_heap_attach,
	.detach		= carveout_heap_detach,
	.map_dma_buf	= carveout_heap_map_dma_buf,
	.unmap_dma_buf	= carveout_heap_unmap_dma_buf,
	.begin_cpu_access	= carveout_heap_dma_buf_begin_cpu_access,
	.end_cpu_access	= carveout_heap_dma_buf_end_cpu_access,
	.mmap		= carveout_heap_mmap,
	.vmap           = carveout_heap_vmap,
	.vunmap         = carveout_heap_vunmap,
	.release	= carveout_heap_dma_buf_release,
};

static struct dma_buf *carveout_heap_allocate(struct dma_heap *heap,
					      unsigned long len,
					      unsigned long fd_flags,
					      unsigned long heap_flags)
{
	struct carveout_heap_priv *heap_priv = dma_heap_get_drvdata(heap);
	struct carveout_heap_buffer_priv *buffer_priv;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	size_t size = PAGE_ALIGN(len);
	struct dma_buf *buf;
	dma_addr_t daddr;
	void *vaddr;
	int ret;

	buffer_priv = kzalloc(sizeof(*buffer_priv), GFP_KERNEL);
	if (!buffer_priv)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer_priv->attachments);
	mutex_init(&buffer_priv->lock);

	vaddr = gen_pool_dma_zalloc(heap_priv->pool, size, &daddr);
	if (!vaddr) {
		ret = -ENOMEM;
		goto err_free_buffer_priv;
	}

	buffer_priv->vaddr = vaddr;
	buffer_priv->dma_addr = daddr;
	buffer_priv->heap = heap_priv;
	buffer_priv->len = size;
	buffer_priv->num_pages = size / PAGE_SIZE;

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &carveout_heap_buf_ops;
	exp_info.size = buffer_priv->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer_priv;

	buf = dma_buf_export(&exp_info);
	if (IS_ERR(buf)) {
		ret = PTR_ERR(buf);
		goto err_free_buffer;
	}

	return buf;

err_free_buffer:
	gen_pool_free(heap_priv->pool, (unsigned long)vaddr, len);
err_free_buffer_priv:
	kfree(buffer_priv);

	return ERR_PTR(ret);
}

static const struct dma_heap_ops carveout_heap_ops = {
	.allocate = carveout_heap_allocate,
};

#define TOTAL_ENTITY_PER_LINE    (20)

static int debug_carveout_heap_show(struct seq_file *s, void *unused)
{
	struct carveout_heap_priv *priv = s->private;
	struct gen_pool *pool = NULL;
	struct gen_pool_chunk *chunk;
	unsigned int size, total_bits, bits_per_unit;
	unsigned int i, index, offset, tmp, busy;
	unsigned int busy_cnt = 0, free_cnt = 0, total_cnt;
	unsigned int dump_unit = SZ_128K;
	unsigned char tmp1 = 0x0;
	unsigned char offset1 = 0x0;
	unsigned int line_cnt = 0;
	unsigned int num_per_line_cnt = 0;

	pool = priv->pool;
	rcu_read_lock();

	list_for_each_entry_rcu(chunk, &pool->chunks, next_chunk) {
		size = (unsigned int)(chunk->end_addr + 1 - chunk->start_addr);
		total_bits = size >> (unsigned int)pool->min_alloc_order;
		bits_per_unit = dump_unit >> (unsigned int)pool->min_alloc_order;
		seq_printf(s, "\n Carveout memory layout(0: free, 1: busy, unit: %dKB):\n",
			dump_unit / 1024);
		total_cnt = (unsigned int)(chunk->end_addr + 1 - chunk->start_addr) / dump_unit;
		busy_cnt = 0;
		free_cnt = 0;
		tmp1 = 0x0;
		offset1 = 0x0;
		line_cnt = 0;
		num_per_line_cnt = 0;
		seq_printf(s, "\n 0x%lx: ", chunk->start_addr);

		for (i = 0, tmp = 0, busy = 0; i < total_bits; i++) {
			index = i / (sizeof(chunk->bits[0]) * 8);
			offset = i & (sizeof(chunk->bits[0]) * 8 - 1);

			if (!busy && (chunk->bits[index] & (1 << offset)))
				busy = 1;

			if (++tmp == bits_per_unit) {
				if (busy) {
					tmp1 |= (0x1 << (offset1 % 8));
					busy_cnt++;
				}

				++offset1;
				if (!(offset1 % 8)) {
					seq_printf(s, " %02x", tmp1);

					++num_per_line_cnt;
					if (!(num_per_line_cnt % TOTAL_ENTITY_PER_LINE)) {
						++line_cnt;
						seq_printf(s, "\n 0x%lx: ",
							chunk->start_addr +
								(TOTAL_ENTITY_PER_LINE * dump_unit * 8 * line_cnt));
						num_per_line_cnt = 0;
					}

					offset1 = 0;
					tmp1 = 0;
				}

				busy = 0;
				tmp = 0;
			}
		}

		free_cnt = total_cnt - busy_cnt;
		seq_printf(s, "\n\n Carveout area start:0x%lx end:0x%lx\n",
			chunk->start_addr, chunk->end_addr + 1);
		seq_printf(s, " Carveout area Total:%uMB Free:%uKB ~= %uMB\n\n",
			total_cnt * dump_unit / (1024 * 1024),
			free_cnt * dump_unit / 1024,
			free_cnt * dump_unit / (1024 * 1024));
	}

	rcu_read_unlock();

	return 0;

}

static int debug_carveout_heap_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_carveout_heap_show, inode->i_private);
}

static const struct file_operations debug_carveout_heap_fops = {
	.open = debug_carveout_heap_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init carveout_heap_setup(struct device_node *node)
{
	struct dma_heap_export_info exp_info = {};
	const struct reserved_mem *rmem;
	struct carveout_heap_priv *priv;
	struct dma_heap *heap;
	struct gen_pool *pool;
	void *base;
	int ret;
	struct dentry *carveout_heap_debugfs_dir = NULL;

	rmem = of_reserved_mem_lookup(node);
	if (!rmem)
		return -EINVAL;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	pool = gen_pool_create(PAGE_SHIFT, NUMA_NO_NODE);
	if (!pool) {
		ret = -ENOMEM;
		goto err_cleanup_heap;
	}

	gen_pool_set_algo(pool, gen_pool_best_fit, NULL);
	priv->pool = pool;

	base = memremap(rmem->base, rmem->size, MEMREMAP_WB);
	if (!base) {
		ret = -ENOMEM;
		goto err_release_mem_region;
	}

	ret = gen_pool_add_virt(pool, (unsigned long)base, rmem->base,
				rmem->size, NUMA_NO_NODE);
	if (ret)
		goto err_unmap;

	exp_info.name = node->full_name;
	exp_info.ops = &carveout_heap_ops;
	exp_info.priv = priv;

	heap = dma_heap_add(&exp_info);
	if (IS_ERR(heap)) {
		ret = PTR_ERR(heap);
		goto err_cleanup_pool_region;
	}
	priv->heap = heap;

	carveout_heap_debugfs_dir = debugfs_create_dir("carveout_heap", NULL);
	if (!carveout_heap_debugfs_dir) {
		pr_err("failed to  create carveout heap debugfs directory\n");
		ret = -1;
		goto err_cleanup_pool_region;
	}

	if (!debugfs_create_file("carveout", 0400, carveout_heap_debugfs_dir,
		priv, &debug_carveout_heap_fops)) {
		pr_err("failed to  create carveout_heap/carveout debugfs file\n");
		ret  = -1;
		goto err_rmdir;
	}

	return 0;

err_rmdir:
	debugfs_remove_recursive(carveout_heap_debugfs_dir);
err_cleanup_pool_region:
	gen_pool_free(pool, (unsigned long)base, rmem->size);
err_unmap:
	memunmap(base);
err_release_mem_region:
	gen_pool_destroy(pool);
err_cleanup_heap:
	kfree(priv);
	return ret;
}

static int __init carveout_heap_init(void)
{
	struct device_node *rmem_node;
	struct device_node *node;
	int ret;

	rmem_node = of_find_node_by_path("/reserved-memory");
	if (!rmem_node)
		return 0;

	for_each_child_of_node(rmem_node, node) {
		if (!of_property_read_bool(node, "dma-buf"))
			continue;

		ret = carveout_heap_setup(node);
		if (ret)
			return ret;
	}

	return 0;
}

module_init(carveout_heap_init);

MODULE_LICENSE("GPL v3");
MODULE_VERSION("1.0.1");
MODULE_AUTHOR("panzhijian<panzhijian@allwinnertech.com>");
