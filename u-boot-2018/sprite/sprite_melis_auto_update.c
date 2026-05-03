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
#include <common.h>
#include <malloc.h>
#include <sprite.h>
#include <private_toc.h>
#include <private_boot0.h>
#include <sunxi_board.h>
#include <spare_head.h>
#include "sparse/sparse.h"
#include <fs.h>
#include <usb.h>
#include "sprite_verify.h"
#include "firmware/imgdecode.h"
#include "sys_config.h"
#include "./cartoon/sprite_cartoon.h"

#include "firmware/imgdecode.h"
#include "sprite_common.h"
#include <private_uboot.h>

#define IMG_NAME "update/FIRMWARE.bin"

static void *imghd;
static void *imgitemhd;
static char *imgname;
char interface[8] = "usb";

#define SCRIPT_FILE_COMMENT '#' // symbol for comment
#define SCRIPT_FILE_END '%' // symbol for file end
#define MAX_LINE_SIZE 8000
#define MAX_FILE_SIZE (0x800000)
#define IS_COMMENT(x) (SCRIPT_FILE_COMMENT == (x))
#define IS_FILE_END(x) (SCRIPT_FILE_END == (x))
#define IS_LINE_END(x) ('\r' == (x) || '\n' == (x))

#define AU_HEAD_BUFF (32 * 1024)
#if defined(CONFIG_SUNXI_SPINOR)
#define AU_ONCE_DATA_DEAL (2 * 1024 * 1024)
#else
#define AU_ONCE_DATA_DEAL (3 * 1024 * 1024)
#endif
#define AU_ONCE_SECTOR_DEAL (AU_ONCE_DATA_DEAL / 512)

extern struct spare_boot_head_t  uboot_spare_head;

static int sunxi_update_fail_exit(void)
{
	extern int run_command(const char *cmd, int flag);
	usb_stop();
	run_command("boot_melis", 0);
	return 0;
}

static int sunxi_update_success_exit(void)
{
	extern int run_command(const char *cmd, int flag);
	usb_stop();
	run_command("boot_melis", 0);
	return 0;
}

static int melis_auto_update_firmware_probe(char *name)
{
	imghd = Img_Fat_Open(name);

	if (!imghd) {
		return -1;
	}
	return 0;
}

static int melis_auto_update_fetch_download_map(sunxi_download_info *dl_map)
{
	imgitemhd = Img_OpenItem(imghd, "12345678", "1234567890DLINFO");
	if (!imgitemhd) {
		return -1;
	}

	if (!Img_Fat_ReadItem(imghd, imgitemhd, imgname, (void *)dl_map,
			      sizeof(sunxi_download_info))) {
		printf("sunxi sprite error : read dl map failed\n");

		return -1;
	}

	Img_CloseItem(imghd, imgitemhd);
	imgitemhd = NULL;

	return sunxi_sprite_verify_dlmap(dl_map);
}

loff_t fat_fs_read(const char *filename, void *buf, int offset, int len)
{
	loff_t len_read;

	if (!buf || !filename)
		return -1;

	if (fs_set_blk_dev(interface, "0", FS_TYPE_FAT))
		return -1;

	if (fs_read(filename, (ulong)buf, offset, len, &len_read))
		return -1;

	return len_read;
}

