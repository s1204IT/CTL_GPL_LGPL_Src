/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "../util.h"
#include "irq.h"
#include "ctrl_phy.h"

#ifdef SUPPORT_PHY_JTAG

static void _phy_jtag_send_data_pulse(hdmi_tx_dev_t *dev, u8 tms, u8 tdi)
{
	u8 in_value = 0;

	in_value = set(in_value, JTAG_PHY_TAP_IN_JTAG_TMS_MASK, tms);
	in_value = set(in_value, JTAG_PHY_TAP_IN_JTAG_TDI_MASK, tdi);

	dev_write(dev, JTAG_PHY_TAP_TCK, (u8) 0);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_TAP_IN, in_value);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_TAP_TCK, (u8) 1);
	snps_sleep(100);
}

static void _phy_jtag_tap_soft_reset(hdmi_tx_dev_t *dev)
{
	int i;

	for (i = 0; i < 5; i++)
		_phy_jtag_send_data_pulse(dev, 1, 0);

	_phy_jtag_send_data_pulse(dev, 0, 0);
}

static void _phy_jtag_tap_goto_shift_dr(hdmi_tx_dev_t *dev)
{
	/* RTI -> Select-DR */
	_phy_jtag_send_data_pulse(dev, 1, 0);
	/* Select-DR-Scan -> Capture-DR -> Shift-DR */
	_phy_jtag_send_data_pulse(dev, 0, 0);
	_phy_jtag_send_data_pulse(dev, 0, 0);
}

static void _phy_jtag_tap_goto_shift_ir(hdmi_tx_dev_t *dev)
{
	/* RTI->Sel IR_Scan */
	_phy_jtag_send_data_pulse(dev, 1, 0);
	_phy_jtag_send_data_pulse(dev, 1, 0);
	/* Select-IR-Scan -> Shift_IR */
	_phy_jtag_send_data_pulse(dev, 0, 0);
	_phy_jtag_send_data_pulse(dev, 0, 0);
}

static void _phy_jtag_tap_goto_run_test_idle(hdmi_tx_dev_t *dev)
{
	/* Exit1_DR -> Update_DR */
	_phy_jtag_send_data_pulse(dev, 1, 0);

	/* Update_DR -> Run_Test_Idle */
	_phy_jtag_send_data_pulse(dev, 0, 0);
}

static void _phy_jtag_send_value_shift_ir(hdmi_tx_dev_t *dev, u8 jtag_addr)
{
	int i;

	for (i = 0; i < 7; i++) {
		_phy_jtag_send_data_pulse(dev, 0, jtag_addr & 0x01);
		jtag_addr = jtag_addr >> 1;
	}
	/* Shift_IR -> Exit_IR w/ last MSB bit */
	_phy_jtag_send_data_pulse(dev, 1, jtag_addr & 0x01);
}

static u16 _phy_jtag_send_value_shift_dr(hdmi_tx_dev_t *dev, u8 cmd, u16 data_in)
{
	int i;
	u32 aux_in = (cmd << 16) | data_in;
	u16 data_out = 0;
	/* Shift_DR */
	for (i = 0; i < 16; i++) {
		_phy_jtag_send_data_pulse(dev, 0, aux_in);
		data_out |= (dev_read(dev, JTAG_PHY_TAP_OUT) & 0x01) << i;
		aux_in = aux_in >> 1;
	}
	/* Shift_DR, TAP command bit */
	_phy_jtag_send_data_pulse(dev, 0, aux_in);
	aux_in = aux_in >> 1;

	/* Shift_DR -> Exit_DR w/ MSB TAP command bit */
	i++;
	_phy_jtag_send_data_pulse(dev, 1, aux_in);
	data_out |= (dev_read(dev, JTAG_PHY_TAP_OUT) & 0x01) << i;

	return data_out;
}

static void _phy_jtag_reset(hdmi_tx_dev_t *dev)
{
	dev_write(dev, JTAG_PHY_TAP_IN, 0x10);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_CONFIG, 0);
	snps_sleep(100);
	dev_write(dev, JTAG_PHY_CONFIG, 1); /* enable interface to JTAG */
	_phy_jtag_send_data_pulse(dev, 0, 0);
}

