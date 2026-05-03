/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <android_image.h>
#include <rtos_image.h>
#include <sunxi_board.h>
#include <jffs2/jffs2.h>
#include <linux/mtd/aw-spinand.h>
#include <linux/mtd/aw-rawnand.h>
#include <console.h>
#include <private_uboot.h>

#if defined(CONFIG_AW_MTD_SPINAND) || defined(CONFIG_AW_MTD_RAWNAND)
#define spinand_to_mtd(spinand) (&spinand->mtd)
#define spinand_to_chip(spinand) (&spinand->chip)

static void nand_print_info(struct mtd_info *mtd, int storage_type, void *chip_info)
{
	if (storage_type == STORAGE_SPI_NAND) {
		struct aw_spinand_chip *chip = (struct aw_spinand_chip *)chip_info;
		printf("========== arch info ==========\n");
		printf("Model:               %s\n", chip->info->phy_info->Model);
		printf("DieCntPerChip:       %u\n", chip->info->phy_info->DieCntPerChip);
		printf("BlkCntPerDie:        %u\n", chip->info->phy_info->BlkCntPerDie);
		printf("PageCntPerBlk:       %u\n", chip->info->phy_info->PageCntPerBlk);
		printf("SectCntPerPage:      %u\n", chip->info->phy_info->SectCntPerPage);
		printf("OobSizePerPage:      %u\n", chip->info->phy_info->OobSizePerPage);
		printf("BadBlockFlag:        0x%x\n", chip->info->phy_info->BadBlockFlag);
		printf("OperationOpt:        0x%x\n", chip->info->phy_info->OperationOpt);
		printf("MaxEraseTimes:       %d\n", chip->info->phy_info->MaxEraseTimes);
		printf("EccFlag:             0x%x\n", chip->info->phy_info->EccFlag);
		printf("EccType:             %d\n", chip->info->phy_info->EccType);
		printf("EccProtectedType:    %d\n", chip->info->phy_info->EccProtectedType);
		printf("================================\n");
	} else if (storage_type == STORAGE_NAND) {
		struct aw_nand_chip *chip = (struct aw_nand_chip *)chip_info;
		printf("========== arch info ==========\n");
		printf("chip: row_cycles@%d\n", chip->row_cycles);
		printf("chip: chip_shift@%d\n", chip->chip_shift);
		printf("chip: pagesize_mask@%d\n", chip->pagesize_mask);
		printf("chip: chip_pages@%d\n", chip->chip_pages);
		printf("chip: pagesize_shift@%d\n", chip->pagesize_shift);
		printf("chip: erase_shift@%d\n", chip->erase_shift);
		printf("chip: ecc_mode@%d\n", chip->ecc_mode);
		printf("chip: clk_rate@%d(MHz)\n", chip->clk_rate);
		printf("chip: avalid_sparesize@%d\n", chip->avalid_sparesize);
		printf("chip: pages_per_blk_shift_shift@%d\n", chip->pages_per_blk_shift);
		printf("================================\n");
	} else {
		printf("%s error\n", __func__);
		return;
	}

	if (mtd) {
		printf("mtd : type@%s\n", mtd->type == MTD_MLCNANDFLASH ? "MLCNAND" : "SLCNAND");
		printf("mtd : flags@nand flash\n");
		printf("mtd : writesize@%u\n", mtd->writesize);
		printf("mtd : oobsize@%d\n", mtd->oobsize);
		printf("mtd : size@%llu\n", mtd->size);
		printf("mtd : bitflips_threshold@%u\n", mtd->bitflip_threshold);
		printf("mtd : ecc_strength@%u\n", mtd->ecc_strength);
		printf("================================\n");
	}

	return;
}

static int do_nand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmd = argv[1];
	int storage_type = uboot_spare_head.boot_data.storage_type;
	struct mtd_info *mtd = NULL;
	loff_t addr;
	void *chip = NULL;
	int ret = 0;
	uint64_t op_size, phy_block_size;
	loff_t part_offset = 0;

