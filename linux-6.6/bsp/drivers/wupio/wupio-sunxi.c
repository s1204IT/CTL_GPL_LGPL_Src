// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Wupio driver for Sunxi Platform of Allwinner SoC
 *
 * Copyright (c) 2024, luwinkey<luwinkey@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#include <sunxi-log.h>
#include <sunxi-gpio.h>
#include <wupio-sunxi.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/pm_wakeirq.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include <asm/sbi.h>

struct sunxi_wupio_dev *wupio_dev;

/* For self-test or No other drivers registered */
//#define WUPIO_SELF_CONFIG

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
/* TODO: value of LPCLK should be obtained dynamically */
#define LFCLK 32768
#define MAX_CLK_SEL 14
#define MIN_DEBC_CYCLE 1
#define MAX_DEBC_CYCLE 16

#define RTC_WAKE_IO_EN_REG			0x050
#define RTC_WAKE_IO_DEB_CLK_REG		0x054
#define RTC_WAKE_IO_STA_REG			0x058
#define RTC_WAKE_IO_HOLD_CTRL_REG	0x05C
#define RTC_WAKE_IO_WUP_GEN_REG		0x060
#define RTC_WAKE_IO_DEB_CYCLSE0_REG	0x064
#define RTC_WAKE_IO_DEB_CYCLSE1_REG	0x068

#define WUPIO_DEB_CLK_IO_SEL_SHIFT 0
#define WUPIO_DEB_CLK_SET0_MASK 0xF
#define WUPIO_DEB_CLK_SET0_SHIFT 24
#define WUPIO_DEB_CLK_SET1_MASK 0xF
#define WUPIO_DEB_CLK_SET1_SHIFT 28

#define WUPIO_DEB_CYCLSE0_MASK 0xF
#define WUPIO_DEB_CYCLSE0_SHIFT(wupio_index) ((wupio_index) * 4)
#define WUPIO_DEB_CYCLSE1_MASK 0xF
#define WUPIO_DEB_CYCLSE1_SHIFT(wupio_index) ((wupio_index) * 4)

#define WUPIO_EDGE_MODE_SHIFT 16
#define WUPIO_EDGE_HOLD_CTRL_SHIFT 0
#define WUPIO_EDGE_WUP_GEN_SHIFT 0
#define WUPIO_EN_SHIFT 0

#define GET_PL_INDEX(gpio_number) (gpio_number - SUNXI_PL_BASE)
static const int wupio_map[] = {0, 1, 2, 3, 4, 5, 6, 7};
#define MAX_WAKEIO_COUNT ARRAY_SIZE(wupio_map)

int gpio_num_to_wupio_index(int gpio_number)
{
	int pl_index = GET_PL_INDEX(gpio_number);

	if ((pl_index >= sizeof(wupio_map) / sizeof(wupio_map[0])) || (pl_index < 0))
		return -EINVAL;

	return wupio_map[pl_index];
}
#endif

typedef int (*wupio_callback_t)(void);
static wupio_callback_t callback_functions[MAX_WAKEIO_COUNT] = {0};

static int sunxi_wupio_set_callback(int wakeio_index, wupio_callback_t callback)
{
	if (callback_functions[wakeio_index] != NULL) {
		sunxi_info(wupio_dev->dev, "Callback of wupio[%d] is registered\n", wakeio_index);
		return -1;
	}

	callback_functions[wakeio_index] = callback;

	return 0;
}

/* Function to calculate Debounce given CLK_SEL and DEBC_CYCLE */
static u32 calculate_debounce(u32 clk_sel, u32 debc_cycles)
{
	/* Equivalent to 2^CLK_SEL */
	u32 freq_div = 1 << clk_sel;
	/* Rounded up division equivalent to LFCLK / freq_div in microseconds */
	u32 cycle = (1000000 + (LFCLK / 2) - 1) / (LFCLK / freq_div);

	return debc_cycles * cycle;
}

/* Function to find closest CLK_SEL and DEBC_CYCLE for a given Debounce value */
static void find_clk_sel_debc_cycles(u32 debounce,  u8 *best_clk_sel, u32 *best_debc_cycles)
{
	u32 best_diff = UINT_MAX;
	u8 clk_sel;
	u32 debc_cycles;

	for (clk_sel = 0; clk_sel <= MAX_CLK_SEL; clk_sel++) {
		for (debc_cycles = MIN_DEBC_CYCLE; debc_cycles <= MAX_DEBC_CYCLE; debc_cycles++) {
			u32 calculated_debounce = calculate_debounce(clk_sel, debc_cycles);
			u32 diff = (calculated_debounce > debounce) ?
						(calculated_debounce - debounce) :
						(debounce - calculated_debounce);
			if (diff < best_diff) {
				best_diff = diff;
				*best_clk_sel = clk_sel;
				*best_debc_cycles = debc_cycles;
			}
		}
	}
}

