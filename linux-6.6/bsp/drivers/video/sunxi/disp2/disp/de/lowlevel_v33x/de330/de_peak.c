/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *  File name   :   de_peak.c
 *
 *  Description :   display engine 3.0 peaking basic function definition
 *
 *  History     :   2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#include "../disp_al_de.h"
#include "de_peak_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

#define PEAK_PARA_NUM (12)

enum {
	PEAK_PEAK_REG_BLK = 0,
	PEAK_REG_BLK_NUM,
};

struct de_peak_dump_regs_priv {
	struct dump_regs_module *mod;
};

struct de_peak_dump_regs_priv dr_peak_priv;

struct de_peak_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[PEAK_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_peak_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_peak_private peak_priv[DE_NUM][VI_CHN_NUM];


static inline struct peak_reg *get_peak_reg(struct de_peak_private *priv)
{
	return (struct peak_reg *)(priv->reg_blks[0].vir_addr);
}

static void peak_set_block_dirty(
	struct de_peak_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void peak_set_rcq_head_dirty(
	struct de_peak_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_peak_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_peak_private *priv = &peak_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_PEAK_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_PEAK_REG_MEM_SIZE;

	reg_blk = &(priv->reg_blks[PEAK_PEAK_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x30;
	reg_blk->reg_addr = (u8 __iomem *)base;

	priv->reg_blk_num = PEAK_REG_BLK_NUM;

	*phy_addr += DE_PEAK_REG_MEM_SIZE;
	*vir_addr += DE_PEAK_REG_MEM_SIZE;
	*size -= DE_PEAK_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = peak_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = peak_set_block_dirty;

	return 0;
}

s32 de_peak_exit(u32 disp, u32 chn)
{
	return 0;
}

s32 de_peak_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_peak_private *priv = &(peak_priv[disp][chn]);
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

s32 de_peak_enable(u32 disp, u32 chn, u32 en)
{
	struct de_peak_private *priv = &(peak_priv[disp][chn]);
	struct peak_reg *reg = get_peak_reg(priv);

	DE_INFO("disp=%d, chn=%d, en=%d\n", disp, chn, en);
	reg->ctrl.bits.en = en;
	priv->set_blk_dirty(priv, PEAK_PEAK_REG_BLK, 1);
	return 0;
}

s32 de_peak_set_size(u32 disp, u32 chn, u32 width,
			   u32 height)
{
	struct de_peak_private *priv = &(peak_priv[disp][chn]);
	struct peak_reg *reg = get_peak_reg(priv);

	reg->size.dwval = (height - 1) << 16 | (width - 1);
	priv->set_blk_dirty(priv, PEAK_PEAK_REG_BLK, 1);
	return 0;
}

s32 de_peak_set_window(u32 disp, u32 chn,
			     u32 win_enable, struct de_rect_o window)
{
	struct de_peak_private *priv = &(peak_priv[disp][chn]);
	struct peak_reg *reg = get_peak_reg(priv);

	reg->ctrl.bits.win_en = win_enable;

	if (win_enable) {
		reg->win0.dwval = window.y << 16 | window.x;
		reg->win1.dwval =
			(window.y + window.h - 1) << 16 |
			(window.x + window.w - 1);
	}

	priv->set_blk_dirty(priv, PEAK_PEAK_REG_BLK, 1);
	return 0;
}

s32 de_peak_init_para(u32 disp, u32 chn)
{
	struct de_peak_private *priv = &(peak_priv[disp][chn]);
	struct peak_reg *reg = get_peak_reg(priv);

	reg->gainctrl.dwval = 128 << 16 | 0;
	reg->shootctrl.dwval = 31;
	reg->coring.dwval = 12;

	priv->set_blk_dirty(priv, PEAK_PEAK_REG_BLK, 1);
	return 0;
}

s32 de_peak_info2para(u32 disp, u32 chn,
			   u32 fmt, u32 dev_type,
			   struct __peak_config_data *para,
			   u32 bypass)
{
	static s32 peak_para[PEAK_PARA_NUM][2] = {
		/* lcd / hdmi */
		{0,   0}, /* 00 gain for yuv */
		{4,   2}, /* 01			 */
		{10,  5}, /* 02			 */
		{16,  8}, /* 03			 */
		{20, 10}, /* 04			 */
		{24, 12}, /* 05			 */
		{28, 14}, /* 06			 */
		{32, 16}, /* 07			 */
		{36, 18}, /* 08			 */
		{44, 22}, /* 09			 */
		{52, 26}, /* 10 gain for yuv */
		{8,   4}, /* 11 gain for rgb */
	};

	struct de_peak_private *priv = &(peak_priv[disp][chn]);
	struct peak_reg *reg = get_peak_reg(priv);
	s32 gain, en;
	s32 hp_ratio, bp0_ratio;
	u32 linebuf = 2048;
	u32 peak2d_bypass;

	linebuf = de_feat_get_scale_linebuf_for_ed(disp, chn);
	if ((para->inw >= para->outw) || (para->inh >= para->outh) ||
	    (para->inw > linebuf))
		peak2d_bypass = 1;
	else
		peak2d_bypass = 0;

	/* parameters */
	en = (((fmt == 0) && (para->level == 0)) || (bypass != 0)) ? 0 : 1;
	para->mod_en = en; /* return enable info */

	if (en == 0 || (peak2d_bypass == 0 && para->peak2d_exist == 1)) {
		/* if level=0, module will be disabled,
		* no need to set para register
		*/
		reg->ctrl.bits.en = en;
		goto exit;
	}

	if (para->level >= PEAK_PARA_NUM || dev_type >= 2) {
		DE_WARN("peak para over range level %d dev_type %d",
			 para->level, dev_type);
		return -1;
	}

	if (fmt == 1) /* rgb */
		gain = peak_para[PEAK_PARA_NUM-1][dev_type];
	else
		gain = peak_para[para->level][dev_type];

	if (dev_type == 0) {
		/* lcd */
		hp_ratio = 0x4;
		bp0_ratio = 0xc;
	} else {
		/* hdmi */
		hp_ratio = 0xe;
		bp0_ratio = 0x2;
	}
	reg->ctrl.bits.en = en;
	reg->filter.dwval = 0 << 31 | hp_ratio << 16 |
			bp0_ratio << 8 | 0;
	reg->gain.dwval = gain;

exit:
	priv->set_blk_dirty(priv, PEAK_PEAK_REG_BLK, 1);
	return 0;
}

static int de_peak_get_enable_info(const struct dump_regs_user_setting *setting, char *buf)
{

	return 0;
}

static int de_peak_get_enable_status(const struct dump_regs_user_setting *setting)
{
	int disp = setting->index.disp;
	int chn = setting->index.chn;
	struct de_peak_private *priv = &(peak_priv[disp][chn]);
	struct peak_reg __iomem *regs = (struct peak_reg *)(priv->reg_blks[0].reg_addr);
	u32 value = 0;
	u32 enable;

	if (!de_feat_is_support_vep_by_chn(disp, chn))
		return 0;

	de_lowlevel_ioread(&(regs->ctrl), (void *)&value, 4);
	enable = ((union lp_ctrl_reg)value).bits.en & 0x1;
	DUMP_REGS_PRINT("peak: disp%d chn%d enable%d\n", disp, chn, enable);

	return enable;
}

static int de_peak_dump_by_user_setting(const struct dump_regs_user_setting *setting, char *buf)
{
	unsigned int num = 0;
	struct de_peak_private *priv;
	int i, j, disp, chn, screen_num, chn_num, is_enable, is_support;

	num += sprintf(buf + num, "%s module:\n\n", dr_peak_priv.mod->name);

	if (setting->use_index_find) {
		disp = setting->index.disp;
		chn = setting->index.chn;
		priv = &(peak_priv[disp][chn]);
		is_enable = de_peak_get_enable_status(setting);
		is_support = de_feat_is_support_vep_by_chn(disp, chn);

		num += sprintf(buf + num, "index(%d, %d) (%s) (%s)\n", disp, chn, is_support ? "supported" : "not supported", is_enable ? "enabled" : "disabled");
		if ((is_enable || setting->force_dump) && is_support)
			num += do_dump_regs_block(dr_peak_priv.mod, priv->reg_blks, ARRAY_SIZE(priv->reg_blks), buf + num);
		num += sprintf(buf + num, "\n");
	} else {
		screen_num = de_feat_get_num_screens();
		for (i = 0; i < screen_num; i++) {
			chn_num = de_feat_get_num_chns(i);
			for (j = 0; j < chn_num; j++) {
				struct dump_regs_user_setting msetting = {.index = {i, j, 0}};
				priv = &(peak_priv[i][j]);
				is_enable = de_peak_get_enable_status(&msetting);
				is_support = de_feat_is_support_vep_by_chn(i, j);

				num += sprintf(buf + num, "index(%d, %d) (%s) (%s)\n", i, j, is_support ? "supported" : "not supported", is_enable ? "enabled" : "disabled");
				if ((is_enable || setting->force_dump) && is_support)
					num += do_dump_regs_block(dr_peak_priv.mod, priv->reg_blks, ARRAY_SIZE(priv->reg_blks), buf + num);
				num += sprintf(buf + num, "\n");
			}
		}
	}

	return num;
}

int de_peak_register_dr_module(void)
{
#define PEAK_MAX_BLOCK_SIZE (0x30)
	struct dump_regs_module *mod = dr_module_alloc(PEAK_MAX_BLOCK_SIZE);

	dr_peak_priv.mod = mod;

	mod->type = DISP_PEAK_MODULE;
	strcpy(mod->name, module_name[mod->type]);
	mod->ops.dump_by_user_setting = de_peak_dump_by_user_setting;
	mod->ops.get_enable_info = de_peak_get_enable_info;
	mod->ops.get_enable_status = de_peak_get_enable_status;
	register_dr_module(mod);

	return 0;
}

int de_peak_unregister_dr_module(void)
{
	unregister_dr_module(dr_peak_priv.mod);
	return 0;
}
