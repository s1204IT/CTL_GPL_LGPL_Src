// SPDX-License-Identifier: GPL-2.0
/*
 * ksc/ksc.c
 *
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

#include "ksc.h"
#include "ksc_mem.h"
#include <linux/iopoll.h>
static bool judge_ksc_smooth_disp;
static bool is_ksc_smooth_disp;
static unsigned int ksc_pq_sharpen_level = 1;

char *ksc_boot_para_parse_str(struct ksc_device *p_ksc, char *name)
{
	const char *str;
	if (!of_property_read_string(p_ksc->p_device->of_node, name, &str))
		return str;
	dev_err(p_ksc->p_device, "of_property_read_string ksc.%s fail\n", name);
	return NULL;
}

static void ksc_smooth_disp_judge(struct ksc_device *p_ksc)
{
	if (ksc_boot_para_parse_str(p_ksc, KSC_REV_MEM) != NULL) {
		is_ksc_smooth_disp = true;
		p_ksc->ksc_para.is_first_frame = false;
	} else
		is_ksc_smooth_disp = false;
	dev_info(p_ksc->p_device, "ksc smooth display is %s\n",
				(is_ksc_smooth_disp == true)?"turn on":"turn off");
	judge_ksc_smooth_disp = true;
}

static ssize_t ksc_show_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	unsigned int i = 0;
	void *reg_base;

	struct ksc_device *p_ksc =
		container_of(dev, struct ksc_device, attr_device);
	if (!p_ksc) {
		dev_err(dev, "Null ksc_device\n");
		goto OUT;
	}

	count += sprintf(buf + count, "Enabled:%d irq:%d irq_no:%u be err:%d fe err:%d svp2ksc_dmt:%d\n",
			 p_ksc->enabled, p_ksc->irq_cnt, p_ksc->irq_no,
			 p_ksc->be_err_cnt, p_ksc->fe_err_cnt, p_ksc->svp2ksc_dmt_err_cnt);
	if (p_ksc->enabled == true) {
		count += sprintf(buf + count, "=========== ksc reg value ===========\n");
		reg_base = (void * __force)p_ksc->p_reg;
		for (i = 0; i < 60; i += 4) {
			count += sprintf(buf + count, "0x%.8lx: 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n",
					 (unsigned long)reg_base, readl(reg_base), readl(reg_base + 4),
					 readl(reg_base + 8), readl(reg_base + 12));
			reg_base += 0x10;
		}
		count += sprintf(buf + count, "=========== ksc clk info ===========\n");

		for (i = 0; i < MAX_CLK_NUM; ++i) {
			if (!IS_ERR_OR_NULL(p_ksc->clks[i])) {
				count += sprintf(buf + count, "clk%d rate:%lu\n", i, clk_get_rate(p_ksc->clks[i]));
			}
			if (!IS_ERR_OR_NULL(p_ksc->rsts[i])) {
				count += sprintf(buf + count, "Reset%d state:%d\n", i, reset_control_status(p_ksc->rsts[i]));
			}
		}
	}

OUT:
	return count;
}

static DEVICE_ATTR(info, 0660, ksc_show_info, NULL);

static ssize_t ksc_antialias_para_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ksc_device *p_ksc = NULL;
	struct sunxi_ksc_para *para = NULL;

	p_ksc = container_of(dev, struct ksc_device, attr_device);
	if (!p_ksc) {
		dev_err(dev, "Null ksc_device\n");
		return -EINVAL;
	}
	para = p_ksc->get_ksc_para(p_ksc);
	if (!para) {
		dev_err(dev, "Null sunxi_ksc_para\n");
		p_ksc = NULL;
		return -EINVAL;
	}

	return sprintf(buf, "%d\n", para->pq_para.antialias_str);
}

static ssize_t ksc_antialias_para_store(struct device *dev,
				 struct device_attribute *attr, const char *buf, size_t count)
{
	int err, ret;
	unsigned int val;
	struct ksc_device *p_ksc = NULL;
	struct sunxi_ksc_para *para = NULL;

	p_ksc = container_of(dev, struct ksc_device, attr_device);
	if (!p_ksc) {
		dev_err(dev, "Null ksc_device\n");
		goto OUT1;
	}

	para = p_ksc->get_ksc_para(p_ksc);
	if (!para) {
		dev_err(dev, "Null sunxi_ksc_para\n");
		goto OUT2;
	}

	if (para->ksc_en == 0) {
		dev_err(dev, "ksc online mode disable !!!\n");
		goto OUT3;
	}

	/* get sys param*/
	err = kstrtou32(buf, 10, &val);
	if (err) {
		dev_err(dev, "Invalid ksc antialias pq param\n");
		goto OUT3;
	}

	para->pq_para.post_proc_en = 1;
	if (val < 0 || val > 128) {
		dev_err(dev, "Invalid ksc antialias pq param, antialias param range should be in 0~128 \n");
		goto OUT3;
	}
	para->pq_para.antialias_str = val;
	dev_info(dev, "antialias strength  = %u\n", para->pq_para.antialias_str);
	ret = p_ksc->set_ksc_para(p_ksc, para);
	if (ret) {
		dev_err(dev, "ksc_set_para error\n");
		goto OUT3;
	}

