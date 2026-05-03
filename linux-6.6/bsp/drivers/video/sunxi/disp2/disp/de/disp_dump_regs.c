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

#include "disp_dump_regs.h"
#include "disp_manager.h"

const char *module_name[DISP_MODULE_NUM] = {
	[DISP_MODULE_ALL] = "all",
	[DISP_DE_TOP] = "de_top",
	[DISP_BASE_BLOCK] = "base",
	[DISP_VEP_BLOCK] = "vep",
	[DISP_DEP_BLOCK] = "dep",
	[DISP_WB_BLOCK] = "capture",

	/* base module */
	[DISP_OVL_MODULE] = "ovl",
	[DISP_FBD_MODULE] = "fbd",
	[DISP_SCALER_MODULE] = "scaler",
	[DISP_BLENDER_MODULE] = "blender",
	[DISP_CSC_MODULE] = "csc",

	/* vep module */
	[DISP_SNR_MODULE] = "snr",
	[DISP_SHARP_MODULE] = "sharp",
	[DISP_CDC_MODULE] = "cdc",
	[DISP_DCI_MODULE] = "dci",
	[DISP_FCM_MODULE] = "fcm",
	[DISP_FCE_MODULE] = "fce",
	[DISP_PEAK_MODULE] = "peak",
	[DISP_LCTI_MODULE] = "lcti",
	[DISP_BLS_MODULE] = "bls",
	[DISP_FCC_MODULE] = "fcc",

	/* dep module */
	[DISP_DEBAND_MODULE] = "deband",
	[DISP_DRC_MODULE] = "drc",
	[DISP_COLOR_MATRIX_MODULE] = "color_matrix",
	[DISP_GAMMA_MODULE] = "gamma",
	[DISP_DITHER_MODULE] = "dither",
	[DISP_KSC_MODULE] = "ksc",
	[DISP_FMT_MODULE] = "fmt",

	/* timing contrl module */
	[DISP_TCON_MODULE] = "tcon",

	/* help info */
	[DISP_HELP_INFO] = "help",
	[DISP_ENABLE_STATUS] = "enable_status"
};

struct dump_regs_info {
	struct mutex lock; /* lock for user setting */
	struct mutex buf_lock; /* lock for buf */
	struct dump_regs_user_setting setting;
	char buf[PAGE_SIZE * 2];

	struct dump_regs_module *register_module[DISP_MODULE_NUM];
	unsigned int module_num;
};

int dr_debug_val;
struct dump_regs_info dr_info;

static DEFINE_MUTEX(registration_lock);

static int dr_help_info(char *buf)
{
	int i, j, misalign_num, num = 0;

	num += sprintf(buf + num, "Disp2 Dump Regs:\n\n");
	num += sprintf(buf + num, "regs info:\n");
	num += sprintf(buf + num, "dump_regs, to get regs info\n");
	num += sprintf(buf + num, "\tex:\n");
	num += sprintf(buf + num, "\t\tcat dump_regs\n\n");
	num += sprintf(buf + num, "ctrl cmd:\n");
	num += sprintf(buf + num, "dump_regs_force, forced printing of disabled modules, may cause crashes, 1: forced 0: not\n");
	num += sprintf(buf + num, "\tex:\n");
	num += sprintf(buf + num, "\t\techo 1/0 > dump_regs_force, cat dump_regs_force\n");
	num += sprintf(buf + num, "dump_regs_module, set the current printing module, support:\n");
	num += sprintf(buf + num, "\t<name>: \n");
	for (i = 0; i < DISP_MODULE_NUM / 4; i++) {
		num += sprintf(buf + num, "\t\t%-14s %-14s %-14s %-14s\n", module_name[i * 4],
			  module_name[i * 4 + 1], module_name[i * 4 + 2], module_name[i * 4 + 3]);
	}
	misalign_num = DISP_MODULE_NUM % 4;
	if (misalign_num) {
		num += sprintf(buf + num, "\t\t");
		for (j = 0; j < misalign_num; j++) {
			num += sprintf(buf + num, "%-14s ", module_name[(i - 1) * 4 + j]);
		}
		num += sprintf(buf + num, "\n");
	}
	num += sprintf(buf + num, "\tex:\n");
	num += sprintf(buf + num, "\t\techo <name> > dump_regs_module, cat dump_regs_module\n");
	num += sprintf(buf + num, "dump_regs_index, set index, 0 0 0 -> disp0 chn0 id0\n");
	num += sprintf(buf + num, "\tex:\n");
	num += sprintf(buf + num, "\t\techo 0 0 0 > dump_regs_module, cat dump_regs_module\n");
	num += sprintf(buf + num, "dump_regs_debug, set debug level, 0: close 1: err >1: info\n");
	num += sprintf(buf + num, "\tex:\n");
	num += sprintf(buf + num, "\t\techo 1/0 > dump_regs_debug, cat dump_regs_debug\n");

	return num;
}