#ifdef CONFIG_AW_MTD_SPINAND
	if (storage_type == STORAGE_SPI_NAND) {
		struct aw_spinand *spinand = get_spinand();
		chip = (void *)spinand_to_chip(spinand);
		mtd = spinand_to_mtd(spinand);
		part_offset = spinand_sys_part_offset();
	}
#endif
#ifdef CONFIG_AW_MTD_RAWNAND
	if (storage_type == STORAGE_NAND) {
		chip = (void *)get_rawnand();
		mtd = awnand_chip_to_mtd((struct aw_nand_chip *)chip);
	}
#endif
	if (!mtd) {
		printf("Non-mtd devices art not support\n");
		return -1;
	}

	phy_block_size = mtd->erasesize / 2;

	if (strcmp(cmd, "info") == 0) {
		putc('\n');

		nand_print_info(mtd, storage_type, chip);
		return 0;
	}

	if (strncmp(cmd, "read", 4) == 0 || strncmp(cmd, "write", 5) == 0) {
		size_t rwsize = 0;
		int read = strncmp(cmd, "read", 4) == 0; /* 1 = read, 0 = write */;
		int args = 4;
		char *part_name = NULL, *s;
		disk_partition_t info = { 0 };
		ulong load_addr;

		load_addr = (ulong)simple_strtoul(argv[2], NULL, 16);
		if (argc == 5)
			rwsize = (size_t)simple_strtoul(argv[4], NULL, 16);

		s = strchr(cmd, '.');
		if (s && !strncmp(s, ".part", 5)) {
			part_name = argv[3];
			ret = sunxi_partition_parse(part_name, &info);
			if (ret < 0)
				return -1;
			if (rwsize)
				info.size = rwsize;
			info.size = (info.size + 511) / 512;
		} else {
			args++;
			info.start = (ulong)simple_strtoul(argv[3], NULL, 16);
			info.size = (rwsize + 511) / 512;
		}

		if (argc < args)
			goto usage;

		printf("\nNAND %s: ", read ? "read" : "write");

		if (read)
			ret = sunxi_flash_phyread(info.start, info.size, (void *)load_addr);
		else
			ret = sunxi_flash_phywrite(info.start, info.size, (void *)load_addr);


		printf(" %lu bytes %s: %s\n", info.size * 512,
		       read ? "read" : "written", ret ? "ERROR" : "OK");

		return ret == 0 ? 0 : 1;
	}

	/*
	 * Syntax is:
	 *   0    1     2       3             4
	 *   nand erase [clean] off|partition [size]
	 */
	if (strncmp(cmd, "erase", 5) == 0 || strncmp(cmd, "scrub", 5) == 0) {
		/* "clean" at index 2 means request to write cleanmarker */
		int clean = argc > 2 && !strcmp("clean", argv[2]);
		int scrub_yes = argc > 2 && !strcmp("-y", argv[2]);
		int args = (clean || scrub_yes) ? 4 : 3;
		int scrub = !strncmp(cmd, "scrub", 5);
		char *part_name = NULL;
		disk_partition_t info = { 0 };
		struct erase_info instr;
		const char *scrub_warn =
			"Warning: "
			"scrub option will erase all factory set bad blocks!\n"
			"         "
			"There is no reliable way to recover them.\n"
			"         "
			"Use this command only for testing purposes if you\n"
			"         "
			"are sure of what you are doing!\n"
			"\nReally scrub this NAND flash? <y/N>\n";

		/*
		 * Don't allow missing arguments to cause full chip/partition
		 * erases -- easy to do accidentally, e.g. with a misspelled
		 * variable name.
		 */
		if (argc < args)
			goto usage;

		if (!strcmp(&cmd[5], ".part")) {
			part_name = argv[args -1];
			ret = sunxi_partition_parse(part_name, &info);
			if (ret < 0)
				return -1;
			if (argc == args + 1)
				info.size = (lbaint_t)simple_strtoul(argv[4], NULL, 16);
		} else {
			info.start = (lbaint_t)simple_strtoul(argv[args - 1], NULL, 16);
			if (argc == args + 1)
				info.size = (lbaint_t)simple_strtoul(argv[args], NULL, 16);
			else {
				if (info.start >= part_offset)
					info.size = mtd->erasesize;
				else
					info.size = phy_block_size;
			}
		}

		printf("\nNAND %s: ", cmd);

		memset(&instr, 0, sizeof(instr));
		instr.mtd = mtd;
		instr.addr = info.start;
		instr.len = info.size;
		instr.callback = NULL;

		if (scrub | clean) {
			if (scrub_yes) {
				ret = mtd_erase(mtd, &instr);
				goto erase_end;
			} else {
				puts(scrub_warn);
				if (confirm_yesno()) {
					ret = mtd_erase(mtd, &instr);
					goto erase_end;
				} else {
					puts("scrub aborted\n");
					return 1;
				}
			}
		}

		do {
			if (instr.addr >= part_offset)
				op_size = mtd->erasesize;
			else
				op_size = phy_block_size;

			if (mtd_block_isbad(mtd, instr.addr)) {
				pr_err("phy-block %lld is bad:", instr.addr / phy_block_size);
				instr.addr += op_size;
				continue;
			}

			instr.len = op_size;
			mtd_erase(mtd, &instr);
			info.size = info.size <= op_size ? 0 : (info.size - op_size);
			instr.addr += op_size;
		} while (info.size);

erase_end:
		printf("%s\n", ret ? "ERROR" : "OK");

		return ret == 0 ? 0 : 1;
	}

	if (strcmp(cmd, "bad") == 0) {
		for (addr = 0; addr < mtd->size; addr += phy_block_size)
			if (mtd_block_isbad(mtd, addr))
				printf("phy-block %lld is bad\n", addr / phy_block_size);
		return 0;
	}

	if (strcmp(cmd, "markbad") == 0) {
		argc -= 2;
		argv += 2;

		if (argc <= 0)
			goto usage;

		while (argc > 0) {
			addr = simple_strtoul(*argv, NULL, 16);

			if (mtd_block_markbad(mtd, addr)) {
				printf("phy-block 0x%08llx NOT marked "
					"as bad! ERROR %d\n",
					addr, ret);
				ret = 1;
			} else {
				printf("phy-block 0x%08llx successfully "
					"marked as bad\n",
					addr / phy_block_size);
			}
			--argc;
			++argv;
		}
		return ret;
	}
