/*
 * (C) Copyright 2018 allwinnertech  <xulu@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * xulu@allwinnertech.com
 */

#include <common.h>
#include <asm/global_data.h>
#include <command.h>
#include <malloc.h>
#include <rtos_image.h>
#include <private_uboot.h>
#include <private_toc.h>
#include <sunxi_image_verifier.h>
#include <mapmem.h>
#include <bootm.h>
#include <memalign.h>
#include <sunxi_flash.h>
#include <sys_partition.h>
#include <lzma/LzmaTools.h>
#include <../../spl/include/private_atf.h>
DECLARE_GLOBAL_DATA_PTR;
static unsigned char *addr;
struct spare_rtos_head_t {
	struct spare_boot_ctrl_head    boot_head;
	uint8_t   rotpk_hash[32];
	unsigned int rtos_dram_size;     /* rtos dram size, passed by uboot*/
};

static int boot_flash_read(u32 start_sector, u32 blkcnt, void *buff)
{
    memcpy(buff, (void *)(addr + 512 * start_sector), 512 * blkcnt);
    return blkcnt;
}
static void update_opensbi_param(void *image_base)
{
	struct private_atf_head *opensbi_head = (struct private_atf_head *)image_base;

	if (strncmp((const char *)opensbi_head->magic, "opensbi", 7) == 0) {
		opensbi_head->platform[0] = 0x05;
		opensbi_head->platform[1] = 0x52;
		opensbi_head->platform[2] = 0x41;
		opensbi_head->platform[3] = 0x57;
		opensbi_head->platform[4] = 0x9d;
		opensbi_head->platform[5] = 0xe9;
		opensbi_head->platform[6] = 0x00;
		opensbi_head->platform[7] = 0x00;
	}
}
extern int sunxi_flash_read_part(struct blk_desc *desc, disk_partition_t *info,
				 ulong buffer, ulong load_size);

static int do_sunxi_boot_melis(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = 0;
	void (*rtos_entry)(void);
	struct blk_desc *desc;
	disk_partition_t info = { 0 };
	sbrom_toc1_head_info_t	*boot_head = NULL;
	struct sbrom_toc1_item_info  *boot_item = NULL;
	u32  i = 0;

	desc = blk_get_devnum_by_typename("sunxi_flash", 0);
	if (!desc)
		return -ENODEV;

	ret = sunxi_flash_try_partition(desc, "bootA", &info);
	if (ret < 0)
		return -ENODEV;
	addr = (unsigned char *)malloc(info.size * 512);
	sunxi_flash_read_part(desc, &info, (unsigned long)addr, 0);
	boot_head = (struct sbrom_toc1_head_info *)addr;
	if (boot_head->magic != TOC_MAIN_INFO_MAGIC) {
		printf("boot magic error\n");
	}
#ifdef BOOT_DEBUG
	printf("The size of toc is %x.\n", boot_head->valid_len);
	printf("*******************TOC1 Head Message*************************\n");
	printf("Toc_name		  = %s\n",	 boot_head->name);
	printf("Toc_magic		  = 0x%x\n", boot_head->magic);
	printf("Toc_add_sum 	  = 0x%x\n", boot_head->add_sum);
	printf("Toc_serial_num	  = 0x%x\n", boot_head->serial_num);
	printf("Toc_status		  = 0x%x\n", boot_head->status);
	printf("Toc_items_nr	  = 0x%x\n", boot_head->items_nr);
	printf("Toc_valid_len	  = 0x%x\n", boot_head->valid_len);
	printf("TOC_MAIN_END	  = 0x%x\n", boot_head->end);
	printf("***************************************************************\n\n");
#endif
    boot_item = (struct sbrom_toc1_item_info *)(addr + sizeof(struct sbrom_toc1_head_info));
	for (i = 0; i < boot_head->items_nr; i++, boot_item++) {
#ifdef BOOT_DEBUG
		printf("\n*******************TOC1 Item Message*************************\n");
		printf("Entry_name		  = %s\n",	 boot_item->name);
		printf("Entry_data_offset = 0x%x\n", boot_item->data_offset);
		printf("Entry_data_len	  = 0x%x\n", boot_item->data_len);
		printf("encrypt 		  = 0x%x\n", boot_item->encrypt);
		printf("Entry_type		  = 0x%x\n", boot_item->type);
		printf("run_addr		  = 0x%x\n", boot_item->run_addr);
		printf("index			  = 0x%x\n", boot_item->index);
		printf("Entry_end		  = 0x%x\n", boot_item->end);
		printf("***************************************************************\n\n");
#endif
		if (strncmp(boot_item->name, "melis-lzma", strlen("melis-lzma")) == 0) {
			size_t dst_len = ~0U;
			int ret = lzmaBuffToBuffDecompress((unsigned char *)0x40000000, (SizeT *)&dst_len, (addr + boot_item->data_offset), boot_item->data_len);
			if (ret) {
				printf("Error: lzmaBuffToBuffDecompress returned %d\n", ret);
				return -1;
			}
			update_opensbi_param((void *)0x40000000);
		} else if (strncmp(boot_item->name, "melis-config", strlen("melis-config")) == 0) {
			boot_flash_read(boot_item->data_offset/512, (boot_item->data_len+511)/512, (void *)0x43000200);
		} else {
			printf("unknow boot package file \n");
			return -1;
		}
	}
	board_quiesce_devices();
	cleanup_before_linux();
	rtos_entry = (void (*)(void))0x40000000;
	rtos_entry();
	return 0;
}
U_BOOT_CMD(
	boot_melis,	3,	1,	do_sunxi_boot_melis,
	"boot rtos",
	"rtos_gz_addr rtos_addr"
);
