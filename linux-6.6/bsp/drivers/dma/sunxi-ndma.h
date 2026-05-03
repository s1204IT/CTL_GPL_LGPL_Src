/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013-2023 Allwinner Tech Co., Ltd
 */
#ifndef SUNXI_NDMA_H
#define SUNXI_NDMA_H

#include <linux/dmaengine.h>
#include <linux/interrupt.h>

#include "dmaengine.h"

/* Maximum data length of one nDMA transfer */
#define NDMA_DATA_MAX_LEN	(128 * 1024U)

#define NDMA_DRQ_SDRAM		0

#define NDMA_CHAN_MAX_DRQ	0x1f

#define PER_CHAN_OFFSET		0x2

#define NDMA_IRQ_CHAN_NR	8

#define NDMA_IRQ_EN		0x00
#define NDMA_IRQ_HALF		BIT(0)
#define NDMA_IRQ_PKG		BIT(1)
#define NDMA_IRQ_ALL		(BIT(0) | BIT(1))

#define NDMA_PEND_STAT		0x04
#define NDMA_PEND_HALF		BIT(0)
#define NDMA_PEND_PKG		BIT(1)
#define NDMA_PEND_ALL		(BIT(0) | BIT(1))

#define NDMA_AUTO_GAT		0x08

#define NDMA_AUTOGAT_LEN	0x0c

#define NDMA_START_LOAD		BIT(31)
#define NDMA_BUSY_STAT		BIT(30)
#define NDMA_CTRL_REG		0x00

#define NDMA_SRC_ADDR_REG	0x04

#define NDMA_DST_ADDR_REG	0x08

#define NDMA_BC_REG		0x0C

#define END_OF_LLI		0xdeadbeaf

/*
 *index : burst length
 *  00  :  1
 *  01  :  4
 * */
#define NDMA_CHAN_CFG_SRC_BURST_V100(x)		(((x) & 0x1) << 8)
#define NDMA_CHAN_CFG_DST_BURST_V100(x)		(((x) & 0x1) << 23)

#define NDMA_CHAN_MAX_DRQ_V100			0x3f
#define NDMA_CHAN_CFG_SRC_DRQ_V100(x)		((x) & NDMA_CHAN_MAX_DRQ_V100)
#define NDMA_CHAN_CFG_DST_DRQ_V100(x)		(((x) & NDMA_CHAN_MAX_DRQ_V100) << 15)

#define INCREMENT_MODE				0
#define NOCHANGE_MODE				1
#define NDMA_CHAN_CFG_SRC_MODE_V100(x)		(((x) & 0x1) << 6)
#define NDMA_CHAN_CFG_DST_MODE_V100(x)		(((x) & 0x1) << 21)

#define NDMA_WAIT_CYCLES(x)			(ilog2(x))
#define NDMA_CHAN_CFG_WAIT_CYCLES_V100(x)	(((x) & 0x7) << 26)
/*
 *index : data width
 *  00  :  8-bit
 *  01  :  16-bit
 *  10  :  32-bit
 *  11  :  /
 * */
#define NDMA_CHAN_CFG_SRC_WIDTH_V100(x)		(((x) & 0x3) << 9)
#define NDMA_CHAN_CFG_DST_WIDTH_V100(x)		(((x) & 0x3) << 25)

/* forward declaration */
struct sunxi_ndma_dev;

/*
 * Hardware channels / ports representation
 *
 * The hardware is used in several SoCs, with differing numbers
 * of channels and endpoints. This structure ties those numbers
 * to a certain compatible string.
 */
struct sunxi_ndma_config {
	u32 nr_max_channels;
	u32 nr_max_requests;
	u32 nr_max_vchans;
	void (*clock_autogate_enable)(struct sunxi_ndma_dev *);
	void (*set_burst_length)(u32 *p_cfg, s8 src_burst, s8 dst_burst);
	void (*set_drq)(u32 *p_cfg, s8 src_drq, s8 dst_drq);
	void (*set_mode)(u32 *p_cfg, s8 src_mode, s8 dst_mode);
	void (*set_wait_cycle)(u32 *p_cfg, s8 cycles);
	bool has_mbus_clk;
	u32 src_burst_lengths;
	u32 dst_burst_lengths;
	u32 src_addr_widths;
	u32 dst_addr_widths;
	void (*irq_enable)(struct sunxi_ndma_dev *sdev, u32 irq_val);
	u32 (*read_irq_enable)(struct sunxi_ndma_dev *sdev);
	u32 (*get_irq_status)(struct sunxi_ndma_dev *sdev);
	void (*clear_irq_status)(struct sunxi_ndma_dev *sdev, u32 status);
};

struct sunxi_ndma_lli {
	u32			cfg;
	u32			src;
	u32			dst;
	u32			len;
	u32			p_lli_next;
	struct sunxi_ndma_lli	*v_lli_next;
	dma_addr_t	        this_phy;
};

struct sunxi_desc {
	struct virt_dma_desc	vd;
	dma_addr_t		p_lli;
	struct sunxi_ndma_lli	*v_lli;
};

struct sunxi_pchan {
	u32			idx;
	void __iomem		*base;
	struct sunxi_vchan	*vchan;
	struct sunxi_desc	*desc;
	struct sunxi_desc	*done;
};

struct sunxi_vchan {
	struct virt_dma_chan	vc;
	struct list_head	node;
	struct dma_slave_config	cfg;
	struct sunxi_pchan	*phy;
	u8			port;
	u8			irq_type;
	bool			cyclic;
};

struct sunxi_ndma_dev {
	struct dma_device	slave;
	void __iomem		*base;
	struct clk		*clk;
	struct clk		*clk_mbus;
	int			irq;
	spinlock_t		lock;
	struct reset_control	*rstc;
	struct tasklet_struct	task;
	atomic_t		tasklet_shutdown;
	struct list_head	pending;
	struct dma_pool		*pool;
	struct sunxi_pchan	*pchans;
	struct sunxi_vchan	*vchans;
	const struct sunxi_ndma_config *cfg;
	u32			num_pchans;
	u32			num_vchans;
	u32			max_request;
};



static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static inline struct sunxi_ndma_dev *to_sunxi_ndma_dev(struct dma_device *d)
{
	return container_of(d, struct sunxi_ndma_dev, slave);
}

static inline struct sunxi_vchan *to_sunxi_vchan(struct dma_chan *chan)
{
	return container_of(chan, struct sunxi_vchan, vc.chan);
}

static inline struct sunxi_desc *
to_sunxi_desc(struct dma_async_tx_descriptor *tx)
{
	return container_of(tx, struct sunxi_desc, vd.tx);
}

static inline struct sunxi_vchan *to_sunxi_ndma_chan(struct dma_chan *c)
{
	return container_of(c, struct sunxi_vchan, vc.chan);
}

#endif
