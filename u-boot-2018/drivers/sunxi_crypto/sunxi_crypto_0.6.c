/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/ce.h>
#include <asm/arch/clock.h>
#include <memalign.h>
#include <sunxi_board.h>
#include "ss_op.h"

__attribute__((weak))
int sunxi_sha_calc_with_software(char *algo_name, u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len)
{
	printf("call weak fun: %s\n", __func__);
	return -1;
}

void sunxi_ss_open(void)
{
#if 0
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	u32 reg_val;
	static int initd;

	if (initd)
		return;
	initd = 1;

	/* enable SS working clock */
	reg_val = readl(&ccm->ss_clk_cfg); /* SS CLOCK */
	reg_val &= ~(0xf << 24);
	reg_val |= 0x1 << 24;
	reg_val &= ~(0x3 << 16);
	reg_val |= 0x0 << 16;
	reg_val &= ~(0xf);
	reg_val |= (4 - 1);
	reg_val |= 0x1U << 31;
	writel(reg_val, &ccm->ss_clk_cfg);
	/* enable SS AHB clock */
	reg_val = readl(&ccm->ahb_gate0);
	reg_val |= 0x1 << 5; /* SS AHB clock on */
	writel(reg_val, &ccm->ahb_gate0);
	/* del-assert SS reset */
	reg_val = readl(&ccm->ahb_reset0_cfg);
	reg_val |= 0x1 << 5; /* SS AHB clock reset */
	writel(reg_val, &ccm->ahb_reset0_cfg);
#endif
}

void sunxi_ss_close(void)
{
}

int sunxi_sha_calc(u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len)
{
	return sunxi_sha_calc_with_software("sha256", dst_addr, dst_len, src_addr, src_len);
}

s32 sunxi_rsa_calc(u8 *n_addr, u32 n_len, u8 *e_addr, u32 e_len, u8 *dst_addr,
		   u32 dst_len, u8 *src_addr, u32 src_len)
{
	printf("not support rsa, use software if you need\n");
	return -1;
}

int sunxi_trng_gen(u8 *rng_buf, u32 rng_byte)
{
	printf("not support trng\n");
	return -1;
}

#ifdef SUNXI_HASH_TEST
int do_sha256_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 hash[32] = { 0 };
	phys_addr_t x1      = simple_strtol(argv[1], NULL, 16);
	u32 x2      = simple_strtol(argv[2], NULL, 16);
	if (argc < 3) {
		return -1;
	}
	printf("src = 0x%x, len = 0x%x\n", x1, x2);

	tick_printf("sha256 test start 0\n");
	sunxi_ss_open();
	sunxi_sha_calc(hash, 32, (u8 *)x1, x2);
	tick_printf("sha256 test end\n");
	sunxi_dump(hash, 32);

	return 0;
}

U_BOOT_CMD(sha256_test, 3, 0, do_sha256_test,
	   "do a sha256 test, arg1: src address, arg2: len(hex)", "NULL");
#endif
