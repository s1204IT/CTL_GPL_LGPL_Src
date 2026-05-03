// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 panzhijian@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

#include "pinctrl-sunxi.h"

#define SUNXI_PINCTRL_VERSION	"0.9.0"

static const struct sunxi_desc_pin sun55iw6_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_S4) || IS_ENABLED(CONFIG_AW_FPGA_V7)
/* Pin banks are: A B C D F G */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clkrx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxdv_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_mdc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_md_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txen_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "cpus_twi0"),     /* cpus_twi0_scl_di */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "cpus_twi0"),     /* cpus_twi0_sda_di */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_sda_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "cpus_uart0"),    /* cpus_uart0_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "cpus_uart0"),    /* cpus_uart0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "cpus_fir"),      /* cpus_fir_di */
		SUNXI_FUNCTION(0x3, "ir"),            /* ir_rx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_bclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_lrc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_mclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_ale_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_cen_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_ren_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_rb_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dq_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dq_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_wpn_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_re_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dqsn_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[0] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[0] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[1] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[1] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[2] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[2] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[3] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[3] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[4] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[4] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[5] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[0] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[5] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[6] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[1] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[7] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[2] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[8] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[3] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[8] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[9] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[4] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[9] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[10] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[10] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[11] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[11] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[12] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[12] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[13] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[13] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[14] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[14] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[15] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[15] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[16] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[16] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[17] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[17] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[18] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[18] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[19] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[19] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[20] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[20] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[21] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[21] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[22] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[22] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[23] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[23] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[24] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[24] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[25] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[25] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[26] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[26] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[27] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb0_do[27] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_ccmd_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_clk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dqs_oe */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[5] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_cle_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_mosi_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_rb_di[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_ss_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_hold_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_cen_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_miso_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[1] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_wp_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_wen_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_sck_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 31),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir_tx_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d3_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d2_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d1_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#else
	/* Pin banks are: A B C D E F G H I J K */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),      /* gpadc0_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2_tx */
		SUNXI_FUNCTION(0x3, "spi4"),      /* spi4_cs0 */
		SUNXI_FUNCTION(0x4, "jtag"),/* jtag_ms<jtag_sel,0> */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x6, "test"),          /* test */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_ms */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2_rx */
		SUNXI_FUNCTION(0x3, "spi4"),      /* spi4_clk */
		SUNXI_FUNCTION(0x4, "jtag"),/* jtag_ck<jtag_sel,0> */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x6, "test"),          /* test */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_ck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),     /* uart2 */
		SUNXI_FUNCTION(0x3, "spi4"),     /* spi4_mosi */
		SUNXI_FUNCTION(0x4, "jtag"),/* jtag_do<jtag_sel,0> */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),     /* uart2 */
		SUNXI_FUNCTION(0x3, "spi4"),     /* spi4_miso */
		SUNXI_FUNCTION(0x4, "jtag"),/* jtag_di<jtag_sel,0> */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "i2s2_mclk"),     /* i2s2_mclk */
		SUNXI_FUNCTION(0x4, "trace_data2"),   /* trace_data2 */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x6, "ir_rx0"),        /* ir_rx0 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION(0x3, "i2s2_bclk"),     /* i2s2_bclk */
		SUNXI_FUNCTION(0x4, "trace_clk"),     /* trace_clk */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x6, "ir_rx1"),        /* ir_rx1 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "i2s2_lrck"),     /* i2s2_lrck */
		SUNXI_FUNCTION(0x4, "trace_data0"),   /* trace_data0 */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION(0x6, "ir_tx"),         /* ir_tx */
		SUNXI_FUNCTION(0x7, "uart8"),     /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa_in"),        /* owa_in */
		SUNXI_FUNCTION(0x3, "i2s2_dout0"),    /* i2s2_dout0 */
		SUNXI_FUNCTION(0x4, "i2s2_din1"),     /* i2s2_din1 */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x7, "uart8"),     /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa_out"),       /* owa_out */
		SUNXI_FUNCTION(0x3, "i2s2_din0"),     /* i2s2_din0 */
		SUNXI_FUNCTION(0x4, "i2s2_dout1"),    /* i2s2_dout1 */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x7, "uart8"),     /* uart8_dcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_tx */
		SUNXI_FUNCTION(0x3, "i2s2_din2"),     /* i2s2_din2 */
		SUNXI_FUNCTION(0x4, "trace_data1"),   /* trace_data1 */
		SUNXI_FUNCTION(0x5, "i2s2_dout2"),    /* i2s2_dout2 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "uart8"),     /* uart8_dsr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_rx */
		SUNXI_FUNCTION(0x3, "i2s2_din3"),     /* i2s2_din3 */
		SUNXI_FUNCTION(0x4, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x5, "i2s2_dout3"),    /* i2s2_dout3 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0_sda */
		SUNXI_FUNCTION(0x7, "uart8"),     /* uart8_dtr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "spi1"),      /* spi1_cs0 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "watchdog_sig"),  /* watchdog_sig */
		SUNXI_FUNCTION(0x7, "uart8"),      /* uart8_ri */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi5"),          /* twi5_sda */
		SUNXI_FUNCTION(0x3, "uart7"),     /* uart7 */
		SUNXI_FUNCTION(0x4, "spi1"),      /* spi1_clk */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x6, "pll_lock_dbg"),  /* pll_lock_dbg */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),      /* twi4 */
		SUNXI_FUNCTION(0x3, "uart7"),      /* uart7_tx */
		SUNXI_FUNCTION(0x4, "spi1"),     /* spi1_mosi */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x6, "ir_rx2"),        /* ir_rx2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4_sda */
		SUNXI_FUNCTION(0x3, "uart7"),      /* uart7_rx */
		SUNXI_FUNCTION(0x4, "spi1"),     /* spi1_miso */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "ir_rx3"),        /* ir_rx3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),       /* nand_we */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_ds */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_ale */
		SUNXI_FUNCTION(0x3, "sdc2"),      /* sdc2_rst */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_cle */
		SUNXI_FUNCTION(0x4, "spi0"),     /* spi0_mosi */
		SUNXI_FUNCTION(0x5, "spif"),     /* spif_mosi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_ce1 */
		SUNXI_FUNCTION(0x4, "spi0"),      /* spi0_cs0 */
		SUNXI_FUNCTION(0x5, "spif"),      /* spif_cs0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_ce0 */
		SUNXI_FUNCTION(0x4, "spi0"),     /* spi0_miso */
		SUNXI_FUNCTION(0x5, "spif"),     /* spif_miso */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),       /* nand_re */
		SUNXI_FUNCTION(0x3, "sdc2"),      /* sdc2_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_rb0 */
		SUNXI_FUNCTION(0x3, "sdc2"),      /* sdc2_cmd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_rb1 */
		SUNXI_FUNCTION(0x4, "spi0"),      /* spi0_cs1 */
		SUNXI_FUNCTION(0x5, "spif"),      /* spif_dqs */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq7 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d3 */
		SUNXI_FUNCTION(0x5, "spif"),       /* spif_d7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq6 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d4 */
		SUNXI_FUNCTION(0x5, "spif"),       /* spif_d6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq5 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d0 */
		SUNXI_FUNCTION(0x5, "spif"),       /* spif_d5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq4 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d5 */
		SUNXI_FUNCTION(0x5, "spif"),       /* spif_d4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dqs */
		SUNXI_FUNCTION(0x4, "spi0"),      /* spi0_clk */
		SUNXI_FUNCTION(0x5, "spif"),      /* spif_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq3 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq2 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq1 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d2 */
		SUNXI_FUNCTION(0x4, "spi0"),       /* spi0_wp */
		SUNXI_FUNCTION(0x5, "spif"),       /* spif_wp */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),      /* nand_dq0 */
		SUNXI_FUNCTION(0x3, "sdc2"),       /* sdc2_d7 */
		SUNXI_FUNCTION(0x4, "spi0"),     /* spi0_hold */
		SUNXI_FUNCTION(0x5, "spif"),     /* spif_hold */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d2 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d0p */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d0p */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x7, "uart9"),      /* uart9_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d3 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d0n */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d0n */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x7, "uart9"),      /* uart9_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d4 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d1p */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d1p */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d5 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d1n */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d1n */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d6 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d2p */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_ckp */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x7, "uart10"),        /* uart10_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d7 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d2n */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_ckn */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x7, "uart10"),        /* uart10_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d10 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_ckp */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d2p */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x7, "uart10"),        /* uart10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d11 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_ckn */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d2n */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x7, "uart10"),        /* uart10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d12 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d3p */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d3p */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d13 */
		SUNXI_FUNCTION(0x3, "lvds0"),     /* lvds0_d3n */
		SUNXI_FUNCTION(0x4, "dsi"),       /* dsi_d3n */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d14 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d0p */
		SUNXI_FUNCTION(0x4, "spi1"),/* spi1_cs0<dbi_csx> */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION(0x6, "twi6"),          /* twi6 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d15 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d0n */
		SUNXI_FUNCTION(0x4, "spi1"),/* spi1_clk<dbi_sclk> */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x6, "twi6"),          /* twi6_sda */
		SUNXI_FUNCTION(0x7, "uart8"),          /* uart8_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d18 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d1p */
		SUNXI_FUNCTION(0x4, "spi1"),/* spi1_mosi<dbi_sdo> */
		SUNXI_FUNCTION(0x5, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d19 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d1n */
		SUNXI_FUNCTION(0x4, "spi1"),/* spi1_miso<dbi_sdi/dbi_te/dbi_dcx> */
		SUNXI_FUNCTION(0x5, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d20 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d2p */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1_hold<dbi_dcx/dbi_wrx> */
		SUNXI_FUNCTION(0x5, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3_tx */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8_dcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd_d21 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d2n */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1_wp<dbi_te> */
		SUNXI_FUNCTION(0x5, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3_rx */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8_dsr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),       /* lcd_d22 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_ckp */
		SUNXI_FUNCTION(0x5, "pwm1_6"),        /* pwm1_6 */
		SUNXI_FUNCTION(0x6, "uart3"),     /* uart3 */
		SUNXI_FUNCTION(0x7, "uart8"),     /* uart8_dtr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),       /* lcd_d23 */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_ckn */
		SUNXI_FUNCTION(0x5, "pwm1_7"),        /* pwm1_7 */
		SUNXI_FUNCTION(0x6, "uart3"),     /* uart3 */
		SUNXI_FUNCTION(0x7, "uart8"),      /* uart8_ri */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),       /* lcd_clk */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d3p */
		SUNXI_FUNCTION(0x4, "twi6"),      /* twi6 */
		SUNXI_FUNCTION(0x5, "pwm1_8"),        /* pwm1_8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),        /* lcd_de */
		SUNXI_FUNCTION(0x3, "lvds1"),     /* lvds1_d3n */
		SUNXI_FUNCTION(0x4, "twi6"),          /* twi6_sda */
		SUNXI_FUNCTION(0x5, "pwm1_9"),        /* pwm1_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),     /* lcd_hsync */
		SUNXI_FUNCTION(0x3, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2_tx */
		SUNXI_FUNCTION(0x5, "twi5"),          /* twi5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),     /* lcd_vsync */
		SUNXI_FUNCTION(0x3, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2_rx */
		SUNXI_FUNCTION(0x5, "twi5"),          /* twi5_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "twi4"),          /* twi4_sda */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank E */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi0_mclk"),    /* mcsi0_mclk */
		SUNXI_FUNCTION(0x6, "spi3"),      /* spi3_cs0 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_cs0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4_tx */
		SUNXI_FUNCTION(0x6, "spi3"),      /* spi3_clk */
		SUNXI_FUNCTION(0x7, "lbus_cs1"),      /* lbus_cs1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2_sda */
		SUNXI_FUNCTION(0x3, "uart4"),      /* uart4_rx */
		SUNXI_FUNCTION(0x7, "lbus_cs2"),      /* lbus_cs2 */
		SUNXI_FUNCTION(0x6, "spi3"),     /* spi3_mosi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "csi0_xhs"),      /* csi0_xhs */
		SUNXI_FUNCTION(0x6, "spi3"),     /* spi3_miso */
		SUNXI_FUNCTION(0x7, "lbus_cs3"),      /* lbus_cs3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3_sda */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "csi1_xhs"),      /* csi1_xhs */
		SUNXI_FUNCTION(0x6, "spi3"),       /* spi3_wp */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_dp0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi1_mclk"),    /* mcsi1_mclk */
		SUNXI_FUNCTION(0x3, "pll_lock_dbg"),  /* pll_lock_dbg */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x6, "spi3"),     /* spi3_hold */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_dp1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart12"),        /* uart12_tx */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "tcon_fsync0<tcon_lcd0>"),/* tcon_fsync0<tcon_lcd0> */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_dp2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart12"),        /* uart12_rx */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2_sda */
		SUNXI_FUNCTION(0x5, "clk_fanout0"),   /* clk_fanout0 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_dp3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart12"),        /* uart12 */
		SUNXI_FUNCTION(0x4, "ncsi_d0"),       /* ncsi_d0 */
		SUNXI_FUNCTION(0x5, "clk_fanout1"),   /* clk_fanout1 */
		SUNXI_FUNCTION(0x7, "lbus"),       /* lbus_wr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart12"),        /* uart12 */
		SUNXI_FUNCTION(0x4, "ncsi_d1"),       /* ncsi_d1 */
		SUNXI_FUNCTION(0x6, "spi2"),      /* spi2_cs0 */
		SUNXI_FUNCTION(0x7, "lbus"),    /* lbus_ready */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi3_mclk"),    /* mcsi3_mclk */
		SUNXI_FUNCTION(0x3, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x4, "ncsi_d2"),       /* ncsi_d2 */
		SUNXI_FUNCTION(0x6, "spi2"),      /* spi2_clk */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ale */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ncsi_d3"),       /* ncsi_d3 */
		SUNXI_FUNCTION(0x5, "uart6"),      /* uart6_tx */
		SUNXI_FUNCTION(0x6, "spi2"),     /* spi2_mosi */
		SUNXI_FUNCTION(0x7, "lbus"),   /* lbus_burst0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ncsi_d4"),       /* ncsi_d4 */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6_rx */
		SUNXI_FUNCTION(0x6, "spi2"),     /* spi2_miso */
		SUNXI_FUNCTION(0x7, "lbus"),   /* lbus_burst1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5_tx */
		SUNXI_FUNCTION(0x4, "ncsi_d5"),       /* ncsi_d5 */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x6, "spi2"),       /* spi2_wp */
		SUNXI_FUNCTION(0x7, "lbus"),   /* lbus_burst2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4_sda */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5_rx */
		SUNXI_FUNCTION(0x4, "ncsi_d6"),       /* ncsi_d6 */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x6, "spi2"),     /* spi2_hold */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_lclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi2_mclk"),    /* mcsi2_mclk */
		SUNXI_FUNCTION(0x3, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x4, "ncsi_d7"),       /* ncsi_d7 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_intr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi6"),      /* twi6 */
		SUNXI_FUNCTION(0x3, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x4, "csi0_xvs_fsync"),/* csi0_xvs_fsync */
		SUNXI_FUNCTION(0x5, "twi0"),      /* twi0 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_drq */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi6"),      	  /* twi6_sda */
		SUNXI_FUNCTION(0x3, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x4, "csi1_xvs_fsync"),/* csi1_xvs_fsync */
		SUNXI_FUNCTION(0x5, "twi0"),      	  /* twi0_sda */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_lbe */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),       /* sdc0_d1 */
		SUNXI_FUNCTION(0x3, "jtag"),/* jtag_ms<jtag_sel,1> */
		SUNXI_FUNCTION(0x4, "trace_data2"),   /* trace_data2 */
		SUNXI_FUNCTION(0x5, "i2s3_din0"),     /* i2s3_din0 */
		SUNXI_FUNCTION(0x6, "i2s3_dout1"),    /* i2s3_dout1 */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_ms */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),       /* sdc0_d0 */
		SUNXI_FUNCTION(0x3, "jtag"),/* jtag_di<jtag_sel,1> */
		SUNXI_FUNCTION(0x4, "trace_data1"),   /* trace_data1 */
		SUNXI_FUNCTION(0x5, "i2s3_dout0"),    /* i2s3_dout0 */
		SUNXI_FUNCTION(0x6, "i2s3_din1"),     /* i2s3_din1 */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),      /* sdc0_clk */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0_tx */
		SUNXI_FUNCTION(0x5, "i2s3_din2"),     /* i2s3_din2 */
		SUNXI_FUNCTION(0x6, "spi4"),      /* spi4_clk */
		SUNXI_FUNCTION(0x7, "i2s3_dout2"),    /* i2s3_dout2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),      /* sdc0_cmd */
		SUNXI_FUNCTION(0x3, "jtag"),/* jtag_do<jtag_sel,1> */
		SUNXI_FUNCTION(0x4, "trace_data0"),   /* trace_data0 */
		SUNXI_FUNCTION(0x5, "i2s3_lrck"),     /* i2s3_lrck */
		SUNXI_FUNCTION(0x6, "spi4"),      /* spi4_cs0 */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),       /* sdc0_d3 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0_rx */
		SUNXI_FUNCTION(0x5, "i2s3_din3"),     /* i2s3_din3 */
		SUNXI_FUNCTION(0x6, "spi4"),     /* spi4_mosi */
		SUNXI_FUNCTION(0x7, "i2s3_dout3"),    /* i2s3_dout3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),       /* sdc0_d2 */
		SUNXI_FUNCTION(0x3, "jtag"),/* jtag_ck<jtag_sel,1> */
		SUNXI_FUNCTION(0x4, "trace_clk"),     /* trace_clk */
		SUNXI_FUNCTION(0x5, "i2s3_bclk"),     /* i2s3_bclk */
		SUNXI_FUNCTION(0x6, "spi4"),     /* spi4_miso */
		SUNXI_FUNCTION(0x7, "rjtag"),      /* rjtag_ck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "i2s3_mclk"),     /* i2s3_mclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),      /* sdc1_clk */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),      /* sdc1_cmd */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),       /* sdc1_d0 */
		SUNXI_FUNCTION(0x3, "pcie_perstn"),   /* pcie_perstn */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),       /* sdc1_d1 */
		SUNXI_FUNCTION(0x3, "pcie_waken"),    /* pcie_waken */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),       /* sdc1_d2 */
		SUNXI_FUNCTION(0x3, "pcie_clkreqn"),  /* pcie_clkreqn */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),       /* sdc1_d3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1_tx */
		SUNXI_FUNCTION(0x3, "twi6"),      /* twi6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1_rx */
		SUNXI_FUNCTION(0x3, "twi6"),      /* twi6_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi5"),      /* twi5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi5"),      	  /* twi5_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart14"),     /* uart14_tx */
		SUNXI_FUNCTION(0x3, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0x4, "twi2"),      /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart14"),        /* uart14_rx */
		SUNXI_FUNCTION(0x3, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0x4, "twi2"),      /* twi2_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart14"),        /* uart14 */
		SUNXI_FUNCTION(0x3, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart14"),        /* uart14 */
		SUNXI_FUNCTION(0x3, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0x4, "i2s1_din1"),     /* i2s1_din1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0x4, "i2s1_dout1"),    /* i2s1_dout1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank H */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),      /* twi0 */
		SUNXI_FUNCTION(0x3, "pwm2_4"),        /* pwm2_4 */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1_tx */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld24 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),      	  /* twi0_sda */
		SUNXI_FUNCTION(0x3, "pwm2_5"),        /* pwm2_5 */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1_rx */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld25 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),      /* twi1 */
		SUNXI_FUNCTION(0x3, "cpu_cur_w"),     /* cpu_cur_w */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "clk_fanout0"),   /* clk_fanout0 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld26 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),      /* twi1_sda */
		SUNXI_FUNCTION(0x3, "ir_tx"),         /* ir_tx */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "clk_fanout1"),   /* clk_fanout1 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld27 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3_tx */
		SUNXI_FUNCTION(0x3, "spi3"),      /* spi3_cs0 */
		SUNXI_FUNCTION(0x4, "cpu_cur_w"),     /* cpu_cur_w */
		SUNXI_FUNCTION(0x5, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic_clk */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld28 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3_rx */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3_clk */
		SUNXI_FUNCTION(0x4, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x5, "pcie_perstn"),   /* pcie_perstn */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic_data0 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld29 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "spi3"),     /* spi3_mosi */
		SUNXI_FUNCTION(0x4, "twi6"),          /* twi6 */
		SUNXI_FUNCTION(0x5, "pcie_waken"),    /* pcie_waken */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic_data1 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld30 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3_miso */
		SUNXI_FUNCTION(0x4, "twi6"),          /* twi6_sda */
		SUNXI_FUNCTION(0x5, "pcie_clkreqn"),  /* pcie_clkreqn */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic_data2 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld31 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_6"),        /* pwm2_6 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3_wp */
		SUNXI_FUNCTION(0x4, "owa_in"),        /* owa_in */
		SUNXI_FUNCTION(0x5, "lcd"),           /* lcd_d0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic_data3 */
		SUNXI_FUNCTION(0x7, "twi3"),      /* twi3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_7"),        /* pwm2_7 */
		SUNXI_FUNCTION(0x3, "spi3"),     /* spi3_hold */
		SUNXI_FUNCTION(0x4, "i2s3_mclk"),     /* i2s3_mclk */
		SUNXI_FUNCTION(0x5, "lcd"),        /* lcd_d1 */
		SUNXI_FUNCTION(0x7, "twi3"),      /* twi3_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_8"),        /* pwm2_8 */
		SUNXI_FUNCTION(0x4, "i2s3_bclk"),     /* i2s3_bclk */
		SUNXI_FUNCTION(0x5, "lcd"),        /* lcd_d8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir_rx3"),        /* ir_rx3 */
		SUNXI_FUNCTION(0x3, "pwm2_9"),        /* pwm2_9 */
		SUNXI_FUNCTION(0x4, "i2s3_lrck"),     /* i2s3_lrck */
		SUNXI_FUNCTION(0x5, "lcd"),        /* lcd_d9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir_rx1"),        /* ir_rx1 */
		SUNXI_FUNCTION(0x3, "i2s3_dout0"),    /* i2s3_dout0 */
		SUNXI_FUNCTION(0x4, "i2s3_din1"),     /* i2s3_din1 */
		SUNXI_FUNCTION(0x5, "lcd"),       /* lcd_d16 */
		SUNXI_FUNCTION(0x7, "uart11"),        /* uart11_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir_rx2"),        /* ir_rx2 */
		SUNXI_FUNCTION(0x3, "i2s3_dout1"),    /* i2s3_dout1 */
		SUNXI_FUNCTION(0x4, "i2s3_din0"),     /* i2s3_din0 */
		SUNXI_FUNCTION(0x5, "lcd"),       /* lcd_d17 */
		SUNXI_FUNCTION(0x7, "uart11"),        /* uart11_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir_tx"),         /* ir_tx */
		SUNXI_FUNCTION(0x3, "i2s3_dout2"),    /* i2s3_dout2 */
		SUNXI_FUNCTION(0x4, "i2s3_din2"),     /* i2s3_din2 */
		SUNXI_FUNCTION(0x5, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION(0x7, "uart11"),        /* uart11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir_rx0"),        /* ir_rx0 */
		SUNXI_FUNCTION(0x3, "i2s3_dout3"),    /* i2s3_dout3 */
		SUNXI_FUNCTION(0x4, "i2s3_din3"),     /* i2s3_din3 */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x7, "uart11"),        /* uart11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank I */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x3, "uart9"),         /* uart9_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x3, "uart9"),         /* uart9_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x3, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi5"),          /* twi5_sda */
		SUNXI_FUNCTION(0x3, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6_tx */
		SUNXI_FUNCTION(0x3, "spi2"),      /* spi2_cs0 */
		SUNXI_FUNCTION(0x4, "pwm2_0"),        /* pwm2_0 */
		SUNXI_FUNCTION(0x5, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x6, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x7, "clk_fanout0"),   /* clk_fanout0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6_rx */
		SUNXI_FUNCTION(0x3, "spi2"),      /* spi2_clk */
		SUNXI_FUNCTION(0x4, "pwm2_1"),        /* pwm2_1 */
		SUNXI_FUNCTION(0x5, "i2s0_din2"),     /* i2s0_din2 */
		SUNXI_FUNCTION(0x6, "i2s0_dout2"),    /* i2s0_dout2 */
		SUNXI_FUNCTION(0x7, "clk_fanout1"),   /* clk_fanout1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x3, "spi2"),     /* spi2_mosi */
		SUNXI_FUNCTION(0x4, "pwm2_2"),        /* pwm2_2 */
		SUNXI_FUNCTION(0x5, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x3, "spi2"),     /* spi2_miso */
		SUNXI_FUNCTION(0x4, "pwm2_3"),        /* pwm2_3 */
		SUNXI_FUNCTION(0x5, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION(0x7, "ir_tx"),         /* ir_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa_out"),       /* owa_out */
		SUNXI_FUNCTION(0x3, "spi2"),       /* spi2_wp */
		SUNXI_FUNCTION(0x4, "pwm2_7"),        /* pwm2_7 */
		SUNXI_FUNCTION(0x5, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0x6, "i2s0_din1"),     /* i2s0_din1 */
		SUNXI_FUNCTION(0x7, "pcie_perstn"),   /* pcie_perstn */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),      /* twi3 */
		SUNXI_FUNCTION(0x3, "spi2"),     /* spi2_hold */
		SUNXI_FUNCTION(0x4, "pwm2_8"),        /* pwm2_8 */
		SUNXI_FUNCTION(0x5, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x6, "i2s0_dout1"),    /* i2s0_dout1 */
		SUNXI_FUNCTION(0x7, "pcie_waken"),    /* pcie_waken */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),      /* twi3_sda */
		SUNXI_FUNCTION(0x3, "pwm2_4"),        /* pwm2_4 */
		SUNXI_FUNCTION(0x4, "dmic"),      /* dmic_clk */
		SUNXI_FUNCTION(0x5, "i2s0_din3"),     /* i2s0_din3 */
		SUNXI_FUNCTION(0x6, "i2s0_dout3"),    /* i2s0_dout3 */
		SUNXI_FUNCTION(0x7, "pcie_clkreqn"),  /* pcie_clkreqn */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4_tx */
		SUNXI_FUNCTION(0x3, "pwm2_5"),        /* pwm2_5 */
		SUNXI_FUNCTION(0x4, "dmic"),    /* dmic_data0 */
		SUNXI_FUNCTION(0x6, "ir_rx0"),        /* ir_rx0 */
		SUNXI_FUNCTION(0x7, "i2s3_bclk"),     /* i2s3_bclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4_rx */
		SUNXI_FUNCTION(0x3, "pwm2_6"),        /* pwm2_6 */
		SUNXI_FUNCTION(0x4, "dmic"),    /* dmic_data1 */
		SUNXI_FUNCTION(0x6, "ir_rx1"),        /* ir_rx1 */
		SUNXI_FUNCTION(0x7, "i2s3_lrck"),     /* i2s3_lrck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "dmic"),    /* dmic_data2 */
		SUNXI_FUNCTION(0x6, "ir_rx2"),        /* ir_rx2 */
		SUNXI_FUNCTION(0x7, "i2s3_din0"),     /* i2s3_din0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2_sda */
		SUNXI_FUNCTION(0x4, "dmic"),    /* dmic_data3 */
		SUNXI_FUNCTION(0x6, "ir_rx3"),        /* ir_rx3 */
		SUNXI_FUNCTION(0x7, "i2s3_mclk"),     /* i2s3_mclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank J */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x3, "i2s2_mclk"),     /* i2s2_mclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_rxd1/rmii0_rxd1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x3, "i2s2_bclk"),     /* i2s2_bclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_rxd0/rmii0_rxd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x3, "i2s2_lrck"),     /* i2s2_lrck */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_rxctl/rmii0_crs_dv */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x3, "i2s2_dout0"),    /* i2s2_dout0 */
		SUNXI_FUNCTION(0x4, "i2s2_din1"),     /* i2s2_din1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_clkin/rmii0_rxer */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x3, "i2s2_din0"),     /* i2s2_din0 */
		SUNXI_FUNCTION(0x4, "i2s2_dout1"),    /* i2s2_dout1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_txd1/rmii0_txd1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x3, "i2s2_din2"),     /* i2s2_din2 */
		SUNXI_FUNCTION(0x4, "i2s2_dout2"),    /* i2s2_dout2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_txd0/rmii0_txd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x3, "i2s2_din3"),     /* i2s2_din3 */
		SUNXI_FUNCTION(0x4, "i2s2_dout3"),    /* i2s2_dout3 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_txck/rmii0_txck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x3, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_txctl/rmii0_txen */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x3, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),    /* rgmii0_mdc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x3, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION(0x5, "rgmii0"),   /* rgmii0_mdio */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_0"),        /* pwm2_0 */
		SUNXI_FUNCTION(0x3, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0x4, "i2s1_din1"),     /* i2s1_din1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_ephy_25/50m */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_1"),        /* pwm2_1 */
		SUNXI_FUNCTION(0x3, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0x4, "i2s1_dout1"),    /* i2s1_dout1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_rxd3/rmii0_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_2"),        /* pwm2_2 */
		SUNXI_FUNCTION(0x3, "uart10"),        /* uart10_tx */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_rxd2/rmii0_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_3"),        /* pwm2_3 */
		SUNXI_FUNCTION(0x3, "uart10"),        /* uart10_rx */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_rxck/rmii0_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_4"),        /* pwm2_4 */
		SUNXI_FUNCTION(0x3, "uart10"),        /* uart10 */
		SUNXI_FUNCTION(0x4, "twi6"),      /* twi6 */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_txd3/rmii0_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_5"),        /* pwm2_5 */
		SUNXI_FUNCTION(0x3, "uart10"),        /* uart10 */
		SUNXI_FUNCTION(0x4, "twi6"),      /* twi6_sda */
		SUNXI_FUNCTION(0x5, "rgmii0"),/* rgmii0_txd2/rmii0_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_6"),        /* pwm2_6 */
		SUNXI_FUNCTION(0x4, "ir_rx0"),        /* ir_rx0 */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_rxd1/rmii1_rxd1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_7"),        /* pwm2_7 */
		SUNXI_FUNCTION(0x4, "ir_rx1"),        /* ir_rx1 */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_rxd0/rmii1_rxd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_8"),        /* pwm2_8 */
		SUNXI_FUNCTION(0x4, "ir_rx2"),        /* ir_rx2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_rxctl/rmii1_crs_dv */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_9"),        /* pwm2_9 */
		SUNXI_FUNCTION(0x4, "ir_rx3"),        /* ir_rx3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_clkin/rmii1_rxer */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x3, "uart14"),        /* uart14_tx */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_txd1/rmii1_txd1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),          /* twi0_sda */
		SUNXI_FUNCTION(0x3, "uart14"),        /* uart14_rx */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_txd0/rmii1_txd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart14"),        /* uart14 */
		SUNXI_FUNCTION(0x4, "ir_tx"),         /* ir_tx */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_txck/rmii1_txck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart14"),        /* uart14 */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_txctl/rmii1_txen */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_0"),        /* pwm2_0 */
		SUNXI_FUNCTION(0x3, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13 */
		SUNXI_FUNCTION(0x5, "rgmii1"),    /* rgmii1_mdc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_1"),        /* pwm2_1 */
		SUNXI_FUNCTION(0x3, "twi4"),          /* twi4_sda */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13 */
		SUNXI_FUNCTION(0x5, "rgmii1"),   /* rgmii1_mdio */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_2"),        /* pwm2_2 */
		SUNXI_FUNCTION(0x3, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13_tx */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_ephy_25/50m */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_3"),        /* pwm2_3 */
		SUNXI_FUNCTION(0x3, "twi5"),          /* twi5_sda */
		SUNXI_FUNCTION(0x4, "uart13"),        /* uart13_rx */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_rxd3/rmii1_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_rxd2/rmii1_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_rxck/rmii1_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_txd3/rmii1_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "rgmii1"),/* rgmii1_txd2/rmii1_null */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 31),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank K */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),     /* mcsia_d0n */
		SUNXI_FUNCTION(0x4, "uart11"),        /* uart11_tx */
		SUNXI_FUNCTION(0x6, "spi2"),      /* spi2_cs0 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 0),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),     /* mcsia_d0p */
		SUNXI_FUNCTION(0x4, "uart11"),        /* uart11_rx */
		SUNXI_FUNCTION(0x6, "spi2"),      /* spi2_clk */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 1),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),     /* mcsia_d1n */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart11"),        /* uart11 */
		SUNXI_FUNCTION(0x6, "spi2"),     /* spi2_mosi */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 2),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),     /* mcsia_d1p */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2_sda */
		SUNXI_FUNCTION(0x4, "uart11"),        /* uart11 */
		SUNXI_FUNCTION(0x6, "spi2"),     /* spi2_miso */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 3),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),     /* mcsia_ckn */
		SUNXI_FUNCTION(0x4, "uart12"),        /* uart12_tx */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d0 */
		SUNXI_FUNCTION(0x6, "spi2"),       /* spi2_wp */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 4),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),     /* mcsia_ckp */
		SUNXI_FUNCTION(0x4, "uart12"),        /* uart12_rx */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d1 */
		SUNXI_FUNCTION(0x6, "spi2"),     /* spi2_hold */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 5),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),     /* mcsib_d0n */
		SUNXI_FUNCTION(0x4, "uart12"),        /* uart12 */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d2 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 6),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),     /* mcsib_d0p */
		SUNXI_FUNCTION(0x4, "uart12"),        /* uart12 */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d3 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 7),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),     /* mcsib_d1n */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d4 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 8),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),     /* mcsib_d1p */
		SUNXI_FUNCTION(0x3, "mcsi3_mclk"),    /* mcsi3_mclk */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d5 */
		SUNXI_FUNCTION(0x7, "lbus"),      /* lbus_ld9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 9),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),     /* mcsib_ckn */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d6 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 10), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),     /* mcsib_ckp */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d7 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 11), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),     /* mcsic_d0n */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7_tx */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d8 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld12 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 12), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),     /* mcsic_d0p */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7_rx */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION(0x5, "ncsi"),       /* ncsi_d9 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld13 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 13), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),     /* mcsic_d1n */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "ncsi"),      /* ncsi_d10 */
		SUNXI_FUNCTION(0x6, "spi3"),      /* spi3_cs0 */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld14 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 14), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),     /* mcsic_d1p */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "ncsi"),      /* ncsi_d11 */
		SUNXI_FUNCTION(0x6, "spi3"),      /* spi3_clk */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld15 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 15), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),     /* mcsic_ckn */
		SUNXI_FUNCTION(0x3, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5_tx */
		SUNXI_FUNCTION(0x5, "ncsi"),      /* ncsi_d12 */
		SUNXI_FUNCTION(0x6, "spi3"),     /* spi3_mosi */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld16 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 16), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),     /* mcsic_ckp */
		SUNXI_FUNCTION(0x3, "twi4"),          /* twi4_sda */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5_rx */
		SUNXI_FUNCTION(0x5, "ncsi"),      /* ncsi_d13 */
		SUNXI_FUNCTION(0x6, "spi3"),     /* spi3_miso */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld17 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 17), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsid"),     /* mcsid_d0n */
		SUNXI_FUNCTION(0x3, "mcsi0_mclk"),    /* mcsi0_mclk */
		SUNXI_FUNCTION(0x4, "uart6"),         /* uart6_tx */
		SUNXI_FUNCTION(0x5, "ncsi"),      /* ncsi_d14 */
		SUNXI_FUNCTION(0x6, "spi3"),       /* spi3_wp */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld18 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 18), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsid"),     /* mcsid_d0p */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart6"),         /* uart6_rx */
		SUNXI_FUNCTION(0x5, "ncsi"),      /* ncsi_d15 */
		SUNXI_FUNCTION(0x6, "spi3"),     /* spi3_hold */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld19 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 19), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsid"),     /* mcsid_d1n */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2_sda */
		SUNXI_FUNCTION(0x4, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x5, "ncsi"),     /* ncsi_pclk */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld20 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 20), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsid"),     /* mcsid_d1p */
		SUNXI_FUNCTION(0x3, "mcsi1_mclk"),    /* mcsi1_mclk */
		SUNXI_FUNCTION(0x4, "uart6"),        /* uart6 */
		SUNXI_FUNCTION(0x5, "ncsi"),     /* ncsi_mclk */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld21 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 21), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsid"),     /* mcsid_ckn */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x5, "ncsi"),    /* ncsi_hsync */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld22 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 22), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsid"),     /* mcsid_ckp */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3_sda */
		SUNXI_FUNCTION(0x4, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x5, "ncsi"),    /* ncsi_vsync */
		SUNXI_FUNCTION(0x7, "lbus"),     /* lbus_ld23 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 10, 23), /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#endif
};

static const unsigned int sun55iw6_bank_base[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
	SUNXI_BANK_OFFSET('I', 'A'),
	SUNXI_BANK_OFFSET('J', 'A'),
	SUNXI_BANK_OFFSET('K', 'A'),
};

static const unsigned int sun55iw6_irq_bank_map[] = {
#if IS_ENABLED(CONFIG_HYPER_GUEST_OS)
#else
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
	SUNXI_BANK_OFFSET('I', 'A'),
	SUNXI_BANK_OFFSET('J', 'A'),
	SUNXI_BANK_OFFSET('K', 'A'),
#endif
};

static const struct sunxi_pinctrl_desc sun55iw6_pinctrl_data = {
	.pins = sun55iw6_pins,
	.npins = ARRAY_SIZE(sun55iw6_pins),
	.banks = ARRAY_SIZE(sun55iw6_bank_base),
	.bank_base = sun55iw6_bank_base,
	.irq_banks = ARRAY_SIZE(sun55iw6_irq_bank_map),
	.irq_bank_map = sun55iw6_irq_bank_map,
	.pf_power_source_switch = false,
	.auto_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_4,
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

	if (mem) {
		sunxi_err(&pdev->dev, "pm mem has been allocated\n");
		return -EBUSY;
	}
	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
	return 0;
}

static int sun55iw6_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	sunxi_info(dev, "pinctrl suspend\n");

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sun55iw6_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl_desc const *desc = &sun55iw6_pinctrl_data;
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;
	int idx, bank;
	int initial_bank_offset = sunxi_pinctrl_hw_info[desc->hw_type].initial_bank_offset;
	int bank_mem_size = sunxi_pinctrl_hw_info[desc->hw_type].bank_mem_size;
	int irq_cfg_reg = sunxi_pinctrl_hw_info[desc->hw_type].irq_cfg_reg;
	int irq_mem_size = sunxi_pinctrl_hw_info[desc->hw_type].irq_mem_size;
	int irq_mem_used = sunxi_pinctrl_hw_info[desc->hw_type].irq_mem_used;
	int data_clr_regs_offset = sunxi_pinctrl_hw_info[desc->hw_type].data_clr_regs_offset;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	/* recover PX_REG_BASE and GPIO_POW_MOD/CTL/VAL */
	memcpy_toio(pctl->membase, mem, 0x80);

	/* clear data_clr_reg before recover data_reg to ensure data_reg correctness */
	for (idx = 0; idx < desc->banks; idx++) {
		bank = desc->bank_base[idx];
		*(unsigned int *)(mem + initial_bank_offset + bank * bank_mem_size + data_clr_regs_offset) = 0x0;
	}

	for (idx = 0; idx < desc->banks; idx++) {
		bank = desc->bank_base[idx];
		/* cfg/dat/drv/pull */
		memcpy_toio(pctl->membase + initial_bank_offset + bank * bank_mem_size,
			    mem + initial_bank_offset + bank * bank_mem_size,
			    0x40);
	}

	for (idx = 0; idx < desc->irq_banks; idx++) {
		bank = desc->irq_bank_map[idx];
		/* irq cfg/ctr/status/deb */
		memcpy_toio(pctl->membase + irq_cfg_reg + bank * irq_mem_size,
			    mem + irq_cfg_reg + bank * irq_mem_size,
			   irq_mem_used);
	}

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	sunxi_info(dev, "pinctrl resume\n");

	return 0;
}

