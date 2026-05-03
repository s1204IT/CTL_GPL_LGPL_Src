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

#include "sprite_common.h"
#include "./firmware/imgdecode.h"
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void __dump_dlmap(sunxi_download_info *dl_info)
{
	dl_one_part_info *part_info;
	u32 i;
	char buffer[32];

	printf("*************DOWNLOAD MAP DUMP************\n");
	printf("total download part %d\n", dl_info->download_count);
	printf("\n");
	for (part_info = dl_info->one_part_info, i = 0;
	     i < dl_info->download_count; i++, part_info++) {
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->name, 16);
		printf("download part[%d] name          :%s\n", i, buffer);
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->dl_filename, 16);
		printf("download part[%d] download file :%s\n", i, buffer);
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->vf_filename, 16);
		printf("download part[%d] verify file   :%s\n", i, buffer);
		printf("download part[%d] lenlo         :0x%x\n", i,
		       part_info->lenlo);
		printf("download part[%d] addrlo        :0x%x\n", i,
		       part_info->addrlo);
		printf("download part[%d] encrypt       :0x%x\n", i,
		       part_info->encrypt);
		printf("download part[%d] verify        :0x%x\n", i,
		       part_info->verify);
		printf("\n");
	}
}
