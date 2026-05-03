/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_LBC_TYPE_H_
#define _DE_LBC_TYPE_H_

#include <linux/types.h>

union __lbc_ctl_reg_t {
	u32 dwval;
	struct {
		u32 lbc_en:1;
		u32 lbc_fcen:1;
		u32 alpmode:2;
		u32 clk_gate:1;
		u32 is_lossy:1;
		u32 r0:2;
		u32 rc_en:1;
		u32 r1:15;
		u32 alpha:8;
	} bits;
};

union __lbc_size_reg_t {
	u32 dwval;
	struct {
		u32 lay_width:12;
		u32 r0:4;
		u32 lay_height:12;
		u32 r1:4;
	} bits;
};

union __lbc_crop_coor_reg_t {
	u32 dwval;
	struct {
		u32 left_crop:16;
		u32 top_crop:16;
	} bits;
};

union __lbc_crop_size_reg_t {
	u32 dwval;
	struct {
		u32 width_crop:12;
		u32 r0:4;
		u32 height_crop:12;
		u32 r1:4;
	} bits;
};

union __lbc_laddr_reg_t {
	u32 dwval;
	struct {
		u32 laddr;
	} bits;
};

union __lbc_haddr_reg_t {
	u32 dwval;
	struct {
		u32 haddr:8;
		u32 r0:24;
	} bits;
};

union __lbc_seg_reg_t {
	u32 dwval;
	struct {
		u32 pitch:16;
		u32 seg_bit:16;
	} bits;
};

union __lbc_ovl_size_reg_t {
	u32 dwval;
	struct {
		u32 width_ovl:12;
		u32 r0:4;
		u32 height_ovl:12;
		u32 r1:4;
	} bits;
};

union __lbc_ovl_coor_reg_t {
	u32 dwval;
	struct {
		u32 ovl_coorx:16;
		u32 ovl_coory:16;
	} bits;
};

union __lbc_fcolor_reg_t {
	u32 dwval;
	struct {
		u32 lbc_fc;
	} bits;
};

struct __lbc_ovl_reg_t {
	union __lbc_ctl_reg_t lbc_ctl;
	union __lbc_size_reg_t lbc_v_size;
	union __lbc_crop_coor_reg_t lbc_crop_coor;
	union __lbc_crop_size_reg_t lbc_crop_size;
	union __lbc_laddr_reg_t lbc_laddr;
	union __lbc_haddr_reg_t lbc_haddr;
	union __lbc_seg_reg_t lbc_seg;
	union __lbc_ovl_size_reg_t lbc_ovl_size;
	union __lbc_ovl_coor_reg_t lbc_lay_coor;
	union __lbc_fcolor_reg_t lbc_fcolor;
};

#endif /* #ifndef _DE_LBC_TYPE_H_ */
