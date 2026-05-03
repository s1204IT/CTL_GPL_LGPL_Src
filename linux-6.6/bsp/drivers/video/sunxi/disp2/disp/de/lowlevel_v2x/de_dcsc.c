/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*****************************************************************************
 *All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *File name   :       de_ccsc.c
 *
 *Description :       display engine 2.0 device csc basic function definition
 *
 *History     :       2014/05/19  vito cheng  v0.1  Initial version
 *****************************************************************************/

#include "de_rtmx.h"
#include "de_csc_type.h"
#include "de_csc.h"

#define DCSC_OFST	0xB0000
#define CSC_ENHANCE_MODE_NUM 3
/* must equal to ENHANCE_MODE_NUM */

struct __scal_matrix4x4 {
	__s64 x00;
	__s64 x01;
	__s64 x02;
	__s64 x03;
	__s64 x10;
	__s64 x11;
	__s64 x12;
	__s64 x13;
	__s64 x20;
	__s64 x21;
	__s64 x22;
	__s64 x23;
	__s64 x30;
	__s64 x31;
	__s64 x32;
	__s64 x33;
};

void pq_get_enhance(struct disp_csc_config *conig);
static volatile struct __csc_reg_t *dcsc_dev[DE_NUM];
static volatile struct __csc2_reg_t *dcsc2_dev[DE_NUM];
static volatile struct __csc3_reg_t *dcsc3_dev[DE_NUM];
static struct de_reg_blocks dcsc_coeff_block[DE_NUM];
static struct de_reg_blocks dcsc_enable_block[DE_NUM];
static struct disp_csc_config g_dcsc_config[DE_NUM];

static bool use_user_matrix;
static unsigned int is_in_smbl[DE_NUM];
static unsigned int is_in_gamma[DE_NUM];
/* device csc and smbl in the same module or not */

extern int user_matrix[32];

int y2r8bit[192] = {

	/* bt601 */
	0x000012A0, 0x0, 0x00000000, 0x0, 0x00001989, 0x0, 0xFFF21168, 0xFFFFFFFF, 0x000012A0, 0x0,
	0xFFFFF9BE, 0xFFFFFFFF, 0xFFFFF2FE, 0xFFFFFFFF, 0x000877CF, 0x0,
	0x000012A0, 0x0, 0x0000204A, 0x0, 0x00000000, 0x0, 0xFFEEB127, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 */
	0x000012A0, 0x0, 0x00000000, 0x0, 0x00001CB0, 0x0, 0xFFF07DF4, 0xFFFFFFFF, 0x000012A0, 0x0,
	0xfffffC98, 0xFFFFFFFF, 0xfffff775, 0xFFFFFFFF, 0x0004CFDF, 0x0,
	0x000012A0, 0x0, 0x000021D7, 0x0, 0x00000000, 0x0, 0xFFEDEA7F, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ycc */
	0x00001000, 0x0, 0x00000000, 0x0, 0x0000166F, 0x0, 0xFFF4C84B, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFA78, 0xFFFFFFFF, 0xFFFFF491, 0xFFFFFFFF, 0x00087B16, 0x0,
	0x00001000, 0x0, 0x00001C56, 0x0, 0x00000000, 0x0, 0xFFF1D4FE, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ehance */
	0x00001000, 0x0, 0x00000000, 0x0, 0x00001933, 0x0, 0xFFF36666, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFD02, 0xFFFFFFFF, 0xFFFFF883, 0xFFFFFFFF, 0x00053D71, 0x0,
	0x00001000, 0x0, 0x00001DB2, 0x0, 0x00000000, 0x0, 0xFFF126E9, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt601 studio */
	0x00001000, 0x0, 0x00000000, 0x0, 0x000015F0, 0x0, 0xFFF50831, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFAA0, 0xFFFFFFFF, 0xFFFFF4FA, 0xFFFFFFFF, 0x00083333, 0x0,
	0x00001000, 0x0, 0x00001BB6, 0x0, 0x00000000, 0x0, 0xFFF224DD, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 studio */
	0x00001000, 0x0, 0x00000000, 0x0, 0x000018A4, 0x0, 0xFFF3AE14, 0xFFFFFFFF, 0x00001000, 0x0,
	0xFFFFFD12, 0xFFFFFFFF, 0xFFFFF8A8, 0xFFFFFFFF, 0x000522D1, 0x0,
	0x00001000, 0x0, 0x00001D0E, 0x0, 0x00000000, 0x0, 0xFFF178D5, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

};

