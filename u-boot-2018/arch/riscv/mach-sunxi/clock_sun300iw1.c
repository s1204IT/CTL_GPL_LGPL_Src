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
	u32 reg_val = readl(IOMEM_ADDR(CCMU_FUNC_CFG_REG));
	void aw_set_hosc_freq(u32 hosc_freq);
	if (reg_val & PLL_FUNC_CFG_REG_DCXO_ST_CLEAR_MASK)
		aw_set_hosc_freq(24);
	else
		aw_set_hosc_freq(40);

	int port = uboot_spare_head.boot_data.uart_port;
	volatile void __iomem *ccmu_reset0_base = IOMEM_ADDR(CCMU_BUS_Reset0_REG);
	volatile void __iomem *ccmu_gating0_base = IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG);
	u32 i, reg;
	/* reset */
	reg = readl(ccmu_reset0_base);
	reg &= ~(1 << (BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET + port));
	reg |= (1 << (BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET + port));
	writel(reg, ccmu_reset0_base);

	/* gate */
	reg = readl(ccmu_gating0_base);
	reg &= ~(1<< (BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET + port));
	writel(reg, ccmu_gating0_base);
	for (i = 0; i < 100; i++)
		;
	reg |= (1 << (BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET + port));
	writel(reg, ccmu_gating0_base);
}

uint clock_get_pll_peri(void)
{

	u32 reg_val		       = 0;
	volatile void __iomem *clk_reg = IOMEM_ADDR(CCMU_PLL_PERI_CTRL0_REG);
	int factor_n, factor_m;
	int clock;

	reg_val = readl(clk_reg);

	factor_n = (reg_val & PLL_PERI_CTRL0_REG_PLL_N_CLEAR_MASK) >> PLL_PERI_CTRL0_REG_PLL_N_OFFSET;
	factor_m = reg_val & PLL_PERI_CTRL0_REG_PLL_INPUT_DIV_CLEAR_MASK;
	clock = (aw_get_hosc_freq() *2 * (factor_n + 1)) / (factor_m + 1);

	return clock;
}

