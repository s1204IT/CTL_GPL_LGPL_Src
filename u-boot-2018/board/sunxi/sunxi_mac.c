/*
 * (C) Copyright 2016
 *Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *zhouhuacai <zhouhuacai@allwinnertech.com>
 *
 * SPDX-License-Identifier:›GPL-2.0+
 */
#include <common.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <efuse_map.h>

static int str2num(char *str, char *num)
{
	int val = 0, i;
	char *p = str;
	for (i = 0; i < 2; i++) {
		val *= 16;
		if (*p >= '0' && *p <= '9')
			val += *p - '0';
		else if (*p >= 'A' && *p <= 'F')
			val += *p - 'A' + 10;
		else if (*p >= 'a' && *p <= 'f')
			val += *p - 'a' + 10;
		else
			return -1;
		p++;
	}
	*num = val;
	return 0;
}

static int addr_parse(const char *addr_str, int check)
{
	char addr[6];
	char cmp_buf[6];
	char *p = (char *)addr_str;
	int i;
	if (!p || strlen(p) < 17)
		return -1;

	for (i = 0; i < 6; i++) {
		if (str2num(p, &addr[i]))
			return -1;

		p += 2;
		if ((i < 5) && (*p != ':'))
			return -1;

		p++;
	}

	if (check && (addr[0] & 0x3))
		return -1;

	memset(cmp_buf, 0x00, 6);
	if (memcmp(addr, cmp_buf, 6) == 0)
		return -1;

	memset(cmp_buf, 0xFF, 6);
	if (memcmp(addr, cmp_buf, 6) == 0)
		return -1;

	return 0;
}

struct addr_info_t {
	char *envname;
	char *dtsname;
	int   flag;
};

static struct addr_info_t addr[] = {
	{"mac",      "addr_eth",  1},
	{"wifi_mac", "addr_wifi", 1},
	{"bt_mac",   "addr_bt",   0},
};

#ifdef EFUSE_WMAC
void update_sunxi_wmac_from_efuse(void)
{
	u8 wmac_sel = 0;
	int wmac_sel_len = sizeof(u8);
	u8 wmac[6] = {0};
	int wmac_len = 6 * sizeof(u8);
	char wmac_addr[18] = {0};

	if (!sunxi_efuse_read("wmac_sel", &wmac_sel, &wmac_sel_len)) {
		pr_debug("Get wmac_sel: %d\n", wmac_sel & EFUSE_WMAC_SEL_FLAG_MASK);
		/* wmac_sel == 0b01 : only use area wmac addr1 */
		if ((wmac_sel & EFUSE_WMAC_SEL_FLAG_MASK) == 0b01) {
			if (sunxi_efuse_read("wmac_addr1", wmac, &wmac_len)) {
				pr_err("Get wmac from efuse addr1 fail\n");
			}
		} else {
			if (sunxi_efuse_read("wmac_addr2", wmac, &wmac_len)) {
				/*
				 * Area efuse addr2 read failed
				 * wmac_sel == 0b1X : Don't do anything, just read area efuse addr2
				 * wmac_sel == 0b00 : Failed to read area efuse addr2, try to read area efuse addr1
				 */
				if ((wmac_sel & EFUSE_WMAC_SEL_FLAG_MASK) == 0b00) {
					pr_debug("Get wmac from efuse addr2 fail, try get wmac from efuse addr1\n");
					sunxi_efuse_read("wmac_addr1", wmac, &wmac_len);
				}
			} else {
				/*
				 * Area efuse addr2 read successfully
				 * wmac_sel == 0b00 : Area efuse addr2 is not burning, try to read area efuse addr1
				 */
				if ((wmac_sel & EFUSE_WMAC_SEL_FLAG_MASK) == 0b00) {
					pr_debug("Get wmac from efuse addr2 sucess\n");
					if ((wmac[0] == 0) && (wmac[1] == 0) && (wmac[2] == 0) && (wmac[3] == 0) && (wmac[4] == 0) && (wmac[5] == 0)) {
						pr_debug("The wmac address is not burned in efuse addr2. Try to get it from efuse addr1.\n");
						sunxi_efuse_read("wmac_addr1", wmac, &wmac_len);
					}
				}
			}
		}
	};
	/* Check whether the read area has been burned, If it burns, write it to env*/
	if ((wmac[0] == 0) && (wmac[1] == 0) && (wmac[2] == 0) && (wmac[3] == 0) && (wmac[4] == 0) && (wmac[5] == 0)) {
		pr_debug("There is no mac address burning in area efuse wmac\n");
	} else {
		sprintf(wmac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", wmac[0], wmac[1], wmac[2], wmac[3], wmac[4], wmac[5]);
		pr_info("Get mac addr(%s) from efuse wmac\n", wmac_addr);
		env_set("wifi_mac", wmac_addr);
	}
}
#endif

int update_sunxi_mac(void)
{
	char *p = NULL;
	int   i = 0;
	int   nodeoffset = 0;
	struct fdt_header *dtb_base = working_fdt;

#ifdef EFUSE_WMAC
	if (env_get("wifi_mac") == NULL) {
		update_sunxi_wmac_from_efuse();
	}
#endif

	nodeoffset = fdt_path_offset(dtb_base, "/soc/addr_mgt");

	for (i = 0; i < ARRAY_SIZE(addr); i++) {
		p = env_get(addr[i].envname);
		if (p != NULL) {
			if (addr_parse(p, addr[i].flag)) {
				/*if not pass, clean it, do not pass through cmdline*/
				pr_err("%s format illegal\n", addr[i].envname);
				env_set(addr[i].envname, "");
				continue;
			}

			if (nodeoffset >= 0)
				fdt_setprop_string(dtb_base, nodeoffset, addr[i].dtsname, p);
		}
	}

	return 0;
}
