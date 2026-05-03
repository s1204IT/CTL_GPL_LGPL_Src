/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_bld_type.h"

#include "../../include.h"
#include "../disp_al_de.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_rtmx.h"

#include "de_bld.h"

enum {
	BLD_REG_BLK_ATTR = 0,
	BLD_REG_BLK_CTL,
	BLD_REG_BLK_CK,
	BLD_REG_BLK_NUM,
};

struct de_bld_dump_regs_priv {
	struct dump_regs_module *mod;
};

struct de_bld_dump_regs_priv dr_bld_priv;

struct de_bld_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[BLD_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_bld_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_bld_private bld_priv[DE_NUM];

static inline struct bld_reg *get_bld_reg(struct de_bld_private *priv)
{
	return (struct bld_reg *)(priv->reg_blks[0].vir_addr);
}

static inline struct bld_reg *get_bld_hw_reg(struct de_bld_private *priv)
{
	return (struct bld_reg *)(priv->reg_blks[0].reg_addr);
}

static void bld_set_block_dirty(
	struct de_bld_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void bld_set_rcq_head_dirty(
	struct de_bld_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_bld_set_bg_color(u32 disp, u32 color)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);

	reg->bg_color.dwval = color;
	priv->set_blk_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

s32 de_bld_set_pipe_fcolor(u32 disp, u32 pipe, u32 color)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);

	reg->pipe.en.bits.pipe0_fcolor_en = 1;
	reg->pipe.attr[0].fcolor.dwval = color;
	priv->set_blk_dirty(priv, BLD_REG_BLK_ATTR, 1);
	return 0;
}

s32 de_bld_get_out_size(u32 disp, u32 *p_width, u32 *p_height)
{
	struct de_bld_private *priv = NULL;
	struct bld_reg *reg = NULL;
	s32 ret = -1;

	if (!p_width || !p_height || disp >= DE_NUM)
		return ret;

	priv = &(bld_priv[disp]);
	reg = get_bld_hw_reg(priv);
	if (reg) {
		*p_width = reg->out_size.bits.width + 1;
		*p_height = reg->out_size.bits.height + 1;
		ret = 0;
	}

	return ret;
}

s32 de_bld_set_out_size(u32 disp, u32 width, u32 height)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);

	reg->out_size.dwval = (width ? ((width - 1) & 0x1FFF) : 0)
		| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
	priv->set_blk_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

s32 de_bld_set_fmt_space(u32 disp, u32 fmt_space)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);

	reg->out_ctl.bits.fmt_space = fmt_space;
	reg->out_ctl.bits.premul_en = 0;
	priv->set_blk_dirty(priv, BLD_REG_BLK_CK, 1);
	return 0;
}

s32 de_bld_set_out_scan_mode(u32 disp, u32 mode)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);

	reg->out_ctl.bits.interlace_en = mode;
	priv->set_blk_dirty(priv, BLD_REG_BLK_CK, 1);
	return 0;
}

s32 de_bld_set_pipe_ctl(u32 disp,
	struct de_bld_pipe_info *pipe_info, u32 pipe_num)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);
	u32 dwval;
	u32 i;

	dwval = 0x1;
	for (i = 0; i < pipe_num; ++i)
		dwval |= pipe_info[i].enable ? (1 << (i + 8)) : 0;
	reg->pipe.en.dwval = dwval;

	dwval = 0;
	for (i = 0; i < pipe_num; ++i)
		dwval |= ((pipe_info[i].chn & 0xF) << (i << 2));
	reg->rout_ctl.dwval = dwval;

	dwval = 0;
	for (i = 0; i < pipe_num; ++i)
		dwval |= ((pipe_info[i].premul_ctl & 0x1) << i);
	reg->premul_ctl.dwval = dwval;

	priv->set_blk_dirty(priv, BLD_REG_BLK_ATTR, 1);
	priv->set_blk_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

