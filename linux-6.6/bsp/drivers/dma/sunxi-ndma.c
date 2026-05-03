// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013-2023 Allwinner Tech Co., Ltd
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <virt-dma.h>

#include "sunxi-ndma.h"


static void set_drq_v100(u32 *p_cfg, s8 src_drq, s8 dst_drq)
{
	*p_cfg |= NDMA_CHAN_CFG_SRC_DRQ_V100(src_drq) |
		  NDMA_CHAN_CFG_DST_DRQ_V100(dst_drq);
}

static void set_mode_v100(u32 *p_cfg, s8 src_mode, s8 dst_mode)
{
	*p_cfg |= NDMA_CHAN_CFG_SRC_MODE_V100(src_mode) |
		  NDMA_CHAN_CFG_DST_MODE_V100(dst_mode);
}

static void irq_enable_v100(struct sunxi_ndma_dev *sdev, u32 irq_val)
{
	writel(irq_val, sdev->base + NDMA_IRQ_EN);
}

static u32 get_irq_status_v100(struct sunxi_ndma_dev *sdev)
{
	u32 status;

	status = readl(sdev->base + NDMA_PEND_STAT);

	return status;
}

static u32 read_irq_enable_v100(struct sunxi_ndma_dev *sdev)
{
	u32 reg_val;

	reg_val = readl(sdev->base + NDMA_IRQ_EN);

	return reg_val;
}

static void clear_irq_status_v100(struct sunxi_ndma_dev *sdev, u32 status)
{
	writel(status, sdev->base + NDMA_PEND_STAT);
}

static void set_burst_length_v100(u32 *p_cfg, s8 src_burst, s8 dst_burst)
{
	*p_cfg |= NDMA_CHAN_CFG_SRC_BURST_V100(src_burst) |
		  NDMA_CHAN_CFG_DST_BURST_V100(dst_burst);
}

static void set_dma_wait_cycle_v100(u32 *p_cfg, s8 cycles)
{
	*p_cfg |= NDMA_CHAN_CFG_WAIT_CYCLES_V100(cycles);
}

static inline s8 convert_burst(u32 maxburst)
{
	switch (maxburst) {
	case 1:
		return 0;
	case 4:
		return 1;
	default:
		return -EINVAL;
	}
}

static inline s8 convert_buswidth(enum dma_slave_buswidth addr_width)
{
	return ilog2(addr_width);
}

static inline void sunxi_ndma_dump_com_regs(struct sunxi_ndma_dev *sdev)
{
	dev_dbg(sdev->slave.dev, "Common register:\n"
			"NDMA_IRQ_EN_REG  : 0x%08x\n"
			"NDMA_IRQ_PEND_REG: 0x%08x\n"
			"NDMA_AUTO_GAT_REG: 0x%08x\n"
			"NDMA_AUTOGAT_LEN : 0x%08x\n",
			readl(sdev->base + NDMA_IRQ_EN),
			readl(sdev->base + NDMA_PEND_STAT),
			readl(sdev->base + NDMA_AUTO_GAT),
			readl(sdev->base + NDMA_AUTOGAT_LEN));
}

static inline void sunxi_ndma_dump_chan_regs(struct sunxi_ndma_dev *sdev,
					    struct sunxi_pchan *pchan)
{

	dev_dbg(sdev->slave.dev, "Chan %d\n"
		" cfg : 0x%08x\n"
		" src : 0x%08x\n"
		" dst : 0x%08x\n"
		"count: 0x%08x\n",
		pchan->idx,
		readl(pchan->base + NDMA_CTRL_REG),
		readl(pchan->base + NDMA_SRC_ADDR_REG),
		readl(pchan->base + NDMA_DST_ADDR_REG),
		readl(pchan->base + NDMA_BC_REG));
}


static void *sunxi_ndma_lli_add(struct sunxi_ndma_lli *prev,
			       struct sunxi_ndma_lli *next,
			       dma_addr_t next_phy,
			       struct sunxi_desc *txd)
{
	if ((!prev && !txd) || !next)
		return NULL;

	if (!prev) {
		txd->p_lli = next_phy;
		txd->v_lli = next;
	} else {
		prev->p_lli_next = next_phy;
		prev->v_lli_next = next;
	}

	next->p_lli_next = END_OF_LLI;
	next->v_lli_next = NULL;

	return next;
}

