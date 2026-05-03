// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/libfdt.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/ioport.h>
#include <linux/cpumask.h>
#include <linux/minmax.h>
#include <linux/crash_core.h>
#include <linux/utsname.h>
#include <linux/elfcore.h>
#include <linux/elf.h>
#include <linux/kexec.h>
#include <linux/version.h>
#include <asm/system_misc.h>
#include <asm/ptrace.h>
#include <linux/dma-mapping.h>
#include "sunxi-crashnote.h"

spinlock_t cpu_dump_lock;

#if IS_ENABLED(CONFIG_ARM)
static const char *processor_modes[] __maybe_unused = {
  "USER_26", "FIQ_26", "IRQ_26", "SVC_26", "UK4_26", "UK5_26", "UK6_26", "UK7_26",
  "UK8_26", "UK9_26", "UK10_26", "UK11_26", "UK12_26", "UK13_26", "UK14_26", "UK15_26",
  "USER_32", "FIQ_32", "IRQ_32", "SVC_32", "UK4_32", "UK5_32", "MON_32", "ABT_32",
  "UK8_32", "UK9_32", "HYP_32", "UND_32", "UK12_32", "UK13_32", "UK14_32", "SYS_32"
};

static const char *isa_modes[] __maybe_unused = {
  "ARM", "Thumb", "Jazelle", "ThumbEE"
};
#endif

#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
static char *sunxi_crashdump_info;
#endif

void crash_info_init(void)
{
#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
	sunxi_crashdump_info = kzalloc(CRASH_CORE_NOTE_BYTES * 2, GFP_KERNEL);
#endif
	spin_lock_init(&cpu_dump_lock);
}

#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)

void sunxi_crash_save_cpu(struct pt_regs *regs, int cpu)
{
	struct elf_prstatus prstatus;
	u32 *buf;

	if ((cpu < 0) || (cpu >= nr_cpu_ids))
		return;

	buf = (u32 *)per_cpu_ptr(crash_notes, cpu);
	if (!buf)
		return;
	memset(&prstatus, 0, sizeof(prstatus));
	prstatus.common.pr_pid = current->pid;
	elf_core_copy_kernel_regs(&prstatus.pr_reg, regs);
	buf = append_elf_note(buf, CRASH_CORE_NOTE_NAME, NT_PRSTATUS,
			      &prstatus, sizeof(prstatus));
	final_note(buf);
}

void crash_setup_elfheader(void)
{
	struct elf_phdr *nhdr = NULL;
	sunxi_crashdump_info += sizeof(struct elfhdr);
	nhdr = (struct elf_phdr *)sunxi_crashdump_info;
	memset(nhdr, 0, sizeof(struct elf_phdr));
	nhdr->p_memsz = CRASH_CORE_NOTE_BYTES;
}

#endif /*IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)*/

#if IS_ENABLED(CONFIG_ARM)
static void sunxi_show_regs32(struct pt_regs *regs)
{
	unsigned long flags;
	char buf[64];
#ifndef CONFIG_CPU_V7M
	unsigned int domain;
#ifdef CONFIG_CPU_SW_DOMAIN_PAN
	/*
	 * Get the domain register for the parent context. In user
	 * mode, we don't save the DACR, so lets use what it should
	 * be. For other modes, we place it after the pt_regs struct.
	 */
	if (user_mode(regs)) {
		domain = DACR_UACCESS_ENABLE;
	} else {
		domain = to_svc_pt_regs(regs)->dacr;
	}
#else
	domain = get_domain();
#endif
#endif

	show_regs_print_info(KERN_DEFAULT);

	printk("PC is at %pS\n", (void *)instruction_pointer(regs));
	printk("LR is at %pS\n", (void *)regs->ARM_lr);
	printk("pc : [<%08lx>]    lr : [<%08lx>]    psr: %08lx\n",
	       regs->ARM_pc, regs->ARM_lr, regs->ARM_cpsr);
	printk("sp : %08lx  ip : %08lx  fp : %08lx\n",
	       regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	printk("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9,
		regs->ARM_r8);
	printk("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6,
		regs->ARM_r5, regs->ARM_r4);
	printk("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2,
		regs->ARM_r1, regs->ARM_r0);

	flags = regs->ARM_cpsr;
	buf[0] = flags & PSR_N_BIT ? 'N' : 'n';
	buf[1] = flags & PSR_Z_BIT ? 'Z' : 'z';
	buf[2] = flags & PSR_C_BIT ? 'C' : 'c';
	buf[3] = flags & PSR_V_BIT ? 'V' : 'v';
	buf[4] = '\0';

#ifndef CONFIG_CPU_V7M
	{
		const char *segment;

		if ((domain & domain_mask(DOMAIN_USER)) ==
		    domain_val(DOMAIN_USER, DOMAIN_NOACCESS))
			segment = "none";
		else
			segment = "user";

		printk("Flags: %s  IRQs o%s  FIQs o%s  Mode %s  ISA %s  Segment %s\n",
			buf, interrupts_enabled(regs) ? "n" : "ff",
			fast_interrupts_enabled(regs) ? "n" : "ff",
			processor_modes[processor_mode(regs)],
			isa_modes[isa_mode(regs)], segment);
	}
#else
	printk("xPSR: %08lx\n", regs->ARM_cpsr);
#endif

#ifdef CONFIG_CPU_CP15
	{
		unsigned int ctrl;

		buf[0] = '\0';
#ifdef CONFIG_CPU_CP15_MMU
		{
			unsigned int transbase;
			asm("mrc p15, 0, %0, c2, c0\n\t"
			    : "=r" (transbase));
			snprintf(buf, sizeof(buf), "  Table: %08x  DAC: %08x",
				transbase, domain);
		}
#endif
		asm("mrc p15, 0, %0, c1, c0\n" : "=r" (ctrl));

		printk("Control: %08x%s\n", ctrl, buf);
	}
#endif
}
#endif

#if IS_ENABLED(CONFIG_ARM64)
static void sunxi_show_regs64(struct pt_regs *regs)
{
	int i, top_reg;
	u64 lr, sp;

	printk("\ncpu %d regs status:\n", smp_processor_id());
	if (compat_user_mode(regs)) {
		lr = regs->compat_lr;
		sp = regs->compat_sp;
		top_reg = 12;
	} else {
		lr = regs->regs[30];
		sp = regs->sp;
		top_reg = 29;
	}

	if (!user_mode(regs)) {
		printk("pc : %pS\n", (void *)regs->pc);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		printk("lr : %pS\n", (void *)ptrauth_strip_kernel_insn_pac(lr));
#else
		printk("lr : %pS\n", (void *)ptrauth_clear_pac(lr));
#endif
	} else {
		printk("pc : %016llx\n", regs->pc);
		printk("lr : %016llx\n", lr);
	}
	i = top_reg;

	while (i >= 0) {
		printk("x%-2d: %016llx", i, regs->regs[i]);

		while (i-- % 3)
			pr_cont(" x%-2d: %016llx", i, regs->regs[i]);

		pr_cont("\n");
	}
}
#endif

static void sunxi_show_regs(struct pt_regs *regs)
{
#if IS_ENABLED(CONFIG_ARM64)
	sunxi_show_regs64(regs);
#endif

#if IS_ENABLED(CONFIG_ARM)
	sunxi_show_regs32(regs);
#endif
}

void sunxi_dump_status(struct pt_regs *regs)
{
	unsigned long flags;
	spin_lock_irqsave(&cpu_dump_lock, flags);
	sunxi_show_regs(regs);
	dump_stack();
	spin_unlock_irqrestore(&cpu_dump_lock, flags);
}
