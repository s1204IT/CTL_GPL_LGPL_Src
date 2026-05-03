// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <asm/io.h>
#include <asm/arch/aw_clic.h>
#include <sunxi_board.h>
DECLARE_GLOBAL_DATA_PTR;
struct _irq_handler {
	void *m_data;
	void (*m_func)(void *data);
};
struct _irq_handler sunxi_int_handlers[AW_IRQ_CLIC_NUM];
extern int interrupts_is_open(void);
#define PLAT_IRQ_CONTROLLER_CNT 1
#define ROOT_IRQ_CONTROLLER_ID 0
#define PLAT_ROOT_IC_REG_BASE_ADDR PLAT_CLIC_BASE_ADDR
#define PLAT_CLIC_IRQ_CNT (AW_IRQ_CLIC_NUM)
#define PLAT_ROOT_IC_IRQ_CNT PLAT_CLIC_IRQ_CNT
static irq_controller_t g_plat_ic_arr[PLAT_IRQ_CONTROLLER_CNT];
__attribute__((weak)) int sunxi_plat_irq_init(void)
{
	uint32_t i;
	irq_controller_t *ic;
	for (i = 0; i < PLAT_IRQ_CONTROLLER_CNT; i++) {
		ic     = &g_plat_ic_arr[i];
		ic->id = i;
	}
	ic		  = &g_plat_ic_arr[ROOT_IRQ_CONTROLLER_ID];
	ic->reg_base_addr = PLAT_ROOT_IC_REG_BASE_ADDR;
	ic->irq_cnt	  = 208;
	ic->parent_id	  = 0;
	ic->irq_id	  = 0;
	// ic->ops = &g_rv_clic_ops;
	return 0;
}
static inline void clic_set_irq_ctrl_bit(uint32_t reg_addr, uint8_t mask,
					 int is_set)
{
	uint8_t reg_data = readb((void *)reg_addr);
	if (is_set)
		reg_data |= mask;
	else
		reg_data &= ~mask;
	writeb(reg_data, (void *)reg_addr);
}
static inline void clic_set_enable(uint32_t reg_addr, int enabled)
{
	clic_set_irq_ctrl_bit(reg_addr, IE_BIT_MASK, enabled);
}
static inline int clic_set_pending(uint32_t reg_addr, int pending)
{
	clic_set_irq_ctrl_bit(reg_addr, IP_BIT_MASK, pending);
	return 0;
}
static inline void clic_set_vec_mode(uint32_t reg_addr, int vec_mode)
{
	clic_set_irq_ctrl_bit(reg_addr, HW_VECTOR_IRQ_BIT_MASK, vec_mode);
}
static inline void clic_set_trigger_type(uint32_t reg_addr,
					 irq_trigger_type_t type)
{
	uint8_t reg_data, field_value;
	if (type == IRQ_TRIGGER_TYPE_LEVEL) {
		field_value = 0;
	} else if (type == IRQ_TRIGGER_TYPE_EDGE_RISING) {
		field_value = 1;
	} else if (type == IRQ_TRIGGER_TYPE_EDGE_FALLING) {
		field_value = 3;
	} else {
		return;
	}
	reg_data = readb((void *)reg_addr);
	reg_data &= ~TRIGGER_TYPE_BIT_MASK;
	reg_data |= field_value << TRIGGER_TYPE_SHIFT;
	writeb(reg_data, (void *)reg_addr);
}
int clic_init(const struct irq_controller *ic)
{
	uint32_t i, reg_data, irq_cnt, preemption_bits;
	uint32_t reg_addr;

	sunxi_plat_irq_init();
	reg_addr = ic->reg_base_addr + CLIC_INFO_REG_OFF;
	reg_data = readl((void *)reg_addr);

	irq_cnt = (reg_data & IRQ_CNT_MASK) >> IRQ_CNT_SHIFT;
	debug("%s...%d irq_cnt:%d ic->irq_cnt:%d\n", __func__, __LINE__,
	       irq_cnt, ic->irq_cnt);
	if (ic->irq_cnt != irq_cnt)
		return -1;
	preemption_bits =
		(reg_data & CTRL_REG_BITS_MASK) >> CTRL_REG_BITS_SHIFT;
	preemption_bits <<= PREEMPTION_PRIORITY_BITS_SHIFT;
	preemption_bits &= PREEMPTION_PRIORITY_BITS_MASK;
	reg_addr = ic->reg_base_addr + CLIC_CFG_REG_OFF;
	writel(preemption_bits, (void *)reg_addr);
	for (i = 0; i < irq_cnt; i++) {
		//disable all interrupt
		reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(i);
		clic_set_enable(reg_addr, 0);
		reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(i);
		clic_set_vec_mode(reg_addr, 0);
		//clear pending
		reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(i);
		clic_set_pending(reg_addr, 1);
	}
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(i);
	debug("addr 0x%x(0x%x)set_enable 0  ", reg_addr,
	       readb((void *)reg_addr));
	reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(i);
	debug("addr 0x%x(0x%x) set_vec 1  ", reg_addr,
	       readb((void *)reg_addr));
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(i);
	debug("addr 0x%x(0x%x) clear pending 1  ", reg_addr,
	       readb((void *)reg_addr));
	return 0;
}
int sunxi_clic_init(void)
{
	return clic_init(&g_plat_ic_arr[ROOT_IRQ_CONTROLLER_ID]);
}
int clic_irq_enable(const struct irq_controller *ic, uint32_t irq_id)
{
	uint32_t reg_addr;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id);
	clic_set_enable(reg_addr, 1);
	return 0;
}
int clic_irq_disable(const struct irq_controller *ic, uint32_t irq_id)
{
	uint32_t reg_addr;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id);
	clic_set_enable(reg_addr, 0);
	return 0;
}
int clic_irq_is_enabled(const struct irq_controller *ic, uint32_t irq_id)
{
	if (readb((void *)(ic->reg_base_addr + CLIC_INT_X_IE_REG_OFF(irq_id))) &
	    IE_BIT_MASK)
		return 1;
	return 0;
}
int clic_irq_is_pending(const struct irq_controller *ic, uint32_t irq_id)
{
	if (readb((void *)(ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(irq_id))) &
	    IP_BIT_MASK)
		return 1;
	return 0;
}
int clic_irq_set_pending(const struct irq_controller *ic, uint32_t irq_id,
			 int pending)
{
	uint32_t reg_addr;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_IP_REG_OFF(irq_id);
	clic_set_pending(reg_addr, pending);
	return 0;
}
int clic_irq_set_trigger_type(const struct irq_controller *ic, uint32_t irq_id,
			      irq_trigger_type_t type)
{
	uint32_t reg_addr;
	if (type == IRQ_TRIGGER_TYPE_EDGE_BOTH)
		return -1;
	reg_addr = ic->reg_base_addr + CLIC_INT_X_ATTR_REG_OFF(irq_id);
	clic_set_trigger_type(reg_addr, type);
	return 0;
}
static void default_isr(void *data)
{
	printf("default_isr():  called from IRQ 0x%lx\n", (phys_addr_t)data);
	while (1)
		;
}
int irq_enable(int irq_no)
{
	/* static int old_irq_no; */
	if (irq_no >= AW_IRQ_CLIC_NUM) {
		printf("irq NO.(%d) > AW_IRQ_CLIC_NUM(%d) !!\n", irq_no,
		       AW_IRQ_CLIC_NUM);
		return -1;
	}
	clic_irq_enable(&g_plat_ic_arr[ROOT_IRQ_CONTROLLER_ID], irq_no);
	return 0;
}
int irq_disable(int irq_no)
{
	if (irq_no >= AW_IRQ_CLIC_NUM) {
		printf("irq NO.(%d) > AW_IRQ_CLIC_NUM(%d) !!\n", irq_no,
		       AW_IRQ_CLIC_NUM);
		return -1;
	}
	clic_irq_disable(&g_plat_ic_arr[ROOT_IRQ_CONTROLLER_ID], irq_no);
	return 0;
}
// static void plic_spi_handler(uint irq_no)
// {
// 	if (sunxi_int_handlers[irq_no].m_func != default_isr) {
// 		sunxi_int_handlers[irq_no].m_func(sunxi_int_handlers[irq_no].m_data);
// 	}
// }
void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data)
{
	if (irq >= AW_IRQ_CLIC_NUM || !handle_irq) {
		return;
	}
	sunxi_int_handlers[irq].m_data = data;
	sunxi_int_handlers[irq].m_func = handle_irq;
}
void irq_free_handler(int irq)
{
	disable_interrupts();
	if (irq >= AW_IRQ_CLIC_NUM) {
		enable_interrupts();
		return;
	}
	sunxi_int_handlers[irq].m_data = NULL;
	sunxi_int_handlers[irq].m_func = default_isr;
	enable_interrupts();
}
static void clic_spi_handler(uint irq_no)
{
	if (sunxi_int_handlers[irq_no].m_func != default_isr) {
		sunxi_int_handlers[irq_no].m_func(
			sunxi_int_handlers[irq_no].m_data);
	}
}
void external_interrupt(struct pt_regs *regs)
{
	u32 idnum     = 0;
	u32 rv_mcause = 0;
	irq_disable(idnum);
	asm volatile("csrr %0, mcause" : "=r"(rv_mcause));
	unsigned long riscv_mode = get_cur_riscv_mode();

	debug("%s...%d:rv_mcause:0x%x riscv_mode:0x%lx\n", __func__, __LINE__, rv_mcause, riscv_mode);
	riscv_mode == PRV_M ? csr_clear(mie, SR_MIE) : csr_clear(sie, SR_SIE);
	do {
		idnum = (rv_mcause & 0xFFF);
		if (idnum != 0) {
			clic_spi_handler(idnum);
			irq_enable(idnum);
			// writel(idnum, PLIC_M_S_CLAIM_REG);
			return;
		}
	} while (idnum != 0);
	riscv_mode == PRV_M ? csr_set(mie, SR_MIE) : csr_set(sie, SR_SIE);
	return;
}
