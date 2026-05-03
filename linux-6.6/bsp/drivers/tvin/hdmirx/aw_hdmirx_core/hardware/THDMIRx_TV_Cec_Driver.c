/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/delay.h>
#include "../../aw_hdmirx_define.h"
#include "../include/display_datatype.h"
#include "../THDMIRx_System.h"
#include "THDMIRx_TV_Cec_Driver.h"
#include "ThdmiRxIPReg.h"

extern u32 bHdmi_drv_enable;

void HdmiRx_Cec_Hardware_set_regbase(uintptr_t edid_top)
{
	edid_top_base = edid_top;
}

/**hardware cec soft & enable
* bit2--0
* bit4--0
* bit5--1
* bit8--1
* bit9--1
*/
void HdmiRx_Cec_Hardware_assist_ctrl(void)
{
	HDMIRx_WriteRegMaskU32(edid_top_base + HDMIRX_ASSIST_CTR_REG, 0x334, 0x320);
}

/**
 * clear locked status
 * @param dev: address and device information
 * @return LOCKED status
 */
static int HdmiRx_Cec_Hardware_clear_lock(void)
{
	HDMIRx_WriteRegU32(edid_top_base + CEC_LOCK, HDMIRx_ReadRegU32(edid_top_base + CEC_LOCK) &
			~CEC_LOCK_LOCKED_BUFFER_MASK);
	return true;
}

/**
 * Enable interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to enable
 * @return error code
 */
static void HdmiRx_Cec_Hardware_interrupt_enable(unsigned char mask)
{
	HDMIRx_WriteRegU32(edid_top_base + IH_ENABLE_CEC_STAT0, HDMIRx_ReadRegU32(edid_top_base + IH_ENABLE_CEC_STAT0) | mask);
}

/**
 * Disable interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to disable
 * @return error code
 */
static void HdmiRx_Cec_Hardware_interrupt_disable(unsigned char mask)
{
	HDMIRx_WriteRegU32(edid_top_base + IH_ENABLE_CEC_STAT0, HDMIRx_ReadRegU32(edid_top_base + IH_ENABLE_CEC_STAT0) & ~mask);
}

/**
 * Clear interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to clear
 * @return error code
 */
/*static void _dw_cec_interrupt_clear(dw_hdmi_dev_t *dev, unsigned char mask)
{
	dev_write(dev, CEC_MASK, mask);
}*/

/**
 * Read interrupts
 * @param dev:  address and device information
 * @param mask: interrupt mask to read
 * @return INT content
 */
int HdmiRx_Cec_Hardware_interrupt_get_state(void)
{
	return HDMIRx_ReadRegU32(edid_top_base + IH_CEC_STAT0);
}

/**
 * Clear interrupts state
 * @param dev:  address and device information
 * @param mask: interrupt mask to clear
 * @return INT content
 */
int HdmiRx_Cec_Hardware_interrupt_clear_state(unsigned state)
{
	/* write 1 to clear */
	HDMIRx_WriteRegU32(edid_top_base + IH_CEC_STAT0, state);
	return 0;
}

/**
 * Set cec logical address
 * @param dev:  address and device information
 * @param addr: logical address
 * @return INT content
 * @return error code or bytes configured
 */
int HdmiRx_Cec_Hardware_set_logical_addr(unsigned int addr)
{
	LOG_TRACE1(addr);
	HDMIRx_WriteRegU32(edid_top_base + CEC_ADDR_L, (addr & 0xff));
	HDMIRx_WriteRegU32(edid_top_base + CEC_ADDR_H, (addr >> 8));
	return 0;
}

/**
 * Get cec logical address
 * @param dev:  address and device information
 * @return error code or bytes configured
 */
int HdmiRx_Cec_Hardware_get_logical_addr(void)
{
	return ((HDMIRx_ReadRegU32(edid_top_base + CEC_ADDR_H) << 8) | HDMIRx_ReadRegU32(edid_top_base + CEC_ADDR_L));
}

/**
 * Write transmission buffer
 * @param dev:  address and device information
 * @param buf:  data to transmit
 * @param size: data length [byte]
 * @return error code or bytes configured
 */
