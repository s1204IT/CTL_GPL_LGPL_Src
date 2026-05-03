// SPDX-License-Identifier: GPL-2.0+
/*
 *  drivers/dsp/dsp.c
 *
 * Copyright (c) 2020 Allwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 */

#include <asm/io.h>
#include <common.h>
#include "elf.h"
#include "fdt_support.h"
#include <sys_config.h>
#include <sunxi_image_verifier.h>

#include "dsp_reg.h"

#include <asm/arch-sunxi/efuse.h>

#define readl_dsp(addr)         readl((const volatile void*)(addr))
#define writel_dsp(val, addr)   writel((u32)(val), (volatile void*)(addr))

#define ROUND_DOWN(a, b) ((a) & ~((b)-1))
#define ROUND_UP(a,   b) (((a) + (b)-1) & ~((b)-1))

#define ROUND_DOWN_CACHE(a) ROUND_DOWN(a, CONFIG_SYS_CACHELINE_SIZE)
#define ROUND_UP_CACHE(a)   ROUND_UP(a, CONFIG_SYS_CACHELINE_SIZE)

#define DRIVER_VERSION "1.0.0"

/*
 * dsp need to remap addresses for some addr.
 */
struct vaddr_range_t {
	unsigned long vstart;
	unsigned long vend;
	unsigned long pstart;
};

static struct vaddr_range_t addr_mapping[] = {
	{ 0x00400000, 0x00410000, 0x00020000 },
	{ 0x10000000, 0x1fffffff, 0x40000000 },
	{ 0x30000000, 0x3fffffff, 0x40000000 },
};

static unsigned long set_img_va_to_pa(unsigned long vaddr,
								struct vaddr_range_t *map,
								int size)
{
	unsigned long paddr = vaddr;
	int i;

	for (i = 0; i < size; i++) {
			if (vaddr >= map[i].vstart
					&& vaddr <= map[i].vend) {
					paddr = paddr - map[i].vstart + map[i].pstart;
					break;
			}
	}

	return paddr;
}

static int dts_get_dsp_memory(ulong *start, u32 *size, u32 id)
{
	struct fdt_header *dtb_base = working_fdt;
	int nodeoffset;
	int ret;
	u32 reg_data[8];

	if (id == 0) {
		nodeoffset = fdt_path_offset(dtb_base, "/soc/dsp0_rproc");
		if (nodeoffset < 0)
			pr_err("no /soc/dsp0_rproc\n");
		nodeoffset = fdt_path_offset(dtb_base, "/reserved-memory/dsp0ddr");
		if (nodeoffset < 0) {
			pr_err("%s: no /reserved-memory/dsp0ddr_reserved in fdt\n", __func__);
			return -1;
		}
	} else {
		nodeoffset = fdt_path_offset(dtb_base, "/reserved-memory/dsp1ddr_reserved");
		if (nodeoffset < 0) {
			pr_err("%s: no /reserved-memory/dsp1ddr_reserved in fdt\n", __func__);
			return -1;
		}
	}

	memset(reg_data, 0, sizeof(reg_data));
	ret = fdt_getprop_u32(dtb_base, nodeoffset, "reg", reg_data);
	if (ret < 0) {
		pr_err("%s: error fdt get reg\n", __func__);
		return -2;
	}

	*start = reg_data[1];
	*size = reg_data[3];
	pr_err("start = 0x%x size =0x%x\n", (u32)*start, *size);
	return 0;
}

static void sunxi_dsp_set_runstall(u32 dsp_id, u32 value)
{
	u32 reg_val;

	reg_val = readl(SUNXI_DSP_CFG_BASE + DSP_CTRL_REG);
	reg_val &= ~(1 << BIT_RUN_STALL);
	reg_val |= (value << BIT_RUN_STALL);
	writel(reg_val, SUNXI_DSP_CFG_BASE + DSP_CTRL_REG);
}

static int load_image(u32 img_addr, u32 dsp_id)
{
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	int i;
	void *dst = NULL;
	void *src = NULL;
	int size = sizeof(addr_mapping) / sizeof(struct vaddr_range_t);
	ulong mem_start = 0;
	u32 mem_size = 0;

	ehdr = (Elf32_Ehdr *)img_addr;
	phdr = (Elf32_Phdr *)(img_addr + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; ++i) {

		dst = (void *)set_img_va_to_pa((uintptr_t)phdr->p_paddr, \
					addr_mapping, \
					size);

		src = (void *)img_addr + phdr->p_offset;

		pr_msg("Loading phdr %i is 0x%pto 0x%p (%i bytes)\n",
		      i, src, dst, phdr->p_filesz);

		if (phdr->p_filesz)
			memcpy(dst, src, phdr->p_filesz);

		if (phdr->p_filesz != phdr->p_memsz) {
			memset(dst + phdr->p_filesz, 0x00,
			       phdr->p_memsz - phdr->p_filesz);
			pr_err("phdr->p_filesz != phdr->p_memsz\n");
		}

		++phdr;
	}

	dts_get_dsp_memory(&mem_start, &mem_size, dsp_id);
	if (!mem_start || !mem_size) {
		pr_err("dts_get_dsp_memory fail\n");
	} else {
		flush_cache(ROUND_DOWN_CACHE(mem_start),
		ROUND_UP_CACHE(mem_size));
	}

	return 0;
}

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

