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

#include "de_cdc_type.h"
#include "de_cdc_table.h"

#include "../../include.h"
#include "../disp_al_de.h"
#include "de_feat.h"
#include "de_top.h"

#include "de_cdc.h"

enum {
	DE_SDR = 0,
	DE_WCG,
	DE_HDR10,
	DE_HLG,
	DE_HDR10P,
	DE_HDRVIVID,
};

enum {
	CDC_REG_BLK_CTL = 0,
	CDC_REG_BLK_LUT0,
	CDC_REG_BLK_LUT1,
	CDC_REG_BLK_LUT2,
	CDC_REG_BLK_LUT3,
	CDC_REG_BLK_LUT4,
	CDC_REG_BLK_LUT5,
	CDC_REG_BLK_LUT6,
	CDC_REG_BLK_LUT7,
	CDC_REG_BLK_NUM,
};

struct de_cdc_dump_regs_priv {
	struct dump_regs_module *mod;
};

struct de_cdc_dump_regs_priv dr_cdc_priv;

struct de_cdc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[CDC_REG_BLK_NUM];
	enum de_format_space in_px_fmt_space;
	u32 convert_type;
	u8 has_csc;

	void (*set_blk_dirty)(struct de_cdc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_cdc_private cdc_priv[GLB_MAX_CDC_NUM];
static u8 cdc_priv_alloced[GLB_MAX_CDC_NUM];

static struct de_cdc_private *cdc_alloc_private(void)
{
	u32 i = 0;

	for (; i < GLB_MAX_CDC_NUM; i++) {
		if (cdc_priv_alloced[i] == 0) {
			cdc_priv_alloced[i] = 1;
			return &cdc_priv[i];
		}
	}
	DE_WARN("not enough cdc priv to alloc\n");
	return NULL;
}

static s32 cdc_free_private(struct de_cdc_private *priv)
{
	u32 i = 0;

	for (; i < GLB_MAX_CDC_NUM; i++) {
		if (&cdc_priv[i] == priv) {
			memset((void *)priv, 0, sizeof(*priv));
			cdc_priv_alloced[i] = 0;
			return 0;
		}
	}
	DE_WARN("free failed: priv=%p, cdc_priv=%p\n",
		priv, cdc_priv);
	return -1;
}

static inline struct cdc_reg *get_cdc_reg(struct de_cdc_private *priv)
{
	return (struct cdc_reg *)(priv->reg_blks[0].vir_addr);
}

static void cdc_set_block_dirty(
	struct de_cdc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void cdc_set_rcq_head_dirty(
	struct de_cdc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static u32 cdc_get_wcg_type(struct de_csc_info *info, bool isIn)
{
	/*DE_WARN("info->hdr_type =%d, eotf=%d, fmt=%d, cs=%d\n",
		   info->hdr_type, info->eotf,
		   info->px_fmt_space, info->color_space);*/
	if (info->color_space == DE_COLOR_SPACE_BT2020NC
		|| info->color_space == DE_COLOR_SPACE_BT2020C) {
		if (info->eotf == DE_EOTF_SMPTE2084) {
			if (info->hdr_type == HDR10P
				&& info->px_fmt_space == DE_FORMAT_SPACE_YUV
				&& ((isIn && (info->pMeta != NULL)
					 && (info->pMeta->divLut[
						 NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0101
						 || info->pMeta->divLut[
						 NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0102))
					|| !isIn))
				return DE_HDR10P;
			else if (info->hdr_type == HDRVIVID
				&& info->px_fmt_space == DE_FORMAT_SPACE_YUV
				&& ((isIn && (info->pMeta != NULL)
					 && (info->pMeta->divLut[
						 NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0101
						 || info->pMeta->divLut[
						 NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0102))
					|| !isIn)) {
				return DE_HDRVIVID;
			} else
				return DE_HDR10;
		} else if (info->eotf == DE_EOTF_ARIB_STD_B67) {
			if (info->hdr_type == HDRVIVID
				&& info->px_fmt_space == DE_FORMAT_SPACE_YUV
				&& ((isIn && (info->pMeta != NULL)
					 && (info->pMeta->divLut[
						 NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0101
						 || info->pMeta->divLut[
						 NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0102))
					|| !isIn))
				return DE_HDRVIVID;
			else
				return DE_HLG;
		} else
			return DE_WCG;
	} else {
		return DE_SDR;
	}
}

static u32 cdc_get_convert_type(u32 in_type, u32 out_type)
{
	u32 convert_type = DE_TFC_SDR2SDR;

	if (in_type == DE_SDR) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_SDR2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_SDR2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_SDR2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_SDR2HLG;
		else if (out_type == DE_HDR10P || out_type == DE_HDRVIVID)
			convert_type = DE_TFC_SDR2HDR10;
	} else if (in_type == DE_WCG) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_WCG2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_WCG2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_WCG2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_WCG2HLG;
		else if (out_type == DE_HDR10P || out_type == DE_HDRVIVID)
			convert_type = DE_TFC_WCG2HDR10;
	} else if (in_type == DE_HDR10) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HDR102SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_HDR102WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_HDR102HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_HDR102HLG;
		else if (out_type == DE_HDR10P || out_type == DE_HDRVIVID)
			convert_type = DE_TFC_HDR102HDR10;
	} else if (in_type == DE_HLG) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HLG2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_HLG2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_HLG2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_HLG2HLG;
		else if (out_type == DE_HDR10P || out_type == DE_HDRVIVID)
			convert_type = DE_TFC_HLG2HDR10;
	} else if (in_type == DE_HDR10P) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HDR10P2SDR;
		else
			convert_type = DE_TFC_HDR102HDR10;
	} else if (in_type == DE_HDRVIVID) {
		if (out_type == DE_SDR || out_type == DE_HDR10)
			convert_type = DE_TFC_HDRVIVID2SDR;
		else
			convert_type = DE_TFC_HDR102HDR10;
	}

	return convert_type;
}

static u32 cdc_check_bypass(struct de_csc_info *in_info, u32 convert_type)
{
	if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		switch (convert_type) {
		case DE_TFC_SDR2SDR:
		case DE_TFC_WCG2WCG:
		case DE_TFC_HDR102HDR10:
		case DE_TFC_HLG2HLG:
			return 1;
		case DE_TFC_SDR2WCG:
		case DE_TFC_SDR2HDR10:
			return 0;
		default:
			DE_WARN("conversion type no support %d", convert_type);
			return 1;
		}
	} else if (in_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		switch (convert_type) {
		case DE_TFC_SDR2SDR:
		case DE_TFC_WCG2WCG:
		case DE_TFC_HDR102HDR10:
		case DE_TFC_HLG2HLG:
			return 1;
		case DE_TFC_SDR2WCG:
		case DE_TFC_SDR2HDR10:
		case DE_TFC_WCG2SDR:
		case DE_TFC_WCG2HDR10:
		case DE_TFC_HDR102SDR:
		case DE_TFC_HDR102WCG:
		case DE_TFC_HLG2SDR:
		case DE_TFC_HLG2WCG:
		case DE_TFC_HLG2HDR10:
		case DE_TFC_HDR10P2SDR:
		case DE_TFC_HDRVIVID2SDR:
			return 0;
		default:
			DE_WARN("conversion type no support %d", convert_type);
			return 1;
		}
	} else {
		DE_WARN("px_fmt_space %d no support", in_info->px_fmt_space);
		return 1;
	}
}

static void cdc_set_csc_coeff(struct cdc_reg *reg,
	u32 *icsc_coeff, u32 *ocsc_coeff)
{
	u32 dwval0, dwval1, dwval2;

	dwval0 = ((icsc_coeff[12] & 0x80000000) ? (u32)(-(s32)icsc_coeff[12]) : icsc_coeff[12]) & 0x3ff;
	dwval1 = ((icsc_coeff[13] & 0x80000000) ? (u32)(-(s32)icsc_coeff[13]) : icsc_coeff[13]) & 0x3ff;
	dwval2 = ((icsc_coeff[14] & 0x80000000) ? (u32)(-(s32)icsc_coeff[14]) : icsc_coeff[14]) & 0x3ff;

	reg->in_d[0].dwval = dwval0;
	reg->in_d[1].dwval = dwval1;
	reg->in_d[2].dwval = dwval2;
	reg->in_c0[0].dwval = icsc_coeff[0];
	reg->in_c0[1].dwval = icsc_coeff[1];
	reg->in_c0[2].dwval = icsc_coeff[2];
	reg->in_c03.dwval = icsc_coeff[3];
	reg->in_c1[0].dwval = icsc_coeff[4];
	reg->in_c1[1].dwval = icsc_coeff[5];
	reg->in_c1[2].dwval = icsc_coeff[6];
	reg->in_c13.dwval = icsc_coeff[7];
	reg->in_c2[0].dwval = icsc_coeff[8];
	reg->in_c2[1].dwval = icsc_coeff[9];
	reg->in_c2[2].dwval = icsc_coeff[10];
	reg->in_c23.dwval = icsc_coeff[11];

	dwval0 = ((ocsc_coeff[12] & 0x80000000) ? (u32)(-(s32)ocsc_coeff[12]) : ocsc_coeff[12]) & 0x3ff;
	dwval1 = ((ocsc_coeff[13] & 0x80000000) ? (u32)(-(s32)ocsc_coeff[13]) : ocsc_coeff[13]) & 0x3ff;
	dwval2 = ((ocsc_coeff[14] & 0x80000000) ? (u32)(-(s32)ocsc_coeff[14]) : ocsc_coeff[14]) & 0x3ff;

	reg->out_d[0].dwval = dwval0;
	reg->out_d[1].dwval = dwval1;
	reg->out_d[2].dwval = dwval2;
	reg->out_c0[0].dwval = ocsc_coeff[0];
	reg->out_c0[1].dwval = ocsc_coeff[1];
	reg->out_c0[2].dwval = ocsc_coeff[2];
	reg->out_c03.dwval = ocsc_coeff[3];
	reg->out_c1[0].dwval = ocsc_coeff[4];
	reg->out_c1[1].dwval = ocsc_coeff[5];
	reg->out_c1[2].dwval = ocsc_coeff[6];
	reg->out_c13.dwval = ocsc_coeff[7];
	reg->out_c2[0].dwval = ocsc_coeff[8];
	reg->out_c2[1].dwval = ocsc_coeff[9];
	reg->out_c2[2].dwval = ocsc_coeff[10];
	reg->out_c23.dwval = ocsc_coeff[11];
}

static void cdc_set_csc_coeff_hdr10p(struct cdc_reg *reg,
	u32 *icsc_coeff, u32 *ocsc_coeff)
{
	u32 dwval0, dwval1, dwval2;

	dwval0 = (icsc_coeff[12] >= 0) ? (icsc_coeff[12] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[12] & 0x3ff));
	dwval1 = (icsc_coeff[13] >= 0) ? (icsc_coeff[13] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[13] & 0x3ff));
	dwval2 = (icsc_coeff[14] >= 0) ? (icsc_coeff[14] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[14] & 0x3ff));

	reg->in_d[0].dwval = dwval0;
	reg->in_d[1].dwval = dwval1;
	reg->in_d[2].dwval = dwval2;
	reg->in_c0[0].dwval = icsc_coeff[0];
	reg->in_c0[1].dwval = icsc_coeff[1];
	reg->in_c0[2].dwval = icsc_coeff[2];
	reg->in_c03.dwval = icsc_coeff[3];
	reg->in_c1[0].dwval = icsc_coeff[4];
	reg->in_c1[1].dwval = icsc_coeff[5];
	reg->in_c1[2].dwval = icsc_coeff[6];
	reg->in_c13.dwval = icsc_coeff[7];
	reg->in_c2[0].dwval = icsc_coeff[8];
	reg->in_c2[1].dwval = icsc_coeff[9];
	reg->in_c2[2].dwval = icsc_coeff[10];
	reg->in_c23.dwval = icsc_coeff[11];

	dwval0 = (ocsc_coeff[12] >= 0) ? (ocsc_coeff[12] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[12] & 0x3ff));
	dwval1 = (ocsc_coeff[13] >= 0) ? (ocsc_coeff[13] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[13] & 0x3ff));
	dwval2 = (ocsc_coeff[14] >= 0) ? (ocsc_coeff[14] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[14] & 0x3ff));

	reg->out_d[0].dwval = dwval0;
	reg->out_d[1].dwval = dwval1;
	reg->out_d[2].dwval = dwval2;
	reg->out_c0[0].dwval = ocsc_coeff[0];
	reg->out_c0[1].dwval = ocsc_coeff[1];
	reg->out_c0[2].dwval = ocsc_coeff[2];
	reg->out_c03.dwval = ocsc_coeff[3];
	reg->out_c1[0].dwval = ocsc_coeff[4];
	reg->out_c1[1].dwval = ocsc_coeff[5];
	reg->out_c1[2].dwval = ocsc_coeff[6];
	reg->out_c13.dwval = ocsc_coeff[7];
	reg->out_c2[0].dwval = ocsc_coeff[8];
	reg->out_c2[1].dwval = ocsc_coeff[9];
	reg->out_c2[2].dwval = ocsc_coeff[10];
	reg->out_c23.dwval = ocsc_coeff[11];
}