static inline void sunxi_ndma_dump_lli(struct sunxi_vchan *vchan,
				      struct sunxi_ndma_lli *lli)
{
	dev_dbg(chan2dev(&vchan->vc.chan),
		"desc:   p_lli : %pad v_lli : 0x%p\n"
		"cfg : 0x%08x src : 0x%08x dst : 0x%08x\n"
		"len : 0x%08x next_p_lli : 0x%08x\n",
		&lli->this_phy, lli,
		lli->cfg, lli->src, lli->dst,
		lli->len, lli->p_lli_next);

}

static inline void sunxi_ndma_kill_tasklet(struct sunxi_ndma_dev *sdev)
{
	/* Disable all interrupts from NDMA */
	writel(0, sdev->base + NDMA_IRQ_EN);

	/* Prevent spurious interrupts from scheduling the tasklet */
	atomic_inc(&sdev->tasklet_shutdown);

	/* Make sure we won't have any further interrupts */
	devm_free_irq(sdev->slave.dev, sdev->irq, sdev);

	/* Actually prevent the tasklet from being scheduled */
	tasklet_kill(&sdev->task);
}

static struct dma_chan *sunxi_ndma_of_xlate(struct of_phandle_args *dma_spec,
					   struct of_dma *ofdma)
{
	struct sunxi_ndma_dev *sdev = ofdma->of_dma_data;
	struct sunxi_vchan *vchan;
	struct dma_chan *chan;
	u8 port = dma_spec->args[0];

	if (port > sdev->max_request)
		return NULL;

	chan = dma_get_any_slave_channel(&sdev->slave);
	if (!chan)
		return NULL;

	vchan = to_sunxi_vchan(chan);
	vchan->port = port;

	return chan;
}

static void sunxi_ndma_free_desc(struct virt_dma_desc *vd)
{
	struct sunxi_desc *txd = to_sunxi_desc(&vd->tx);
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(vd->tx.chan->device);
	struct sunxi_ndma_lli *v_lli;
	dma_addr_t p_lli;

	if (unlikely(!txd))
		return;

	p_lli = txd->p_lli;
	v_lli = txd->v_lli;

	dma_pool_free(sdev->pool, v_lli, p_lli);

	if (v_lli->p_lli_next != END_OF_LLI) {
		txd->p_lli = v_lli->p_lli_next;
		txd->v_lli = v_lli->v_lli_next;

	/*
	 * In this txd, there are still unprocessed descriptors,
	 * return to continue processing.
	 */
		return;
	}

	txd->vd.tx.callback = NULL;
	txd->vd.tx.callback_result = NULL;
	txd->vd.tx.callback_param = NULL;
	kfree(txd);
	txd = NULL;
}

static int sunxi_ndma_start_desc(struct sunxi_vchan *vchan)
{
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(vchan->vc.chan.device);
	struct virt_dma_desc *desc = vchan_next_desc(&vchan->vc);
	struct sunxi_pchan *pchan = vchan->phy;
	u32 irq_val;

	if (!pchan)
		return -EAGAIN;

	if (!desc) {
		pchan->desc = NULL;
		pchan->done = NULL;
		return -EAGAIN;
	}

	pchan->desc = to_sunxi_desc(&desc->tx);

	if (!pchan->desc->v_lli->v_lli_next) {
		list_del(&desc->node);
		pchan->done = NULL;
	}

	sunxi_ndma_dump_lli(vchan, pchan->desc->v_lli);

	vchan->irq_type = NDMA_IRQ_PKG;

	irq_val = sdev->cfg->read_irq_enable(sdev);
	irq_val &= ~(NDMA_IRQ_ALL << (pchan->idx * PER_CHAN_OFFSET));
	irq_val |= vchan->irq_type << (pchan->idx * PER_CHAN_OFFSET);
	sdev->cfg->irq_enable(sdev, irq_val);

	writel(pchan->desc->v_lli->src, pchan->base + NDMA_SRC_ADDR_REG);
	writel(pchan->desc->v_lli->dst, pchan->base + NDMA_DST_ADDR_REG);
	writel(pchan->desc->v_lli->len, pchan->base + NDMA_BC_REG);
	writel((pchan->desc->v_lli->cfg | NDMA_START_LOAD), pchan->base + NDMA_CTRL_REG);

	sunxi_ndma_dump_com_regs(sdev);
	sunxi_ndma_dump_chan_regs(sdev, pchan);

	return 0;
}