OUT3:
	para = NULL;
OUT2:
	p_ksc = NULL;
OUT1:
	return count;
}

static DEVICE_ATTR(antialias_str, 0660, ksc_antialias_para_show, ksc_antialias_para_store);

static ssize_t ksc_sharpen_para_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	switch (ksc_pq_sharpen_level) {
	case 0:
		return sprintf(buf, "sharpen_level = %d, Image sharpening NORMAL 1/2x\n", ksc_pq_sharpen_level);
	case 1:
		return sprintf(buf, "sharpen_level = %d, Image sharpening NORMAL 1/2x\n", ksc_pq_sharpen_level);
	case 2:
		return sprintf(buf, "sharpen_level = %d, Image sharpening NORMAL 1.5x\n", ksc_pq_sharpen_level);
	case 3:
		return sprintf(buf, "sharpen_level = %d, Image sharpening NORMAL 2x\n", ksc_pq_sharpen_level);
	case 4:
		return sprintf(buf, "sharpen_level = %d, Image sharpening NORMAL MAX\n", ksc_pq_sharpen_level);
	default:
		break;
	}
	return NULL;
}

static int ksc_set_sharpen_para(struct ksc_device *p_ksc, struct sunxi_ksc_para *para)
{
	struct ksc_pq_para *pq = NULL;

	pq = &para->pq_para;
	if (!pq) {
		dev_err(p_ksc->p_device, "Null ksc_pq_para\n");
		return -1;
	}

	switch (ksc_pq_sharpen_level) {
	case 0:
		pq->wht_clip_val = 24;
		pq->blk_clip_val = 32;
		pq->wht_clip_rat  = 16;
		pq->blk_clip_rat  = 16;
		pq->wht_gain      = 8;
		pq->blk_gain      = 16;
		pq->res_peak_rat  = 12;
		pq->res_gain      = 12;
		pq->res_max_clp   = 32;
		pq->res_min_clp   =  2;
		pq->res_str       = 46;
		dev_info(p_ksc->p_device, "Image sharpening NORMAL 1/2x\n");
		break;
	case 1:
		pq->wht_clip_val = 48;
		pq->blk_clip_val = 64;
		pq->wht_clip_rat  = 32;
		pq->blk_clip_rat  = 32;
		pq->wht_gain      = 16;
		pq->blk_gain      = 32;
		pq->res_peak_rat  = 24;
		pq->res_gain      = 16;
		pq->res_max_clp   = 64;
		pq->res_min_clp   =  4;
		pq->res_str       = 92;
		dev_info(p_ksc->p_device, "Image sharpening NORMAL 1x\n");
		break;
	case 2:
		pq->wht_clip_val = 48;
		pq->blk_clip_val = 64;
		pq->wht_clip_rat  = 32;
		pq->blk_clip_rat  = 32;
		pq->wht_gain      = 24;
		pq->blk_gain      = 48;
		pq->res_peak_rat  = 24;
		pq->res_gain      = 24;
		pq->res_max_clp   = 64;
		pq->res_min_clp   =  4;
		pq->res_str       = 92;
		dev_info(p_ksc->p_device, "Image sharpening NORMAL 1.5x\n");
		break;
	case 3:
		pq->wht_clip_val = 48;
		pq->blk_clip_val = 64;
		pq->wht_clip_rat  = 32;
		pq->blk_clip_rat  = 32;
		pq->wht_gain      = 32;
		pq->blk_gain      = 64;
		pq->res_peak_rat  = 24;
		pq->res_gain      = 32;
		pq->res_max_clp   = 64;
		pq->res_min_clp   =  4;
		pq->res_str       = 92;
		dev_info(p_ksc->p_device, "Image sharpening NORMAL 2x\n");
		break;
	case 4:
		pq->wht_clip_val = 48;
		pq->blk_clip_val = 64;
		pq->wht_clip_rat  = 32;
		pq->blk_clip_rat  = 32;
		pq->wht_gain      = 128;
		pq->blk_gain      = 128;
		pq->res_peak_rat  = 24;
		pq->res_gain      = 64;
		pq->res_max_clp   = 64;
		pq->res_min_clp   =  4;
		pq->res_str       = 92;
		dev_info(p_ksc->p_device, "Image sharpening NORMAL MAX\n");
		break;
	default:
		dev_err(p_ksc->p_device, "Invalid ksc sharpen pq param, sharpen param range should be in level 0~4 \n");
		return -1;
	}
	return 0;
}

