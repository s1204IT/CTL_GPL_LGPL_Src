/*
 * platform interfaces for XRadio drivers
 *
 * Copyright (c) 2013, XRadio
 * Author: XRadio
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/regulator/consumer.h>
//#include <asm/mach-types.h>
//#include <mach/sys_config.h>
#include "xradio.h"
#include "platform.h"
#include "sbus.h"
#include "hwio.h"
#include <linux/gpio.h>
#include <sunxi-gpio.h>
#include <linux/types.h>
//#include <linux/power/scenelock.h>
//#include <linux/power/aw_pm.h>
#include <linux/pm_wakeirq.h>

MODULE_AUTHOR("XRadioTech");
MODULE_DESCRIPTION("XRadioTech WLAN driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xradio_wlan");

extern void wlan_set_reset_pin(bool state);
extern void sunxi_wlan_set_power(bool on);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(int *, int *);
static int wlan_bus_id;
static u32 gpio_irq_handle;
static int irq_flags, wakeup_enable;

#ifndef CONFIG_DRIVER_V821

int xradio_get_syscfg(void)
{
	int wlan_bus_index = 0;
	wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0)
		return wlan_bus_index;
	else
		wlan_bus_id = wlan_bus_index;
	return wlan_bus_index;
}
/*********************Interfaces called by xradio core. *********************/
int xradio_plat_init(void)
{
	int ret = xradio_get_syscfg();
	if (ret < 0)
		return ret;

	xradio_dbg(XRADIO_DBG_ALWY, "Force remove card first\n");
	wlan_set_reset_pin(0);
	MCI_RESCAN_CARD(wlan_bus_id);
	return 0;
}

int xradio_wlan_power(int on)
{
	wlan_set_reset_pin(on);
	mdelay(100);
	return 0;
}


void xradio_plat_deinit(void)
{
;
}

#else
int xradio_get_syscfg(void)
{
	return 0;
}

void xradio_plat_deinit(void)
{
;
}

int xradio_plat_init(void)
{
	wakeup_enable = 0;
	return 0;
}
#endif

void xradio_sdio_detect(int enable)
{
	MCI_RESCAN_CARD(wlan_bus_id);
	xradio_dbg(XRADIO_DBG_ALWY, "%s SDIO card %d\n",
				enable ? "Detect" : "Remove", wlan_bus_id);
	mdelay(10);
}

static irqreturn_t xradio_gpio_irq_handler(int irq, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	unsigned long flags;

	SYS_BUG(!self);
	spin_lock_irqsave(&self->lock, flags);
	if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	spin_unlock_irqrestore(&self->lock, flags);
	return IRQ_HANDLED;
}

int xradio_request_gpio_irq(struct device *dev, void *sbus_priv)
{
	int ret = -1;
	gpio_irq_handle = sunxi_wlan_get_oob_irq(&irq_flags, &wakeup_enable);
	ret = devm_request_irq(dev, gpio_irq_handle,
					(irq_handler_t)xradio_gpio_irq_handler,
					irq_flags, "xradio_irq", sbus_priv);
	if (IS_ERR_VALUE((unsigned long)ret)) {
			gpio_irq_handle = 0;
			xradio_dbg(XRADIO_DBG_ERROR, "%s: request_irq FAIL!ret=%d\n",
					__func__, ret);
	}

	if (wakeup_enable) {
		ret = device_init_wakeup(dev, true);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "device init wakeup failed!\n");
			return ret;
		}

		ret = dev_pm_set_wake_irq(dev, gpio_irq_handle);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "can't enable wakeup src!\n");
			return ret;
		}
	}
	return ret;
}

void xradio_free_gpio_irq(struct device *dev, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	if (wakeup_enable) {
		device_init_wakeup(dev, false);
		dev_pm_clear_wake_irq(dev);
	}
	devm_free_irq(dev, gpio_irq_handle, self);
	gpio_irq_handle = 0;
}



#if defined(CONFIG_DRIVER_V821)
void HAL_CCM_ForceSys3Reset(void)
{
	u32 val = REG_READ_U32(CCM_MOD_RST_CTRL) & ~(CCM_WLAN_RST_BIT);
	REG_WRITE_U32(CCM_MOD_RST_CTRL, val);
}

void HAL_CCM_ReleaseSys3Reset(void)
{
	u32 val = REG_READ_U32(CCM_MOD_RST_CTRL) | CCM_WLAN_RST_BIT;
	REG_WRITE_U32(CCM_MOD_RST_CTRL, val);
}

int HAL_CCM_IsSys3Release(void)
{
	u32 val = REG_READ_U32(CCM_MOD_RST_CTRL) & (CCM_WLAN_RST_BIT);
	return !!val;
}

int HAL_PMU_IsSys3Alive(void)
{
	return !!(REG_READ_U32(HIF_WLAN_STATE) & WLAN_STATE_ACTIVE);
}