static void sunxi_ndma_tasklet(unsigned long data)
{
	struct sunxi_ndma_dev *sdev = (struct sunxi_ndma_dev *)data;
	struct sunxi_vchan *vchan;
	struct sunxi_pchan *pchan;
	unsigned int pchan_alloc = 0;
	unsigned int pchan_idx;

	list_for_each_entry(vchan, &sdev->slave.channels, vc.chan.device_node) {
		spin_lock_irq(&vchan->vc.lock);

		pchan = vchan->phy;

		if (pchan && pchan->done) {
			if (sunxi_ndma_start_desc(vchan)) {
				/*
				 * No current txd associated with this channel
				 */
				dev_dbg(sdev->slave.dev, "pchan %u: free\n",
					pchan->idx);

				/* Mark this channel free */
				vchan->phy = NULL;
				pchan->vchan = NULL;
			}
		}
		spin_unlock_irq(&vchan->vc.lock);
	}

	spin_lock_irq(&sdev->lock);
	for (pchan_idx = 0; pchan_idx < sdev->num_pchans; pchan_idx++) {
		pchan = &sdev->pchans[pchan_idx];

		if (pchan->vchan || list_empty(&sdev->pending))
			continue;

		vchan = list_first_entry(&sdev->pending,
					 struct sunxi_vchan, node);

		/* Remove from pending channels */
		list_del_init(&vchan->node);
		pchan_alloc |= BIT(pchan_idx);

		/* Mark this channel allocated */
		pchan->vchan = vchan;
		vchan->phy = pchan;
		dev_dbg(sdev->slave.dev, "pchan %u: alloc vchan %p\n",
			pchan->idx, &vchan->vc);
	}
	spin_unlock_irq(&sdev->lock);

	for (pchan_idx = 0; pchan_idx < sdev->num_pchans; pchan_idx++) {
		if (!(pchan_alloc & BIT(pchan_idx)))
			continue;

		pchan = sdev->pchans + pchan_idx;
		vchan = pchan->vchan;
		if (vchan) {
			spin_lock_irq(&vchan->vc.lock);
			sunxi_ndma_start_desc(vchan);
			spin_unlock_irq(&vchan->vc.lock);
		}
	}
}

static irqreturn_t sunxi_ndma_interrupt(int irq, void *dev_id)
{
	struct sunxi_ndma_dev *sdev = dev_id;
	struct sunxi_vchan *vchan;
	struct sunxi_pchan *pchan;
	int j, ret = IRQ_NONE;
	u32 status;

	status = sdev->cfg->get_irq_status(sdev);
	if (!status)
		goto exit;

	dev_dbg(sdev->slave.dev, "NDMA irq status : 0x%x\n", status);

	sdev->cfg->clear_irq_status(sdev, status);

	for (j = 0; (j < NDMA_IRQ_CHAN_NR) && status; j++) {
		pchan = sdev->pchans + j;
		vchan = pchan->vchan;

		if (vchan && (status & vchan->irq_type)) {
			spin_lock(&vchan->vc.lock);
			if (pchan->desc) {
				struct sunxi_ndma_lli *v_lli = pchan->desc->v_lli;

				if (v_lli->p_lli_next != END_OF_LLI) {
					sunxi_ndma_free_desc(&pchan->desc->vd);
					list_add_tail(&vchan->node, &sdev->pending);
					continue;
				}
				vchan_cookie_complete(&pchan->desc->vd);
				pchan->done = pchan->desc;
			}
			spin_unlock(&vchan->vc.lock);
		}
		status = status >> PER_CHAN_OFFSET;
	}

	if (!atomic_read(&sdev->tasklet_shutdown))
		tasklet_schedule(&sdev->task);

	ret = IRQ_HANDLED;

exit:
	return ret;
}