static int melis_download_normal_part(dl_one_part_info *part_info, uchar *source_buff)
{
	int ret = -1;
	uint partstart_by_sector;
	uint tmp_partstart_by_sector;
	s64 partsize_by_byte;
	s64 partdata_by_byte;
	s64 tmp_partdata_by_bytes;
	uint onetime_read_sectors;
	uint first_write_bytes;
	uint imgfile_start;
	uint tmp_imgfile_start;
	int partdata_format;
	uint active_verify;
	uint origin_verify;
	uchar verify_data[1024];
	uint *tmp;
	u8 *down_buffer = source_buff + AU_HEAD_BUFF;
	extern int sunxi_sprite_erase_area(uint start_block, uint nblock);
	struct blk_desc *desc;
	disk_partition_t info = { 0 };

	tmp_partstart_by_sector = partstart_by_sector = part_info->addrlo;
	partsize_by_byte			      = part_info->lenlo;
	partsize_by_byte <<= 9;

	desc = blk_get_devnum_by_typename("sunxi_flash", 0);
	if (desc == NULL)
		return -ENODEV;

	ret = sunxi_flash_try_partition(desc, (const char *)part_info->name, &info);
	if (ret < 0)
		return -ENODEV;
	sunxi_sprite_erase_area(info.start, info.size);

	imgitemhd = Img_OpenItem(imghd, "RFSFAT16", (char *)part_info->dl_filename);
	if (!imgitemhd) {
		printf("sunxi sprite error: open part %s failed\n",
		       part_info->dl_filename);

		return -1;
	}

	partdata_by_byte = Img_GetItemSize(imghd, imgitemhd);
	if (partdata_by_byte <= 0) {
		printf("sunxi sprite error: fetch part len %s failed\n",
		       part_info->dl_filename);

		goto __download_normal_part_err1;
	}
	printf("partdata hi 0x%x\n", (uint)(partdata_by_byte >> 32));
	printf("partdata lo 0x%x\n", (uint)partdata_by_byte);

	if (partdata_by_byte > partsize_by_byte) {
		printf("sunxi sprite: data size 0x%x is larger than part %s size 0x%x\n",
		       (uint)(partdata_by_byte / 512), part_info->dl_filename,
		       (uint)(partsize_by_byte / 512));

		goto __download_normal_part_err1;
	}

	tmp_partdata_by_bytes = partdata_by_byte;
	if (tmp_partdata_by_bytes >= AU_ONCE_DATA_DEAL) {
		onetime_read_sectors = AU_ONCE_SECTOR_DEAL;
		first_write_bytes    = AU_ONCE_DATA_DEAL;
	} else {
		onetime_read_sectors = (tmp_partdata_by_bytes + 511) >> 9;
		first_write_bytes    = (uint)tmp_partdata_by_bytes;
	}

	imgfile_start = Img_GetItemOffset(imghd, imgitemhd);
	if (!imgfile_start) {
		printf("sunxi sprite err : cant get part data imgfile_start %s\n",
		       part_info->dl_filename);

		goto __download_normal_part_err1;
	}
	tmp_imgfile_start = imgfile_start;

	if (fat_fs_read(imgname, down_buffer, imgfile_start,
			onetime_read_sectors * 512) !=
	    onetime_read_sectors * 512) {
		printf("sunxi sprite error : read sdcard start %d, total %d failed\n",
		       tmp_imgfile_start, onetime_read_sectors);

		goto __download_normal_part_err1;
	}
	/* position of next data to be read*/
	tmp_imgfile_start += onetime_read_sectors * 512;

	/* check sparse format or not */
	partdata_format = unsparse_probe((char *)down_buffer, first_write_bytes,
					 partstart_by_sector);
	if (partdata_format != ANDROID_FORMAT_DETECT) {
		if (sunxi_sprite_write(tmp_partstart_by_sector,
				       onetime_read_sectors,
				       down_buffer) != onetime_read_sectors) {
			printf("sunxi sprite error: download rawdata error %s\n",
			       part_info->dl_filename);

			goto __download_normal_part_err1;
		}
		tmp_partdata_by_bytes -= first_write_bytes;
		tmp_partstart_by_sector += onetime_read_sectors;

		while (tmp_partdata_by_bytes >= AU_ONCE_DATA_DEAL) {
			/* continue read partition data from img*/
			if (fat_fs_read(imgname, down_buffer, tmp_imgfile_start,
					AU_ONCE_DATA_DEAL) !=
			    AU_ONCE_DATA_DEAL) {
				printf("sunxi sprite error : read sdcard start %d, total %d failed\n",
				       tmp_imgfile_start, AU_ONCE_DATA_DEAL);

				goto __download_normal_part_err1;
			}
			if (sunxi_sprite_write(tmp_partstart_by_sector,
					       AU_ONCE_SECTOR_DEAL,
					       down_buffer) !=
			    AU_ONCE_SECTOR_DEAL) {
				printf("sunxi sprite error: download rawdata error %s, start 0x%x, sectors 0x%x\n",
				       part_info->dl_filename,
				       tmp_partstart_by_sector,
				       AU_ONCE_SECTOR_DEAL);

				goto __download_normal_part_err1;
			}
			tmp_imgfile_start += AU_ONCE_SECTOR_DEAL * 512;
			tmp_partdata_by_bytes -= AU_ONCE_DATA_DEAL;
			tmp_partstart_by_sector += AU_ONCE_SECTOR_DEAL;
		}
		if (tmp_partdata_by_bytes > 0) {
			uint rest_sectors = (tmp_partdata_by_bytes + 511) >> 9;
			if (fat_fs_read(imgname, down_buffer, tmp_imgfile_start,
					rest_sectors * 512) !=
			    rest_sectors * 512) {
				printf("sunxi sprite error : read sdcard start %d, total %d failed\n",
				       tmp_imgfile_start, rest_sectors * 512);

				goto __download_normal_part_err1;
			}
			if (sunxi_sprite_write(tmp_partstart_by_sector,
					       rest_sectors,
					       down_buffer) != rest_sectors) {
				printf("sunxi sprite error: download rawdata error %s, start 0x%x, sectors 0x%x\n",
				       part_info->dl_filename,
				       tmp_partstart_by_sector, rest_sectors);

				goto __download_normal_part_err1;
			}
		}
	} else {
		if (unsparse_direct_write(down_buffer, first_write_bytes)) {
			printf("sunxi sprite error: download sparse error %s\n",
			       part_info->dl_filename);

			goto __download_normal_part_err1;
		}
		tmp_partdata_by_bytes -= first_write_bytes;

		while (tmp_partdata_by_bytes >= AU_ONCE_DATA_DEAL) {
			if (fat_fs_read(imgname, down_buffer, tmp_imgfile_start,
					AU_ONCE_DATA_DEAL) !=
			    AU_ONCE_DATA_DEAL) {
				printf("sunxi sprite error : read sdcard start 0x%x, total 0x%x failed\n",
				       tmp_imgfile_start, AU_ONCE_DATA_DEAL);

				goto __download_normal_part_err1;
			}
			if (unsparse_direct_write(down_buffer,
						  AU_ONCE_DATA_DEAL)) {
				printf("sunxi sprite error: download sparse error %s\n",
				       part_info->dl_filename);

				goto __download_normal_part_err1;
			}
			tmp_imgfile_start += AU_ONCE_SECTOR_DEAL * 512;
			tmp_partdata_by_bytes -= AU_ONCE_DATA_DEAL;
		}
		if (tmp_partdata_by_bytes > 0) {
			uint rest_sectors = (tmp_partdata_by_bytes + 511) >> 9;
			if (fat_fs_read(imgname, down_buffer, tmp_imgfile_start,
					rest_sectors * 512) !=
			    rest_sectors * 512) {
				printf("sunxi sprite error : read sdcard start 0x%x, total 0x%x failed\n",
				       tmp_imgfile_start, rest_sectors * 512);

				goto __download_normal_part_err1;
			}
			if (unsparse_direct_write(down_buffer,
						  tmp_partdata_by_bytes)) {
				printf("sunxi sprite error: download sparse error %s\n",
				       part_info->dl_filename);

				goto __download_normal_part_err1;
			}
		}
	}
	sunxi_flash_flush();
	tick_printf("successed in writting part %s\n", part_info->name);
	ret = 0;
	if (imgitemhd) {
		Img_CloseItem(imghd, imgitemhd);
		imgitemhd = NULL;
	}
	/* verify */
	if (part_info->verify) {
		ret = -1;
		if (part_info->vf_filename[0]) {
			imgitemhd =
				Img_OpenItem(imghd, "RFSFAT16",
					     (char *)part_info->vf_filename);
			if (!imgitemhd) {
				printf("sprite update warning: open part %s failed\n",
				       part_info->vf_filename);

				goto __download_normal_part_err1;
			}
			if (!Img_Fat_ReadItem(imghd, imgitemhd, imgname,
					      (void *)verify_data, 1024)) {
				printf("sprite update warning: fail to read data from %s\n",
				       part_info->vf_filename);

				goto __download_normal_part_err1;
			}
			if (partdata_format == ANDROID_FORMAT_DETECT) {
				active_verify =
					sunxi_sprite_part_sparsedata_verify();
			} else {
				active_verify =
					sunxi_sprite_part_rawdata_verify(
						partstart_by_sector,
						partdata_by_byte);
			}
			tmp	   = (uint *)verify_data;
			origin_verify = *tmp;
			printf("origin_verify value = %x, active_verify value = %x\n",
			       origin_verify, active_verify);
			if (origin_verify != active_verify) {
				printf("origin checksum=%x, active checksum=%x\n",
				       origin_verify, active_verify);
				printf("sunxi sprite: part %s verify error\n",
				       part_info->dl_filename);

				goto __download_normal_part_err1;
			}
			ret = 0;
		} else {
			printf("sunxi sprite err: part %s unablt to find verify file\n",
			       part_info->dl_filename);
		}
		tick_printf("successed in verify part %s\n", part_info->name);
	} else {
		printf("sunxi sprite err: part %s not need to verify\n",
		       part_info->dl_filename);
	}

__download_normal_part_err1:
	if (imgitemhd) {
		Img_CloseItem(imghd, imgitemhd);
		imgitemhd = NULL;
	}

	return ret;
}

