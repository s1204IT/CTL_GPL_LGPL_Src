/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * drivers/riscv/common/riscv_img.h
 *
 * Copyright (c) 2007-2025 Allwinnertech Co., Ltd.
 * Author: wujiayi <wujiayi@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 */

#ifndef __RISCV_IMG_H
#define __RISCV_IMG_H
#include"riscv_ic.h"
#include "elf_helpers.h"
struct vaddr_range_t {
	unsigned long vstart;
	unsigned long vend;
	unsigned long pstart;
};

int show_img_version(const char *head_addr, u32 riscv_id);

int find_img_section(u32 img_addr,
			const char *section_name,
			unsigned long *section_addr);

int img_len_get(u32 img_addr,
		unsigned long section_addr,
		u32 *img_len);

int set_msg_dts(u32 img_addr,
		unsigned long section_addr,
		struct dts_msg_t *dts_msg);

unsigned long set_img_va_to_pa(unsigned long vaddr,
				struct vaddr_range_t *map,
				int size);

static inline int get_elf_fw_entry(u32 elf_fw_addr)
{
	return elf_hdr_get_e_entry(fw_elf_get_class(elf_fw_addr), (void *)elf_fw_addr);
}

const char *get_elf_fw_version(u32 elf_fw_addr, struct vaddr_range_t *addr_map, int map_size);
const char *get_elf_fw_version_for_melis(u32 elf_fw_addr, struct vaddr_range_t *addr_map, int map_size);

int load_elf_fw(u32 elf_fw_addr, struct vaddr_range_t *addr_map, int map_size);
#endif

