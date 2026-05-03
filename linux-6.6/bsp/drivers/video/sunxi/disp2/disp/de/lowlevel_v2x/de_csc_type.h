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
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :       de_csc_type.h
 *
 *  Description :       display engine 2.0 csc struct declaration
 *
 *  History     :       2014/05/16  vito cheng  v0.1  Initial version
****************************************************************************/

#ifndef __DE_CSC_TYPE_H__
#define __DE_CSC_TYPE_H__

union CSC_BYPASS_REG {
	unsigned int dwval;
	struct {
		unsigned int enable:1;
		unsigned int res0:31;
	} bits;
};

union CSC_BYPASS_REG2 {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int enable:1;
		unsigned int res1:30;
	} bits;
};

union CSC_COEFF_REG {
	unsigned int dwval;
	struct {
		unsigned int coeff:13;
		unsigned int res0:19;
	} bits;
};

union CSC_CONST_REG {
	unsigned int dwval;
	struct {
		unsigned int cnst:20;
		unsigned int res0:12;
	} bits;
};

union CSC_CONST_REG2 {
	unsigned int dwval;
	struct {
		unsigned int cnst:14;
		unsigned int res0:18;
	} bits;
};

union GLB_ALPHA_REG {
	unsigned int dwval;
	struct {
		unsigned int cnst:24;
		unsigned int alpha:8;
	} bits;
};

/*offset:0x0000*/
union gamma_cm_en_reg {
	u32 dwval;
	struct {
		u32 cm_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0004*/
union gamma_cm_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0010*/
union gamma_cm_c00_reg {
	u32 dwval;
	struct {
		u32 c00:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0014*/
union gamma_cm_c01_reg {
	u32 dwval;
	struct {
		u32 c01:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0018*/
union gamma_cm_c02_reg {
	u32 dwval;
	struct {
		u32 c02:20;
		u32 res0:12;
	} bits;
};

/*offset:0x001c*/
union gamma_cm_c03_reg {
	u32 dwval;
	struct {
		u32 c03:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0020*/
union gamma_cm_c10_reg {
	u32 dwval;
	struct {
		u32 c10:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0024*/
union gamma_cm_c11_reg {
	u32 dwval;
	struct {
		u32 c11:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0028*/
union gamma_cm_c12_reg {
	u32 dwval;
	struct {
		u32 c12:20;
		u32 res0:12;
	} bits;
};

/*offset:0x002c*/
union gamma_cm_c13_reg {
	u32 dwval;
	struct {
		u32 c13:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0030*/
union gamma_cm_c20_reg {
	u32 dwval;
	struct {
		u32 c20:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0034*/
union gamma_cm_c21_reg {
	u32 dwval;
	struct {
		u32 c21:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0038*/
union gamma_cm_c22_reg {
	u32 dwval;
	struct {
		u32 c22:20;
		u32 res0:12;
	} bits;
};

/*offset:0x003c*/
union gamma_cm_c23_reg {
	u32 dwval;
	struct {
		u32 c23:20;
		u32 res0:12;
	} bits;
};

/* Channel CSC and Device CSC */
struct __csc_reg_t {
	union CSC_BYPASS_REG bypass;
	unsigned int res[3];
	union CSC_COEFF_REG c00;
	union CSC_COEFF_REG c01;
	union CSC_COEFF_REG c02;
	union CSC_CONST_REG c03;
	union CSC_COEFF_REG c10;
	union CSC_COEFF_REG c11;
	union CSC_COEFF_REG c12;
	union CSC_CONST_REG c13;
	union CSC_COEFF_REG c20;
	union CSC_COEFF_REG c21;
	union CSC_COEFF_REG c22;
	union CSC_CONST_REG c23;
	union GLB_ALPHA_REG alpha;
};

/* CSC IN SMBL */
struct __csc2_reg_t {
	union CSC_BYPASS_REG2 bypass;
	unsigned int res[31];
	union CSC_COEFF_REG c00;
	union CSC_COEFF_REG c01;
	union CSC_COEFF_REG c02;
	union CSC_CONST_REG2 c03;
	union CSC_COEFF_REG c10;
	union CSC_COEFF_REG c11;
	union CSC_COEFF_REG c12;
	union CSC_CONST_REG2 c13;
	union CSC_COEFF_REG c20;
	union CSC_COEFF_REG c21;
	union CSC_COEFF_REG c22;
	union CSC_CONST_REG2 c23;
};

/* CSC IN GAMMA */
struct __csc3_reg_t {
    /*offset:0x0000*/
    union gamma_cm_en_reg cm_en;
    union gamma_cm_size_reg cm_size;
    u32 res0[2];
    /*offset:0x0010*/
    union gamma_cm_c00_reg cm_c00;
    union gamma_cm_c01_reg cm_c01;
    union gamma_cm_c02_reg cm_c02;
    union gamma_cm_c03_reg cm_c03;
    /*offset:0x0020*/
    union gamma_cm_c10_reg cm_c10;
    union gamma_cm_c11_reg cm_c11;
    union gamma_cm_c12_reg cm_c12;
    union gamma_cm_c13_reg cm_c13;
    /*offset:0x0030*/
    union gamma_cm_c20_reg cm_c20;
    union gamma_cm_c21_reg cm_c21;
    union gamma_cm_c22_reg cm_c22;
    union gamma_cm_c23_reg cm_c23;
};

/* Input CSC in FCE */
struct __icsc_reg_t {
	union CSC_BYPASS_REG bypass;
};

#endif