static inline void gprcm_reg_write(struct sunxi_wupio_dev *dev, u32 reg, u32 val)
{
	writel(val, dev->gprcm_base + reg);
}

static inline u32 gprcm_reg_read(struct sunxi_wupio_dev *dev, u32 reg)
{
	return readl(dev->gprcm_base + reg);
}

static void __iomem *map_gprcm_base(struct device *dev)
{
	u32 count;
	u32 base, size;
	struct device_node *np = dev->of_node;
	const __be32 *gprcm_reg;

	gprcm_reg = of_get_property(np, "gprcm_reg", &count);
	if (!gprcm_reg || (count != sizeof(u32) * 4))
		return NULL;

	base = be32_to_cpu(gprcm_reg[1]);
	size = be32_to_cpu(gprcm_reg[3]);

	return ioremap(base, size);
}

static int count_wakeup_pins(struct device_node *parent_node)
{
	int count = 0;
	struct device_node *child_node;

	for_each_child_of_node(parent_node, child_node) {
		if (of_node_name_eq(child_node, "wakeup_pins")) {
			count = of_get_child_count(child_node);
			break;
		}
	}

	return count;
}

static void handle_wakeio_detection(struct sunxi_wupio_dev *wupio_dev)
{
	u32 reg_value, i;

	reg_value = gprcm_reg_read(wupio_dev, RTC_WAKE_IO_STA_REG);
	for (i = 0; i < ARRAY_SIZE(wupio_map); i++) {
		if (reg_value & (1 << i)) {
			sunxi_info(wupio_dev->dev, "WakeIO[%d] is detected!\n", i);
			if (callback_functions[i])
				callback_functions[i]();
			reg_value &= ~(1 << i);
			gprcm_reg_write(wupio_dev, RTC_WAKE_IO_STA_REG, reg_value);
			/* write 1 to clear status */
			reg_value |= (1 << i);
			gprcm_reg_write(wupio_dev, RTC_WAKE_IO_STA_REG, reg_value);
		}
	}
}

/* Interrupt Sevice Routine */
static irqreturn_t sunxi_wupio_isr(int irq, void *data)
{
	struct sunxi_wupio_dev *wupio_dev = (struct sunxi_wupio_dev *)data;

	handle_wakeio_detection(wupio_dev);

	return IRQ_HANDLED;
}

void sunxi_wupio_set_debounce_clk(struct sunxi_wupio_dev *dev,
								u8 wupio_index, u8 clk_src, u8 clk_sel)
{
	u32 reg_value;
	u8 bit_offset;

	if (clk_src == WAKEIO_DEB_CLK_0) {
		reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_DEB_CLK_REG);
		/* set wupio num */
		bit_offset = WUPIO_DEB_CLK_IO_SEL_SHIFT + wupio_index;
		reg_value &= ~(1 << bit_offset);
		reg_value |= 0 << bit_offset;
		/* set clk_sel0 value */
		reg_value &= ~(WUPIO_DEB_CLK_SET0_MASK << WUPIO_DEB_CLK_SET0_SHIFT);
		reg_value |= (clk_sel & WUPIO_DEB_CLK_SET0_MASK) << WUPIO_DEB_CLK_SET0_SHIFT;
		gprcm_reg_write(dev, RTC_WAKE_IO_DEB_CLK_REG, reg_value);
	} else if (clk_src == WAKEIO_DEB_CLK_1) {
		reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_DEB_CLK_REG);
		/* set wupio num */
		bit_offset = WUPIO_DEB_CLK_IO_SEL_SHIFT + wupio_index;
		reg_value &= ~(1 << bit_offset);
		reg_value |= 1 << bit_offset;
		/* set clk_sel1 value */
		reg_value &= ~(WUPIO_DEB_CLK_SET1_MASK << WUPIO_DEB_CLK_SET1_SHIFT);
		reg_value |= (clk_sel & WUPIO_DEB_CLK_SET1_MASK) << WUPIO_DEB_CLK_SET1_SHIFT;
		gprcm_reg_write(dev, RTC_WAKE_IO_DEB_CLK_REG, reg_value);
	}
}