/* static int dump_enable_status_diagram(void); */

static int do_register_dr_module(struct dump_regs_module *mod)
{
	if (!mod) {
		DUMP_REGS_PRINT("register module null hdl\n");
		return -1;
	}
	if (dr_info.module_num == DISP_MODULE_NUM) {
		DUMP_REGS_PRINT("register module num out of range\n");
		return -1;
	}

	dr_info.register_module[dr_info.module_num++] = mod;
	DUMP_REGS_PRINT("register module %s, index %d", mod->name, dr_info.module_num);
	return 0;
}

static void do_unregister_dr_module(struct dump_regs_module *mod)
{
	int i, j;
	unsigned int rgst_num = dr_info.module_num;
	struct dump_regs_module **mod_list = dr_info.register_module;

	if (!mod) {
		DUMP_REGS_PRINT("unregister module null hdl\n");
		return;
	}

	for (i = 0; i < rgst_num; i++) {
		if (mod->type != mod_list[i]->type)
			continue;

		mod_list[i] = NULL;
		/* Compactly arranged module list */
		for (j = i; j < rgst_num; j++)
			mod_list[j] = mod_list[j + 1];
		dr_info.module_num--;

		DUMP_REGS_PRINT("unregister module %s, index %d\n", mod->name, i + 1);
	}
}

static int check_buf_overflow(unsigned int size, char *buf)
{
	char *cursor;
	char help_info[128] = {'\0'};
	int help_info_num;

	if (!buf || size < PAGE_SIZE)
		return 0;

	DUMP_REGS_PRINT("buf overflow\n");
	help_info_num = sprintf(help_info, "\n**buf overflow, lost size %ld+,"
		       "you can use more precise parameters to control regs printing**\n", size - PAGE_SIZE);
	cursor = buf + (PAGE_SIZE - help_info_num - 1);
	memcpy(cursor, help_info, help_info_num);
	cursor[help_info_num] = '\0';
	return -1;
}

static int is_base_module(enum disp_module_type type)
{
	return (type == DISP_OVL_MODULE) || (type == DISP_FBD_MODULE) ||
	       (type == DISP_SCALER_MODULE) || (type == DISP_BLENDER_MODULE) ||
	       (type == DISP_CSC_MODULE);
}
static int is_vep_module(enum disp_module_type type)
{
	return (type == DISP_SNR_MODULE) || (type == DISP_SHARP_MODULE) ||
	       (type == DISP_CDC_MODULE) || (type == DISP_DCI_MODULE) ||
	       (type == DISP_FCM_MODULE) || (type == DISP_FCE_MODULE) ||
	       (type == DISP_PEAK_MODULE) || (type == DISP_LCTI_MODULE) ||
	       (type == DISP_BLS_MODULE) || (type == DISP_FCC_MODULE);
}

static int is_dep_module(enum disp_module_type type)
{
	return (type == DISP_DEBAND_MODULE) || (type == DISP_DRC_MODULE) ||
	       (type == DISP_COLOR_MATRIX_MODULE) || (type == DISP_GAMMA_MODULE) ||
	       (type == DISP_DITHER_MODULE) || (type == DISP_KSC_MODULE) ||
	       (type == DISP_FMT_MODULE);
}