#ifdef CONFIG_CPU_E90X
uint clock_get_corepll(void)
{
	u32 reg_val		       = 0;
	volatile void __iomem *clk_reg = IOMEM_ADDR(CCMU_PLL_T_HEAD__CTRL_REG);
	int factor_n, factor_m;
	int clock, clock_src;
	u32 vd_reg, pll_cpu_reg;

	reg_val	  = readl(clk_reg);
	clock_src = ((reg_val & T_HEAD_CLK_REG_T_HEAD_CLK_SEL_CLEAR_MASK) >>
		     T_HEAD_CLK_REG_T_HEAD_CLK_SEL_OFFSET);
	switch (clock_src) {
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_HOSC:
		clock = aw_get_hosc_freq();
		break;
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_VIDEOPLL2X:
		vd_reg = readl(IOMEM_ADDR(CCMU_PLL_VIDEO_CTRL_REG));
		factor_n   = (vd_reg & PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK) >>
			   PLL_VIDEO_CTRL_REG_PLL_N_OFFSET;
		factor_m = (vd_reg &
			    PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK) >>
			   PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_OFFSET;
		clock = (aw_get_hosc_freq() * 2 * (factor_n + 1)) /
			(factor_m + 1);
		break;
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_RC1M:
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_RC1M0:
		clock = 1;
		break;
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_CPU_PLL: /*PLL_CPUX*/
		pll_cpu_reg = readl(IOMEM_ADDR(CCMU_PLL_CPUX_CTRL_REG));
		factor_n = (pll_cpu_reg & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >>
			   PLL_CPU_CTRL_REG_PLL_N_OFFSET;
		clock = aw_get_hosc_freq() * (factor_n + 1);
		break;
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_PERI_PLL_1024M:
		clock = 1024;
		break;
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_PERI_PLL_614M:
	case T_HEAD_CLK_REG_T_HEAD_CLK_SEL_PERI_PLL_614M0:
		clock = 614;
		break;
	default:
		return 0;
	}
	factor_m = reg_val & T_HEAD_CLK_REG_T_HEAD_CLK_DIV_CLEAR_MASK;
	return clock / (factor_m + 1);
}
#else
uint clock_get_corepll(void)
{
	u32 reg_val		       = 0;
	volatile void __iomem *clk_reg = IOMEM_ADDR(CCMU_ANSC_CLK_REG);
	int factor_n, factor_m;
	int clock, clock_src;
	u32 vd_reg, pll_cpu_reg;

	reg_val	  = readl(clk_reg);
	clock_src = ((reg_val & ANSC_CLK_REG_ANSC_CLK_SEL_CLEAR_MASK) >>
		     ANSC_CLK_REG_ANSC_CLK_SEL_OFFSET);
	// printf("reg_val:0x%x->0x%x\n", reg_val, clock_src);
	switch (clock_src) {
	case ANSC_CLK_REG_ANSC_CLK_SEL_HOSC:
		clock = aw_get_hosc_freq();
		break;
	case ANSC_CLK_REG_ANSC_CLK_SEL_VIDEOPLL2X:
		vd_reg = readl(IOMEM_ADDR(CCMU_PLL_VIDEO_CTRL_REG));
		factor_n   = (vd_reg & PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK) >>
			   PLL_VIDEO_CTRL_REG_PLL_N_OFFSET;
		factor_m = (vd_reg &
			    PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK) >>
			   PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_OFFSET;
		clock = (aw_get_hosc_freq() * 2 * (factor_n + 1)) /
			(factor_m + 1);
		break;
	case ANSC_CLK_REG_ANSC_CLK_SEL_RC1M:
	case ANSC_CLK_REG_ANSC_CLK_SEL_RC1M0:
		clock = 1;
		break;
	case ANSC_CLK_REG_ANSC_CLK_SEL_CPU_PLL: /*PLL_CPUX*/
		pll_cpu_reg = readl(IOMEM_ADDR(CCMU_PLL_CPUX_CTRL_REG));
		factor_n = (pll_cpu_reg & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >>
			   PLL_CPU_CTRL_REG_PLL_N_OFFSET;
		factor_m = (pll_cpu_reg & PLL_CPU_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK) >>
			   PLL_CPU_CTRL_REG_PLL_INPUT_DIV_OFFSET;
		clock = aw_get_hosc_freq() * (factor_n + 1) /
			(factor_m > 1 ? 4 : factor_m + 1);
		break;
	case ANSC_CLK_REG_ANSC_CLK_SEL_PERI_PLL_1024M:
		clock = 1024;
		break;
	case ANSC_CLK_REG_ANSC_CLK_SEL_PERI_PLL_768M:
	case ANSC_CLK_REG_ANSC_CLK_SEL_PERI_PLL_768M0:
		clock = 768;
		break;
	default:
		return 0;
	}
	factor_m = reg_val & ANSC_CLK_REG_ANSC_CLK_DIV_CLEAR_MASK;

	return clock / (factor_m + 1);
}
#endif

uint clock_get_ahb(void)
{
	u32 reg_val		       = 0;
	volatile void __iomem *clk_reg = IOMEM_ADDR(CCMU_AHB_CLK_REG);
	int factor_m;
	int clock, clock_src;

	reg_val	  = readl(clk_reg);
	clock_src = ((reg_val & AHB_CLK_REG_AHB_SEL_CLEAR_MASK) >>
		     AHB_CLK_REG_AHB_SEL_OFFSET);
	// printf("reg_val:0x%x->0x%x\n", reg_val, clock_src);
	switch (clock_src) {
	case AHB_CLK_REG_AHB_SEL_HOSC:
		clock = aw_get_hosc_freq();
		break;
	case AHB_CLK_REG_AHB_SEL_PERI_768M:
		clock = 768;
		break;
	case AHB_CLK_REG_AHB_SEL_RC1M:
		clock = 1;
		break;
	default:
		return 0;
	}
	factor_m = reg_val & AHB_CLK_REG_AHB_CLK_DIV_CLEAR_MASK;
	return clock / (factor_m + 1);
}

uint clock_get_apb(void)
{
	u32 reg_val		       = 0;
	volatile void __iomem *clk_reg = IOMEM_ADDR(CCMU_APB_CLK_REG);
	int factor_m;
	int clock, clock_src;

	reg_val	  = readl(clk_reg);
	clock_src = ((reg_val & APB_CLK_REG_APB_SEL_CLEAR_MASK) >>
		     APB_CLK_REG_APB_SEL_OFFSET);
	// printf("reg_val:0x%x->0x%x\n", reg_val, clock_src);
	switch (clock_src) {
	case APB_CLK_REG_APB_SEL_HOSC:
		clock = aw_get_hosc_freq();
		break;
	case APB_CLK_REG_APB_SEL_PERI_384M:
		clock = 384;
		break;
	case APB_CLK_REG_APB_SEL_RC1M:
		clock = 1;
		break;
	default:
		return 0;
	}
	factor_m = reg_val & APB_CLK_REG_APB_CLK_DIV_CLEAR_MASK;
	return clock / (factor_m + 1);
}


int clock_set_corepll(int frequency)
{
	return 0;
}

void clock_open_timer(int timernum)
{
	u32 reg_value = 0;

	reg_value = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_value |= (BUS_Reset0_REG_PRESETN_TIMER_SW_DE_ASSERT
		      << (BUS_Reset0_REG_PRESETN_TIMER_SW_OFFSET));
	writel(reg_value, IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	__msdelay(1);
	reg_value = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_value |= (BUS_Reset0_REG_HRESETN_HSTIMER_SW_DE_ASSERT
		      << (BUS_Reset0_REG_HRESETN_HSTIMER_SW_OFFSET));
	writel(reg_value, IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	__msdelay(1);
	reg_value = readl(IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));
	reg_value |= (BUS_CLK_GATING0_REG_TIMER_PCLK_EN_CLOCK_IS_ON
		      << (BUS_CLK_GATING0_REG_TIMER_PCLK_EN_OFFSET));
	writel(reg_value, IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));
	__msdelay(1);
	reg_value = readl(IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));
	reg_value |= (BUS_CLK_GATING0_REG_HSTIMER_HCLKEN_CLOCK_IS_ON
		      << (BUS_CLK_GATING0_REG_HSTIMER_HCLKEN_OFFSET));
	writel(reg_value, IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));
	__msdelay(1);
}