static void sram_remap_set(int value)
{
	u32 val = 0;

	val = readl(SUNXI_R_PRCM_BASE + REMAP_CTRL_REG);
	val &= ~(1 << DSP_RAM_REMAP);
	val |= (value << DSP_RAM_REMAP);
	writel(val, SUNXI_R_PRCM_BASE + REMAP_CTRL_REG);
}

static void dsp_freq_default_set(void)
{
	u32 val = 0;

	val = DSP_CLK_SRC_PERI2X | DSP_CLK_FACTOR_M(2)
		| (1 << BIT_DSP_SCLK_GATING);
	writel_dsp(val, (SUNXI_CCMU_BASE + CCMU_DSP_CLK_REG));
}

static void ahbs_clk_set(void)
{
	u32 reg = SUNXI_R_PRCM_BASE;
	u32 val = 0;

	val |= BIT_AHBS_CLK_SRC_PERIPLL_DIV;
	writel(val, reg);

}

static int update_reset_vec(u32 img_addr, u32 *run_addr)
{
	Elf32_Ehdr *ehdr; /* Elf header structure pointer */

	ehdr = (Elf32_Ehdr *)img_addr;
	if (!*run_addr)
		*run_addr = ehdr->e_entry;

	return 0;
}

int sunxi_dsp_init(u32 img_addr, u32 run_ddr, u32 dsp_id)
{
	u32 reg_val = 0;
	u32 image_len = 0;
	int ret = 0;

	/* A523 Only one dsp core */
	pr_msg("Init DSP%d start\n", dsp_id);

	/* set uboot use local ram */
	sram_remap_set(1);

	/* update run addr */
	update_reset_vec(img_addr, &run_ddr);

	/* get image len */
	ret = get_image_len(img_addr, &image_len);
	if (ret) {
		pr_err("dsp%d:get img len err\n", dsp_id);
		return -1;
	}

	/* set mod_clk_freq */
	dsp_freq_default_set();

	/* set mod_clk */
	reg_val = readl_dsp(SUNXI_DSP_PRCM_BASE + DSP_CLK_REG);
	reg_val |= BIT_DSP_GATING | BIT_DSP_SYS_CLK;
	writel_dsp(reg_val, SUNXI_DSP_PRCM_BASE + DSP_CLK_REG);

	/* set cfg_clk */
	reg_val = readl_dsp(SUNXI_DSP_PRCM_BASE + DSP_CFG_BGR_REG);
	reg_val |= BIT_DSP_CFG_GATING;
	writel_dsp(reg_val, SUNXI_DSP_PRCM_BASE + DSP_CFG_BGR_REG);

	/* set ahbs_clk */
	ahbs_clk_set();

	/* set cfg to deassert  */
	reg_val = readl_dsp(SUNXI_DSP_PRCM_BASE + DSP_CFG_BGR_REG);
	reg_val |= BIT_DSP_CFG_RST;
	writel_dsp(reg_val, SUNXI_DSP_PRCM_BASE + DSP_CFG_BGR_REG);

	/* set external Reset Vector if needed */
	if (run_ddr != DSP_DEFAULT_RST_VEC) {
		writel_dsp(run_ddr, SUNXI_DSP_CFG_BASE + DSP_ALT_RESET_VEC_REG);
		reg_val = readl_dsp(SUNXI_DSP_CFG_BASE + DSP_CTRL_REG);
		reg_val |= (1 << BIT_START_VEC_SEL);
		writel_dsp(reg_val, SUNXI_DSP_CFG_BASE + DSP_CTRL_REG);
	}

	/* set runstall */
	sunxi_dsp_set_runstall(dsp_id, 1);

	/* de-assert dsp */
	reg_val = readl_dsp(SUNXI_DSP_PRCM_BASE + DSP_DBG_BGR_REG);
	reg_val |= BIT_DSP_RST;
	writel_dsp(reg_val, SUNXI_DSP_PRCM_BASE + DSP_DBG_BGR_REG);

	/* load image*/
	load_image(img_addr, dsp_id);

	/* set dsp use local ram */
	sram_remap_set(0);

	/* clear runstall */
	sunxi_dsp_set_runstall(dsp_id, 0);

	pr_msg("DSP%d start ok, img length %d, booting from 0x%x\n",
		dsp_id, image_len, run_ddr);

	return 0;
}