void sunxi_wupio_set_debounce_cycles(struct sunxi_wupio_dev *dev,
									u8 wupio_index, u8 edge_mode, u8 debc_cycles)
{
	u32 reg_value;
	u8 set_debc_cycles;
	u8 bit_offset;

	set_debc_cycles = debc_cycles - 1;

	if (edge_mode == WUPIO_POSITIVE_EDGE) {
		/* set cycles to DEB_CYCLSE1 */
		reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_DEB_CYCLSE1_REG);
		bit_offset = WUPIO_DEB_CYCLSE1_SHIFT(wupio_index);
		reg_value &= ~(WUPIO_DEB_CYCLSE1_MASK << bit_offset);
		reg_value |= (set_debc_cycles & WUPIO_DEB_CYCLSE1_MASK) << bit_offset;
		gprcm_reg_write(dev, RTC_WAKE_IO_DEB_CYCLSE1_REG, reg_value);
	} else if (edge_mode == WUPIO_NEGATIVE_EDGE) {
		/* set cycles to DEB_CYCLSE0 */
		reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_DEB_CYCLSE0_REG);
		bit_offset = WUPIO_DEB_CYCLSE0_SHIFT(wupio_index);
		reg_value &= ~(WUPIO_DEB_CYCLSE0_MASK << bit_offset);
		reg_value |= (set_debc_cycles & WUPIO_DEB_CYCLSE0_MASK) << bit_offset;
		gprcm_reg_write(dev, RTC_WAKE_IO_DEB_CYCLSE0_REG, reg_value);
	}
}

static void sunxi_wupio_set_debounce(struct sunxi_wupio_dev *dev, u8 wupio_index,
									u8 clk_src, u8 edge_mode, u32 time_us)
{
	u8 clk_sel;
	u32 debc_cycles;

	find_clk_sel_debc_cycles(time_us, &clk_sel, &debc_cycles);
	sunxi_wupio_set_debounce_clk(dev, wupio_index, clk_src, clk_sel);
	sunxi_wupio_set_debounce_cycles(dev, wupio_index, edge_mode, debc_cycles);
}

static void sunxi_wupio_set_edge_mode(struct sunxi_wupio_dev *dev,
										u8 wupio_index, u8 edge_mode)
{
	u32 reg_value;
	u8 bit_offset;

	reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_EN_REG);
	bit_offset = WUPIO_EDGE_MODE_SHIFT + wupio_index;
	reg_value &= ~(1 << bit_offset);
	reg_value |= edge_mode << bit_offset;

	gprcm_reg_write(dev, RTC_WAKE_IO_EN_REG, reg_value);
}

static void sunxi_wupio_hold_control(struct sunxi_wupio_dev *dev,
									u8 wupio_index, u8 hold_status)
{
	u32 reg_value;
	u8 bit_offset;

	reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_HOLD_CTRL_REG);
	bit_offset = WUPIO_EDGE_HOLD_CTRL_SHIFT + wupio_index;
	reg_value &= ~(1 << bit_offset);
	reg_value |= hold_status << bit_offset;

	gprcm_reg_write(dev, RTC_WAKE_IO_HOLD_CTRL_REG, reg_value);
}

static void sunxi_wupio_enable_interrupt(struct sunxi_wupio_dev *dev,
										u8 interrupt_status)
{
	u32 reg_value;

	reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_WUP_GEN_REG);
	reg_value &= ~(1 << WUPIO_EDGE_WUP_GEN_SHIFT);
	reg_value |= interrupt_status << WUPIO_EDGE_WUP_GEN_SHIFT;

	gprcm_reg_write(dev, RTC_WAKE_IO_WUP_GEN_REG, reg_value);
}

static void sunxi_wupio_detection_enable(struct sunxi_wupio_dev *dev,
										u8 wupio_index, u8 detection_status)
{
	u32 reg_value;
	u8 bit_offset;

	reg_value = gprcm_reg_read(dev, RTC_WAKE_IO_EN_REG);
	bit_offset = WUPIO_EN_SHIFT + wupio_index;
	reg_value &= ~(1 << bit_offset);
	reg_value |= detection_status << bit_offset;

	gprcm_reg_write(dev, RTC_WAKE_IO_EN_REG, reg_value);
}

