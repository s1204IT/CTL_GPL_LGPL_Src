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
 *  All Winner Tech, All Right Reserved. 2015-2016 Copyright (c)
 *
 *  File name   :       de_fcc.c
 *
 *  Description :       display engine fcc base functions implement
 *
 *  History     :       2016/03/22  iptang  v0.1  Initial version
 *
 ******************************************************************************/

#include "../disp_al_de.h"
#include "de_fcc_type.h"
#include "de_rtmx.h"
#include "de_vep_table.h"
#include "de_enhance.h"

#define FCC_PARA_NUM (12)

enum {
	FCC_PARA_REG_BLK = 0,
	FCC_CSC_REG_BLK,
	FCC_LUT_REG_BLK,
	FCC_REG_BLK_NUM,
};

struct de_fcc_dump_regs_priv {
	struct dump_regs_module *mod;
};

struct de_fcc_dump_regs_priv dr_fcc_priv;

struct de_fcc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FCC_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_fcc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_fcc_private fcc_priv[DE_NUM][VI_CHN_NUM];


static inline struct fcc_reg *get_fcc_reg(struct de_fcc_private *priv)
{
	return (struct fcc_reg *)(priv->reg_blks[FCC_PARA_REG_BLK].vir_addr);
}

static inline struct fcc_lut_reg *get_lut_reg(struct de_fcc_private *priv)
{
	return (struct fcc_lut_reg *)(priv->reg_blks[FCC_LUT_REG_BLK].vir_addr);
}

static void fcc_set_block_dirty(
	struct de_fcc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void fcc_set_rcq_head_dirty(
	struct de_fcc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_fcc_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_fcc_private *priv = &fcc_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_FCC_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_FCC_REG_MEM_SIZE;

	priv->reg_blk_num = FCC_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[FCC_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x58;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[FCC_CSC_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x60;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x60;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x60);

	reg_blk = &(priv->reg_blks[FCC_LUT_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x100;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x100;
	reg_blk->size = sizeof(struct fcc_lut_reg);
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x100);

	*phy_addr += DE_FCC_REG_MEM_SIZE;
	*vir_addr += DE_FCC_REG_MEM_SIZE;
	*size -= DE_FCC_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = fcc_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = fcc_set_block_dirty;

	return 0;
}

s32 de_fcc_exit(u32 disp, u32 chn)
{
	return 0;
}

s32 de_fcc_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
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

s32 de_fcc_enable(u32 disp, u32 chn, u32 en)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);

	u32 tmp;

	tmp = reg->ctl.dwval & 0x170;
	tmp |= (7 << 4);
	tmp |= (en & 0x1);
	reg->ctl.dwval = tmp;
	priv->set_blk_dirty(priv, FCC_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcc_set_size(u32 disp, u32 chn, u32 width,
		    u32 height)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);
	u32 tmp;

	tmp = width == 0 ? 0 : width - 1;
	tmp |= ((height == 0 ? 0 : height - 1) << 16);
	reg->size.dwval = tmp;

	priv->set_blk_dirty(priv, FCC_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcc_set_window(u32 disp, u32 chn, u32 win_en,
		      struct de_rect_o window)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);
	u32 tmp;

	tmp = reg->ctl.dwval & 0x71;
	tmp |= ((win_en & 0x1) << 8);
	reg->ctl.dwval = tmp;
	if (win_en) {
		tmp = window.x | (window.y << 16);
		reg->win0.dwval = tmp;
		tmp = (window.x + window.w - 1);
		tmp |= ((window.y + window.h - 1) << 16);
		reg->win1.dwval = tmp;
	}

	priv->set_blk_dirty(priv, FCC_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcc_set_lut(u32 disp, u32 chn)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_lut_reg *reg = get_lut_reg(priv);
	u32 i, tmp;

	for (i = 0; i < 256; i++) {
		tmp = fcc_hue_tab[i << 1] | (fcc_hue_tab[(i << 1)+1] << 16);
		reg->lut[i].dwval = tmp;
	}

	priv->set_blk_dirty(priv, FCC_LUT_REG_BLK, 1);

	return 0;
}

s32 de_fcc_init_para(u32 disp, u32 chn)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);

	aw_memcpy_toio((void *)reg->hue_range,
	       (void *)&fcc_range_gain[0], sizeof(s32) * 6);
	aw_memcpy_toio((void *)reg->hue_gain, (void *)&fcc_hue_gain[0],
	       sizeof(s32) * 6);

	reg->color_gain.dwval = 0x268;
	reg->light_ctrl.dwval = 0x1000080;
	reg->lut_ctrl.dwval = 0x1;
	priv->set_blk_dirty(priv, FCC_PARA_REG_BLK, 1);
	de_fcc_set_lut(disp, chn);

	return 0;
}