static void cdc_set_csc_coeff_hdrvvd(struct cdc_reg *reg,
	u32 *icsc_coeff, u32 *ocsc_coeff, int maxPix)
{
	u32 dwval0, dwval1, dwval2;
	u32 tmp_csc[12];
	int i;

	dwval0 = (icsc_coeff[12] >= 0) ? (icsc_coeff[12] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[12] & 0x3ff));
	dwval1 = (icsc_coeff[13] >= 0) ? (icsc_coeff[13] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[13] & 0x3ff));
	dwval2 = (icsc_coeff[14] >= 0) ? (icsc_coeff[14] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[14] & 0x3ff));

	reg->in_d[0].dwval = dwval0;
	reg->in_d[1].dwval = dwval1;
	reg->in_d[2].dwval = dwval2;

	for (i = 0; i <= 11; i++) {
		if (icsc_coeff[i] & 0x80000) {
			tmp_csc[i] =  ~(icsc_coeff[i]) + 1;
			tmp_csc[i] = ((tmp_csc[i] & 0x7FFFF) << 10) / maxPix;
			tmp_csc[i] = ~(tmp_csc[i]) + 1;
		} else {
			tmp_csc[i] = icsc_coeff[i];
			tmp_csc[i] = ((tmp_csc[i] & 0x7FFFF) << 10) / maxPix;
		}
	}

	reg->in_c0[0].dwval = tmp_csc[0];
	reg->in_c0[1].dwval = tmp_csc[1];
	reg->in_c0[2].dwval = tmp_csc[2];
	reg->in_c03.dwval = tmp_csc[3];
	reg->in_c1[0].dwval = tmp_csc[4];
	reg->in_c1[1].dwval = tmp_csc[5];
	reg->in_c1[2].dwval = tmp_csc[6];
	reg->in_c13.dwval = tmp_csc[7];
	reg->in_c2[0].dwval = tmp_csc[8];
	reg->in_c2[1].dwval = tmp_csc[9];
	reg->in_c2[2].dwval = tmp_csc[10];
	reg->in_c23.dwval = tmp_csc[11];

	dwval0 = (ocsc_coeff[12] >= 0) ? (ocsc_coeff[12] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[12] & 0x3ff));
	dwval1 = (ocsc_coeff[13] >= 0) ? (ocsc_coeff[13] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[13] & 0x3ff));
	dwval2 = (ocsc_coeff[14] >= 0) ? (ocsc_coeff[14] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[14] & 0x3ff));

	reg->out_d[0].dwval = dwval0;
	reg->out_d[1].dwval = dwval1;
	reg->out_d[2].dwval = dwval2;
	reg->out_c0[0].dwval = ocsc_coeff[0];
	reg->out_c0[1].dwval = ocsc_coeff[1];
	reg->out_c0[2].dwval = ocsc_coeff[2];
	reg->out_c03.dwval = ocsc_coeff[3];
	reg->out_c1[0].dwval = ocsc_coeff[4];
	reg->out_c1[1].dwval = ocsc_coeff[5];
	reg->out_c1[2].dwval = ocsc_coeff[6];
	reg->out_c13.dwval = ocsc_coeff[7];
	reg->out_c2[0].dwval = ocsc_coeff[8];
	reg->out_c2[1].dwval = ocsc_coeff[9];
	reg->out_c2[2].dwval = ocsc_coeff[10];
	reg->out_c23.dwval = ocsc_coeff[11];
}

