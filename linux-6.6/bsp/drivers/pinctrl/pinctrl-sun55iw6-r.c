// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2024 rengaomin@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

#define SUNXI_R_PINCTRL_VERSION          "0.8.3"

static const struct sunxi_desc_pin sun55iw6_r_pins[] = {
	/* Pin banks are: L M */

	/* bank L */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),        /* s_twi0_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),        /* s_twi0_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),       /* s_uart0_tx */
		SUNXI_FUNCTION(0x3, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x4, "s_pwm0_2"),      /* s_pwm0_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),       /* s_uart0_rx */
		SUNXI_FUNCTION(0x4, "s_pwm0_3"),      /* s_pwm0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_ms"),     /* s_jtag_ms */
		SUNXI_FUNCTION(0x3, "s_uart1"),    /* s_uart1_tx */
		SUNXI_FUNCTION(0x4, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_ck"),     /* s_jtag_ck */
		SUNXI_FUNCTION(0x3, "s_uart1"),    /* s_uart1_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_do"),     /* s_jtag_do */
		SUNXI_FUNCTION(0x3, "s_uart1"),   /* s_uart1_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_di"),     /* s_jtag_di */
		SUNXI_FUNCTION(0x3, "s_uart1"),   /* s_uart1_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),        /* s_twi1_sck */
		SUNXI_FUNCTION(0x4, "s_rjtag_ms"),    /* s_rjtag_ms */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),        /* s_twi1_sda */
		SUNXI_FUNCTION(0x4, "s_rjtag_ck"),    /* s_rjtag_ck */
		SUNXI_FUNCTION(0x5, "s_pwm0_1"),      /* s_pwm0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_pwm0_0"),      /* s_pwm0_0 */
		SUNXI_FUNCTION(0x3, "s_uart0"),       /* s_uart0_tx */
		SUNXI_FUNCTION(0x6, "s_spi0"),    /* s_spi0_cs0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x3, "s_uart0"),       /* s_uart0_rx */
		SUNXI_FUNCTION(0x6, "s_spi0"),    /* s_spi0_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_pwm0_1"),      /* s_pwm0_1 */
		SUNXI_FUNCTION(0x3, "s_uart0"),       /* s_uart0_rts */
		SUNXI_FUNCTION(0x6, "s_spi0"),   /* s_spi0_mosi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_pwm0_3"),      /* s_pwm0_3 */
		SUNXI_FUNCTION(0x3, "s_uart0"),       /* s_uart0_cts */
		SUNXI_FUNCTION(0x6, "s_spi0"),   /* s_spi0_miso */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank M */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),        /* s_twi0_sck */
		SUNXI_FUNCTION(0x3, "s_uart1"),    /* s_uart1_tx */
		SUNXI_FUNCTION(0x4, "s_pwm0_0"),      /* s_pwm0_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),        /* s_twi0_sda */
		SUNXI_FUNCTION(0x3, "s_uart1"),    /* s_uart1_rx */
		SUNXI_FUNCTION(0x4, "s_pwm0_1"),      /* s_pwm0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),        /* s_twi1_sck */
		SUNXI_FUNCTION(0x3, "s_uart1"),   /* s_uart1_rts */
		SUNXI_FUNCTION(0x4, "s_uart0"),       /* s_uart0_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),        /* s_twi1_sda */
		SUNXI_FUNCTION(0x3, "s_uart1"),   /* s_uart1_cts */
		SUNXI_FUNCTION(0x4, "s_uart0"),       /* s_uart0_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "s_pwm0_2"),      /* s_pwm0_2 */
		SUNXI_FUNCTION(0x4, "s_uart0"),       /* s_uart0_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x3, "s_pwm0_3"),      /* s_pwm0_3 */
		SUNXI_FUNCTION(0x4, "s_uart0"),       /* s_uart0_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
};

static const unsigned int sun55iw6_r_irq_bank_map[] = {
#if IS_ENABLED(CONFIG_HYPER_GUEST_OS)
#else
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
#endif
};

static const struct sunxi_pinctrl_desc sun55iw6_r_pinctrl_data = {
	.pins = sun55iw6_r_pins,
	.npins = ARRAY_SIZE(sun55iw6_r_pins),
	.irq_banks = ARRAY_SIZE(sun55iw6_r_irq_bank_map),
	.irq_bank_map = sun55iw6_r_irq_bank_map,
	.pin_base = SUNXI_PIN_BASE('L'),
	.auto_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_7,
};

static int sun55iw6_r_pinctrl_probe(struct platform_device *pdev)
{
	sunxi_info(NULL, "sunxi r-pinctrl version: %s\n", SUNXI_R_PINCTRL_VERSION);

	return sunxi_bsp_pinctrl_init(pdev, &sun55iw6_r_pinctrl_data);
}

static struct of_device_id sun55iw6_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun55iw6-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun55iw6_r_pinctrl_match);

static struct platform_driver sun55iw6_r_pinctrl_driver = {
	.probe	= sun55iw6_r_pinctrl_probe,
	.driver	= {
		.name		= "sun55iw6-r-pinctrl",
		.of_match_table	= sun55iw6_r_pinctrl_match,
	},
};

static int __init sun55iw6_r_pio_init(void)
{
	return platform_driver_register(&sun55iw6_r_pinctrl_driver);
}
postcore_initcall(sun55iw6_r_pio_init);

MODULE_DESCRIPTION("Allwinner sun55iw6 R_PIO pinctrl driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<rengaomin@allwinnertech>");
MODULE_VERSION(SUNXI_R_PINCTRL_VERSION);
