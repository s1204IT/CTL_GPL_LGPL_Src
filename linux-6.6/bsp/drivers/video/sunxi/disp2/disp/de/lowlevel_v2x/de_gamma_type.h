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
 *  File name   :       de_gamma_type.h
 *
 *  Description :       display engine 35x vep basic function declaration
 *
 *  History     :       2021/10/29 v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_GAMMA_TYPE_H_
#define _DE_GAMMA_TYPE_H_

#include "linux/types.h"

/*color mode:
 * 0,use 0x44 color;
 * 1,vertical gradient;
 * 2,horizontal gradient;
 * 3,colorbar;
 * 4, 16 gray;
 * other,reserved*/
/*offset:0x0040*/
union gamma_ctl_reg {
	u32 dwval;
	struct {
		u32 gamma_en:1;
		u32 color_mode:3;
		u32 blue_en:1;
		u32 res0:27;
	} bits;
};

/*offset:0x0044*/
union gamma_blue_color_reg {
	u32 dwval;
	struct {
		u32 b:10;
		u32 g:10;
		u32 r:10;
		u32 res0:2;
	} bits;
};

/*offset:0x0100+n*4*/
union gamma_tab_reg {
	u32 dwval;
	struct {
		u32 tab_b:8;
		u32 res0:2;
		u32 tab_g:8;
		u32 res1:2;
		u32 tab_r:8;
		u32 res2:4;
	} bits;
};

struct __gamma_reg_t {
    /*offset:0x0040*/
    union gamma_ctl_reg ctl;
    union gamma_blue_color_reg blue_color;
    u32 res1[46];
    /*offset:0x0100*/
    union gamma_tab_reg tab[1024];
};

struct gamma_data {
	u8 brighten_level;
};

#endif /* #ifndef _DE_GAMMA_TYPE_H_ */
