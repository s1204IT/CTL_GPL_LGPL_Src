cmd_arch/arm/cpu/armv7/smccc-call.o := ./../tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -Wp,-MD,arch/arm/cpu/armv7/.smccc-call.o.d  -nostdinc -isystem /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/7.2.1/include -Iinclude    -I./arch/arm/include -include ./include/linux/kconfig.h -I./include/openssl -D__KERNEL__ -D__UBOOT__ -D__ASSEMBLY__ -g -D__ARM__ -Wa,-mimplicit-it=always -mthumb -mthumb-interwork -mabi=aapcs-linux -mword-relocations -fno-pic -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe -march=armv7-a -D__LINUX_ARM_ARCH__=7 -I./arch/arm/mach-sunxi/include   -c -o arch/arm/cpu/armv7/smccc-call.o arch/arm/cpu/armv7/smccc-call.S

source_arch/arm/cpu/armv7/smccc-call.o := arch/arm/cpu/armv7/smccc-call.S

deps_arch/arm/cpu/armv7/smccc-call.o := \
  include/linux/linkage.h \
  arch/arm/include/asm/linkage.h \
  arch/arm/include/asm/opcodes-sec.h \
  arch/arm/include/asm/opcodes.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/cpu/endian/be32.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/stringify.h \
  arch/arm/include/asm/opcodes-virt.h \

arch/arm/cpu/armv7/smccc-call.o: $(deps_arch/arm/cpu/armv7/smccc-call.o)

$(deps_arch/arm/cpu/armv7/smccc-call.o):