static int melis_download_udisk(dl_one_part_info *part_info, uchar *source_buff)
{
	HIMAGEITEM imgitemhd = NULL;
	u32 flash_sector;
	s64 packet_len;
	s32 ret = -1, ret1;

	imgitemhd =
		Img_OpenItem(imghd, "RFSFAT16", (char *)part_info->dl_filename);
	if (!imgitemhd) {
		printf("sunxi sprite error: open part %s failed\n",
		       part_info->dl_filename);

		return -1;
	}

	packet_len = Img_GetItemSize(imghd, imgitemhd);
	if (packet_len <= 0) {
		printf("sunxi sprite error: fetch part len %s failed\n",
		       part_info->dl_filename);

		goto __download_udisk_err1;
	}
	if (packet_len <= CONFIG_FW_BURN_UDISK_MIN_SIZE) {
		printf("download UDISK: the data length of udisk is too small, ignore it\n");

		ret = 1;
		goto __download_udisk_err1;
	}

	flash_sector = sunxi_sprite_size();
	if (!flash_sector) {
		printf("sunxi sprite error: download_udisk, the flash size is invalid(0)\n");

		goto __download_udisk_err1;
	}
	printf("the flash size is %d MB\n", flash_sector / 2 / 1024);
	part_info->lenlo = flash_sector - part_info->addrlo;
	part_info->lenhi = 0;
	printf("UDISK low is 0x%x Sectors\n", part_info->lenlo);
	printf("UDISK high is 0x%x Sectors\n", part_info->lenhi);

	ret = melis_download_normal_part(part_info, source_buff);
__download_udisk_err1:
	ret1 = Img_CloseItem(imghd, imgitemhd);
	if (ret1 != 0) {
		printf("sunxi sprite error: __download_udisk, close udisk image failed\n");

		return -1;
	}

	return ret;
}


