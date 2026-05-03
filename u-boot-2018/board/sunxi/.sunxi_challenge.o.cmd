cmd_board/sunxi/sunxi_challenge.o := ./../tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -Wp,-MD,board/sunxi/.sunxi_challenge.o.d  -nostdinc -isystem /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/7.2.1/include -Iinclude    -I./arch/arm/include -include ./include/linux/kconfig.h -I./include/openssl -D__KERNEL__ -D__UBOOT__ -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -std=gnu11 -Werror -fshort-wchar -O2 -fno-stack-protector -fno-delete-null-pointer-checks -g -fstack-usage -Wno-format-nonliteral -Werror=date-time -D__ARM__ -Wa,-mimplicit-it=always -mthumb -mthumb-interwork -mabi=aapcs-linux -mword-relocations -fno-pic -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe -march=armv7-a -D__LINUX_ARM_ARCH__=7 -I./arch/arm/mach-sunxi/include    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(sunxi_challenge)"  -D"KBUILD_MODNAME=KBUILD_STR(sunxi_challenge)" -c -o board/sunxi/sunxi_challenge.o board/sunxi/sunxi_challenge.c

source_board/sunxi/sunxi_challenge.o := board/sunxi/sunxi_challenge.c

deps_board/sunxi/sunxi_challenge.o := \

board/sunxi/sunxi_challenge.o: $(deps_board/sunxi/sunxi_challenge.o)

$(deps_board/sunxi/sunxi_challenge.o):
