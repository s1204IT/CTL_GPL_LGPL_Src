
/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Liaoyongming <liaoyongming@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 */

#include <sunxi_board.h>
#include <smc.h>
#include <spare_head.h>
#include <common.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <securestorage.h>
#include <asm/arch/rtc.h>

typedef enum start_mode {
	COLD_START = 0x0,
	FAKE_POWEROFF = 0x2,
	FAKE_POWEROFF_E = 0xe,	/* compatible with old platform, such as sun50iw9 */
	BOOT_NORMAL = 0xf
} start_mode_e;

typedef enum start_type {
	START2FAKE_POWEROFF = 0x0,
	START2NORMAL_BOOT = 0x1
} start_type_e;

static void sunxi_fake_poweroff(void)
{
	arm_svc_fake_poweroff((ulong)working_fdt);
}

/* parse startup type from factoty menu
 *
 * The result decide homlet/TV startup in fake poweroff
 * or startup direcely.
 *
 * return:
 * 1 if parse result is fake poweroff.
 * 0 if not fake poweroff type or factory menu not support.
 */
uint parse_factory_menu(void)
{
	int ret = 0;
	char buffer[32];
	int data_len;
	char standby_char[10] = "standby";

	if (sunxi_secure_storage_init()) {
		pr_msg("secure storage init fail\n");
		return 0;
	} else {
		/* judge if factory menu has set standby flag */
		ret = sunxi_secure_object_read("BOOTMODE", buffer, 32, &data_len);
		if (ret) {
			pr_msg("sunxi secure storage has no start mode flag\n");
			return 0;
		} else {
			if (!strncmp(buffer, standby_char, strlen(standby_char))) {
				return 1;
			}
		}
	}
	return 0;
}

/*
 * parse_box_standby
 *
 * return:
 * 0 - normal boot
 * 1 - maybe box standby
 */
#if defined(CONFIG_MACH_SUN50IW9) || defined(CONFIG_MACH_SUN50IW12)
static int parse_box_standby(void)
{
	return 1;
}
#else
static uint32_t __maybe_unused read_start_mode(void)
{
	return rtc_read_data(FAKE_POWEROFF_FLAG_INDEX);
}

static void __maybe_unused write_start_mode(uint32_t mode)
{
	rtc_write_data(FAKE_POWEROFF_FLAG_INDEX, mode);
}

static int parse_box_standby(void)
{
	int node;
	int ret = 0;
	uint32_t start_type = 1, start_mode;

	node = fdt_path_offset(working_fdt, "/box_start_os0");
	if (node < 0) {
		return 0;
	}

	/* get start_type from dts config*/
	fdt_getprop_u32(working_fdt, node, "start_type", &start_type);

	/* get start_mode from FAKE_POWEROFF_FLAG rtc reg */
	start_mode = read_start_mode();

	pr_force("start_type: %x, start_mode: %x\n", start_type, start_mode);

	switch (start_mode) {
	case FAKE_POWEROFF:
	case FAKE_POWEROFF_E:
		{
			pr_force("enter fake poweroff\n");
			ret = 1;
			break;
		}
	case COLD_START:
		{
			if (start_type == START2FAKE_POWEROFF)
				ret = 1;
			else if (start_type == START2NORMAL_BOOT)
				ret = 0;
			else
				ret = 0;
			break;
		}
	case BOOT_NORMAL:
	default:
		{
			ret = 0;
			break;
		}
	}

	return ret;
}
#endif

int atf_box_standby(void)
{

	if (get_boot_work_mode() != WORK_MODE_BOOT) {
		return 0;
	}

	/* only cold start need to judge fake poweroff or not */
	if (parse_factory_menu() > 0) {
		if (rtc_read_data(CONFIG_START_MODE_RTC_REG_INDEX) == 0) {
			rtc_write_data(CONFIG_START_MODE_RTC_REG_INDEX, 0x2);
		}
	}

	if (!parse_box_standby())
		return 0;

	sunxi_fake_poweroff();
	return 0;
}
