/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <sys_config.h>
#include <private_uboot.h>
#include <sunxi_board.h>

DECLARE_GLOBAL_DATA_PTR;

extern int do_abts_detect(cmd_tbl_t *cmdtp, int flag, int argc,
			     char *const argv[]);

int sunxi_abts_detect(void)
{
	return do_abts_detect(NULL, 0, 0, NULL);
}