static void cdc_set_lut(struct cdc_reg *reg, u32 **lut_ptr)
{
	aw_memcpy_toio((void *)reg->lut0, (void *)lut_ptr[0], sizeof(reg->lut0));
	aw_memcpy_toio((void *)reg->lut1, (void *)lut_ptr[1], sizeof(reg->lut1));
	aw_memcpy_toio((void *)reg->lut2, (void *)lut_ptr[2], sizeof(reg->lut2));
	aw_memcpy_toio((void *)reg->lut3, (void *)lut_ptr[3], sizeof(reg->lut3));
	aw_memcpy_toio((void *)reg->lut4, (void *)lut_ptr[4], sizeof(reg->lut4));
	aw_memcpy_toio((void *)reg->lut5, (void *)lut_ptr[5], sizeof(reg->lut5));
	aw_memcpy_toio((void *)reg->lut6, (void *)lut_ptr[6], sizeof(reg->lut6));
	aw_memcpy_toio((void *)reg->lut7, (void *)lut_ptr[7], sizeof(reg->lut7));
}

s32 de_cdc_set_para(void *cdc_hdl,
	struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct cdc_reg *reg = get_cdc_reg(priv);

	u32 in_type, out_type;
	u32 convert_type;
	u32 bypass = 0;

	in_type = cdc_get_wcg_type(in_info, true);
	out_type = cdc_get_wcg_type(out_info, false);
	convert_type = cdc_get_convert_type(in_type, out_type);
	bypass = cdc_check_bypass(in_info, convert_type);

	/* DE_WARN("DE_CDC: in out type %d,%d conver %d bpas %d", */
	/*        in_type, out_type, convert_type, bypass); */

	if (!bypass) {
		u32 *lut_ptr[DE_CDC_LUT_NUM];
		u32 i;
		int hdr10p2sdr_mode = -1;
		int hdrvvd2sdr_mode = -1;

		if (in_info->hdr_type == HDR10P
			&& convert_type == DE_TFC_HDR10P2SDR
			&& in_info->px_fmt_space == DE_FORMAT_SPACE_YUV
			&& in_info->pMeta != NULL) {
			if (in_info->pMeta->divLut[
				NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0101) {
				/*same meta, change csc only*/
				hdr10p2sdr_mode = 1;
			} else if (in_info->pMeta->divLut[
					   NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0102) {
				/*meta change, change csc and cdc*/
				hdr10p2sdr_mode = 2;
				priv->convert_type = DE_TFC_INIT;
			} else {
				/*meta eror, fall back to other mode*/
				hdr10p2sdr_mode = 0;
				priv->convert_type = DE_TFC_INIT;
			}
		} else if (in_info->hdr_type == HDRVIVID
			&& convert_type == DE_TFC_HDRVIVID2SDR
			&& in_info->px_fmt_space == DE_FORMAT_SPACE_YUV
			&& in_info->pMeta != NULL) {
			if (in_info->pMeta->divLut[
				NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0101) {
				/*same meta, change csc only*/
				hdrvvd2sdr_mode = 1;
			} else if (in_info->pMeta->divLut[
					   NUM_DIV - 1][MAX_LUT_SIZE - 1] == 0x0102) {
				/*meta change, change csc and cdc*/
				hdrvvd2sdr_mode = 2;
				priv->convert_type = DE_TFC_INIT;
			} else {
				/*meta eror, fall back to other mode*/
				hdrvvd2sdr_mode = 0;
				priv->convert_type = DE_TFC_INIT;
			}
		}

		/* DE_WARN("DE_CDC: hdrvvd2sdr_mode, hdr10p2sdr_mode=%d, %d", */
		/*        hdrvvd2sdr_mode, hdr10p2sdr_mode); */

		reg->ctl.dwval = 1;
		priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);

		if (priv->has_csc) {
			struct de_csc_info icsc_out, ocsc_in;
			u32 *icsc_coeff, *ocsc_coeff;

			icsc_out.eotf = in_info->eotf;
			memcpy((void *)&ocsc_in, out_info, sizeof(ocsc_in));
			if (in_info->pMeta != NULL
			    && convert_type == DE_TFC_HDRVIVID2SDR
			    && in_info->pMeta->divLut[NUM_DIV - 1][MAX_LUT_SIZE - 3] == 2) {
				/* it's hdrvvd_hlg2 video if divLut[NUM_DIV - 1][MAX_LUT_SIZE - 3] == 2
				 * hdrvvd_hlg2 means hlg video without dynamic metadata
				 * CUVA forbids hlg video enhanced as hdrvvd_hlg2
				 */
				icsc_out.px_fmt_space = DE_FORMAT_SPACE_YUV;
				icsc_out.color_range = DE_COLOR_RANGE_0_255;
				icsc_out.color_space = DE_COLOR_SPACE_BT2020NC;

				ocsc_in.px_fmt_space = DE_FORMAT_SPACE_YUV;
				ocsc_in.color_range = DE_COLOR_RANGE_0_255;
				ocsc_in.color_space = DE_COLOR_SPACE_BT709;
			} else if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
				icsc_out.px_fmt_space = DE_FORMAT_SPACE_RGB;
				icsc_out.color_space = in_info->color_space;
				icsc_out.color_range = in_info->color_range;
			} else if (in_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
				if (hdr10p2sdr_mode >= 1 || hdrvvd2sdr_mode >= 1) {
					icsc_out.px_fmt_space = DE_FORMAT_SPACE_RGB;
					icsc_out.color_range = DE_COLOR_RANGE_0_255;
					icsc_out.color_space = DE_COLOR_SPACE_BT2020NC;

					ocsc_in.px_fmt_space = DE_FORMAT_SPACE_RGB;
					ocsc_in.color_range = DE_COLOR_RANGE_0_255;
					ocsc_in.color_space = DE_COLOR_SPACE_BT709;
				} else {
					icsc_out.px_fmt_space = DE_FORMAT_SPACE_YUV;
					icsc_out.color_range = DE_COLOR_RANGE_16_235;
					if ((in_info->color_space != DE_COLOR_SPACE_BT2020NC)
					    && (in_info->color_space != DE_COLOR_SPACE_BT2020C))
						icsc_out.color_space = DE_COLOR_SPACE_BT709;
					else
						icsc_out.color_space = in_info->color_space;
					ocsc_in.color_range = DE_COLOR_RANGE_16_235;
				}
			} else {
				DE_WARN("px_fmt_space %d no support",
				       in_info->px_fmt_space);
				return -1;
			}
			/* DE_INFO("in: format=%d color_range=%d color_space=%d;\n  out: format =%d color_range=%d color_space=%d\n", */
			/* 		in_info->px_fmt_space, in_info->color_range, in_info->color_space, */
			/* 		icsc_out.px_fmt_space, icsc_out.color_range, icsc_out.color_space); */
			de_csc_coeff_calc(in_info, &icsc_out, &icsc_coeff);
			de_csc_coeff_calc(&ocsc_in, out_info, &ocsc_coeff);
			if (hdrvvd2sdr_mode >= 1
			    && in_info->pMeta->divLut[NUM_DIV - 1][MAX_LUT_SIZE - 3] != 2) {
				cdc_set_csc_coeff_hdrvvd(reg, icsc_coeff, ocsc_coeff,
							 in_info->pMeta->divLut[NUM_DIV - 1][MAX_LUT_SIZE - 2]);
			} else if (hdr10p2sdr_mode >= 1) {
				cdc_set_csc_coeff_hdr10p(reg, icsc_coeff, ocsc_coeff);
			} else {
				cdc_set_csc_coeff(reg, icsc_coeff, ocsc_coeff);
			}
			priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);
		} else {
			out_info->px_fmt_space = in_info->px_fmt_space;
			out_info->eotf = in_info->eotf;
		}

		if ((convert_type == priv->convert_type)
			&& (in_info->px_fmt_space == priv->in_px_fmt_space))
			return 0;

		priv->convert_type = convert_type;
		priv->in_px_fmt_space = in_info->px_fmt_space;
		if (convert_type == DE_TFC_HDR10P2SDR
		    || convert_type == DE_TFC_HDR10P2SDR)
			convert_type = DE_TFC_HDR102SDR;
		if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
			for (i = 0; i < DE_CDC_LUT_NUM; i++) {
				lut_ptr[i] = cdc_lut_ptr_r[convert_type][i];
				priv->set_blk_dirty(priv, CDC_REG_BLK_LUT0 + i, 1);
			}
		} else {
			for (i = 0; i < DE_CDC_LUT_NUM; i++) {
				if (hdr10p2sdr_mode >= 1)
					lut_ptr[i] = (u32 *)(in_info->pMeta->divLut[i]);
				else if (hdrvvd2sdr_mode >= 1) {
					lut_ptr[i] = (u32 *)(in_info->pMeta->divLut[i]);
				} else
					lut_ptr[i] = cdc_lut_ptr_y[convert_type][i];
				priv->set_blk_dirty(priv, CDC_REG_BLK_LUT0 + i, 1);
			}
		}
		cdc_set_lut(reg, (u32 **)lut_ptr);
	} else {
		de_cdc_disable(cdc_hdl);
		memcpy((void *)out_info, (void *)in_info, sizeof(*out_info));
	}

	return bypass;
}

