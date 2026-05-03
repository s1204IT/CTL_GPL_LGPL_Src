/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved.
 */
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
 *  File name   :  display engine 35x dlc basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_dlc_type.h"
#include "de_enhance.h"
#include "de_rtmx.h"
#include "linux/kernel.h"

enum { DLC_CTL_REG_BLK = 0,
       DLC_PDF_REG_BLK,
       DLC_REG_BLK_NUM,
};

struct dlc_sw_config {
	int limited_color_range;
	int ble_en;
	int normal_gamma;
	int dark_level;
	int dark_dcgain;
	int dark_acgain;
	int bsratio;
	int white_level;
	int white_dcgain;
	int white_acgain;
	int wsratio;
	int uvgain;

	int adl_en;
	int yavg_thrl;
	int yavg_thrm;
	int yavg_thrh;
	int dyn_alpha;
};

struct dlc_status {
	/* Frame number previous tasklet frame_cnt*/
	u32 pre_frame_cnt;
	u32 pre_updata_cnt;
	/* dlc enabled */
	u32 isenable;
};

struct de_dlc_private {
	enum enhance_init_state init_state;
	struct dlc_status status;
	struct dlc_sw_config sw_config;
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DLC_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_dlc_private *priv, u32 blk_id,
			      u32 dirty);
};

#define HIST_BINS (32)

static struct de_dlc_private dlc_priv[DE_NUM][VI_CHN_NUM];
static bool ahb_read_enable;

static int curve_ble[1024];
static int curve_dlc[1024];
static int curve_final[1024];
static int gamma_table[1024];

static int static_curvel[32] = {0,   16,  37,  60,  84,  110, 137, 165,
				194, 223, 254, 284, 316, 347, 380, 413,
				446, 479, 513, 548, 583, 618, 653, 689,
				725, 761, 798, 835, 872, 910, 948, 986};
static int static_curvem[32] = {0,   45,  84,  122, 158, 193, 227, 261,
				294, 327, 359, 392, 424, 455, 487, 518,
				549, 580, 610, 641, 671, 701, 731, 761,
				790, 820, 849, 879, 908, 937, 966, 995};
static int static_curveh[32] = {0,   64,  111, 154, 194, 232, 268, 304,
				338, 371, 404, 436, 467, 498, 529, 559,
				588, 617, 646, 675, 703, 731, 759, 786,
				813, 840, 867, 894, 920, 946, 972, 998};
static int dynamic_limit[32] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
				15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
				15, 15, 15, 15, 15, 15, 15, 15, 15, 15};

static void hist_filter(int *hist, int *histtemp, int histbinnum)
{
	int i;
	histtemp[0] = (hist[0] * 3 + hist[1]) / 4;
	histtemp[histbinnum - 1] =
	    (hist[histbinnum - 1] * 3 + hist[histbinnum - 2]) / 4;
	for (i = 1; i < histbinnum - 1; i++) {
		histtemp[i] = ((hist[i - 1] + 2 * hist[i] + hist[i + 1])) / 4;
	}
}

static void analy_hist(struct dlc_sw_config *config, int *histtemp,
		       int histbinnum, int *blevel, int *wlevel, int *avgl)
{
	int sum = 0, i, w_thr, sumbin;
	*blevel = 0;
	*wlevel = 31;
	// cal the blevel
	for (i = 0; i < histbinnum; i++) {
		sum = sum + histtemp[i];
		if (sum > 1024 * config->dark_level / 100) {
			*blevel = i;
			break;
		}
	}

	// cal the wlevel
	sum = 0;
	w_thr = 1024 - 1024 * config->white_level / 100;
	for (i = 0; i < histbinnum; i++) {
		sum = sum + histtemp[i];
		if (sum > w_thr) {
			*wlevel = i;
			break;
		}
	}

	sum = 0;
	sumbin = 0;
	// cal the avg
	for (i = 0; i < histbinnum; i++) {
		sum += (int)(histtemp[i] * (i * 32 + 16));
		sumbin += (int)(histtemp[i]);
	}

	if (*blevel >= 10)
		*blevel = 10;
	if (*wlevel <= 16)
		*wlevel = 16;
}

