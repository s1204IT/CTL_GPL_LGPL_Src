// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <sunxi_image_verifier.h>
#include <fdt_support.h>
#include <sunxi_board.h>
#include <sys_partition.h>
#include <sunxi_flash.h>
#include <elf.h>
#include <asm/arch/ce.h>

#ifdef CONFIG_BOOT_RISCV
#define BYTES_PER_BLOCK					(512)

extern int sunxi_riscv_init(u32 img_addr, u32 run_ddr, u32 riscv_id);

static int sunxi_flash_read_part(struct blk_desc *desc, disk_partition_t *info,
				 unsigned long buffer, unsigned long rblock)
{
	int ret;
	unsigned long start_block;
	uint8_t *addr;

	addr = (uint8_t *)buffer;
	start_block = (unsigned long)info->start;

	ret = blk_dread(desc, start_block, rblock, addr);

	debug("sunxi flash read :offset %lx, %ld blocks %s\n",
					(unsigned long)info->start,
					rblock, (ret == rblock) ? "OK" : "ERROR");

	return ret;
}

static int do_sunxi_flash_read(const char *part_name,
				unsigned long addr, unsigned long size)
{
	struct blk_desc *desc;
	disk_partition_t info = { 0 };
	int ret;

	desc = blk_get_devnum_by_typename("sunxi_flash", 0);
	if (desc == NULL) {
		pr_err("Can't get blk sunxi_flash\r\n");
		return -ENODEV;
	}

	ret = sunxi_flash_try_partition(desc, part_name, &info);
	if (ret < 0) {
		pr_err("Can't find %s partition\r\n", part_name);
		return -ENODEV;
	}

	if (info.size * BYTES_PER_BLOCK > size) {
		pr_err("temporary memory too small, require 0x%lx\r\n", info.size);
		return -ENOMEM;
	}

	pr_msg("partinfo: name %s, start 0x%lx, size 0x%lx\n", info.name,
	       info.start, info.size * BYTES_PER_BLOCK);
	ret = sunxi_flash_read_part(desc, &info, addr, info.size);
	if (ret > 0)
		ret *= BYTES_PER_BLOCK;

	return ret;
}
#endif

#ifdef CONFIG_SUNXI_VERIFY_RV
static int do_sunxi_flash_write(const char *part_name,
				unsigned long addr, unsigned long size)
{
	struct blk_desc *desc;
	disk_partition_t info = { 0 };
	int ret, wblocks;

	desc = blk_get_devnum_by_typename("sunxi_flash", 0);
	if (desc == NULL) {
		pr_err("Can't get blk sunxi_flash\r\n");
		return -ENODEV;
	}

	ret = sunxi_flash_try_partition(desc, part_name, &info);
	if (ret < 0) {
		pr_err("Can't find %s partition\r\n", part_name);
		return -ENODEV;
	}

	wblocks = (size + BYTES_PER_BLOCK - 1) / BYTES_PER_BLOCK;
	if (wblocks > info.size) {
		pr_err("write data too large, max 0x%lx\r\n", info.size * BYTES_PER_BLOCK);
		return -ENOMEM;
	}

	return (sunxi_flash_write(info.start, wblocks, (void *)addr) == wblocks) ?
			0 : -EFAULT;
}

#define CHECK_ELF_MAGIC(ehdr) ((ehdr)->e_ident[EI_MAG0] == ELFMAG0 && \
		      (ehdr)->e_ident[EI_MAG1] == ELFMAG1 && \
		      (ehdr)->e_ident[EI_MAG2] == ELFMAG2 && \
		      (ehdr)->e_ident[EI_MAG3] == ELFMAG3)

static Elf32_Shdr *find_section(unsigned long img_addr, const char *seg)
{
	Elf32_Ehdr *ehdr = NULL;
	Elf32_Shdr *shdr = NULL;
	int i = 0;
	char *strtab = NULL;

	ehdr = (Elf32_Ehdr *)img_addr;
	shdr =  (Elf32_Shdr *)(img_addr + ehdr->e_shoff);

	if (!CHECK_ELF_MAGIC(ehdr)) {
		pr_err("invalid image header.\n");
		return NULL;
	}

	strtab = (char *)img_addr + shdr[ehdr->e_shstrndx].sh_offset;

	shdr = NULL;
	for (i = 0; i < ehdr->e_shnum; ++i) {
		shdr = (Elf32_Shdr *)(img_addr + ehdr->e_shoff
				+ (i * sizeof(Elf32_Shdr)));

		char *pstr = strtab + shdr->sh_name;

		if (!(shdr->sh_flags & SHF_ALLOC)
			|| (shdr->sh_addr == 0)
			|| (shdr->sh_size == 0)) {
			continue;
		}

		if (strcmp(pstr, seg) == 0)
			break;
	}

	return shdr;
}