static int sunxi_set_config(struct sunxi_ndma_dev *sdev,
			struct dma_slave_config *sconfig,
			enum dma_transfer_direction direction,
			u32 *p_cfg)
{
	enum dma_slave_buswidth src_addr_width, dst_addr_width;
	u32 src_maxburst, dst_maxburst;
	s8 src_width, dst_width, src_burst, dst_burst;

	src_addr_width = sconfig->src_addr_width;
	dst_addr_width = sconfig->dst_addr_width;
	src_maxburst = sconfig->src_maxburst;
	dst_maxburst = sconfig->dst_maxburst;

	switch (direction) {
	case DMA_MEM_TO_DEV:
		if (src_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
			src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		src_maxburst = src_maxburst ? src_maxburst : 4;
		break;
	case DMA_DEV_TO_MEM:
		if (dst_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
			dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		dst_maxburst = dst_maxburst ? dst_maxburst : 4;
		break;
	case DMA_MEM_TO_MEM:
		if (dst_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
			dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		dst_maxburst = dst_maxburst ? dst_maxburst : 4;
		break;
	default:
		return -EINVAL;
	}

	if (!(BIT(src_addr_width) & sdev->slave.src_addr_widths))
		return -EINVAL;
	if (!(BIT(dst_addr_width) & sdev->slave.dst_addr_widths))
		return -EINVAL;
	if (!(BIT(src_maxburst) & sdev->cfg->src_burst_lengths))
		return -EINVAL;
	if (!(BIT(dst_maxburst) & sdev->cfg->dst_burst_lengths))
		return -EINVAL;

	/*  Ndma has restrictions on data transmission configuration, check here. */
	if ((dst_maxburst == 4) && (src_maxburst != 4)) {
		if (dst_maxburst * dst_addr_width < src_maxburst * src_addr_width) {
			dev_err(sdev->slave.dev,
				"Failed to config, check dst width.\n");
			return -EINVAL;
		}
	} else if ((dst_maxburst != 4) && (src_maxburst == 4)) {
		if (dst_maxburst * dst_addr_width > src_maxburst * src_addr_width) {
			dev_err(sdev->slave.dev,
				"Failed to config, check src width.\n");
			return -EINVAL;
		}
	} else if ((dst_maxburst == 4) && (src_maxburst == 4)) {
		if (dst_maxburst * dst_addr_width != src_maxburst * src_addr_width) {
			dev_err(sdev->slave.dev,
				"Failed to config, check dst and src width.\n");
			return -EINVAL;
		}
	}

	src_width = convert_buswidth(src_addr_width);
	dst_width = convert_buswidth(dst_addr_width);
	dst_burst = convert_burst(dst_maxburst);
	src_burst = convert_burst(src_maxburst);

	*p_cfg = NDMA_CHAN_CFG_SRC_WIDTH_V100(src_width) |
			NDMA_CHAN_CFG_DST_WIDTH_V100(dst_width);

	sdev->cfg->set_burst_length(p_cfg, src_burst, dst_burst);

	return 0;
}

static void sunxi_ndma_free_chan_resources(struct dma_chan *chan)
{
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(chan->device);
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);
	unsigned long flags;

	spin_lock_irqsave(&sdev->lock, flags);
	list_del_init(&vchan->node);
	spin_unlock_irqrestore(&sdev->lock, flags);

	vchan_free_chan_resources(&vchan->vc);
}

static enum dma_status sunxi_ndma_tx_status(struct dma_chan *chan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *state)
{
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);
	struct sunxi_pchan *pchan = vchan->phy;
	struct sunxi_ndma_lli *lli;
	struct virt_dma_desc *vd;
	struct sunxi_desc *txd;
	enum dma_status ret;
	unsigned long flags;
	size_t bytes = 0;

	ret = dma_cookie_status(chan, cookie, state);
	if (ret == DMA_COMPLETE || !state)
		return ret;

	spin_lock_irqsave(&vchan->vc.lock, flags);

	vd = vchan_find_desc(&vchan->vc, cookie);
	txd = to_sunxi_desc(&vd->tx);

	if (vd) {
		for (lli = txd->v_lli; lli != NULL; lli = lli->v_lli_next)
			bytes += lli->len;
	} else if (!pchan || !pchan->desc) {
		bytes = 0;
	} else {
		dev_warn(chan2dev(chan), "NDMA is transferring data!");
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	dma_set_residue(state, bytes);

	return ret;
}

static void sunxi_ndma_issue_pending(struct dma_chan *chan)
{
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(chan->device);
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);
	unsigned long flags;

	spin_lock_irqsave(&vchan->vc.lock, flags);

	if (vchan_issue_pending(&vchan->vc)) {
		spin_lock(&sdev->lock);

		if (!vchan->phy && list_empty(&vchan->node)) {
			list_add_tail(&vchan->node, &sdev->pending);
			tasklet_schedule(&sdev->task);
			dev_dbg(chan2dev(chan), "vchan %p: issued\n",
				&vchan->vc);
		}

		spin_unlock(&sdev->lock);
	} else {
		dev_dbg(chan2dev(chan), "vchan %p: nothing to issue\n",
			&vchan->vc);
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);
}

static struct dma_async_tx_descriptor *sunxi_ndma_prep_dma_memcpy(
		struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(chan->device);
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);
	struct sunxi_ndma_lli *v_lli;
	struct sunxi_desc *txd;
	dma_addr_t p_lli;
	s8 burst, width;

	dev_dbg(chan2dev(chan),
		"%s; chan: %d, dest: %pad, src: %pad, len: %zu. flags: 0x%08lx\n",
		__func__, vchan->vc.chan.chan_id, &dest, &src, len, flags);

	if (!len || (len > NDMA_DATA_MAX_LEN)) {
		dev_err(chan2dev(chan),
			"NDMA not support data len of 0 or greater than 128KB!");
		return NULL;
	}

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;

	v_lli = dma_pool_alloc(sdev->pool, GFP_NOWAIT, &p_lli);
	if (!v_lli) {
		dev_err(sdev->slave.dev, "Failed to alloc lli memory\n");
		goto err_txd_free;
	}
	v_lli->this_phy = p_lli;

	v_lli->src = src;
	v_lli->dst = dest;
	v_lli->len = len;

	burst = convert_burst(4);
	width = convert_buswidth(DMA_SLAVE_BUSWIDTH_4_BYTES);

	v_lli->cfg = NDMA_CHAN_CFG_SRC_WIDTH_V100(width) |
			NDMA_CHAN_CFG_DST_WIDTH_V100(width);

	sdev->cfg->set_burst_length(&v_lli->cfg, burst, burst);
	sdev->cfg->set_drq(&v_lli->cfg, NDMA_DRQ_SDRAM, NDMA_DRQ_SDRAM);
	sdev->cfg->set_mode(&v_lli->cfg, INCREMENT_MODE, INCREMENT_MODE);
	sdev->cfg->set_wait_cycle(&v_lli->cfg, NDMA_WAIT_CYCLES(8));

	sunxi_ndma_lli_add(NULL, v_lli, p_lli, txd);

	sunxi_ndma_dump_lli(vchan, v_lli);

	return vchan_tx_prep(&vchan->vc, &txd->vd, flags);

err_txd_free:
	kfree(txd);
	return NULL;
}

