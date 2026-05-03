/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2015 Regents of the University of California
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Taken from Linux arch/riscv/include/asm/sbi.h
 */

#ifndef _ASM_RISCV_SBI_H
#define _ASM_RISCV_SBI_H

#include <linux/types.h>

enum sbi_ext_id {
#ifdef CONFIG_SBI_V01
	SBI_EXT_0_1_SET_TIMER = 0x0,
	SBI_EXT_0_1_CONSOLE_PUTCHAR = 0x1,
	SBI_EXT_0_1_CONSOLE_GETCHAR = 0x2,
	SBI_EXT_0_1_CLEAR_IPI = 0x3,
	SBI_EXT_0_1_SEND_IPI = 0x4,
	SBI_EXT_0_1_REMOTE_FENCE_I = 0x5,
	SBI_EXT_0_1_REMOTE_SFENCE_VMA = 0x6,
	SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID = 0x7,
	SBI_EXT_0_1_SHUTDOWN = 0x8,
#endif
	SBI_EXT_BASE = 0x10,
	SBI_EXT_TIME = 0x54494D45,
	SBI_EXT_IPI = 0x735049,
	SBI_EXT_RFENCE = 0x52464E43,
	SBI_EXT_ANDES = 0x0900031E,
};

enum sbi_ext_base_fid {
	SBI_EXT_BASE_GET_SPEC_VERSION = 0,
	SBI_EXT_BASE_GET_IMP_ID,
	SBI_EXT_BASE_GET_IMP_VERSION,
	SBI_EXT_BASE_PROBE_EXT,
	SBI_EXT_BASE_GET_MVENDORID,
	SBI_EXT_BASE_GET_MARCHID,
	SBI_EXT_BASE_GET_MIMPID,
};

enum sbi_ext_andes_fid {
	SBI_EXT_ANDES_GET_MCACHE_CTL_STATUS = 0,
	SBI_EXT_ANDES_GET_MMISC_CTL_STATUS,
	SBI_EXT_ANDES_SET_MCACHE_CTL,
	SBI_EXT_ANDES_SET_MMISC_CTL,
	SBI_EXT_ANDES_ICACHE_OP,
	SBI_EXT_ANDES_DCACHE_OP,
	SBI_EXT_ANDES_L1CACHE_I_PREFETCH,
	SBI_EXT_ANDES_L1CACHE_D_PREFETCH,
	SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE,
	SBI_EXT_ANDES_WRITE_AROUND,
	SBI_EXT_ANDES_TRIGGER,
	SBI_EXT_ANDES_SET_PFM,
	SBI_EXT_ANDES_READ_POWERBRAKE,
	SBI_EXT_ANDES_WRITE_POWERBRAKE,
	SBI_EXT_ANDES_SUSPEND_PREPARE,
	SBI_EXT_ANDES_SUSPEND_MEM,
	SBI_EXT_ANDES_SET_SUSPEND_MODE,
	SBI_EXT_ANDES_ENTER_SUSPEND_MODE,
	SBI_EXT_ANDES_RESTART,
	SBI_EXT_ANDES_SET_RESET_VEC,
	SBI_EXT_ANDES_SET_PMA,
	SBI_EXT_ANDES_FREE_PMA,
	SBI_EXT_ANDES_PROBE_PMA,
	SBI_EXT_ANDES_DCACHE_WBINVAL_ALL,
	SBI_EXT_ANDES_HPM,
	SBI_EXT_ANDES_WAKEUP_SRC_SET,
	SBI_EXT_ANDES_WAKEUP_SRC_CLR,
	SBI_EXT_ANDES_DCACHE_WB,
	SBI_EXT_ANDES_DCACHE_INVAL,
};

enum sbi_ext_time_fid {
	SBI_EXT_TIME_SET_TIMER = 0,
};

enum sbi_ext_ipi_fid {
	SBI_EXT_IPI_SEND_IPI = 0,
};

enum sbi_ext_rfence_fid {
	SBI_EXT_RFENCE_REMOTE_FENCE_I = 0,
	SBI_EXT_RFENCE_REMOTE_SFENCE_VMA,
	SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID,
};

#ifdef CONFIG_SBI_V01
#define SBI_EXT_SET_TIMER		SBI_EXT_0_1_SET_TIMER
#define SBI_FID_SET_TIMER		0
#define SBI_EXT_SEND_IPI		SBI_EXT_0_1_SEND_IPI
#define SBI_FID_SEND_IPI		0
#define SBI_EXT_REMOTE_FENCE_I		SBI_EXT_0_1_REMOTE_FENCE_I
#define SBI_FID_REMOTE_FENCE_I		0
#define SBI_EXT_REMOTE_SFENCE_VMA	SBI_EXT_0_1_REMOTE_SFENCE_VMA
#define SBI_FID_REMOTE_SFENCE_VMA	0
#define SBI_EXT_REMOTE_SFENCE_VMA_ASID	SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID
#define SBI_FID_REMOTE_SFENCE_VMA_ASID	0
#else
#define SBI_EXT_SET_TIMER		SBI_EXT_TIME
#define SBI_FID_SET_TIMER		SBI_EXT_TIME_SET_TIMER
#define SBI_EXT_SEND_IPI		SBI_EXT_IPI
#define SBI_FID_SEND_IPI		SBI_EXT_IPI_SEND_IPI
#define SBI_EXT_REMOTE_FENCE_I		SBI_EXT_RFENCE
#define SBI_FID_REMOTE_FENCE_I		SBI_EXT_RFENCE_REMOTE_FENCE_I
#define SBI_EXT_REMOTE_SFENCE_VMA	SBI_EXT_RFENCE
#define SBI_FID_REMOTE_SFENCE_VMA	SBI_EXT_RFENCE_REMOTE_SFENCE_VMA
#define SBI_EXT_REMOTE_SFENCE_VMA_ASID	SBI_EXT_RFENCE
#define SBI_FID_REMOTE_SFENCE_VMA_ASID	SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID
#endif

#define SBI_SPEC_VERSION_DEFAULT	0x1
#define SBI_SPEC_VERSION_MAJOR_SHIFT	24
#define SBI_SPEC_VERSION_MAJOR_MASK	0x7f
#define SBI_SPEC_VERSION_MINOR_MASK	0xffffff

/* SBI return error codes */
#define SBI_SUCCESS			0
#define SBI_ERR_FAILURE			-1
#define SBI_ERR_NOT_SUPPORTED		-2
#define SBI_ERR_INVALID_PARAM		-3
#define SBI_ERR_DENIED			-4
#define SBI_ERR_INVALID_ADDRESS		-5

extern unsigned long sbi_spec_version;
struct sbiret {
	long error;
	long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5);

#ifdef CONFIG_SBI_V01
void sbi_console_putchar(int ch);
int sbi_console_getchar(void);
void sbi_clear_ipi(void);
void sbi_shutdown(void);
#endif
void sbi_set_timer(uint64_t stime_value);
void sbi_send_ipi(const unsigned long *hart_mask);
void sbi_remote_fence_i(const unsigned long *hart_mask);
void sbi_remote_sfence_vma(const unsigned long *hart_mask,
			   unsigned long start,
			   unsigned long size);
void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
				unsigned long start,
				unsigned long size,
				unsigned long asid);

int sbi_probe_extension(int ext);

#endif
