/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
*/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/usb.h>
#include <sunxi_board.h>

#define VE_BOOT_SRAM_REMAP_REG (SUNXI_SRAMC_BASE + 0x4)
int sunxi_set_sramc_mode(void)
{
	u32 reg_val;
	void __iomem *reg_base = (void __iomem *)VE_BOOT_SRAM_REMAP_REG;

	/* SRAM:set sram to VE, default boot mode */
	reg_val = readl(reg_base);
	reg_val &= ~(0x1 << 1);
	writel(reg_val, reg_base);
	debug("set sram to VE\n");

	reg_val = readl(reg_base);
	if (reg_val & (0x1 << 1))
		pr_err("set sram to VE fail!\n");
	return 0;
}

void otg_phy_config(void)
{
	u32 reg_val;
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBOTG_BASE +
							USBC_REG_o_PHYCTL));
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBOTG_BASE +
						  USBC_REG_o_PHYCTL));
}