// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/riscv/sun8iw21/riscv.c
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

//#include <asm/arch-sunxi/cpu_ncat_v2.h>
#include <asm/io.h>
#include <common.h>
#include <sys_config.h>
#include <sunxi_image_verifier.h>

#include "platform.h"
#include "elf.h"
#include "fdt_support.h"
#include "riscv_reg.h"
#include "../common/riscv_fdt.h"
#include "../common/riscv_img.h"
#include "../common/riscv_ic.h"

#ifdef CONFIG_SUNXI_IMAGE_HEADER
#include <sunxi_image_header.h>
#endif

#define readl_riscv(addr)	readl((const volatile void*)(addr))
#define writel_riscv(val, addr)	writel((u32)(val), (volatile void*)(addr))

/*
 * riscv need to remap addresses for some addr.
 * riscv has the same addresses mapping as a7.
 */
static struct vaddr_range_t addr_mapping[] = {
};

static int get_image_len(u32 img_addr, u32 *img_len)
{
	int i = 0;
	int ret = -1;
	struct spare_rtos_head_t *prtos = NULL;
	Elf32_Ehdr *ehdr = NULL; /* Elf header structure pointer */
	Elf32_Phdr *phdr = NULL; /* Program header structure pointer */

	ehdr = (Elf32_Ehdr *)img_addr;
	phdr = (Elf32_Phdr *)(img_addr + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; ++i) {
		if (!(unsigned long)phdr->p_paddr) {
			prtos = (struct spare_rtos_head_t *)(img_addr
					+ phdr->p_offset);
			*img_len = prtos->rtos_img_hdr.image_size;
			ret = 0;
			break;
		}
		++phdr;
	}

	return ret;
}

extern void sunxi_e907_clock_reset(void);
extern void sunxi_e907_clock_init(u32 addr);
extern int sunxi_get_secureboard(void);

int sunxi_riscv_init(u32 img_addr, u32 run_addr, u32 riscv_id)
{
	u32 image_len = 0;
	int ret;
	const char *fw_version = NULL;
	int map_size;
	int secure_mode = 0;

	secure_mode = sunxi_get_secureboard();
	if (!secure_mode)
		return 0;

#ifdef CONFIG_SUNXI_IMAGE_HEADER
       sunxi_image_header_t *ih = (sunxi_image_header_t *)img_addr;

       image_len = get_image_len((u32)((uint8_t *)ih + ih->ih_hsize), &image_len);
#else
       image_len = get_image_len(img_addr, &image_len);
#endif

#ifdef CONFIG_SUNXI_VERIFY_RISCV
	if (sunxi_verify_riscv(img_addr, image_len, riscv_id) < 0) {
		return -1;
	}
#endif
	/* update run addr */
	if (!run_addr)
		run_addr = get_elf_fw_entry(img_addr);

	/* load image to ram */
	map_size = sizeof(addr_mapping) / sizeof(struct vaddr_range_t);
	ret = load_elf_fw(img_addr, addr_mapping, map_size);
	if (ret) {
		printf("load elf fw faild, ret: %d\n", ret);
		return -2;
	}

	fw_version = get_elf_fw_version(img_addr, addr_mapping, map_size);
	if (fw_version) {
		show_img_version(fw_version, riscv_id);
	} else {
		printf("get elf fw version failed\n");
	}

	if (riscv_id == 0) { /* RISCV0 */
		/* assert e907 */
		sunxi_e907_clock_reset();

		/* de-assert e907 */
		sunxi_e907_clock_init(run_addr);
	}

	RISCV_DEBUG("RISCV%d start, img length %d, booting from 0x%x\n",
			riscv_id, image_len, run_addr);

	return 0;
}