static ssize_t ksc_sharpen_para_store(struct device *dev,
				 struct device_attribute *attr, const char *buf, size_t count)
{
	int err, ret;
	unsigned int val;
	struct ksc_device *p_ksc = NULL;
	struct sunxi_ksc_para *para = NULL;
	struct ksc_pq_para *pq = NULL;

	p_ksc = container_of(dev, struct ksc_device, attr_device);
	if (!p_ksc) {
		dev_err(dev, "Null ksc_device\n");
		goto OUT1;
	}

	para = p_ksc->get_ksc_para(p_ksc);
	if (!para) {
		dev_err(dev, "Null sunxi_ksc_para\n");
		goto OUT2;
	}

	pq = &para->pq_para;
	if (!pq) {
		dev_err(dev, "Null ksc_pq_para\n");
		goto OUT3;
	}

	if (para->ksc_en == 0) {
		dev_err(dev, "ksc online mode disable !!!\n");
		goto OUT4;
	}

	/* get sys param*/
	err = kstrtou32(buf, 10, &val);
	if (err) {
		dev_err(dev, "Invalid ksc sharpen pq param\n");
		goto OUT4;
	}

	pq->post_proc_en = 1;
	ksc_pq_sharpen_level = val;
	ret = ksc_set_sharpen_para(p_ksc, para);
	if (ret) {
		dev_err(dev, "ksc_set_sharpen_para error\n");
		goto OUT4;
	}

	ret = p_ksc->set_ksc_para(p_ksc, para);
	if (ret) {
		dev_err(dev, "ksc_set_para error\n");
		goto OUT4;
	}

OUT4:
	pq = NULL;
OUT3:
	para = NULL;
OUT2:
	p_ksc = NULL;
OUT1:
	return count;
}

static DEVICE_ATTR(sharpen_level, 0660, ksc_sharpen_para_show, ksc_sharpen_para_store);

static struct attribute *ksc_attributes[] = {
	&dev_attr_info.attr,
	&dev_attr_antialias_str.attr,
	&dev_attr_sharpen_level.attr,
	NULL,
};

static irqreturn_t ksc_irq_handler(int irq, void *parg)
{
	struct ksc_device *p_ksc = (struct ksc_device *)parg;
	int state = 0;

	state = ksc_reg_irq_query(p_ksc->p_reg);
	if (state & KSC_IRQ_BE_FINISH) {
		p_ksc->offline_finish_flag = true;
		wake_up(&p_ksc->event_wait);
	}

	if (state & KSC_IRQ_FE_ERR) {
		p_ksc->fe_err_cnt++;
	}

	if (state & SVP2KSC_DMT_ERR) {
		p_ksc->svp2ksc_dmt_err_cnt++;
	}


	if (state & KSC_IRQ_BE_ERR) {
		p_ksc->be_err_cnt++;
	}

	p_ksc->irq_cnt++;

	return IRQ_HANDLED;
}


static void __free_reserve_mem(struct work_struct *w)
{
	struct ksc_device *p_ksc = container_of(w, struct ksc_device, free_reserve_mem.work);
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;
	unsigned long mem0_phy_addr, mem1_phy_addr;
	int i = 0, ret = -1;
	char *p;
	char *ksc_mem_phy_addr_str = NULL;
	char ksc_str[256] = { 0 };
	unsigned long *addr_buf[] = {
		&mem0_phy_addr,
		&mem1_phy_addr,
	};
	unsigned int mem_len_buf[] = {
		buf->mem_size,
		buf->mem_size,
	};
	struct iommu_domain *domain;

	/* free reserve memory area */
	ksc_mem_phy_addr_str = (char *)ksc_boot_para_parse_str(p_ksc, KSC_REV_MEM);
	if (ksc_mem_phy_addr_str == NULL) {
		printk("ksc_mem_phy_addr_str is null !!!\n");
		return;
	}
	// printk("ksc_mem_phy_addr_str is %s",ksc_mem_phy_addr_str);
	int str_len = strlen(ksc_mem_phy_addr_str);
	memcpy((void *)ksc_str, (void *)ksc_mem_phy_addr_str, str_len);
	ksc_str[str_len] = '\0';
	ksc_mem_phy_addr_str = ksc_str;

	i = 0;
	do {
		p = strstr(ksc_mem_phy_addr_str, ",");
		if (p != NULL)
			*p = '\0';
		ret = kstrtoul(ksc_mem_phy_addr_str, 10, addr_buf[i]);
		if (ret) {
			dev_info(p_ksc->p_device, "parse %s fail! index = %d\n", ksc_mem_phy_addr_str, i);
			break;
		}
		i++;
		ksc_mem_phy_addr_str = p + 1;
	} while (i < sizeof(addr_buf) / sizeof(addr_buf[0]) && p != NULL);

	for (i = 0; i < sizeof(addr_buf) / sizeof(addr_buf[0]); i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && !defined(MODULE)
		memblock_free(__va(*(addr_buf[i])), mem_len_buf[i]);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
		memblock_free(*(addr_buf[i]), mem_len_buf[i]);
#endif

#ifndef MODULE
	free_reserved_area(__va(*(addr_buf[i])),
				__va(*(addr_buf[i]) + PAGE_ALIGN(mem_len_buf[i])),
				0x100, "ksc_reserve_mem");
	}
#endif

#if IS_ENABLED(CONFIG_AW_IOMMU)
	/* free iommu iova to dma_addr */
	domain = iommu_get_domain_for_dev(p_ksc->p_device);
	if (domain != NULL) {
		iommu_unmap(domain, addr_buf[0], mem_len_buf[0]);
		iommu_unmap(domain, addr_buf[1], mem_len_buf[1]);
	}
#endif

}