static void cal_bscurve(struct dlc_sw_config *config, int *histtemp, int blevel,
			int histbinnum, int *curveb)
{
	int bs_offsethigh = (int)(blevel * 2 / 3);
	int bs_offsetlow = (int)(blevel / 5);
	int bs_offindexhigh = 10;
	int bs_offindexlow = 1;
	int sum1 = 0;
	int sum2 = 0;
	int blackindex = 0;
	int blcak_offset;
	int cdf[32] = {0};
	int step;
	int curvedc[32] = {0};
	int curveac[32] = {0};
	int i;

	for (i = 0; i < blevel; i++) {
		sum1 += histtemp[i] - histtemp[i] * i / blevel;
		sum2 += histtemp[i];
	}
	if (sum2 != 0) {
		blackindex = (int)((sum1 * 5 + (sum2 >> 1)) / sum2);
	} else {
		blackindex = 3;
	}

	blcak_offset =
	    (int)(bs_offsetlow + (bs_offsethigh - bs_offsetlow) *
				     (bs_offindexhigh - blackindex) /
				     (bs_offindexhigh - bs_offindexlow));
	/* get the cdf */
	for (i = 1; i < histbinnum; i++) {
		cdf[i] = cdf[i - 1] + histtemp[i - 1];
	}

	/* get the dccurve */
	step = 1024 / histbinnum;
	for (i = blcak_offset; i <= blevel; i++) {
		curvedc[i] =
		    config->white_dcgain * (i - blcak_offset + 1) * step / 100;
	}
	for (i = 0; i <= blevel; i++) {
		curveac[i] = curvedc[i] + cdf[i] * config->white_acgain / 100;
		curveb[i] = (curvedc[i] - curvedc[i] * config->bsratio / 100 +
			     curveac[i] * config->bsratio / 100);
	}
}

static void cal_wscurve(struct dlc_sw_config *config, int *histtemp, int wlevel,
			int histbinnum, int *curvew)
{

	int ws_offsethigh = (int)((histbinnum - wlevel) / 2);
	int ws_offsetlow = (int)((histbinnum - wlevel) / 4);
	int ws_offindexhigh = 10;
	int ws_offindexlow = 3;
	int sum1 = 0;
	int sum2 = 0;
	int whiteindex = 0;
	int step;
	int curvedc[32] = {0};
	int curveac[32] = {0};
	int cdf[32] = {0};
	int white_offset;
	int i;

	for (i = wlevel; i < histbinnum; i++) {
		sum1 += histtemp[i] * (i - wlevel + 1) *
			(1 / (histbinnum - wlevel + 1));
		sum2 += histtemp[i];
	}
	if (sum2 != 0) {
		whiteindex = (sum1 * 30 / sum2);
	} else {
		whiteindex = ws_offindexlow;
	}
	if (whiteindex <= ws_offindexlow)
		whiteindex = ws_offindexlow;
	if (whiteindex > ws_offindexhigh)
		whiteindex = ws_offindexhigh;

	white_offset =
	    (int)(histbinnum - ((ws_offindexhigh - whiteindex) *
				    (ws_offsethigh - ws_offsetlow) /
				    (ws_offindexhigh - ws_offindexlow) +
				ws_offsetlow));

	// get the dccurve
	// get the cdf
	for (i = 1; i < histbinnum; i++) {
		cdf[i] = cdf[i - 1] + histtemp[i - 1];
	}

	// get the dc and ac curve
	step = 1024 / histbinnum;
	for (i = wlevel; i < white_offset; i++) {
		curvedc[i] = 1023 - config->white_dcgain * (white_offset - i) *
					step / 100;
	}
	for (i = white_offset; i < histbinnum; i++) {
		curvedc[i] = 1023;
	}

	for (i = wlevel; i < histbinnum; i++) {
		curveac[i] =
		    curvedc[i] - (1024 - cdf[i]) * config->white_acgain / 100;
		curvew[i] = ((curvedc[i] - curvedc[i] * config->wsratio / 100) +
			     curveac[i] * config->wsratio / 100);
	}
}

static void cal_normal_curve(struct dlc_sw_config *config, int *histtemp,
			     int blevel, int blevelout, int wlevel,
			     int wlevelout, int *curvenormal)
{
	int gamma = config->normal_gamma;
	short *table = g_gamma_funcs[gamma];
	int xdistance = wlevel - blevel;
	int ydistance = wlevelout - blevelout;
	int i, x, y;

	if (xdistance > 0) {
		for (i = blevel + 1; i < wlevel; i++) {
			x = (i - blevel) * 1024 / xdistance;
			y = table[x] * ydistance / 1024 + blevelout;
			curvenormal[i] = y;
		}
	}
}

