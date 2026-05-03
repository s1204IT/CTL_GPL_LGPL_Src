// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/riscv/common/riscv_img.c
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

#include <asm/io.h>
#include <common.h>
#include <sys_config.h>
#include "elf.h"
#include "imgdts.h"
#include "riscv_img.h"
#include "riscv_ic.h"
#include "riscv_fdt.h"
#include "elf_helpers.h"
#define ROUND_DOWN(a, b)	((a) & ~((b)-1))
#define ROUND_UP(a, b)		(((a) + (b)-1) & ~((b)-1))

#define ROUND_DOWN_CACHE(a) ROUND_DOWN(a, CONFIG_SYS_CACHELINE_SIZE)
#define ROUND_UP_CACHE(a)   ROUND_UP(a, CONFIG_SYS_CACHELINE_SIZE)

int show_img_version(const char *head_addr, u32 riscv_id)
{

	printf("riscv%d version:\n%s\n", riscv_id, head_addr);

	return 0;
}

int find_img_section(u32 img_addr,
			const char *section_name,
			unsigned long *section_addr)
{
	int i = 0;
	unsigned char *strtab = NULL;
	int ret = -1;

	void *ehdr = NULL;
	void *shdr = NULL;
	u8 class = fw_elf_get_class(img_addr);

	ehdr = (void *)(ADDR_TPYE)img_addr;
	shdr = (void *)(ADDR_TPYE)(img_addr + elf_hdr_get_e_shoff(class, ehdr)
		+ (elf_hdr_get_e_shstrndx(class, ehdr) * elf_size_of_shdr(class)));

	if (elf_shdr_get_sh_type(class, shdr) == SHT_STRTAB)
		strtab = (unsigned char *)(ADDR_TPYE)(img_addr + elf_shdr_get_sh_offset(class, shdr));

	for (i = 0; i < elf_hdr_get_e_shnum(class, ehdr); ++i) {
		shdr = (void *)(ADDR_TPYE)(img_addr + elf_hdr_get_e_shoff(class, ehdr)
				+ (i * elf_size_of_shdr(class)));

		if (!(elf_shdr_get_sh_flags(class, shdr) & SHF_ALLOC)
			|| (elf_shdr_get_sh_addr(class, shdr) == 0)
			|| (elf_shdr_get_sh_size(class, shdr) == 0)) {

			continue;
			}

		if (strtab) {

			char *pstr = (char *)(&strtab[elf_shdr_get_sh_name(class, shdr)]);

			if (strcmp(pstr, section_name) == 0) {
				RISCV_DEBUG("find riscv section: %s ,addr = 0x%lx\n",
					pstr, (unsigned long)elf_shdr_get_sh_addr(class, shdr));
				*section_addr = elf_shdr_get_sh_addr(class, shdr);
				ret = 0;
				break;
			}
		}
	}

	return ret;
}


int img_len_get(u32 img_addr,
		unsigned long section_addr,
		u32 *img_len)
{
	int ret = -1;
	int i = 0;
	struct spare_rtos_head_t *prtos = NULL;
	void *ehdr = NULL;
	void *phdr = NULL;
	u8 class = fw_elf_get_class(img_addr);

	ehdr = (void *)(ADDR_TPYE)img_addr;
	phdr = (void *)(ADDR_TPYE)(img_addr + elf_hdr_get_e_phoff(class, ehdr));


	for (i = 0; i < elf_hdr_get_e_phnum(class, ehdr); ++i) {
		if (section_addr == (unsigned long)elf_phdr_get_p_paddr(class, phdr)) {
			prtos = (struct spare_rtos_head_t *)(ADDR_TPYE)(img_addr
					+ elf_phdr_get_p_offset(class, phdr));
			*img_len = prtos->rtos_img_hdr.image_size;
			ret = 0;
			break;
		}
		phdr += elf_size_of_phdr(class);
	}
	return ret;
}


int set_msg_dts(u32 img_addr,
		unsigned long section_addr,
		struct dts_msg_t *dts_msg)
{
	int ret = -1;
	int i = 0;
	struct spare_rtos_head_t *prtos = NULL;
	void *ehdr = NULL;
	void *phdr = NULL;
	u8 class = fw_elf_get_class(img_addr);

	ehdr = (void *)(ADDR_TPYE)img_addr;
	phdr = (void *)(ADDR_TPYE)(img_addr + elf_hdr_get_e_phoff(class, ehdr));

	for (i = 0; i < elf_hdr_get_e_phnum(class, ehdr); ++i) {
		if (section_addr == (unsigned long)elf_phdr_get_p_paddr(class, phdr)) {
			prtos = (struct spare_rtos_head_t *)(ADDR_TPYE)(img_addr
					+ elf_phdr_get_p_offset(class, phdr));
			memcpy((void *)&prtos->rtos_img_hdr.dts_msg,
					(void *)dts_msg,
					sizeof(struct dts_msg_t));
			ret = 0;
			break;
		}
		phdr += elf_size_of_phdr(class);
	}
	return ret;
}



unsigned long set_img_va_to_pa(unsigned long vaddr,
				struct vaddr_range_t *map,
				int size)
{
	unsigned long paddr = vaddr;
	int i;

	for (i = 0; i < size; i++) {
		if (vaddr >= map[i].vstart
				&& vaddr <= map[i].vend) {
			paddr = vaddr - map[i].vstart + map[i].pstart;
			break;
		}
	}

	return paddr;
}