static void __ksc_free_memory(struct ksc_device *p_ksc)
{

	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;

	if (buf->mem0_virt && buf->mem0_phy_addr)
		ksc_mem_free(p_ksc->p_device, buf->mem_size, buf->mem0_virt,
				  buf->mem0_phy_addr);
	if (buf->mem1_virt && buf->mem1_phy_addr)
		ksc_mem_free(p_ksc->p_device, buf->mem_size, buf->mem1_virt,
				  buf->mem1_phy_addr);
	if (buf->lut_mem0_virt && buf->lut_mem0_phy_addr) {
		ksc_mem_free(p_ksc->p_device, buf->lut_size, buf->lut_mem0_virt,
			     buf->lut_mem0_phy_addr);
	}
	if (buf->lut_mem1_virt && buf->lut_mem1_phy_addr) {
		ksc_mem_free(p_ksc->p_device, buf->lut_size, buf->lut_mem1_virt,
			     buf->lut_mem1_phy_addr);
	}

	buf->mem0_virt = NULL;
	buf->mem1_virt = NULL;
	buf->lut_virt = NULL;
	buf->lut_mem0_virt = NULL;
	buf->mem0_phy_addr = 0;
	buf->lut_mem1_virt = NULL;
	buf->mem1_phy_addr = 0;
	buf->lut_phy_addr = 0;

	if (buf->src_item) {
		if (!IS_ERR_OR_NULL(buf->src_item->buf)) {
			ksc_dma_unmap(buf->src_item);
		}
		kfree(buf->src_item);
		buf->src_item = NULL;
	}

	if (buf->dst_item) {
		if (!IS_ERR_OR_NULL(buf->dst_item->buf)) {
			ksc_dma_unmap(buf->dst_item);
		}
		kfree(buf->dst_item);
		buf->dst_item = NULL;
	}


	buf->alloced = false;
}