static void color_range(struct dlc_sw_config *config, int *curveidx,
			int *curveout, int histbinnum)
{
	int step = 1024 / histbinnum;
	int i, xidx, yout;

	for (i = 0; i < histbinnum; i++) {
		xidx = i * step;
		if (config->limited_color_range == 1) {
			xidx = ((xidx * 219 + 128) >> 8) + 64;
			/* yout = (int)((curveout[i]) * 1023); */
			yout = (int)((curveout[i]));
			yout = ((yout * 219 + 128) >> 8) + 64;
			clamp(xidx, 64, 940);
			clamp(yout, 64, 940);
			curveidx[i] = xidx;
			curveout[i] = yout;
		} else {
			curveidx[i] = xidx;
		}
	}
}

static void interp2(int *curveidx, int *curveout, int histbinnum, int *curve,
		    int curvebinnum)
{
	int i, idx, bin_index, step;

	for (idx = 0; idx < curvebinnum; idx++) {
		if (idx >= curveidx[31] && idx <= 1023) {
			i = 31;
		} else if (idx < curveidx[0]) {
			i = -1;
		} else {
			for (i = 0; i < histbinnum; i++) {
				if (idx >= curveidx[i] && idx < curveidx[i + 1])
					break;
			}
		}

		bin_index = i;
		if (bin_index < 31 && bin_index >= 0) {
			step = idx - curveidx[bin_index];
			curve[idx] = (int)((
			    curveout[bin_index] +
			    (curveout[bin_index + 1] - curveout[bin_index]) *
				step /
				(curveidx[bin_index + 1] -
				 curveidx[bin_index])));
		} else if (bin_index == 31) {
			step = idx - curveidx[31];
			curve[idx] = (int)((curveout[bin_index] +
					    (1023 - curveout[31]) * step /
						(1023 - curveidx[31])));
		} else {
			step = idx;
			curve[idx] =
			    (int)((curveout[0]) * step / (curveidx[0]));
		}
	}
}

static void get_ble_curve_from_hist(struct dlc_sw_config *config, int *hist,
				    int histbinnum, int *curve, int curvebinnum)
{
	int histtemp[32] = {0};
	int curveb[32] = {0};
	int curvew[32] = {0};
	int curveout[32] = {0};
	int curvenormal[32] = {0};

	int blevel = 0;
	int wlevel = 0;
	int avgl = 0;
	int blevel_out;
	int wlevel_out;
	int curveidx[32] = {0};
	int i;

	hist_filter(hist, histtemp, histbinnum);
	analy_hist(config, histtemp, histbinnum, &blevel, &wlevel, &avgl);
	if (blevel != 0) {
		cal_bscurve(config, histtemp, blevel, histbinnum, curveb);
	}
	if (wlevel != 0) {
		cal_wscurve(config, histtemp, wlevel, histbinnum, curvew);
	}

	blevel_out = curveb[blevel];
	wlevel_out = curvew[wlevel];
	cal_normal_curve(config, histtemp, blevel, blevel_out, wlevel,
			 wlevel_out, curvenormal);
	for (i = 0; i < histbinnum; i++) {
		curveout[i] = curveb[i] + curvew[i] + curvenormal[i];
	}

	/* Debug info */
	/*
	trace_printk("%s: curveb :\n", __func__);
	for (int i = 0; i < (histbinnum / 4); i++) {
		trace_printk("%8d %8d %8d %8d\n", curveb[i * 4 + 0], curveb[i *
	4 + 1], curveb[i * 4 + 2], curveb[i * 4 + 3]);
	}

	trace_printk("%s: curvew :\n", __func__);
	for (int i = 0; i < (histbinnum / 4); i++) {
		trace_printk("%8d %8d %8d %8d\n", curvew[i * 4 + 0], curvew[i *
	4 + 1], curvew[i * 4 + 2], curvew[i * 4 + 3]);
	}

	trace_printk("%s: curvenormal :\n", __func__);
	for (int i = 0; i < (histbinnum / 4); i++) {
		trace_printk("%8d %8d %8d %8d\n", curvenormal[i * 4 + 0],
	curvenormal[i * 4 + 1], curvenormal[i * 4 + 2], curvenormal[i * 4 + 3]);
	}
	*/

	/* change curve to suit for color range */
	color_range(config, curveidx, curveout, histbinnum);
	interp2(curveidx, curveout, histbinnum, curve, curvebinnum);
}

