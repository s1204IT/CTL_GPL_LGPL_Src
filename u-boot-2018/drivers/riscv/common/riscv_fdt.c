// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/riscv/common/riscv_fdt.c
 *
 * Copyright (c) 2007-2025 Allwinnertech Co., Ltd.
 * Author: wujiayi <wujiayi@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 */
#include <common.h>
#include <fdt_support.h>
#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <sys_config.h>
#include <fdt_support.h>

#include "imgdts.h"
#include "riscv_ic.h"
#include "riscv_fdt.h"
#include "elf_helpers.h"

#define UART_NUM (6)

int match_compatible(const char *path,
		     const char *compatible_str,
		     int *nodeoffset)
{
	int nodeoff = 0;
	char *pstr = NULL;
	int pstr_num = 0;

	RISCV_DEBUG("riscv:---- name [%s] msg -----\n", path);
	nodeoff = fdt_path_offset(working_fdt, path);
	if (nodeoff < 0) {
		RISCV_DEBUG("riscv:find [%s] node err\n", path);
		return -1;
	}

	pstr = (char *)fdt_getprop(working_fdt, nodeoff, "compatible", &pstr_num);
	if (pstr != NULL) {
		/* match string of compatible  */
		if (strcmp(pstr, compatible_str) == 0) {
			*nodeoffset = nodeoff;
			return 0;

		} else {
			RISCV_DEBUG("riscv:expect compatible:[%s], but use compatible:[%s]\n",\
						compatible_str, pstr);
			return -1;
		}

	} else {
		RISCV_DEBUG("riscv:find compatible err\n");
		return -1;
	}

	return -1;
}

int match_status(int nodeoffset)
{
	char *string_val = NULL;
	int ret = -1;

	fdt_getprop_string(working_fdt,
				nodeoffset,
				"status",
				&string_val);

	if (strcmp("okay", string_val) == 0)
		ret = 0;

	return ret;
}

int match_pinctrl_names(int nodeoffset)
{
	int ret = fdt_stringlist_search(working_fdt,
					 nodeoffset,
					"pinctrl-names",
					"default");
	return ret;
}

int dts_riscv_status(struct dts_msg_t *pmsg, u32 riscv_id)
{
	const char *compatible_str = RISCV_STATUS_STR;
	int nodeoff = 0;
	int ret = 0;
	char str[20];

	memset(str, 0, sizeof(str));
	sprintf(str, "riscv%d", riscv_id);
	ret = match_compatible(str, compatible_str, &nodeoff);
	if (ret < 0) {
		RISCV_DEBUG("riscv%d:dts no config [%s]\n", riscv_id, str);
		return -1;
	}

	RISCV_DEBUG("riscv%d:dts can find config [%s]\n", riscv_id, str);

	ret = match_status(nodeoff);
	if (ret < 0) {
		pmsg->riscv_status = DTS_CLOSE;
		RISCV_DEBUG("%s status is close\n", str);
		return -1;
	}
	pmsg->riscv_status = DTS_OPEN;
	return 0;
}



int riscv_dts_uart_msg(struct dts_msg_t *pmsg, u32 riscv_id)
{
	const char *compatible_str = RISCV_UART_STR;
	int i = 0;
	int nodeoff = 0;
	int ret = 0;
	int pin_count = 0;
	char str[20];
	user_gpio_set_t  pin_set[32];

	for (i = 0; i < UART_NUM; i++) {
		memset(str, 0, sizeof(str));
		sprintf(str, "serial%d", i);
		ret = match_compatible(str, compatible_str, &nodeoff);
		if (ret < 0) {
			continue;
		} else {
			RISCV_DEBUG("riscv%d:dts can find config [%s]\n",\
						riscv_id,\
						str);
			break;
		}
	}

	/*  no uart match ok */
	if (i == UART_NUM) {
		RISCV_DEBUG("riscv%d:dts no config [%s]\n", riscv_id, str);
		return -1;
	}

	pmsg->uart_msg.uart_port = i;
	ret = match_status(nodeoff);
	if (ret < 0) {
		pmsg->uart_msg.status = DTS_CLOSE;
		return -1;
	}
	pmsg->uart_msg.status = DTS_OPEN;

	ret = match_pinctrl_names(nodeoff);
	if (ret < 0)
		return -1;

	memset(str, 0, sizeof(str));
	sprintf(str, "pinctrl-%d", ret);

	/* uart has two pin*/
	pin_count = fdt_get_all_pin(nodeoff, str, pin_set);
	if ((pin_count < 0) || (pin_count != 2))
		return -1;

	for (i = 0; i < pin_count; i++) {
		pmsg->uart_msg.uart_pin_msg[i].port = pin_set[i].port;
		pmsg->uart_msg.uart_pin_msg[i].port_num = pin_set[i].port_num;
		pmsg->uart_msg.uart_pin_msg[i].mul_sel = pin_set[i].mul_sel;
	}

	return 0;
}

