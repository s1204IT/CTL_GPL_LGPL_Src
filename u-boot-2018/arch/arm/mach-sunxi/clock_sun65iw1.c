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
#include <asm/arch/usb.h>
#include "private_uboot.h"

#define SUNXI_IR_GATING_REG		(R_PRCM_REG_BASE + 0x1CC)
#define SUNXI_IR_RESET_REG		(R_PRCM_REG_BASE + 0x1CC)
#define SUNXI_IR_GATING_OFFSET		0
#define SUNXI_IR_RESET_OFFSET		16
#define R_IR_RX_CLK_REG			(R_PRCM_REG_BASE + 0x1C0)
#define R_IR_SCLK_GATING_OFFSET		31
#define R_IR_CLK_SRC_OFFSET		24
#define R_IR_CLK_RATIO_M_OFFSET		0
#define	SUNXI_PLL_DDR0_CTRL_REG	 0xA030000

static int ss_base_mode;

void clock_init_uart(void)
{
	return;
}

int clock_set_corepll(int frequency)
{
	return  0;
}

uint clock_get_pll6(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1, pll6;

	reg_val = readl(&ccm->pll6_cfg);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 16) & 0x03) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6 = (24 * factor_n /factor_p0/factor_m1)>>1;


	return pll6;
}

uint clock_get_ddrpll(void)
{
	uint reg_val, ddrpll;
	uint factor_n, factor_m1, factor_m0;
	/*fre: ddrpll = 24*N/M1/M0 */
	reg_val = readl(SUNXI_PLL_DDR0_CTRL_REG);

	factor_n  = ((reg_val >> 8) & 0xff) + 1;
	factor_m0 = ((reg_val >> 1) & 0x01) + 1;
	factor_m1 = ((reg_val >> 0) & 0x01) + 1;
	ddrpll    = 24 * factor_n / factor_m0 / factor_m1;

	return ddrpll;
}

uint clock_get_peri1_pll6_pll4(int div)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1, pll6;

	reg_val = readl(&ccm->pll_peri1_ctrl_reg);

	/*pll = 24*N/M1/P0*/
	factor_n  = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 16) & 0x07) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6      = (24 * factor_n / factor_p0 / factor_m1) / div;

	return pll6;
}
uint clock_get_corepll(void)
{
	struct sunxi_cpu_pll_reg *const ccm =
		(struct sunxi_cpu_pll_reg *)CCMU_PLL_CPU0_CTRL_REG;
	unsigned int clu0_sel_val, clu0_pll_val, clu0_div_val;
	int 	div_m, div_p, div_m1;
	int 	factor_n, factor_p;
	int 	clock, clock_src;

	clu0_sel_val = readl(&ccm->clu0_clk_reg);
	clock_src = (clu0_sel_val >> 24) & 0x03;

	clu0_div_val = readl(&ccm->clu0_div_cfg_reg);
	factor_p = 1 << ((clu0_div_val >> 16) & 0x3);

	switch (clock_src) {
	case 0://OSC24M
		clock = 24;
		break;
	case 1://RTC32K
		clock = 32/1000 ;
		break;
	case 2://RC16M
		clock = 16;
		break;
	case 3://PLL_CPU0
		clu0_pll_val = readl(&ccm->pll_cpu0_ctrl_reg);
		div_p    = ((clu0_pll_val>> 16) & 0xf) + 1;
		factor_n = ((clu0_pll_val>> 8) & 0xff);
		div_m    = ((clu0_pll_val>> 0) & 0xf) + 1;
		div_m1   = ((clu0_pll_val>> 20) & 0x3) + 1;

		clock = 24 * factor_n / div_p / (div_m * div_m1);
		break;
	case 4://PLL_PERI0_2X
		clock = 1200;
		break;
	case 5://PLL_PERI0_600M
		clock = 600;
		break;
	case 6://PLL_PERI1_2X
		clock = 1200;
		break;
	default:
		return 0;
	}
	return clock / factor_p;
}


