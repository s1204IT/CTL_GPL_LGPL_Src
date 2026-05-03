/*
 * (C) Copyright 2018 allwinnertech  <wangwei@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <console.h>
#include <command.h>
#include <securestorage.h>
#include <sunxi_board.h>
#include <android_ab.h>

extern volatile int sunxi_abts_detect_runnning;
extern volatile int sunxi_burn_detect_abts;

extern int sunxi_usb_dev_register(uint dev_name);
extern  void sunxi_usb_main_loop(int delaytime);
extern  int sunxi_usb_extern_loop(void);
extern  int sunxi_usb_init(int delaytime);
extern  int sunxi_usb_exit(void);

DECLARE_GLOBAL_DATA_PTR;

extern volatile int sunxi_usb_burn_from_boot_handshake, sunxi_usb_burn_from_boot_init, sunxi_usb_burn_from_boot_setup;


int do_abts_detect(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	ulong begin_time = 0, over_time = 0;

	tick_printf("enter do_abts_detect\n");
	if (sunxi_usb_dev_register(4) < 0) {
		pr_err("usb burn fail: not support burn private data\n");
		return -1;
	}

	if (sunxi_usb_init(0)) {
		pr_err("%s usb init fail\n", __func__);
		sunxi_usb_exit();
		return 0;
	}

	tick_printf("usb prepare ok\n");
	begin_time = get_timer(0);
	over_time = 300;
	while (1) {
		if (sunxi_usb_burn_from_boot_init) {
			tick_printf("usb sof ok\n");
			break;
		}
		if (get_timer(begin_time) > over_time) {
			tick_printf("overtime\n");
			sunxi_usb_exit();
			tick_printf("%s usb : no usb exist\n", __func__);

			return 0;
		}
	}
	tick_printf("usb probe ok\n");
	tick_printf("usb setup ok\n");

	begin_time = get_timer(0);
	over_time = 400;
	sunxi_abts_detect_runnning = 1;
	while (1) {
		ret = sunxi_usb_extern_loop();
		if (ret)
			break;

		if (sunxi_burn_detect_abts == 2)
			break;

		if (get_timer(begin_time) > over_time) {
			sunxi_abts_detect_runnning = 0;
			tick_printf("%s usb : have no handshake\n", __func__);
			sunxi_usb_exit();
			return 0;
		}

		if (ctrlc()) {
			ret = SUNXI_UPDATE_NEXT_ACTION_NORMAL;
			break;
		}
	}
	sunxi_abts_detect_runnning = 0;
	tick_printf("exit abts detect\n");
	sunxi_usb_exit();
	sunxi_update_subsequent_processing(ret);

	return 0;
}

U_BOOT_CMD(
	abts_detect, CONFIG_SYS_MAXARGS, 1, do_abts_detect,
	"do abts handshake detect",
	"pburn [mode]"
	"NULL"
);