static void _phy_jtag_slave_address(hdmi_tx_dev_t *dev, u8 jtag_addr)
{
	_phy_jtag_tap_goto_shift_ir(dev);

	/* Shift-IR - write jtag slave address */
	_phy_jtag_send_value_shift_ir(dev, jtag_addr);

	_phy_jtag_tap_goto_run_test_idle(dev);
}

static int _phy_jtag_init(hdmi_tx_dev_t *dev, u8 jtag_addr)
{
	dev_write(dev, JTAG_PHY_ADDR, jtag_addr);
	_phy_jtag_reset(dev);
	_phy_jtag_tap_soft_reset(dev);
	_phy_jtag_slave_address(dev, jtag_addr);

	return 1;
}

static int _phy_jtag_read(hdmi_tx_dev_t *dev, u16 addr,  u16 *pvalue)
{
	_phy_jtag_tap_goto_shift_dr(dev);

	/* Shift-DR (shift 16 times) and -> Exit1 -DR */
	_phy_jtag_send_value_shift_dr(dev, JTAG_TAP_ADDR_CMD, addr << 8);

	_phy_jtag_tap_goto_run_test_idle(dev);
	_phy_jtag_tap_goto_shift_dr(dev);

	*pvalue = _phy_jtag_send_value_shift_dr(dev, JTAG_TAP_READ_CMD, 0xFFFF);

	_phy_jtag_tap_goto_run_test_idle(dev);

	return 0;
}

static int _phy_jtag_write(hdmi_tx_dev_t *dev, u16 addr,  u16 value)
{
	_phy_jtag_tap_goto_shift_dr(dev);

	/* Shift-DR (shift 16 times) and -> Exit1 -DR */
	_phy_jtag_send_value_shift_dr(dev, JTAG_TAP_ADDR_CMD, addr << 8);

	_phy_jtag_tap_goto_run_test_idle(dev);
	_phy_jtag_tap_goto_shift_dr(dev);

	_phy_jtag_send_value_shift_dr(dev, JTAG_TAP_WRITE_CMD, value);
	_phy_jtag_tap_goto_run_test_idle(dev);

	return 0;
}
#endif

static void _ctrl_phy_power_down(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SPARES_2_MASK, (bit ? 1 : 0));
}

static void _ctrl_phy_enable_tmds(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SPARES_1_MASK, (bit ? 1 : 0));
}

static void _ctrl_phy_data_enable_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SELDATAENPOL_MASK,
					(bit ? 1 : 0));
}

static void _ctrl_phy_interface_control(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SELDIPIF_MASK, (bit ? 1 : 0));
}

static int _ctrl_phy_lock_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

static void _ctrl_phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	/* Mask will determine which bits will be enabled */
	dev_write_mask(dev, PHY_MASK0, mask, 0xff);
}

static void _ctrl_phy_i2c_config(hdmi_tx_dev_t *dev)
{
	dev_write(dev, JTAG_PHY_CONFIG, JTAG_PHY_CONFIG_I2C_JTAGZ_MASK);
}

static void _ctrl_phy_i2c_mask_interrupts(hdmi_tx_dev_t *dev, int mask)
{
	LOG_TRACE1(mask);
	dev_write_mask(dev, PHY_I2CM_INT,
			PHY_I2CM_INT_DONE_MASK_MASK, mask ? 1 : 0);
	dev_write_mask(dev, PHY_I2CM_CTLINT,
			PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK, mask ? 1 : 0);
	dev_write_mask(dev, PHY_I2CM_CTLINT,
			PHY_I2CM_CTLINT_NACK_MASK_MASK, mask ? 1 : 0);
}

static void _ctrl_phy_i2c_mask_state(hdmi_tx_dev_t *dev)
{
	u8 mask = 0;

	mask |= IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK;
	mask |= IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK;
	dev_write_mask(dev, IH_I2CMPHY_STAT0, mask, 0);
}

static void _ctrl_phy_i2c_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, PHY_I2CM_SLAVE,
			PHY_I2CM_SLAVE_SLAVEADDR_MASK, value);
}

