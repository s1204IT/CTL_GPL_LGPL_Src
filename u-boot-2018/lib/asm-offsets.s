	.arch armv7-a
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 2	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 0	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 2	@ Tag_ABI_PCS_wchar_t
	.file	"asm-offsets.c"
@ GNU C11 (Linaro GCC 7.2-2017.11) version 7.2.1 20171011 (arm-linux-gnueabi)
@	compiled by GNU C version 4.8.4, GMP version 6.1.2, MPFR version 3.1.5, MPC version 1.0.3, isl version none
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -I include -I ./arch/arm/include
@ -I ./include/openssl -I ./arch/arm/mach-sunxi/include -imultilib .
@ -imultiarch arm-linux-gnueabi
@ -iprefix /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/7.2.1/
@ -isysroot /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../arm-linux-gnueabi/libc
@ -D __KERNEL__ -D __UBOOT__ -D __ARM__ -D __LINUX_ARM_ARCH__=7
@ -D DO_DEPS_ONLY -D KBUILD_STR(s)=#s
@ -D KBUILD_BASENAME=KBUILD_STR(asm_offsets)
@ -D KBUILD_MODNAME=KBUILD_STR(asm_offsets)
@ -isystem /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/7.2.1/include
@ -include ./include/linux/kconfig.h -MD lib/.asm-offsets.s.d
@ lib/asm-offsets.c -mthumb -mthumb-interwork -mabi=aapcs-linux
@ -mword-relocations -mno-unaligned-access -mfloat-abi=soft -march=armv7-a
@ -mtune=cortex-a9 -mtls-dialect=gnu -auxbase-strip lib/asm-offsets.s -g
@ -O2 -Wall -Wstrict-prototypes -Wno-format-security -Werror
@ -Wno-format-nonliteral -Werror=date-time -std=gnu11 -fno-builtin
@ -ffreestanding -fshort-wchar -fno-stack-protector
@ -fno-delete-null-pointer-checks -fstack-usage -fno-pic
@ -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -fverbose-asm
@ options enabled:  -faggressive-loop-optimizations -falign-jumps
@ -falign-labels -falign-loops -fauto-inc-dec -fbranch-count-reg
@ -fcaller-saves -fchkp-check-incomplete-type -fchkp-check-read
@ -fchkp-check-write -fchkp-instrument-calls -fchkp-narrow-bounds
@ -fchkp-optimize -fchkp-store-bounds -fchkp-use-static-bounds
@ -fchkp-use-static-const-bounds -fchkp-use-wrappers -fcode-hoisting
@ -fcombine-stack-adjustments -fcompare-elim -fcprop-registers
@ -fcrossjumping -fcse-follow-jumps -fdata-sections -fdefer-pop
@ -fdevirtualize -fdevirtualize-speculatively -fdwarf2-cfi-asm
@ -fearly-inlining -feliminate-unused-debug-types -fexpensive-optimizations
@ -fforward-propagate -ffp-int-builtin-inexact -ffunction-cse
@ -ffunction-sections -fgcse -fgcse-lm -fgnu-runtime -fgnu-unique
@ -fguess-branch-probability -fhoist-adjacent-loads -fident -fif-conversion
@ -fif-conversion2 -findirect-inlining -finline -finline-atomics
@ -finline-functions-called-once -finline-small-functions -fipa-bit-cp
@ -fipa-cp -fipa-icf -fipa-icf-functions -fipa-icf-variables -fipa-profile
@ -fipa-pure-const -fipa-ra -fipa-reference -fipa-sra -fipa-vrp
@ -fira-hoist-pressure -fira-share-save-slots -fira-share-spill-slots
@ -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
@ -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
@ -fmath-errno -fmerge-constants -fmerge-debug-strings
@ -fmove-loop-invariants -fomit-frame-pointer -foptimize-sibling-calls
@ -foptimize-strlen -fpartial-inlining -fpeephole -fpeephole2 -fplt
@ -fprefetch-loop-arrays -freg-struct-return -freorder-blocks
@ -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-insns
@ -fschedule-insns2 -fsection-anchors -fsemantic-interposition
@ -fshow-column -fshrink-wrap -fshrink-wrap-separate -fsigned-zeros
@ -fsplit-ivs-in-unroller -fsplit-wide-types -fssa-backprop -fssa-phiopt
@ -fstdarg-opt -fstore-merging -fstrict-aliasing -fstrict-overflow
@ -fstrict-volatile-bitfields -fsync-libcalls -fthread-jumps
@ -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp -ftree-builtin-call-dce
@ -ftree-ccp -ftree-ch -ftree-coalesce-vars -ftree-copy-prop -ftree-cselim
@ -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-forwprop -ftree-fre
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop -ftree-pre
@ -ftree-pta -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr
@ -ftree-sra -ftree-switch-conversion -ftree-tail-merge -ftree-ter
@ -ftree-vrp -funit-at-a-time -fvar-tracking -fvar-tracking-assignments
@ -fverbose-asm -fzero-initialized-in-bss -masm-syntax-unified -mglibc
@ -mlittle-endian -mpic-data-is-text-relative -msched-prolog -mthumb
@ -mvectorize-with-neon-quad -mword-relocations

	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.startup.main,"ax",%progbits
	.align	1
	.p2align 2,,3
	.global	main
	.syntax unified
	.thumb
	.thumb_func
	.fpu softvfp
	.type	main, %function
