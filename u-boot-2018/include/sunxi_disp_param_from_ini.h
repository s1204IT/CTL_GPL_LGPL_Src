/*
 * include/sunxi_disp_param_from_ini.h
 *
 * Copyright (c) 2023 Allwinnertech Co., Ltd.
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
 */
#ifndef __SUNXI_DISP_PARAM_INI_H__
#define __SUNXI_DISP_PARAM_INI_H__

struct key_info {
	char key_name[50];
	char key_value[50];
};

extern char *get_file_from_partiton(char *file_name, char *part_name);
extern struct key_info *ini_get_group_info(char *group_name, const char *data);
extern int ini_get_int(struct key_info *p, char *key_name);
extern char *ini_get_string(struct key_info *p, char *key_name);
extern int get_struct_count(struct key_info *group_info);

#endif