const char *get_elf_fw_version(u32 elf_fw_addr, struct vaddr_range_t *addr_map, int map_size)
{
	void *ehdr;
	void *shdr;
	void *shdr_shstrndx;
	u8 class = fw_elf_get_class(elf_fw_addr);
	const char *version_str = NULL;
	const char *name_table;
	int i = 0;

	ehdr = (void *)(ADDR_TPYE)elf_fw_addr;
	shdr = (void *)(ADDR_TPYE)(elf_fw_addr + elf_hdr_get_e_shoff(class, ehdr));
	shdr_shstrndx = ehdr + elf_hdr_get_e_shoff(class, ehdr)
			+ (elf_hdr_get_e_shstrndx(class, ehdr) * elf_size_of_shdr(class));
	name_table = ehdr + elf_shdr_get_sh_offset(class, shdr_shstrndx);
	for (i = 0; i < elf_hdr_get_e_shnum(class, ehdr); i++, shdr += elf_size_of_shdr(class)) {
		if (strcmp(name_table + elf_shdr_get_sh_name(class, shdr), ".version_table"))
			continue;

		version_str = (void *)(ADDR_TPYE)set_img_va_to_pa((unsigned long)elf_shdr_get_sh_addr(class, shdr), \
					addr_map, \
					map_size);
		return version_str;
	}
	return NULL;
}

const char *get_elf_fw_version_for_melis(u32 elf_fw_addr, struct vaddr_range_t *addr_map, int map_size)
{
	void *ehdr; /* Elf header structure pointer */
	void *phdr; /* Program header structure pointer */

	const char *version_str = NULL;
	void *dst = NULL;
	int i = 0;
	u8 class = fw_elf_get_class(elf_fw_addr);

	ehdr = (void *)(ADDR_TPYE)elf_fw_addr;
	phdr = (void *)(ADDR_TPYE)(elf_fw_addr + elf_hdr_get_e_phoff(class, ehdr));

	/* Load each program header */
	for (i = 0; i < elf_hdr_get_e_phnum(class, ehdr); ++i) {

		//remap addresses
		dst = (void *)(ADDR_TPYE)set_img_va_to_pa((unsigned long)elf_phdr_get_p_paddr(class, phdr), \
					addr_map, \
					map_size);

		if (i == 0) {
			version_str = (const char *)dst + 896;
			return version_str;
		}

		phdr += elf_size_of_phdr(class);
	}

	return NULL;
}

int load_elf_fw(u32 elf_fw_addr, struct vaddr_range_t *addr_map, int map_size)
{
	void *ehdr;
	void *phdr;
	void *dst = NULL;
	void *src = NULL;
	int i = 0;
	u8 class = fw_elf_get_class(elf_fw_addr);
	uint32_t flush_start_addr = 0, flush_cache_size = 0;

	ehdr = (void *)(ADDR_TPYE)elf_fw_addr;
	phdr = (void *)(ADDR_TPYE)elf_fw_addr + elf_hdr_get_e_phoff(class, ehdr);;

	if (!elf_hdr_get_e_phnum(class, ehdr)) {
		RISCV_DEBUG("there is no program header, please check whether the ELF file is correct\n");
		return -1;
	}

	/* Load each program header */
	for (i = 0; i < elf_hdr_get_e_phnum(class, ehdr); ++i) {

		//remap addresses
		dst = (void *)(ADDR_TPYE)set_img_va_to_pa((unsigned long)elf_phdr_get_p_paddr(class, phdr), \
					addr_map, \
					map_size);

		src = (void *)(ADDR_TPYE)elf_fw_addr + elf_phdr_get_p_offset(class, phdr);
		RISCV_DEBUG("Loading phdr %i from 0x%p to 0x%p (%lli bytes)\n",
		      i, src, dst, elf_phdr_get_p_filesz(class, phdr));

		if (elf_phdr_get_p_filesz(class, phdr))
			memcpy(dst, src, elf_phdr_get_p_filesz(class, phdr));

		/*
		 * In order to speed up firmware loading, don't clear .bss section in here.
		 * Remote processor will clean it when boot up.
		 */
		/*
		if (elf_phdr_get_p_filesz(class, phdr) != elf_phdr_get_p_memsz(class, phdr))
			memset(dst + elf_phdr_get_p_filesz(class, phdr), 0x00,
			elf_phdr_get_p_memsz(class, phdr) - elf_phdr_get_p_filesz(class, phdr));
		*/

		/*
		 * In order to avoid current cpu write back the cache content to memory after
		 * remote processor boot up(it will cause strange issue), we need to flush cache.
		 * And the range of flush cache is all memory which is used by remote processor.
		 */
		flush_start_addr = ROUND_DOWN_CACHE((unsigned long)dst);
		flush_cache_size = ROUND_UP_CACHE(elf_phdr_get_p_memsz(class, phdr));
		RISCV_DEBUG("flush_start_addr: 0x%08x, flush_cache_size: 0x%08x\n", flush_start_addr, flush_cache_size);
		flush_cache(flush_start_addr, flush_cache_size);
		phdr += elf_size_of_phdr(class);
	}
	return 0;
}