#define GROUP_NUM  (7)
int riscv_dts_gpio_int_msg(struct dts_msg_t *pmsg, u32 riscv_id)
{
	const char *compatible_str = RISCV_GPIO_INT_STR;
	const char *group_str[GROUP_NUM] = {"PA", "PB",
					"PC", "PD",
					"PE", "PF",
					"PG"};
	int nodeoff = 0;
	int ret = 0;
	unsigned int *pdata = NULL;
	char str[20];
	char *pins = NULL;

	memset(str, 0, sizeof(str));
	sprintf(str, "riscv%d_gpio_int", riscv_id);
	ret = match_compatible(str, compatible_str, &nodeoff);
	if (ret < 0) {
		RISCV_DEBUG("riscv%d:dts no config [%s]\n", riscv_id, str);
		return -1;
	}

	RISCV_DEBUG("riscv%d:dts can find config [%s]\n", riscv_id, str);

	ret = match_status(nodeoff);
	if (ret < 0) {
		memset((void *)&pmsg->gpio_int, 0,
				sizeof(struct dts_gpio_int_t));
		return -1;
	}

	ret = fdt_getprop_string(working_fdt, nodeoff, "pin-group", &pins);
	if (ret < 0) {
		memset((void *)&pmsg->gpio_int, 0,
				sizeof(struct dts_gpio_int_t));
		return -1;
	}

	int i = 0, j = 0;

	if (ret%3 != 0) {
		RISCV_DEBUG("riscv%d:[%s] format err\n", riscv_id, str);
		return -1;
	}
	ret /= 3;
	pdata = &pmsg->gpio_int.gpio_a;
	for (i = 0; i < ret; i++) {
		memset(str, 0, sizeof(str));
		memcpy(str, pins, 2);
		RISCV_DEBUG("[%d]%s\n", i, str);
		for (j = 0; j < GROUP_NUM; j++) {
			if (strcmp(str, group_str[j]) == 0) {
				RISCV_DEBUG("---%s ok\n", str);
				pdata[j] = DTS_OPEN;
			}
		}
		pins += 3;
	}

	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_a);
	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_b);
	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_c);
	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_d);
	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_e);
	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_f);
	RISCV_DEBUG("%d\n", pmsg->gpio_int.gpio_g);
	return 0;
}


int riscv_dts_sharespace_msg(struct dts_msg_t *pmsg, u32 riscv_id)
{
	const char *compatible_str = RISCV_SHARE_SPACE;
	int i = 0;
	int nodeoff = 0;
	int ret = 0;
	char str[30];
	u32 reg_data[8];

	memset(reg_data, 0, sizeof(reg_data));
	memset(str, 0, sizeof(str));
	sprintf(str, "share_space%d", riscv_id);
	ret = match_compatible(str, compatible_str, &nodeoff);
	if (ret < 0) {
		RISCV_DEBUG("riscv%d:dts no config [%s]\n", riscv_id, str);
		return -1;
	}
	RISCV_DEBUG("riscv%d:dts can find config [%s]\n", riscv_id, str);

	ret = match_status(nodeoff);
	if (ret < 0) {
		pmsg->dts_sharespace.status = DTS_CLOSE;
		return -1;
	}

	pmsg->dts_sharespace.status = DTS_OPEN;

	ret = fdt_getprop_u32(working_fdt, nodeoff, "reg", reg_data);
	if (ret < 0) {
		memset((void *)&pmsg->dts_sharespace, 0,
				sizeof(struct dts_sharespace_t));
		return -1;
	}

	for (i = 0; i < 8; i++) {
		RISCV_DEBUG("riscv%d:dts reg[%d]=0x%x\n", riscv_id, i, reg_data[i]);
	}
	pmsg->dts_sharespace.riscv_write_addr = reg_data[0];
	pmsg->dts_sharespace.riscv_write_size = reg_data[1];
	pmsg->dts_sharespace.arm_write_addr = reg_data[2];
	pmsg->dts_sharespace.arm_write_size = reg_data[3];
	pmsg->dts_sharespace.riscv_log_addr = reg_data[4];
	pmsg->dts_sharespace.riscv_log_size = reg_data[5];

	return 0;
}