usage:
	return CMD_RET_USAGE;
}
#else
static int do_nand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}
#endif
static char nand_help_text[] =
	"info - show available NAND devices\n"
	"nand read[.part] - mem_addr off|partition size(Sector alignment)\n"
	"nand write[.part] - mem_addr off|partition size(Sector alignment)\n"
	"    read/write 'size' bytes starting at offset 'off'\n"
	"    to/from memory address 'addr', skipping bad blocks.\n"
	"nand erase [clean] off [size] - erase 'size' bytes "
	"    from offset 'off'\n"
	"    'size' includes skipped bad blocks.\n"
	"nand erase.part [clean] partition - erase entire mtd partition'\n"
	"nand scrub [-y] off [size]\n"
	"    really clean NAND erasing bad blocks (UNSAFE)\n"
	"nand scrub.part [-y] partition - erase entire mtd partition'\n"
	"nand bad - show bad blocks\n"
	"nand markbad off [...] - mark bad block(s) at offset (UNSAFE)\n"
	"";

U_BOOT_CMD(
	nand, CONFIG_SYS_MAXARGS, 1, do_nand,
	"NAND sub-system", nand_help_text
);

#if defined(CONFIG_AW_MTD_SPINAND) || defined(CONFIG_AW_MTD_RAWNAND)
static int nand_load_image(cmd_tbl_t *cmdtp, struct part_info *part,
			ulong addr, char *cmd)
{
	int ret;
	size_t len;
	image_header_t *uz_hdr;

#ifdef CONFIG_SUNXI_RTOS
	struct rtos_img_hdr *rtos_hdr;
	rtos_hdr = (struct rtos_img_hdr *)addr;
#endif

#ifdef CONFIG_ANDROID_BOOT_IMAGE
	struct andr_img_hdr *fb_hdr;
	fb_hdr = (struct andr_img_hdr *)addr;
#endif

	printf("\nLoading from %s\n", part->name);

	ret = sunxi_flash_phyread(part->offset, 4, (u_char *)addr);
	if (ret) {
		puts("** Read error\n");
		return 1;
	}

	uz_hdr = (image_header_t *)addr;
	if (image_check_magic(uz_hdr))
		len = image_get_data_size(uz_hdr) + image_get_header_size();
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	else if (!memcmp(fb_hdr->magic, ANDR_BOOT_MAGIC, 8)) {
		len = android_image_get_end(fb_hdr) - (ulong)fb_hdr;

		/*secure boot img may attached with an embbed cert*/
		len += sunxi_boot_image_get_embbed_cert_len(fb_hdr);
	}
#endif
#ifdef CONFIG_SUNXI_RTOS
	else if (!memcmp(rtos_hdr->rtos_magic, RTOS_BOOT_MAGIC, 8)) {
		len = sizeof(struct rtos_img_hdr) + rtos_hdr->rtos_size;
	}
#endif
	else {
		debug("bad boot image magic, maybe not a boot.img?\n");
		len = part->size;
	}

	ret = sunxi_flash_phyread(part->offset, len >> 9, (u_char *)addr);
	if (ret) {
		puts("** Read error\n");
		return 1;
	}

	/* Loading ok, update default load address */

	load_addr = addr;

	return bootm_maybe_autostart(cmdtp, cmd);
}

