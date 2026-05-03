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
 *  File name   :  display engine 35x dci basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_dci_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

enum {
	DCI_PARA_REG_BLK = 0,
	DCI_CDF_REG_BLK,
	DCI_PDF_REG_BLK,
	DCI_REG_BLK_NUM,
};

struct de_dci_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct dci_status status;
	struct de_reg_block reg_blks[DCI_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_dci_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_dci_private dci_priv[DE_NUM][VI_CHN_NUM];
static u16 g_update_rate;
static bool g_start;
static enum enhance_init_state g_init_state;
static win_percent_t win_per;

static inline struct dci_reg *get_dci_reg(struct de_dci_private *priv)
{
	return (struct dci_reg *)(priv->reg_blks[DCI_PARA_REG_BLK].vir_addr);
}

static void dci_set_block_dirty(
	struct de_dci_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void dci_set_rcq_head_dirty(
	struct de_dci_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

#define NUM_BLKS 16
#define HIST_BINS 16
#define PDF_REC_NUM 2 /*need two record to calculate*/
static u32 *g_pdf[DE_NUM][VI_CHN_NUM][PDF_REC_NUM];
static u32 *g_cur_pdf[DE_NUM][VI_CHN_NUM];
static u32 g_last_cdfs[NUM_BLKS][HIST_BINS];
static bool ahb_read_enable;

s32 de_dci_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_dci_private *priv = &dci_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
    int i;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_DCI_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_DCI_REG_MEM_SIZE;

	priv->reg_blk_num = DCI_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[DCI_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x100;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[DCI_CDF_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x100;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x100;
	reg_blk->size = 0x400;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x100);

	reg_blk = &(priv->reg_blks[DCI_PDF_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x500;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x500;
	reg_blk->size = 0x400;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x500);

	*phy_addr += DE_DCI_REG_MEM_SIZE;
	*vir_addr += DE_DCI_REG_MEM_SIZE;
	*size -= DE_DCI_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = dci_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = dci_set_block_dirty;

	g_cur_pdf[disp][chn] = kmalloc(NUM_BLKS * HIST_BINS * sizeof(u32),
				       GFP_KERNEL | __GFP_ZERO);
	if (g_cur_pdf[disp][chn] == NULL) {
		__wrn("malloc g_cur_pdf[%d][%d] memory fail! \n", disp, chn);
		return -1;
	}

	for (i = 0; i < PDF_REC_NUM; i++) {
		g_pdf[disp][chn][i] =
			kmalloc(NUM_BLKS * HIST_BINS * sizeof(u32),
				GFP_KERNEL | __GFP_ZERO);
		if (g_pdf[disp][chn][i] == NULL) {
			__wrn("malloc g_pdf[%d][%d] memory fail!\n", disp, chn);
			return -1;
		}
	}
	g_init_state = ENHANCE_INVALID;
	g_update_rate = 243;
	g_start = false;

	return 0;
}

s32 de_dci_exit(u32 disp, u32 chn)
{
	int i;
	kfree(g_cur_pdf[disp][chn]);
	for (i = 0; i < PDF_REC_NUM; i++) {
		kfree(g_pdf[disp][chn][i]);
	}

	return 0;
}

s32 de_dci_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
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

s32 de_dci_enable(u32 disp, u32 chn, u32 en)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);

	en = 0;
	if (g_init_state == ENHANCE_TIGERLCD_ON) {
		en = 1;
	}
#ifdef SUPPORT_AHB_READ
	reg->ctl.bits.en = en;
	reg->ctl.bits.ftd_en = en;
	priv->status.isenable = en;
#else
	reg->ctl.bits.en = 0;
	reg->ctl.bits.ftd_en = 0;
	priv->status.isenable = 0;
#endif
	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	return 0;
}

s32 de_dci_init_para(u32 disp, u32 chn)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);

	if (g_init_state >= ENHANCE_INITED) {
		return 0;
	}
	g_init_state = ENHANCE_INITED;
	/*reg->ctl.dwval = 0x80000001;*/
	/* reg->ctl.dwval = 0x80000000; */
	/* reg->color_range.dwval = 0x1; */
	/* reg->skin_protect.dwval = 0x00800000; */
	/* reg->pdf_radius.dwval = 0x0; */
	/* reg->count_bound.dwval = 0x03ff0000; */
	/* reg->border_map_mode.dwval = 0x1; */
	/* reg->brighten_level0.dwval = 0x05030100; */
	/* reg->brighten_level1.dwval = 0x08080808; */
	/* reg->brighten_level2.dwval = 0x08080808; */
	/* reg->brighten_level3.dwval = 0x80808; */
	/* reg->darken_level0.dwval = 0x8080808; */
	/* reg->darken_level1.dwval = 0x8080808; */
	/* reg->darken_level2.dwval = 0x8080807; */
	/* reg->darken_level3.dwval = 0x30207; */
	/* reg->chroma_comp_br_th0.dwval = 0x32281e14; */
	/* reg->chroma_comp_br_th1.dwval = 0xff73463c; */
	/* reg->chroma_comp_br_gain0.dwval = 0x0c0a0804; */
	/* reg->chroma_comp_br_gain1.dwval = 0x10100e0e; */
	/* reg->chroma_comp_br_slope0.dwval = 0x06060603; */
	/* reg->chroma_comp_br_slope1.dwval = 0x00010606; */
	/* reg->chroma_comp_dark_th0.dwval = 0x5a46321e; */
	/* reg->chroma_comp_dark_th1.dwval = 0xffaa9682; */
	/* reg->chroma_comp_dark_gain0.dwval = 0x02020201; */
	/* reg->chroma_comp_dark_gain1.dwval = 0x02020202; */
	/* reg->chroma_comp_dark_slope0.dwval = 0x03030302; */
	/* reg->chroma_comp_dark_slope1.dwval = 0x00030301; */

	/* reg->inter_frame_para.dwval = 0x0001001e; */
	/* reg->ftd_hue_thr.dwval = 0x96005a; */
	/* reg->ftd_chroma_thr.dwval = 0x28000a; */
	/* reg->ftd_slp.dwval = 0x4040604; */

	reg->ctl.dwval = 0x80000000;
	reg->ftd_hue_thr.dwval = 0x96005a;
	reg->ftd_chroma_thr.dwval = 0x28000a;
	reg->ftd_slp.dwval = 0x4040604;

	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	return 0;
}