int __ksc_enable(struct ksc_device *p_ksc, bool enable)
{
	int ret = 0, i = 0;
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;

	mutex_lock(&p_ksc->dev_lock);

	if (p_ksc->enabled == enable) {
		goto OUT;
	}
	for (i = 0; i < MAX_CLK_NUM; ++i) {
		if (enable) {
			if (!IS_ERR_OR_NULL(p_ksc->rsts[i])) {
				reset_control_deassert(p_ksc->rsts[i]);
			}
			if (!IS_ERR_OR_NULL(p_ksc->clks[i])) {
				if (p_ksc->clk_freq[i]) {
					clk_set_rate(p_ksc->clks[i], p_ksc->clk_freq[i]);
				}
				clk_prepare_enable(p_ksc->clks[i]);
			}
		} else {
			if (!IS_ERR_OR_NULL(p_ksc->clks[i]))
				clk_disable_unprepare(p_ksc->clks[i]);
			if (!IS_ERR_OR_NULL(p_ksc->rsts[i]))
				reset_control_assert(p_ksc->rsts[i]);
		}
	}
	tvdisp_ksc_enable(p_ksc->tv_disp_top_reg, enable);

	if (enable) {
		buf->src_item = kmalloc(sizeof(struct dmabuf_item), __GFP_ZERO | GFP_KERNEL);
		buf->dst_item = kmalloc(sizeof(struct dmabuf_item), __GFP_ZERO | GFP_KERNEL);
		if (!buf->src_item || !buf->dst_item) {
			ret = -ENOMEM;
			goto OUT;
		}
		buf->src_item->p_device = p_ksc->p_device;
		buf->dst_item->p_device = p_ksc->p_device;

		ret = request_irq(p_ksc->irq_no, (irq_handler_t)ksc_irq_handler, 0x0,
				  "ksc", (void *)p_ksc);
		if (ret) {
			dev_err(p_ksc->p_device, "request irq fail:%d\n", p_ksc->irq_no);
			ret = -ENODEV;
			goto OUT;
		}
		p_ksc->ksc_para.is_first_frame = true;

		if (judge_ksc_smooth_disp == false) {
			ksc_smooth_disp_judge(p_ksc);
		}

		buf->lut_size = LUT_MEM_SIZE;//double buffer


		buf->lut_mem0_virt =
			ksc_mem_alloc(p_ksc->p_device, buf->lut_size,
					   &buf->lut_mem0_phy_addr);
		if (!buf->lut_mem0_virt || !buf->lut_mem0_phy_addr) {
			dev_err(p_ksc->p_device, "Memory0 alloc %d Byte ksc lut buffer fail!\n", buf->lut_size);
			ret = -ENOMEM;
			goto OUT;
		}

		buf->lut_mem1_virt =
			ksc_mem_alloc(p_ksc->p_device, buf->lut_size,
					   &buf->lut_mem1_phy_addr);
		if (!buf->lut_mem1_virt || !buf->lut_mem1_phy_addr) {
			dev_err(p_ksc->p_device, "Memory1 alloc %d Byte ksc lut buffer fail!\n", buf->lut_size);
			ret = -ENOMEM;
			goto OUT;
		}


		buf->lut_virt = buf->lut_mem0_virt;
		buf->lut_phy_addr = buf->lut_mem0_phy_addr;


	} else {
		free_irq(p_ksc->irq_no, (void *)p_ksc);
		__ksc_free_memory(p_ksc);

	}

	p_ksc->offline_finish_flag = false;
	p_ksc->enabled = enable;
	mutex_unlock(&p_ksc->dev_lock);
	return 0;

OUT:
	if (ret) {
		for (i = 0; i < MAX_CLK_NUM; ++i) {
			if (!IS_ERR_OR_NULL(p_ksc->clks[i]))
				clk_disable_unprepare(p_ksc->clks[i]);
			if (!IS_ERR_OR_NULL(p_ksc->rsts[i]))
				reset_control_assert(p_ksc->rsts[i]);
		}
		free_irq(p_ksc->irq_no, (void *)p_ksc);
		__ksc_free_memory(p_ksc);
	}
	mutex_unlock(&p_ksc->dev_lock);
	return ret;
}