s32 de_cdc_disable(void *cdc_hdl)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct cdc_reg *reg = get_cdc_reg(priv);

	if (reg->ctl.bits.en != 0) {
		reg->ctl.dwval = 0;
		priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);
		priv->convert_type = DE_TFC_INIT;
		priv->in_px_fmt_space = 0xFF;
	}
	return 0;
}

void *de_cdc_create(u8 __iomem *reg_base,
	u32 rcq_used, u8 has_csc)
{
	struct de_cdc_private *priv;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_block *block;

	priv = cdc_alloc_private();
	if (priv == NULL)
		return NULL;

	reg_mem_info = &priv->reg_mem_info;
	reg_mem_info->size = sizeof(struct cdc_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (NULL == reg_mem_info->vir_addr) {
		DE_WARN("alloc for cdc reg mm fail! size=0x%x\n",
		     reg_mem_info->size);
		cdc_free_private(priv);
		return NULL;
	}

	block = &(priv->reg_blks[CDC_REG_BLK_CTL]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = has_csc ? 0x90 : 0x04;
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT0]);
	block->phy_addr = reg_mem_info->phy_addr + 0x1000;
	block->vir_addr = reg_mem_info->vir_addr + 0x1000;
	block->size = 729 * 4;
	block->reg_addr = reg_base + 0x1000;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT1]);
	block->phy_addr = reg_mem_info->phy_addr + 0x1C00;
	block->vir_addr = reg_mem_info->vir_addr + 0x1C00;
	block->size = 648 * 4;
	block->reg_addr = reg_base + 0x1C00;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT2]);
	block->phy_addr = reg_mem_info->phy_addr + 0x2800;
	block->vir_addr = reg_mem_info->vir_addr + 0x2800;
	block->size = 648 * 4;
	block->reg_addr = reg_base + 0x2800;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT3]);
	block->phy_addr = reg_mem_info->phy_addr + 0x3400;
	block->vir_addr = reg_mem_info->vir_addr + 0x3400;
	block->size = 576 * 4;
	block->reg_addr = reg_base + 0x3400;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT4]);
	block->phy_addr = reg_mem_info->phy_addr + 0x4000;
	block->vir_addr = reg_mem_info->vir_addr + 0x4000;
	block->size = 648 * 4;
	block->reg_addr = reg_base + 0x4000;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT5]);
	block->phy_addr = reg_mem_info->phy_addr + 0x4C00;
	block->vir_addr = reg_mem_info->vir_addr + 0x4C00;
	block->size = 576 * 4;
	block->reg_addr = reg_base + 0x4C00;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT6]);
	block->phy_addr = reg_mem_info->phy_addr + 0x5800;
	block->vir_addr = reg_mem_info->vir_addr + 0x5800;
	block->size = 576 * 4;
	block->reg_addr = reg_base + 0x5800;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT7]);
	block->phy_addr = reg_mem_info->phy_addr + 0x6400;
	block->vir_addr = reg_mem_info->vir_addr + 0x6400;
	block->size = 512 * 4;
	block->reg_addr = reg_base + 0x6400;

	priv->reg_blk_num = CDC_REG_BLK_NUM;

	priv->has_csc = has_csc;
	priv->convert_type = DE_TFC_INIT;
	priv->in_px_fmt_space = 0xFF;

	if (rcq_used)
		priv->set_blk_dirty = cdc_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = cdc_set_block_dirty;

	return priv;
}

