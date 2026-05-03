// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 zhaozeyan@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

#include "pinctrl-sunxi.h"

#define SUNXI_PINCTRL_VERSION   "0.0.4"

static const struct sunxi_desc_pin sun251iw1_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
/* Pin banks are: A B C D F G */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[3] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[2] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[1] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[0] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[3] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[2] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[1] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[0] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clkrx_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_clkrx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxdv_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxdv_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_mdc_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_mdc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_md_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_md_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txen_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txen_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_clktx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_clktx_do */
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
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spdif"),         /* spdif_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spdif"),         /* spdif_di */
		SUNXI_FUNCTION(0x2, "//spdif"),       /* //spdif_mclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
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
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[27] */
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
		SUNXI_FUNCTION(0x1, "spif"),          /* spif_dqs_di */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dqs_oe */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[4] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq4_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[5] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq5_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[5] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[6] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq6_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[7] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq7_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_ccmd_di */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_mosi_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_cle_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_mosi_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cclk_do */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_ss_do[0] */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_rb_di[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_ss_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_hold_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_hold_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_ds_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 27),  /* eint */
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
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_miso_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_cen_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_miso_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_wp_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[1] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_wp_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_sck_do */
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
	/* Pin banks are: A B C D E F G */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "tp"),            /* tp */
		SUNXI_FUNCTION(0x3, "pwm_1"),         /* pwm_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "tp"),            /* tp */
		SUNXI_FUNCTION(0x3, "pwm_2"),         /* pwm_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "tp"),            /* tp */
		SUNXI_FUNCTION(0x3, "pwm_3"),         /* pwm_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "tp"),            /* tp */
		SUNXI_FUNCTION(0x3, "pwm_4"),         /* pwm_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm_3"),         /* pwm_3 */
		SUNXI_FUNCTION(0x3, "ir"),            /* ir */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x6, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x8, "owa"),           /* owa */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout3"),    /* i2s2_dout3<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "i2s2_din3"),     /* i2s2_din3<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x6, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x8, "ir"),            /* ir */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout2"),    /* i2s2_dout2<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "i2s2_din2"),     /* i2s2_din2<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x7, "uart4"),         /* uart4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout1"),    /* i2s2_dout1<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "i2s2_din0"),     /* i2s2_din0<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x7, "uart4"),         /* uart4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout0"),    /* i2s2_dout0<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "i2s2_din1"),     /* i2s2_din1<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x7, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "i2s2_bclk"),     /* i2s2_bclk<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "pwm_0"),         /* pwm_0 */
		SUNXI_FUNCTION(0x7, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "i2s2_lrck"),     /* i2s2_lrck<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x5, "pwm_1"),         /* pwm_1 */
		SUNXI_FUNCTION(0x7, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm_4"),         /* pwm_4 */
		SUNXI_FUNCTION(0x3, "i2s2_mclk"),     /* i2s2_mclk<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x4, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir */
		SUNXI_FUNCTION(0x7, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "boot"),          /* boot */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "boot"),          /* boot */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x6, "dbg"),           /* dbg */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x6, "tcon"),          /* tcon */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "pwm_4"),         /* pwm_4 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "pwm_5"),         /* pwm_5 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "pwm_6"),         /* pwm_6 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "pwm_7"),         /* pwm_7 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "clk"),           /* clk */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "pwm_2"),         /* pwm_2 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "clk"),           /* clk */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "pwm_0"),         /* pwm_0 */
		SUNXI_FUNCTION(0x4, "owa"),           /* owa */
		SUNXI_FUNCTION(0x5, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x6, "clk"),           /* clk */
		SUNXI_FUNCTION(0x7, "ir"),            /* ir */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart4"),         /* uart4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "uart4"),         /* uart4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm_6"),         /* pwm_6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x5, "pwm_0"),         /* pwm_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x5, "pwm_1"),         /* pwm_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x5, "pwm_2"),         /* pwm_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x5, "pwm_3"),         /* pwm_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x5, "pwm_4"),         /* pwm_4 */
		SUNXI_FUNCTION(0x6, "hdmi0"),         /* hdmi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "pwm_5"),         /* pwm_5 */
		SUNXI_FUNCTION(0x6, "hdmi0"),         /* hdmi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa"),           /* owa */
		SUNXI_FUNCTION(0x3, "ir"),            /* ir */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "pwm_7"),         /* pwm_7 */
		SUNXI_FUNCTION(0x6, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank E */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x5, "owa"),           /* owa */
		SUNXI_FUNCTION(0x6, "hdmi0"),         /* hdmi0 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x5, "owa"),           /* owa */
		SUNXI_FUNCTION(0x6, "hdmi0"),         /* hdmi0 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "pwm_2"),         /* pwm_2 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm_5"),         /* pwm_5 */
		SUNXI_FUNCTION(0x3, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "pwm_3"),         /* pwm_3 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x6, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "pwm_4"),         /* pwm_4 */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "ir"),            /* ir */
		SUNXI_FUNCTION(0x5, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "pwm_7"),         /* pwm_7 */
		SUNXI_FUNCTION(0x5, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "pwm_6"),         /* pwm_6 */
		SUNXI_FUNCTION(0x5, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x4, "i2s0_dout1"),    /* i2s0_dout1 */
		SUNXI_FUNCTION(0x5, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x4, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0x5, "i2s0_din1"),     /* i2s0_din1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x8, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x4, "i2s0_dout2"),    /* i2s0_dout2 */
		SUNXI_FUNCTION(0x5, "i2s0_din2"),     /* i2s0_din2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x4, "i2s0_dout3"),    /* i2s0_dout3 */
		SUNXI_FUNCTION(0x5, "i2s0_din3"),     /* i2s0_din3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "djtag"),         /* djtag */
		SUNXI_FUNCTION(0x4, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x5, "i2s2_dout1"),    /* i2s2_dout1<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x6, "i2s2_din0"),     /* i2s2_din0<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "djtag"),         /* djtag */
		SUNXI_FUNCTION(0x4, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x5, "i2s2_dout0"),    /* i2s2_dout0<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x6, "i2s2_din1"),     /* i2s2_din1<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x6, "owa"),           /* owa */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "djtag"),         /* djtag */
		SUNXI_FUNCTION(0x4, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x5, "i2s2_bclk"),     /* i2s2_bclk<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "pwm_6"),         /* pwm_6 */
		SUNXI_FUNCTION(0x6, "ir"),            /* ir */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "djtag"),         /* djtag */
		SUNXI_FUNCTION(0x4, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x5, "i2s2_lrck"),     /* i2s2_lrck<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "owa"),           /* owa */
		SUNXI_FUNCTION(0x4, "ir"),            /* ir */
		SUNXI_FUNCTION(0x5, "i2s2_mclk"),     /* i2s2_mclk<mux_hdmi_rx> */
		SUNXI_FUNCTION(0x6, "pwm_5"),         /* pwm_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1_d1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "pwm_7"),         /* pwm_7 */
		SUNXI_FUNCTION(0x6, "sdc1_d3"),          /* sdc1 */
		//SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1_d0"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "pwm_6"),         /* pwm_6 */
		SUNXI_FUNCTION(0x6, "sdc1_d1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "sdc1_d3"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1_d0"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "uart4"),         /* uart4 */
		//SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		//SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1_cmd"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x6, "sdc1_d0"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "sdc1_d2"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1_d3"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "pwm_5"),         /* pwm_5 */
		SUNXI_FUNCTION(0x6, "sdc1_cmd"),          /* sdc1 */
		//SUNXI_FUNCTION(0x7, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1_d2"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "pwm_4"),         /* pwm_4 */
		//SUNXI_FUNCTION(0x6, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x7, "sdc1_d0"),          /* sdc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "pwm_1"),         /* pwm_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "owa"),           /* owa */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x8, "hdmi1"),         /* hdmi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x8, "hdmi1"),         /* hdmi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm_3"),         /* pwm_3 */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "ir"),            /* ir */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "tcon"),          /* tcon */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION(0x3, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "pwm_0"),         /* pwm_0 */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0x3, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "pwm_2"),         /* pwm_2 */
		SUNXI_FUNCTION(0x6, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "i2s1_dout1"),    /* i2s1_dout1 */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x5, "i2s1_din1"),     /* i2s1_din1 */
		SUNXI_FUNCTION(0x7, "uart1"),         /* uart1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir"),            /* ir */
		SUNXI_FUNCTION(0x3, "tcon"),          /* tcon */
		SUNXI_FUNCTION(0x4, "pwm_5"),         /* pwm_5 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "owa"),           /* owa */
		SUNXI_FUNCTION(0x7, "ledc"),          /* ledc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "pwm_7"),         /* pwm_7 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "ir"),            /* ir */
		SUNXI_FUNCTION(0x7, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x8, "hdmi1"),         /* hdmi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "pwm_6"),         /* pwm_6 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "owa"),           /* owa */
		SUNXI_FUNCTION(0x7, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x8, "hdmi1"),         /* hdmi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#endif
};

static const unsigned int sun251iw1_bank_base[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
#else
	SUNXI_BANK_OFFSET('E', 'A'),
#endif
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
};

static const unsigned int sun251iw1_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
#if IS_ENABLED(CONFIG_AW_FPGA_BOARD)
#else
	SUNXI_BANK_OFFSET('E', 'A'),
#endif
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
};

static const struct sunxi_pinctrl_desc sun251iw1_pinctrl_data = {
	.pins = sun251iw1_pins,
	.npins = ARRAY_SIZE(sun251iw1_pins),
	.banks = ARRAY_SIZE(sun251iw1_bank_base),
	.bank_base = sun251iw1_bank_base,
	.irq_banks = ARRAY_SIZE(sun251iw1_irq_bank_map),
	.irq_bank_map = sun251iw1_irq_bank_map,
	.auto_power_source_switch = true,
	.pf_power_source_switch = true,
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
		return -ENOMEM;
	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
	return 0;
}

static int sun251iw1_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	sunxi_info(dev, "pinctrl suspend\n");

	return 0;
}

static int sun251iw1_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl_desc const *desc = &sun251iw1_pinctrl_data;
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;
	int idx, bank;
	int initial_bank_offset = sunxi_pinctrl_hw_info[desc->hw_type].initial_bank_offset;
	int bank_mem_size = sunxi_pinctrl_hw_info[desc->hw_type].bank_mem_size;
	int irq_cfg_reg = sunxi_pinctrl_hw_info[desc->hw_type].irq_cfg_reg;
	int irq_mem_size = sunxi_pinctrl_hw_info[desc->hw_type].irq_mem_size;
	int irq_debounce_reg = sunxi_pinctrl_hw_info[desc->hw_type].irq_debounce_reg;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	for (idx = 0; idx < desc->banks; idx++) {
		bank = desc->bank_base[idx];
		/* cfg/dat/drv/pull */
		memcpy_toio(pctl->membase + initial_bank_offset + bank * bank_mem_size,
			    mem + initial_bank_offset + bank * bank_mem_size,
			    bank_mem_size);
	}

	for (idx = 0; idx < desc->irq_banks; idx++) {
		bank = desc->irq_bank_map[idx];
		/* irq cfg */
		memcpy_toio(pctl->membase + irq_cfg_reg + bank * irq_mem_size,
			    mem + irq_cfg_reg + bank * irq_mem_size,
			   0x10);
		/* irq deb */
		writel(readl(mem + irq_debounce_reg + bank * irq_mem_size),
			pctl->membase + irq_debounce_reg + bank * irq_mem_size);
	}

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	sunxi_info(dev, "pinctrl resume\n");
	return 0;
}

