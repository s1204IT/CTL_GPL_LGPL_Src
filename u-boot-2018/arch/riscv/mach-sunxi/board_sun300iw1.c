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
#include <asm/arch/usb.h>
#include "../../../drivers/sunxi_usb/usb_base.h"

DECLARE_GLOBAL_DATA_PTR;

void otg_phy_config(void)
{
	uint reg_val = 0;
	reg_val	     = readl(IOMEM_ADDR(SUNXI_USBOTG_BASE) + USBC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	// reg_val |= 0x01<<USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, IOMEM_ADDR(SUNXI_USBOTG_BASE) + USBC_REG_o_PHYCTL);
	reg_val = readl(IOMEM_ADDR(SUNXI_USBOTG_BASE) + USBC_REG_o_PHYCTL);

	writel(0x01, IOMEM_ADDR(SUNXI_USBOTG_BASE) + USBC_REG_o_DMA_WADDR);

	/* For 24Mhz crystal oscillator, you need to modify the phy pll
	 * configuration, please refer to the spec.
	 */
	if (24 == aw_get_hosc_freq())
		usbc_new_phyx_tp_write(SUNXI_USBOTG_BASE, 0xb, 20, 0x8);
}

#if defined(CONFIG_SUNXI_WDT_V2)
#define AW_RESET_CTRL (SUNXI_PRCM_BASE + 0x1c)
void aw_wdt_reset_cpu(ulong addr)
{
	static const struct sunxi_wdog *wdog =
		(struct sunxi_wdog *)SUNXI_WDT_BASE;
	writel(readl(IOMEM_ADDR(SUNXI_PRCM_BASE))|(1 << 3), IOMEM_ADDR(AW_RESET_CTRL));
	writel(((WDT_CFG_KEY << 16) | WDT_MODE_EN), IOMEM_ADDR(&wdog->srst));
	while (1) { }
}
#endif

#include <asm/arch/plic.h>
#include <sunxi_board.h>
#include <sbi.h>

#ifdef CONFIG_CPU_A27L2
extern u64 notrace arch_read_timer(void);

__weak void sbi_set_arch_timer(uint64_t stime_value)
{
	return;
}

static void rv_arch_timer_reload(void)
{
	u64 value;
	u64 expires = 1;//1 ms

	value = arch_read_timer() + expires * ((gd->hosc_freq * 1000));

	sbi_set_arch_timer(value);

	csr_set(sie, 0x1 << 5);//enable riscv timer0 irq
	//pr_err("sie:0x%lx\n", csr_read(sie));

	return;
}

static void irq_remedy_handle_callback(void *data)
{
	int reg_n, hwirq;
	u32 val, pending_bits, tmp1, tmp2;

	csr_clear(sie, 0x1 << 5);// disable riscv timer0 irq

	for (reg_n = 0; reg_n < 6; reg_n++) {
		//val = readl(PLIC_SIE_REG(reg_n)); // enable bits
		val = readl(PLIC_M_S_IE_REG(reg_n)); // enable bits
		//pr_debug("enable reg[%p] = %x\n", PLIC_M_S_IE_REG(reg_n), val);

		pending_bits = readl(PLIC_IP_REG(reg_n)); // pending bits
		//pr_debug("pending reg[%p] = %x\n", PLIC_IP_REG(reg_n), pending_bits);

		tmp1 = val | pending_bits;
		tmp2 = val & (~pending_bits);
		val = tmp1 & tmp2; // plic enable bit is 1, and pending bit is 0
		//pr_debug("plic chk val %x\n", val);

		hwirq = reg_n * 32;
		while (val) {
			if (val & 0x1) {
				writel(hwirq, PLIC_M_S_CLAIM_REG);
				//pr_debug("clean hwirq: %d\n", hwirq);
			}
			val >>= 1;
			++hwirq;
		}
	}

	rv_arch_timer_reload();
	return;
}

static void arch_timer_irq_remedy_init(void)
{
	rv_arch_timer_reload();
	void riscv_irq_install_handler (int irq, interrupt_handler_t handle_irq, void *data);
	riscv_irq_install_handler(IRQ_S_TIMER, irq_remedy_handle_callback, NULL);

	return;
}


#if 1
int do_arch_timer_irq(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	arch_timer_irq_remedy_init();
	return 0;
}

U_BOOT_CMD(
	timer_irq, 1, 0, do_arch_timer_irq,
	"do a arch timer irq test", ""
);
#endif
#endif// CONFIG_CPU_A27L2

int sunxi_plic_init(void)
{
	writel(0x0, PLIC_M_S_TH_REG);
#ifdef CONFIG_CPU_A27L2
	arch_timer_irq_remedy_init();
#endif
	return 0;
}