s32 de_cdc_destroy(void *cdc_hdl)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct de_reg_mem_info *reg_mem_info = &priv->reg_mem_info;

	if (reg_mem_info->vir_addr != NULL)
		de_top_reg_memory_free(reg_mem_info->vir_addr,
			reg_mem_info->phy_addr, reg_mem_info->size);
	cdc_free_private(priv);

	return 0;
}

s32 de_cdc_get_reg_blks(void *cdc_hdl,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct de_reg_block *blk_begin, *blk_end;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num)
		*blk_num = priv->reg_blk_num;
	else
		DE_WARN("should not happen\n");
	blk_begin = priv->reg_blks;
	blk_end = blk_begin + CDC_REG_BLK_NUM;
	for (; blk_begin != blk_end; ++blk_begin) {
		if (blk_begin->size != 0)
			*blks++ = blk_begin;
	}

	return 0;
}

/* lowlevel v33x cdc registration sequence: vch0(0) -> uch0(1) -> wb(2) */
static struct de_cdc_private *de_cdc_get_priv_by_index(int disp, int chn, int wb)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
#define DE_CDC0_PHY_CHN_ID (0) /* VCH0 */
#define DE_CDC1_PHY_CHN_ID (6) /* UCH0 */
#else
#define DE_CDC0_PHY_CHN_ID (0) /* VCH0 */
#define DE_CDC1_PHY_CHN_ID (1) /* VCH1 */
#endif
	int phy_chn;

	if (wb)
		return &(cdc_priv[2]);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	switch (phy_chn) {
	case DE_CDC0_PHY_CHN_ID:
		return &(cdc_priv[0]);
	case DE_CDC1_PHY_CHN_ID:
		return &(cdc_priv[1]);
	default:
		return NULL;
	}
}