static int sunxi_wupio_set(struct wakeup_pin *pin)
{
	int gpio_num;
	int ret;
	u8 wupio_index, edge_mode;

	/* get wupio_index */
	gpio_num = desc_to_gpio(pin->gpio);
	ret = gpio_num_to_wupio_index(gpio_num);
	if (ret < 0)
		return ret;

	wupio_index = (u8)ret;

	/* 1. set gpio as input */
	ret = gpiod_direction_input(pin->gpio);
	if (ret)
		return ret;
	/* 2. set edge mode */
	if (strcmp(pin->edge_mode, "positive") == 0)
		edge_mode = WUPIO_POSITIVE_EDGE;
	else
		edge_mode = WUPIO_NEGATIVE_EDGE;
	sunxi_wupio_set_edge_mode(wupio_dev, wupio_index, edge_mode);
	/* 3. set debounce:clk_sel and debc_cycles */
	sunxi_wupio_set_debounce(wupio_dev, wupio_index, pin->clk_src,
								edge_mode, pin->debounce_us);
	/* 4. enable io hold */
	sunxi_wupio_hold_control(wupio_dev, wupio_index, WUPIO_HOLD_ENABLE);
	/* 5. enable wakeup interrupt */
	sunxi_wupio_enable_interrupt(wupio_dev, WUPIO_INTERRUPT_ENABLE);
	/* 6. enable io detection */
	sunxi_wupio_detection_enable(wupio_dev, wupio_index, WUPIO_DETECTION_ENABLE);

	return 0;
}

static void sunxi_wupio_set_pins(void)
{
	int i;
	struct wakeup_pin *pin;

	for (i = 0; i < wupio_dev->num_pins; i++) {
		pin = &wupio_dev->pins[i];
		if (pin->share_io)
			continue;

		if (sunxi_wupio_set(pin)) {
			sunxi_err(wupio_dev->dev, "wakeup io %s: set pins failed\n", pin->name);
		}
	}
}

static struct wakeup_pin *sunxi_wupio_find(struct device *dev)
{
	int i;
	const char *node_name;
	struct wakeup_pin *pin = NULL;
	struct device_node *wakeup_pin_np;

	wakeup_pin_np = of_parse_phandle(dev->of_node, "wakeup_io", 0);
	if (!wakeup_pin_np) {
		sunxi_err(wupio_dev->dev, "Failed to parse wakeup_io, please check dts\n");
		return NULL;
	}

	node_name = of_node_full_name(wakeup_pin_np);
	if (!node_name) {
		of_node_put(wakeup_pin_np);
		return NULL;
	}

	for (i = 0; i < wupio_dev->num_pins; i++) {
		pin = &wupio_dev->pins[i];
		if (strcmp(node_name, pin->name) == 0) {
			break;
		} else {
			pin = NULL;
		}
	}
	of_node_put(wakeup_pin_np);

	return pin;
}

int sunxi_wupio_get_index(struct device *dev)
{
	int gpio_num;
	struct wakeup_pin *pin;

	if (!dev) {
		sunxi_err(wupio_dev->dev, "invalid param for get_index\n");
		return -EINVAL;
	}

	if (!wupio_dev || (wupio_dev->num_pins == 0)) {
		sunxi_err(wupio_dev->dev, "wupio is not exist\n");
		return -ENODEV;
	}

	pin = sunxi_wupio_find(dev);
	if (pin == NULL) {
		sunxi_err(wupio_dev->dev, "get_index does not find wupio pin\n");
		return -EINVAL;
	}

	if (pin->share_io) {
		sunxi_err(wupio_dev->dev, "wupio pin %s is share_io, which can not get index\n", pin->name);
		return -EINVAL;
	}
	gpio_num = desc_to_gpio(pin->gpio);

	return gpio_num_to_wupio_index(gpio_num);
}
EXPORT_SYMBOL_GPL(sunxi_wupio_get_index);

int sunxi_wupio_is_illegal(struct device *dev, struct gpio_desc *gpio)
{
	struct wakeup_pin *pin;

	if (IS_ERR(gpio) || !dev) {
		sunxi_err(wupio_dev->dev, "invalid params\n");
		return -EINVAL;
	}

	if (!wupio_dev || (wupio_dev->num_pins == 0)) {
		sunxi_err(wupio_dev->dev, "wupio is not exist\n");
		return -ENODEV;
	}

	pin = sunxi_wupio_find(dev);
	if (pin == NULL)
		return 1;

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_wupio_is_illegal);

