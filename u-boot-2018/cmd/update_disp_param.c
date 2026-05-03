/*
 * Allwinner sun55iw3 do disp param update in uboot
 *
 * (C) Copyright 2023  <hongyaobin@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <stdio.h>
#include <stdlib.h>
#include <fs.h>
#include <common.h>
#include <environment.h>
#include <malloc.h>
#include <sunxi_disp_param.h>
#include <sunxi_display2.h>
#include <iniparser.h>

#if defined(CONFIG_AIOT_DISP_PARAM_UPDATE) && defined(CONFIG_FAT_WRITE)
extern int do_fat_fswrite(cmd_tbl_t *cmdtp, int flag, int argc,
			  char *const argv[]);

extern int sunxi_partition_get_partno_byname(const char *part_name);

#define PARAM_NUM	64
#define isblank(c)	(c == ' ' || c == '\t')
#define KEY_NUM		256
#define KEY_LENGTH	1024
#define INI_SIZE	(KEY_NUM * KEY_LENGTH)

static char *ini_name = "disp_config.ini";

void update_dict(dictionary *dst, dictionary *src, char *section_name, char **key, char *val)
{
	int i, key_num;

	key_num = iniparser_getsecnkeys(src, section_name);
	key = (char **)iniparser_getseckeys(src, section_name, (const char **)key);

	for (i = 0; i < key_num; i++) {
		val = (char *)iniparser_getstring(src, key[i], NULL);
		iniparser_set(dst, key[i], val);
	}
}

void param_compare(dictionary *dst, dictionary *src)
{
	int i, j;
	char *dst_section_name;
	char *src_section_name;
	char **key = malloc(KEY_NUM * KEY_LENGTH);
	char *val = malloc(KEY_LENGTH);

	int dst_section_num = iniparser_getnsec(dst);
	int src_section_num = iniparser_getnsec(src);

	for (i = 0; i < src_section_num; i++) {
		src_section_name = (char *)iniparser_getsecname(src, i);
		/* compare different ini file's section */
		for (j = 0; j < dst_section_num; j++) {
			dst_section_name = (char *)iniparser_getsecname(dst, j);
			/*
				the src ini file has the same section
				name as in dst ini file, just update it
				and begin next compare
			*/
			if (strcmp(src_section_name, dst_section_name) == 0) {
				update_dict(dst, src, src_section_name, key, val);
				break;
			} else {
				/*
					the section from the src ini file do no
					exit int the dst ini file, just copy it
					(make sure the current secton from
					the src file has already compared with
					all the sections int the dst ini file)
				*/
				if (j == (dst_section_num - 1)) {
					iniparser_set(dst, src_section_name, "UNDEF");
					update_dict(dst, src, src_section_name, key, val);
				} else
					continue;
			}
		}
	}
	free(key);
	free(val);
}

int parse_param_line(char *file_name, char **buf, int storage)
{
	char *data;
	char part_info[16] = { 0 };
	int partno = -1;
	loff_t data_len;
	char (*line)[KEY_NUM] = (char (*)[KEY_NUM])buf;
	int i = 0, count = 0, k = 0, j = 0;
	int ret = -1;

	partno = sunxi_partition_get_partno_byname("boot-resource"); /* for linux */
	if (partno < 0) {
		partno = sunxi_partition_get_partno_byname("bootloader"); /* for android */
		if (partno < 0) {
			pr_err("boot-resource or bootloader part is not found!\n");
			return ret;
		}
	}

	data = memalign(ARCH_DMA_MINALIGN, ALIGN(INI_SIZE, ARCH_DMA_MINALIGN));
	memset(data, 0, INI_SIZE);
	if (storage == 1) { /* read the cfg file to ram from usb storage */
		fs_set_blk_dev("usb", "0", FS_TYPE_ANY);
		fs_read(file_name, (ulong)data, 0, 0, &data_len);
	} else { /* read the cfg file to ram from flash */
		snprintf(part_info, 16, "0:%x", partno);
		fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
		fs_read(file_name, (ulong)data, 0, 0, &data_len);
	}

	data[data_len] = '\n';
	for (i = 0; data[i] != '\0'; i++) {
		count++;

		if (data[i] == '\n') {
			memset(line[j], 0, KEY_LENGTH); /* initialize the buf */
			strncpy(line[j], &data[k], count);
			count = 0;
			j++;
			k = i + 1;
		}
	}

	free(data);

	return j;
}