s32 de_bld_set_pipe_attr(u32 disp, u32 chn, u32 pipe)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);
	u32 dwval;

	dwval = (chn_info->scn_win.width ?
		((chn_info->scn_win.width - 1) & 0x1FFF) : 0)
		| (chn_info->scn_win.height ?
		(((chn_info->scn_win.height - 1) & 0x1FFF) << 16) : 0);
	reg->pipe.attr[pipe].in_size.dwval = dwval;

	dwval = (chn_info->scn_win.left & 0xFFFF)
		| ((chn_info->scn_win.top & 0xFFFF) << 16);
	reg->pipe.attr[pipe].in_coord.dwval = dwval;

	priv->set_blk_dirty(priv, BLD_REG_BLK_ATTR, 1);
	return 0;
}

s32 de_bld_set_blend_mode(u32 disp, u32 ibld_id,
	enum de_blend_mode mode)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);
	u32 dwval;

	switch (mode) {
	case DE_BLD_MODE_CLEAR:
		dwval = 0x00000000;
		break;
	case DE_BLD_MODE_SRC:
		dwval = 0x00010001;
		break;
	case DE_BLD_MODE_DST:
		dwval = 0x01000100;
		break;
	case DE_BLD_MODE_DSTOVER:
		dwval = 0x01030103;
		break;
	case DE_BLD_MODE_SRCIN:
		dwval = 0x00020002;
		break;
	case DE_BLD_MODE_DSTIN:
		dwval = 0x02000200;
		break;
	case DE_BLD_MODE_SRCOUT:
		dwval = 0x00030003;
		break;
	case DE_BLD_MODE_DSTOUT:
		dwval = 0x03000300;
		break;
	case DE_BLD_MODE_SRCATOP:
		dwval = 0x03020302;
		break;
	case DE_BLD_MODE_DSTATOP:
		dwval = 0x02030203;
		break;
	case DE_BLD_MODE_XOR:
		dwval = 0x03030303;
		break;
	case DE_BLD_MODE_SRCOVER:
	default:
		dwval = 0x03010301;
		break;
	}
	reg->blend_ctl[ibld_id].dwval = dwval;

	priv->set_blk_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

s32 de_bld_disable(u32 disp)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg *reg = get_bld_reg(priv);

	reg->pipe.en.dwval = 0;
	priv->set_blk_dirty(priv, BLD_REG_BLK_ATTR, 1);

	return 0;
}