static const struct dev_pm_ops sun251iw1_pinctrl_pm_ops = {
	.suspend_noirq = sun251iw1_pinctrl_suspend_noirq,
	.resume_noirq = sun251iw1_pinctrl_resume_noirq,
};
#define PINCTRL_PM_OPS	(&sun251iw1_pinctrl_pm_ops)

#else
static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	return 0;
}
#define PINCTRL_PM_OPS	NULL
#endif

static int sun251iw1_pinctrl_probe(struct platform_device *pdev)
{
	int ret;
	ret = pinctrl_pm_alloc_mem(pdev);
	if (ret) {
		sunxi_err(&pdev->dev, "alloc pm mem err\n");
		return ret;
	}

	sunxi_info(NULL, "sunxi pinctrl version: %s\n", SUNXI_PINCTRL_VERSION);
	return sunxi_bsp_pinctrl_init(pdev, &sun251iw1_pinctrl_data);
}

static struct of_device_id sun251iw1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun251iw1-pinctrl", },
	{}
};

MODULE_DEVICE_TABLE(of, sun251iw1_pinctrl_match);

static struct platform_driver sun251iw1_pinctrl_driver = {
	.probe	= sun251iw1_pinctrl_probe,
	.driver	= {
		.name		= "sun251iw1-pinctrl",
		.pm		= PINCTRL_PM_OPS,
		.of_match_table	= sun251iw1_pinctrl_match,
	},
};

static int __init sun251iw1_pio_init(void)
{
	return platform_driver_register(&sun251iw1_pinctrl_driver);
}
fs_initcall(sun251iw1_pio_init);

MODULE_DESCRIPTION("Allwinner sun251iw1 pio pinctrl driver");
MODULE_AUTHOR("<rengaomin@allwinnertech>");
MODULE_LICENSE("GPL");
MODULE_VERSION(SUNXI_PINCTRL_VERSION);