static const struct dev_pm_ops sun55iw6_pinctrl_pm_ops = {
	.suspend_noirq = sun55iw6_pinctrl_suspend_noirq,
	.resume_noirq = sun55iw6_pinctrl_resume_noirq,
};
#define PINCTRL_PM_OPS	(&sun55iw6_pinctrl_pm_ops)

#else
static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	return 0;
}
#define PINCTRL_PM_OPS	NULL
#endif

static int sun55iw6_pinctrl_probe(struct platform_device *pdev)
{
	int ret;
	ret = pinctrl_pm_alloc_mem(pdev);
	if (ret) {
		sunxi_err(&pdev->dev, "alloc pm mem err\n");
		return ret;
	}

	sunxi_info(NULL, "sunxi pinctrl version: %s\n", SUNXI_PINCTRL_VERSION);

#if IS_ENABLED(CONFIG_PINCTRL_SUNXI_DEBUGFS)
	dev_set_name(&pdev->dev, "pio");
#endif
	return sunxi_bsp_pinctrl_init(pdev, &sun55iw6_pinctrl_data);
}

static struct of_device_id sun55iw6_pinctrl_match[] = {
	{ .compatible = "allwinner,sun55iw6-pinctrl", },
	{}
};

MODULE_DEVICE_TABLE(of, sun55iw6_pinctrl_match);

static struct platform_driver sun55iw6_pinctrl_driver = {
	.probe	= sun55iw6_pinctrl_probe,
	.driver	= {
		.name		= "sun55iw6-pinctrl",
		.pm		= PINCTRL_PM_OPS,
		.of_match_table	= sun55iw6_pinctrl_match,
	},
};

static int __init sun55iw6_pio_init(void)
{
	return platform_driver_register(&sun55iw6_pinctrl_driver);
}
fs_initcall(sun55iw6_pio_init);

MODULE_DESCRIPTION("Allwinner sun55iw6 pio pinctrl driver");
MODULE_AUTHOR("<panzhijian@allwinnertech>");
MODULE_LICENSE("GPL");
MODULE_VERSION(SUNXI_PINCTRL_VERSION);
