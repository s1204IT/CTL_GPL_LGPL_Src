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

#include <asm/arch-sunxi/cpu_ncat_v2.h>
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
 * riscv has the same addresses mapping as a7.
 */
static struct vaddr_range_t addr_mapping[] = {
};

int sunxi_riscv_init(u32 img_addr, u32 run_addr, u32 riscv_id)
{

	u32 reg_val;
	u32 image_len = 0;
	int ret;
	const char *fw_version = NULL;
	int map_size;

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

	fw_version = get_elf_fw_version_for_melis(img_addr, addr_mapping, map_size);
	if (fw_version) {
		show_img_version(fw_version, riscv_id);
	} else {
		printf("get elf fw version failed\n");
	}

	if (riscv_id == 0) { /* RISCV0 */
		printf("[bsp]: %s: %s(): +%d\n", __FILE__, __func__, __LINE__);
		/* clock gating */
		reg_val = readl_riscv(SUNXI_CCM_BASE + RISCV_GATING_RST_REG);
		reg_val |= RISCV_CLK_GATING;
		reg_val |= RISCV_GATING_RST_FIELD;
		writel_riscv(reg_val, SUNXI_CCM_BASE + RISCV_GATING_RST_REG);

		/* reset */
		reg_val = readl_riscv(SUNXI_CCM_BASE + RISCV_CFG_BGR_REG);
		reg_val |= RISCV_CFG_RST;
		reg_val |= RISCV_CFG_GATING;
		writel_riscv(reg_val, SUNXI_CCM_BASE + RISCV_CFG_BGR_REG);

		/* set start addr */
		reg_val = readl_riscv(RISCV_CFG_BASE + RISCV_STA_ADD_REG);
		reg_val |= run_addr;
		writel_riscv(reg_val, RISCV_CFG_BASE + RISCV_STA_ADD_REG);
		/* clock gating */
		reg_val = readl_riscv(SUNXI_CCM_BASE + RISCV_GATING_RST_REG);
		reg_val |= RISCV_CLK_GATING;
		reg_val |= RISCV_GATING_RST_FIELD;
		writel_riscv(reg_val, SUNXI_CCM_BASE + RISCV_GATING_RST_REG);

		/* set e907 clock to peri_600M */
		reg_val = readl_riscv(SUNXI_CCM_BASE + CCMU_RISCV_CLK_REG);
		reg_val |= RISCV_CLK_PERI_600M;
		writel_riscv(reg_val, SUNXI_CCM_BASE + CCMU_RISCV_CLK_REG);

		/* clock gating reset*/
		reg_val = readl_riscv(SUNXI_CCM_BASE + RISCV_GATING_RST_REG);
		reg_val &= ~(0xffff << 16);
		reg_val |= (0x3 << 1 | 0x16aa << 16);
		writel_riscv(reg_val, SUNXI_CCM_BASE + RISCV_GATING_RST_REG);

		RISCV_DEBUG("cfg bgr reg(0x%08x):0x%08x\n", SUNXI_CCM_BASE + RISCV_CFG_BGR_REG,
						readl_riscv(SUNXI_CCM_BASE + RISCV_CFG_BGR_REG));
		RISCV_DEBUG("start addr reg(0x%08x):0x%08x\n", RISCV_CFG_BASE + RISCV_STA_ADD_REG,
						readl_riscv(RISCV_CFG_BASE + RISCV_STA_ADD_REG));
		RISCV_DEBUG("clock gating reg(0x%08x):0x%08x\n", SUNXI_CCM_BASE + RISCV_GATING_RST_REG,
						readl_riscv(SUNXI_CCM_BASE + RISCV_GATING_RST_REG));
	}
	RISCV_DEBUG("RISCV%d start ok, img length %d, booting from 0x%x\n",
			riscv_id, image_len, run_addr);
	return 0;
}