struct hdmi_spec hdmi_item[] = {
	{DISP_TV_MOD_480P, "720x480P60"},
	{DISP_TV_MOD_576P, "720x576P50"},

	{DISP_TV_MOD_720P_50HZ, "1280x720P50"},
	{DISP_TV_MOD_720P_60HZ, "1280x720P60"},
	{0xffff, "1280x720P24"},
	{0xffff, "1280x720P25"},
	{0xffff, "1280x720P30"},

	{DISP_TV_MOD_1080P_24HZ, "1920x1080P24"},
	{DISP_TV_MOD_1080P_25HZ, "1920x1080P25"},
	{DISP_TV_MOD_1080P_30HZ, "1920x1080P30"},
	{DISP_TV_MOD_1080I_50HZ, "1920x1080I50"},
	{DISP_TV_MOD_1080P_50HZ, "1920x1080P50"},
	{DISP_TV_MOD_1080I_60HZ, "1920x1080I60"},
	{DISP_TV_MOD_1080P_60HZ, "1920x1080P60"},

	{DISP_TV_MOD_3840_2160P_24HZ, "3840x2160P24"},
	{DISP_TV_MOD_3840_2160P_25HZ, "3840x2160P25"},
	{DISP_TV_MOD_3840_2160P_30HZ, "3840x2160P30"},
	{DISP_TV_MOD_3840_2160P_50HZ, "3840x2160P50"},
	{DISP_TV_MOD_3840_2160P_60HZ, "3840x2160P60"},

	{DISP_TV_MOD_4096_2160P_24HZ, "4096x2160P24"},
	{DISP_TV_MOD_4096_2160P_25HZ, "4096x2160P25"},
	{DISP_TV_MOD_4096_2160P_30HZ, "4096x2160P30"},
	{DISP_TV_MOD_4096_2160P_50HZ, "4096x2160P50"},
	{DISP_TV_MOD_4096_2160P_60HZ, "4096x2160P60"},
};

static int save_file_to_flash(char *name, int partno)
{
	int ret = -1;
	char *argv[6], addr[32];
	char part_info[16] = { 0 }, size[32] = { 0 };
	void *file_addr = NULL;
	loff_t read_len;

	file_addr = malloc(INI_SIZE);
	fs_set_blk_dev("usb", "0", FS_TYPE_ANY);
	ret = fs_read(name, (ulong)file_addr, 0, 0, &read_len); /* read the cfg file to ram from usb storage */
	if (ret < 0) {
		pr_err("read error!\n");
		free(file_addr);
		return ret;
	}

	snprintf(part_info, 16, "0:%x", partno);
	sprintf(addr, "%lx", (unsigned long)file_addr);
	snprintf(size, 16, "%lx", (unsigned long)read_len);

	argv[0] = "fatwrite";
	argv[1] = "sunxi_flash";
	argv[2] = part_info;
	argv[3] = addr;
	argv[4] = name;
	argv[5] = size;

	if (do_fat_fswrite(0, 0, 6, argv)) { /* copy the cfg file to the boot-resource block */
		pr_err("do_fat_fswrite fail!\n");
		ret = -1;
	} else
		ret = 0;

	free(file_addr);

	return ret;
}

static char *param_get(char *name, struct hsearch_data *htab)
{
	ENTRY e, *ep;

	e.key = name;
	e.data	= NULL;

	hsearch_r(e, FIND, &ep, htab, 0);

	return ep->data;
}