uint clock_get_axi(void)
{
	struct sunxi_cpu_pll_reg *const ccm =
		(struct sunxi_cpu_pll_reg *)CCMU_PLL_CPU0_CTRL_REG;
	unsigned int reg_val = 0;
	int factor = 0;
	int clock = 0;

	reg_val   = readl(&ccm->pll_cpu1_ctrl_reg);
	factor    = ((reg_val >> 0) & 0x03) + 1;
	clock = clock_get_corepll()/factor;

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
	src = (reg_val >> 24)&0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//RC16M
		src_clock = 16;
		break;
	case 3://PLL_PERI0(1X)
		src_clock   = clock_get_pll6();
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
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;
	src = (reg_val >> 24)&0x3;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = 16;
		break;
	case 3://PLL_PERI0(1X)
		src_clock = clock_get_pll6();
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
	src = (reg_val >> 24)&0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = 16;
		break;
	case 3:	//PSI
		src_clock = clock_get_pll6();
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;

}


uint clock_get_mbus(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val;
	unsigned int src = 0, clock = 0, div = 0;
	reg_val = readl(&ccm->mbus_cfg);

	//get src
	src = (reg_val >> 24) & 0x3;
	//get div M, the divided clock is divided by M+1
	div = (reg_val & 0x3) + 1;

	switch (src) {
	case 0: //src is ddr
		clock = clock_get_ddrpll();
		break;
	case 1: //src is   pll_periph1(2x)/2
		clock = clock_get_peri1_pll6_pll4(2);
		break;
	case 2: //src is periph1_480
		clock = 480;
		break;
	case 3: //src is periph1_400
		clock = clock_get_peri1_pll6_pll4(3);
		break;
	}

	clock = clock / div;

	return clock;
}



int usb_open_clock(void)
{
	unsigned int reg_val;

	/* HSI_SYS Gating And Reset */
	setbits_le32(SUNXI_CCM_BASE + HSI_SYS_GAR_REG,
		     BIT(HSI_SYS_GAR_REG_HSI_SYS_RST_N_OFFSET));
	setbits_le32(SUNXI_CCM_BASE + HSI_SYS_GAR_REG,
		     BIT(HSI_SYS_GAR_REG_HSI_AHB_CLK_EN_OFFSET));

	/* USB2P0_SYS Gating And Reset */
	setbits_le32(SUNXI_CCM_BASE + USB2P0_SYS_GAR_REG,
		     BIT(USB2P0_SYS_GAR_REG_USB2P0_SYS_RST_N_OFFSET));
	setbits_le32(SUNXI_CCM_BASE + USB2P0_SYS_GAR_REG,
		     BIT(USB2P0_SYS_GAR_REG_USB2P0_SYS_AHB_CLK_EN_OFFSET));

	/* USB0 Gating And Reset */
	setbits_le32(SUNXI_CCM_BASE + USB0_GAR_REG,
		     BIT(USB0_GAR_REG_USB0_DEV_AHB_CLK_EN_OFFSET));
	clrbits_le32(SUNXI_CCM_BASE + USB0_GAR_REG,
		     BIT(USB0_GAR_REG_USB0_DEV_RST_N_OFFSET));
	setbits_le32(SUNXI_CCM_BASE + USB0_GAR_REG,
		     BIT(USB0_GAR_REG_USB0_DEV_RST_N_OFFSET));

	reg_val = readl(SUNXI_USBOTG_BASE + USBC_REG_o_SEL);
	reg_val |= (0x01 << 0);
	writel(reg_val, (SUNXI_USBOTG_BASE + USBC_REG_o_SEL));

	/* enable usb0 phy */
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBPHY_BASE +
							USBC_REG_o_PHYCTRL));
	reg_val &= ~(0x01 << USBC_PHY_CTRL_SIDDQ);
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBPHY_BASE +
						  USBC_REG_o_PHYCTRL));

	/* deassert phy reset */
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBPHY_BASE +
							USBC_REG_o_RSTCTRL));
	reg_val |= (0x01 << USBC_RST_CTRL_PHYRST);
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBPHY_BASE +
						  USBC_REG_o_RSTCTRL));

	return 0;
}

