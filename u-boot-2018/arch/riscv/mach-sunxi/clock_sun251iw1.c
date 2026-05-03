/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
*/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/prcm.h>
#include <asm/arch/efuse.h>
#include "private_uboot.h"

void clock_init_uart(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* close the clock for uart */
	clrbits_le32(&ccm->uart_gate_reset,
		     1 << (uboot_spare_head.boot_data.uart_port));
	udelay(2);

	/* assert uart reset */
	clrbits_le32(&ccm->uart_gate_reset,
		     1 << (RESET_SHIFT + uboot_spare_head.boot_data.uart_port));
	udelay(2);

	/* deassert uart reset */
	setbits_le32(&ccm->uart_gate_reset,
		     1 << (RESET_SHIFT + uboot_spare_head.boot_data.uart_port));

	/* open the clock for uart */
	setbits_le32(&ccm->uart_gate_reset,
		     1 << (uboot_spare_head.boot_data.uart_port));
}

uint clock_get_pll_ddr(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_m, factor_p0, pll_ddr;

	reg_val = readl(&ccm->pll5_cfg);

	factor_n = (((reg_val & PLL_DDR_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_DDR_CTRL_REG_PLL_N_OFFSET)) + 1;
	factor_p0 = ((reg_val & PLL_DDR_CTRL_REG_PLL_INPUT_DIV2_CLEAR_MASK) >> PLL_DDR_CTRL_REG_PLL_INPUT_DIV2_OFFSET) + 1;
	factor_m = ((reg_val & PLL_DDR_CTRL_REG_PLL_OUTPUT_DIV2_CLEAR_MASK) >> PLL_DDR_CTRL_REG_PLL_OUTPUT_DIV2_OFFSET) + 1;
	pll_ddr = (24 * factor_n / factor_m / factor_p0);

	return pll_ddr;
}

uint clock_get_pll_cpu(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_m0, factor_m1, factor_p, pll_cpu;

	reg_val = readl(&ccm->pll1_cfg);

	factor_n = (((reg_val & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_N_OFFSET));
	factor_m0 = ((reg_val & PLL_CPU_CTRL_REG_PLL_M0_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_M0_OFFSET) + 1;
	factor_m1 = ((reg_val & PLL_CPU_CTRL_REG_PLL_M1_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_M1_OFFSET) + 1;
	factor_p = ((reg_val & PLL_CPU_CTRL_REG_PLL_P_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_P_OFFSET) + 1;
	pll_cpu = (24 * factor_n / factor_p / factor_m0 / factor_m1);

	return pll_cpu;
}

uint clock_get_pll_peri1x(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_m, factor_p0, pll_peri1x;

	reg_val = readl(&ccm->pll6_cfg);

	factor_n = (((reg_val & PLL_PERI_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_PERI_CTRL_REG_PLL_N_OFFSET)) + 1;
	factor_p0 = ((reg_val & PLL_PERI_CTRL_REG_PLL_P0_CLEAR_MASK) >> PLL_PERI_CTRL_REG_PLL_P0_OFFSET) + 1;
	factor_m = ((reg_val & PLL_PERI_CTRL_REG_PLL_INPUT_DIV2_CLEAR_MASK) >> PLL_PERI_CTRL_REG_PLL_INPUT_DIV2_OFFSET) + 1;
	pll_peri1x = (24 * factor_n / factor_m / factor_p0) / 2;

	return pll_peri1x;
}

uint clock_get_dram(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint clock = 0;
	uint clock_src = 0;
	uint factor_m, factor_p0;

	reg_val   = readl(&ccm->dram_clk_cfg);
	clock_src = (reg_val & DRAM_CLK_REG_DRAM_CLK_SEL_CLEAR_MASK) >> DRAM_CLK_REG_DRAM_CLK_SEL_OFFSET;

	switch (clock_src) {
	case 0:/*DDRPLL*/
		clock = clock_get_pll_ddr();
		break;
	case 1:/*AUDIO1PLL_DIV2*/
		clock = 0;
		break;
	case 2:/*PERIPLL2X*/
		clock = clock_get_pll_peri1x() * 2;
		break;
	case 3:/*PERIPLL_800M*/
		clock = 800;
		break;
	default:
		return 0;
	}

	factor_p0 = (1 << ((reg_val & DRAM_CLK_REG_DRAM_DIV2_CLEAR_MASK) >> DRAM_CLK_REG_DRAM_DIV2_OFFSET));
	factor_m = ((reg_val & DRAM_CLK_REG_DRAM_DIV1_CLEAR_MASK) >> DRAM_CLK_REG_DRAM_DIV1_OFFSET) + 1;

	clock = clock / factor_p0 /factor_m;

	return clock;
}

uint clock_get_corepll(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val;
	int 	factor_m;
	int 	clock, clock_src;

	reg_val   = readl(&ccm->riscv_clk_reg);
	clock_src = (reg_val & RISCV_CLK_REG_RISCV_CLK_SEL_CLEAR_MASK) >> RISCV_CLK_REG_RISCV_CLK_SEL_OFFSET;
	switch (clock_src) {
	case 0:/*OSC24M*/
		clock = 24;
		break;
	case 1:/*RTC32K*/
		clock = 0;
		break;
	case 2:/*RC16M*/
		clock = 16;
		break;
	case 3:/*PERIPLL_800M*/
		clock = 800;
		break;
	case 4:/*PERIPLL1X*/
		clock = clock_get_pll_peri1x();
		break;
	case 5:/*PLL_CPUX*/
		clock = clock_get_pll_cpu();
		break;
	case 6:/*AUDIO1PLL_DIV2*/
		clock = 0;
		break;
	default:
		return 0;
	}

	factor_m = ((reg_val & RISCV_CLK_REG_RISCV_DIV_CFG_CLEAR_MASK) >> RISCV_CLK_REG_RISCV_DIV_CFG_OFFSET) + 1;

	clock = clock / factor_m;

	return clock;
}

uint clock_get_ahb(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int factor_m = 0, factor_n = 0;
	int clock = 0;
	int src = 0, src_clock = 0;

	reg_val = readl(&ccm->psi_ahb1_ahb2_cfg);
	src = (reg_val & PSI_CLK_REG_CLK_SRC_SEL_CLEAR_MASK) >> PSI_CLK_REG_CLK_SRC_SEL_OFFSET;
	factor_m  = ((reg_val & PSI_CLK_REG_FACTOR_M_CLEAR_MASK) >> PSI_CLK_REG_FACTOR_M_OFFSET) + 1;
	factor_n  = (1 << ((reg_val & PSI_CLK_REG_FACTOR_N_CLEAR_MASK) >> PSI_CLK_REG_FACTOR_N_OFFSET));

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 0;
		break;
	case 2:	//RC16M
		src_clock = 16;
		break;
	case 3://PLL_PERI0(1X)
		src_clock = clock_get_pll_peri1x();
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;
}


uint clock_get_apb1(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int src = 0, src_clock = 0;
	int clock = 0, factor_m = 0, factor_n = 0;

	reg_val = readl(&ccm->apb1_cfg);
	factor_m  = ((reg_val & APB0_CLK_REG_FACTOR_M_CLEAR_MASK) >> APB0_CLK_REG_FACTOR_M_OFFSET) + 1;
	factor_n  = (1 << ((reg_val && APB0_CLK_REG_FACTOR_N_CLEAR_MASK) >> APB0_CLK_REG_FACTOR_N_OFFSET));
	src = (reg_val & APB0_CLK_REG_CLK_SRC_SEL_CLEAR_MASK) >> APB0_CLK_REG_CLK_SRC_SEL_OFFSET;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 0;
		break;
	case 2:	//PSI
		src_clock = clock_get_ahb();
		break;
	case 3://PLL_PERI0(1X)
		src_clock = clock_get_pll_peri1x();
		break;
	default:
		return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;
}

uint clock_get_apb2(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int clock = 0, factor_m = 0, factor_n = 0;
	int src = 0, src_clock = 0;

	reg_val = readl(&ccm->apb2_cfg);
	src = (reg_val & APB1_CLK_REG_CLK_SRC_SEL_CLEAR_MASK) >> APB1_CLK_REG_CLK_SRC_SEL_OFFSET;
	factor_m  = ((reg_val & APB1_CLK_REG_FACTOR_M_CLEAR_MASK) >> APB1_CLK_REG_FACTOR_M_OFFSET) + 1;
	factor_n  = (1 << ((reg_val & APB1_CLK_REG_FACTOR_N_CLEAR_MASK) >> APB1_CLK_REG_FACTOR_N_OFFSET));

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 0;
		break;
	case 2:	//PSI
		src_clock = clock_get_ahb();
		break;
	case 3:	//PLL_PERI0(1X)
		src_clock = clock_get_pll_peri1x();
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;

}


uint clock_get_mbus(void)
{
	unsigned int clock;

	clock = clock_get_dram();
	clock = clock / 4;

	return clock;
}

#if 0
static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int pll_clk)
{
	int index;

	index = pll_clk / 24;
	factor->FactorP = 0;
	factor->FactorN = (index - 1);
	factor->FactorM = 0;

	return 0;
}

static u32 get_efuse_data(u32 offset, u32 mask, u32 shift)
{
	u32 reg_val = 0x0;
	u32 val = 0x0;

	reg_val = readl((SID_EFUSE + offset));
	val = (reg_val >> shift) & mask;

	return val;
}

static int get_cpu_clock_type(void)
{
	u32 chip_type = 0;
	u32 bin_type = 0;
	u32 cpu_clock = -1;

	chip_type = get_efuse_data(0x0, 0xffff, 0x0);

	if (chip_type == 0x7400) {
		cpu_clock = 1008;
	} else if (chip_type == 0x5c00) {
		bin_type = get_efuse_data(0x28, 0xf, 12);
		if (bin_type & 0x8) { //force
			cpu_clock = 720;
		} else if ((bin_type & 0x3) == 0x2) { //slow
			cpu_clock = 720;
		} else if ((bin_type == 0x1) || (bin_type == 0x0)) { //normal
			cpu_clock = 1008;
		} else {
			printf("can't support bin type %d\n", bin_type);
			return -1;
		}
	} else {
		printf("can't support chip type %d\n", chip_type);
		return -1;
	}

	return cpu_clock;
}

int clock_set_corepll(int frequency)
{

	return  0;

	unsigned int reg_val = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	struct core_pll_freq_tbl  pll_factor;
	u32 cpu_clock;

	cpu_clock = get_cpu_clock_type();
	if (cpu_clock < 0) {
		return -1;
	} else {
		if (frequency == clock_get_corepll())
			return 0;
		else if (frequency >= cpu_clock)
			frequency = cpu_clock;
	}

	/* switch to 24M*/
	reg_val   = readl(&ccm->riscv_clk_reg);
	reg_val &= ~(0x07 << 24);
	writel(reg_val, &ccm->riscv_clk_reg);
	__udelay(20);

	/*pll disable*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val &= ~(0x01 << 31);
	writel(reg_val, &ccm->pll1_cfg);

	/*pll output disable*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val &= ~(0x01 << 27);
	writel(reg_val, &ccm->pll1_cfg);

	/*get config para form freq table*/
	clk_get_pll_para(&pll_factor, frequency);

	reg_val = readl(&ccm->pll1_cfg);
	reg_val &= ~((0xff << 8)  | (0x03 << 0));
	reg_val |= (pll_factor.FactorN << 8) | (pll_factor.FactorM << 0) ;
	writel(reg_val, &ccm->pll1_cfg);
	__udelay(20);

	/*enable lock*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val |=  (0x1 << 29);
	writel(reg_val, &ccm->pll1_cfg);

	/*enable pll*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val |=	(0x1 << 31);
	writel(reg_val, &ccm->pll1_cfg);
#ifndef FPGA_PLATFORM
	do {
		reg_val = readl(&ccm->pll1_cfg);
	} while (!(reg_val & (0x1 << 28)));
#endif

	/*enable pll output*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val |=  (0x1 << 27);
	writel(reg_val, &ccm->pll1_cfg);
	__udelay(20);

	/* switch clk src to COREPLL*/
	reg_val = readl(&ccm->riscv_clk_reg);
	reg_val &= ~(0x07 << 24);
	reg_val |=  (0x05 << 24);
	writel(reg_val, &ccm->riscv_clk_reg);

	return  0;
}
#endif
int usb_open_clock(void)
{
	u32 reg_val = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	//USB0 Clock Reg
	//bit31: Gating Special Clk for USB PHY0
	//Bit30: USB PHY0 Reset
	reg_val = readl(&ccm->usb0_clk_cfg);
	reg_val |= (USB0_CLK_REG_USB0_CLKEN_CLOCK_IS_ON << USB0_CLK_REG_USB0_CLKEN_OFFSET);
	writel(reg_val, &ccm->usb0_clk_cfg);
	//delay some time
	__msdelay(1);

	reg_val = readl(&ccm->usb0_clk_cfg);
	reg_val |= (USB0_CLK_REG_USBPHY0_RSTN_DE_ASSERT << USB0_CLK_REG_USBPHY0_RSTN_OFFSET);
	writel(reg_val, &ccm->usb0_clk_cfg);
	//delay some time
	__msdelay(1);

	//USB BUS Gating Reset Reg
	//bit24:USB_OTG Reset
	//bit8:USB_OTG Gating
	reg_val = readl(&ccm->usb_gate_reset);
	reg_val |= (USB_BGR_REG_USBOTG0_RST_DE_ASSERT << USB_BGR_REG_USBOTG0_RST_OFFSET);
	writel(reg_val, &ccm->usb_gate_reset);

	//delay to wati SIE stable
	__msdelay(1);

	reg_val = readl(&ccm->usb_gate_reset);
	reg_val |= (USB_BGR_REG_USBOTG0_GATING_PASS << USB_BGR_REG_USBOTG0_GATING_OFFSET);
	writel(reg_val, &ccm->usb_gate_reset);
	__msdelay(1);

	return 0;
}

int usb_close_clock(void)
{
	u32 reg_val = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	reg_val = readl(&ccm->usb_gate_reset);
	reg_val &= ~(USB_BGR_REG_USBOTG0_RST_CLEAR_MASK);
	writel(reg_val, &ccm->usb_gate_reset);
	__msdelay(1);

	reg_val = readl(&ccm->usb_gate_reset);
	reg_val &= ~(USB_BGR_REG_USBOTG0_GATING_CLEAR_MASK);
	writel(reg_val, &ccm->usb_gate_reset);
	__msdelay(1);

	return 0;
}

uint clock_get_pll6(void)
{
	return clock_get_pll_peri1x();
}