int xradio_wlan_power(int on)
{
	if (on) {
#ifdef CONFIG_DRIVER_R128
		/* set wlan sram default state to work mode from retention mode. */
		REG_SET_BIT(WLAN_SRAM_CTRL_REG, WLAN_SRAM_CTRL_WORK_BIT);
		HAL_CCM_EnableCPUWClk(1);
		msleep(1);
#endif
		HAL_CCM_ForceSys3Reset();
		xradio_dbg(XRADIO_DBG_ALWY, "xradio_wlan_power %d!\n", on);
		while (1) {
			if (HAL_PMU_IsSys3Alive())
				xradio_dbg(XRADIO_DBG_WARN, "%d wlan steal active\n", __LINE__);
			else
				break;
		}
		HAL_CCM_ReleaseSys3Reset();
		msleep(20);
	} else {
		HAL_CCM_ForceSys3Reset();
		while (1) {
			msleep(5);
			if (HAL_PMU_IsSys3Alive())
				xradio_dbg(XRADIO_DBG_WARN, "%d wlan steal active\n", __LINE__);
			else
				break;
		}
		msleep(20);
#ifdef CONFIG_DRIVER_R128
		HAL_CCM_EnableCPUWClk(0);
		msleep(5);
		/* set wlan sram default state to retention mode from work mode. */
		REG_CLR_BIT(WLAN_SRAM_CTRL_REG, WLAN_SRAM_CTRL_WORK_BIT);
#endif
	}
	return 0;
}

int xradio_get_freq_offset_from_efuse(u8 *freq_offset)
{
	// TODO: get efuse freq_offset if efuse freq_offset writed, eg: efpg_read_dcxo
	xradio_dbg(XRADIO_DBG_WARN, "%s: no get efuse freq_offset now!\n", __func__);
	return -1;
}

int xradio_set_freq_offset(struct xradio_common *hw_priv, u8 freq_offset)
{
	struct wsm_config_freq_offset arg;
	int ret = 0;
	u32 val = REG_READ_U32(CCU_AON_DCXO_CFG);

	if (val & CCU_AON_DCXO_FLAG_BIT) {
		u8 enhance_rf_clk = 0x1;

		val &= ~(CCU_AON_DCXO_DIE_FREQ_OFFSET_MASK | CCU_AON_DCXO_ENHANCE_RFCLK_OUTV9_MASK);
		val |= (freq_offset << CCU_AON_DCXO_DIE_FREQ_OFFSET_SHIFT) & CCU_AON_DCXO_DIE_FREQ_OFFSET_MASK;
		val |= (enhance_rf_clk << CCU_AON_DCXO_ENHANCE_RFCLK_OUTV9_SHIFT) & CCU_AON_DCXO_ENHANCE_RFCLK_OUTV9_MASK;
		REG_WRITE_U32(CCU_AON_DCXO_CFG, val);
		arg.dcxo_from_a_die = 0;
		xradio_dbg(XRADIO_DBG_ALWY, "dcxo from D die, set freq offset=%d\n", freq_offset);
	} else {
		arg.dcxo_from_a_die = 1;
		xradio_dbg(XRADIO_DBG_ALWY, "dcxo from A die, set freq offset=%d\n", freq_offset);
	}
	arg.freq_offset = freq_offset;
	ret = wsm_set_freq_offset(hw_priv, &arg, 0);
	if (ret)
		xradio_dbg(XRADIO_DBG_ERROR, "Set dcxo freq offset err:%d\n", ret);

	return ret;
}

int xradio_get_freq_offset(struct xradio_common *hw_priv, u8 *freq_offset, u8 *dcxo_from_a_die)
{
	int ret = 0;
	u32 val = REG_READ_U32(CCU_AON_DCXO_CFG);

	if (val & CCU_AON_DCXO_FLAG_BIT) {
		if (freq_offset)
			*freq_offset = (val & CCU_AON_DCXO_DIE_FREQ_OFFSET_MASK) >> CCU_AON_DCXO_DIE_FREQ_OFFSET_SHIFT;
		if (dcxo_from_a_die)
			*dcxo_from_a_die = 0;
		xradio_dbg(XRADIO_DBG_MSG, "dcxo from D die, freq offset=%d\n", *freq_offset);
	} else {
		struct wsm_config_freq_offset arg = {0};

		ret = wsm_get_freq_offset(hw_priv, &arg);
		if (ret) {
			xradio_dbg(XRADIO_DBG_ERROR, "Get dcxo freq offset err:%d\n", ret);
			return ret;
		}

		if (freq_offset)
			*freq_offset = arg.freq_offset;
		if (dcxo_from_a_die)
			*dcxo_from_a_die = 1;
		xradio_dbg(XRADIO_DBG_MSG, "dcxo from A die, freq offset=%d\n", *freq_offset);
	}

	return ret;
}
#endif


