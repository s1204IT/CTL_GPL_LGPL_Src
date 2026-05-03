/*
 * include/boot_param.h
 *
 * Copyright (c) 2021-2025 Allwinnertech Co., Ltd.
 * Author: lujianliang <lujianliang@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __BOOT_PARAM_H
#define __BOOT_PARAM_H
#include <common.h>

#define BOOT_PARAM_MAGIC		"bootpara"
#define CHECK_SUM			0x5F0A6C39


/* bit map
 * transfer_flag:bit0
 * 1:uboot download bootparam  0: nothing */

#define BOOTPARAM_DOWNLOAD_MASK  (0x1)
struct sunxi_boot_parameter_header {
	u8 magic[8]; //bootpara
	u32 version; // describe the region version
	u32 check_sum;
	u32 length;
	u32 boot0_checksum;
	u32 transfer_flag;
	u8 reserved[4];
};


#define MAX_DRAMPARA_SIZE (96)
typedef struct {
	unsigned int dram_para[MAX_DRAMPARA_SIZE];
	char res[512 - (4 * MAX_DRAMPARA_SIZE)];
} boot_dram_info_t;

// BOOT_ PARAM_ SIZE maximum value is 4k
#define BOOT_PARAM_SIZE (2048)
typedef struct sunxi_boot_param_region{
	struct sunxi_boot_parameter_header header;
	char sdmmc_info[256 -32];
	char nand_info[256];
	char spiflash_info[256];
	char ufs[256];
	char ddr_info[512];
	char reserved[BOOT_PARAM_SIZE - 512*3];// = 2048 - 32 - sdmmc_size - nand_size - spi_size - ddr_size - res
} typedef_sunxi_boot_param;

typedef_sunxi_boot_param *sunxi_bootparam_get_buf_in_relocate_f(void);
int sunxi_bootparam_format(typedef_sunxi_boot_param *sunxi_boot_param);
void sunxi_bootparam_set_boot0_checksum(
	typedef_sunxi_boot_param *sunxi_boot_param, int boot0_checksum);
int sunxi_bootparam_check_magic(typedef_sunxi_boot_param *sunxi_boot_param);
int sunxi_bootparam_down(void);
#endif
