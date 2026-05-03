/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef __SUNXI_SPINAND_H
#define __SUNXI_SPINAND_H

#include <linux/mtd/aw-spinand.h>
#include <linux/mtd/mtd.h>
#include <linux/mutex.h>
#include <linux/spi/sunxi-spi.h>

#define SECTOR_SHIFT 9

#define AW_MTD_SPINAND_VER_MAIN		0x02
#define AW_MTD_SPINAND_VER_SUB		0x07
#define AW_MTD_SPINAND_VER_DATE		0x20240110

#define UBOOT_START_BLOCK_BIGNAND 4
#define UBOOT_START_BLOCK_SMALLNAND 8
#define PHY_BLKS_FOR_BOOT0	8
#define PHY_BLKS_FOR_SECURE_STORAGE 8
#define PSTORE_SIZE_KB	512
#define NAND_BOOT0_BLK_START    0
#define VALID_PAGESIZE_FOR_BOOT0 2048
#define AW_SAMP_MODE_DL_DEFAULT 0xaaaaffff
#define UBOOTA	0
#define UBOOTB	1
#define DOWNLOAD_UBOOTA	0
#define DOWNLOAD_UBOOTB	1
#define UBOOT_STRING_LEN	6

struct aw_spinand {
	struct mutex lock;
	struct aw_spinand_chip chip;
	struct mtd_info mtd;
	int sector_shift;
	int page_shift;
	int block_shift;
	int phy_page_shift;
	int phy_block_shift;
#if IS_ENABLED(CONFIG_AW_SPINAND_SECURE_STORAGE)
	struct aw_spinand_sec_sto sec_sto;
#endif
	unsigned int right_sample_delay;
	unsigned int right_sample_mode;
};

extern struct aw_spinand *get_aw_spinand(void);
extern uint64_t get_sys_part_offset(void);

#define spinand_to_mtd(spinand) (&spinand->mtd)
#define spinand_to_chip(spinand) (&spinand->chip)
#define mtd_to_spinand(mtd) container_of(mtd, struct aw_spinand, mtd)
#define spi_to_spinand(spi) spi_get_drvdata(spi)

#endif