int usb_close_clock(void)
{
	unsigned int reg_val;

	/* disable usb0 phy */
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBPHY_BASE +
							USBC_REG_o_PHYCTRL));
	reg_val |= (0x01 << USBC_PHY_CTRL_SIDDQ);
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBPHY_BASE +
						  USBC_REG_o_PHYCTRL));

	/* assert phy reset */
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBPHY_BASE +
							USBC_REG_o_RSTCTRL));
	reg_val &= ~(0x01 << USBC_RST_CTRL_PHYRST);
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBPHY_BASE +
						  USBC_REG_o_RSTCTRL));

	clrbits_le32(SUNXI_CCM_BASE + USB0_GAR_REG,
		     BIT(USB0_GAR_REG_USB0_DEV_AHB_CLK_EN_OFFSET) |
			     BIT(USB0_GAR_REG_USB0_DEV_RST_N_OFFSET));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + USB2P0_SYS_GAR_REG,
		     BIT(USB2P0_SYS_GAR_REG_USB2P0_SYS_AHB_CLK_EN_OFFSET) |
			     BIT(USB2P0_SYS_GAR_REG_USB2P0_SYS_RST_N_OFFSET));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + HSI_SYS_GAR_REG,
		     BIT(HSI_SYS_GAR_REG_HSI_AHB_CLK_EN_OFFSET) |
			     BIT(HSI_SYS_GAR_REG_HSI_SYS_RST_N_OFFSET));
	__msdelay(1);

	return 0;
}


int sunxi_set_sramc_mode(void)
{
	u32 reg_val;

	/* SRAM Area C 128K Bytes Configuration by AHB ,default map to VE*/
	reg_val = readl(SUNXI_SYSCTRL_BASE);
	reg_val &= ~(0xFFFFFFFF);
	writel(reg_val, SUNXI_SYSCTRL_BASE);

	/* VE SRAM:set sram to normal mode, default boot mode */
	reg_val = readl(SUNXI_SYSCTRL_BASE + 0X0004);
	reg_val &= ~(0x1 << 24);
	writel(reg_val, SUNXI_SYSCTRL_BASE + 0X0004);

	return 0;
}

int sunxi_get_active_boot0_id(void)
{
	uint32_t val = *(uint32_t *)(SUNXI_RTC_BASE + 0x304);
	if (val & (1 << 15)) {
		return (val >> 8) & 0x7;
	} else {
		return (val >> 24) & 0x7;
	}
}

void clock_open_timer(int timernum)
{
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + 0x730 + timernum * 4);
	reg_value |= (0 <<24);
	reg_value |= (2 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + 0x730 + timernum * 4));

	/*enable timer*/
	reg_value = readl(SUNXI_CCM_BASE + 0x730 + timernum * 4);
	reg_value |= (1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0x730 + timernum * 4));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (1 << 16);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (1 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));
	__msdelay(1);

}

void clock_close_timer(int timernum)
{
	u32 reg_value = 0;

	/*disable timer*/
	reg_value = readl(SUNXI_CCM_BASE + 0x730 + timernum * 4);
	reg_value |= (0 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0x730 + timernum * 4));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (0 << 16);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (0 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));
	__msdelay(1);
}

void clock_set_gic(void)
{
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + GIC_CLK_REG);
	reg_value |= ((1 << 31) | (2 << 24) | (0 << 0));
	writel(reg_value, SUNXI_CCM_BASE + GIC_CLK_REG);
	udelay(20);
}

int ir_clk_cfg(void)
{
	/* reset */
	clrbits_le32(SUNXI_IR_RESET_REG, 1 << SUNXI_IR_RESET_OFFSET);

	__msdelay(1);

	setbits_le32(SUNXI_IR_RESET_REG, 1 << SUNXI_IR_RESET_OFFSET);

	/* gating */
	clrbits_le32(SUNXI_IR_GATING_REG, 1 << SUNXI_IR_GATING_OFFSET);

	__msdelay(1);

	setbits_le32(SUNXI_IR_GATING_REG, 1 << SUNXI_IR_GATING_OFFSET);

	/* config Special Clock for IR   (24/1/(0+1))=24MHz) */
	/* Select 24MHz */
	clrbits_le32(R_IR_RX_CLK_REG, 0x3 << R_IR_CLK_SRC_OFFSET);
	setbits_le32(R_IR_RX_CLK_REG, 1 << R_IR_CLK_SRC_OFFSET);

	__msdelay(1);

	/* open Clock */
	setbits_le32(R_IR_RX_CLK_REG, 1 << R_IR_SCLK_GATING_OFFSET);

	__msdelay(2);
	return 0;
}