static int de_cdc_get_enable_info(const struct dump_regs_user_setting *setting, char *buf)
{

	return 0;
}

static int de_cdc_get_enable_status(const struct dump_regs_user_setting *setting)
{
	int disp = setting->index.disp;
	int chn = setting->index.chn;
	struct de_cdc_private *priv = de_cdc_get_priv_by_index(disp, chn, 0);
	struct cdc_reg __iomem *regs;
	u32 value = 0;
	u32 enable;

	if (!priv)
		return 0;

	regs = (struct cdc_reg *)(priv->reg_blks[0].reg_addr);
	de_lowlevel_ioread(&(regs->ctl), (void *)&value, 4);
	enable = ((union cdc_ctl_reg)value).bits.en & 0x1;
	DUMP_REGS_PRINT("cdc: disp%d chn%d enable%d\n", disp, chn, enable);

	return enable;
}

static int de_cdc_dump_by_user_setting(const struct dump_regs_user_setting *setting, char *buf)
{
	unsigned int num = 0;
	struct de_cdc_private *priv;
	int i, j, disp, chn, screen_num, chn_num, is_enable, is_support = 1;

	num += sprintf(buf + num, "%s module:\n\n", dr_cdc_priv.mod->name);

	if (setting->use_index_find) {
		disp = setting->index.disp;
		chn = setting->index.chn;
		is_enable = de_cdc_get_enable_status(setting);
		priv = de_cdc_get_priv_by_index(disp, chn, 0);
		if (!priv)
			is_support = 0;

		num += sprintf(buf + num, "index(%d, %d) (%s) (%s)\n", disp, chn, is_support ? "supported" : "not supported", is_enable ? "enabled" : "disabled");
		if ((is_enable || setting->force_dump) && is_support)
			num += do_dump_regs_block(dr_cdc_priv.mod, priv->reg_blks, CDC_REG_BLK_CTL + 1, buf + num);
		num += sprintf(buf + num, "\n");
	} else {
		screen_num = de_feat_get_num_screens();
		for (i = 0; i < screen_num; i++) {
			chn_num = de_feat_get_num_chns(i);
			for (j = 0; j < chn_num; j++) {
				struct dump_regs_user_setting msetting = {.index = {i, j, 0}};
				is_enable = de_cdc_get_enable_status(&msetting);
				priv = de_cdc_get_priv_by_index(i, j, 0);
				if (!priv)
					is_support = 0;

				num += sprintf(buf + num, "index(%d, %d) (%s) (%s)\n", i, j, is_support ? "supported" : "not supported", is_enable ? "enabled" : "disabled");
				if ((is_enable || setting->force_dump) && is_support)
					num += do_dump_regs_block(dr_cdc_priv.mod, priv->reg_blks, CDC_REG_BLK_CTL + 1, buf + num);
				num += sprintf(buf + num, "\n");
			}
		}
	}

	return num;
}

int de_cdc_register_dr_module(void)
{
#define CDC_MAX_BLOCK_SIZE (0x90)
	struct dump_regs_module *mod = dr_module_alloc(CDC_MAX_BLOCK_SIZE);

	dr_cdc_priv.mod = mod;

	mod->type = DISP_CDC_MODULE;
	strcpy(mod->name, module_name[mod->type]);
	mod->ops.dump_by_user_setting = de_cdc_dump_by_user_setting;
	mod->ops.get_enable_info = de_cdc_get_enable_info;
	mod->ops.get_enable_status = de_cdc_get_enable_status;
	register_dr_module(mod);

	return 0;
}

int de_cdc_unregister_dr_module(void)
{
	unregister_dr_module(dr_cdc_priv.mod);
	return 0;
}
