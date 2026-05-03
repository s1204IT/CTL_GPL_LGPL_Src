/*
 * (C) Copyright 2000-2999
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Author: qinjian <qinjian@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <asm/io.h>
#include <efuse_map.h>
#include <sunxi_board.h>
#include <smc.h>

/* SECURE_BIT_OFFSET */
#ifdef CONFIG_ARCH_SUN8IW12P1
#define SECURE_BIT_OFFSET		31
#elif CONFIG_MACH_SUN55IW3
#define SECURE_BIT_OFFSET		0
#elif CONFIG_MACH_SUN55IW5 || CONFIG_MACH_SUN50IW15
#define SECURE_BIT_OFFSET		0
#elif CONFIG_MACH_SUN60IW2 || CONFIG_MACH_SUN300IW1 || CONFIG_MACH_SUN65IW1
#define SECURE_BIT_OFFSET		0
#else
#define SECURE_BIT_OFFSET		11
#endif

/* SBROM_ACCELERATION_ENABLE_BIT. For CONFIG_ARCH_SUN50IW1P1 Only */
#ifdef CONFIG_ARCH_SUN50IW1P1
#define SBROM_ACCELERATION_ENABLE_BIT	29
#endif

#define EFUSE_DBG_E 0

#if EFUSE_DBG_E
static void efuse_dump(char *str, unsigned char *data, int len, int align)
{
	int i = 0;
	if (str)
		printf("\n%s: ", str);
	for (i = 0; i < len; i++) {
		if ((i % align) == 0) {
			printf("\n");
		}
		printf("%x", *(data++));
	}
	printf("\n");
}
#define EFUSE_DBG printf
#define EFUSE_DBG_DUMP		efuse_dump
#define EFUSE_DUMP_LEN		16
#else  /* !EFUSE_DBG_E */
#define EFUSE_DBG_DUMP(...)	\
	do {			\
	} while (0)
#define EFUSE_DBG(...)		\
	do {			\
	} while (0)
#endif  /* EFUSE_DBG_E */

#ifndef EFUSE_READ_PROTECT
#define EFUSE_READ_PROTECT	EFUSE_CHIPCONFIG
#define EFUSE_WRITE_PROTECT	EFUSE_CHIPCONFIG
#endif

/* internal struct */
typedef struct efuse_key_map_new {
	char      name[SUNXI_KEY_NAME_LEN];
	int       offset;		/* Address in SID SRAM */
	int       size;			/* unit: bit */
#ifdef LONG_EFUSE_RW_PROTECT_OFFSET
	long long rd_fbd_offset;	/* read  protect */
	long long burned_flg_offset;	/* write protect */
#else
	int       rd_fbd_offset;	/* read  protect */
	int       burned_flg_offset;	/* write protect */
#endif
	int       sw_rule;		/* acl */
} efuse_key_map_new_t;

/* @sw_rule */
#define EFUSE_PRIVATE			(0)       /* It can not be seen */
#define EFUSE_NACCESS			(1 << 1)  /* After burned, CPU can not access */
#define EFUSE_RW			(2 << 1)
#define EFUSE_RO			(3 << 1)

#ifndef LONG_EFUSE_RW_PROTECT_OFFSET
#define EFUSE_ACL_SET_BRUN_BIT		(1 << 29)
#define EFUSE_ACL_SET_RD_FORBID_BIT	(1 << 30)
#define EFUSE_BRUN_RD_OFFSET_MASK	(0xFFFFFF)
#endif

#define EFUSE_DEF_ITEM(name, offset, size_bits, rd_offset, burn_offset, acl)	\
	{									\
		name, offset, size_bits, rd_offset, burn_offset, acl		\
	}