int sunxi_share_wupio_enable(struct device *dev, struct gpio_desc *gpio)
{
	int gpio_num;
	struct wakeup_pin *pin;

	if (IS_ERR(gpio) || !dev) {
		sunxi_err(wupio_dev->dev, "invalid params for wupio_set\n");
		return -EINVAL;
	}

	if (!wupio_dev || (wupio_dev->num_pins == 0)) {
		sunxi_err(wupio_dev->dev, "wupio is not exist\n");
		return -ENODEV;
	}

	pin = sunxi_wupio_find(dev);
	if (pin == NULL) {
		sunxi_err(wupio_dev->dev, "wupio_set does not find wupio pin\n");
		return -EINVAL;
	}

	gpio_num = desc_to_gpio(gpio);
	if ((gpio_num != pin->ionum) || (pin->share_io == 0)) {
		sunxi_err(wupio_dev->dev, "invalid wupio pin\n");
		return -EINVAL;
	}
	pin->gpio = gpio;

	return sunxi_wupio_set(pin);
}
EXPORT_SYMBOL_GPL(sunxi_share_wupio_enable);

int sunxi_wupio_register_callback(int wupio_index, void *callback)
{
	wupio_callback_t func;

	if (wupio_index < 0 || wupio_index >= MAX_WAKEIO_COUNT) {
		sunxi_err(wupio_dev->dev, "wakeio_index is invalid\n");
		return -1;
	}

	if (callback == NULL) {
		sunxi_err(wupio_dev->dev, "callback func is invalid\n");
		return -1;
	}

	func = (wupio_callback_t)callback;

	return sunxi_wupio_set_callback(wupio_index, func);
}
EXPORT_SYMBOL_GPL(sunxi_wupio_register_callback);

static const struct of_device_id sunxi_wupio_dt_ids[] = {
	{.compatible = "allwinner,sunxi_wupio"},
	{},
};

static int sunxi_wupio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	const struct of_device_id *of_id;
	struct wakeup_pin *pin;
	int err, i = 0;
	struct device_node *pin_node, *wakeup_pins_node;

	if (wupio_dev) {
		kfree(wupio_dev);
		wupio_dev = NULL;
	}

	wupio_dev = devm_kzalloc(dev, sizeof(*wupio_dev), GFP_KERNEL);
	if (!wupio_dev)
		return -ENOMEM;

	of_id = of_match_device(sunxi_wupio_dt_ids, dev);
	if (!of_id) {
		sunxi_err(dev, "of_match_device() failed\n");
		return -EINVAL;
	}

	wupio_dev->dev = dev;
	wupio_dev->gprcm_base = map_gprcm_base(dev);
	wupio_dev->irq = platform_get_irq(pdev, 0);
	if (wupio_dev->irq < 0) {
		err = wupio_dev->irq;
		return err;
	}
	/* Count number of wakeup pins */
	wupio_dev->num_pins = count_wakeup_pins(np);
	if (wupio_dev->num_pins <= 0) {
		sunxi_err(dev, "No wakeup pins found\n");
		return -ENODEV;
	}
	/* Allocate memory for wakeup pins */
	wupio_dev->pins = devm_kzalloc(dev, wupio_dev->num_pins * sizeof(struct wakeup_pin),
									GFP_KERNEL);
	if (!wupio_dev->pins)
		return -ENOMEM;

	wakeup_pins_node = of_get_child_by_name(np, "wakeup_pins");
	if (!wakeup_pins_node) {
		sunxi_err(dev, "No wakeup_pins node found\n");
		return -ENODEV;
	}
	/* Get wakeup pins config info */
	for_each_child_of_node(wakeup_pins_node, pin_node) {
		const char *node_name;
		pin = &wupio_dev->pins[i++];
		node_name = of_node_full_name(pin_node);
		if (node_name != NULL) {
			strlcpy(pin->name, node_name, sizeof(pin->name));
		}

		pin->share_io = of_property_read_bool(pin_node, "share-io");
		if (pin->share_io == 0) {
			pin->gpio = devm_gpiod_get_from_of_node(dev, pin_node, "gpio_info", 0, 0, "null");
			if (IS_ERR(pin->gpio)) {
				sunxi_err(dev, "Failed to get gpio info\n");
				return -EINVAL;
			}
		} else {
			pin->gpio = NULL;
			sunxi_info(dev, "pin %s is share_io\n", pin->name);
		}

		pin->ionum = of_get_named_gpio(pin_node, "gpio_info", 0);
		if (pin->ionum < 0) {
			sunxi_err(dev, "Failed to get ionum\n");
			return -EFAULT;
		}

		pin->edge_mode = of_get_property(pin_node, "edge_mode", NULL);
		sunxi_debug(dev, "edge_mode:%s\n", pin->edge_mode);
		if (!pin->edge_mode) {
			sunxi_err(dev, "Failed to get edge_mode property\n");
			return -EINVAL;
		}

		err = of_property_read_u32(pin_node, "clk_src", &pin->clk_src);
		sunxi_debug(dev, "clk_src:%d\n", pin->clk_src);
		if (err < 0) {
			sunxi_err(dev, "Failed to get clk_src property\n");
			return -EINVAL;
		}

		err = of_property_read_u32(pin_node, "debounce_us", &pin->debounce_us);
		sunxi_debug(dev, "debounce_us:%d\n", pin->debounce_us);
		if (err < 0) {
			sunxi_err(dev, "Failed to get debounce_us property\n");
			return -EINVAL;
		}
	}

	err = devm_request_irq(dev, wupio_dev->irq, sunxi_wupio_isr, 0,
							dev_name(dev), wupio_dev);
	if (err) {
		sunxi_err(dev, "Could not request wupio irq\n");
		return -1;
	}

	if (of_property_read_bool(np, "wakeup-source")) {
		device_init_wakeup(dev, true);
		/* enable wakeup source before suspend_noirq callback */
		dev_pm_set_wake_irq(dev, wupio_dev->irq);
	}

	platform_set_drvdata(pdev, wupio_dev);
	sunxi_wupio_set_pins();

	sunxi_info(dev, "sunxi wupio probe success\n");

	return err;
}