void ss_clk_init(void)
{
	u32 reg_val;
	/* enable mbus master ce gating clk */
	reg_val = readl(SUNXI_CE_MBUS_MAT_CLK_REG);
	reg_val |= (SUNXI_CE_GATING_ON << SUNXI_CE_MBUS_MAT_CLK_GATE_OFFSET);
	writel(reg_val, SUNXI_CE_MBUS_MAT_CLK_REG);

	/* eable mbus ce gating clk */
	reg_val = readl(SUNXI_CE_MBUS_CLK_REG);
	reg_val |= (SUNXI_CE_GATING_ON << SUNXI_CE_MBUS_CLK_GATE_OFFSET);
	writel(reg_val, SUNXI_CE_MBUS_CLK_REG);

	/* assert ce_sys clk */
	reg_val = readl(SUNXI_CE_SYS_GATING_RESET_REG);
	reg_val &= !(0x1 << SUNXI_CE_SYS_GATING_RESET_OFFSET);
	writel(reg_val, SUNXI_CE_SYS_GATING_RESET_REG);
	/* de-assert ce_sys clk */
	reg_val = readl(SUNXI_CE_SYS_GATING_RESET_REG);
	reg_val |= (0x1 << SUNXI_CE_SYS_GATING_RESET_OFFSET);
	writel(reg_val, SUNXI_CE_SYS_GATING_RESET_REG);
	/* enable ce sys IP AHB clk gating */
	reg_val = readl(SUNXI_CE_SYS_GATING_RESET_REG);
	reg_val |= (SUNXI_CE_GATING_ON << SUNXI_CE_GATING_OFFSET);
	writel(reg_val, SUNXI_CE_SYS_GATING_RESET_REG);

	/* set clk sys src 600M, ce sys factor_m=0, so ce sys clk is 600M */
	reg_val = readl(SUNXI_CE_SYS_CLK_REG);
	reg_val |= (SUNXI_CE_SRC_600M << SUNXI_CE_SYS_CLK_SRC_SEL_OFFSET);
	reg_val |= (SUNXI_CE_FACTOR_0 << SUNXI_CE_SYS_CLK_FACTOR_M_OFFSET);
	writel(reg_val, SUNXI_CE_SYS_CLK_REG);
	/* enable ce_sys gating */
	reg_val = readl(SUNXI_CE_SYS_CLK_REG);
	reg_val |= (SUNXI_CE_GATING_ON << SUNXI_CE_SYS_GATING_OFFSET);
	writel(reg_val, SUNXI_CE_SYS_CLK_REG);

	// printf("%d reg=0x%x, val=0x%x\n", __LINE__, SUNXI_CE_MBUS_MAT_CLK_REG, readl(SUNXI_CE_MBUS_MAT_CLK_REG));
	// printf("%d reg=0x%x, val=0x%x\n", __LINE__, SUNXI_CE_MBUS_CLK_REG, readl(SUNXI_CE_MBUS_CLK_REG));
	// printf("%d reg=0x%x, val=0x%x\n", __LINE__, SUNXI_CE_SYS_GATING_RESET_REG, readl(SUNXI_CE_SYS_GATING_RESET_REG));
	// printf("%d reg=0x%x, val=0x%x\n", __LINE__, SUNXI_CE_SYS_CLK_REG, readl(SUNXI_CE_SYS_CLK_REG));

	// printf("%s() %d return ok\n", __func__, __LINE__);
}

void ss_open(void)
{
	static int initd;

	/* get soc secure status */
#ifdef CONFIG_SUNXI_SECURE_CRYPTO
	if (sid_get_security_status()) {
		ss_base_mode = 0;
	} else {
		ss_base_mode = 1;
	}
#else
	ss_base_mode = 0;
#endif
	// printf("ss_base_mode=%d, \n", ss_base_mode);

	if (initd)
		return;
	initd = 1;

	ss_clk_init();
}