static int melis_auto_update_deal_part(sunxi_download_info *dl_map)
{
	dl_one_part_info *part_info;
	int ret = -1;
	int ret1;
	int i		 = 0;
	uchar *down_buff = NULL;
	__maybe_unused int rate;

	if (!dl_map->download_count) {
		printf("sunxi sprite: no part need to write\n");
		return 0;
	}

	rate = (70 - 10) / dl_map->download_count;

	down_buff = (uchar *)memalign(CONFIG_SYS_CACHELINE_SIZE,
								AU_ONCE_DATA_DEAL + AU_HEAD_BUFF);
	if (!down_buff) {
		printf("sunxi sprite err: unable to malloc memory for sunxi_sprite_deal_part\n");
		goto __auto_update_deal_part_err1;
	}

	for (part_info = dl_map->one_part_info, i = 0;
					i < dl_map->download_count; i++, part_info++) {
		tick_printf("begin to download part %s\n", part_info->name);
		if (!strncmp("UDISK", (char *)part_info->name, strlen("UDISK"))) {
			ret1 = melis_download_udisk(part_info, down_buff);
			if (ret1 < 0) {
				printf("sunxi sprite err: sunxi_sprite_deal_part, download_udisk failed\n");
				goto __auto_update_deal_part_err1;
			} else if (ret1 > 0) {
				printf("do NOT need download UDISK\n");
			}
		} else if (strncmp("private", (char *)part_info->name, strlen("private")) == 0 ||
				strncmp("env", (char *)part_info->name, strlen("env")) == 0) {
			printf("IGNORE private part\n");
			/*private partition: check if need to burn private data*/
		} else {
			ret1 = melis_download_normal_part(part_info, down_buff);
			if (ret1 != 0) {
				printf("sunxi sprite err: sunxi_sprite_deal_part, download normal failed\n");
				goto __auto_update_deal_part_err1;
			}
		}
#ifdef CONFIG_SUNXI_SPRITE_CARTOON
		sprite_cartoon_upgrade(10 + rate * (i + 1));
#endif
		tick_printf("successed in download part %s\n", part_info->name);
	}
	ret = 0;
__auto_update_deal_part_err1:
	if (down_buff) {
		free(down_buff);
	}
	return ret;
}

