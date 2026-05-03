/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2022 Copyright (c)
 *
 *  File name   :       de_dci_type.h
 *
 *  Description :       display engine 35x basic function declaration
 *
 *  History     :       2021/10/20  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_DCI_TYPE_H_
#define _DE_DCI_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union dci_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:3;
		u32 ftd_en:1;
		u32 res1:11;
		u32 demo_en:1;
		u32 res2:14;
		u32 chroma_comp_en:1;
	} bits;
};

/*offset:0x0090*/
union dci_ftd_hue_thr_reg {
	u32 dwval;
	struct {
		u32 ftd_hue_low_thr:9;
		u32 res0:7;
		u32 ftd_hue_high_thr:9;
		u32 res1:7;
	} bits;
};

/*offset:0x0094*/
union dci_ftd_chroma_thr_reg {
	u32 dwval;
	struct {
		u32 ftd_chr_low_thr:9;
		u32 res0:7;
		u32 ftd_chr_high_thr:9;
		u32 res1:7;
	} bits;
};

/*offset:0x0098*/
union dci_ftd_slp_reg {
	u32 dwval;
	struct {
		u32 ftd_hue_low_slp:4;
		u32 res0:4;
		u32 ftd_hue_high_slp:4;
		u32 res1:4;
		u32 ftd_chr_low_slp:4;
		u32 res3:4;
		u32 ftd_chr_high_slp:4;
		u32 res4:4;
	} bits;
};

/*offset:0x009c*/
union dci_ftd_people_gain_reg {
	u32 dwval;
	struct {
		u32 ftd_people_gain_en:1;
		u32 res0:7;
		u32 ftd_people_gain:8;
		u32 ftd_chr_little_thr:8;
		u32 res1:8;
	} bits;
};

struct dci_reg {
    /*0x0000*/
    union dci_ctl_reg ctl;
    /*0x0004*/
    u32 res0[35];
    /*0x0090*/
    union dci_ftd_hue_thr_reg ftd_hue_thr;
    /*0x0094*/
    union dci_ftd_chroma_thr_reg ftd_chroma_thr;
    /*0x0098*/
    union dci_ftd_slp_reg ftd_slp;
    /*0x009C*/
    union dci_ftd_people_gain_reg ftd_people_gain;
};

struct dci_status {
	/* Frame number of dci run */
	u32 runtime;
	/* dci enabled */
	u32 isenable;
};

struct dci_config {
    u8 contrast_level;
};

#endif /* #ifndef _DE_DCI_TYPE_H_ */
