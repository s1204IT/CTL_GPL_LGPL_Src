// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Allwinnertech Technology Corporation
 *
 */

#include <common.h>
#include <asm/cache.h>
#include <asm/arch-andes/csr.h>
#include <asm/sbi.h>

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	struct sbiret ret;

	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

void flush_dcache_all(void)
{
	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_DCACHE_WBINVAL_ALL, 0, 0, 0, 0, 0, 0);
}

void flush_dcache_range(unsigned long start, unsigned long end)
{
	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_DCACHE_WB, start, end - start, 0, 0, 0, 0);
}

void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_DCACHE_INVAL, start, end - start, 0, 0, 0, 0);
}

/* We have opened L1&L2 cache in boot0 */
void icache_enable(void)
{
//	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_ICACHE_OP, 1, 0, 0, 0, 0, 0);
}

/* Andes note: open cache before kernel */
void icache_disable(void)
{
//	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_ICACHE_OP, 0, 0, 0, 0, 0, 0);
}

/* We have opened L1&L2 cache in boot0 */
void dcache_enable(void)
{
//	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_ICACHE_OP, 1, 0, 0, 0, 0, 0);
}

/* Andes note: open cache before kernel */
void dcache_disable(void)
{
//	sbi_ecall(SBI_EXT_ANDES, SBI_EXT_ANDES_DCACHE_OP, 0, 0, 0, 0, 0, 0);
}

int icache_status(void)
{
	int ret = 0;

	return ret;
}

int dcache_status(void)
{
	int ret = 0;

	return ret;
}