#ifdef CONFIG_USB_STORAGE
extern int usb_stor_curr_dev;
static int sunxi_udisk_check(void)
{
	int ret;
	usb_stor_curr_dev = -1; /* current device */
	usb_stop();
	printf("sunxi_udisk_check...\n");
	ret = usb_init();
	if (ret == 0) {
		/* try to recognize storage devices immediately */
		usb_stor_curr_dev = usb_stor_scan(1);
	}
	return usb_stor_curr_dev;
}

static int detect_udisk(void)
{
	int ret;
	ret = sunxi_udisk_check();
	if (ret) {
		printf("No Udisk insert\n");
		sunxi_update_fail_exit();
	} else {
#ifdef CONFIG_SUNXI_SPRITE_LED
		sprite_led_init();
#endif
		printf("Udisk found,update image...\n");
		printf("detect update image:%s\n", IMG_NAME);
		ret = melis_auto_update_firmware_probe(IMG_NAME);
		if (ret == 0) {
			uboot_spare_head.boot_data.work_mode = WORK_MODE_UDISK_UPDATE;
		} else {
			printf("firmware not found will reboot\n");
			sunxi_update_fail_exit();
		}
	}
	return 0;
}
#endif
int melis_auto_update_check(void)
{
	printf("%s %d %s %s len:%ld\n", __FILE__, __LINE__, __func__, uboot_spare_head.boot_data.nand_spare_data, strlen(uboot_spare_head.boot_data.nand_spare_data));
	int workmode = uboot_spare_head.boot_data.work_mode;
	if ((workmode != WORK_MODE_USB_PRODUCT) && (workmode != WORK_MODE_CARD_PRODUCT)) {
#ifdef CONFIG_USB_STORAGE
		int ret = 0;
		ret = detect_udisk();
		return ret;
	}
#endif
	return 0;
}