int ir2y8bit[128] = {

	/* bt601 */
	0x0000041D, 0x0, 0x00000810, 0x0, 0x00000191, 0x0, 0x00010000, 0x0, 0xFFFFFDA2, 0xFFFFFFFF,
	0xFFFFFB58, 0xFFFFFFFF, 0x00000706, 0x0, 0x00080000, 0x0,
	0x00000706, 0x0, 0xFFFFFA1D, 0xFFFFFFFF, 0xFFFFFEDD, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 */
	0x000002EE, 0x0, 0x000009D3, 0x0, 0x000000FE, 0x0, 0x00010000, 0x0, 0xfffffe62, 0xFFFFFFFF,
	0xfffffA98, 0xFFFFFFFF, 0x00000706, 0x0, 0x00080000, 0x0,
	0x00000706, 0x0, 0xfffff99E, 0xFFFFFFFF, 0xffffff5C, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ycc */
	0x000004C8, 0x0, 0x00000963, 0x0, 0x000001D5, 0x0, 0x00000000, 0x0, 0xFFFFFD4D, 0xFFFFFFFF,
	0xFFFFFAB3, 0xFFFFFFFF, 0x00000800, 0x0, 0x00080000, 0x0,
	0x00000800, 0x0, 0xFFFFF94F, 0xFFFFFFFF, 0xFFFFFEB2, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* ehance */
	0x00000368, 0x0, 0x00000B71, 0x0, 0x00000127, 0x0, 0x00000000, 0x0, 0xFFFFFE29, 0xFFFFFFFF,
	0xFFFFF9D7, 0xFFFFFFFF, 0x00000800, 0x0, 0x00080000, 0x0,
	0x00000800, 0x0, 0xFFFFF8BC, 0xFFFFFFFF, 0xFFFFFF44, 0xFFFFFFFF, 0x00080000, 0x0, 0x00000000,
	0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int y2y8bit[64] = {

	/* bt601 to bt709 */
	0x00001000, 0x0, 0xFFFFFE27, 0xFFFFFFFF, 0xFFFFFCAC, 0xFFFFFFFF, 0x00029681, 0x0, 0x00000000,
	0x0, 0x0000104C, 0x0, 0x000001D5, 0x0, 0xFFFEEF17, 0xFFFFFFFF,
	0x00000000, 0x0, 0x00000133, 0x0, 0x00001068, 0x0, 0xFFFF326E, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,

	/* bt709 to bt601 */
	0x00001000, 0x0, 0x00000197, 0x0, 0x00000311, 0x0, 0xFFFDAC02, 0xFFFFFFFF, 0x00000000, 0x0,
	0x00000FD6, 0x0, 0xFFFFFE3B, 0xFFFFFFFF, 0x0000F765, 0x0,
	0x00000000, 0x0, 0xFFFFFED7, 0xFFFFFFFF, 0x00000FBC, 0x0, 0x0000B663, 0x0, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int ir2r8bit[32] = {

	/* 0-255 to 16-235 */
	0x00000DC0, 0x0, 0x00000000, 0x0, 0x00000000, 0x0, 0x00010000, 0x0, 0x00000000, 0x0,
	0x00000DC0, 0x0, 0x00000000, 0x0, 0x00010000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00000DC0, 0x0, 0x00010000, 0x0, 0x00000000, 0x0,
	0x00000000, 0x0, 0x00000000, 0x0, 0x00001000, 0x0,
};

int bypass_csc8bit[12] = {

	0x00000400, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000400, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000400, 0x00000000,
};

unsigned int sin_cos8bit[128] = {
	/* sin table */
	0xffffffbd, 0xffffffbf, 0xffffffc1, 0xffffffc2, 0xffffffc4, 0xffffffc6, 0xffffffc8, 0xffffffca,
	0xffffffcc, 0xffffffce, 0xffffffd1, 0xffffffd3, 0xffffffd5, 0xffffffd7, 0xffffffd9, 0xffffffdb,
	0xffffffdd, 0xffffffdf, 0xffffffe2, 0xffffffe4, 0xffffffe6, 0xffffffe8, 0xffffffea, 0xffffffec,
	0xffffffef, 0xfffffff1, 0xfffffff3, 0xfffffff5, 0xfffffff8, 0xfffffffa, 0xfffffffc, 0xfffffffe,
	0x00000000, 0x00000002, 0x00000004, 0x00000006, 0x00000008, 0x0000000b, 0x0000000d, 0x0000000f,
	0x00000011, 0x00000014, 0x00000016, 0x00000018, 0x0000001a, 0x0000001c, 0x0000001e, 0x00000021,
	0x00000023, 0x00000025, 0x00000027, 0x00000029, 0x0000002b, 0x0000002d, 0x0000002f, 0x00000032,
	0x00000034, 0x00000036, 0x00000038, 0x0000003a, 0x0000003c, 0x0000003e, 0x0000003f, 0x00000041,
	/* cos table */
	0x0000006c, 0x0000006d, 0x0000006e, 0x0000006f, 0x00000071, 0x00000072, 0x00000073, 0x00000074,
	0x00000074, 0x00000075, 0x00000076, 0x00000077, 0x00000078, 0x00000079, 0x00000079, 0x0000007a,
	0x0000007b, 0x0000007b, 0x0000007c, 0x0000007c, 0x0000007d, 0x0000007d, 0x0000007e, 0x0000007e,
	0x0000007e, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
	0x00000080, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
	0x0000007e, 0x0000007e, 0x0000007e, 0x0000007d, 0x0000007d, 0x0000007c, 0x0000007c, 0x0000007b,
	0x0000007b, 0x0000007a, 0x00000079, 0x00000079, 0x00000078, 0x00000077, 0x00000076, 0x00000075,
	0x00000074, 0x00000074, 0x00000073, 0x00000072, 0x00000071, 0x0000006f, 0x0000006e, 0x0000006d
};

inline __s64 IntRight_shift64(__s64 datain, unsigned int shiftbit)
{
	__s64 dataout;
	__s64 tmp;

	tmp = (shiftbit >= 1) ? (1 << (shiftbit - 1)) : 0;
	if (datain >= 0)
		dataout = (datain + tmp) >> shiftbit;
	else
		dataout = -((-datain + tmp) >> shiftbit);

	return dataout;
}

static s32 IDE_SCAL_MATRIC_MUL(struct __scal_matrix4x4 *in1,
	struct __scal_matrix4x4 *in2, struct __scal_matrix4x4 *result)
{

	result->x00 =
	    IntRight_shift64(in1->x00 * in2->x00 + in1->x01 * in2->x10 +
			    in1->x02 * in2->x20 + in1->x03 * in2->x30, 10);
	result->x01 =
	    IntRight_shift64(in1->x00 * in2->x01 + in1->x01 * in2->x11 +
			    in1->x02 * in2->x21 + in1->x03 * in2->x31, 10);
	result->x02 =
	    IntRight_shift64(in1->x00 * in2->x02 + in1->x01 * in2->x12 +
			    in1->x02 * in2->x22 + in1->x03 * in2->x32, 10);
	result->x03 =
	    IntRight_shift64(in1->x00 * in2->x03 + in1->x01 * in2->x13 +
			    in1->x02 * in2->x23 + in1->x03 * in2->x33, 10);
	result->x10 =
	    IntRight_shift64(in1->x10 * in2->x00 + in1->x11 * in2->x10 +
			    in1->x12 * in2->x20 + in1->x13 * in2->x30, 10);
	result->x11 =
	    IntRight_shift64(in1->x10 * in2->x01 + in1->x11 * in2->x11 +
			    in1->x12 * in2->x21 + in1->x13 * in2->x31, 10);
	result->x12 =
	    IntRight_shift64(in1->x10 * in2->x02 + in1->x11 * in2->x12 +
			    in1->x12 * in2->x22 + in1->x13 * in2->x32, 10);
	result->x13 =
	    IntRight_shift64(in1->x10 * in2->x03 + in1->x11 * in2->x13 +
			    in1->x12 * in2->x23 + in1->x13 * in2->x33, 10);
	result->x20 =
	    IntRight_shift64(in1->x20 * in2->x00 + in1->x21 * in2->x10 +
			    in1->x22 * in2->x20 + in1->x23 * in2->x30, 10);
	result->x21 =
	    IntRight_shift64(in1->x20 * in2->x01 + in1->x21 * in2->x11 +
			    in1->x22 * in2->x21 + in1->x23 * in2->x31, 10);
	result->x22 =
	    IntRight_shift64(in1->x20 * in2->x02 + in1->x21 * in2->x12 +
			    in1->x22 * in2->x22 + in1->x23 * in2->x32, 10);
	result->x23 =
	    IntRight_shift64(in1->x20 * in2->x03 + in1->x21 * in2->x13 +
			    in1->x22 * in2->x23 + in1->x23 * in2->x33, 10);
	result->x30 =
	    IntRight_shift64(in1->x30 * in2->x00 + in1->x31 * in2->x10 +
			    in1->x32 * in2->x20 + in1->x33 * in2->x30, 10);
	result->x31 =
	    IntRight_shift64(in1->x30 * in2->x01 + in1->x31 * in2->x11 +
			    in1->x32 * in2->x21 + in1->x33 * in2->x31, 10);
	result->x32 =
	    IntRight_shift64(in1->x30 * in2->x02 + in1->x31 * in2->x12 +
			    in1->x32 * in2->x22 + in1->x33 * in2->x32, 10);
	result->x33 =
	    IntRight_shift64(in1->x30 * in2->x03 + in1->x31 * in2->x13 +
			    in1->x32 * in2->x23 + in1->x33 * in2->x33, 10);

	return 0;
}

static int de_dcsc_set_reg_base(unsigned int sel, void *base)
{
	DE_INFO("sel=%d, base=0x%p\n", sel, base);
	if (is_in_smbl[sel])
		dcsc2_dev[sel] = (struct __csc2_reg_t *) base;
	else if (is_in_gamma[sel])
		dcsc3_dev[sel] = (struct __csc3_reg_t *) base;
	else
		dcsc_dev[sel] = (struct __csc_reg_t *) base;

	return 0;
}

int _csc_enhance_setting[CSC_ENHANCE_MODE_NUM][4] = {
	{50, 50, 50, 50},
	/* normal */
	{50, 50, 50, 50},
	/* vivid */
	{50, 40, 50, 50},
	/* soft */
};

void de_dcsc_pq_get_enhance(u32 sel, int *pq_enh)
{
	struct disp_csc_config *config = &g_dcsc_config[sel];
	unsigned int enhance_mode = config->enhance_mode;

	pq_enh[0] = _csc_enhance_setting[enhance_mode][0];
	pq_enh[1] = _csc_enhance_setting[enhance_mode][1];
	pq_enh[2] = _csc_enhance_setting[enhance_mode][2];
	pq_enh[3] = _csc_enhance_setting[enhance_mode][3];
}

void de_dcsc_pq_set_enhance(u32 sel, int *pq_enh)
{
	struct disp_csc_config *config = &g_dcsc_config[sel];
	unsigned int enhance_mode = config->enhance_mode;

	_csc_enhance_setting[enhance_mode][0] = pq_enh[0];
	_csc_enhance_setting[enhance_mode][1] = pq_enh[1];
	_csc_enhance_setting[enhance_mode][2] = pq_enh[2];
	_csc_enhance_setting[enhance_mode][3] = pq_enh[3];
}

int de_dcsc_set_colormatrix(unsigned int sel, long long *matrix3x4, bool is_identity)
{
	int i;
	if (is_identity) {
		/* when input identity matrix, skip the process of this matrix*/
		use_user_matrix = false;
		return 0;
	}

	use_user_matrix = true;
	/* user_matrix is 64-bit and combined with two int, the last 8 int is fixed*/
	for (i = 0; i < 12; i++) {
		/*   matrix3x4 = real value*2^48
		 *   user_matrix = real value*2^12
		 *   so here >> 36
		 */
		user_matrix[2 * i] = matrix3x4[i] >> 36;
		/* constants  multiply by data bits  */
		if (i == 3 || i == 7 || i == 11)
			user_matrix[2 * i] *= 256;
		user_matrix[2 * i + 1] = matrix3x4[i] >= 0 ? 0 : -1;
	}
	return 0;
}

/* normal case:
 *display a SD video:
 *infmt = DE_YUV, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a HD video:
 *infmt = DE_YUV, incscmod = BT_709, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a JPEG picture:
 *infmt = DE_YUV, incscmod = BT_YCC, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a UI (RGB format)     with ENHANCE enable
 *infmt = DE_YUV, incscmod = BT_ENHANCE, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in RGB mode:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_16_235
 *output to PC with HDMI in RGB mode:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in YCbCr mode, 480i/576i/480p/576p:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV, outcscmod = BT_601
 *out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in YCbCr mode, 720p/1080p/2160p:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV
 *outcscmod = BT_709, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with CVBS:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *bypass:
 *outfmt = infmt, outcscmod = incscmod
 *out_color_range = DISP_COLOR_RANGE_0_255
 *brightness=contrast=saturation=hue=50
 */
int de_csc_coeff_calc_inner8bit(unsigned int infmt, unsigned int incscmod,
		      unsigned int outfmt, unsigned int outcscmod,
		      unsigned int brightness, unsigned int contrast,
		      unsigned int saturation, unsigned int hue,
		      unsigned int out_color_range, int *csc_coeff,
		      bool use_user_matrix)
{
	struct __scal_matrix4x4 *enhancecoeff, *tmpcoeff;
	struct __scal_matrix4x4 *coeff[5], *in0coeff, *in1coeff;
	int oper, i;
	int i_bright, i_contrast, i_saturation, i_hue, sinv, cosv;

	oper = 0;

	enhancecoeff = kmalloc(sizeof(struct __scal_matrix4x4),
			GFP_KERNEL | __GFP_ZERO);
	tmpcoeff = kmalloc(sizeof(struct __scal_matrix4x4),
		GFP_KERNEL | __GFP_ZERO);
	in0coeff = kmalloc(sizeof(struct __scal_matrix4x4),
		GFP_KERNEL | __GFP_ZERO);

	if (!enhancecoeff || !tmpcoeff || !in0coeff) {
		DE_WARN("kmalloc fail\n");
		goto err;
	}

	/* BYPASS */
	if (infmt == outfmt && incscmod == outcscmod
	    && out_color_range == DISP_COLOR_RANGE_0_255 && brightness == 50
	    && contrast == 50 && saturation == 50 && hue == 50 && !use_user_matrix) {
		memcpy(csc_coeff, bypass_csc8bit, 48);
		goto err;
	}

	/* dcsc's infmt=DE_RGB, use_user_matrix is used by dcsc, it's ok to do so*/
	if (use_user_matrix) {
		coeff[oper] = (struct __scal_matrix4x4 *)(user_matrix);
		oper++;

	}

	/* NON-BYPASS */
	if (infmt == DE_RGB) {
		/* convert to YCbCr */
		if (outfmt == DE_RGB) {
			coeff[oper] = (struct __scal_matrix4x4 *) (ir2y8bit + 0x20);
			oper++;
		} else {
			if (outcscmod == DE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *) (ir2y8bit);
				oper++;
			} else if (outcscmod == DE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *)(ir2y8bit + 0x20);
				oper++;
			}
		}
	} else {
		if (incscmod != outcscmod && outfmt == DE_YUV) {
			if (incscmod == DE_BT601
				&& outcscmod == DE_BT709) {
				coeff[oper] = (struct __scal_matrix4x4 *) (y2y8bit);
				oper++;
			} else if (incscmod == DE_BT709
				   && outcscmod == DE_BT601) {
				coeff[oper] = (struct __scal_matrix4x4 *)(y2y8bit + 0x20);
				oper++;
			}
		}
	}

	if (brightness != 50 || contrast != 50
		|| saturation != 50 || hue != 50) {
		brightness = brightness > 100 ? 100 : brightness;
		contrast = contrast > 100 ? 100 : contrast;
		saturation = saturation > 100 ? 100 : saturation;
		hue = hue > 100 ? 100 : hue;

		i_bright = (int)(brightness * 64 / 100);
		i_saturation = (int)(saturation * 64 / 100);
		i_contrast = (int)(contrast * 64 / 100);
		i_hue = (int)(hue * 64 / 100);

		sinv = sin_cos8bit[i_hue & 0x3f];
		cosv = sin_cos8bit[64 + (i_hue & 0x3f)];

		/* calculate enhance matrix */
		enhancecoeff->x00 = i_contrast << 7;
		enhancecoeff->x01 = 0;
		enhancecoeff->x02 = 0;
		enhancecoeff->x03 =
		    (((i_bright - 32) * 5 + 16) << 12) - (i_contrast << 11);
		enhancecoeff->x10 = 0;
		enhancecoeff->x11 = (i_contrast * i_saturation * cosv) >> 5;
		enhancecoeff->x12 = (i_contrast * i_saturation * sinv) >> 5;
		enhancecoeff->x13 =
		    (1 << 19) - ((enhancecoeff->x11 + enhancecoeff->x12) << 7);
		enhancecoeff->x20 = 0;
		enhancecoeff->x21 = (-i_contrast * i_saturation * sinv) >> 5;
		enhancecoeff->x22 = (i_contrast * i_saturation * cosv) >> 5;
		enhancecoeff->x23 =
		    (1 << 19) - ((enhancecoeff->x22 + enhancecoeff->x21) << 7);
		enhancecoeff->x30 = 0;
		enhancecoeff->x31 = 0;
		enhancecoeff->x32 = 0;
		enhancecoeff->x33 = 4096;

		coeff[oper] = enhancecoeff;
		oper++;

	}

	if (outfmt == DE_RGB) {
		if (infmt == DE_RGB) {
			coeff[oper] = (struct __scal_matrix4x4 *) (y2r8bit + 0x20);
			oper++;

			if (out_color_range == DISP_COLOR_RANGE_16_235) {
				coeff[oper] = (struct __scal_matrix4x4 *) (ir2r8bit);
				oper++;
			}
		} else {
			if (out_color_range == DISP_COLOR_RANGE_16_235) {
				if (incscmod == DE_BT601) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x80);
					oper++;
				} else if (incscmod == DE_BT709) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0xa0);
					oper++;
				}
			} else {
				if (incscmod == DE_BT601) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *) (y2r8bit);
					oper++;
				} else if (incscmod == DE_BT709) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x20);
					oper++;
				}
				/*else if (incscmod == DE_YCC) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x40);
					oper++;
				} else if (incscmod == DE_ENHANCE) {
					coeff[oper] =
					    (struct __scal_matrix4x4 *)
						(y2r8bit + 0x60);
					oper++;
				}*/
			}
		}
	}
	/* matrix multiply */
	if (oper == 0) {
		memcpy(csc_coeff, bypass_csc8bit, sizeof(bypass_csc8bit));
	} else if (oper == 1) {
		for (i = 0; i < 12; i++)
			*(csc_coeff + i) =
			    IntRight_shift64((int)(*((__s64 *) coeff[0] + i)),
					    oper << 1);
	} else {
		memcpy((void *)in0coeff, (void *)coeff[0],
		    sizeof(struct __scal_matrix4x4));
		for (i = 1; i < oper; i++) {
			in1coeff = coeff[i];
			IDE_SCAL_MATRIC_MUL(in1coeff, in0coeff, tmpcoeff);
			memcpy((void *)in0coeff, (void *)tmpcoeff,
				sizeof(struct __scal_matrix4x4));
		}

		for (i = 0; i < 12; i++)
			*(csc_coeff + i) =
			    IntRight_shift64((int)(*((__s64 *) tmpcoeff + i)),
					    oper << 1);
	}

