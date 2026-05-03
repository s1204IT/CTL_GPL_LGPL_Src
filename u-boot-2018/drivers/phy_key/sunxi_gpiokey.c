/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */
#include <common.h>
#include <sys_config.h>
#include <asm/io.h>
#include <physical_key.h>
#include <sys_config.h>
#include <fdt_support.h>
#include <console.h>
#include <physical_key.h>

__attribute__((section(".data")))

struct Volume_key{
	user_gpio_set_t gpio_volume_up;
	user_gpio_set_t gpio_volume_down;
	__u32 gpio_volumedown_hd;
	__u32 gpio_volumeup_hd;
	__u32 active_level;
};
static struct Volume_key volume_key;

__weak int axp_probe_key(void)
{
	return 0;
}

int sunxi_gpiokey_init(void)
{
	int ret;
	int nodeoffset;
	char *key_type = NULL;

	nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_KEY_DETECT);
	if (nodeoffset > 0) {
		fdt_getprop_string(working_fdt, nodeoffset, "key_type", &key_type);
		if (!strcmp(key_type, "gpiokey"))
			fdt_getprop_u32(working_fdt, nodeoffset, "active-level", &volume_key.active_level);
		else
			return -1;
	} else {
		return -1;
	}

	ret = fdt_get_one_gpio(FDT_PATH_KEY_DETECT, "volume-up-gpios",
							&volume_key.gpio_volume_up);
	if (ret) {
		printf("[gpio_volume_up] can't find volume-up-gpios config.\n");
		return -1;
	}

	ret = fdt_get_one_gpio(FDT_PATH_KEY_DETECT, "volume-down-gpios",
							&volume_key.gpio_volume_down);
	if (ret) {
		printf("[gpio_volume_down] can't find volume-down-gpios config.\n");
		return -1;
	}

	volume_key.gpio_volume_up.mul_sel = 0;              /*forced to input*/
	volume_key.gpio_volume_down.mul_sel = 0;              /*forced to input*/

	volume_key.gpio_volumeup_hd = sunxi_gpio_request(&volume_key.gpio_volume_up, 1);
	if (!volume_key.gpio_volumeup_hd) {
		printf("[key gpio_volume_up] gpio request fail!\n");
		return -1;
		/*gpio request fail,just return.*/
	}

	volume_key.gpio_volumedown_hd = sunxi_gpio_request(&volume_key.gpio_volume_down, 1);
	if (!volume_key.gpio_volumedown_hd) {
		printf("[key gpio_volume_down] gpio request fail!\n");
		return -1;
		/*gpio request fail,just return.*/
	}

	return 0;
}

int sunxi_gpiokey_read(void)
{
	int volume_down = -1;
	int volume_up = -1;

	volume_up = gpio_read_one_pin_value(volume_key.gpio_volumeup_hd, "volume-up-gpios");
	volume_down = gpio_read_one_pin_value(volume_key.gpio_volumedown_hd, "volume-down-gpios");

	if (volume_down == volume_key.active_level || volume_up == volume_key.active_level) {
		pr_err("volume key pressed! volume_up:%d, volume_down:%d\n", volume_up, volume_down);
		return 1;
	}

	return 0;
}

int sunxi_gpiokey_exit(void)
{
	return 0;
}


int do_gpiokey_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 power_key = 0;
	int ret;

	ret = sunxi_gpiokey_init();
	if (ret) {
		printf("gpiokey init failed\n");
		return -1;
	}

	puts(" press a key:\n");
	while (!ctrlc()) {
		sunxi_gpiokey_read();
		power_key = axp_probe_key();
		if (power_key > 0) {
			break;
		}
	}

	return 0;
}

U_BOOT_CMD(
	gpiokey_test, 1, 1,	do_gpiokey_test,
	"Test the gpiokey value\n",
	""
);

U_BOOT_PHY_KEY_INIT(gpiokey) = {
	.phykey_name	= "gpiokey",
	.phykey_init	= sunxi_gpiokey_init,
	.phykey_read	= sunxi_gpiokey_read,
	.phykey_exit	= sunxi_gpiokey_exit,
};