static int sunxi_melis_auto_update_main(void)
{
	int ret = 0;
	/* uchar img_mbr[1024 * 1024]; */
#ifdef CONFIG_SUNXI_SPRITE_CARTOON
	int processbar_direct = 0;
#endif
	uchar *img_mbr = memalign(CONFIG_SYS_CACHELINE_SIZE, 1024 * 1024);
	sunxi_download_info *dl_map;
	dl_map = (sunxi_download_info *)memalign(CONFIG_SYS_CACHELINE_SIZE,
						 sizeof(sunxi_download_info));

	tick_printf("sunxi update begin\n");

	if (strlen(uboot_spare_head.boot_data.nand_spare_data) == 0) {
		printf("detect update image:%s\n", IMG_NAME);
		imgname = memalign(CONFIG_SYS_CACHELINE_SIZE, strlen(IMG_NAME) + 1);
		if (imgname == NULL) {
			printf("imgname : null\n");
			ret = -1;
			goto out;
		}
		strcpy(imgname, IMG_NAME);
	} else {
		printf("detect update image:%s\n", uboot_spare_head.boot_data.nand_spare_data);
		imgname = memalign(CONFIG_SYS_CACHELINE_SIZE, strlen(uboot_spare_head.boot_data.nand_spare_data) + 1);
		if (imgname == NULL) {
			printf("imgname : null\n");
			ret = -1;
			goto out;
		}
		strcpy(imgname, uboot_spare_head.boot_data.nand_spare_data);
	}
#ifdef CONFIG_SUNXI_SPRITE_CARTOON
	sprite_cartoon_create(processbar_direct);
#endif

	if (melis_auto_update_firmware_probe(imgname)) {
		printf("sunxi sprite firmware probe fail\n");
		ret = -1;
		goto out;
	}
#ifndef CONFIG_SUNXI_AUTO_UPDATE_ENV
	env_set("boot_partation", "NULL");
	env_save();
#endif

#ifdef CONFIG_SUNXI_SPRITE_CARTOON
	sprite_cartoon_upgrade(5);
#endif

	/* download dlmap file to get the download files*/
	tick_printf("fetch download map\n");
	if (melis_auto_update_fetch_download_map(dl_map)) {
		printf("sunxi sprite error : fetch download map error\n");
		ret = -1;
		goto out;
	}
	__dump_dlmap(dl_map);

#ifdef CONFIG_SUNXI_SPRITE_CARTOON
	sprite_cartoon_upgrade(10);
#endif
	tick_printf("begin to download part\n");
	/* start burning partition data*/
	if (melis_auto_update_deal_part(dl_map)) {
		printf("sunxi sprite error : download part error\n");
		ret = -1;
		goto out;
	}
	tick_printf("successed in downloading part\n");
#ifdef CONFIG_SUNXI_SPRITE_CARTOON
	sprite_cartoon_upgrade(100);
#endif
	tick_printf("update firmware success \n");
	mdelay(3000);
#ifndef CONFIG_SUNXI_AUTO_UPDATE_ENV
	env_set("boot_partation", "bootA");
	env_save();
#endif
	sunxi_update_success_exit();
out:
	if (dl_map) {
		free(dl_map);
		dl_map = NULL;
	}

	if (img_mbr) {
		free(img_mbr);
		img_mbr = NULL;
	}
	return ret;
}

int do_melis_auto_update_check(cmd_tbl_t *cmdtp, int flag, int argc,
			 char *const argv[])
{
#ifdef CONFIG_CMD_MMC
	if (!run_command("sunxi_card0_probe", 0)) {
		strncpy(interface, "mmc", sizeof("mmc"));
	} else
#endif
	if (!run_command("usb reset", 0)) {
		strncpy(interface, "usb", sizeof("usb"));
	} else {
		return 0;
	}

	return sunxi_melis_auto_update_main();
}

U_BOOT_CMD(auto_update_check, CONFIG_SYS_MAXARGS, 1, do_melis_auto_update_check,
	"auto_update_v2   - Do TFCard of Udisk update\n",
	"\nnote:\n"
	"	- The auto_update.txt configuration file must be\n"
	"	added to the scripts directory of the U disk.\n"
	"	- The command after ‘#’ will be considered as a comment.\n"
	"	- All text & commands after ‘%’ are invalid\n"
	"	auto_update.txt format:\n"
	"		sunxi_flash write <file path> <load partition>\n"
	"	exp:\n"
	"		sunxi_flash write update/boot.fex boot\n"
	"		sunxi_flash write update/boot_package.fex boot_package\n");

