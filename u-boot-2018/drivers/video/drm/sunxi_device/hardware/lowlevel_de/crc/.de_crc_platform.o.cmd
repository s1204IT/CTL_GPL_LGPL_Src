cmd_drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o := ./../tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -Wp,-MD,drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/.de_crc_platform.o.d  -nostdinc -isystem /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/7.2.1/include -Iinclude    -I./arch/arm/include -include ./include/linux/kconfig.h -I./include/openssl -D__KERNEL__ -D__UBOOT__ -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -std=gnu11 -Werror -fshort-wchar -O2 -fno-stack-protector -fno-delete-null-pointer-checks -g -fstack-usage -Wno-format-nonliteral -Werror=date-time -I./drivers/video/drm/sunxi_device/hardware/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_lcd/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_tcon/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_edp/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_hdmi20/ -I./drivers/video/drm/ -I./drivers/video/drm// -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/../../../ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/csc/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/cdc/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/snr/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/sharp/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/dci/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/fcm/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/bld/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/ovl/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/scaler/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/afbd/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/wb/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/dither/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/ -D__ARM__ -Wa,-mimplicit-it=always -mthumb -mthumb-interwork -mabi=aapcs-linux -mword-relocations -fno-pic -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe -march=armv7-a -D__LINUX_ARM_ARCH__=7 -I./arch/arm/mach-sunxi/include    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(de_crc_platform)"  -D"KBUILD_MODNAME=KBUILD_STR(de_crc_platform)" -c -o drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.c

source_drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o := drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.c

deps_drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o := \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.h \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/de_base.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/use/stdint.h) \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  arch/arm/include/asm/posix_types.h \
  arch/arm/include/asm/types.h \
    $(wildcard include/config/arm64.h) \
    $(wildcard include/config/phys/64bit.h) \
    $(wildcard include/config/dma/addr/t/64bit.h) \
  /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/lib/gcc/arm-linux-gnueabi/7.2.1/include/stdbool.h \
  include/linux/kernel.h \
  include/linux/io.h \
    $(wildcard include/config/64bit.h) \
    $(wildcard include/config/have/arch/ioremap.h) \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/kasan.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
    $(wildcard include/config/gcov/kernel.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  arch/arm/include/asm/io.h \
  arch/arm/include/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/linux/byteorder/swab.h \
  include/linux/byteorder/generic.h \
  arch/arm/include/asm/memory.h \
    $(wildcard include/config/discontigmem.h) \
  arch/arm/include/asm/barriers.h \
  include/asm-generic/io.h \
  include/iotrace.h \
    $(wildcard include/config/io/trace.h) \
    $(wildcard include/config/spl/build.h) \
  include/drm/drm_print.h \
  include/linux/printk.h \
  include/stdio.h \
    $(wildcard include/config/tpl/build.h) \
    $(wildcard include/config/tpl/serial/support.h) \
    $(wildcard include/config/spl/serial/support.h) \
  /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/lib/gcc/arm-linux-gnueabi/7.2.1/include/stdarg.h \
  include/drm/drm_framebuffer.h \
  include/drm/drm_fourcc.h \
  include/linux/list.h \
  include/linux/poison.h \
  include/drm/drm_rect.h \

drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o: $(deps_drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o)

$(deps_drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/de_crc_platform.o):