static int cal_yavg_from_hist(int *histtemp, int histbinnum)
{
	int yavg = 0;
	int step = 1024 / histbinnum;
	int sum = 0;
	int sumbin = 0;
	int i = 0;

	// cal the avg
	for (i = 0; i < histbinnum; i++) {
		sum += (int)(histtemp[i] * (i * step + step / 2));
		sumbin += (int)(histtemp[i]);
	}
	yavg = sum / sumbin;
	return yavg;
}

static void get_static_curve(struct dlc_sw_config *config, int yavg,
			     int *staticcurve, int histbinnum)
{
	int i;

	if (yavg <= config->yavg_thrl) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] = static_curvel[i];
		}
	} else if (yavg >= config->yavg_thrh) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] = static_curveh[i];
		}
	} else if (yavg > config->yavg_thrl && yavg <= config->yavg_thrm) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] =
			    static_curvel[i] -
			    static_curvel[i] *
				((yavg - config->yavg_thrl) /
				 (config->yavg_thrm - config->yavg_thrl)) +
			    static_curvem[i] *
				((yavg - config->yavg_thrl) /
				 (config->yavg_thrm - config->yavg_thrl));
		}
	} else if (yavg > config->yavg_thrm && yavg < config->yavg_thrh) {
		for (i = 0; i < histbinnum; i++) {
			staticcurve[i] =
			    static_curvem[i] -
			    static_curvem[i] *
				((yavg - config->yavg_thrm) /
				 (config->yavg_thrh - config->yavg_thrm)) +
			    static_curveh[i] *
				((yavg - config->yavg_thrm) /
				 (config->yavg_thrh - config->yavg_thrm));
		}
	}
}

static void get_dynamic_curve(int *histtemp, int histbinnum, int *dynamiccurve)
{
	int step = 1024 / histbinnum;
	int sumd = 0;
	int suma = 0;
	int i, max;
	for (i = 0; i < histbinnum; i++) {
		max = dynamic_limit[i] * step / 10;
		if (histtemp[i] > max) {
			sumd += histtemp[i] - max;
			histtemp[i] = max;
		} else {
			suma += max - histtemp[i];
		}
	}
	for (i = 0; i < histbinnum; i++) {
		max = dynamic_limit[i] * step / 10;
		if (histtemp[i] < max) {
			histtemp[i] += sumd * (max - histtemp[i]) / suma;
		}
	}
	dynamiccurve[0] = 0;
	for (i = 1; i < histbinnum; i++) {
		dynamiccurve[i] = dynamiccurve[i - 1] + histtemp[i - 1];
	}
}

static void merge_curve(struct dlc_sw_config *config, int *staticcurve,
			int *dynamiccurve, int *curveout, int histbinnum)
{
	int i;
	for (i = 0; i < histbinnum; i++) {
		curveout[i] = (staticcurve[i] * (100 - config->dyn_alpha) +
			       dynamiccurve[i] * config->dyn_alpha) /
			      (100);
	}
}

static void get_dlc_curve_from_hist(struct dlc_sw_config *config, int *hist,
				    int histbinnum, int *curve, int curvebinnum)
{
	int staticcurve[32] = {0};
	int dynamiccurve[32] = {0};
	int curveout[32] = {0};
	int curveidx[32] = {0};
	int yavg = cal_yavg_from_hist(hist, histbinnum);
	get_static_curve(config, yavg, staticcurve, histbinnum);
	get_dynamic_curve(hist, histbinnum, dynamiccurve);
	merge_curve(config, staticcurve, dynamiccurve, curveout, histbinnum);
	color_range(config, curveidx, curveout, histbinnum);
	interp2(curveidx, curveout, histbinnum, curve, curvebinnum);
}

static inline struct dlc_reg *get_dlc_reg(struct de_dlc_private *priv)
{
	return (struct dlc_reg *)(priv->reg_blks[DLC_CTL_REG_BLK].vir_addr);
}