err:
	kfree(in0coeff);
	kfree(tmpcoeff);
	kfree(enhancecoeff);

	return 0;

}

int de_dcsc_apply(unsigned int sel, struct disp_csc_config *config)
{
	int csc_coeff[12];
	unsigned int enhance_mode;

	config->enhance_mode =
	    (config->enhance_mode > CSC_ENHANCE_MODE_NUM - 1)
	     ? g_dcsc_config[sel].enhance_mode : config->enhance_mode;
	enhance_mode = config->enhance_mode;
	config->brightness = _csc_enhance_setting[enhance_mode][0];
	config->contrast = _csc_enhance_setting[enhance_mode][1];
	config->saturation = _csc_enhance_setting[enhance_mode][2];
	config->hue = _csc_enhance_setting[enhance_mode][3];
	if (config->brightness < 50)
		config->brightness = config->brightness * 3 / 5 + 20;/*map from 0~100 to 20~100*/
	if (config->contrast < 50)
		config->contrast = config->contrast * 3 / 5 + 20;/*map from 0~100 to 20~100*/

	DE_INFO("sel=%d, in_fmt=%d, mode=%d, out_fmt=%d, mode=%d, range=%d\n",
	      sel, config->in_fmt, config->in_mode, config->out_fmt,
	      config->out_mode, config->out_color_range);

	memcpy(&g_dcsc_config[sel], config, sizeof(struct disp_csc_config));
	if (is_in_gamma[sel]) {
		de_csc_coeff_calc_inner8bit(
		    config->in_fmt, config->in_mode, config->out_fmt,
		    config->out_mode, config->brightness, config->contrast,
		    config->saturation, config->hue, config->out_color_range,
		    csc_coeff, use_user_matrix);
	} else {
		de_csc_coeff_calc(config->in_fmt, config->in_mode, config->out_fmt,
			  config->out_mode, config->brightness,
			  config->contrast, config->saturation, config->hue,
			  config->out_color_range, csc_coeff, use_user_matrix);
	}

	if (is_in_smbl[sel]) {
		dcsc2_dev[sel]->c00.dwval = *(csc_coeff);
		dcsc2_dev[sel]->c01.dwval = *(csc_coeff + 1);
		dcsc2_dev[sel]->c02.dwval = *(csc_coeff + 2);
		dcsc2_dev[sel]->c03.dwval = *(csc_coeff + 3) >> 6;
		dcsc2_dev[sel]->c10.dwval = *(csc_coeff + 4);
		dcsc2_dev[sel]->c11.dwval = *(csc_coeff + 5);
		dcsc2_dev[sel]->c12.dwval = *(csc_coeff + 6);
		dcsc2_dev[sel]->c13.dwval = *(csc_coeff + 7) >> 6;
		dcsc2_dev[sel]->c20.dwval = *(csc_coeff + 8);
		dcsc2_dev[sel]->c21.dwval = *(csc_coeff + 9);
		dcsc2_dev[sel]->c22.dwval = *(csc_coeff + 10);
		dcsc2_dev[sel]->c23.dwval = *(csc_coeff + 11) >> 6;
		dcsc2_dev[sel]->bypass.bits.enable = 1;
		/* always enable csc */
	} else if (is_in_gamma[sel]) {
		dcsc3_dev[sel]->cm_c00.dwval = (*(csc_coeff)) << 7;
		dcsc3_dev[sel]->cm_c01.dwval = (*(csc_coeff + 1)) << 7;
		dcsc3_dev[sel]->cm_c02.dwval = (*(csc_coeff + 2)) << 7;
		dcsc3_dev[sel]->cm_c03.dwval = (*(csc_coeff + 3) + 0x200) << 7;
		dcsc3_dev[sel]->cm_c10.dwval = (*(csc_coeff + 4)) << 7;
		dcsc3_dev[sel]->cm_c11.dwval = (*(csc_coeff + 5)) << 7;
		dcsc3_dev[sel]->cm_c12.dwval = (*(csc_coeff + 6)) << 7;
		dcsc3_dev[sel]->cm_c13.dwval = (*(csc_coeff + 7) + 0x200) << 7;
		dcsc3_dev[sel]->cm_c20.dwval = (*(csc_coeff + 8)) << 7;
		dcsc3_dev[sel]->cm_c21.dwval = (*(csc_coeff + 9)) << 7;
		dcsc3_dev[sel]->cm_c22.dwval = (*(csc_coeff + 10)) << 7;
		dcsc3_dev[sel]->cm_c23.dwval = (*(csc_coeff + 11) + 0x200) << 7;
		dcsc3_dev[sel]->cm_en.dwval = 0x1;
	} else {
		dcsc_dev[sel]->c00.dwval = *(csc_coeff);
		dcsc_dev[sel]->c01.dwval = *(csc_coeff + 1);
		dcsc_dev[sel]->c02.dwval = *(csc_coeff + 2);
		dcsc_dev[sel]->c03.dwval = *(csc_coeff + 3) + 0x200;
		dcsc_dev[sel]->c10.dwval = *(csc_coeff + 4);
		dcsc_dev[sel]->c11.dwval = *(csc_coeff + 5);
		dcsc_dev[sel]->c12.dwval = *(csc_coeff + 6);
		dcsc_dev[sel]->c13.dwval = *(csc_coeff + 7) + 0x200;
		dcsc_dev[sel]->c20.dwval = *(csc_coeff + 8);
		dcsc_dev[sel]->c21.dwval = *(csc_coeff + 9);
		dcsc_dev[sel]->c22.dwval = *(csc_coeff + 10);
		dcsc_dev[sel]->c23.dwval = *(csc_coeff + 11) + 0x200;
		dcsc_dev[sel]->bypass.bits.enable = 1;
		/* always enable csc */
	}

	dcsc_coeff_block[sel].dirty = 1;
	dcsc_enable_block[sel].dirty = 1;

	return 0;
}