static int _ctrl_phy_i2c_write(hdmi_tx_dev_t *dev, u8 addr, u16 data)
{
	int timeout = PHY_TIMEOUT;
	u32 status  = 0;

	LOG_TRACE2(data, addr);

	/* Set address */
	dev_write(dev, PHY_I2CM_ADDRESS, addr);

	/* Set value */
	dev_write(dev, PHY_I2CM_DATAO_1, (u8) ((data >> 8) & 0xFF));
	dev_write(dev, PHY_I2CM_DATAO_0, (u8) (data & 0xFF));

	dev_write(dev, PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_WR_MASK);

	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CMPHY_STAT0,
				IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK |
				IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CMPHY_STAT0, status); /* clear read status */

	if (status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK) {
		pr_err("I2C PHY write failed\n");
		return -1;
	}

	if (status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK)
		return 0;

	pr_warn("ASSERT I2C Write timeout - check PHY - exiting\n");
	return -1;
}

static int _ctrl_phy_i2c_read(hdmi_tx_dev_t *dev, u8 addr, u16 *value)
{
	int timeout = PHY_TIMEOUT;
	u32 status  = 0;

	/* Set address */
	dev_write(dev, PHY_I2CM_ADDRESS, addr);

	dev_write(dev, PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_RD_MASK);

	do {
		snps_sleep(10);
		status = dev_read_mask(dev, IH_I2CMPHY_STAT0,
				IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK |
				IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(dev, IH_I2CMPHY_STAT0, status); /* clear read status */

	if (status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK) {
		pr_info(" I2C Read failed\n");
		return -1;
	}

	if (status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK) {

		*value = ((u16) (dev_read(dev, (PHY_I2CM_DATAI_1)) << 8)
				| dev_read(dev, (PHY_I2CM_DATAI_0)));
		return 0;
	}

	pr_info(" ASSERT I2C Read timeout - check PHY - exiting\n");
	return -1;
}

static int _ctrl_phy_set_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		_phy_jtag_slave_address(dev, 0xD4);
		return 0;
#endif
	case PHY_I2C:
		_ctrl_phy_i2c_slave_address(dev, value);
		return 0;
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}

static int _ctrl_phy_set_interface(hdmi_tx_dev_t *dev, phy_access_t interface)
{
	if (dev->snps_hdmi_ctrl.phy_access == interface) {
		pr_info("Phy interface already set to %s",
				interface == PHY_I2C ? "I2C" : "JTAG");

		(dev->snps_hdmi_ctrl.phy_access == PHY_I2C) ?
				_ctrl_phy_set_slave_address(dev, PHY_I2C_SLAVE_ADDR) : 0;

		return 0;
	}

	switch (interface) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		_phy_jtag_init(dev, 0xD4);
		break;
#endif
	case PHY_I2C:
		_ctrl_phy_i2c_config(dev);
		_ctrl_phy_set_slave_address(dev, PHY_I2C_SLAVE_ADDR);
		break;
	default:
		pr_err("PHY interface not defined");
		return -1;
	}
	dev->snps_hdmi_ctrl.phy_access = interface;
	pr_info("PHY interface set to %s",
			interface == PHY_I2C ? "I2C" : "JTAG");
	return 0;
}

void ctrl_phy_config_svsret(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, (bit ? 1 : 0));
}

void ctrl_phy_gen2_pddq(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, (bit ? 1 : 0));
}

void ctrl_phy_gen2_tx_power_on(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, (bit ? 1 : 0));
}

void ctrl_phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_I2CM_DIV, PHY_I2CM_DIV_FAST_STD_MODE_MASK, bit);
}

void ctrl_phy_i2c_master_reset(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	dev_write_mask(dev, PHY_I2CM_SOFTRSTZ,
			PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK, 1);
}

int ctrl_phy_reconfigure_interface(hdmi_tx_dev_t *dev)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		_phy_jtag_init(dev, 0xD4);
		break;
#endif
	case PHY_I2C:
		_ctrl_phy_i2c_config(dev);
		_ctrl_phy_set_slave_address(dev, PHY_I2C_SLAVE_ADDR);
		break;
	default:
		pr_err("PHY interface not defined");
		return -1;
	}
	pr_info("PHY interface reconfiguration, set to %s",
		dev->snps_hdmi_ctrl.phy_access == PHY_I2C ? "I2C" : "JTAG");
	return 0;

}

