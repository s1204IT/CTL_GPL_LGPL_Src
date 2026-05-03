/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * de210_feat.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
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
 * chn_id_lut:0~2 reserve for video channel, 3~5 reserve for ui channel
 *
 */

#include "de_feat.h"

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
static const u32 sun55iw6_mode0_num_de_disps[] = {
	1, /* RTMX0 */
};
static const u32 sun55iw6_mode0_skip_disps[] = {
	0, /* DISP0 */
};
static const s32 sun55iw6_mode0_num_chns[] = {
	3, /* DISP0 */
};
static const s32 sun55iw6_mode0_num_vi_chns[] = {
	1, /* DISP0 */
};
static const s32 sun55iw6_mode0_num_layers[] = {
	4, 4, 4, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_vep[] = {
	1, 0, 0,/* DISP0 */
};
static const s32 sun55iw6_mode0_num_csc2_chns[] = {
	2, 0, 0, /* DISP0 */
};
static const s32 sun55iw6_mode0_num_csc2_disps[] = {
	1, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_smbl[] = {
	0, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_deband[] = {
	0, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_dither[] = {
	1, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_gamma[] = {
	1, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_fmt[] = {
	0, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_ksc[] = {
	0, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_wb[] = {
	1, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_crc[] = {
	0, /* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_scale[] = {
	1, 1, 1, /* DISP0 */
};
static const s32 sun55iw6_mode0_scale_line_buffer[] = {
	2048, /* DISP0 */
};
static const s32 sun55iw6_mode0_scale_line_buffer_yuv[] = {
	2048, 2048, 2048, /* DISP0 */
};
static const s32 sun55iw6_mode0_scale_line_buffer_rgb[] = {
	2048, 2048, 2048, /* DISP0 */
};
static const s32 sun55iw6_mode0_scale_line_buffer_ed[] = {
	2048, 2048, 2048,/* DISP0 */
};
static const s32 sun55iw6_mode0_is_support_edscale[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_asuscale[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_lti[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_peak[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_dlc[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_chn_gamma[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_dci[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_sharp[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_fcm[] = {
	1, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_de_noise[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_cdc[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_snr[] = {
	0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_tfbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_lbc[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
};
static const s32 sun55iw6_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
	/* DISP0 CH2 */
	0, 0, 0, 0,
};
static const u32 sun55iw6_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU10,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_GSU,
	/* DISP0 CH2 */
	DE_SCALER_TYPE_GSU,
};
static const s32 sun55iw6_mode0_chn_id_lut[] = {
	0,    /* DISP0, VIDEO CHANNEL 0~2 */
	3, 4, /* DISP0, UI CHANNEL 3~5 */
};

const struct de_feat sun55iw6_de_features = {
	.num_screens = 1,
	.num_de_rtmx = 1,
	.num_de_disps = sun55iw6_mode0_num_de_disps,
	.num_skip_disps = sun55iw6_mode0_skip_disps,
	.num_chns = sun55iw6_mode0_num_chns,
	.num_vi_chns = sun55iw6_mode0_num_vi_chns,
	.num_layers = sun55iw6_mode0_num_layers,
	.num_csc2_chns = sun55iw6_mode0_num_csc2_chns,
	.num_csc2_disps = sun55iw6_mode0_num_csc2_disps,
	.is_support_vep = sun55iw6_mode0_is_support_vep,
	.is_support_smbl = sun55iw6_mode0_is_support_smbl,
	.is_support_deband = sun55iw6_mode0_is_support_deband,
	.is_support_gamma = sun55iw6_mode0_is_support_gamma,
	.is_support_dither = sun55iw6_mode0_is_support_dither,
	.is_support_fmt = sun55iw6_mode0_is_support_fmt,
	.is_support_ksc = sun55iw6_mode0_is_support_ksc,
	.is_support_wb = sun55iw6_mode0_is_support_wb,
	.is_support_crc = sun55iw6_mode0_is_support_crc,
	.is_support_scale = sun55iw6_mode0_is_support_scale,
	.scale_line_buffer_yuv = sun55iw6_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = sun55iw6_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = sun55iw6_mode0_scale_line_buffer_ed,
	.is_support_edscale = sun55iw6_mode0_is_support_edscale,
	.is_support_asuscale = sun55iw6_mode0_is_support_asuscale,
	.is_support_lti = sun55iw6_mode0_is_support_lti,
	.is_support_peak = sun55iw6_mode0_is_support_peak,
	.is_support_dlc = sun55iw6_mode0_is_support_dlc,
	.is_support_chn_gamma = sun55iw6_mode0_is_support_chn_gamma,
	.is_support_dci = sun55iw6_mode0_is_support_dci,
	.is_support_fcm = sun55iw6_mode0_is_support_fcm,
	.is_support_sharp = sun55iw6_mode0_is_support_sharp,
	.is_support_de_noise = sun55iw6_mode0_is_support_de_noise,
	.is_support_cdc = sun55iw6_mode0_is_support_cdc,
	.is_support_fbd = sun55iw6_mode0_is_support_fbd,
	.is_support_lbc = sun55iw6_mode0_is_support_lbc,
	.is_support_tfbd = sun55iw6_mode0_is_support_tfbd,
	.is_support_atw = sun55iw6_mode0_is_support_atw,
	.is_support_snr = sun55iw6_mode0_is_support_snr,
	.scaler_type = sun55iw6_mode0_scaler_type,
	.chn_id_lut = sun55iw6_mode0_chn_id_lut,
};

#elif IS_ENABLED(CONFIG_MACH_SUN251IW1)
static const u32 sun251iw1_mode0_num_de_disps[] = {
	1, /* RTMX0 */
};
static const u32 sun251iw1_mode0_skip_disps[] = {
	0, /* DISP0 */
};
static const s32 sun251iw1_mode0_num_chns[] = {
	2, /* DISP0 */
};
static const s32 sun251iw1_mode0_num_vi_chns[] = {
	2, /* DISP0 */
};
static const s32 sun251iw1_mode0_num_layers[] = {
	4, 4, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_vep[] = {
	1, 0, /* DISP0 */
};
static const s32 sun251iw1_mode0_num_csc2_chns[] = {
	2, 1, /* DISP0 */
};
static const s32 sun251iw1_mode0_num_csc2_disps[] = {
	1, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_smbl[] = {
	0, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_deband[] = {
	0, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_dither[] = {
	1, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_gamma[] = {
	1, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_fmt[] = {
	0, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_ksc[] = {
	1, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_wb[] = {
	1, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_crc[] = {
	0, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_scale[] = {
	1, 1, /* DISP0 */
};
static const s32 sun251iw1_mode0_scale_line_buffer_yuv[] = {
	2048, 2048, /* DISP0 */
};
static const s32 sun251iw1_mode0_scale_line_buffer_rgb[] = {
	2048, 2048, /* DISP0 */
};
static const s32 sun251iw1_mode0_scale_line_buffer_ed[] = {
	2048, 2048, /* DISP0 */
};
static const s32 sun251iw1_mode0_is_support_edscale[] = {
	0, 0,
};
static const s32 sun251iw1_mode0_is_support_asuscale[] = {
	0, 0,
};
static const s32 sun251iw1_mode0_is_support_lti[] = {
	1, 0,
};
static const s32 sun251iw1_mode0_is_support_peak[] = {
	1, 0,
};
static const s32 sun251iw1_mode0_is_support_dlc[] = {
	1, 0,
};
static const s32 sun251iw1_mode0_is_support_chn_gamma[] = {
	1, 0,
};
static const s32 sun251iw1_mode0_is_support_dci[] = { /* dci(ftd)*/
	1, 0,
};
static const s32 sun251iw1_mode0_is_support_sharp[] = {
	0, 0,
};
static const s32 sun251iw1_mode0_is_support_fcm[] = {
	0, 0,
};
static const s32 sun251iw1_mode0_is_support_de_noise[] = {
	0, 0,
};
static const s32 sun251iw1_mode0_is_support_cdc[] = {
	0, 0,
};
static const s32 sun251iw1_mode0_is_support_snr[] = {
	1, 0,
};
static const s32 sun251iw1_mode0_is_support_fbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
};
static const s32 sun251iw1_mode0_is_support_tfbd[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
};
static const s32 sun251iw1_mode0_is_support_lbc[] = {
	/* DISP0 CH0 */
	1, 0, 0, 0,
	/* DISP0 CH1 */
	1, 0, 0, 0,
};
static const s32 sun251iw1_mode0_is_support_atw[] = {
	/* DISP0 CH0 */
	0, 0, 0, 0,
	/* DISP0 CH1 */
	0, 0, 0, 0,
};
static const u32 sun251iw1_mode0_scaler_type[] = {
	/* DISP0 CH0 */
	DE_SCALER_TYPE_VSU10,
	/* DISP0 CH1 */
	DE_SCALER_TYPE_VSU10,
};
static const s32 sun251iw1_mode0_chn_id_lut[] = {
	0,    /* DISP0, VIDEO CHANNEL 0~2 */
	1,
};

const struct de_feat sun251iw1_de_features = {
	.num_screens = 1,
	.num_de_rtmx = 1,
	.num_de_disps = sun251iw1_mode0_num_de_disps,
	.num_skip_disps = sun251iw1_mode0_skip_disps,
	.num_chns = sun251iw1_mode0_num_chns,
	.num_vi_chns = sun251iw1_mode0_num_vi_chns,
	.num_layers = sun251iw1_mode0_num_layers,
	.num_csc2_chns = sun251iw1_mode0_num_csc2_chns,
	.num_csc2_disps = sun251iw1_mode0_num_csc2_disps,
	.is_support_vep = sun251iw1_mode0_is_support_vep,
	.is_support_smbl = sun251iw1_mode0_is_support_smbl,
	.is_support_deband = sun251iw1_mode0_is_support_deband,
	.is_support_gamma = sun251iw1_mode0_is_support_gamma,
	.is_support_dither = sun251iw1_mode0_is_support_dither,
	.is_support_fmt = sun251iw1_mode0_is_support_fmt,
	.is_support_ksc = sun251iw1_mode0_is_support_ksc,
	.is_support_wb = sun251iw1_mode0_is_support_wb,
	.is_support_crc = sun251iw1_mode0_is_support_crc,
	.is_support_scale = sun251iw1_mode0_is_support_scale,
	.scale_line_buffer_yuv = sun251iw1_mode0_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = sun251iw1_mode0_scale_line_buffer_rgb,
	.scale_line_buffer_ed = sun251iw1_mode0_scale_line_buffer_ed,
	.is_support_edscale = sun251iw1_mode0_is_support_edscale,
	.is_support_asuscale = sun251iw1_mode0_is_support_asuscale,
	.is_support_lti = sun251iw1_mode0_is_support_lti,
	.is_support_peak = sun251iw1_mode0_is_support_peak,
	.is_support_dlc = sun251iw1_mode0_is_support_dlc,
	.is_support_chn_gamma = sun251iw1_mode0_is_support_chn_gamma,
	.is_support_dci = sun251iw1_mode0_is_support_dci,
	.is_support_fcm = sun251iw1_mode0_is_support_fcm,
	.is_support_sharp = sun251iw1_mode0_is_support_sharp,
	.is_support_de_noise = sun251iw1_mode0_is_support_de_noise,
	.is_support_cdc = sun251iw1_mode0_is_support_cdc,
	.is_support_fbd = sun251iw1_mode0_is_support_fbd,
	.is_support_tfbd = sun251iw1_mode0_is_support_tfbd,
	.is_support_lbc = sun251iw1_mode0_is_support_lbc,
	.is_support_atw = sun251iw1_mode0_is_support_atw,
	.is_support_snr = sun251iw1_mode0_is_support_snr,
	.scaler_type = sun251iw1_mode0_scaler_type,
	.chn_id_lut = sun251iw1_mode0_chn_id_lut,
};
#endif