/* g_key_info[] */
#if defined(CONFIG_MACH_SUN50IW11)
static efuse_key_map_new_t g_key_info[] = {
	EFUSE_DEF_ITEM(EFUSE_CHIPID_NAME, EFUSE_CHIPID, 128, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_ROTPK_NAME, EFUSE_ROTPK, 256, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM("", 0, 0, 0, 0, EFUSE_PRIVATE),
};
#elif defined(CONFIG_ARCH_SUN8IW17P1)
static efuse_key_map_new_t g_key_info[] = {
	EFUSE_DEF_ITEM(EFUSE_THM_SENSOR_NAME, 0x14, 96, 2, 2, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_ROTPK_NAME, 0x70, 256, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM("sensor", 0x14, 96, 2, 2, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_FT_ZONE_NAME, 0x20, 128, 3, 3, EFUSE_RW),
	EFUSE_DEF_ITEM(EFUSE_TV_OUT_NAME, 0x30, 32, 4, 4, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_OEM_NAME, 0x34, 96, 4, 4, EFUSE_RO),
	EFUSE_DEF_ITEM("", 0, 0, 0, 0, EFUSE_PRIVATE),
};
#elif defined(CONFIG_MRCH_SUN8IW19P1)
static efuse_key_map_new_t g_key_info[] = {
	EFUSE_DEF_ITEM(EFUSE_CHIPID_NAME, 0x0, 128, -1, 0, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_THM_SENSOR_NAME, EFUSE_THERMAL_SENSOR, 64, -1, 0, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_FT_ZONE_NAME, EFUSE_TF_ZONE, 128, -1, 0, EFUSE_RO),
	EFUSE_DEF_ITEM("psensor", EFUSE_PSENSOR, 32, -1, 15, EFUSE_RO),
	EFUSE_DEF_ITEM("anti_blushing", EFUSE_ANTI_BLUSHING, 32, -1, 15, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_RESERVED_NAME, EFUSE_CLIENT_RESERVE, 256, -1, 12, EFUSE_RO),
	EFUSE_DEF_ITEM("", 0, 0, 0, 0, EFUSE_PRIVATE),
};
#elif defined(CONFIG_MRCH_SUN8IW7P1)
static efuse_key_map_new_t g_key_info[] = {
	EFUSE_DEF_ITEM(EFUSE_ROTPK_NAME, 0x64, 256, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM("", 0, 0, 0, 0, EFUSE_PRIVATE),
};
#elif defined(CONFIG_MRCH_SUN8IW18P1)
static efuse_key_map_new_t g_key_info[] = {
	EFUSE_DEF_ITEM(EFUSE_ROTPK_NAME, 0x40, 256, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM("", 0, 0, 0, 0, EFUSE_PRIVATE),
};
#else  /* g_key_info[] */

#ifndef SCC_ROTPK_BURNED_FLAG
#define SCC_ROTPK_BURNED_FLAG		(12)
#endif

/* Please extend key_maps for new arch here */
static efuse_key_map_new_t g_key_info[] = {
#ifdef EFUSE_CHIPID
	EFUSE_DEF_ITEM(EFUSE_CHIPID_NAME, EFUSE_CHIPID, 128, -1, 0, EFUSE_RO),
#endif

#ifdef EFUSE_ROTPK
	EFUSE_DEF_ITEM(EFUSE_ROTPK_NAME, EFUSE_ROTPK, 256, -1, SCC_ROTPK_BURNED_FLAG, EFUSE_RO),
#endif

#ifdef EFUSE_SSK
	/* NOT USED */
	EFUSE_DEF_ITEM(EFUSE_SSK_NAME, EFUSE_SSK, SID_SSK_SIZE, -1, SCC_SSK_BURNED_FLAG, EFUSE_RO),
#endif

#ifdef EFUSE_OEM_PROGRAM
	EFUSE_DEF_ITEM(EFUSE_OEM_NAME, EFUSE_OEM_PROGRAM, SID_OEM_PROGRAM_SIZE, -1, -1, EFUSE_RO),
#endif

#ifdef EFUSE_OEM_PROGRAM_SECURE
	/* NOT USED */
	EFUSE_DEF_ITEM(EFUSE_OEM_SEC_NAME, EFUSE_OEM_PROGRAM_SECURE, SID_OEM_PROGRAM_SECURE_SIZE, -1, -1, EFUSE_RO),
#endif

#ifdef EFUSE_WMAC
	/* NOT USED */
	EFUSE_DEF_ITEM(EFUSE_WMAC_SEL_FLAG_NAME, EFUSE_WMAC, 8, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_WMAC_ADDR1_NAME, EFUSE_WMAC_ADDR1, SID_WMAC_SIZE, -1, -1, EFUSE_RO),
	EFUSE_DEF_ITEM(EFUSE_WMAC_ADDR2_NAME, EFUSE_WMAC_ADDR2, SID_WMAC_SIZE, -1, -1, EFUSE_RO),
#endif

	EFUSE_DEF_ITEM("", 0, 0, 0, 0, EFUSE_PRIVATE),
};

#endif  /* g_key_info[] */

__weak int set_efuse_voltage(int vol)
{
	EFUSE_DBG("Not implemented\n");
	return 0;
}

/* @key_index is the address in Efuse Mapping Table
 * Please refer to <AW1728 spec> page 11 for the reason of adding this function.
 * Burn efuse: efuse sram can not get the latest value unless via sid read or reboot.
 */
static uint __sid_reg_read_key(uint key_index)
{
	uint reg_val;

	/* Write the key into PG_INDEX  */
	reg_val = readl(SID_PRCTL);
	reg_val &= ~((0x1ff << 16) | 0x3);
	reg_val |= key_index << 16;
	writel(reg_val, SID_PRCTL);

	/* READ_START */
	reg_val &= ~((0xff << 8) | 0x3);
	reg_val |= (SID_OP_LOCK << 8) | 0x2;
	writel(reg_val, SID_PRCTL);

	/* Wait READ done */
	while (readl(SID_PRCTL) & 0x2) {
		;
	}

	/* Clear PG_INDEX/OP_LOCK/READ_START/PG_START */
	reg_val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	writel(reg_val, SID_PRCTL);

	/* Read Key */
	reg_val = readl(SID_RDKEY);

	EFUSE_DBG("key = 0x%02x, value = 0x%08x\n", key_index, reg_val);
	return reg_val;
}

uint sid_read_key(uint key_index)
{
//#ifdef CONFIG_SID_READ_KEY_FROM_SRAM
	//sid_sram_read(key_index);  // FIXME
//#else
	return __sid_reg_read_key(key_index);
//#endif
}

void sid_program_key(uint key_index, uint key_value)
{
	uint reg_val;

	EFUSE_DBG("key = 0x%02x, value = 0x%08x\n", key_index, key_value);

	/* SID-Program Power On */
#ifdef CONFIG_SUNXI_SET_EFUSE_POWER
	set_efuse_voltage(1);
#else
	set_efuse_voltage(1900);
#endif

#ifdef EFUSE_HV_SWITCH
	writel(1, EFUSE_HV_SWITCH);
#endif

	/* Write the value */
	writel(key_value, SID_PRKEY);

	/* Write the key into PG_INDEX  */
	reg_val = readl(SID_PRCTL);
	reg_val &= ~((0x1ff << 16) | 0x3);
	reg_val |= key_index << 16;
	writel(reg_val, SID_PRCTL);

	/* PG_START */
	reg_val &= ~((0xff << 8) | 0x3);
	reg_val |= (SID_OP_LOCK << 8) | 0x1;
	writel(reg_val, SID_PRCTL);

	/* Wait PROGRAM done */
	while (readl(SID_PRCTL) & 0x1) {
		;
	}

	/* Clear PG_INDEX/OP_LOCK/READ_START/PG_START */
	reg_val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	writel(reg_val, SID_PRCTL);

	/* SID-Program Power Off */
#ifdef CONFIG_SUNXI_SET_EFUSE_POWER
	set_efuse_voltage(0);
#else
	set_efuse_voltage(1800);
#endif

#ifdef EFUSE_HV_SWITCH
	writel(0, EFUSE_HV_SWITCH);
#endif
}

#define EFUSE_BURN_MAX_TRY_CNT		3

/* Burn a key with retries */
static int uni_burn_key(uint key_index, uint key_value)
{
	uint key_burn_bitmask;
	int retries = 0;

	/* Find out the bits which needs burning */
	key_burn_bitmask = ~(sid_read_key(key_index)) & key_value;

	/* Try burnning for EFUSE_BURN_MAX_TRY_CNT times */
	while (key_burn_bitmask) {
		if (++retries > EFUSE_BURN_MAX_TRY_CNT) {
			pr_err("Burn key failed: key_burn_bitmask = 0x%x\n", key_burn_bitmask);
			return -1;
		}
		sid_program_key(key_index, key_burn_bitmask);

		key_burn_bitmask &= ~(sid_read_key(key_index));
	}
	return 0;
}

#ifndef SECURE_BIT_VAL
#define SECURE_BIT_VAL			1
#endif

/* Burn Secure-Enable Bit */
int sid_set_security_mode(void)
{
	EFUSE_DBG("%s(): Begin\n", __func__);
#if defined(EFUSE_LCJS)
    #ifdef CONFIG_ARCH_SUN50IW1P1
	return uni_burn_key(EFUSE_LCJS, ((0x1 << SECURE_BIT_OFFSET) |
					 (0x1 << SBROM_ACCELERATION_ENABLE_BIT)));
    #else
	return uni_burn_key(EFUSE_LCJS, (SECURE_BIT_VAL << SECURE_BIT_OFFSET));
    #endif
#elif defined(EFUSE_ANTI_BRUSH)
	return uni_burn_key(EFUSE_ANTI_BRUSH, (0x1 << ANTI_BRUSH_BIT_OFFSET));
#else
	printf("%s(): Not implemented\n", __func__);
	return 0;
#endif
}

/* Get Secure-Enable Bit */
int sid_probe_security_mode(void)
{
#if defined(EFUSE_LCJS)
	return (sid_read_key(EFUSE_LCJS) >> SECURE_BIT_OFFSET) & 1;
#elif defined(EFUSE_ANTI_BRUSH)
	return (sid_read_key(EFUSE_ANTI_BRUSH) >> ANTI_BRUSH_BIT_OFFSET) & 1;
#else
	printf("%s(): Not implemented\n", __func__);
	return 0;
#endif
}

int sid_get_security_status(void)
{
#ifdef SID_SECURE_MODE
	return readl(SID_SECURE_MODE) & 0x1;
#else
	//TODO: Why need this?
	return sid_probe_security_mode();
#endif
}

/* Burn the bit @bit_offset in @key */
static void _set_cfg_flg(int key, int bit_offset)
{
	uni_burn_key(key, (1 << bit_offset));
	return;
}

/* Check if the given key @key_map is burned */
static int _get_burned_flag(efuse_key_map_new_t *key_map)
{
	if (key_map->burned_flg_offset < 0) {
		EFUSE_DBG("Not burned: burned_flg_offset < 0\n");
		return 0;
	} else {
		uint shift = key_map->burned_flg_offset & EFUSE_BRUN_RD_OFFSET_MASK;
		uint val = sid_read_key(EFUSE_WRITE_PROTECT);
		int burned = (val >> shift) & 1;
		EFUSE_DBG("Key '%s' is %s burned\n", key_map->name, burned ? "already" : "NOT");
		return burned;
	}
}

static int __sw_acl_ck(efuse_key_map_new_t *key_map, int need_burn)
{
	if (key_map->sw_rule == EFUSE_PRIVATE) {
		pr_err("Key '%s' is PRIVATE\n", key_map->name);
		return EFUSE_ERR_PRIVATE;
	}

	if (need_burn == 0) {  /* Action is READ */
		if (key_map->sw_rule == EFUSE_NACCESS) {
			pr_err("Key '%s' is NACCESS\n", key_map->name);
			return EFUSE_ERR_NO_ACCESS;
		}
	} else {  /* Action is WRITE */
		if (_get_burned_flag(key_map)) {  /* Already burned */
			if ((key_map->sw_rule == EFUSE_NACCESS) ||
			    (key_map->sw_rule == EFUSE_RO)) {
				pr_err("Key '%s' is already burned\n", key_map->name);
				return EFUSE_ERR_ALREADY_BURNED;
			}
		}
#if 0
		//TODO: why need 'if ((key_map->sw_rule == EFUSE_NACCESS) ...' ? Why not using the following instead?
		if (_get_burned_flag(key_map)) {  /* Already burned */
			EFUSE_DBG("Key '%s' is already burned\n", key_map->name);
			return EFUSE_ERR_ALREADY_BURNED;
		}
#endif

		if (key_map->sw_rule == EFUSE_RW) {
			key_map->burned_flg_offset |= EFUSE_ACL_SET_BRUN_BIT;
			key_map->rd_fbd_offset     |= EFUSE_ACL_SET_RD_FORBID_BIT;
		}
	}

	return 0;  /* Check passed */
}

/* Efuse access control rule check */
static int __efuse_acl_ck(efuse_key_map_new_t *key_map, int need_burn)
{
	/* For normal solution only check EFUSE_PRIVATE, other will be seemed as EFUSE_RW */
	if (sid_get_security_status() == 0) {
		if (key_map->sw_rule == EFUSE_PRIVATE) {
			return EFUSE_ERR_PRIVATE;
		}
		return 0;
	}

	int err = __sw_acl_ck(key_map, need_burn);
	if (err)
		return err;

	if (need_burn) {
		if (_get_burned_flag(key_map)) {  /* Already burned */
			pr_err("Key '%s' is already burned\n", key_map->name);
			EFUSE_DBG("EFUSE_WRITE_PROTECT = 0x%x\n", sid_read_key(EFUSE_WRITE_PROTECT));
			return EFUSE_ERR_ALREADY_BURNED;
		}
	} else {
		if ((key_map->rd_fbd_offset >= 0) &&
		    ((sid_read_key(EFUSE_READ_PROTECT) >> key_map->rd_fbd_offset) & 1)) {
			/* Read is not allowed because of the read forbidden bit was set */
			pr_err("Key '%s' was set READ-FORBIDDEN\n", key_map->name);
			EFUSE_DBG("EFUSE_READ_PROTECT = 0x%x\n", sid_read_key(EFUSE_READ_PROTECT));
			return EFUSE_ERR_READ_FORBID;
		}
	}
	return 0;
}

/* get a uint value from unsigned char *k_src */
static unsigned int _get_uint_val(unsigned char *k_src)
{
	unsigned int test = 0x12345678;
	if ((unsigned long)k_src & 0x3) {  /* Not word-aligned */
		if (*(unsigned char *)(&test) == 0x12) {  /* Big endian */
			memcpy((void *)&test, (void *)k_src, 4);
			return test;
		} else {
			test = ((*(k_src + 3)) << 24) | ((*(k_src + 2)) << 16) |
			       ((*(k_src + 1)) << 8)  | (*k_src);
			return test;
		}
	} else {
		return *(unsigned int *)k_src;
	}
}

int sunxi_efuse_write(void *key_inf)
{
	efuse_key_info_t *list = key_inf;
	unsigned char *k_src   = NULL;
	unsigned int niddle = 0, tmp_data = 0, k_d_lft = 0;
	efuse_key_map_new_t *key_map = g_key_info;
	int err;

	if (list == NULL) {
		pr_err("key_inf is null\n");
		return EFUSE_ERR_ARG;
	}

	/* Search key with name */
	for (; key_map->size != 0; key_map++) {
		if (!memcmp(list->name, key_map->name, strlen(key_map->name))) {
			EFUSE_DBG("key name = %s, offset = 0x%x\n", key_map->name, key_map->offset);
			/* Check if there is enough space to store the key */
			if ((key_map->size >> 3) < list->len) {
				pr_err("No enough space: dst size = %d, src size = %d\n",
					key_map->size >> 3, list->len);
				return EFUSE_ERR_KEY_SIZE_TOO_BIG;
			}
			break;
		}
	}

	/* Key name not found */
	if (key_map->size == 0) {
		pr_err("Unknown key '%s'\n", list->name);
		return EFUSE_ERR_KEY_NAME_WRONG;
	}

	EFUSE_DBG("key = '%s', size = %d, offset = 0x%x\n", key_map->name, key_map->size, key_map->offset);

	err = __efuse_acl_ck(key_map, 1);
	if (err)
		return err;

	EFUSE_DBG_DUMP(list->name, list->key_data, list->len, EFUSE_DUMP_LEN);
	niddle  = key_map->offset;
	k_d_lft = list->len;
	k_src   = list->key_data;

	while (k_d_lft >= 4) {
		tmp_data = _get_uint_val(k_src);
		EFUSE_DBG("offset = 0x%x, val = 0x%x\n", niddle, tmp_data);
		if (tmp_data) {
			if (uni_burn_key(niddle, tmp_data))
				return EFUSE_ERR_BURN_TIMING;
		}
		k_d_lft -= 4;
		niddle += 4;
		k_src += 4;
	}

	if (k_d_lft) {
		uint mask = (1UL << (k_d_lft << 3)) - 1;
		tmp_data  = _get_uint_val(k_src);
		mask &= tmp_data;
		EFUSE_DBG("offset = 0x%x, val = 0x%x\n", niddle, mask);
		if (mask) {
			if (uni_burn_key(niddle, mask))
				return EFUSE_ERR_BURN_TIMING;
		}
	}

	/* Already burned bit: Set this bit to indicate it is already burned */
	if ((key_map->burned_flg_offset >= 0) &&
	    (key_map->burned_flg_offset <= EFUSE_BRUN_RD_OFFSET_MASK) &&
	     sid_get_security_status()) {
		_set_cfg_flg(EFUSE_WRITE_PROTECT, key_map->burned_flg_offset);
	}

	/* Read forbidden bit: Set to indicate cpu can not access this key again */
	if ((key_map->rd_fbd_offset >= 0) &&
	    (key_map->rd_fbd_offset <= EFUSE_BRUN_RD_OFFSET_MASK) &&
	     sid_get_security_status()) {
		_set_cfg_flg(EFUSE_READ_PROTECT, key_map->rd_fbd_offset);
	}
	return 0;
}

#define EFUSE_ROUND_UP(x, y) ((((x) + ((y)-1)) / (y)) * (y))

/* This API assume the caller already prepared enough buffer to receive data.
 * Because the lenth of key is exported as MACRO.
 */
int sunxi_efuse_read(void *key_name, void *rd_buf, int *len)
{
	efuse_key_map_new_t *key_map = g_key_info;
	uint tmp = 0, i = 0, k_u32_l = 0, bit_lft = 0;
	int offset = 0, tmp_sz = 0;
	__attribute__((unused)) int show_status = 0;
	unsigned int *u32_p = (unsigned int *)rd_buf;
	unsigned char *u8_p = (unsigned char *)rd_buf;
	int err;

	*len = 0;

	if (!(key_name && rd_buf)) {
		pr_err("key_name or rd_buf is null\n");
		return EFUSE_ERR_ARG;
	}

	/* Search key with name */
	for (; key_map->size != 0; key_map++) {
		if (strlen(key_map->name) != strlen(key_name))
			continue;
		if (!memcmp(key_name, key_map->name, strlen(key_map->name)))
			break;
	}

	/* Key name not found */
	if (key_map->size == 0) {
		pr_err("Unknown key '%s'\n", key_name);
		return EFUSE_ERR_KEY_NAME_WRONG;
	}

	EFUSE_DBG("key = '%s', size = %d, offset = 0x%x\n", key_map->name, key_map->size, key_map->offset);

	err = __efuse_acl_ck(key_map, 0);
	if (err)
		return err;

	k_u32_l = key_map->size / 32;
	bit_lft = key_map->size % 32;
	offset  = key_map->offset;
	for (i = 0; i < k_u32_l; i++) {
		tmp = sid_read_key(offset);
		if (((unsigned long)rd_buf & 0x3) == 0) {
			u32_p[i] = tmp;
		} else {
			memcpy((void *)(u8_p + i * 4), (void *)(&tmp), 4);
		}
		offset += 4;
		tmp_sz += 4;
	}

	if (bit_lft) {
		EFUSE_DBG("bit_lft is %d\n", bit_lft);
		tmp = sid_read_key(offset);
		memcpy((void *)(u8_p + k_u32_l * 4), (void *)(&tmp), EFUSE_ROUND_UP(bit_lft, 8) / 8);
		tmp_sz += EFUSE_ROUND_UP(bit_lft, 8) / 8;
	}
	*len = tmp_sz;

	return 0;
}

/* Check whether the ROTPK is burned */
int sunxi_efuse_get_rotpk_status(void)
{
#ifndef SID_ROTPK_CTRL
	EFUSE_DBG("Not implemented\n");
	return -1;
#else
	int burned;
	uint32_t mask = (1 << SID_ROTPK_EFUSED_BIT);
	burned = (readl(SID_ROTPK_CTRL) & mask) == mask;
	EFUSE_DBG("ROTPK is %s burned\n", burned ? "already" : " NOT");
	return burned;
#endif
}

/* Verify ROTPK with given @hash[].
 * Actually, this is done by comparing burned-ROTPK's hash with @hash[]
 */
int sunxi_efuse_verify_rotpk(u8 *hash)
{
#ifdef SID_ROTPK_CMP_RET_BIT
	int i;
	u32 *tmp = (u32 *)hash;
	u32 val;

	/* Write ROTPK */
	for (i = 0; i < 8 ; i++) {
		writel(tmp[i], SID_ROTPK_VALUE(i));
	}

	/* Start Comparation */
	val = readl(SID_ROTPK_CTRL);
	val |= (0x1U << 31);	/* COMPARE_EN */
	writel(val, SID_ROTPK_CTRL);

	for (i = 0; i < 30; i++) {
		;
	}

	/* Check Comparation Result */
	val = readl(SID_ROTPK_CTRL);
	if (val & (1 << SID_ROTPK_EFUSED_BIT)) {  /* Burned */
		if (val & (1 << SID_ROTPK_CMP_RET_BIT)) {  /* Compare pass */
			EFUSE_DBG("ROTPK verify passed\n");
			return 0;
		} else {
			EFUSE_DBG("ROTPK verify failed\n");
			return -2;
		}
	}
	pr_err("ROTPK not burned\n");
	return 0;
#else
	EFUSE_DBG("Not implemented\n");
	return -1;
#endif
}

/* read key information from sid sram */
u32 sunxi_sid_sram_read(u32 key_index)
{
	return readl((void *)SID_EFUSE + key_index);
}

/* Get SoC's version */
int sunxi_efuse_get_soc_ver(void)
{
#ifdef SID_GET_SOC_VER
	u32 val;
	val = sunxi_sid_sram_read(0);
	val = (val >> SID_SOC_VER_OFFSET) & SID_SOC_VER_MASK;
	return val;
#elif defined(SUNXI_SYSCTRL_BASE)
	int version = -1;
	u32 mask = 0x7;
	u32 ver_reg_offet = 0x24;
	version = readl(IOMEM_ADDR(SUNXI_SYSCTRL_BASE) + ver_reg_offet) & mask;
	EFUSE_DBG("SoC Version: 0x%x\n", version);
	return version;
#else
	EFUSE_DBG("Not implemented\n");
	return 0;
#endif
}

/* Get SoC's mark ID */
int sunxi_efuse_get_markid(void)
{
#if defined(SID_EFUSE)
	u32 sunxi_soc_markid;
	sunxi_soc_markid = sunxi_sid_sram_read(0) & 0xffff;
	EFUSE_DBG("SoC mark ID: 0x%x\n", sunxi_soc_markid);
	return sunxi_soc_markid;
#else
	EFUSE_DBG("Not implemented\n");
	return 0;
#endif
}

/* Enable BROM's verification for FES and Uboot binaries */
void sid_enable_verify_fel(void)
{
#ifdef EFUSE_CONFIG
	uint reg_val;

	reg_val  = sid_read_key(EFUSE_CONFIG);
	if (reg_val & (0x1 << FEL_VERIFY_OFFSET)) {
		EFUSE_DBG("Already burned\n");
		return;
	} else {
		reg_val |= (0x01 << FEL_VERIFY_OFFSET);
		sid_program_key(EFUSE_CONFIG, reg_val);

		reg_val = (sid_read_key(EFUSE_CONFIG) >> FEL_VERIFY_OFFSET) & 1;
		printf("Burn Verify-FEL bit %s\n", reg_val ? "OK" : "FAILED");
	}
#else
	EFUSE_DBG("Not implemented\n");
#endif
}

#ifdef EFUSE_NPU_STATUS_NAME
int sunxi_efuse_get_npu_status(void)
{
	int npu_enabled, ret;

	ret = arm_svc_efuse_read(EFUSE_NPU_STATUS_NAME, &npu_enabled);
	if (ret < 0) {
		printf("not support get sec reg\n");
		return -1;
	}

	npu_enabled = !(npu_enabled & EFUSE_NPU_0T_LMT_VALUE);

	return npu_enabled;
}
#endif