static void dlc_set_block_dirty(struct de_dlc_private *priv, u32 blk_id,
				u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void dlc_set_rcq_head_dirty(struct de_dlc_private *priv, u32 blk_id,
				   u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_dlc_init(u32 disp, u32 chn, uintptr_t reg_base, u8 __iomem **phy_addr,
		u8 **vir_addr, u32 *size)
{
	struct de_dlc_private *priv = &dlc_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_DLC_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_DLC_REG_MEM_SIZE;

	priv->reg_blk_num = DLC_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[DLC_CTL_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x10;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[DLC_PDF_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x10;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x10;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x10);

	*phy_addr += DE_DLC_REG_MEM_SIZE;
	*vir_addr += DE_DLC_REG_MEM_SIZE;
	*size -= DE_DLC_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = dlc_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = dlc_set_block_dirty;

	priv->init_state = ENHANCE_INVALID;

	return 0;
}

s32 de_dlc_exit(u32 disp, u32 chn) { return 0; }

s32 de_dlc_get_reg_blocks(u32 disp, u32 chn, struct de_reg_block **blks,
			  u32 *blk_num)
{
	struct de_dlc_private *priv = &(dlc_priv[disp][chn]);
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

s32 de_dlc_set_size(u32 disp, u32 chn, u32 width, u32 height)
{
	struct de_dlc_private *priv = &(dlc_priv[disp][chn]);
	struct dlc_reg *reg = get_dlc_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, DLC_CTL_REG_BLK, 1);

	return 0;
}

s32 de_dlc_enable(u32 disp, u32 chn, u32 en)
{
	struct de_dlc_private *priv = &(dlc_priv[disp][chn]);
	struct dlc_reg *reg = get_dlc_reg(priv);

	/* if (priv->init_state == ENHANCE_TIGERLCD_ON && en == 1) */
	/* en = 1; */
	/* else */
	/* en = 0; */

#ifdef SUPPORT_AHB_READ
	reg->ctl.bits.en = en;
	priv->status.isenable = en;
#else
	reg->ctl.bits.en = 0;
	priv->status.isenable = 0;
#endif
	priv->set_blk_dirty(priv, DLC_CTL_REG_BLK, 1);

	return 0;
}

s32 de_dlc_set_corlor_range(u32 disp, u32 chn, enum de_color_range cr)
{
	if (de_feat_is_support_dlc_by_chn(disp, chn)) {
		struct de_dlc_private *priv = &(dlc_priv[disp][chn]);
		struct dlc_reg *reg = get_dlc_reg(priv);

		reg->color_range.dwval = (cr == DE_COLOR_RANGE_16_235 ? 1 : 0);
		priv->set_blk_dirty(priv, DLC_CTL_REG_BLK, 1);
	}

	return 0;
}

s32 de_dlc_init_para(u32 disp, u32 chn)
{
	struct de_dlc_private *priv = &(dlc_priv[disp][chn]);

	if (priv->init_state >= ENHANCE_INITED) {
		return 0;
	}
	priv->init_state = ENHANCE_INITED;

	priv->sw_config.limited_color_range = 1;
	priv->sw_config.ble_en = 1;
	priv->sw_config.normal_gamma = 8;
	priv->sw_config.dark_level = 25;
	priv->sw_config.dark_dcgain = 80;
	priv->sw_config.dark_acgain = 80;
	priv->sw_config.bsratio = 40;
	priv->sw_config.white_level = 15;
	priv->sw_config.white_dcgain = 120;
	priv->sw_config.white_acgain = 100;
	priv->sw_config.wsratio = 70;
	priv->sw_config.uvgain = 12;

	priv->sw_config.adl_en = 1;
	priv->sw_config.yavg_thrl = 200;
	priv->sw_config.yavg_thrm = 400;
	priv->sw_config.yavg_thrh = 700;
	priv->sw_config.dyn_alpha = 40;

	return 0;
}

/* fetch frame 0 dlc data, calculate in frame 1, act on frame 2 */
s32 de_dlc_tasklet(u32 disp, u32 chn, u32 frame_cnt, u32 frame_updata_cnt)
{
	struct de_dlc_private *priv = &(dlc_priv[disp][chn]);
	struct dlc_reg *hw_reg = (struct dlc_reg *)(priv->reg_blks[0].reg_addr);
	int *cur_cdfs = (int *)priv->reg_blks[DLC_PDF_REG_BLK].vir_addr;
	int temp, i;

	if (priv->status.isenable < 1 || !ahb_read_enable)
		return 0;

	/* Debug info */
	/* trace_printk("dlc tasklet start, updatacnt[pre %d, cur %d]
	   frame_cnt[pre %d, cur %d]\n",
	   priv->status.pre_updata_cnt, frame_updata_cnt,
	   priv->status.pre_frame_cnt, frame_cnt);
	*/
	if (priv->status.pre_updata_cnt == 0) {
		priv->status.pre_updata_cnt = frame_updata_cnt;
		priv->status.pre_frame_cnt = frame_cnt;
		return 0;
	} else if (frame_updata_cnt - priv->status.pre_updata_cnt > 1 ||
		   frame_cnt == priv->status.pre_frame_cnt) {
		DE_WARN("tasklet scheduling is not timely,\
			updatacnt[pre %d, cur %d] frame_cnt[pre %d, cur %d]\n",
			priv->status.pre_updata_cnt, frame_updata_cnt,
			priv->status.pre_frame_cnt, frame_cnt);
		/* reset  */
	}
	priv->status.pre_updata_cnt = frame_updata_cnt;
	priv->status.pre_frame_cnt = frame_cnt;

	aw_memcpy_fromio(cur_cdfs, hw_reg->pdf_config,
			 sizeof(hw_reg->pdf_config));

	if (priv->sw_config.ble_en == 1)
		get_ble_curve_from_hist(&(priv->sw_config), cur_cdfs, HIST_BINS,
					curve_ble, 1024);
	if (priv->sw_config.adl_en == 1)
		get_dlc_curve_from_hist(&(priv->sw_config), cur_cdfs, HIST_BINS,
					curve_dlc, 1024);

	for (i = 0; i < 1024; i++) {
		temp = i;
		curve_final[i] = i;
		if (priv->sw_config.ble_en == 1) {
			temp = curve_ble[i];
			curve_final[i] = curve_ble[i];
		}
		if (priv->sw_config.adl_en == 1) {
			if (temp >= 1024) {
				DE_WARN("%s: i %d temp index %d over range\n",
					__func__, i, temp);
				curve_final[i] = curve_dlc[1023];
				continue;
			}
			curve_final[i] = curve_dlc[temp];
		}
	}

	for (i = 0; i < 1024; i++) {
		u32 back_shift[3] = {20, 10, 0};
		int temp_ratio = (i - 512) * priv->sw_config.uvgain / 10 + 512;

		if (temp_ratio >= 1023)
			temp_ratio = 1023;
		if (temp_ratio <= 0)
			temp_ratio = 0;
		gamma_table[i] = (curve_final[i] & 0x3ff) << back_shift[0] |
				 (temp_ratio & 0x3ff) << back_shift[1] |
				 (temp_ratio & 0x3ff) << back_shift[2];
	}

	de_gamma_set_table(disp, chn, CHN_GAMMA_TYPE, 1, 1, gamma_table);

	/* Debug info */
	/*
	trace_printk("%s: dlc curve_ble:\n", __func__);
	for (i = 0; i < 256; i++) {
		trace_printk("0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
		curve_ble[i * 4 + 0], curve_ble[i * 4 + 1], curve_ble[i * 4 +
	2], curve_ble[i * 4 + 3]);
	}

	trace_printk("%s: dlc curve_dlc:\n", __func__);
	for (i = 0; i < 256; i++) {
		trace_printk("0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
		curve_dlc[i * 4 + 0], curve_dlc[i * 4 + 1], curve_dlc[i * 4 +
	2], curve_dlc[i * 4 + 3]);
	}

	trace_printk("%s: dlc curve_final table:\n", __func__);
	for (i = 0; i < 256; i++) {
		trace_printk("0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
		curve_final[i * 4 + 0], curve_final[i * 4 + 1], curve_final[i *
	4 + 2], curve_final[i * 4 + 3]);
	}

	trace_printk("%s: dlc gamma table:\n", __func__);
	for (i = 0; i < 256; i++) {
		trace_printk("0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
		gamma_table[i * 4 + 0], gamma_table[i * 4 + 1], gamma_table[i *
	4 + 2], gamma_table[i * 4 + 3]);
	}
	*/

	return 0;
}

int de_dlc_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data) { return 0; }

s32 de_dlc_enable_ahb_read(u32 disp, bool en)
{
#ifdef SUPPORT_AHB_READ
	DE_WARN("de_dlc_enable_ahb_read=%d\n", en);
	ahb_read_enable = en;
#else
	DE_WARN("force de_dlc_enable_ahb_read=%d to 0\n", en);
	ahb_read_enable = 0;
#endif
	return 0;
}