static int __ksc_init_memory(struct ksc_device *p_ksc, struct sunxi_ksc_para *para)
{
	int mem_size = 0,  ret = -1;
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;


	if (para->pix_fmt == YUV422SP) {
		//Greater then actual size
		mem_size = ALIGN(para->dns_w, 64) * para->dns_h * 2;
		buf->chm_offset = ALIGN(para->dns_w, 64) * para->dns_h;
		if (para->bit_depth == 10) {
			mem_size += ALIGN(para->dns_w, 64) * para->dns_h * 2 / 4;
			buf->pix_2b_offset = buf->chm_offset + ALIGN(para->dns_w, 64) * para->dns_h;
		}
	} else if (para->pix_fmt == AYUV) {
		mem_size = ALIGN(para->dns_w, 16) * para->dns_h * 4;
		buf->chm_offset = ALIGN(para->dns_w, 16) * para->dns_h * 2;
		if (para->bit_depth == 10) {
			mem_size += ALIGN(para->dns_w, 64) * para->dns_h;
			buf->pix_2b_offset = buf->chm_offset + ALIGN(para->dns_w, 64) * para->dns_h * 2;
		}
	} else {
		if (para->mode == ONLINE_MODE) {
			dev_err(p_ksc->p_device, "Invalid FE output pixel format:%d\n", para->pix_fmt);
			return -1;
		}
	}

	if ((buf->alloced == false || mem_size != buf->mem_size) &&
	    para->mode == ONLINE_MODE) {
		if (buf->alloced == true) {
			if (buf->buf_to_be_free.mem_size != 0) {
				cancel_delayed_work_sync(
				    &p_ksc->free_buffer_work);
				dev_info(p_ksc->p_device, "Last frame's memory "
							  "has not been free "
							  "yet!\n");
			}
			buf->buf_to_be_free.mem_size = buf->mem_size;
			buf->buf_to_be_free.mem0_virt = buf->mem0_virt;
			buf->buf_to_be_free.mem1_virt = buf->mem1_virt;
			buf->buf_to_be_free.mem0_phy_addr = buf->mem0_phy_addr;
			buf->buf_to_be_free.mem1_phy_addr = buf->mem1_phy_addr;
			buf->alloced = false;
		}

		dev_info(p_ksc->p_device, "Alloc %u Byte memory for ksc\n",
			 mem_size);

		buf->mem_size = mem_size;

		buf->mem0_virt =
			ksc_mem_alloc(p_ksc->p_device, buf->mem_size,
					   &buf->mem0_phy_addr);
		buf->mem1_virt =
			ksc_mem_alloc(p_ksc->p_device, buf->mem_size,
					   &buf->mem1_phy_addr);
		if (!buf->mem0_virt || !buf->mem0_phy_addr || !buf->mem1_virt || !buf->mem1_phy_addr) {
			dev_err(p_ksc->p_device, "Memory alloc %d Byte ksc double buffer fail!\n", buf->mem_size * 2);
			goto OUT;
		}

		if (buf->buf_to_be_free.mem_size) {
			schedule_delayed_work(&p_ksc->free_buffer_work,
					      msecs_to_jiffies(20));
		}
		buf->alloced = true;
	}

	if (buf->lut_virt && buf->lut_phy_addr) {
		ksc_mem_sync(p_ksc->p_device, buf->lut_virt, buf->lut_phy_addr, buf->lut_size);
	}

	if (para->mode != ONLINE_MODE) {
		ret = ksc_dma_map(para->src_img.fd, buf->src_item);
		if (ret) {
			dev_err(p_ksc->p_device, "ksc_dma_map fail:%d\n", ret);
			goto OUT;
		}
	}

	if (para->online_wb_en == true || para->mode != ONLINE_MODE) {
		ret = ksc_dma_map(para->dst_img.fd, buf->dst_item);
		if (ret) {
			dev_err(p_ksc->p_device, "ksc_dma_map dst_item fail:%d\n", ret);
			goto OUT;
		}
	}

	return 0;
OUT:
	__ksc_free_memory(p_ksc);
	return -1;
}
int __ksc_set_pq_para(struct ksc_device *p_ksc, struct sunxi_ksc_para *para)
{
	struct ksc_pq_para *pq = NULL;

	pq = &para->pq_para;
	if (!pq) {
		dev_err(p_ksc->p_device, "Null ksc_pq_para\n");
		return -1;
	}
	pq->post_proc_en = 1;

	pq->csc_dns_type = 0;

	ksc_set_sharpen_para(p_ksc, para);

	pq->filt_type_bod = 0;
	pq->filt_para_adj = 1;
	pq->filt_type_thL = 21;
	pq->filt_type_thH = 32;
	pq->antialias_thL = 22;
	pq->antialias_thH = 35;
	pq->antialias_str = 64;
	pq->bod_pix_num_h0 = 0;
	pq->bod_pix_dif_h0 = 16;
	pq->bod_pix_num_h1 = 0;
	pq->bod_pix_dif_h1 = 16;
	pq->bod_pix_num_v0 = 0;
	pq->bod_pix_dif_v0 = 16;
	pq->bod_pix_num_v1 = 0;
	pq->bod_pix_dif_v1 = 16;

	pq->def_val_ch0 = 0;
	pq->def_val_ch1 = 128;
	pq->def_val_ch2 = 128;
	pq->def_val_ch3 = 0;
	pq->antialias_ref = 0;
	return 0;
}
int __ksc_set_para(struct ksc_device *p_ksc, struct sunxi_ksc_para *para)
{
	int ret = -1;
	bool is_finished = false;
	long timeout = 0;

	mutex_lock(&p_ksc->dev_lock);


	if (p_ksc->enabled == false) {
		dev_err(p_ksc->p_device, "Ksc has not been enabled yet!\n");
		goto OUT;
	}
	memcpy(&p_ksc->ksc_para.para, para, sizeof(struct sunxi_ksc_para));


	ret = __ksc_init_memory(p_ksc, para);
	if (ret) {
		dev_err(p_ksc->p_device, "ksc_init_memory error yet!\n");
		goto OUT;
	}

	/*dev_err(p_ksc->p_device, "fmt:%d %d %d %d\n",*/
		/*p_ksc->ksc_para.para.wb_fmt, p_ksc->ksc_para.para.file_fmt,*/
		/*p_ksc->ksc_para.para.pix_fmt,*/
		/*p_ksc->ksc_para.para.src_img.pix_fmt);*/
	ret = ksc_reg_update_all(p_ksc->p_reg, &p_ksc->ksc_para);
	if (ret) {
		goto OUT;
	}

	if (p_ksc->ksc_para.is_first_frame == true) {
		p_ksc->ksc_para.is_first_frame = false;
	}

	if (para->mode != ONLINE_MODE) {
		//wait offline process finish
		timeout = wait_event_timeout(p_ksc->event_wait,
					     p_ksc->offline_finish_flag == true,
					     msecs_to_jiffies(500));
		if (timeout <= 0) {
			dev_err(p_ksc->p_device, "Wait ksc offline process finish timout!\n");
			p_ksc->offline_finish_flag = true;
			wake_up(&p_ksc->event_wait);
		}
		p_ksc->offline_finish_flag = false;
		ksc_dma_unmap(p_ksc->ksc_para.buf.dst_item);
		ksc_dma_unmap(p_ksc->ksc_para.buf.src_item);
	} else {
		if (para->online_wb_en == true) {
			timeout = 0;
			is_finished = false;
			timeout = read_poll_timeout(is_mem_sync_finish, is_finished,
						    is_finished, 100, 50000, false, p_ksc->p_reg);
			if (timeout) {
				dev_err(p_ksc->p_device, "Wait mem sync timout!\n");
			}
			ksc_dma_unmap(p_ksc->ksc_para.buf.dst_item);
		}
	}
	if (is_ksc_smooth_disp == true) {
		/* release reserve mem */
		schedule_delayed_work(&p_ksc->free_reserve_mem,
					msecs_to_jiffies(20));
		is_ksc_smooth_disp = false;
	}

OUT:
	mutex_unlock(&p_ksc->dev_lock);
	return ret;
}


