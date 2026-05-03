// SPDX-License-Identifier: GPL-2.0
#ifndef __SUNXI_CLIC_H__
#define __SUNXI_CLIC_H__

#include <linux/types.h>
#include <asm/encoding.h>
#include <asm/csr.h>
#include <cpu.h>

#if defined(CONFIG_MACH_SUN300IW1)
#define AW_IRQ_USB_OTG (45)
#define AW_IRQ_USB_EHCI0 (46)
#define AW_IRQ_USB_OHCI0 (47)
#define AW_IRQ_TIMER0 (75)
#define AW_IRQ_TIMER1 (76)
#define AW_IRQ_CLIC_NUM (186)
#else
#error "Interrupt definition not available for this architecture"
#endif

/* RISC-V CLIC register offset */
#define CLIC_CFG_REG_OFF 0x00
#define CLIC_INFO_REG_OFF 0x04

#define CLIC_MINTTHRESH_REG_OFF 0x08

#define CLIC_INT_IP_REG_OFF 0x1000
#define CLIC_INT_IE_REG_OFF 0x1001
#define CLIC_INT_ATTR_REG_OFF 0x1002
#define CLIC_INT_CTRL_REG_OFF 0x1003

#define CLIC_INT_REG_ADDR_INTERVAL 0x4

#define CLIC_INT_X_IP_REG_OFF(id) (0x1000 + ((id)*CLIC_INT_REG_ADDR_INTERVAL))
#define CLIC_INT_X_IE_REG_OFF(id) (0x1001 + ((id)*CLIC_INT_REG_ADDR_INTERVAL))
#define CLIC_INT_X_ATTR_REG_OFF(id) (0x1002 + ((id)*CLIC_INT_REG_ADDR_INTERVAL))
#define CLIC_INT_X_CTRL_REG_OFF(id) (0x1003 + ((id)*CLIC_INT_REG_ADDR_INTERVAL))

#define CLIC_INX_X_32BIT_REG_OFF(id)                                           \
	(0x1000 + ((id)*CLIC_INT_REG_ADDR_INTERVAL))

/* CLIC_CFG_REG */
#define PREEMPTION_PRIORITY_BITS_SHIFT 1
#define PREEMPTION_PRIORITY_BITS_MASK (0xF << PREEMPTION_PRIORITY_BITS_SHIFT)

/* CLIC_INFO_REG */
#define IRQ_CNT_SHIFT 0
#define IRQ_CNT_MASK (0x1FFF << IRQ_CNT_SHIFT)

#define HW_VERSION_SHIFT 13
#define HW_VERSION_MASK (0xF << HW_VERSION_SHIFT)

#define HW_IMPL_VERSION_SHIFT 17
#define HW_IMPL_VERSION_MASK (0xF << HW_IMPL_VERSION_SHIFT)

#define CTRL_REG_BITS_SHIFT 21
#define CTRL_REG_BITS_MASK (0xF << CTRL_REG_BITS_SHIFT)

/* CLIC_INT_IP_REG */
#define IP_SHIFT 0
#define IP_BIT_MASK (0x1 << IP_SHIFT)

/* CLIC_INT_IE_REG */
#define IE_SHIFT 0
#define IE_BIT_MASK (0x1 << IE_SHIFT)

/* CLIC_INT_ATTR_REG */
#define HW_VECTOR_IRQ_SHIFT 0
#define HW_VECTOR_IRQ_BIT_MASK (0x1 << HW_VECTOR_IRQ_SHIFT)

#define TRIGGER_TYPE_SHIFT 1
#define TRIGGER_TYPE_BIT_MASK (0x3 << TRIGGER_TYPE_SHIFT)

typedef enum irq_trigger_type {
	IRQ_TRIGGER_TYPE_LEVEL,
	IRQ_TRIGGER_TYPE_EDGE_RISING,
	IRQ_TRIGGER_TYPE_EDGE_FALLING,
	IRQ_TRIGGER_TYPE_EDGE_BOTH
} irq_trigger_type_t;

typedef struct irq_controller {
	uint16_t id;
	uint16_t irq_cnt;
	uint16_t parent_id;
	uint16_t irq_id;
	//uint32_t flag; /* currently useless */

	unsigned long reg_base_addr;
	//const char *name; /* currently useless */
	// irq_controller_ops_t *ops;
	// #if defined(CONFIG_COMPONENTS_PM)
	// 	struct syscore_ops pm_ops;
	// 	void *priv;
	// #endif
} irq_controller_t;

int irq_enable(int irq_no);
int irq_disable(int irq_no);
void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data);
void irq_free_handler(int irq);

#endif //__SUNXI_CLIC_H__