s32 de_bld_init(u32 disp, u8 __iomem *de_reg_base)
{

	u8 __iomem *reg_base = de_reg_base
		+ DE_DISP_OFFSET(disp) + DISP_BLD_OFFSET;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	struct de_bld_private *priv = &bld_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *block;

	reg_mem_info->size = sizeof(struct bld_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (NULL == reg_mem_info->vir_addr) {
		DE_WARN("alloc bld[%d] mm fail!size=0x%x\n",
		     disp, reg_mem_info->size);
		return -1;
	}

	block = &(priv->reg_blks[BLD_REG_BLK_ATTR]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x60;
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[BLD_REG_BLK_CTL]);
	block->phy_addr = reg_mem_info->phy_addr + 0x80;
	block->vir_addr = reg_mem_info->vir_addr + 0x80;
	block->size = 0x24;
	block->reg_addr = reg_base + 0x80;

	block = &(priv->reg_blks[BLD_REG_BLK_CK]);
	block->phy_addr = reg_mem_info->phy_addr + 0xA0;
	block->vir_addr = reg_mem_info->vir_addr + 0xA0;
	block->size = 0x60;
	block->reg_addr = reg_base + 0xA0;

	priv->reg_blk_num = BLD_REG_BLK_NUM;

	if (rcq_used)
		priv->set_blk_dirty = bld_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = bld_set_block_dirty;

	return 0;
}

s32 de_bld_exit(u32 disp)
{

	struct de_bld_private *priv = &bld_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

	if (reg_mem_info->vir_addr != NULL)
		de_top_reg_memory_free(reg_mem_info->vir_addr,
			reg_mem_info->phy_addr, reg_mem_info->size);

	return 0;
}

s32 de_bld_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_bld_private *priv = &(bld_priv[disp]);
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

static int de_bld_get_enable_info(const struct dump_regs_user_setting *setting, char *buf)
{

	return 0;
}

static int de_bld_get_enable_status(const struct dump_regs_user_setting *setting)
{
	int disp = setting->index.disp;
	struct de_bld_private *priv = &(bld_priv[disp]);
	struct bld_reg __iomem *regs = (struct bld_reg *)(priv->reg_blks[0].reg_addr);
	union bld_en_ctl_reg enr;
	u32 value = 0;
	u32 enable;

	de_lowlevel_ioread(&(regs->pipe.en), (void *)&value, 4);
	enr.dwval = value;
	/* maybe always enable */
	enable = enr.bits.pipe0_fcolor_en | enr.bits.pipe1_fcolor_en | enr.bits.pipe2_fcolor_en	|
		 enr.bits.pipe3_fcolor_en | enr.bits.pipe4_fcolor_en | enr.bits.pipe5_fcolor_en |
		 enr.bits.pipe0_en | enr.bits.pipe1_en | enr.bits.pipe2_en |
		 enr.bits.pipe3_en | enr.bits.pipe4_en | enr.bits.pipe5_en;
	DUMP_REGS_PRINT("blender: disp%d enable%d\n", disp, enable);

	return enable;
}

static int de_bld_dump_by_user_setting(const struct dump_regs_user_setting *setting, char *buf)
{
	unsigned int num = 0;
	struct de_bld_private *priv;
	int i, disp, screen_num, is_enable;

	num += sprintf(buf + num, "%s module:\n\n", dr_bld_priv.mod->name);

	if (setting->use_index_find) {
		disp = setting->index.disp;
		priv = &(bld_priv[disp]);
		is_enable = de_bld_get_enable_status(setting);

		num += sprintf(buf + num, "index(%d) (%s)\n", disp, is_enable ? "enabled" : "disabled");
		if (is_enable || setting->force_dump)
			num += do_dump_regs_block(dr_bld_priv.mod, priv->reg_blks, ARRAY_SIZE(priv->reg_blks), buf + num);
		num += sprintf(buf + num, "\n");
	} else {
		screen_num = de_feat_get_num_screens();
		for (i = 0; i < screen_num; i++) {
			struct dump_regs_user_setting msetting = {.index = {i, 0, 0}};
			priv = &(bld_priv[i]);
			is_enable = de_bld_get_enable_status(&msetting);

			num += sprintf(buf + num, "index(%d) (%s)\n", i, is_enable ? "enabled" : "disabled");
			if (is_enable || setting->force_dump)
				num += do_dump_regs_block(dr_bld_priv.mod, priv->reg_blks, ARRAY_SIZE(priv->reg_blks), buf + num);
			num += sprintf(buf + num, "\n");
		}
	}

	return num;
}

int de_bld_register_dr_module(void)
{
#define BLD_MAX_BLOCK_SIZE (0x60)
	struct dump_regs_module *mod = dr_module_alloc(BLD_MAX_BLOCK_SIZE);;

	dr_bld_priv.mod = mod;

	mod->type = DISP_BLENDER_MODULE;
	strcpy(mod->name, module_name[mod->type]);
	mod->ops.dump_by_user_setting = de_bld_dump_by_user_setting;
	mod->ops.get_enable_info = de_bld_get_enable_info;
	mod->ops.get_enable_status = de_bld_get_enable_status;
	register_dr_module(mod);

	return 0;
}

int de_bld_unregister_dr_module(void)
{
	unregister_dr_module(dr_bld_priv.mod);
	return 0;
}