static int check_image_data(const char *partition,
				unsigned long addr, unsigned long size)
{
	int ret;
	Elf32_Shdr *shdr;
	uint8_t src_digest[16], dst_digest[16];
	uint32_t img_size;

	ret = do_sunxi_flash_read(partition, addr, size);
	if (ret <= 0)
		return ret;

	/* format: 4bytes image size + 16bytes digest */
	shdr = find_section(addr, ".digest");
	if (!shdr) {
		printf("Can't find .digest section from %s\r\n", partition);
		pr_err("%s partition data damaged\r\n", partition);
		return -ENODEV;
	}
	printf("find .digest section from %s\r\n", partition);

	img_size = *(uint32_t *)(addr + shdr->sh_offset);
	printf("the img_size is %u\n", img_size);
	memcpy(src_digest, (void *)(addr + shdr->sh_offset + 4), 16);
	memset((void *)(addr + shdr->sh_offset), 0, 20);

	if (sunxi_md5_calc(dst_digest, 16, (uint8_t *)addr, img_size)) {
		pr_err("sunxi_md5_calc: failed\n");
		return -EFAULT;
	}

	/* restore digest data */
	memcpy((void *)(addr + shdr->sh_offset), &img_size, 4);
	memcpy((void *)(addr + shdr->sh_offset + 4), src_digest, 16);

	if (memcmp(src_digest, dst_digest, 16)) {
		pr_err("%s partition data damaged\r\n", partition);
		return -EFAULT;
	}

	return 0;
}

static int partition_copy(const char *src, const char *dst,
				unsigned long addr, unsigned long size)
{
	int ret;

	ret = do_sunxi_flash_read(src, addr, size);
	if (ret <= 0)
		return ret;

	return do_sunxi_flash_write(dst, addr, ret);
}
#endif

/*******************************************************************/
/* bootrv - boot application image from image in memory */
/*******************************************************************/
int do_bootrv(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	__attribute__((unused)) u32 riscv_id = 0;
	__attribute__((unused)) u32 mem_addr = 0;
	__attribute__((unused)) u32 mem_size = 0;
#ifdef CONFIG_SUNXI_VERIFY_RV
	int i;
#endif
#ifdef CONFIG_BOOT_RISCV
	mem_addr = simple_strtoul(argv[1], NULL, 16);
	mem_size = simple_strtoul(argv[2], NULL, 16);
	riscv_id = simple_strtoul(argv[3], NULL, 16);


	printf("temporary mem: 0x%lx+0x%lx\r\n", (long)mem_addr, (long)mem_size);
#ifdef CONFIG_SUNXI_VERIFY_RV
	for (i = 4; i < argc; i++) {
		if (!check_image_data(argv[i], mem_addr, mem_size))
			break;
	}

	if (i == argc) {
		pr_err("all partition is damaged.\r\n");
		printf("the argc is %d\n", argc);
		return 0;
	}
#else
	if (argc > 4) {
		if (do_sunxi_flash_read(argv[4], mem_addr, mem_size) <= 0)
			return -EBUSY;
	}
#endif

	sunxi_riscv_init(mem_addr, 0, riscv_id);

#ifdef CONFIG_SUNXI_VERIFY_RV
	if (i > 4) {
		int avail = i;
		for (i = 4; i < argc; i++) {
			if (i == avail)
				continue;

			printf("restore %s partition from %s partition\r\n",
							argv[i], argv[avail]);
			/* restore previous partition data */
			if (partition_copy(argv[avail], argv[i], mem_addr, mem_size))
				pr_err("restore %s partition failed.\r\n", argv[i]);
		}
	}
#endif
#else
	printf("Don't Support Any Riscv PlatForm.\n");
#endif
	return  0;
}

#ifdef CONFIG_SYS_LONGHELP
static char bootrv_help_text[] =
	"[addr [arg ...]]\n    - boot application image stored in memory\n"
	"\tpassing arguments 'arg ...'; when booting a rtos image,\n"
	"\t'arg[1]' can be the loader address of image\n"
	"\t'arg[2]' can be the run address of image\n"
	"\t'arg[3]' can be cpu id of the ip\n";
#endif

U_BOOT_CMD(
	bootrv,	CONFIG_SYS_MAXARGS,	1,	do_bootrv,
	"boot riscv image from memory", bootrv_help_text
);