static struct dma_async_tx_descriptor *sunxi_ndma_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction dir,
		unsigned long flags, void *context)
{
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(chan->device);
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);
	struct dma_slave_config *sconfig = &vchan->cfg;
	struct sunxi_ndma_lli *v_lli, *prev = NULL;
	struct sunxi_desc *txd;
	struct scatterlist *sg;
	dma_addr_t p_lli;
	u32 lli_cfg;
	int i, ret;

	if (!sgl)
		return NULL;

	ret = sunxi_set_config(sdev, sconfig, dir, &lli_cfg);
	if (ret) {
		dev_err(chan2dev(chan), "Invalid NDMA configuration\n");
		return NULL;
	}

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		v_lli = dma_pool_alloc(sdev->pool, GFP_NOWAIT, &p_lli);
		if (!v_lli)
			goto err_lli_free;

		v_lli->this_phy = p_lli;
		v_lli->len = sg_dma_len(sg);

		if (dir == DMA_MEM_TO_DEV) {
			v_lli->src = sg_dma_address(sg);
			v_lli->dst = sconfig->dst_addr;
			v_lli->cfg = lli_cfg;
			sdev->cfg->set_drq(&v_lli->cfg, NDMA_DRQ_SDRAM, vchan->port);
			sdev->cfg->set_mode(&v_lli->cfg, INCREMENT_MODE, NOCHANGE_MODE);

			dev_dbg(chan2dev(chan),
				"%s; chan: %d, dest: %pad, src: %pad, len: %u. flags: 0x%08lx\n",
				__func__, vchan->vc.chan.chan_id,
				&sconfig->dst_addr, &sg_dma_address(sg),
				sg_dma_len(sg), flags);

		} else if (dir == DMA_MEM_TO_MEM) {
			v_lli->src = sg_dma_address(sg);
			v_lli->dst = sconfig->dst_addr;
			v_lli->cfg = lli_cfg;

			sdev->cfg->set_drq(&v_lli->cfg, NDMA_DRQ_SDRAM, NDMA_DRQ_SDRAM);
			sdev->cfg->set_mode(&v_lli->cfg, INCREMENT_MODE, INCREMENT_MODE);

			dev_dbg(chan2dev(chan),
				"%s; chan: %d, dest: %pad, src: %pad, len: %u. flags: 0x%08lx\n",
				__func__, vchan->vc.chan.chan_id,
				&sconfig->dst_addr, &sg_dma_address(sg),
				sg_dma_len(sg), flags);
		} else {
			v_lli->src = sconfig->src_addr;
			v_lli->dst = sg_dma_address(sg);
			v_lli->cfg = lli_cfg;
			sdev->cfg->set_drq(&v_lli->cfg, vchan->port, NDMA_DRQ_SDRAM);
			sdev->cfg->set_mode(&v_lli->cfg, NOCHANGE_MODE, INCREMENT_MODE);

			dev_dbg(chan2dev(chan),
				"%s; chan: %d, dest: %pad, src: %pad, len: %u. flags: 0x%08lx\n",
				__func__, vchan->vc.chan.chan_id,
				&sg_dma_address(sg), &sconfig->src_addr,
				sg_dma_len(sg), flags);
		}

		sdev->cfg->set_wait_cycle(&v_lli->cfg, NDMA_WAIT_CYCLES(8));

		prev = sunxi_ndma_lli_add(prev, v_lli, p_lli, txd);
	}

	dev_dbg(chan2dev(chan), "First: %pad\n", &txd->p_lli);
	for (prev = txd->v_lli; prev; prev = prev->v_lli_next)
		sunxi_ndma_dump_lli(vchan, prev);

	return vchan_tx_prep(&vchan->vc, &txd->vd, flags);