void clock_close_timer(int timernum)
{
	u32 reg_value = 0;
	reg_value     = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_value &= ~(BUS_Reset0_REG_PRESETN_TIMER_SW_DE_ASSERT
		       << (BUS_Reset0_REG_PRESETN_TIMER_SW_OFFSET));
	writel(reg_value, IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	__msdelay(1);
	reg_value     = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_value &= ~(BUS_Reset0_REG_HRESETN_HSTIMER_SW_DE_ASSERT
		       << (BUS_Reset0_REG_HRESETN_HSTIMER_SW_OFFSET));
	writel(reg_value, IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	__msdelay(1);
}

int usb_open_clock(void)
{
	u32 reg_value			 = 0;
	volatile void __iomem *ccmu_base = IOMEM_ADDR(SUNXI_CCM_APP_BASE);

	debug("[%s %d] 0x007c: 0x%08x, 0x0080: 0x%08x, 0x0084: 0x%08x, 0x0090: 0x%08x\n",
	      __func__, __LINE__, readl(ccmu_base + 0x7c),
	      readl(ccmu_base + 0x80), readl(ccmu_base + 0x84),
	      readl(ccmu_base + 0x90));

	//disable usb phy reset
	reg_value = readl(ccmu_base + 0x90);
	reg_value &= ~(1 << 23);
	writel(reg_value, (ccmu_base + 0x90));

	//disable otg clk gating
	reg_value = readl(ccmu_base + 0x80);
	reg_value &= ~(1 << 20);
	writel(reg_value, (ccmu_base + 0x80));

	//reset
	reg_value = readl(ccmu_base + 0x90);
	reg_value &= ~(1 << 20);
	writel(reg_value, (ccmu_base + 0x90));

	reg_value = readl(ccmu_base + 0x90);
	reg_value &= ~(1 << 19);
	writel(reg_value, (ccmu_base + 0x90));

	reg_value = readl(ccmu_base + 0x80);
	reg_value &= ~(1 << 19);
	writel(reg_value, (ccmu_base + 0x80));

	debug("[%s %d] 0x007c: 0x%08x, 0x0080: 0x%08x, 0x0084: 0x%08x, 0x0090: 0x%08x\n",
	      __func__, __LINE__, readl(ccmu_base + 0x7c),
	      readl(ccmu_base + 0x80), readl(ccmu_base + 0x84),
	      readl(ccmu_base + 0x90));

	// USB0: the 24M clock is default enable
	reg_value = readl(ccmu_base + 0x7c);
	reg_value |= (1 << 3);
	writel(reg_value, (ccmu_base + 0x7c));
	//delay some time
	__msdelay(1);
	// USB0: open the usb_hclk
	reg_value = readl(ccmu_base + 0x80);
	reg_value |= (1 << 19);
	writel(reg_value, (ccmu_base + 0x80));

	// USB0: open the usbotg_busclk
	reg_value = readl(ccmu_base + 0x80);
	reg_value |= (1 << 20);
	writel(reg_value, (ccmu_base + 0x80));
	__msdelay(1);

	// USB0: open the usb mbus ahb_gate
	reg_value = readl(ccmu_base + 0x84);
	reg_value |= (1 << 14);
	writel(reg_value, (ccmu_base + 0x84));
	//delay some time
	__msdelay(1);

	// USB0: de-assert the reg
	reg_value = readl(ccmu_base + 0x90);
	reg_value |= (1 << 19);
	writel(reg_value, (ccmu_base + 0x90));
	//delay some time
	__msdelay(1);

	// USB0: de-assert the reg
	reg_value = readl(ccmu_base + 0x90);
	reg_value |= (1 << 20);
	writel(reg_value, (ccmu_base + 0x90));
	//delay some time
	__msdelay(1);

	// USB0: de-assert the reg
	reg_value = readl(ccmu_base + 0x90);
	reg_value |= (1 << 23);
	writel(reg_value, (ccmu_base + 0x90));
	//delay some time
	__msdelay(1);
	debug("[%s %d] 0x007c: 0x%08x, 0x0080: 0x%08x, 0x0084: 0x%08x, 0x0090: 0x%08x\n",
	      __func__, __LINE__, readl(ccmu_base + 0x7c),
	      readl(ccmu_base + 0x80), readl(ccmu_base + 0x84),
	      readl(ccmu_base + 0x90));
	return 0;
}

int usb_close_clock(void)
{
	u32 reg_value			 = 0;
	volatile void __iomem *ccmu_base = IOMEM_ADDR(SUNXI_CCM_APP_BASE);

	// USB0: assert the reg
	reg_value = readl(ccmu_base + 0x90);
	reg_value &= ~(1 << 23);
	writel(reg_value, (ccmu_base + 0x90));
	//delay some time
	__msdelay(1);

	// USB0: assert the reg
	reg_value = readl(ccmu_base + 0x90);
	reg_value &= ~(1 << 20);
	writel(reg_value, (ccmu_base + 0x90));
	//delay some time
	__msdelay(1);

	// USB0: assert the reg
	reg_value = readl(ccmu_base + 0x90);
	reg_value &= ~(1 << 19);
	writel(reg_value, (ccmu_base + 0x90));
	//delay some time
	__msdelay(1);

	return 0;
}

void aw_board_clk_print(void)
{
	tick_printf("CPU=%d MHz,PERI=%d Mhz,AHB=%d Mhz, APB=%dMhz\n",
		clock_get_corepll(),
		clock_get_pll_peri(), clock_get_ahb(),
		clock_get_apb());
	return;
}

void sunxi_e907_clock_reset(void)
{
	u32 reg_val;

	/*riscv_cfg Gating Clock OFF */
	reg_val = readl(IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));
	reg_val &= ~(CCMU_E907_CFG_CLK_GATING);
	writel(reg_val, IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));

	/*assert e907 rv_cfg_reset*/
	reg_val = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_val &= ~(CCMU_E907_CFG_RST);
	writel(reg_val, IOMEM_ADDR(CCMU_BUS_Reset0_REG));

	/*assert e907 rv_sys_apb_reset*/
	reg_val = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_val &= ~(CCMU_E907_SYS_APB_RST);
	writel(reg_val, IOMEM_ADDR(CCMU_BUS_Reset0_REG));

	/*assert e907_rstn*/
	reg_val = 0xA5690000;
	writel(reg_val, IOMEM_ADDR(CCMU_E907_RSTN_REG));
}

void sunxi_e907_clock_init(u32 addr)
{
	u32 reg_val;

	/*DE-assert e907 rv_cfg_reset*/
	reg_val = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_val |= CCMU_E907_CFG_RST;
	writel(reg_val, IOMEM_ADDR(CCMU_BUS_Reset0_REG));

	/*DE-assert e907 rv_sys_apb_reset*/
	reg_val = readl(IOMEM_ADDR(CCMU_BUS_Reset0_REG));
	reg_val |= CCMU_E907_SYS_APB_RST;
	writel(reg_val, IOMEM_ADDR(CCMU_BUS_Reset0_REG));

	/*riscv_cfg Gating Clock ON */
	reg_val = readl(IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));
	reg_val |= CCMU_E907_CFG_CLK_GATING;
	writel(reg_val, IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG));

	/*set e907 start addr */
	reg_val = addr;
	writel(reg_val, IOMEM_ADDR(E907_STA_ADD_REG));

	/*DE-assert e907_rstn_sw */
	reg_val = 0xA5690001;
	writel(reg_val, IOMEM_ADDR(CCMU_E907_RSTN_REG));
}