int ctrl_phy_standby(hdmi_tx_dev_t *dev)
{
#ifndef PHY_THIRD_PARTY
	u8 phy_mask = 0;

	phy_mask |= PHY_MASK0_TX_PHY_LOCK_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_0_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_1_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_2_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_3_MASK;
	_ctrl_phy_interrupt_mask(dev, phy_mask);

	_ctrl_phy_enable_tmds(dev, 0);
	_ctrl_phy_power_down(dev, 0);	/* disable PHY */
	ctrl_phy_gen2_tx_power_on(dev, 0);
	ctrl_phy_gen2_pddq(dev, 1);
#endif
	return TRUE;
}

int ctrl_phy_enable_hpd_sense(hdmi_tx_dev_t *dev)
{
#ifndef PHY_THIRD_PARTY
	dev_write_mask(dev, PHY_CONF0,
		PHY_CONF0_ENHPDRXSENSE_MASK, 1);
#endif
	return TRUE;
}

int ctrl_phy_disable_hpd_sense(hdmi_tx_dev_t *dev)
{
#ifndef PHY_THIRD_PARTY
	dev_write_mask(dev, PHY_CONF0,
		PHY_CONF0_ENHPDRXSENSE_MASK, 0);
#endif
	return TRUE;
}

u8 ctrl_phy_hot_plug_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_HPD_MASK);
}

u8 ctrl_phy_rxsense_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return (u8)(dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_0_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_1_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_2_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_3_MASK));
}

u8 ctrl_phy_pll_lock_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

u8 ctrl_phy_power_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK);
}

void ctrl_phy_power_enable(hdmi_tx_dev_t *dev, u8 enable)
{
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, !enable);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, enable);
}

/* wait PHY_TIMEOUT no of cycles at most for the PLL lock signal to raise ~around 20us max */
int ctrl_phy_wait_lock(hdmi_tx_dev_t *dev)
{
	int i = 0;

	for (i = 0; i < PHY_TIMEOUT; i++) {
		snps_sleep(5);
		if (_ctrl_phy_lock_state(dev) & 0x1) {
			return 1;
		}
	}
	return 0;
}

int ctrl_phy_write(hdmi_tx_dev_t *dev, u8 addr, u16 data)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		return _phy_jtag_write(dev, addr, data);
#endif
	case PHY_I2C:
		return _ctrl_phy_i2c_write(dev, addr, data);
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}

int ctrl_phy_read(hdmi_tx_dev_t *dev, u8 addr, u16 *value)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
#ifdef SUPPORT_PHY_JTAG
	case PHY_JTAG:
		return _phy_jtag_read(dev, addr, value);
#endif
	case PHY_I2C:
		return _ctrl_phy_i2c_read(dev, addr, value);
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}

int ctrl_phy_initialize(hdmi_tx_dev_t *dev)
{
	u8 phy_mask = 0;
	u8 data_polarity = 0;

	LOG_TRACE();

	if (_ctrl_phy_set_interface(dev, PHY_I2C) < 0) {
		pr_err("set phy interface i2c failed!\n");
		return FALSE;
	}

	data_polarity = dev->snps_hdmi_ctrl.data_enable_polarity;

#ifndef PHY_THIRD_PARTY
	ctrl_phy_gen2_tx_power_on(dev, 0);
	ctrl_phy_gen2_pddq(dev, 1);

	phy_mask |= PHY_MASK0_TX_PHY_LOCK_MASK;
	phy_mask |= PHY_STAT0_HPD_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_0_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_1_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_2_MASK;
	phy_mask |= PHY_MASK0_RX_SENSE_3_MASK;
	_ctrl_phy_interrupt_mask(dev, phy_mask);

	_ctrl_phy_data_enable_polarity(dev, data_polarity);

	_ctrl_phy_interface_control(dev, 0);

	_ctrl_phy_enable_tmds(dev, 0);

	_ctrl_phy_power_down(dev, 0);	/* disable PHY */

	_ctrl_phy_i2c_mask_interrupts(dev, 0);
#else
	pr_info("Third Party PHY build\n");
#endif

	/* Clean IH_I2CMPHY_STAT0 */
	_ctrl_phy_i2c_mask_state(dev);

	return TRUE;
}