s32 de_fcc_info2para(u32 disp, u32 chn, u32 fmt,
		     u32 dev_type, struct __fcc_config_data *para,
		     u32 bypass)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);
	s32 sgain;
	u32 en;

	s32 fcc_para[FCC_PARA_NUM][2] = {
		/* lcd / hdmi */
		{-255,  -255},		/* 00 gain for yuv */
		{-128,  -128},		/* 01 */
		{-64,   -64},		/* 02 */
		{-32,   -32},		/* 03 */
		{0,      0},		/* 04 */
		{32,     32},		/* 05 */
		{64,     64},		/* 06 */
		{80,     80},		/* 07 */
		{96,     96},		/* 08 */
		{128,    128},		/* 09 */
		{192,    192},		/* 10 gain for yuv */
		{0,      0},		/* 11 gain for rgb */
	};

	if (para->level >= FCC_PARA_NUM || dev_type >= 2) {
		DE_WARN("fcc para over range level %d dev_type %d",
			 para->level, dev_type);
		return -1;
	}

	/* parameters */
	if (fmt == 1)
		sgain = fcc_para[FCC_PARA_NUM - 1][dev_type];
	else
		sgain = fcc_para[para->level][dev_type];

	en = (((fmt == 0) && (sgain == 0)) || (bypass != 0)) ? 0 : 1;
	de_fcc_enable(disp, chn, en);

	reg->sat_gain.dwval = sgain;
	priv->set_blk_dirty(priv, FCC_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcc_set_icsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);
	u32 *ccsc_coeff = NULL; /* point to csc coeff table */

	de_csc_coeff_calc(in_info, out_info, &ccsc_coeff);

	if (NULL != ccsc_coeff) {
		u32 dwval;

		dwval = *(ccsc_coeff + 12);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->icsc_d0.dwval = dwval;
		dwval = *(ccsc_coeff + 13);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->icsc_d1.dwval = dwval;
		dwval = *(ccsc_coeff + 14);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->icsc_d2.dwval = dwval;

		reg->icsc_c0[0].dwval = *(ccsc_coeff + 0);
		reg->icsc_c0[1].dwval = *(ccsc_coeff + 1);
		reg->icsc_c0[2].dwval = *(ccsc_coeff + 2);
		reg->icsc_c0[3].dwval = *(ccsc_coeff + 3);

		reg->icsc_c1[0].dwval = *(ccsc_coeff + 4);
		reg->icsc_c1[1].dwval = *(ccsc_coeff + 5);
		reg->icsc_c1[2].dwval = *(ccsc_coeff + 6);
		reg->icsc_c1[3].dwval = *(ccsc_coeff + 7);

		reg->icsc_c2[0].dwval = *(ccsc_coeff + 8);
		reg->icsc_c2[1].dwval = *(ccsc_coeff + 9);
		reg->icsc_c2[2].dwval = *(ccsc_coeff + 10);
		reg->icsc_c2[3].dwval = *(ccsc_coeff + 11);

		priv->set_blk_dirty(priv, FCC_CSC_REG_BLK, 1);
	}
	return 0;
}

s32 de_fcc_set_ocsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg *reg = get_fcc_reg(priv);
	u32 *ccsc_coeff = NULL; /* point to csc coeff table */

	de_csc_coeff_calc(in_info, out_info, &ccsc_coeff);

	if (NULL != ccsc_coeff) {
		u32 dwval;

		dwval = *(ccsc_coeff + 12);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->ocsc_d0.dwval = dwval;
		dwval = *(ccsc_coeff + 13);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->ocsc_d1.dwval = dwval;
		dwval = *(ccsc_coeff + 14);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->ocsc_d2.dwval = dwval;

		reg->ocsc_c0[0].dwval = *(ccsc_coeff + 0);
		reg->ocsc_c0[1].dwval = *(ccsc_coeff + 1);
		reg->ocsc_c0[2].dwval = *(ccsc_coeff + 2);
		reg->ocsc_c0[3].dwval = *(ccsc_coeff + 3);

		reg->ocsc_c1[0].dwval = *(ccsc_coeff + 4);
		reg->ocsc_c1[1].dwval = *(ccsc_coeff + 5);
		reg->ocsc_c1[2].dwval = *(ccsc_coeff + 6);
		reg->ocsc_c1[3].dwval = *(ccsc_coeff + 7);

		reg->ocsc_c2[0].dwval = *(ccsc_coeff + 8);
		reg->ocsc_c2[1].dwval = *(ccsc_coeff + 9);
		reg->ocsc_c2[2].dwval = *(ccsc_coeff + 10);
		reg->ocsc_c2[3].dwval = *(ccsc_coeff + 11);

		priv->set_blk_dirty(priv, FCC_CSC_REG_BLK, 1);
	}
	return 0;
}