struct ksc_buffers *__get_ksc_buffers(struct ksc_device *p_ksc)
{
	bool is_finished = false;
	long timeout = 0;

	if (p_ksc->ksc_para.is_first_frame == false &&
	    p_ksc->ksc_para.para.mode == ONLINE_MODE) {
		// wait online reg update finish
		timeout = read_poll_timeout(is_reg_update_finish, is_finished,
					    is_finished, 100, 150000, false,
					    p_ksc->p_reg);
		if (timeout) {
			dev_err(p_ksc->p_device, "Wait reg update timout!\n");
		}
	}
	ksc_reg_switch_lut_buf(p_ksc->p_reg, &p_ksc->ksc_para);

	return &p_ksc->ksc_para.buf;
}

struct sunxi_ksc_para *__get_ksc_para(struct ksc_device *p_ksc)
{
	struct sunxi_ksc_para *para;
	mutex_lock(&p_ksc->dev_lock);
	para = &p_ksc->ksc_para.para;
	mutex_unlock(&p_ksc->dev_lock);
	return para;
}
static void __free_buffer_work(struct work_struct *w)
{
	struct ksc_device *p_ksc = container_of(w, struct ksc_device, free_buffer_work.work);
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;

	if (buf->buf_to_be_free.mem_size) {
		if (buf->buf_to_be_free.mem0_virt && buf->buf_to_be_free.mem0_phy_addr)
			ksc_mem_free(p_ksc->p_device, buf->buf_to_be_free.mem_size, buf->buf_to_be_free.mem0_virt,
					  buf->buf_to_be_free.mem0_phy_addr);
		if (buf->buf_to_be_free.mem1_virt && buf->buf_to_be_free.mem1_phy_addr)
			ksc_mem_free(p_ksc->p_device, buf->buf_to_be_free.mem_size, buf->buf_to_be_free.mem1_virt,
					  buf->buf_to_be_free.mem1_phy_addr);
		buf->buf_to_be_free.mem0_phy_addr = 0;
		buf->buf_to_be_free.mem1_phy_addr = 0;
		buf->buf_to_be_free.mem0_virt = NULL;
		buf->buf_to_be_free.mem1_virt = NULL;
		dev_info(p_ksc->p_device, "Free memory %u\n", buf->buf_to_be_free.mem_size);
		buf->buf_to_be_free.mem_size = 0;
	}

}

extern void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);

struct ksc_device *ksc_dev_init(struct device *dev)
{
	struct ksc_device *p_ksc = NULL;
	int i;
	unsigned int iommu_master_id, iommu_flag;
	char id[32];

	if (!dev || !dev->parent) {
		pr_err("Null pointer! %p %p\n", dev, dev->parent);
		goto OUT;
	}

	p_ksc = kmalloc(sizeof(struct ksc_device), GFP_KERNEL | __GFP_ZERO);
	if (!p_ksc) {
		dev_err(p_ksc->p_device->parent, "Malloc ksc_device fail!\n");
		goto OUT;
	}

	memcpy(&p_ksc->attr_device, dev, sizeof(struct device));
	p_ksc->p_device = dev->parent;

