/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <sunxi_mbr.h>
#include <sys_config.h>
#include <private_boot0.h>
#include <malloc.h>
#include <sunxi_board.h>
#include <fdt_support.h>
#include "sunxi_flash.h"
void __dump_dlmap(sunxi_download_info *dl_info);
