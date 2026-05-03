// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun300iw1_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
/* Pin banks are: A B C D */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),         /* sd0*/
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),         /* sd0*/
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),         /* sd0*/
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),         /* sd0*/
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),         /* sd0*/
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),         /* sd0*/
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),         /* spi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),         /* spi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),         /* spi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),         /* spi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),         /* spi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),         /* spi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION(0x7, "twi0"),         /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION(0x7, "twi0"),         /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION(0x7, "twi1"),         /* twi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION(0x7, "twi1"),         /* twi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION(0x7, "twi2"),         /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION(0x7, "twi2"),         /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "rmii0"),         /* rmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* uart1 */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "pwm2"),          /* pwm2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "pwm3"),          /* pwm3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "pwm4"),          /* pwm4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x6, "sd1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x6, "sd1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x6, "sd1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x6, "sd1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "pwm11"),          /* pwm11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x6, "sd1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x6, "sd1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

#else
	/* Pin banks are: A C D */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "csi"),           /* csi */
		SUNXI_FUNCTION(0x3, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "clk"),           /* clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipi"),          /* mipi */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x6, "pwm0_10"),       /* pwm0_10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipi"),          /* mipi */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x6, "pwm0_11"),       /* pwm0_11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "csi"),           /* csi */
		SUNXI_FUNCTION(0x3, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x6, "clk"),           /* clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "csi"),           /* csi */
		SUNXI_FUNCTION(0x3, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x6, "clk"),           /* clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia"),         /* mipia */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia"),         /* mipia */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia"),         /* mipia */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia"),         /* mipia */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib"),         /* mipia */
		SUNXI_FUNCTION(0x3, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib"),         /* mipia */
		SUNXI_FUNCTION(0x3, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib"),         /* mipib */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib"),         /* mipib */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x6, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x7, "dbg"),           /* dbg */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x5, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x6, "fem"),           /* fem */
		SUNXI_FUNCTION(0x7, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x5, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x6, "fem"),           /* fem */
		SUNXI_FUNCTION(0x7, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x8, "csi"),           /* csi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "boot"),          /* boot */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x7, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x7, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x6, "pwm0_10"),       /* pwm0_10 */
		SUNXI_FUNCTION(0x7, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "boot"),          /* boot */
		SUNXI_FUNCTION(0x5, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x6, "pwm0_11"),       /* pwm0_11 */
		SUNXI_FUNCTION(0x7, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x8, "test"),           /* test */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x8, "test"),           /* test */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x4, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x3, "pwm0_11"),       /* pwm0_11 */
		SUNXI_FUNCTION(0x4, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "pwm0_10"),       /* pwm0_10 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0x7, "csi"),           /* csi */
		SUNXI_FUNCTION(0x8, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xd, "debug0"),        /* debug0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xd, "debug1"),        /* debug1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xd, "debug2"),        /* debug2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xd, "debug3"),        /* debug3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x8, "spi1_hold"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xd, "debug4"),        /* debug4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x8, "spi1_wp"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xd, "debug5"),        /* debug5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x6, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x8, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x9, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0xa, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0xd, "debug6"),        /* debug6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x6, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x8, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x9, "i2s0_dout1"),    /* i2s0_dout1 */
		SUNXI_FUNCTION(0xa, "i2s0_din1"),     /* i2s0_din1 */
		SUNXI_FUNCTION(0xd, "debug7"),        /* debug7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x6, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x8, "lcds_ctl"),           /* lcd */
		SUNXI_FUNCTION(0x9, "i2s0_dout2"),    /* i2s0_dout2 */
		SUNXI_FUNCTION(0xa, "i2s0_din2"),     /* i2s0_din2 */
		SUNXI_FUNCTION(0xd, "debug8"),        /* debug8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x6, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x8, "lcds_ctl"),           /* lcd */
		SUNXI_FUNCTION(0x9, "i2s0_dout3"),    /* i2s0_dout3 */
		SUNXI_FUNCTION(0xa, "i2s0_din3"),     /* i2s0_din3 */
		SUNXI_FUNCTION(0xd, "debug9"),        /* debug9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "rmii"),          /* rmii */
		SUNXI_FUNCTION(0x5, "pwm0_10"),       /* pwm0_10 */
		SUNXI_FUNCTION(0x6, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x7, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x8, "lcds_ctl"),           /* lcd */
		SUNXI_FUNCTION(0xd, "debug10"),       /* debug10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION(0x7, "clk"),           /* clk */
		SUNXI_FUNCTION(0x8, "lcds_ctl"),           /* lcd */
		SUNXI_FUNCTION(0xd, "debug11"),       /* debug11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION(0x7, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x8, "tcon"),          /* tcon */
		SUNXI_FUNCTION(0xc, "debug"),         /* debug */
		SUNXI_FUNCTION(0xd, "debug12"),       /* debug12 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x7, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xc, "debug"),         /* debug */
		SUNXI_FUNCTION(0xd, "debug13"),       /* debug13 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xc, "debug"),         /* debug */
		SUNXI_FUNCTION(0xd, "debug14"),       /* debug14 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "csi"),           /* csi */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0xc, "debug"),         /* debug */
		SUNXI_FUNCTION(0xd, "debug15"),       /* debug15 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x5, "pwm0_11"),       /* pwm0_11 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "fem"),           /* fem */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x5, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "fem"),           /* fem */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x5, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x4, "pwm0_10"),       /* pwm0_10 */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x8, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x9, "clk"),           /* clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "pwm0_11"),       /* pwm0_11 */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x5, "pwm0_10"),       /* pwm0_10 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "fem"),           /* fem */
		SUNXI_FUNCTION(0x8, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "tcon"),          /* tcon */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "rjtag"),             /* rjtag */
		SUNXI_FUNCTION(0x5, "pwm0_11"),       /* pwm0_11 */
		SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "fem"),           /* fem */
		SUNXI_FUNCTION(0x8, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#endif
};

