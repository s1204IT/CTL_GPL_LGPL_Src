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
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :       de_sharp_type.h
 *
 *  Description :       display engine 35x basic function declaration
 *
 *  History     :       2021/10/26 v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_SHARP_TYPE_H_
#define _DE_SHARP_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union sharp_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:15;
		u32 demo_en:1;
		u32 res1:15;
	} bits;
};

/*offset:0x0004*/
union sharp_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0010*/
union sharp_strengths_reg {
	u32 dwval;
	struct {
		u32 strength_bp0:8;
		u32 strength_bp1:8;
		u32 strength_bp2:8;
		u32 strength_bp3:8;
	} bits;
};

/*offset:0x0014*/
union sharp_extrema_reg {
	u32 dwval;
	struct {
		u32 extrema_idx:3;
		u32 res0:29;
	} bits;
};

/*offset:0x0018*/
union sharp_edge_adaptive_reg {
	u32 dwval;
	struct {
		u32 edge_gain:6;
		u32 res0:2;
		u32 weak_edge_th:8;
		u32 edge_trans_width:4;
		u32 res1:4;
		u32 min_sharp_strength:8;
	} bits;
};

/*offset:0x001c*/
union sharp_overshoot_ctrl_reg {
	u32 dwval;
	struct {
		u32 neg_shift:8;
		u32 neg_up:8;
		u32 pst_shift:8;
		u32 pst_up:8;
	} bits;
};

/*offset:0x0020*/
union sharp_d0_boosting_reg {
	u32 dwval;
	struct {
		u32 res0:8;
		u32 d0_gain:8;
		u32 d0_neg_level:5;
		u32 res1:3;
		u32 d0_pst_level:5;
		u32 res2:3;
	} bits;
};

/*offset:0x0024*/
union sharp_coring_reg {
	u32 dwval;
	struct {
		u32 zero:8;
		u32 res0:8;
		u32 width:8;
		u32 res1:8;
	} bits;
};

/*offset:0x0028*/
union sharp_detail0_weight_reg {
	u32 dwval;
	struct {
		u32 th_flat:8;
		u32 res0:23;
		u32 fw_type:1;
	} bits;
};

/*offset:0x002c*/
union sharp_horz_smooth_reg {
	u32 dwval;
	struct {
		u32 hsmooth_trans_width:4;
		u32 res0:27;
		u32 hsmooth_en:1;
	} bits;
};