err_lli_free:
	for (prev = txd->v_lli; prev; prev = prev->v_lli_next)
		dma_pool_free(sdev->pool, prev, prev->this_phy);
	kfree(txd);
	return NULL;
}

static struct dma_async_tx_descriptor *sunxi_ndma_prep_dma_cyclic(
					struct dma_chan *chan,
					dma_addr_t buf_addr,
					size_t buf_len,
					size_t period_len,
					enum dma_transfer_direction dir,
					unsigned long flags)
{
	dev_err(chan2dev(chan), "NDMA does not support cyclic mode!\n");

	return NULL;
}

static int sunxi_ndma_config(struct dma_chan *chan,
			    struct dma_slave_config *config)
{
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);

	memcpy(&vchan->cfg, config, sizeof(*config));

	return 0;
}

static int sunxi_ndma_pause(struct dma_chan *chan)
{
	dev_err(chan2dev(chan), "NDMA does not support pause mode!\n");

	return 0;
}

static int sunxi_ndma_resume(struct dma_chan *chan)
{
	dev_err(chan2dev(chan), "NDMA does not support resume mode!\n");

	return 0;
}

static inline void sunxi_ndma_free(struct sunxi_ndma_dev *sdev)
{
	int i;

	for (i = 0; i < sdev->num_vchans; i++) {
		struct sunxi_vchan *vchan = &sdev->vchans[i];

		list_del(&vchan->vc.chan.device_node);
		tasklet_kill(&vchan->vc.task);
	}
}

static int sunxi_ndma_terminate_all(struct dma_chan *chan)
{
	struct sunxi_ndma_dev *sdev = to_sunxi_ndma_dev(chan->device);
	struct sunxi_vchan *vchan = to_sunxi_vchan(chan);
	struct sunxi_pchan *pchan = vchan->phy;
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock(&sdev->lock);
	list_del_init(&vchan->node);
	spin_unlock(&sdev->lock);

	spin_lock_irqsave(&vchan->vc.lock, flags);

	vchan_get_all_descriptors(&vchan->vc, &head);

	if (pchan) {
		vchan->phy = NULL;
		pchan->vchan = NULL;
		pchan->desc = NULL;
		pchan->done = NULL;
	}

	spin_unlock_irqrestore(&vchan->vc.lock, flags);

	vchan_dma_desc_free_list(&vchan->vc, &head);

	return 0;
}

static void sunxi_ndma_synchronize(struct dma_chan *chan)
{
	struct sunxi_vchan *c = to_sunxi_ndma_chan(chan);

	vchan_synchronize(&c->vc);
}

static struct sunxi_ndma_config sunxi_ndma_v100 = {
	//.clock_autogate_enable = sunxi_enable_clock_autogate_v100,
	.set_burst_length  = set_burst_length_v100,
	.set_drq           = set_drq_v100,
	.set_mode          = set_mode_v100,
	.irq_enable 	   = irq_enable_v100,
	.get_irq_status    = get_irq_status_v100,
	.read_irq_enable   = read_irq_enable_v100,
	.clear_irq_status  = clear_irq_status_v100,
	.set_wait_cycle	   = set_dma_wait_cycle_v100,
	.src_burst_lengths = BIT(1) | BIT(4),
	.dst_burst_lengths = BIT(1) | BIT(4),
	.src_addr_widths   = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
			     BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
			     BIT(DMA_SLAVE_BUSWIDTH_4_BYTES),
	.dst_addr_widths   = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
			     BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
			     BIT(DMA_SLAVE_BUSWIDTH_4_BYTES),
	.has_mbus_clk = false,
};