static int de_fcc_get_enable_info(const struct dump_regs_user_setting *setting, char *buf)
{

	return 0;
}

static int de_fcc_get_enable_status(const struct dump_regs_user_setting *setting)
{
	int disp = setting->index.disp;
	int chn = setting->index.chn;
	struct de_fcc_private *priv = &(fcc_priv[disp][chn]);
	struct fcc_reg __iomem *regs = (struct fcc_reg *)(priv->reg_blks[0].reg_addr);
	u32 value = 0;
	u32 enable;

	if (!de_feat_is_support_vep_by_chn(disp, chn))
		return 0;

	de_lowlevel_ioread(&(regs->ctl), (void *)&value, 4);
	enable = ((union __fcc_ctrl_reg_t)value).bits.en & 0x1;
	DUMP_REGS_PRINT("fcc: disp%d chn%d enable%d\n", disp, chn, enable);

	return enable;
}

static int de_fcc_dump_by_user_setting(const struct dump_regs_user_setting *setting, char *buf)
{
	unsigned int num = 0;
	struct de_fcc_private *priv;
	int i, j, disp, chn, screen_num, chn_num, is_enable, is_support;

	num += sprintf(buf + num, "%s module:\n\n", dr_fcc_priv.mod->name);

	if (setting->use_index_find) {
		disp = setting->index.disp;
		chn = setting->index.chn;
		priv = &(fcc_priv[disp][chn]);
		is_enable = de_fcc_get_enable_status(setting);
		is_support = de_feat_is_support_vep_by_chn(disp, chn);

		num += sprintf(buf + num, "index(%d, %d) (%s) (%s)\n", disp, chn, is_support ? "supported" : "not supported", is_enable ? "enabled" : "disabled");
		if ((is_enable || setting->force_dump) && is_support)
			num += do_dump_regs_block(dr_fcc_priv.mod, priv->reg_blks, FCC_CSC_REG_BLK + 1, buf + num);
		num += sprintf(buf + num, "\n");
	} else {
		screen_num = de_feat_get_num_screens();
		for (i = 0; i < screen_num; i++) {
			chn_num = de_feat_get_num_chns(i);
			for (j = 0; j < chn_num; j++) {
				struct dump_regs_user_setting msetting = {.index = {i, j, 0}};
				priv = &(fcc_priv[i][j]);
				is_enable = de_fcc_get_enable_status(&msetting);
				is_support = de_feat_is_support_vep_by_chn(i, j);

				num += sprintf(buf + num, "index(%d, %d) (%s) (%s)\n", i, j, is_support ? "supported" : "not supported", is_enable ? "enabled" : "disabled");
				if ((is_enable || setting->force_dump) && is_support)
					num += do_dump_regs_block(dr_fcc_priv.mod, priv->reg_blks, FCC_CSC_REG_BLK + 1, buf + num);
				num += sprintf(buf + num, "\n");
			}
		}
	}

	return num;
}

int de_fcc_register_dr_module(void)
{
#define FCC_MAX_BLOCK_SIZE (0x80)
	struct dump_regs_module *mod = dr_module_alloc(FCC_MAX_BLOCK_SIZE);

	dr_fcc_priv.mod = mod;

	mod->type = DISP_FCC_MODULE;
	strcpy(mod->name, module_name[mod->type]);
	mod->ops.dump_by_user_setting = de_fcc_dump_by_user_setting;
	mod->ops.get_enable_info = de_fcc_get_enable_info;
	mod->ops.get_enable_status = de_fcc_get_enable_status;
	register_dr_module(mod);

	return 0;
}

int de_fcc_unregister_dr_module(void)
{
	unregister_dr_module(dr_fcc_priv.mod);
	return 0;
}