/*offset:0x0030*/
union sharp_demo_horz_reg {
	u32 dwval;
	struct {
		u32 demo_horz_start:13;
		u32 res0:3;
		u32 demo_horz_end:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0034*/
union sharp_demo_vert_reg {
	u32 dwval;
	struct {
		u32 demo_vert_start:13;
		u32 res0:3;
		u32 demo_vert_end:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0040*/
union sharp_gaussian_coefs0_reg {
	u32 dwval;
	struct {
		u32 g0:8;
		u32 g1:8;
		u32 g2:8;
		u32 g3:8;
	} bits;
};

/*offset:0x0044*/
union sharp_gaussian_coefs1_reg {
	u32 dwval;
	struct {
		u32 g0:8;
		u32 g1:8;
		u32 g2:8;
		u32 g3:8;
	} bits;
};

/*offset:0x0048*/
union sharp_gaussian_coefs2_reg {
	u32 dwval;
	struct {
		u32 g0:8;
		u32 g1:8;
		u32 g2:8;
		u32 g3:8;
	} bits;
};

/*offset:0x004c*/
union sharp_gaussian_coefs3_reg {
	u32 dwval;
	struct {
		u32 g0:8;
		u32 g1:8;
		u32 g2:8;
		u32 g3:8;
	} bits;
};

/*offset:0x50*/
union sharp_csc_ctl_reg {
	u32 dwval;
	struct {
		u32 csc_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x54*/
union sharp_csc0_d0_reg {
	u32 dwval;
	struct {
		u32 d0:10;
		u32 res0:22;
	} bits;
};

/*offset:0x58*/
union sharp_csc0_d1_reg {
	u32 dwval;
	struct {
		u32 d1:10;
		u32 res0:22;
	} bits;
};

/*offset:0x5c*/
union sharp_csc0_d2_reg {
	u32 dwval;
	struct {
		u32 d2:10;
		u32 res0:22;
	} bits;
};

/*offset:0x60*/
union sharp_csc0_c00_reg {
	u32 dwval;
	struct {
		u32 c00:20;
		u32 res0:12;
	} bits;
};

/*offset:0x64*/
union sharp_csc0_c01_reg {
	u32 dwval;
	struct {
		u32 c01:20;
		u32 res0:12;
	} bits;
};

/*offset:0x68*/
union sharp_csc0_c02_reg {
	u32 dwval;
	struct {
		u32 c02:20;
		u32 res0:12;
	} bits;
};

/*offset:0x6c*/
union sharp_csc0_c03_reg {
	u32 dwval;
	struct {
		u32 c03:10;
		u32 res0:22;
	} bits;
};

/*offset:0x70*/
union sharp_csc0_c10_reg {
	u32 dwval;
	struct {
		u32 c10:20;
		u32 res0:12;
	} bits;
};

/*offset:0x74*/
union sharp_csc0_c11_reg {
	u32 dwval;
	struct {
		u32 c11:20;
		u32 res0:12;
	} bits;
};

/*offset:0x78*/
union sharp_csc0_c12_reg {
	u32 dwval;
	struct {
		u32 c12:20;
		u32 res0:12;
	} bits;
};

/*offset:0x7c*/
union sharp_csc0_c13_reg {
	u32 dwval;
	struct {
		u32 c13:10;
		u32 res0:22;
	} bits;
};

/*offset:0x80*/
union sharp_csc0_c20_reg {
	u32 dwval;
	struct {
		u32 c20:20;
		u32 res0:12;
	} bits;
};

/*offset:0x84*/
union sharp_csc0_c21_reg {
	u32 dwval;
	struct {
		u32 c21:20;
		u32 res0:12;
	} bits;
};

/*offset:0x88*/
union sharp_csc0_c22_reg {
	u32 dwval;
	struct {
		u32 c22:20;
		u32 res0:12;
	} bits;
};

/*offset:0x8c*/
union sharp_csc0_c23_reg {
	u32 dwval;
	struct {
		u32 c23:10;
		u32 res0:22;
	} bits;
};

struct sharp_reg {
    /*offset:0x0000*/
    union sharp_ctl_reg ctrl;
    union sharp_size_reg size;
    u32 res0[2];
    /*offset:0x0010*/
    union sharp_strengths_reg strengths;
    union sharp_extrema_reg extrema;
    union sharp_edge_adaptive_reg edge_adaptive;
    union sharp_overshoot_ctrl_reg overshoot_ctrl;
    /*offset:0x0020*/
    union sharp_d0_boosting_reg d0_boosting;
    union sharp_coring_reg coring;
    union sharp_detail0_weight_reg detail0_weight;
    union sharp_horz_smooth_reg horz_smooth;
    /*offset:0x0030*/
    union sharp_demo_horz_reg demo_horz;
    union sharp_demo_vert_reg demo_vert;
    u32 res1[2];
    /*offset:0x0040*/
    union sharp_gaussian_coefs0_reg gaussian_coefs0;
    union sharp_gaussian_coefs1_reg gaussian_coefs1;
    union sharp_gaussian_coefs2_reg gaussian_coefs2;
    union sharp_gaussian_coefs3_reg gaussian_coefs3;

    /*offset:0x50*/
    union sharp_csc_ctl_reg csc_ctl;
    union sharp_csc0_d0_reg csc_d0;
    union sharp_csc0_d1_reg csc_d1;
    union sharp_csc0_d2_reg csc_d2;
    /*offset:0x60*/
    union sharp_csc0_c00_reg csc_c00;
    union sharp_csc0_c01_reg csc_c01;
    union sharp_csc0_c02_reg csc_c02;
    union sharp_csc0_c03_reg csc_c03;
    /*offset:0x70*/
    union sharp_csc0_c10_reg csc_c10;
    union sharp_csc0_c11_reg csc_c11;
    union sharp_csc0_c12_reg csc_c12;
    union sharp_csc0_c13_reg csc_c13;
    /*offset:0x80*/
    union sharp_csc0_c20_reg csc_c20;
    union sharp_csc0_c21_reg csc_c21;
    union sharp_csc0_c22_reg csc_c22;
    union sharp_csc0_c23_reg csc_c23;
};

struct sharp_config {
    u32 lti_level;
    u32 peak_level;
};

#endif /* #ifndef _DE_SHARP_TYPE_H_ */