main:
.LFB177:
	.file 1 "lib/asm-offsets.c"
	.loc 1 19 0
	.cfi_startproc
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
@ lib/asm-offsets.c:21: 	DEFINE(GENERATED_GBL_DATA_SIZE,
	.loc 1 21 0
	.syntax unified
@ 21 "lib/asm-offsets.c" 1
	
.ascii "->GENERATED_GBL_DATA_SIZE #368 (sizeof(struct global_data) + 15) & ~15"	@
@ 0 "" 2
@ lib/asm-offsets.c:24: 	DEFINE(GENERATED_BD_INFO_SIZE,
	.loc 1 24 0
@ 24 "lib/asm-offsets.c" 1
	
.ascii "->GENERATED_BD_INFO_SIZE #96 (sizeof(struct bd_info) + 15) & ~15"	@
@ 0 "" 2
@ lib/asm-offsets.c:27: 	DEFINE(GD_SIZE, sizeof(struct global_data));
	.loc 1 27 0
@ 27 "lib/asm-offsets.c" 1
	
.ascii "->GD_SIZE #368 sizeof(struct global_data)"	@
@ 0 "" 2
@ lib/asm-offsets.c:29: 	DEFINE(GD_BD, offsetof(struct global_data, bd));
	.loc 1 29 0
@ 29 "lib/asm-offsets.c" 1
	
.ascii "->GD_BD #0 offsetof(struct global_data, bd)"	@
@ 0 "" 2
@ lib/asm-offsets.c:34: 	DEFINE(GD_RELOCADDR, offsetof(struct global_data, relocaddr));
	.loc 1 34 0
@ 34 "lib/asm-offsets.c" 1
	
.ascii "->GD_RELOCADDR #60 offsetof(struct global_data, relocaddr)"	@
@ 0 "" 2
@ lib/asm-offsets.c:36: 	DEFINE(GD_RELOC_OFF, offsetof(struct global_data, reloc_off));
	.loc 1 36 0
@ 36 "lib/asm-offsets.c" 1
	
.ascii "->GD_RELOC_OFF #84 offsetof(struct global_data, reloc_off)"	@
@ 0 "" 2
@ lib/asm-offsets.c:38: 	DEFINE(GD_START_ADDR_SP, offsetof(struct global_data, start_addr_sp));
	.loc 1 38 0
@ 38 "lib/asm-offsets.c" 1
	
.ascii "->GD_START_ADDR_SP #80 offsetof(struct global_data, start_addr_sp)"	@
@ 0 "" 2
@ lib/asm-offsets.c:40: 	DEFINE(GD_NEW_GD, offsetof(struct global_data, new_gd));
	.loc 1 40 0
@ 40 "lib/asm-offsets.c" 1
	
.ascii "->GD_NEW_GD #88 offsetof(struct global_data, new_gd)"	@
@ 0 "" 2
@ lib/asm-offsets.c:43: }
	.loc 1 43 0
	.thumb
	.syntax unified
	movs	r0, #0	@,
	bx	lr	@
	.cfi_endproc
.LFE177:
	.size	main, .-main
	.text
.Letext0:
	.file 2 "include/common.h"
	.file 3 "./arch/arm/include/asm/types.h"
	.file 4 "include/linux/types.h"
	.file 5 "include/errno.h"
	.file 6 "include/linux/string.h"
	.file 7 "include/efi.h"
	.file 8 "include/ide.h"
	.file 9 "include/part.h"
	.file 10 "include/flash.h"
	.file 11 "include/lmb.h"
	.file 12 "include/asm-generic/u-boot.h"
	.file 13 "./arch/arm/include/asm/u-boot-arm.h"
	.file 14 "include/image.h"
	.file 15 "include/init.h"
	.file 16 "include/net.h"
	.file 17 "include/environment.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0xab2
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF166
	.byte	0xc
	.4byte	.LASF167
	.4byte	.LASF168
	.4byte	.Ldebug_ranges0+0
	.4byte	0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.4byte	.LASF4
	.byte	0x2
	.byte	0xc
	.4byte	0x30
	.uleb128 0x3
	.byte	0x1
	.byte	0x8
	.4byte	.LASF0
	.uleb128 0x4
	.4byte	0x30
	.uleb128 0x3
	.byte	0x4
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x3
	.byte	0x2
	.byte	0x7
	.4byte	.LASF2
	.uleb128 0x5
	.4byte	.LASF20
	.byte	0x5
	.byte	0xb
	.4byte	0x55
	.uleb128 0x6
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x3
	.byte	0x1
	.byte	0x6
	.4byte	.LASF3
	.uleb128 0x2
	.4byte	.LASF5
	.byte	0x3
	.byte	0xc
	.4byte	0x30
	.uleb128 0x3
	.byte	0x2
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.4byte	.LASF7
	.byte	0x3
	.byte	0x12
	.4byte	0x80
	.uleb128 0x3
	.byte	0x4
	.byte	0x7
	.4byte	.LASF8
	.uleb128 0x3
	.byte	0x8
	.byte	0x5
	.4byte	.LASF9
	.uleb128 0x3
	.byte	0x8
	.byte	0x7
	.4byte	.LASF10
	.uleb128 0x7
	.ascii	"u8\000"
	.byte	0x3
	.byte	0x1f
	.4byte	0x30
	.uleb128 0x4
	.4byte	0x95
	.uleb128 0x7
	.ascii	"u32\000"
	.byte	0x3
	.byte	0x25
	.4byte	0x80
	.uleb128 0x2
	.4byte	.LASF11
	.byte	0x3
	.byte	0x31
	.4byte	0x8e
	.uleb128 0x2
	.4byte	.LASF12
	.byte	0x3
	.byte	0x32
	.4byte	0x8e
	.uleb128 0x3
	.byte	0x1
	.byte	0x8
	.4byte	.LASF13
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.4byte	.LASF14
	.uleb128 0x8
	.byte	0x4
	.4byte	0xc5
	.uleb128 0x2
	.4byte	.LASF15
	.byte	0x4
	.byte	0x59
	.4byte	0x43
	.uleb128 0x2
	.4byte	.LASF16
	.byte	0x4
	.byte	0x5b
	.4byte	0x3c
	.uleb128 0x2
	.4byte	.LASF17
	.byte	0x4
	.byte	0x69
	.4byte	0x63
	.uleb128 0x2
	.4byte	.LASF18
	.byte	0x4
	.byte	0x6b
	.4byte	0x75
	.uleb128 0x2
	.4byte	.LASF19
	.byte	0x4
	.byte	0x97
	.4byte	0x75
	.uleb128 0x9
	.byte	0x4
	.uleb128 0x5
	.4byte	.LASF21
	.byte	0x6
	.byte	0xb
	.4byte	0xd3
	.uleb128 0x3
	.byte	0x1
	.byte	0x2
	.4byte	.LASF22
	.uleb128 0xa
	.4byte	0xc5
	.4byte	0x12f
	.uleb128 0xb
	.byte	0
	.uleb128 0xc
	.4byte	.LASF23
	.byte	0x7
	.2byte	0x162
	.4byte	0x124
	.uleb128 0xc
	.4byte	.LASF24
	.byte	0x7
	.2byte	0x165
	.4byte	0x124
	.uleb128 0xc
	.4byte	.LASF25
	.byte	0x7
	.2byte	0x165
	.4byte	0x124
	.uleb128 0xa
	.4byte	0xe4
	.4byte	0x15e
	.uleb128 0xb
	.byte	0
	.uleb128 0x5
	.4byte	.LASF26
	.byte	0x8
	.byte	0xf
	.4byte	0x153
	.uleb128 0xa
	.4byte	0x30
	.4byte	0x179
	.uleb128 0xd
	.4byte	0x80
	.byte	0x5
	.byte	0
	.uleb128 0xe
	.4byte	.LASF29
	.byte	0x8
	.byte	0x9
	.byte	0xe
	.4byte	0x19e
	.uleb128 0xf
	.4byte	.LASF27
	.byte	0x9
	.byte	0xf
	.4byte	0xd3
	.byte	0
	.uleb128 0xf
	.4byte	.LASF28
	.byte	0x9
	.byte	0x10
	.4byte	0x1b7
	.byte	0x4
	.byte	0
	.uleb128 0x4
	.4byte	0x179
	.uleb128 0x10
	.4byte	0x55
	.4byte	0x1b7
	.uleb128 0x11
	.4byte	0x55
	.uleb128 0x11
	.4byte	0x55
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x1a3
	.uleb128 0xa
	.4byte	0x19e
	.4byte	0x1c8
	.uleb128 0xb
	.byte	0
	.uleb128 0x4
	.4byte	0x1bd
	.uleb128 0x5
	.4byte	.LASF29
	.byte	0x9
	.byte	0xdb
	.4byte	0x1c8
	.uleb128 0x12
	.2byte	0xa0c
	.byte	0xa
	.byte	0x12
	.4byte	0x21f
	.uleb128 0xf
	.4byte	.LASF30
	.byte	0xa
	.byte	0x13
	.4byte	0xe4
	.byte	0
	.uleb128 0xf
	.4byte	.LASF31
	.byte	0xa
	.byte	0x14
	.4byte	0xd9
	.byte	0x4
	.uleb128 0xf
	.4byte	.LASF32
	.byte	0xa
	.byte	0x15
	.4byte	0xe4
	.byte	0x8
	.uleb128 0xf
	.4byte	.LASF33
	.byte	0xa
	.byte	0x16
	.4byte	0x21f
	.byte	0xc
	.uleb128 0x13
	.4byte	.LASF34
	.byte	0xa
	.byte	0x17
	.4byte	0x230
	.2byte	0x80c
	.byte	0
	.uleb128 0xa
	.4byte	0xe4
	.4byte	0x230
	.uleb128 0x14
	.4byte	0x80
	.2byte	0x1ff
	.byte	0
	.uleb128 0xa
	.4byte	0x25
	.4byte	0x241
	.uleb128 0x14
	.4byte	0x80
	.2byte	0x1ff
	.byte	0
	.uleb128 0x2
	.4byte	.LASF35
	.byte	0xa
	.byte	0x36
	.4byte	0x1d8
	.uleb128 0xa
	.4byte	0x241
	.4byte	0x257
	.uleb128 0xb
	.byte	0
	.uleb128 0x5
	.4byte	.LASF36
	.byte	0xa
	.byte	0x38
	.4byte	0x24c
	.uleb128 0x3
	.byte	0x8
	.byte	0x4
	.4byte	.LASF37
	.uleb128 0xe
	.4byte	.LASF38
	.byte	0x10
	.byte	0xb
	.byte	0xf
	.4byte	0x28e
	.uleb128 0xf
	.4byte	.LASF39
	.byte	0xb
	.byte	0x10
	.4byte	0xaf
	.byte	0
	.uleb128 0xf
	.4byte	.LASF30
	.byte	0xb
	.byte	0x11
	.4byte	0xba
	.byte	0x8
	.byte	0
	.uleb128 0x15
	.4byte	.LASF40
	.2byte	0x120
	.byte	0xb
	.byte	0x14
	.4byte	0x2c0
	.uleb128 0x16
	.ascii	"cnt\000"
	.byte	0xb
	.byte	0x15
	.4byte	0x3c
	.byte	0
	.uleb128 0xf
	.4byte	.LASF30
	.byte	0xb
	.byte	0x16
	.4byte	0xba
	.byte	0x8
	.uleb128 0xf
	.4byte	.LASF41
	.byte	0xb
	.byte	0x17
	.4byte	0x2c0
	.byte	0x10
	.byte	0
	.uleb128 0xa
	.4byte	0x269
	.4byte	0x2d0
	.uleb128 0xd
	.4byte	0x80
	.byte	0x10
	.byte	0
	.uleb128 0x17
	.ascii	"lmb\000"
	.2byte	0x240
	.byte	0xb
	.byte	0x1a
	.4byte	0x2f7
	.uleb128 0xf
	.4byte	.LASF42
	.byte	0xb
	.byte	0x1b
	.4byte	0x28e
	.byte	0
	.uleb128 0x13
	.4byte	.LASF43
	.byte	0xb
	.byte	0x1c
	.4byte	0x28e
	.2byte	0x120
	.byte	0
	.uleb128 0x18
	.byte	0x10
	.byte	0xc
	.byte	0x59
	.4byte	0x318
	.uleb128 0xf
	.4byte	.LASF33
	.byte	0xc
	.byte	0x5a
	.4byte	0xaf
	.byte	0
	.uleb128 0xf
	.4byte	.LASF30
	.byte	0xc
	.byte	0x5b
	.4byte	0xba
	.byte	0x8
	.byte	0
	.uleb128 0xe
	.4byte	.LASF44
	.byte	0x60
	.byte	0xc
	.byte	0x1a
	.4byte	0x409
	.uleb128 0xf
	.4byte	.LASF45
	.byte	0xc
	.byte	0x1b
	.4byte	0x3c
	.byte	0
	.uleb128 0xf
	.4byte	.LASF46
	.byte	0xc
	.byte	0x1c
	.4byte	0xba
	.byte	0x8
	.uleb128 0xf
	.4byte	.LASF47
	.byte	0xc
	.byte	0x1d
	.4byte	0x3c
	.byte	0x10
	.uleb128 0xf
	.4byte	.LASF48
	.byte	0xc
	.byte	0x1e
	.4byte	0x3c
	.byte	0x14
	.uleb128 0xf
	.4byte	.LASF49
	.byte	0xc
	.byte	0x1f
	.4byte	0x3c
	.byte	0x18
	.uleb128 0xf
	.4byte	.LASF50
	.byte	0xc
	.byte	0x20
	.4byte	0x3c
	.byte	0x1c
	.uleb128 0xf
	.4byte	.LASF51
	.byte	0xc
	.byte	0x21
	.4byte	0x3c
	.byte	0x20
	.uleb128 0xf
	.4byte	.LASF52
	.byte	0xc
	.byte	0x23
	.4byte	0x3c
	.byte	0x24
	.uleb128 0xf
	.4byte	.LASF53
	.byte	0xc
	.byte	0x24
	.4byte	0x3c
	.byte	0x28
	.uleb128 0xf
	.4byte	.LASF54
	.byte	0xc
	.byte	0x25
	.4byte	0x3c
	.byte	0x2c
	.uleb128 0xf
	.4byte	.LASF55
	.byte	0xc
	.byte	0x30
	.4byte	0x3c
	.byte	0x30
	.uleb128 0xf
	.4byte	.LASF56
	.byte	0xc
	.byte	0x31
	.4byte	0x3c
	.byte	0x34
	.uleb128 0xf
	.4byte	.LASF57
	.byte	0xc
	.byte	0x32
	.4byte	0x169
	.byte	0x38
	.uleb128 0xf
	.4byte	.LASF58
	.byte	0xc
	.byte	0x33
	.4byte	0x43
	.byte	0x3e
	.uleb128 0xf
	.4byte	.LASF59
	.byte	0xc
	.byte	0x34
	.4byte	0x3c
	.byte	0x40
	.uleb128 0xf
	.4byte	.LASF60
	.byte	0xc
	.byte	0x35
	.4byte	0x3c
	.byte	0x44
	.uleb128 0xf
	.4byte	.LASF61
	.byte	0xc
	.byte	0x56
	.4byte	0xe4
	.byte	0x48
	.uleb128 0xf
	.4byte	.LASF62
	.byte	0xc
	.byte	0x57
	.4byte	0xe4
	.byte	0x4c
	.uleb128 0xf
	.4byte	.LASF63
	.byte	0xc
	.byte	0x5c
	.4byte	0x409
	.byte	0x50
	.byte	0
	.uleb128 0xa
	.4byte	0x2f7
	.4byte	0x419
	.uleb128 0xd
	.4byte	0x80
	.byte	0
	.byte	0
	.uleb128 0x2
	.4byte	.LASF64
	.byte	0xc
	.byte	0x5e
	.4byte	0x318
	.uleb128 0x5
	.4byte	.LASF65
	.byte	0xd
	.byte	0x12
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF66
	.byte	0xd
	.byte	0x13
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF67
	.byte	0xd
	.byte	0x14
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF68
	.byte	0xd
	.byte	0x15
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF69
	.byte	0xd
	.byte	0x16
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF70
	.byte	0xd
	.byte	0x17
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF71
	.byte	0xd
	.byte	0x18
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF72
	.byte	0xe
	.byte	0x73
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF73
	.byte	0xe
	.byte	0x74
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF74
	.byte	0xe
	.byte	0x75
	.4byte	0xe4
	.uleb128 0x19
	.4byte	.LASF75
	.byte	0x40
	.byte	0xe
	.2byte	0x13f
	.4byte	0x53c
	.uleb128 0x1a
	.4byte	.LASF76
	.byte	0xe
	.2byte	0x140
	.4byte	0x105
	.byte	0
	.uleb128 0x1a
	.4byte	.LASF77
	.byte	0xe
	.2byte	0x141
	.4byte	0x105
	.byte	0x4
	.uleb128 0x1a
	.4byte	.LASF78
	.byte	0xe
	.2byte	0x142
	.4byte	0x105
	.byte	0x8
	.uleb128 0x1a
	.4byte	.LASF79
	.byte	0xe
	.2byte	0x143
	.4byte	0x105
	.byte	0xc
	.uleb128 0x1a
	.4byte	.LASF80
	.byte	0xe
	.2byte	0x144
	.4byte	0x105
	.byte	0x10
	.uleb128 0x1a
	.4byte	.LASF81
	.byte	0xe
	.2byte	0x145
	.4byte	0x105
	.byte	0x14
	.uleb128 0x1a
	.4byte	.LASF82
	.byte	0xe
	.2byte	0x146
	.4byte	0x105
	.byte	0x18
	.uleb128 0x1a
	.4byte	.LASF83
	.byte	0xe
	.2byte	0x147
	.4byte	0xef
	.byte	0x1c
	.uleb128 0x1a
	.4byte	.LASF84
	.byte	0xe
	.2byte	0x148
	.4byte	0xef
	.byte	0x1d
	.uleb128 0x1a
	.4byte	.LASF85
	.byte	0xe
	.2byte	0x149
	.4byte	0xef
	.byte	0x1e
	.uleb128 0x1a
	.4byte	.LASF86
	.byte	0xe
	.2byte	0x14a
	.4byte	0xef
	.byte	0x1f
	.uleb128 0x1a
	.4byte	.LASF87
	.byte	0xe
	.2byte	0x14b
	.4byte	0x53c
	.byte	0x20
	.byte	0
	.uleb128 0xa
	.4byte	0xef
	.4byte	0x54c
	.uleb128 0xd
	.4byte	0x80
	.byte	0x1f
	.byte	0
	.uleb128 0x1b
	.4byte	.LASF88
	.byte	0xe
	.2byte	0x14c
	.4byte	0x492
	.uleb128 0x19
	.4byte	.LASF89
	.byte	0x18
	.byte	0xe
	.2byte	0x14e
	.4byte	0x5da
	.uleb128 0x1a
	.4byte	.LASF33
	.byte	0xe
	.2byte	0x14f
	.4byte	0xe4
	.byte	0
	.uleb128 0x1c
	.ascii	"end\000"
	.byte	0xe
	.2byte	0x14f
	.4byte	0xe4
	.byte	0x4
	.uleb128 0x1a
	.4byte	.LASF90
	.byte	0xe
	.2byte	0x150
	.4byte	0xe4
	.byte	0x8
	.uleb128 0x1a
	.4byte	.LASF91
	.byte	0xe
	.2byte	0x150
	.4byte	0xe4
	.byte	0xc
	.uleb128 0x1a
	.4byte	.LASF92
	.byte	0xe
	.2byte	0x151
	.4byte	0xe4
	.byte	0x10
	.uleb128 0x1a
	.4byte	.LASF93
	.byte	0xe
	.2byte	0x152
	.4byte	0xef
	.byte	0x14
	.uleb128 0x1a
	.4byte	.LASF94
	.byte	0xe
	.2byte	0x152
	.4byte	0xef
	.byte	0x15
	.uleb128 0x1c
	.ascii	"os\000"
	.byte	0xe
	.2byte	0x152
	.4byte	0xef
	.byte	0x16
	.uleb128 0x1a
	.4byte	.LASF95
	.byte	0xe
	.2byte	0x153
	.4byte	0xef
	.byte	0x17
	.byte	0
	.uleb128 0x1b
	.4byte	.LASF96
	.byte	0xe
	.2byte	0x154
	.4byte	0x558
	.uleb128 0x1d
	.4byte	.LASF97
	.2byte	0x2d0
	.byte	0xe
	.2byte	0x15a
	.4byte	0x6d0
	.uleb128 0x1a
	.4byte	.LASF98
	.byte	0xe
	.2byte	0x160
	.4byte	0x6d0
	.byte	0
	.uleb128 0x1a
	.4byte	.LASF99
	.byte	0xe
	.2byte	0x161
	.4byte	0x54c
	.byte	0x4
	.uleb128 0x1a
	.4byte	.LASF100
	.byte	0xe
	.2byte	0x162
	.4byte	0xe4
	.byte	0x44
	.uleb128 0x1c
	.ascii	"os\000"
	.byte	0xe
	.2byte	0x179
	.4byte	0x5da
	.byte	0x48
	.uleb128 0x1c
	.ascii	"ep\000"
	.byte	0xe
	.2byte	0x17a
	.4byte	0xe4
	.byte	0x60
	.uleb128 0x1a
	.4byte	.LASF101
	.byte	0xe
	.2byte	0x17c
	.4byte	0xe4
	.byte	0x64
	.uleb128 0x1a
	.4byte	.LASF102
	.byte	0xe
	.2byte	0x17c
	.4byte	0xe4
	.byte	0x68
	.uleb128 0x1a
	.4byte	.LASF103
	.byte	0xe
	.2byte	0x17e
	.4byte	0xd3
	.byte	0x6c
	.uleb128 0x1a
	.4byte	.LASF104
	.byte	0xe
	.2byte	0x17f
	.4byte	0xe4
	.byte	0x70
	.uleb128 0x1a
	.4byte	.LASF105
	.byte	0xe
	.2byte	0x181
	.4byte	0xe4
	.byte	0x74
	.uleb128 0x1a
	.4byte	.LASF106
	.byte	0xe
	.2byte	0x182
	.4byte	0xe4
	.byte	0x78
	.uleb128 0x1a
	.4byte	.LASF107
	.byte	0xe
	.2byte	0x183
	.4byte	0xe4
	.byte	0x7c
	.uleb128 0x1a
	.4byte	.LASF108
	.byte	0xe
	.2byte	0x184
	.4byte	0xe4
	.byte	0x80
	.uleb128 0x1c
	.ascii	"kbd\000"
	.byte	0xe
	.2byte	0x185
	.4byte	0x6d6
	.byte	0x84
	.uleb128 0x1a
	.4byte	.LASF109
	.byte	0xe
	.2byte	0x188
	.4byte	0x55
	.byte	0x88
	.uleb128 0x1a
	.4byte	.LASF110
	.byte	0xe
	.2byte	0x195
	.4byte	0x55
	.byte	0x8c
	.uleb128 0x1c
	.ascii	"lmb\000"
	.byte	0xe
	.2byte	0x198
	.4byte	0x2d0
	.byte	0x90
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x54c
	.uleb128 0x8
	.byte	0x4
	.4byte	0x419
	.uleb128 0x1b
	.4byte	.LASF111
	.byte	0xe
	.2byte	0x19a
	.4byte	0x5e6
	.uleb128 0xc
	.4byte	.LASF112
	.byte	0xe
	.2byte	0x19c
	.4byte	0x6dc
	.uleb128 0xa
	.4byte	0xc5
	.4byte	0x704
	.uleb128 0xd
	.4byte	0x80
	.byte	0x1f
	.byte	0
	.uleb128 0x5
	.4byte	.LASF113
	.byte	0xf
	.byte	0x7b
	.4byte	0xe4
	.uleb128 0xa
	.4byte	0x95
	.4byte	0x71a
	.uleb128 0xb
	.byte	0
	.uleb128 0x5
	.4byte	.LASF114
	.byte	0x2
	.byte	0x6c
	.4byte	0x70f
	.uleb128 0x5
	.4byte	.LASF115
	.byte	0x2
	.byte	0x6d
	.4byte	0x70f
	.uleb128 0x5
	.4byte	.LASF116
	.byte	0x2
	.byte	0x89
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF117
	.byte	0x2
	.byte	0x8a
	.4byte	0xe4
	.uleb128 0x5
	.4byte	.LASF118
	.byte	0x2
	.byte	0x8b
	.4byte	0xe4
	.uleb128 0xe
	.4byte	.LASF119
	.byte	0x4
	.byte	0x10
	.byte	0x2e
	.4byte	0x76a
	.uleb128 0xf
	.4byte	.LASF120
	.byte	0x10
	.byte	0x2f
	.4byte	0x105
	.byte	0
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x25
	.uleb128 0xe
	.4byte	.LASF121
	.byte	0x48
	.byte	0x10
	.byte	0xa6
	.4byte	0x80d
	.uleb128 0xf
	.4byte	.LASF27
	.byte	0x10
	.byte	0xa8
	.4byte	0x80d
	.byte	0
	.uleb128 0xf
	.4byte	.LASF122
	.byte	0x10
	.byte	0xa9
	.4byte	0x169
	.byte	0x10
	.uleb128 0xf
	.4byte	.LASF123
	.byte	0x10
	.byte	0xaa
	.4byte	0xaf
	.byte	0x18
	.uleb128 0xf
	.4byte	.LASF110
	.byte	0x10
	.byte	0xab
	.4byte	0x55
	.byte	0x20
	.uleb128 0xf
	.4byte	.LASF124
	.byte	0x10
	.byte	0xad
	.4byte	0x837
	.byte	0x24
	.uleb128 0xf
	.4byte	.LASF125
	.byte	0x10
	.byte	0xae
	.4byte	0x856
	.byte	0x28
	.uleb128 0xf
	.4byte	.LASF126
	.byte	0x10
	.byte	0xaf
	.4byte	0x86b
	.byte	0x2c
	.uleb128 0xf
	.4byte	.LASF127
	.byte	0x10
	.byte	0xb0
	.4byte	0x87c
	.byte	0x30
	.uleb128 0xf
	.4byte	.LASF128
	.byte	0x10
	.byte	0xb4
	.4byte	0x86b
	.byte	0x34
	.uleb128 0xf
	.4byte	.LASF129
	.byte	0x10
	.byte	0xb5
	.4byte	0x831
	.byte	0x38
	.uleb128 0xf
	.4byte	.LASF130
	.byte	0x10
	.byte	0xb6
	.4byte	0x55
	.byte	0x3c
	.uleb128 0xf
	.4byte	.LASF131
	.byte	0x10
	.byte	0xb7
	.4byte	0x110
	.byte	0x40
	.byte	0
	.uleb128 0xa
	.4byte	0xc5
	.4byte	0x81d
	.uleb128 0xd
	.4byte	0x80
	.byte	0xf
	.byte	0
	.uleb128 0x10
	.4byte	0x55
	.4byte	0x831
	.uleb128 0x11
	.4byte	0x831
	.uleb128 0x11
	.4byte	0x6d6
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x770
	.uleb128 0x8
	.byte	0x4
	.4byte	0x81d
	.uleb128 0x10
	.4byte	0x55
	.4byte	0x856
	.uleb128 0x11
	.4byte	0x831
	.uleb128 0x11
	.4byte	0x110
	.uleb128 0x11
	.4byte	0x55
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x83d
	.uleb128 0x10
	.4byte	0x55
	.4byte	0x86b
	.uleb128 0x11
	.4byte	0x831
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x85c
	.uleb128 0x1e
	.4byte	0x87c
	.uleb128 0x11
	.4byte	0x831
	.byte	0
	.uleb128 0x8
	.byte	0x4
	.4byte	0x871
	.uleb128 0x5
	.4byte	.LASF132
	.byte	0x10
	.byte	0xbd
	.4byte	0x831
	.uleb128 0xa
	.4byte	0x95
	.4byte	0x89d
	.uleb128 0xd
	.4byte	0x80
	.byte	0x5
	.byte	0
	.uleb128 0xc
	.4byte	.LASF133
	.byte	0x10
	.2byte	0x1fb
	.4byte	0x751
	.uleb128 0xc
	.4byte	.LASF134
	.byte	0x10
	.2byte	0x1fc
	.4byte	0x751
	.uleb128 0xc
	.4byte	.LASF135
	.byte	0x10
	.2byte	0x1fe
	.4byte	0x751
	.uleb128 0xc
	.4byte	.LASF136
	.byte	0x10
	.2byte	0x203
	.4byte	0x6f4
	.uleb128 0xc
	.4byte	.LASF137
	.byte	0x10
	.2byte	0x204
	.4byte	0x6f4
	.uleb128 0xa
	.4byte	0xc5
	.4byte	0x8e9
	.uleb128 0xd
	.4byte	0x80
	.byte	0x3f
	.byte	0
	.uleb128 0xc
	.4byte	.LASF138
	.byte	0x10
	.2byte	0x205
	.4byte	0x8d9
	.uleb128 0xc
	.4byte	.LASF139
	.byte	0x10
	.2byte	0x207
	.4byte	0x88d
	.uleb128 0xc
	.4byte	.LASF140
	.byte	0x10
	.2byte	0x208
	.4byte	0x88d
	.uleb128 0xc
	.4byte	.LASF141
	.byte	0x10
	.2byte	0x209
	.4byte	0x751
	.uleb128 0xc
	.4byte	.LASF142
	.byte	0x10
	.2byte	0x20a
	.4byte	0x751
	.uleb128 0xc
	.4byte	.LASF143
	.byte	0x10
	.2byte	0x20b
	.4byte	0x76a
	.uleb128 0xa
	.4byte	0x76a
	.4byte	0x941
	.uleb128 0xd
	.4byte	0x80
	.byte	0x3
	.byte	0
	.uleb128 0xc
	.4byte	.LASF144
	.byte	0x10
	.2byte	0x20c
	.4byte	0x931
	.uleb128 0xc
	.4byte	.LASF145
	.byte	0x10
	.2byte	0x20d
	.4byte	0x76a
	.uleb128 0xc
	.4byte	.LASF146
	.byte	0x10
	.2byte	0x20e
	.4byte	0x55
	.uleb128 0xa
	.4byte	0x9f
	.4byte	0x975
	.uleb128 0xd
	.4byte	0x80
	.byte	0x5
	.byte	0
	.uleb128 0x4
	.4byte	0x965
	.uleb128 0xc
	.4byte	.LASF147
	.byte	0x10
	.2byte	0x20f
	.4byte	0x975
	.uleb128 0xc
	.4byte	.LASF148
	.byte	0x10
	.2byte	0x210
	.4byte	0x975
	.uleb128 0xc
	.4byte	.LASF149
	.byte	0x10
	.2byte	0x214
	.4byte	0xd9
	.uleb128 0xc
	.4byte	.LASF150
	.byte	0x10
	.2byte	0x215
	.4byte	0xd9
	.uleb128 0xc
	.4byte	.LASF151
	.byte	0x10
	.2byte	0x217
	.4byte	0x55
	.uleb128 0xa
	.4byte	0xc5
	.4byte	0x9c7
	.uleb128 0x14
	.4byte	0x80
	.2byte	0x3ff
	.byte	0
	.uleb128 0xc
	.4byte	.LASF152
	.byte	0x10
	.2byte	0x21e
	.4byte	0x9b6
	.uleb128 0xc
	.4byte	.LASF153
	.byte	0x10
	.2byte	0x220
	.4byte	0x11d
	.uleb128 0xc
	.4byte	.LASF154
	.byte	0x10
	.2byte	0x222
	.4byte	0xa4
	.uleb128 0xc
	.4byte	.LASF155
	.byte	0x10
	.2byte	0x224
	.4byte	0xa4
	.uleb128 0x1f
	.4byte	.LASF169
	.byte	0x7
	.byte	0x4
	.4byte	0x80
	.byte	0x10
	.2byte	0x282
	.4byte	0xa22
	.uleb128 0x20
	.4byte	.LASF156
	.byte	0
	.uleb128 0x20
	.4byte	.LASF157
	.byte	0x1
	.uleb128 0x20
	.4byte	.LASF158
	.byte	0x2
	.uleb128 0x20
	.4byte	.LASF159
	.byte	0x3
	.byte	0
	.uleb128 0xc
	.4byte	.LASF160
	.byte	0x10
	.2byte	0x288
	.4byte	0x9f7
	.uleb128 0x21
	.4byte	.LASF161
	.4byte	0x20000
	.byte	0x11
	.byte	0x93
	.4byte	0xa56
	.uleb128 0x16
	.ascii	"crc\000"
	.byte	0x11
	.byte	0x94
	.4byte	0xfa
	.byte	0
	.uleb128 0xf
	.4byte	.LASF162
	.byte	0x11
	.byte	0x98
	.4byte	0xa56
	.byte	0x4
	.byte	0
	.uleb128 0xa
	.4byte	0x30
	.4byte	0xa69
	.uleb128 0x22
	.4byte	0x80
	.4byte	0x1fffb
	.byte	0
	.uleb128 0x2
	.4byte	.LASF163
	.byte	0x11
	.byte	0x99
	.4byte	0xa2e
	.uleb128 0xa
	.4byte	0x37
	.4byte	0xa7f
	.uleb128 0xb
	.byte	0
	.uleb128 0x4
	.4byte	0xa74
	.uleb128 0x5
	.4byte	.LASF164
	.byte	0x11
	.byte	0x9f
	.4byte	0xa7f
	.uleb128 0x5
	.4byte	.LASF165
	.byte	0x11
	.byte	0xa0
	.4byte	0xa9a
	.uleb128 0x8
	.byte	0x4
	.4byte	0xa69
	.uleb128 0x23
	.4byte	.LASF170
	.byte	0x1
	.byte	0x12
	.4byte	0x55
	.4byte	.LFB177
	.4byte	.LFE177-.LFB177
	.uleb128 0x1
	.byte	0x9c
	.byte	0
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x15
	.byte	0x1
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0x15
	.byte	0x1
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1f
	.uleb128 0x4
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x20
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x21
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0x6
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x22
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x23
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.LFB177
	.4byte	.LFE177-.LFB177
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",%progbits
.Ldebug_ranges0:
	.4byte	.LFB177
	.4byte	.LFE177
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF16:
	.ascii	"ulong\000"
.LASF62:
	.ascii	"bi_boot_params\000"
.LASF133:
	.ascii	"net_gateway\000"
.LASF159:
	.ascii	"NETLOOP_FAIL\000"
.LASF46:
	.ascii	"bi_memsize\000"
.LASF80:
	.ascii	"ih_load\000"
.LASF135:
	.ascii	"net_dns_server\000"
.LASF157:
	.ascii	"NETLOOP_RESTART\000"
.LASF94:
	.ascii	"type\000"
.LASF93:
	.ascii	"comp\000"
.LASF28:
	.ascii	"select_hwpart\000"
.LASF66:
	.ascii	"FIQ_STACK_START\000"
.LASF148:
	.ascii	"net_null_ethaddr\000"
.LASF166:
	.ascii	"GNU C11 7.2.1 20171011 -mthumb -mthumb-interwork -m"
	.ascii	"abi=aapcs-linux -mword-relocations -mno-unaligned-a"
	.ascii	"ccess -mfloat-abi=soft -march=armv7-a -mtune=cortex"
	.ascii	"-a9 -mtls-dialect=gnu -g -O2 -std=gnu11 -fno-builti"
	.ascii	"n -ffreestanding -fshort-wchar -fno-stack-protector"
	.ascii	" -fno-delete-null-pointer-checks -fstack-usage -fno"
	.ascii	"-pic -ffunction-sections -fdata-sections -fno-commo"
	.ascii	"n -ffixed-r9\000"
.LASF10:
	.ascii	"long long unsigned int\000"
.LASF122:
	.ascii	"enetaddr\000"
.LASF25:
	.ascii	"_binary_u_boot_bin_end\000"
.LASF129:
	.ascii	"next\000"
.LASF38:
	.ascii	"lmb_property\000"
.LASF137:
	.ascii	"net_hostname\000"
.LASF9:
	.ascii	"long long int\000"
.LASF3:
	.ascii	"signed char\000"
.LASF76:
	.ascii	"ih_magic\000"
.LASF61:
	.ascii	"bi_arch_number\000"
.LASF98:
	.ascii	"legacy_hdr_os\000"
.LASF106:
	.ascii	"initrd_end\000"
.LASF97:
	.ascii	"bootm_headers\000"
.LASF112:
	.ascii	"images\000"
.LASF110:
	.ascii	"state\000"
.LASF14:
	.ascii	"long int\000"
.LASF19:
	.ascii	"__be32\000"
.LASF163:
	.ascii	"env_t\000"
.LASF70:
	.ascii	"_datarelro_start_ofs\000"
.LASF26:
	.ascii	"ide_bus_offset\000"
.LASF96:
	.ascii	"image_info_t\000"
.LASF115:
	.ascii	"__dtb_dt_spl_begin\000"
.LASF149:
	.ascii	"net_our_vlan\000"
.LASF23:
	.ascii	"image_base\000"
.LASF31:
	.ascii	"sector_count\000"
.LASF158:
	.ascii	"NETLOOP_SUCCESS\000"
.LASF146:
	.ascii	"net_rx_packet_len\000"
.LASF109:
	.ascii	"verify\000"
.LASF24:
	.ascii	"_binary_u_boot_bin_start\000"
.LASF155:
	.ascii	"net_boot_file_expected_size_in_blocks\000"
.LASF134:
	.ascii	"net_netmask\000"
.LASF103:
	.ascii	"ft_addr\000"
.LASF116:
	.ascii	"load_addr\000"
.LASF42:
	.ascii	"memory\000"
.LASF150:
	.ascii	"net_native_vlan\000"
.LASF168:
	.ascii	"/home/qylong/disk1/TS503/allwinner_a733_android_15/"
	.ascii	"vendor_android_14/longan/brandy/brandy-2.0/u-boot-2"
	.ascii	"018\000"
.LASF71:
	.ascii	"IRQ_STACK_START_IN\000"
.LASF8:
	.ascii	"unsigned int\000"
.LASF85:
	.ascii	"ih_type\000"
.LASF81:
	.ascii	"ih_ep\000"
.LASF64:
	.ascii	"bd_t\000"
.LASF136:
	.ascii	"net_nis_domain\000"
.LASF5:
	.ascii	"__u8\000"
.LASF1:
	.ascii	"long unsigned int\000"
.LASF7:
	.ascii	"__u32\000"
.LASF126:
	.ascii	"recv\000"
.LASF60:
	.ascii	"bi_busfreq\000"
.LASF27:
	.ascii	"name\000"
.LASF82:
	.ascii	"ih_dcrc\000"
.LASF162:
	.ascii	"data\000"
.LASF55:
	.ascii	"bi_bootflags\000"
.LASF30:
	.ascii	"size\000"
.LASF2:
	.ascii	"short unsigned int\000"
.LASF153:
	.ascii	"net_boot_file_name_explicit\000"
.LASF65:
	.ascii	"IRQ_STACK_START\000"
.LASF120:
	.ascii	"s_addr\000"
.LASF105:
	.ascii	"initrd_start\000"
.LASF95:
	.ascii	"arch\000"
.LASF99:
	.ascii	"legacy_hdr_os_copy\000"
.LASF36:
	.ascii	"flash_info\000"
.LASF56:
	.ascii	"bi_ip_addr\000"
.LASF89:
	.ascii	"image_info\000"
.LASF57:
	.ascii	"bi_enetaddr\000"
.LASF169:
	.ascii	"net_loop_state\000"
.LASF20:
	.ascii	"errno\000"
.LASF92:
	.ascii	"load\000"
.LASF39:
	.ascii	"base\000"
.LASF147:
	.ascii	"net_bcast_ethaddr\000"
.LASF132:
	.ascii	"eth_current\000"
.LASF118:
	.ascii	"save_size\000"
.LASF138:
	.ascii	"net_root_path\000"
.LASF83:
	.ascii	"ih_os\000"
.LASF117:
	.ascii	"save_addr\000"
.LASF52:
	.ascii	"bi_arm_freq\000"
.LASF125:
	.ascii	"send\000"
.LASF161:
	.ascii	"environment_s\000"
.LASF44:
	.ascii	"bd_info\000"
.LASF15:
	.ascii	"ushort\000"
.LASF165:
	.ascii	"env_ptr\000"
.LASF88:
	.ascii	"image_header_t\000"
.LASF111:
	.ascii	"bootm_headers_t\000"
.LASF43:
	.ascii	"reserved\000"
.LASF72:
	.ascii	"image_load_addr\000"
.LASF124:
	.ascii	"init\000"
.LASF131:
	.ascii	"priv\000"
.LASF21:
	.ascii	"___strtok\000"
.LASF47:
	.ascii	"bi_flashstart\000"
.LASF154:
	.ascii	"net_boot_file_size\000"
.LASF90:
	.ascii	"image_start\000"
.LASF40:
	.ascii	"lmb_region\000"
.LASF160:
	.ascii	"net_state\000"
.LASF86:
	.ascii	"ih_comp\000"
.LASF104:
	.ascii	"ft_len\000"
.LASF22:
	.ascii	"_Bool\000"
.LASF123:
	.ascii	"iobase\000"
.LASF0:
	.ascii	"unsigned char\000"
.LASF79:
	.ascii	"ih_size\000"
.LASF142:
	.ascii	"net_server_ip\000"
.LASF101:
	.ascii	"rd_start\000"
.LASF141:
	.ascii	"net_ip\000"
.LASF74:
	.ascii	"image_save_size\000"
.LASF6:
	.ascii	"short int\000"
.LASF51:
	.ascii	"bi_sramsize\000"
.LASF73:
	.ascii	"image_save_addr\000"
.LASF102:
	.ascii	"rd_end\000"
.LASF100:
	.ascii	"legacy_hdr_valid\000"
.LASF128:
	.ascii	"write_hwaddr\000"
.LASF145:
	.ascii	"net_rx_packet\000"
.LASF114:
	.ascii	"__dtb_dt_begin\000"
.LASF53:
	.ascii	"bi_dsp_freq\000"
.LASF113:
	.ascii	"monitor_flash_len\000"
.LASF140:
	.ascii	"net_server_ethaddr\000"
.LASF18:
	.ascii	"uint32_t\000"
.LASF68:
	.ascii	"_datarelrolocal_start_ofs\000"
.LASF67:
	.ascii	"_datarel_start_ofs\000"
.LASF37:
	.ascii	"long double\000"
.LASF13:
	.ascii	"char\000"
.LASF48:
	.ascii	"bi_flashsize\000"
.LASF75:
	.ascii	"image_header\000"
.LASF130:
	.ascii	"index\000"
.LASF164:
	.ascii	"default_environment\000"
.LASF11:
	.ascii	"phys_addr_t\000"
.LASF50:
	.ascii	"bi_sramstart\000"
.LASF91:
	.ascii	"image_len\000"
.LASF32:
	.ascii	"flash_id\000"
.LASF78:
	.ascii	"ih_time\000"
.LASF139:
	.ascii	"net_ethaddr\000"
.LASF58:
	.ascii	"bi_ethspeed\000"
.LASF29:
	.ascii	"block_drvr\000"
.LASF41:
	.ascii	"region\000"
.LASF151:
	.ascii	"net_restart_wrap\000"
.LASF156:
	.ascii	"NETLOOP_CONTINUE\000"
.LASF59:
	.ascii	"bi_intfreq\000"
.LASF108:
	.ascii	"cmdline_end\000"
.LASF12:
	.ascii	"phys_size_t\000"
.LASF35:
	.ascii	"flash_info_t\000"
.LASF167:
	.ascii	"lib/asm-offsets.c\000"
.LASF152:
	.ascii	"net_boot_file_name\000"
.LASF77:
	.ascii	"ih_hcrc\000"
.LASF4:
	.ascii	"uchar\000"
.LASF17:
	.ascii	"uint8_t\000"
.LASF34:
	.ascii	"protect\000"
.LASF63:
	.ascii	"bi_dram\000"
.LASF144:
	.ascii	"net_rx_packets\000"
.LASF127:
	.ascii	"halt\000"
.LASF87:
	.ascii	"ih_name\000"
.LASF54:
	.ascii	"bi_ddr_freq\000"
.LASF121:
	.ascii	"eth_device\000"
.LASF45:
	.ascii	"bi_memstart\000"
.LASF33:
	.ascii	"start\000"
.LASF170:
	.ascii	"main\000"
.LASF84:
	.ascii	"ih_arch\000"
.LASF49:
	.ascii	"bi_flashoffset\000"
.LASF69:
	.ascii	"_datarellocal_start_ofs\000"
.LASF143:
	.ascii	"net_tx_packet\000"
.LASF107:
	.ascii	"cmdline_start\000"
.LASF119:
	.ascii	"in_addr\000"
	.ident	"GCC: (Linaro GCC 7.2-2017.11) 7.2.1 20171011"
	.section	.note.GNU-stack,"",%progbits
