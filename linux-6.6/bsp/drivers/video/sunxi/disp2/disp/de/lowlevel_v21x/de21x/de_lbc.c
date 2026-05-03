/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"
#include "de_lbc_type.h"
#include "de_lbc.h"

enum {
	OVL_LBC_REG_BLK = 0,
	OVL_LBC_REG_BLK_NUM,
};

struct de_lbc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[OVL_LBC_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_lbc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_lbc_private lbc_priv[DE_NUM][MAX_CHN_NUM];

static inline struct __lbc_ovl_reg_t *get_lbc_reg(struct de_lbc_private *priv)
{
	return (struct __lbc_ovl_reg_t *)(priv->reg_blks[0].vir_addr);
}

static void lbc_set_block_dirty(
	struct de_lbc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void lbc_set_rcq_head_dirty(
	struct de_lbc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_lbc_apply_lay(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	u32 width, height, x, y;
	u32 seg_bits, seg_pitch;
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_lbc_private *priv = &(lbc_priv[disp][chn]);
	struct __lbc_ovl_reg_t *reg = get_lbc_reg(priv);
	struct disp_layer_info_inner *lay_info =
	    &(pdata[0]->config.info);

	reg->lbc_ctl.bits.lbc_en = lay_info->fb.lbc_en;
	reg->lbc_ctl.bits.lbc_fcen =  ((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : 1);
	reg->lbc_ctl.bits.alpmode = lay_info->alpha_mode & 0x3;
	reg->lbc_ctl.bits.clk_gate = 1;
	reg->lbc_ctl.bits.is_lossy = lay_info->fb.lbc_info.is_lossy ? 1 : 0;
	reg->lbc_ctl.bits.rc_en = lay_info->fb.lbc_info.rc_en ? 1 : 0;
	reg->lbc_ctl.bits.alpha = lay_info->alpha_value & 0xff;

	width = lay_info->fb.size[0].width;
	height = lay_info->fb.size[0].height;
	reg->lbc_v_size.bits.lay_width = (width ? (width - 1) : 0) & 0x1FFF;
	reg->lbc_v_size.bits.lay_height = (height ? (height - 1) : 0) & 0x1FFF;

	x = (u32)(lay_info->fb.crop.x >> 32);
	y = (u32)(lay_info->fb.crop.y >> 32);
	width = (u32)(lay_info->fb.crop.width >> 32);
	height = (u32)(lay_info->fb.crop.height >> 32);
	reg->lbc_crop_coor.bits.left_crop = x & 0x1FFF;
	reg->lbc_crop_coor.bits.top_crop = y & 0x1FFF;
	reg->lbc_crop_size.bits.width_crop = (width ? (width - 1) : 0) & 0x1FFF;
	reg->lbc_crop_size.bits.height_crop = (height ? (height - 1) : 0) & 0x1FFF;

	reg->lbc_laddr.bits.laddr = lay_info->fb.addr[0];
	reg->lbc_haddr.bits.haddr = (lay_info->fb.addr[0] >> 32) & 0xFF;

	/* parameters are calculated in user space, calculated as bit -> byte, 16 aligned. */
	seg_bits = DISPALIGN(lay_info->fb.lbc_info.seg_bit / 8, 16);
	seg_pitch = DISPALIGN(lay_info->fb.lbc_info.pitch / 8, 16);
	reg->lbc_seg.bits.pitch = seg_pitch & 0xFFFF;
	reg->lbc_seg.bits.seg_bit = seg_bits & 0xFFFF;

	x = chn_info->ovl_win.left;
	y = chn_info->ovl_win.top;
	width = chn_info->ovl_win.width;
	height = chn_info->ovl_win.height;
	reg->lbc_lay_coor.bits.ovl_coorx = x & 0x1FFF;
	reg->lbc_lay_coor.bits.ovl_coory = y & 0x1FFF;
	reg->lbc_ovl_size.bits.width_ovl = (width ? (width - 1) : 0) & 0x1FFF;
	reg->lbc_ovl_size.bits.height_ovl = (height ? (height - 1) : 0) & 0x1FFF;

	reg->lbc_fcolor.dwval = ((lay_info->mode == LAYER_MODE_BUFFER) ? 0 : lay_info->color);

	priv->set_blk_dirty(priv, OVL_LBC_REG_BLK, 1);

	print_hex_dump(KERN_DEBUG, "\t", DUMP_PREFIX_OFFSET, 16, 4, &reg->lbc_ctl.dwval, 52, 0);
	return 0;
}

s32 de_lbc_disable(u32 disp, u32 chn)
{
	struct de_lbc_private *priv = &(lbc_priv[disp][chn]);
	struct __lbc_ovl_reg_t *reg = get_lbc_reg(priv);
	reg->lbc_ctl.dwval = 0;
	priv->set_blk_dirty(priv, OVL_LBC_REG_BLK, 1);
	return 0;
}

s32 de_lbc_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {

		u32 phy_chn = de_feat_get_phy_chn_id(disp, chn);
		u8 __iomem *reg_base =
		    (u8 __iomem *)(de_reg_base + DE_CHN_OFFSET(phy_chn) +
				   CHN_LBC_OFFSET);
		u32 rcq_used = de_feat_is_using_rcq(disp);

		struct de_lbc_private *priv = &lbc_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *block;

		if (!de_feat_is_support_lbc_by_layer(disp, chn, 0))
			continue;

		reg_mem_info->size = sizeof(struct __lbc_ovl_reg_t);
		reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		    reg_mem_info->size,
		    (void *)&(reg_mem_info->phy_addr), rcq_used);
		if (NULL == reg_mem_info->vir_addr) {
			DE_WARN("alloc lbc[%d][%d] mm fail!size=0x%x\n",
			       disp, chn, reg_mem_info->size);
			return -1;
		}

		block = &(priv->reg_blks[OVL_LBC_REG_BLK]);
		block->phy_addr = reg_mem_info->phy_addr;
		block->vir_addr = reg_mem_info->vir_addr;
		block->size = sizeof(struct __lbc_ovl_reg_t);
		block->reg_addr = reg_base;

		priv->reg_blk_num = OVL_LBC_REG_BLK_NUM;

		if (rcq_used)
			priv->set_blk_dirty = lbc_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = lbc_set_block_dirty;
	}

	return 0;
}

s32 de_lbc_exit(u32 disp)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		struct de_lbc_private *priv = &lbc_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
					       reg_mem_info->phy_addr,
					       reg_mem_info->size);
	}

	return 0;
}

s32 de_lbc_get_reg_blocks(u32 disp, struct de_reg_block **blks,
			      u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn)
			total += lbc_priv[disp][chn].reg_blk_num;
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_lbc_private *priv = &(lbc_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WARN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = priv->reg_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin)
			*blks++ = blk_begin;
		total += num;
		*blk_num -= num;
	}
	*blk_num = total;
	return 0;
}