static enum disp_module_type dr_moudle_name2type(const char *name)
{
	int i;

	for (i = DISP_MODULE_NONE; i < DISP_MODULE_NUM; i++) {
		DUMP_REGS_PRINT("name %s, module_name %s\n", name, module_name[i]);
		if (module_name[i] && !strcmp(name, module_name[i]))
			return (enum disp_module_type)i;
	}

	return DISP_MODULE_NONE;
}

static void print_user_setting(struct dump_regs_user_setting *setting)
{
	DUMP_REGS_PRINT("user setting param:\n");
	DUMP_REGS_PRINT("force_dump %d\n", setting->force_dump);
	DUMP_REGS_PRINT("type %d\n", setting->type);
	DUMP_REGS_PRINT("use_index_find %d disp%d chn%d id%d\n", setting->use_index_find,
		setting->index.disp, setting->index.chn, setting->index.id);
}

int get_de_user_setting(struct dump_regs_user_setting *setting)
{
	if (!setting)
		return -1;

	mutex_lock(&dr_info.lock);
	memcpy(setting, &dr_info.setting, sizeof(*setting));
	mutex_unlock(&dr_info.lock);
	return 0;
}

int get_dr_debug_info(void)
{
	return dr_debug_val;
}

int set_dr_force_dump(int enable)
{
	mutex_lock(&dr_info.lock);
	dr_info.setting.force_dump = enable & 0x1;
	mutex_unlock(&dr_info.lock);

	return 0;
}

int set_dr_module_type(const char *name)
{
	mutex_lock(&dr_info.lock);
	dr_info.setting.type = dr_moudle_name2type(name);
	mutex_unlock(&dr_info.lock);

	return 0;
}

int set_dr_index(struct dump_regs_index index)
{
	int enable = 1;
	struct disp_manager *mgr;
	int force_dump;

	mutex_lock(&dr_info.lock);
	force_dump = dr_info.setting.force_dump;
	mutex_unlock(&dr_info.lock);

	if (index.disp >= bsp_disp_feat_get_num_screens()) {
		DUMP_REGS_PRINT("set disp index %d over range\n", index.disp);
		enable = 0;
		goto set_index;
	}

	mgr = disp_get_layer_manager(index.disp);
	if (!mgr && mgr->is_enabled) {
		if (!mgr->is_enabled(mgr) && !force_dump) {
			DUMP_REGS_PRINT("disp %d not enable, set force_dump to bypass\n", index.disp);
			enable = 0;
			goto set_index;
		}
	}

	if (index.chn >= bsp_disp_feat_get_num_channels(index.disp)) {
		DUMP_REGS_PRINT("set disp%d chn index %d over range\n", index.disp, index.chn);
		enable = 0;
		goto set_index;
	}


set_index:
	mutex_lock(&dr_info.lock);
	dr_info.setting.use_index_find = enable;
	memcpy(&dr_info.setting.index, &index, sizeof(index));
	mutex_unlock(&dr_info.lock);

	return 0;
}

int set_dr_debug_info(int value)
{
	dr_debug_val = value;
	return 0;
}

int dr_init(void)
{
	mutex_init(&dr_info.lock);
	mutex_init(&dr_info.buf_lock);

	dr_info.setting.type = DISP_HELP_INFO;

	return 0;
}

int register_dr_module(struct dump_regs_module *mod)
{
	int ret;

	mutex_lock(&registration_lock);
	ret = do_register_dr_module(mod);
	DUMP_REGS_PRINT("register module: %s\n", mod->name);
	mutex_unlock(&registration_lock);

	return ret;
}

void unregister_dr_module(struct dump_regs_module *mod)
{
	mutex_lock(&registration_lock);
	do_unregister_dr_module(mod);
	mutex_unlock(&registration_lock);
}

struct dump_regs_module *dr_module_alloc(size_t size)
{
#define BYTES_PER_LONG (BITS_PER_LONG/8)
#define PADDING (BYTES_PER_LONG - (sizeof(struct fb_info) % BYTES_PER_LONG))
	int module_size = sizeof(struct dump_regs_module);
	struct dump_regs_module *mod;
	char *p;

