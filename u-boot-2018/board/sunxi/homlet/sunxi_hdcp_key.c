/*
 * (C) Copyright 2007-2015
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
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <securestorage.h>
#include <smc.h>
#include <u-boot/crc.h>
#include <asm/arch/ce.h>
#include <sunxi_board.h>
#include <sunxi_keybox.h>

#define HDCPV22_BUFFER_LEN 912

typedef struct {
	char name[64];
	u32 len;
	u32 res;
	u8 *key_data;
} sunxi_efuse_key_info_t;

extern void sunxi_dump(void *addr, unsigned int size);

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

int sunxi_deal_hdcp_key(char *keydata, int keylen, enum AW_HDCP_KEY_TYPE_EN type)
{
	int ret;
	char encrypt_hdcp_key[HDCPV22_BUFFER_LEN];

	if (!keydata || !keylen) {
		pr_err("input para is null\n");
		return -1;
	}
	memset(encrypt_hdcp_key, 0x0, sizeof(encrypt_hdcp_key));
	ret = smc_tee_hdcp_key_encrypt((char *)encrypt_hdcp_key, HDCPV22_BUFFER_LEN,
							(char *)(keydata), keylen, type);
	if (ret < 0) {
		pr_err("smc_tee_hdcp_key_encrypt: failed\n");
		return -2;
	}
#if 0
	pr_err("dump hdcp data\n");
	sunxi_dump(encrypt_hdcp_key, HDCPV22_BUFFER_LEN);
#endif
	switch (type) {
	case AW_HDCP_1_4:
		pr_err("down hdcp 1.4\n");
		ret = sunxi_secure_object_down("hdcpkey", encrypt_hdcp_key,
						SUNXI_HDCP_KEY_LEN, 1, 0);
		if (ret < 0) {
			pr_err("sunxi secure storage write failed\n");
			return -3;
		}

		break;
	case AW_HDCP_2_2:
		pr_err("down hdcp 2.2\n");
		ret = sunxi_secure_object_down("hdcpkeyV22", (char *)encrypt_hdcp_key,
						HDCPV22_BUFFER_LEN, 1, 0);
		if (ret < 0) {
			pr_err("sunxi secure storage write failed\n");
			return -1;
		}
		break;

	case AW_HDCP_2_3:
		pr_err("down hdcp 2.3\n");
		ret = sunxi_secure_object_down("hdcpkeyV23", (char *)encrypt_hdcp_key,
						HDCPV22_BUFFER_LEN, 1, 0);
		if (ret < 0) {
			pr_err("sunxi secure storage write failed\n");
			return -1;
		}
		break;
	default:
		return -2;
	}

	return 0;
}

int sunxi_hdcp_key_post_install(void)
{
	int ret;
	ret = smc_aes_rssk_decrypt_to_keysram();
	if (ret) {
		pr_error("push hdcp key failed\n");
		return -1;
	}
	return 0;
}

#if CONFIG_SUNXI_HDCP_KEY_RX
#define LIMIT_HDCP_HASH_VALUE_LEN 6
int sunxi_hdcp_hash(__maybe_unused const char *name, char *buf, int len,
		    __maybe_unused int encrypt,
		    __maybe_unused int write_protect)
{
	u8 hdcpshabuffer[64];
	u8 retlen;
	u8 ret;
	u8 strtmpbuf[64];
	char hdcphashname[64];
	u8 i, hdcp_type = -1;
	strcpy(hdcphashname, name);
	sunxi_sha_calc(hdcpshabuffer, 32, (u8 *)buf, len);
	retlen = sizeof(hdcpshabuffer) / sizeof(hdcpshabuffer[0]);
	retlen = ((retlen > 32) ? (32) : (retlen));
	for (i = 0; i < LIMIT_HDCP_HASH_VALUE_LEN; i++) {
		sprintf((char *)strtmpbuf + i * 2, "%2x",
			hdcpshabuffer[retlen - LIMIT_HDCP_HASH_VALUE_LEN + i]);
	}

	if (!strcmp("hdcpkey", name)) {
		strcat(hdcphashname, "V14_hash");
		hdcp_type = AW_HDCP_1_4;
	} else if (!strcmp("hdcpkeyV22", name)) {
		strcat(hdcphashname, "_hash");
		hdcp_type = AW_HDCP_2_2;
	} else {
		strcat(hdcphashname, "_hash");
		hdcp_type = AW_HDCP_2_3;
	}

	ret = sunxi_deal_hdcp_key((char *)buf, len, hdcp_type);
	if (ret) {
		printf("sunxi deal with hdcp key failed\n");
		return -1;
	}

	sunxi_secure_object_write(hdcphashname, (char *)strtmpbuf,
				  LIMIT_HDCP_HASH_VALUE_LEN * 2);

	return 0;
}
SUNXI_KEYBOX_KEY(hdcpkey, sunxi_hdcp_hash, NULL);
SUNXI_KEYBOX_KEY(hdcpkeyV22, sunxi_hdcp_hash, NULL);
SUNXI_KEYBOX_KEY(hdcpkeyV23, sunxi_hdcp_hash, NULL);
#else
int sunxi_hdcp_tx(__maybe_unused const char *name, char *buf, int len,
		    __maybe_unused int encrypt,
		    __maybe_unused int write_protect)
{
	int ret;
	ret = sunxi_deal_hdcp_key((char *)buf, len, AW_HDCP_1_4);
	if (ret) {
		printf("sunxi deal with hdcp key failed\n");
		return -1;
	}
	return 0;
}
SUNXI_KEYBOX_KEY(hdcpkey, sunxi_hdcp_tx, NULL);
#endif