#ifdef CONFIG_RISCV_UPDATA_IRQ_TAB
#define CPU_BANK_NUM				(16)
#define READY_TAG				(('R' << 24) | ('E' << 16) | ('D' << 8) | 'Y')

/* fdt info format: major, type, remote_irq, local_hwirq, flags */
#define INFO_NUM				(5)
#define INFO_TYPE_OFFSET			(1)
#define INFO_ARCHIRQ_OFFSET			(2)
#define INFO_HWIRQ_OFFSET			(3)
#define INFO_FLAGS_OFFSET			(4)


#ifdef CONFIG_RISCV_UPDATA_IRQ_TAB_DEBUG
#define irq_debug				printf
#else
#define irq_debug(...)				do { } while (0)
#endif

/*
 * Table Format：
 *     The first 16 byte is reserved for _table_head
 *     The last CPU_BANK_NUM word is resource for gpio mask
 *
 *     ----------------
 *     | _table_head  |
 *     ----------------
 *     | _table_entry |
 *     ----------------
 *     |     ...      |
 *     ----------------
 *     |  GPIOA Mask  |
 *     ----------------
 *     |     ...      |
 *     ---------------
 */
struct _table_head {
	uint32_t tag;
	uint32_t len;
	uint32_t banks_off;
	uint32_t banks_num;
} __packed;

struct _table_entry {
	uint32_t status;
	uint32_t type;
	uint32_t flags;
	uint32_t arch_irq;
} __packed;

void *elf_find_segment_offset(phys_addr_t elf_addr, const char *seg_name)
{
	int i;
	const char *name_table;
	const uint8_t *elf_data = (void *)(uint32_t)elf_addr;
	Elf32_Shdr *shdr32;
	Elf32_Ehdr *ehdr32;
	Elf64_Shdr *shdr64;
	Elf64_Ehdr *ehdr64;

	if (fw_elf_get_class(elf_addr) == ELFCLASS32) {
		ehdr32 = (Elf32_Ehdr *)elf_data;
		shdr32 = (Elf32_Shdr *)(elf_data + ehdr32->e_shoff);
		name_table = (const char *)(elf_data + shdr32[ehdr32->e_shstrndx].sh_offset);

		for (i = 0; i < ehdr32->e_shnum; i++, shdr32++) {
			if (strcmp(name_table + shdr32->sh_name, seg_name))
				continue;

			break;
		}

		if (i == ehdr32->e_shnum)
			return NULL;

		return (void *)(uint32_t)elf_addr + shdr32->sh_offset;
	} else {
		ehdr64 = (Elf64_Ehdr *)elf_data;
		shdr64 = (Elf64_Shdr *)(elf_data + ehdr64->e_shoff);
		name_table = (const char *)(elf_data + shdr64[ehdr64->e_shstrndx].sh_offset);

		for (i = 0; i < ehdr64->e_shnum; i++, shdr64++) {
			if (strcmp(name_table + shdr64->sh_name, seg_name))
				continue;

			break;
		}

		if (i == ehdr64->e_shnum)
			return NULL;

		return (void *)(uint32_t)elf_addr + shdr64->sh_offset;
	}

}