int HdmiRx_Cec_Hardware_send_frame(char *buf, unsigned size, unsigned frame_type)
{

	unsigned i;
	unsigned char data;
	for (i = 0; i < size; ++i) {
		CEC_INF("msg[%d] = 0x%lx\n", i, buf[i]);
	}

	HDMIRx_WriteRegU32(edid_top_base + CEC_TX_CNT, size); /*0x220*/
	for (i = 0; i < size; i++)
		HDMIRx_WriteRegU32(edid_top_base + CEC_TX_DATA + (i * 4), *buf++);  /*230*/

	data = HDMIRx_ReadRegU32(edid_top_base + CEC_CTRL) & (~(CEC_CTRL_FRAME_TYP_MASK));
	data |= frame_type | CEC_CTRL_SEND_MASK;
	HDMIRx_WriteRegU32(edid_top_base + CEC_CTRL, data);  /*210*/

	return 0;
}

/**
 * Read reception buffer
 * @param dev:  address and device information
 * @param buf:  buffer to hold receive data
 * @return size: reception data length [byte]
 */
int HdmiRx_Cec_Hardware_receive_frame(char *buf, unsigned *size)
{
	unsigned i;
	unsigned char cnt;

	cnt = HDMIRx_ReadRegU32(edid_top_base + CEC_RX_CNT);   /* mask 7-5? */   /*0x224*/
	cnt = (cnt > CEC_RX_DATA_SIZE) ? CEC_RX_DATA_SIZE : cnt;

	for (i = 0; i < cnt; i++)
		*buf++ = HDMIRx_ReadRegU32(edid_top_base + CEC_RX_DATA + (i * 4));  /*0x270*/

	HdmiRx_Cec_Hardware_clear_lock(); /*0x228*/

	*size = cnt;
	return 0;
}

/**
 * Open a CEC controller
 * @warning Execute before start using a CEC controller
 * @param dev:  address and device information
 * @return error code
 */
int HdmiRx_Cec_Hardware_enable(void)
{
	unsigned char mask = 0x0;

	/* clear ctrl */
	HDMIRx_WriteRegU32(edid_top_base + CEC_CTRL, 0);   /*0x210*/

	/* clear lock */
	HDMIRx_WriteRegU32(edid_top_base + CEC_LOCK, 0);   /*0x228*/

	/* clear cec logic address */
	HdmiRx_Cec_Hardware_set_logical_addr(0); /*0x218  0x21c*/

	/* TODO */
	HDMIRx_WriteRegU32(edid_top_base + CEC_WAKEUPCTRL, 0xFF);  /* disable wakeup */  /*0x22c*/

	/* clear all interrupt state */
	HdmiRx_Cec_Hardware_interrupt_clear_state(~0);  /*0x200*/

	/* enable irq */
	mask |= IH_ENABLE_CEC_STAT0_WAKEUP_MASK;
	mask |= IH_ENABLE_CEC_STAT0_DONE_MASK;
	mask |= IH_ENABLE_CEC_STAT0_EOM_MASK;
	mask |= IH_ENABLE_CEC_STAT0_NACK_MASK;
	mask |= IH_ENABLE_CEC_STAT0_ARB_LOST_MASK;
	mask |= IH_ENABLE_CEC_STAT0_ERROR_INITIATOR_MASK;
	mask |= IH_ENABLE_CEC_STAT0_ERROR_FOLLOW_MASK;
	HdmiRx_Cec_Hardware_interrupt_enable(mask);  /*0x204 enable*/

	return 0;
}

/**
 * Close a CEC controller
 * @warning Execute before stop using a CEC controller
 * @param dev:    address and device information
 * @return error code
 */
int HdmiRx_Cec_Hardware_disable(void)
{
	unsigned char mask = 0x0;

	/* disable interrupt*/
	mask |= IH_ENABLE_CEC_STAT0_WAKEUP_MASK;
	mask |= IH_ENABLE_CEC_STAT0_DONE_MASK;
	mask |= IH_ENABLE_CEC_STAT0_EOM_MASK;
	mask |= IH_ENABLE_CEC_STAT0_NACK_MASK;
	mask |= IH_ENABLE_CEC_STAT0_ARB_LOST_MASK;
	mask |= IH_ENABLE_CEC_STAT0_ERROR_INITIATOR_MASK;
	mask |= IH_ENABLE_CEC_STAT0_ERROR_FOLLOW_MASK;
	HdmiRx_Cec_Hardware_interrupt_disable(mask);

	/* TODO: support wakeup */
	/* _dw_cec_interrupt_clear(CEC_MASK_WAKEUP_MASK); */
	/* _dw_cec_interrupt_enable(CEC_MASK_WAKEUP_MASK); */
	/* _dw_cec_set_standby_mode(1); */

	return 0;
}