static const struct of_device_id sunxi_ndma_dt_ids[] = {
	{ .compatible = "allwinner,ndma-v100", .data = &sunxi_ndma_v100 },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_ndma_dt_ids);

static int sunxi_ndma_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_ndma_dev *sdev;
	struct resource *res;
	int ret, i;

	sdev = devm_kzalloc(&pdev->dev, sizeof(*sdev), GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sdev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sdev->base))
		return PTR_ERR(sdev->base);

	sdev->cfg = of_device_get_match_data(&pdev->dev);
	if (!sdev->cfg)
		return -ENODEV;

	ret = of_property_read_u32(np, "dma-channels", &sdev->num_pchans);
	if (ret && !sdev->num_pchans) {
		dev_err(&pdev->dev, "Can't get dma-channels.\n");
		return ret;
	}

	ret = of_property_read_u32(np, "dma-requests", &sdev->max_request);
	if (ret && !sdev->max_request) {
		dev_info(&pdev->dev, "Missing dma-requests, using %u.\n",
			 NDMA_CHAN_MAX_DRQ);
		sdev->max_request = NDMA_CHAN_MAX_DRQ;
	}

	sdev->irq = platform_get_irq(pdev, 0);
	if (sdev->irq < 0)
		return sdev->irq;

	sdev->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(sdev->clk)) {
		dev_err(&pdev->dev, "No clock specified\n");
		return PTR_ERR(sdev->clk);
	}

	if (sdev->cfg->has_mbus_clk) {
		sdev->clk_mbus = devm_clk_get(&pdev->dev, "mbus");
		if (IS_ERR(sdev->clk_mbus)) {
			dev_err(&pdev->dev, "No mbus clock specified\n");
			return PTR_ERR(sdev->clk_mbus);
		}
	}

	sdev->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(sdev->rstc)) {
		dev_err(&pdev->dev, "No reset controller specified\n");
		return PTR_ERR(sdev->rstc);
	}

	sdev->pool = dmam_pool_create(dev_name(&pdev->dev), &pdev->dev,
				     sizeof(struct sunxi_ndma_lli), 4, 0);
	if (!sdev->pool) {
		dev_err(&pdev->dev, "No memory for descriptors dma pool\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, sdev);
	INIT_LIST_HEAD(&sdev->pending);
	spin_lock_init(&sdev->lock);

	dma_cap_set(DMA_PRIVATE, sdev->slave.cap_mask);
	dma_cap_set(DMA_MEMCPY, sdev->slave.cap_mask);
	dma_cap_set(DMA_SLAVE, sdev->slave.cap_mask);
	dma_cap_set(DMA_CYCLIC, sdev->slave.cap_mask);

	INIT_LIST_HEAD(&sdev->slave.channels);

	sdev->slave.device_free_chan_resources	= sunxi_ndma_free_chan_resources;
	sdev->slave.device_tx_status		= sunxi_ndma_tx_status;
	sdev->slave.device_issue_pending	= sunxi_ndma_issue_pending;
	sdev->slave.device_prep_dma_memcpy	= sunxi_ndma_prep_dma_memcpy;
	sdev->slave.device_prep_slave_sg	= sunxi_ndma_prep_slave_sg;
	sdev->slave.device_prep_dma_cyclic	= sunxi_ndma_prep_dma_cyclic;
	sdev->slave.copy_align			= DMAENGINE_ALIGN_4_BYTES;
	sdev->slave.device_config		= sunxi_ndma_config;
	sdev->slave.device_pause		= sunxi_ndma_pause;
	sdev->slave.device_resume		= sunxi_ndma_resume;
	sdev->slave.device_terminate_all	= sunxi_ndma_terminate_all;
	sdev->slave.device_synchronize		= sunxi_ndma_synchronize;
	sdev->slave.src_addr_widths		= sdev->cfg->src_addr_widths;
	sdev->slave.dst_addr_widths		= sdev->cfg->dst_addr_widths;
	sdev->slave.directions			= BIT(DMA_DEV_TO_MEM) |
						  BIT(DMA_MEM_TO_DEV);
	sdev->slave.residue_granularity		= DMA_RESIDUE_GRANULARITY_BURST;
	sdev->slave.dev = &pdev->dev;

	if (!sdev->num_vchans)
		sdev->num_vchans = 2 * (sdev->max_request + 1);

	sdev->pchans = devm_kcalloc(&pdev->dev, sdev->num_pchans,
				   sizeof(struct sunxi_pchan), GFP_KERNEL);
	if (!sdev->pchans)
		return -ENOMEM;

	sdev->vchans = devm_kcalloc(&pdev->dev, sdev->num_vchans,
				   sizeof(struct sunxi_vchan), GFP_KERNEL);
	if (!sdev->vchans)
		return -ENOMEM;

	tasklet_init(&sdev->task, sunxi_ndma_tasklet, (unsigned long)sdev);

	for (i = 0; i < sdev->num_pchans; i++) {
		struct sunxi_pchan *pchan = &sdev->pchans[i];

		pchan->idx = i;
		pchan->base = sdev->base + 0x100 + i * 0x20;
	}

	for (i = 0; i < sdev->num_vchans; i++) {
		struct sunxi_vchan *vchan = &sdev->vchans[i];

		INIT_LIST_HEAD(&vchan->node);
		vchan->vc.desc_free = sunxi_ndma_free_desc;
		vchan_init(&vchan->vc, &sdev->slave);
	}

	ret = reset_control_assert(sdev->rstc);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't assert the device from reset\n");
		goto err_chan_free;
	}

	usleep_range(20, 25); /* ensure dma controller is reset */

	ret = reset_control_deassert(sdev->rstc);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't deassert the device from reset\n");
		goto err_chan_free;
	}

	ret = clk_prepare_enable(sdev->clk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable the clock\n");
		goto err_reset_assert;
	}

	if (sdev->cfg->has_mbus_clk) {
		ret = clk_prepare_enable(sdev->clk_mbus);
		if (ret) {
			dev_err(&pdev->dev, "Couldn't enable mbus clock\n");
			goto err_clk_disable;
		}
	}

	ret = devm_request_irq(&pdev->dev, sdev->irq, sunxi_ndma_interrupt, 0,
			       dev_name(&pdev->dev), sdev);
	if (ret) {
		dev_err(&pdev->dev, "Cannot request IRQ\n");
		goto err_mbus_clk_disable;
	}

	ret = dma_async_device_register(&sdev->slave);
	if (ret) {
		dev_warn(&pdev->dev, "Failed to register DMA engine device\n");
		goto err_irq_disable;
	}

	ret = of_dma_controller_register(pdev->dev.of_node, sunxi_ndma_of_xlate,
					 sdev);
	if (ret) {
		dev_err(&pdev->dev, "of_dma_controller_register failed\n");
		goto err_dma_unregister;
	}

	if (sdev->cfg->clock_autogate_enable)
		sdev->cfg->clock_autogate_enable(sdev);

	dev_info(&pdev->dev, "sunxi ndma probed\n");

	return 0;