static const unsigned int sun300iw1_irq_bank_map[] = {
/* TODO : update irq list */
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
};

static const struct sunxi_pinctrl_desc sun300iw1_pinctrl_data = {
	.pins = sun300iw1_pins,
	.npins = ARRAY_SIZE(sun300iw1_pins),
	.irq_banks = ARRAY_SIZE(sun300iw1_irq_bank_map),
	.irq_bank_map = sun300iw1_irq_bank_map,
	.auto_power_source_switch = true,
	.pf_power_source_switch = false,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

/* PINCTRL power management code */
#if IS_ENABLED(CONFIG_PM_SLEEP)

static void *mem;
static int mem_size;

static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	if (mem)
		return -EINVAL;

	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
	return 0;
}

static int sun300iw1_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	sunxi_info(dev, "pinctrl suspend\n");

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sun300iw1_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_toio(pctl->membase, mem, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	sunxi_info(dev, "pinctrl resume\n");

	return 0;
}

static const struct dev_pm_ops sun300iw1_pinctrl_pm_ops = {
	.suspend_noirq = sun300iw1_pinctrl_suspend_noirq,
	.resume_noirq = sun300iw1_pinctrl_resume_noirq,
};
#define PINCTRL_PM_OPS	(&sun300iw1_pinctrl_pm_ops)

#else
static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	return 0;
}
#define PINCTRL_PM_OPS	NULL
#endif

static int sun300iw1_pinctrl_probe(struct platform_device *pdev)
{
	int ret;
	ret = pinctrl_pm_alloc_mem(pdev);
	if (ret) {
		sunxi_err(&pdev->dev, "alloc pm mem err\n");
		return ret;
	}
#if IS_ENABLED(CONFIG_PINCTRL_SUNXI_DEBUGFS)
	dev_set_name(&pdev->dev, "pio");
#endif
	return sunxi_bsp_pinctrl_init(pdev, &sun300iw1_pinctrl_data);
}

static struct of_device_id sun300iw1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun300iw1-pinctrl", },
	{}
};

MODULE_DEVICE_TABLE(of, sun300iw1_pinctrl_match);

static struct platform_driver sun300iw1_pinctrl_driver = {
	.probe	= sun300iw1_pinctrl_probe,
	.driver	= {
		.name		= "sun300iw1-pinctrl",
		.pm		= PINCTRL_PM_OPS,
		.of_match_table	= sun300iw1_pinctrl_match,
	},
};

static int __init sun300iw1_pio_init(void)
{
	return platform_driver_register(&sun300iw1_pinctrl_driver);
}
postcore_initcall(sun300iw1_pio_init);

MODULE_DESCRIPTION("Allwinner sun300iw1 pio pinctrl driver");
MODULE_AUTHOR("<rengaomin@allwinnertech>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.10");