static int sunxi_wupio_remove(struct platform_device *pdev)
{
	int i;

	struct sunxi_wupio_dev *wupio_dev = platform_get_drvdata(pdev);

	for (i = 0; i < ARRAY_SIZE(wupio_map); i++) {
		/* 1. disable io detection */
		sunxi_wupio_detection_enable(wupio_dev, i, WUPIO_DETECTION_DISABLE);
		/* 2. disable io hold */
		sunxi_wupio_hold_control(wupio_dev, i, WUPIO_HOLD_DISABLE);
	}
	/* 3. disable wakeup interrupt */
	sunxi_wupio_enable_interrupt(wupio_dev, WUPIO_INTERRUPT_DISABLE);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_wupio_suspend(struct device *dev)
{
	u8 wupio_index;
	int i, ret, gpio_num;
	struct wakeup_pin *pin;

	for (i = 0; i < wupio_dev->num_pins; i++) {
		pin = &wupio_dev->pins[i];
		gpio_num = desc_to_gpio(pin->gpio);
		ret = gpio_num_to_wupio_index(gpio_num);
		if (ret < 0) {
			sunxi_err(dev, "Could not get wupio index! Please check dts\n");
			return ret;
		}

		wupio_index = (u8)ret;
		sbi_andes_set_wakeup_wakeup_io_num(wupio_index);
	}

	return 0;
}

static int sunxi_wupio_resume(struct device *dev)
{
	return 0;
}

static struct dev_pm_ops wupio_pm_ops = {
	.suspend_noirq = sunxi_wupio_suspend,
	.resume_noirq = sunxi_wupio_resume,
};
#endif /* CONFIG_PM */

static void sunxi_wupio_shutdown(struct platform_device *pdev)
{

}

static struct platform_driver sunxi_wupio_driver = {
	.probe    = sunxi_wupio_probe,
	.remove   = sunxi_wupio_remove,
	.shutdown = sunxi_wupio_shutdown,
	.driver   = {
		.name  = "sunxi_wupio",
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_PM)
		.pm = &wupio_pm_ops,
#endif
		.of_match_table = sunxi_wupio_dt_ids,
	},
};

static int __init sunxi_wupio_init(void)
{
	int err;

	err = platform_driver_register(&sunxi_wupio_driver);
	if (err)
		sunxi_err(NULL, "register sunxi wupio failed\n");

	return err;
}
module_init(sunxi_wupio_init);

static void __exit sunxi_wupio_exit(void)
{
	platform_driver_unregister(&sunxi_wupio_driver);
}
module_exit(sunxi_wupio_exit);

MODULE_DESCRIPTION("sunxi wupio driver");
MODULE_AUTHOR("luwinkey <luwinkey@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
