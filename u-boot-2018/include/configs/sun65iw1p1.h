/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration settings for the Allwinner A7 (sun8i) CPU
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_EHCI_SUNXI
#define CONFIG_USB_MAX_CONTROLLER_COUNT 1
#endif

#define CONFIG_SUNXI_USB_PHYS	1

/* sram layout*/

#define SUNXI_SRAM_BASE		(0x44000)
#define SUNXI_SRAM_SIZE		(0x8000)

#define SUNXI_SYS_SRAM_BASE		SUNXI_SRAM_BASE
#define SUNXI_SYS_SRAM_SIZE		(SUNXI_SRAM_SIZE)

#define CONFIG_SYS_BOOTM_LEN 0x4000000

#ifdef CONFIG_SUNXI_MALLOC_LEN
#define SUNXI_SYS_MALLOC_LEN	CONFIG_SUNXI_MALLOC_LEN
#else
#define SUNXI_SYS_MALLOC_LEN	(32 << 20)
#endif

#define PHOENIX_PRIV_DATA_ADDR      (0x69000)
/*
 * Include common sunxi configuration where most the settings are
 */
#include <configs/sunxi-common.h>

#endif /* __CONFIG_H */