static int parse_disp_param(char *name, int partno, struct sunxi_disp_param *param)
{
	char part_info[16] = { 0 };
	void *data_buf = NULL;
	loff_t data_len;
	char *tmp;
	int i;

	struct hsearch_data disp_param_htab;
	memset(&disp_param_htab, 0, sizeof(disp_param_htab));
	hcreate_r(PARAM_NUM, &disp_param_htab); /* support PARAM_NUM items */

	data_buf = malloc(INI_SIZE);
	snprintf(part_info, 16, "0:%x", partno);

	fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
	fs_read(name, (ulong)data_buf, 0, 0, &data_len);

	if (himport_r(&disp_param_htab, (char *)data_buf,
		data_len, '\n', 0, 0, 0, NULL) == 0)
		pr_err("param import failed: errno = %d!\n", errno);

	param->cpu_id = param_get("cpu_id", &disp_param_htab);
	param->disp_main = param_get("disp_main", &disp_param_htab);
	param->disp_aux = param_get("disp_aux", &disp_param_htab);

	tmp = param_get("hdmi_output", &disp_param_htab);
	for (i = 0; i < (sizeof(hdmi_item) / sizeof(struct hdmi_spec)); i++) {
		if (!strcmp(hdmi_item[i].detail, tmp)) {
			param->hdmi_output = hdmi_item[i].index;
			break;
		} else
			param->hdmi_output = 0xffff; /* do not support */
	}

	param->disp_main_x = simple_strtoul(param_get("disp_main_x", &disp_param_htab), NULL, 10);
	param->disp_main_y = simple_strtoul(param_get("disp_main_y", &disp_param_htab), NULL, 10);
	param->disp_main_dclk_freq_khz = simple_strtoul(param_get("disp_main_dclk_freq_khz", &disp_param_htab), NULL, 10);
	param->disp_main_hbp = simple_strtoul(param_get("disp_main_hbp", &disp_param_htab), NULL, 10);
	param->disp_main_hspw = simple_strtoul(param_get("disp_main_hspw", &disp_param_htab), NULL, 10);
	param->disp_main_hfp = simple_strtoul(param_get("disp_main_hfp", &disp_param_htab), NULL, 10);
	param->disp_main_vbp = simple_strtoul(param_get("disp_main_vbp", &disp_param_htab), NULL, 10);
	param->disp_main_vspw = simple_strtoul(param_get("disp_main_vspw", &disp_param_htab), NULL, 10);
	param->disp_main_vfp = simple_strtoul(param_get("disp_main_vfp", &disp_param_htab), NULL, 10);

	param->disp_aux_x = simple_strtoul(param_get("disp_aux_x", &disp_param_htab), NULL, 10);
	param->disp_aux_y = simple_strtoul(param_get("disp_aux_y", &disp_param_htab), NULL, 10);
	param->disp_aux_hbp = simple_strtoul(param_get("disp_aux_hbp", &disp_param_htab), NULL, 10);
	param->disp_aux_hspw = simple_strtoul(param_get("disp_aux_hspw", &disp_param_htab), NULL, 10);
	param->disp_aux_hfp = simple_strtoul(param_get("disp_aux_hfp", &disp_param_htab), NULL, 10);
	param->disp_aux_vbp = simple_strtoul(param_get("disp_aux_vbp", &disp_param_htab), NULL, 10);
	param->disp_aux_vspw = simple_strtoul(param_get("disp_aux_vspw", &disp_param_htab), NULL, 10);
	param->disp_aux_vfp = simple_strtoul(param_get("disp_aux_vfp", &disp_param_htab), NULL, 10);

	param->lvds_lane_count = simple_strtoul(param_get("lvds_lane_count", &disp_param_htab), NULL, 10);
	param->edp_lane_count = simple_strtoul(param_get("edp_lane_count", &disp_param_htab), NULL, 10);
	param->mipi_lane_count = simple_strtoul(param_get("mipi_lane_count", &disp_param_htab), NULL, 10);
	param->vbo_lane_count = simple_strtoul(param_get("vbo_lane_count", &disp_param_htab), NULL, 10);
	param->lvds_color_depth = simple_strtoul(param_get("lvds_color_depth", &disp_param_htab), NULL, 10);
	param->edp_color_depth = simple_strtoul(param_get("edp_color_depth", &disp_param_htab), NULL, 10);
	param->lvds_color_depth = simple_strtoul(param_get("lvds_color_depth", &disp_param_htab), NULL, 10);
	param->mipi_color_depth = simple_strtoul(param_get("mipi_color_depth", &disp_param_htab), NULL, 10);
	param->vbo_color_depth = simple_strtoul(param_get("vbo_color_depth", &disp_param_htab), NULL, 10);

	param->lvds_fmt = simple_strtoul(param_get("lvds_fmt", &disp_param_htab), NULL, 10);
	param->lvds_reverse = simple_strtoul(param_get("lvds_reverse", &disp_param_htab), NULL, 10);
	param->backlight_level = simple_strtoul(param_get("backlight_level", &disp_param_htab), NULL, 10);
	param->backlight2_level = simple_strtoul(param_get("backlight2_level", &disp_param_htab), NULL, 10);
	param->backlight_freq = simple_strtoul(param_get("backlight_freq", &disp_param_htab), NULL, 10);
	param->backlight2_freq = simple_strtoul(param_get("backlight2_freq", &disp_param_htab), NULL, 10);
	param->backlight_delayms = simple_strtoul(param_get("backlight_delayms", &disp_param_htab), NULL, 10);
	param->backlight2_delayms = simple_strtoul(param_get("backlight2_delayms", &disp_param_htab), NULL, 10);
	param->fb0_width = simple_strtoul(param_get("fb0_width", &disp_param_htab), NULL, 10);
	param->fb0_height = simple_strtoul(param_get("fb0_height", &disp_param_htab), NULL, 10);
	param->backlight_ch = simple_strtoul(param_get("backlight_ch", &disp_param_htab), NULL, 10);
	param->backlight2_ch = simple_strtoul(param_get("backlight2_ch", &disp_param_htab), NULL, 10);

	hdestroy_r(&disp_param_htab);
	free(data_buf);

	return 0;
}

