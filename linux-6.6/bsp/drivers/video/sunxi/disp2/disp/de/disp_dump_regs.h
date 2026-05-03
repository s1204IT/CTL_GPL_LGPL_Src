/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2024 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DISP_DUMP_REGS_H__
#define __DISP_DUMP_REGS_H__

#include "include.h"

extern int dr_debug_val;

#define DUMP_REGS_PRINT(fmt, args...) do {						\
	if (unlikely(dr_debug_val)) {								\
			if (dr_debug_val > 1)								\
				sunxi_info(NULL, "[DumpRegs]: " fmt, ## args);	\
			else												\
				sunxi_err(NULL, "[DumpRegs]: " fmt, ## args);	\
		}														\
	} while (0)

enum disp_module_type {
	DISP_MODULE_NONE = 0,
	DISP_MODULE_ALL,
	DISP_DE_TOP,
	DISP_BASE_BLOCK,
	DISP_VEP_BLOCK,
	DISP_DEP_BLOCK,
	DISP_WB_BLOCK,

	/* base module */
	DISP_OVL_MODULE,
	DISP_FBD_MODULE,
	DISP_SCALER_MODULE,
	DISP_BLENDER_MODULE,
	DISP_CSC_MODULE,

	/* vep module */
	DISP_SNR_MODULE,
	DISP_SHARP_MODULE,
	DISP_CDC_MODULE,
	DISP_DCI_MODULE,
	DISP_FCM_MODULE,
	DISP_FCE_MODULE,
	DISP_PEAK_MODULE,
	DISP_LCTI_MODULE,
	DISP_BLS_MODULE,
	DISP_FCC_MODULE,

	/* dep module */
	DISP_DEBAND_MODULE,
	DISP_DRC_MODULE,
	DISP_COLOR_MATRIX_MODULE,
	DISP_GAMMA_MODULE,
	DISP_DITHER_MODULE,
	DISP_KSC_MODULE,
	DISP_FMT_MODULE,

	/* timing contrl module */
	DISP_TCON_MODULE,

	/* help info */
	DISP_HELP_INFO,
	DISP_ENABLE_STATUS,
	DISP_MODULE_NUM
};

extern const char *module_name[DISP_MODULE_NUM];

struct dump_regs_index {
	unsigned int disp;
	unsigned int chn;
	unsigned int id;
};

struct dump_regs_user_setting {
	int force_dump;

	enum disp_module_type type;

	int use_index_find;
	struct dump_regs_index index;
};

struct dump_regs_ops {
	int (*dump)(char *buf);
	int (*dump_by_user_setting)(const struct dump_regs_user_setting *setting, char *buf);

	int (*get_enable_info)(const struct dump_regs_user_setting *setting, char *buf);
	int (*get_enable_status)(const struct dump_regs_user_setting *setting);
};

struct dump_regs_module {
	char name[32];
	enum disp_module_type type;
	struct dump_regs_ops ops;
	struct mutex lock;
	void *par; /* int [] */
};

int dr_init(void);
int register_dr_module(struct dump_regs_module *mod);
void unregister_dr_module(struct dump_regs_module *mod);
struct dump_regs_module *dr_module_alloc(size_t size);
void dr_module_release(struct dump_regs_module *mod);
void dr_ioread(const void __iomem *addr, void *dst, long count);

int dump_regs(char *buf);
int get_de_user_setting(struct dump_regs_user_setting *setting);
int get_dr_debug_info(void);
int set_dr_force_dump(int enable);
int set_dr_module_type(const char *name);
int set_dr_index(struct dump_regs_index index);
int set_dr_debug_info(int value);

#endif /*__DISP_DUMP_REGS_H__*/