int de_dcsc_get_config(unsigned int sel, struct disp_csc_config *config)
{
	memcpy(config, &g_dcsc_config[sel], sizeof(struct disp_csc_config));

	return 0;
}

s32 de_dcsc_set_size(u32 sel, u32 width, u32 height)
{
	if (!is_in_gamma[sel])
		return -1;

	dcsc3_dev[sel]->cm_size.bits.width = width == 0 ? 0 : width - 1;
	dcsc3_dev[sel]->cm_size.bits.height = height == 0 ? 0 : height - 1;
	dcsc_coeff_block[sel].dirty = 1;

	return 0;
}

int de_dcsc_update_regs(unsigned int sel)
{
	unsigned int reg_val;

	if (dcsc_enable_block[sel].dirty == 0x1) {
		if (is_in_smbl[sel]) {
			reg_val =
			    readl((void __iomem *)dcsc_enable_block[sel].off);
			reg_val &= 0xfffffffd;
			reg_val |=
			    (*((unsigned int *)dcsc_enable_block[sel].val));
			writel(reg_val,
			       (void __iomem *)dcsc_enable_block[sel].off);
		} else {
			aw_memcpy_toio((void *)dcsc_enable_block[sel].off,
			   dcsc_enable_block[sel].val,
			   dcsc_enable_block[sel].size);
		}
		dcsc_enable_block[sel].dirty = 0x0;
	}

	if (dcsc_coeff_block[sel].dirty == 0x1) {
		aw_memcpy_toio((void *)dcsc_coeff_block[sel].off,
		   dcsc_coeff_block[sel].val, dcsc_coeff_block[sel].size);
		dcsc_coeff_block[sel].dirty = 0x0;
	}

	return 0;
}