int de_dci_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data)
{
	struct de_dci_private *priv = NULL;
	struct dci_reg *reg = NULL;
	dci_module_param_t *para = NULL;
	int i = 0;

	DE_INFO("sel=%d, cmd=%d, subcmd=%d, data=%px\n", sel, cmd, subcmd, data);
	para = (dci_module_param_t *)data;
	if (para == NULL) {
		DE_WARN("para NULL\n");
		return -1;
	}

	for (i = 0; i < VI_CHN_NUM; i++) {
		if (!de_feat_is_support_dci_by_chn(sel, i))
			continue;
		priv = &(dci_priv[sel][i]);
		reg = get_dci_reg(priv);
		if (subcmd == 16) { /* read */
			para->value[0] = reg->ctl.bits.en;
			/*para->value[1] = reg->ctl.bits.demo_en;*/
			para->value[2]  = reg->ctl.bits.chroma_comp_en;

			para->value[58] = reg->ftd_hue_thr.bits.ftd_hue_low_thr;
			para->value[59] = reg->ftd_hue_thr.bits.ftd_hue_high_thr;
			para->value[60] = reg->ftd_chroma_thr.bits.ftd_chr_low_thr;
			para->value[61] = reg->ftd_chroma_thr.bits.ftd_chr_high_thr;
			para->value[62] = reg->ftd_slp.bits.ftd_hue_low_slp;
			para->value[63] = reg->ftd_slp.bits.ftd_hue_high_slp;
			para->value[64] = reg->ftd_slp.bits.ftd_chr_low_slp;
			para->value[65] = reg->ftd_slp.bits.ftd_chr_high_slp;
		} else { /* write */
			reg->ctl.bits.en = para->value[0];
			g_init_state = para->value[0] ? ENHANCE_TIGERLCD_ON : ENHANCE_TIGERLCD_OFF;
			reg->ctl.bits.demo_en = para->value[1];
			reg->ctl.bits.chroma_comp_en = para->value[2];

			/*reg->demo_horz.bits.demo_horz_start = para->value[53];
			reg->demo_horz.bits.demo_horz_end = para->value[54];
			reg->demo_vert.bits.demo_vert_start = para->value[55];
			reg->demo_vert.bits.demo_vert_end = para->value[56];*/

			win_per.hor_start = para->value[54];
			win_per.hor_end = para->value[55];
			win_per.ver_start = para->value[56];
			win_per.ver_end = para->value[57];
			win_per.demo_en = para->value[1];

			reg->ftd_hue_thr.bits.ftd_hue_low_thr = para->value[58];
			reg->ftd_hue_thr.bits.ftd_hue_high_thr = para->value[59];
			reg->ftd_chroma_thr.bits.ftd_chr_low_thr = para->value[60];
			reg->ftd_chroma_thr.bits.ftd_chr_high_thr = para->value[61];
			reg->ftd_slp.bits.ftd_hue_low_slp = para->value[62];
			reg->ftd_slp.bits.ftd_hue_high_slp = para->value[63];
			reg->ftd_slp.bits.ftd_chr_low_slp = para->value[64];
			reg->ftd_slp.bits.ftd_chr_high_slp = para->value[65];

			priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);
		}
	}
	return 0;
}


s32 de_dci_enable_ahb_read(u32 disp, bool en)
{
#ifdef SUPPORT_AHB_READ
	DE_WARN("de_dci_enable_ahb_read=%d\n", en);
	ahb_read_enable = en;
#else
	DE_WARN("force de_dci_enable_ahb_read=%d to 0\n", en);
	ahb_read_enable = 0;
#endif
	return 0;
}
