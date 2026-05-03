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

#define readl_riscv(addr)	readl((const volatile void*)(addr))
#define writel_riscv(val, addr)	writel((u32)(val), (volatile void*)(addr))

/*
 * riscv need to remap addresses for some addr.
 */
static struct vaddr_range_t addr_mapping[] = {
	{ 0x3FFC0000, 0x4003FFFF, 0x07280000 },
};

int sunxi_riscv_init(u32 img_addr, u32 run_addr, u32 riscv_id)
{
	u32 reg_val;
	u32 image_len = 0;
	int ret;
	const char *fw_version = NULL;
	int map_size;

	/* A523 Only one riscv core, ignore riscv_id arguent */
	(void)riscv_id;
#ifdef CONFIG_SUNXI_VERIFY_RISCV
	if (sunxi_verify_riscv(img_addr, image_len, riscv_id) < 0) {
		return -1;
	}
#endif
	/* update run addr */
	if (!run_addr)
		run_addr = get_elf_fw_entry(img_addr);

	/* De-assert PUBSRAM Clock and Gating */
	reg_val = readl_riscv(SUNXI_DSP_PRCM_BASE + RISCV_PUBSRAM_CFG_REG);
	reg_val |= BIT_RISCV_PUBSRAM_RST;
	reg_val |= BIT_RISCV_PUBSRAM_GATING;
	writel_riscv(reg_val, SUNXI_DSP_PRCM_BASE + RISCV_PUBSRAM_CFG_REG);

	/* load image to ram */
	map_size = sizeof(addr_mapping) / sizeof(struct vaddr_range_t);
	ret = load_elf_fw(img_addr, addr_mapping, map_size);
	if (ret) {
		printf("load elf fw faild, ret: %d\n", ret);
		return -2;
	}

	fw_version = get_elf_fw_version_for_melis(img_addr, addr_mapping, map_size);
	if (fw_version) {
		show_img_version(fw_version, riscv_id);
	} else {
		printf("get elf fw version failed\n");
	}

	/* assert */
	writel_riscv(0, SUNXI_DSP_PRCM_BASE + RISCV_CFG_BGR_REG);

	/* set start addr */
	reg_val = run_addr;
	writel_riscv(reg_val, RISCV_CFG_BASE + RISCV_STA_ADD_REG);

	/* de-assert */
	reg_val = BIT_RISCV_CORE_RST | BIT_RISCV_APB_DB_RST |
				BIT_RISCV_CFG_RST | BIT_RISCV_CFG_GATING;
	writel_riscv(reg_val, SUNXI_DSP_PRCM_BASE + RISCV_CFG_BGR_REG);

	/* clock gating */
	reg_val = readl_riscv(SUNXI_DSP_PRCM_BASE + RISCV_CLK_REG);
	reg_val |= BIT_RISCV_CLK_GATING;
	writel_riscv(reg_val, SUNXI_DSP_PRCM_BASE + RISCV_CLK_REG);

	RISCV_DEBUG("RISCV start ok, img length %d, boot addr 0x%x\n",
						image_len, run_addr);

	return 0;
}
