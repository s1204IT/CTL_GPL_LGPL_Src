// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2024 geyoupengs@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

#define SUNXI_R_PINCTRL_VERSION          "0.0.2"

static const struct sunxi_desc_pin sun300iw1_rtc_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
#else
	/* Pin banks are: L */

	/* bank L */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "csi"),           /* csi */
		SUNXI_FUNCTION(0x6, "mco"),           /* mco */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#endif
};

static const unsigned int sun300iw1_rtc_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
};

static const unsigned int sun300iw1_rtc_bank_base[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
};

static const struct sunxi_pinctrl_desc sun300iw1_rtc_pinctrl_data = {
	.pins = sun300iw1_rtc_pins,
	.npins = ARRAY_SIZE(sun300iw1_rtc_pins),
	.banks = ARRAY_SIZE(sun300iw1_rtc_bank_base),
	.bank_base = sun300iw1_rtc_bank_base,
	.irq_banks = ARRAY_SIZE(sun300iw1_rtc_irq_bank_map),
	.irq_bank_map = sun300iw1_rtc_irq_bank_map,
	.pin_base = SUNXI_PIN_BASE('L'),
	.auto_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_8,
};

static int sun300iw1_rtc_pinctrl_probe(struct platform_device *pdev)
{
	sunxi_info(NULL, "sunxi rtc-pinctrl version: %s\n", SUNXI_R_PINCTRL_VERSION);

	return sunxi_bsp_pinctrl_init(pdev, &sun300iw1_rtc_pinctrl_data);
}

static struct of_device_id sun300iw1_rtc_pinctrl_match[] = {
	{ .compatible = "allwinner,sun300iw1-rtc-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun300iw1_rtc_pinctrl_match);

static struct platform_driver sun300iw1_rtc_pinctrl_driver = {
	.probe	= sun300iw1_rtc_pinctrl_probe,
	.driver	= {
		.name		= "sun300iw1-r-pinctrl",
		.of_match_table	= sun300iw1_rtc_pinctrl_match,
	},
};

static int __init sun300iw1_rtc_pio_init(void)
{
	return platform_driver_register(&sun300iw1_rtc_pinctrl_driver);
}
postcore_initcall(sun300iw1_rtc_pio_init);

MODULE_DESCRIPTION("Allwinner sun300iw1 RTC_PIO pinctrl driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<geyoupengs@allwinnertech>");
MODULE_VERSION(SUNXI_R_PINCTRL_VERSION);