	p_ksc->drv_data = of_device_get_match_data(p_ksc->p_device);
	if (!p_ksc->drv_data) {
		dev_err(dev, "Fail to get driver data!\n");
		goto OUT;
	}

	p_ksc->p_reg =
		(struct ksc_regs *)of_iomap(p_ksc->p_device->of_node, 0);
	if (!p_ksc->p_reg) {
		dev_err(p_ksc->p_device, "unable to map ksc registers\n");
		goto OUT;
	}

	p_ksc->tv_disp_top_reg =
		(struct ksc_regs *)of_iomap(p_ksc->p_device->of_node, 1);
	if (!p_ksc->tv_disp_top_reg) {
		dev_info(p_ksc->p_device, "unable to map tv display top registers\n");
	}

	p_ksc->irq_no =
		irq_of_parse_and_map(p_ksc->p_device->of_node, 0);
	if (!p_ksc->irq_no) {
		dev_err(p_ksc->p_device, "irq_of_parse_and_map dec irq fail\n");
		goto OUT;
	}

	for (i = 0; i < MAX_CLK_NUM; ++i) {
		snprintf(id, 32, "clk%d", i);
		p_ksc->clks[i] = devm_clk_get(p_ksc->p_device, id);
		if (IS_ERR_OR_NULL(p_ksc->clks[i])) {
			dev_info(p_ksc->p_device->parent, "Fail to get clk%d\n", i);
		}
		snprintf(id, 32, "rst%d", i);
		p_ksc->rsts[i] = devm_reset_control_get_shared(p_ksc->p_device, id);
		if (IS_ERR_OR_NULL(p_ksc->rsts[i])) {
			dev_info(p_ksc->p_device->parent, "Fail to get rst%d\n", i);
		}

		snprintf(id, 32, "clk%d_freq", i);

		if (of_property_read_u32(p_ksc->p_device->of_node, id,
					 &p_ksc->clk_freq[i]) != 0) {
			dev_err(p_ksc->p_device, "Get %s property failed\n", id);
			p_ksc->clk_freq[i] = 0;
		}
	}

	p_ksc->p_attr_grp = kmalloc(sizeof(struct attribute_group), GFP_KERNEL | __GFP_ZERO);
	if (!p_ksc->p_attr_grp) {
		dev_err(p_ksc->p_device, "Faile to kmalloc attribute_group\n");
		goto OUT;
	}
	p_ksc->p_attr_grp->name = kmalloc(10, GFP_KERNEL | __GFP_ZERO);
	snprintf((char *)p_ksc->p_attr_grp->name, 10, "attr");
	p_ksc->p_attr_grp->attrs = ksc_attributes;
	if (sysfs_create_group(&p_ksc->attr_device.kobj, p_ksc->p_attr_grp))
		dev_err(p_ksc->p_device, "sysfs_create_group fail\n");

	/* enable iommu */
#if IS_ENABLED(CONFIG_AW_IOMMU)
	if (of_property_read_u32_index(p_ksc->p_device->of_node,
			"iommus", 1, iommu_master_id)) {
		DE_INFO("of_property_read_u32_index %s.%s.%s fail\n",
			p_ksc->p_device->of_node, "iommus", "iommu_master_id");
	}
	if (of_property_read_u32_index(p_ksc->p_device->of_node,
			"iommus", 2, iommu_flag)) {
		DE_INFO("of_property_read_u32_index %s.%s.%s fail\n",
			p_ksc->p_device->of_node, "iommus", "iommu_flag");
	}
	if (iommu_flag != 0) {
		sunxi_enable_device_iommu(iommu_master_id, true);
	}
#endif

	p_ksc->enable = __ksc_enable;
	p_ksc->ksc_set_pq_para = __ksc_set_pq_para;
	p_ksc->set_ksc_para = __ksc_set_para;
	p_ksc->get_ksc_buffers = __get_ksc_buffers;
	p_ksc->get_ksc_para = __get_ksc_para;


	mutex_init(&p_ksc->dev_lock);
	init_waitqueue_head(&p_ksc->event_wait);
	INIT_DELAYED_WORK(&p_ksc->free_buffer_work, __free_buffer_work);
	INIT_DELAYED_WORK(&p_ksc->free_reserve_mem, __free_reserve_mem);

	dev_info(p_ksc->p_device, "%s finsih\n", __func__);

	return p_ksc;

OUT:
	if (p_ksc) {
		if (p_ksc->p_reg)
			iounmap((char __iomem *)p_ksc->p_reg);
		if (p_ksc->p_attr_grp) {
			if (p_ksc->p_attr_grp->name)
				kfree(p_ksc->p_attr_grp->name);
			kfree(p_ksc->p_attr_grp);
		}

		kfree(p_ksc);
	}
	return NULL;
}

//End of File