int de_dcsc_init(struct disp_bsp_init_para *para)
{
	uintptr_t base;
	void *memory;
	int screen_id, device_num;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {
		is_in_smbl[screen_id] = de_feat_is_support_smbl(screen_id);
		is_in_gamma[screen_id] = de_feat_is_support_gamma(screen_id);

#if defined(SUPPORT_INDEPENDENT_DE)
		base = para->reg_base[DISP_MOD_DE + screen_id]
		    + (screen_id + 1) * 0x00100000 + DCSC_OFST;
		if (screen_id)
			base = base - 0x00100000;
#else
		base = para->reg_base[DISP_MOD_DE]
		    + (screen_id + 1) * 0x00100000 + DCSC_OFST;
#endif
		DE_INFO("sel %d, Dcsc_base=0x%p\n", screen_id, (void *)base);

		if (is_in_smbl[screen_id]) {
			memory = kmalloc(sizeof(struct __csc2_reg_t),
			    GFP_KERNEL | __GFP_ZERO);
			if (memory == NULL) {
				DE_WARN("malloc Ccsc[%d] mm fail! size=0x%x\n",
				     screen_id,
				     (unsigned int)sizeof(struct __csc2_reg_t));
				return -1;
			}

			dcsc_enable_block[screen_id].off = base;
			dcsc_enable_block[screen_id].val = memory;
			dcsc_enable_block[screen_id].size = 0x04;
			dcsc_enable_block[screen_id].dirty = 0;

			dcsc_coeff_block[screen_id].off = base + 0x80;
			dcsc_coeff_block[screen_id].val = memory + 0x80;
			dcsc_coeff_block[screen_id].size = 0x30;
			dcsc_coeff_block[screen_id].dirty = 0;
		} else if (is_in_gamma[screen_id]) {
			memory = kmalloc(sizeof(struct __csc3_reg_t),
			    GFP_KERNEL | __GFP_ZERO);
			if (memory == NULL) {
				DE_WARN("malloc Ccsc[%d] mm fail! size=0x%x\n",
				     screen_id,
				     (unsigned int)sizeof(struct __csc2_reg_t));
				return -1;
			}

			dcsc_enable_block[screen_id].off = base;
			dcsc_enable_block[screen_id].val = memory;
			dcsc_enable_block[screen_id].size = 0x08;
			dcsc_enable_block[screen_id].dirty = 0;

			dcsc_coeff_block[screen_id].off = base + 0x10;
			dcsc_coeff_block[screen_id].val = memory + 0x10;
			dcsc_coeff_block[screen_id].size = 0x30;
			dcsc_coeff_block[screen_id].dirty = 0;
		} else {
			memory =  kmalloc(sizeof(struct __csc_reg_t),
			    GFP_KERNEL | __GFP_ZERO);
			if (memory == NULL) {
				DE_WARN("malloc Ccsc[%d] mm fail! size=0x%x\n",
				     screen_id,
				     (unsigned int)sizeof(struct __csc_reg_t));
				return -1;
			}

			dcsc_enable_block[screen_id].off = base;
			dcsc_enable_block[screen_id].val = memory;
			dcsc_enable_block[screen_id].size = 0x04;
			dcsc_enable_block[screen_id].dirty = 0;

			dcsc_coeff_block[screen_id].off = base + 0x10;
			dcsc_coeff_block[screen_id].val = memory + 0x10;
			dcsc_coeff_block[screen_id].size = 0x30;
			dcsc_coeff_block[screen_id].dirty = 0;
		}
		g_dcsc_config[screen_id].enhance_mode = 0;
		de_dcsc_set_reg_base(screen_id, memory);
	}

	return 0;
}

int de_dcsc_exit(void)
{
	int screen_id, device_num;

	device_num = de_feat_get_num_screens();

	for (screen_id = 0; screen_id < device_num; screen_id++) {
		is_in_smbl[screen_id] = de_feat_is_support_smbl(screen_id);

		if (is_in_smbl[screen_id])
			kfree(dcsc_enable_block[screen_id].val);
		else
			kfree(dcsc_enable_block[screen_id].val);
	}

	return 0;
}
