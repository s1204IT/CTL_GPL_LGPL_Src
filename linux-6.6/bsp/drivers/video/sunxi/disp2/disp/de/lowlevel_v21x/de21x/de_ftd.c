/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :  display engine 21x ftd basic function definition
 *
 *  History     :  2024/10/17 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_enhance.h"
#include "de_ftd_type.h"
#include "de_rtmx.h"

static enum enhance_init_state g_init_state;

enum {
	FTD_REG_BLK = 0,
	FTD_REG_BLK_NUM,
};

struct de_ftd_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FTD_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_ftd_private *priv, u32 blk_id, u32 dirty);
};

static struct de_ftd_private ftd_priv[DE_NUM][VI_CHN_NUM];
static win_percent_t win_per;

static inline struct ftd_reg *get_ftd_reg(struct de_ftd_private *priv) { return (struct ftd_reg *)(priv->reg_blks[FTD_REG_BLK].vir_addr); }

static void ftd_set_block_dirty(struct de_ftd_private *priv, u32 blk_id, u32 dirty) { priv->reg_blks[blk_id].dirty = dirty; }

static void ftd_set_rcq_head_dirty(struct de_ftd_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_ftd_init(u32 disp, u32 chn, uintptr_t reg_base, u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_ftd_private *priv = &ftd_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_FTD_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_FTD_REG_MEM_SIZE;

	priv->reg_blk_num = FTD_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[FTD_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = DE_FTD_REG_MEM_SIZE;
	reg_blk->reg_addr = (u8 __iomem *)base;

	*phy_addr += DE_FTD_REG_MEM_SIZE;
	*vir_addr += DE_FTD_REG_MEM_SIZE;
	*size -= DE_FTD_REG_MEM_SIZE;
	g_init_state = ENHANCE_INVALID;
	if (rcq_used)
		priv->set_blk_dirty = ftd_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = ftd_set_block_dirty;
	return 0;
}

s32 de_ftd_exit(u32 disp, u32 chn) { return 0; }

s32 de_ftd_get_reg_blocks(u32 disp, u32 chn, struct de_reg_block **blks, u32 *blk_num)
{
	struct de_ftd_private *priv = &(ftd_priv[disp][chn]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WARN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_ftd_enable(u32 disp, u32 chn, u32 en)
{
	en = 0;
	if (g_init_state == ENHANCE_TIGERLCD_ON) {
		en = 1;
	}
	struct de_ftd_private *priv = &(ftd_priv[disp][chn]);
	struct ftd_reg *reg = get_ftd_reg(priv);
	reg->ctl.bits.ftd_en = en;
	priv->set_blk_dirty(priv, FTD_REG_BLK, 1);

	return 0;
}

s32 de_ftd_init_para(u32 disp, u32 chn)
{
	if (g_init_state >= ENHANCE_INITED) {
		return 0;
	}
	g_init_state = ENHANCE_INITED;
	struct de_ftd_private *priv = &(ftd_priv[disp][chn]);
	struct ftd_reg *reg = get_ftd_reg(priv);
	reg->ftd_hue_thr.dwval = 0x96005a;
	reg->ftd_chroma_thr.dwval = 0x28000a;
	reg->ftd_slp.dwval = 0x4040604;

	priv->set_blk_dirty(priv, FTD_REG_BLK, 1);

	return 0;
}

int de_ftd_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data)
{
	struct de_ftd_private *priv = NULL;
	struct ftd_reg *reg = NULL;
	ftd_module_param_t *para = NULL;
	int i = 0;

	DE_INFO("sel=%d, cmd=%d, subcmd=%d, data=%px\n", sel, cmd, subcmd, data);
	para = (ftd_module_param_t *)data;
	if (para == NULL) {
		DE_WARN("para NULL\n");
		return -1;
	}

	for (i = 0; i < VI_CHN_NUM; i++) {
		if (!de_feat_is_support_ftd_by_chn(sel, i))
			continue;
		priv = &(ftd_priv[sel][i]);
		reg = get_ftd_reg(priv);
		if (subcmd == 16) { /* read */
			para->value[0] = reg->ctl.bits.ftd_en;
			para->value[1] = reg->ftd_hue_thr.bits.ftd_hue_low_thr;
			para->value[2] = reg->ftd_hue_thr.bits.ftd_hue_high_thr;
			para->value[3] = reg->ftd_chroma_thr.bits.ftd_chr_low_thr;
			para->value[4] = reg->ftd_chroma_thr.bits.ftd_chr_high_thr;
			para->value[5] = reg->ftd_slp.bits.ftd_hue_low_slp;
			para->value[6] = reg->ftd_slp.bits.ftd_hue_high_slp;
			para->value[7] = reg->ftd_slp.bits.ftd_chr_low_slp;
			para->value[8] = reg->ftd_slp.bits.ftd_chr_high_slp;
			para->value[9] = reg->ftd_people_gain.bits.ftd_chr_little_thr;
		} else { /* write */
			reg->ctl.bits.ftd_en = para->value[0];
			reg->ftd_hue_thr.bits.ftd_hue_low_thr = para->value[1];
			reg->ftd_hue_thr.bits.ftd_hue_high_thr = para->value[2];
			reg->ftd_chroma_thr.bits.ftd_chr_low_thr = para->value[3];
			reg->ftd_chroma_thr.bits.ftd_chr_high_thr = para->value[4];
			reg->ftd_slp.bits.ftd_hue_low_slp = para->value[5];
			reg->ftd_slp.bits.ftd_hue_high_slp = para->value[6];
			reg->ftd_slp.bits.ftd_chr_low_slp = para->value[7];
			reg->ftd_slp.bits.ftd_chr_high_slp = para->value[8];
			reg->ftd_people_gain.bits.ftd_chr_little_thr = para->value[9];
			priv->set_blk_dirty(priv, FTD_REG_BLK, 1);
			g_init_state = para->value[0] ? ENHANCE_TIGERLCD_ON : ENHANCE_TIGERLCD_OFF;
		}
	}
	return 0;
}