static void show_disp_param(void)
{
	char *data;
	int partno = -1;
	char part_info[16] = { 0 };
	loff_t data_len;

	data = malloc(INI_SIZE);
	partno = sunxi_partition_get_partno_byname("boot-resource");
	snprintf(part_info, 16, "0:%x", partno);

	fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
	fs_read(ini_name, (ulong)data, 0, 0, &data_len);
	data[data_len] = '\0';

	printf("------------------------------------------------\n");
	printf("%s\n", data);
	printf("------------------------------------------------\n");

	free(data);
}

int update_disp_param(struct sunxi_disp_param *param)
{
	int partno = -1, ret = -1;

	partno = sunxi_partition_get_partno_byname("boot-resource"); /* for linux */
	if (partno < 0) {
		partno = sunxi_partition_get_partno_byname("bootloader"); /* for android */
		if (partno < 0) {
			pr_err("boot-resource or bootloader part is not found!\n");
			return ret;
		}
	}

	if (save_file_to_flash(ini_name, partno) < 0) {
		pr_err("save_file_to_flash fail!\n");
		return ret;
	}

	ret = parse_disp_param(ini_name, partno, param);
	printf("--------- finish updating the display param to the sunxi flash ---------\n");

	return ret;
}

int copy_update_file(char *file_name)
{
	int partno = -1, ret = -1;
	int len[2];
	char part_info[16] = {0};
	int exist = 0;

	partno = sunxi_partition_get_partno_byname("boot-resource"); /* for linux */
	if (partno < 0) {
		partno = sunxi_partition_get_partno_byname("bootloader"); /* for android */
		if (partno < 0) {
			pr_err("boot-resource or bootloader part is not found!\n");
			return ret;
		}
	}

	snprintf(part_info, 16, "0:%x", partno);
	fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
	exist = fs_exists(file_name);

	snprintf(part_info, 16, "0:%x", partno);
	fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);

	if (exist) {
		char **flash_buf = (char **)malloc(INI_SIZE);
		char **usb_buf = (char **)malloc(INI_SIZE);

		len[0] = parse_param_line(file_name, flash_buf, 0);
		len[1] = parse_param_line(file_name, usb_buf, 1);

		dictionary *dict = NULL;
		dictionary *dict1 = NULL;

		dict = iniparser_load(file_name, flash_buf, len[0]);
		dict1 = iniparser_load(file_name, usb_buf, len[1]);

		param_compare(dict, dict1);

		snprintf(part_info, 16, "0:%x", partno);
		fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
		iniparser_dump_ini(dict, partno);

		dictionary_del(dict);
		dictionary_del(dict1);
		free(flash_buf);
		free(usb_buf);
	} else {
		if (save_file_to_flash(file_name, partno) < 0) {
			pr_err("copy update file fail!\n");
			return ret;
		}
	}

	printf("%s finish!\n", __func__);

	return 0;
}

static int do_disp_param(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
	return -1;

	if (strcmp(argv[1], "update") == 0) {
		struct sunxi_disp_param param;
		printf("update disp param...\n");
		update_disp_param(&param);
		return 0;
	} else if (strcmp(argv[1], "show") == 0) {
		printf("show disp param...\n");
		show_disp_param();
		return 0;
	} else {
		printf("do not support this command!\n");
		return 0;
	}
}

U_BOOT_CMD(
		disp_param,	2,	1,	do_disp_param,
		"some test function for sunxi display param",
		"update/show"
);

#endif