static int do_nandboot(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	char *boot_device = NULL;
	int idx;
	ulong addr, offset = 0;
	char *name;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
#if defined(CONFIG_CMD_MTDPARTS)
	if (argc >= 2) {
		name = (argc == 2) ? argv[1] : argv[2];
		if (!(str2long(name, &addr)) && (mtdparts_init() == 0) &&
		    (find_dev_and_part(name, &dev, &pnum, &part) == 0)) {
			if (dev->id->type != MTD_DEV_TYPE_NAND) {
				puts("Not a NAND device\n");
				return CMD_RET_FAILURE;
			}
			if (argc > 3)
				goto usage;
			if (argc == 3)
				addr = simple_strtoul(argv[1], NULL, 16);
			else
				addr = CONFIG_SYS_LOAD_ADDR;

			return nand_load_image(cmdtp, part, addr, argv[0]);
		}
	}
#endif

	switch (argc) {
	case 1:
		addr = CONFIG_SYS_LOAD_ADDR;
		boot_device = env_get("bootdevice");
		break;
	case 2:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = env_get("bootdevice");
		break;
	case 3:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		break;
	case 4:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		offset = simple_strtoul(argv[3], NULL, 16);
		break;
	default:
#if defined(CONFIG_CMD_MTDPARTS)
usage:
#endif
		return CMD_RET_USAGE;
	}

	if (!boot_device) {
		puts("\n** No boot device **\n");
		return CMD_RET_USAGE;
	}

	idx = simple_strtoul(boot_device, NULL, 16);

	if (mtdparts_init()) {
		puts("mtdpart init error\n");
		return CMD_RET_FAILURE;
	}
	name = sunxi_get_mtdparts_name(idx);

	if (find_dev_and_part(name, &dev, &pnum, &part))
		return CMD_RET_FAILURE;

	if (dev->id->type != MTD_DEV_TYPE_NAND) {
		puts("Not a NAND device\n");
		return CMD_RET_FAILURE;
	}

	/*
	 * Get partition offset data.
	 * The read cannot exceed the partition size.
	 */
	part->offset += offset;
	part->size -= offset;

	return nand_load_image(cmdtp, part, addr, argv[0]);
}
#else
static int do_nandboot(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	return 0;
}
#endif
U_BOOT_CMD(nboot, 4, 1, do_nandboot,
	"boot from AW NAND device",
	"[memaddr] <partition> | [[[loadAddr] dev] offset]"
);