	if (size)
		module_size += PADDING;

	p = kzalloc(module_size + size, GFP_KERNEL);
	if (!p) {
		DUMP_REGS_PRINT("alloc dr module fail\n");
		return NULL;
	}

	mod = (struct dump_regs_module *)p;

	if (size)
		mod->par = p + module_size;

	mutex_init(&mod->lock);
	return mod;
}

void dr_module_release(struct dump_regs_module *mod)
{
	if (!mod)
		return;

	kfree(mod);
}

/* only for dump regs func */
#define DUMP_REGS_BY_SETTING_IF_CONDITION_IS_TURE(CONDITION)  do {				\
	for (i = 0; i < rgst_num; i++) {							\
		if (!(CONDITION))								\
			continue;								\
		if (mod_list[i]->ops.dump_by_user_setting)					\
			buf_size += mod_list[i]->ops.dump_by_user_setting(&setting, mbuf);	\
		DUMP_REGS_PRINT("buf_size %d mbuf addr %lx\n", buf_size, (uintptr_t)mbuf);	\
		/* DUMP_REGS_PRINT("buf: %s\n", mbuf); */					\
		if (check_buf_overflow(buf_size, dr_info.buf) < 0) {				\
			buf_size = PAGE_SIZE - 1;						\
			break;									\
		}										\
		mbuf = dr_info.buf + buf_size;							\
	}											\
} while (0)

int dump_regs(char *buf)
{
	struct dump_regs_user_setting setting;
	unsigned int rgst_num = dr_info.module_num;
	struct dump_regs_module **mod_list = dr_info.register_module;
	char *mbuf = dr_info.buf;
	unsigned int i, buf_size = 0;

	mutex_lock(&dr_info.lock);
	memcpy(&setting, &dr_info.setting, sizeof(dr_info.setting));
	mutex_unlock(&dr_info.lock);

	DUMP_REGS_PRINT("%s: start >\n", __func__);
	print_user_setting(&setting);

	mutex_lock(&dr_info.buf_lock);
	memset(dr_info.buf, '\0', sizeof(dr_info.buf));
	switch (setting.type) {
	case DISP_MODULE_NONE:
		DUMP_REGS_PRINT("%s: type none\n", __func__);
		break;
	case DISP_MODULE_ALL:
		DUMP_REGS_PRINT("%s: type all\n", __func__);
		DUMP_REGS_BY_SETTING_IF_CONDITION_IS_TURE(mod_list[i]);
		break;
	case DISP_BASE_BLOCK:
		DUMP_REGS_PRINT("%s: type base block\n", __func__);
		DUMP_REGS_BY_SETTING_IF_CONDITION_IS_TURE(is_base_module(mod_list[i]->type));
		break;
	case DISP_VEP_BLOCK:
		DUMP_REGS_PRINT("%s: type vep block\n", __func__);
		DUMP_REGS_BY_SETTING_IF_CONDITION_IS_TURE(is_vep_module(mod_list[i]->type));
		break;
	case DISP_DEP_BLOCK:
		DUMP_REGS_PRINT("%s: type dep block\n", __func__);
		DUMP_REGS_BY_SETTING_IF_CONDITION_IS_TURE(is_dep_module(mod_list[i]->type));
		break;
	case DISP_HELP_INFO:
		DUMP_REGS_PRINT("%s: type help info\n", __func__);
		buf_size += dr_help_info(dr_info.buf);
		break;
	case DISP_ENABLE_STATUS:
		DUMP_REGS_PRINT("%s: type enable status\n", __func__);

		break;
	default:
		DUMP_REGS_PRINT("%s: type default module\n", __func__);
		DUMP_REGS_BY_SETTING_IF_CONDITION_IS_TURE(setting.type == mod_list[i]->type);
		break;
	}

	/* memcpy(buf, dr_info.buf, buf_size); */
	buf_size = sysfs_emit(buf, "%s", dr_info.buf);
	mutex_unlock(&dr_info.buf_lock);
	DUMP_REGS_PRINT("%s: end, total size %d\n", __func__, buf_size);

	return buf_size;
}