static int update_irq_head_info(const void *fdt, int nodeoffset,
				void *buf, int buf_len)
{
	int i;
	int len;
	int ptable_len;
	struct _table_head *head;
	struct _table_entry *ptable;
	const uint32_t *data;
	uint32_t *gpio_tab;

	head = buf;
	ptable = buf + sizeof(*head);

	data = fdt_getprop(fdt, nodeoffset, "share-irq", &len);
	if (len < 0) {
		irq_debug("updata %s share-irq table failed,"
						"not find share-irq property.\n",
						fdt_get_name(fdt, nodeoffset, NULL));
		return -1;
	}

	if ((len % (INFO_NUM * 4)) != 0) {
		irq_debug("%s share-irq table invalid format.\n",
						fdt_get_name(fdt, nodeoffset, NULL));
		return -1;
	}
	irq_debug("share_irq_table len=%d\n", len);

	head->len = buf_len;
	head->banks_num = CPU_BANK_NUM;

	/*
	 * current we only support update gpio bank info.
	 * GPIO Table Format: major type flags
	 * type = 0 -> nomal interrupt
	 * type = 1 -> GPIOA
	 * type = 2 -> GPIOB
	 * ...
	 *
	 * flags = bank_mask
	 */
	gpio_tab = buf + buf_len - CPU_BANK_NUM * sizeof(uint32_t);
	memset(gpio_tab, 0, CPU_BANK_NUM * sizeof(uint32_t));
	head->banks_off = (uint32_t)((void *)gpio_tab - buf);

	ptable_len = head->banks_off - sizeof(*head);
	memset(ptable, 0, ptable_len);

	for (i = 0; i < (len / 4); i += INFO_NUM) {
		uint32_t type = fdt32_to_cpu(data[i + INFO_TYPE_OFFSET]);
		uint32_t arch_irq = fdt32_to_cpu(data[i + INFO_ARCHIRQ_OFFSET]);
		uint32_t hwirq = fdt32_to_cpu(data[i + INFO_HWIRQ_OFFSET]);
		uint32_t flags = fdt32_to_cpu(data[i + INFO_FLAGS_OFFSET]);

		ptable[hwirq].status = 1;
		ptable[hwirq].type = type;
		ptable[hwirq].flags = flags;
		ptable[hwirq].arch_irq = arch_irq;
		irq_debug("\t: Entry type: 0x%x, flags: 0x%x, IRQ: %d\n",
			ptable[hwirq].type, ptable[hwirq].flags, ptable[hwirq].arch_irq);

		if (type == 0 || type >= CPU_BANK_NUM)
			continue;
		irq_debug("\t: GPIO%c = 0x%x\n", 'A' + type - 1, flags);
		gpio_tab[type - 1] = flags;
	}

	head->tag = READY_TAG;
	flush_dcache_all();

	return 0;
}

static int sunxi_find_irq_tab_rserved(const void *fdt,
				int nodeoffset, uint32_t *addr, uint32_t *size)
{
	int len;
	const void *data;

	data = fdt_getprop(fdt, nodeoffset, "memory-region", &len);
	if (len < 0) {
		irq_debug("find %s memory-region property failed",
						fdt_get_name(fdt, nodeoffset, NULL));
		return -1;
	}

	nodeoffset = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*(uint32_t *)data));
	if (nodeoffset < 0) {
		irq_debug("parse %s memory-region property failed, ret=%s",
						fdt_get_name(fdt, nodeoffset, NULL),
						fdt_strerror(nodeoffset));
		return -1;
	}

	data = fdt_getprop(fdt, nodeoffset, "reg", &len);
	if (len < 0) {
		irq_debug("find %s memory-region property failed",
						fdt_get_name(fdt, nodeoffset, NULL));
		return -1;
	}

	if ((len % 4) != 0) {
		irq_debug("%s reg invalid format, len=%d.\n", fdt_get_name(fdt, nodeoffset, NULL), len);
		return -1;
	}

	/* ignore high word */
	*addr = fdt32_to_cpu(*((uint32_t *)data + 1));
	*size = fdt32_to_cpu(*((uint32_t *)data + 3));

	irq_debug("irq table addr:0x%x len:0x%x\n", *addr, *size);

	return 0;
}

void update_riscv_irq_tab(unsigned long elf_base, int idx)
{
	int i, ret, nodeoffset, node;
	const void *fdt = (void *)(uint32_t)working_fdt;
	uint32_t *tab_info;
	uint32_t addr, len;

	ret = fdt_check_header(fdt);
	if (ret < 0) {
		printf("FDT: %s.\n", fdt_strerror(ret));
		return;
	}

	node = fdt_path_offset(fdt, "/reserved-irq");
	if (node < 0) {
		printf("FDT: /reserved-irq node not found.\n");
		return;
	}

	i = 0;
	for (nodeoffset = fdt_first_subnode(fdt, node);
			nodeoffset > 0;
			nodeoffset = fdt_next_subnode(fdt, nodeoffset)) {
		if (i++ != idx)
			continue;
		ret = sunxi_find_irq_tab_rserved(fdt, nodeoffset, &addr, &len);
		if (ret)
			break;
		tab_info = elf_find_segment_offset(elf_base, ".share_irq_table");
		if (!tab_info) {
			irq_debug("firmware not support share-irq.\n");
			return;
		}
		tab_info[0] = addr;
		tab_info[1] = len;
		irq_debug("0x%x -> 0x%x\n", addr, (uint32_t)tab_info);

		update_irq_head_info(fdt, nodeoffset, (void *)addr, len);
		break;
	}
}
#endif /* CONFIG_RISCV_UPDATA_IRQ_TAB */