err_dma_unregister:
	dma_async_device_unregister(&sdev->slave);
err_irq_disable:
	sunxi_ndma_kill_tasklet(sdev);
err_mbus_clk_disable:
	if (sdev->cfg->has_mbus_clk)
		clk_disable_unprepare(sdev->clk_mbus);
err_clk_disable:
	clk_disable_unprepare(sdev->clk);
err_reset_assert:
	reset_control_assert(sdev->rstc);
err_chan_free:
	sunxi_ndma_free(sdev);

	return ret;
}

static int sunxi_ndma_remove(struct platform_device *pdev)
{
	struct sunxi_ndma_dev *sdev = platform_get_drvdata(pdev);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&sdev->slave);

	sunxi_ndma_kill_tasklet(sdev);

	if (sdev->cfg->has_mbus_clk)
		clk_disable_unprepare(sdev->clk_mbus);

	clk_disable_unprepare(sdev->clk);
	reset_control_assert(sdev->rstc);

	sunxi_ndma_free(sdev);

	return 0;
}

static struct platform_driver sunxi_ndma_driver = {
	.probe		= sunxi_ndma_probe,
	.remove		= sunxi_ndma_remove,
	.driver = {
		.name		= "sunxi-ndma",
		.of_match_table	= sunxi_ndma_dt_ids,
	},
};

static int __init sunxi_ndma_init(void)
{
	return platform_driver_register(&sunxi_ndma_driver);
}
subsys_initcall(sunxi_ndma_init);

static void __exit sunxi_ndma_exit(void)
{
	platform_driver_unregister(&sunxi_ndma_driver);
}
module_exit(sunxi_ndma_exit);

MODULE_DESCRIPTION("Allwinner NDMA Controller Driver");
MODULE_AUTHOR("<songjundong@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.2");
