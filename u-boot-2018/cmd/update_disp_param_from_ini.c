/*
 * Allwinner do disp param update from ini in uboot
 *
 * (C) Copyright 2023  <liujuan1@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <fs.h>
#include <common.h>
#include <environment.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sunxi_disp_param_from_ini.h>
#include <sunxi_display2.h>

#if defined(CONFIG_AIOT_DISP_PARAM_UPDATE) && defined(CONFIG_FAT_WRITE)
#define PARAM_SIZE  10240
#define MAX_KEY_COUNT 1000
#define MAX_GROUP_LENGTH 50

struct key_info group_info[MAX_KEY_COUNT];
extern int sunxi_partition_get_partno_byname(const char *part_name);

void remove_spaces(char *str)
{
	char *src = str;
	char *dst = str;
	while (*src) {
		if (!isspace((unsigned char)*src)) {
			*dst = *src;
			dst++;
		}
		src++;
	}

	*dst = '\0';
}

int parse_group_string(const char *group_string)
{
	int count = 0;
	const char *delimiter = "\n";

	memset(group_info, '\0', sizeof(group_info));
	char *line = strtok((char *)group_string, delimiter);

	while (line != NULL && count < MAX_KEY_COUNT) {
		char *equal_sign = strchr(line, '=');
		if (equal_sign != NULL) {
			strncpy(group_info[count].key_name, line, equal_sign - line);
			group_info[count].key_name[equal_sign - line] = '\0';
			strcpy(group_info[count].key_value, equal_sign + 1);

			remove_spaces(group_info[count].key_name);
			remove_spaces(group_info[count].key_value);

			count++;
		}
		line = strtok(NULL, delimiter);
	}
	return count;
}

char *get_file_from_partiton(char *file_name, char *part_name)
{

	char *data;
	int partno = -1;
	loff_t data_len;
	char part_info[16] = { 0 };

	snprintf(part_info, 16, "0:%x", partno);
	partno = sunxi_partition_get_partno_byname(part_name);
	if (partno < 0) {
		pr_err("boot-resource is not found!\n");
		return NULL;
	}
	/* must free after use by user */
	data = malloc(PARAM_SIZE);

	fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
	fs_read(file_name, (ulong)data, 0, 0, &data_len);
	if (PARAM_SIZE < data_len) {
		pr_err("file:%s size over %d, please modify code\n", file_name, PARAM_SIZE);
		free(data);
		return NULL;
	}
	data[data_len] = '\0';
	if (data_len)
		return data;
	else {
		free(data);
		return NULL;
	}
}

struct key_info *ini_get_group_info(char *group_name, const char *data)
{
	char *line;
	char *temp;
	char *mark;
	char *sp = "\n";
	char *group_string;
	int found_group = 0;
	char current_group[MAX_GROUP_LENGTH] = "";

	if (!data) {
		pr_err("input file is NULL!\n");
		return NULL;
	}

	group_string = malloc(PARAM_SIZE);
	if (!group_string) {
		pr_err("fail to malloc!\n");
		return NULL;
	}

	mark = malloc(PARAM_SIZE);
	if (!mark) {
		pr_err("fail to malloc!\n");
		free(group_string);
		return NULL;
	}
	strcpy(mark, data);
	line = strtok(mark, sp);
	if (line[strlen(line)-1] == '\r')
		sp = "\r\n";

	memset(group_string, 0, strlen(group_string));
	while (line != NULL) {
		if (line[0] == '[') {
			if (found_group) {
				break;
			}
			memset(current_group, 0, sizeof(current_group));
			strncpy(current_group, line + 1, strcspn(line + 1, "]"));
			current_group[strcspn(current_group, "\r")] = '\0';
			if (strcmp(current_group, group_name) == 0) {
				found_group = 1;
			} else {
				found_group = 0;
			}
		} else if (found_group) {
			temp = group_string;
			sprintf(group_string, "%s\n%s", temp, line);
		}

		line = strtok(NULL, sp);
	}
	if (found_group) {
		found_group = parse_group_string(group_string);
		free(group_string);
		free(mark);
		return group_info;
	} else {
		pr_warn("Group [%s] not found\n", group_name);
		memset(group_info, '\0', sizeof(group_info));
		free(group_string);
		free(mark);
		return NULL;
	}
}

char *ini_get_string(struct key_info *p, char *key_name)
{
	for (int i = 0; i < MAX_KEY_COUNT; i++) {
		if (strcmp(p[i].key_name, key_name) == 0) {
			return p[i].key_value;
		}
	}
	pr_err("can not find key:%s\n", key_name);
	return NULL;
}

int ini_get_int(struct key_info *p, char *key_name)
{
	const char *ret;

	ret = ini_get_string(p, key_name);
	if (ret)
		return simple_strtol(ret, NULL, 10);
	else
		return -1;
}

int get_struct_count(struct key_info *group_info)
{
	int valid_count = 0;
	for (int i = 0; i < MAX_KEY_COUNT; i++) {
		if (group_info[i].key_name[0] != '\0' || group_info[i].key_value[0] != '\0') {
			valid_count++;
		} else {
			break;
		}
	}
	return valid_count;
}

static int do_disp_param(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 4)
		return -1;
	struct key_info *p;
	char *temp;
	char *data;
	int i;

	if (strcmp(argv[1], "get_group") == 0 && argc == 4) {
		printf("get group info from ini...\n");
		data = get_file_from_partiton(argv[2], "boot-resource");
		p = ini_get_group_info(argv[3], data);
		int struct_count;
		struct_count = get_struct_count(p);
		for (i = 0; i < struct_count; i++) {
			printf("Key: %s, Value: %s\n", p[i].key_name, p[i].key_value);
		}
	} else if (strcmp(argv[1], "get_int") == 0 && argc == 5) {
		printf("get int from ini...\n");
		data = get_file_from_partiton(argv[2], "boot-resource");
		p = ini_get_group_info(argv[3], data);
		i = ini_get_int(p, argv[4]);
		printf("value:%d\n", i);
	} else if (strcmp(argv[1], "get_string") == 0 && argc == 5) {
		printf("get string from ini...\n");
		data = get_file_from_partiton(argv[2], "boot-resource");
		p = ini_get_group_info(argv[3], data);
		temp = ini_get_string(p, argv[4]);
		printf("value:%s\n", temp);
	} else {
		printf("do not support this command!\n");
		return 0;
	}
	free(data);
	return 0;
}

U_BOOT_CMD(
		disp_param_ini,	5,	1,	do_disp_param,
		"some test function for sunxi display param from ini",
		"get_group/get_int/get_string"
);

#endif
