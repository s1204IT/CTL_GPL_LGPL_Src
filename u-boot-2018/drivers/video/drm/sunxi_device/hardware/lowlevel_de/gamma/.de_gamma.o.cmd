cmd_drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o := ./../tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -Wp,-MD,drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/.de_gamma.o.d  -nostdinc -isystem /home/qylong/disk1/TS503/allwinner_a733_android_15/vendor_android_14/longan/brandy/brandy-2.0/tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/7.2.1/include -Iinclude    -I./arch/arm/include -include ./include/linux/kconfig.h -I./include/openssl -D__KERNEL__ -D__UBOOT__ -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -std=gnu11 -Werror -fshort-wchar -O2 -fno-stack-protector -fno-delete-null-pointer-checks -g -fstack-usage -Wno-format-nonliteral -Werror=date-time -I./drivers/video/drm/sunxi_device/hardware/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_lcd/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_tcon/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_edp/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_hdmi20/ -I./drivers/video/drm/ -I./drivers/video/drm// -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/../../../ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/csc/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/cdc/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/snr/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/sharp/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/dci/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/fcm/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/bld/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/ovl/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/scaler/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/afbd/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/wb/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/dither/ -I./drivers/video/drm/sunxi_device/hardware/lowlevel_de/crc/ -D__ARM__ -Wa,-mimplicit-it=always -mthumb -mthumb-interwork -mabi=aapcs-linux -mword-relocations -fno-pic -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe -march=armv7-a -D__LINUX_ARM_ARCH__=7 -I./arch/arm/mach-sunxi/include    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(de_gamma)"  -D"KBUILD_MODNAME=KBUILD_STR(de_gamma)" -c -o drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.c

source_drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o := drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.c

deps_drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o := \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma_type.h \
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
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.h \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/de_base.h \
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
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/csc/de_csc.h \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/de_channel.h \
  include/drm/drm_plane.h \
  include/linux/compat.h \
    $(wildcard include/config/lbdaf.h) \
  include/malloc.h \
    $(wildcard include/config/sys/malloc/simple.h) \
  include/linux/err.h \
  include/linux/errno.h \
  include/drm/drm_util.h \
  include/drm/drm_modes.h \
  include/fdtdec.h \
    $(wildcard include/config/of/board.h) \
    $(wildcard include/config/of/separate.h) \
  include/linux/libfdt.h \
  include/linux/libfdt_env.h \
  include/linux/string.h \
  arch/arm/include/asm/string.h \
    $(wildcard include/config/use/arch/memcpy.h) \
    $(wildcard include/config/use/arch/memset.h) \
  include/config.h \
    $(wildcard include/config/boarddir.h) \
  include/config_defaults.h \
    $(wildcard include/config/defaults/h/.h) \
    $(wildcard include/config/bootm/linux.h) \
    $(wildcard include/config/bootm/netbsd.h) \
    $(wildcard include/config/bootm/plan9.h) \
    $(wildcard include/config/bootm/rtems.h) \
    $(wildcard include/config/bootm/vxworks.h) \
    $(wildcard include/config/gzip.h) \
    $(wildcard include/config/zlib.h) \
  include/config_uncmd_spl.h \
    $(wildcard include/config/uncmd/spl/h//.h) \
    $(wildcard include/config/spl/dm.h) \
    $(wildcard include/config/dm/serial.h) \
    $(wildcard include/config/dm/gpio.h) \
    $(wildcard include/config/dm/i2c.h) \
    $(wildcard include/config/dm/spi.h) \
    $(wildcard include/config/dm/warn.h) \
    $(wildcard include/config/dm/stdio.h) \
  include/configs/sun60iw2p1.h \
    $(wildcard include/config/h.h) \
    $(wildcard include/config/usb/ehci/hcd.h) \
    $(wildcard include/config/usb/ehci/sunxi.h) \
    $(wildcard include/config/usb/max/controller/count.h) \
    $(wildcard include/config/sunxi/usb/phys.h) \
    $(wildcard include/config/sys/bootm/len.h) \
  include/configs/sunxi-common.h \
    $(wildcard include/config/trace/early/addr.h) \
    $(wildcard include/config/trace.h) \
    $(wildcard include/config/cmd/trace.h) \
    $(wildcard include/config/trace/buffer/size.h) \
    $(wildcard include/config/trace/early/size.h) \
    $(wildcard include/config/trace/early.h) \
    $(wildcard include/config/sunxi/debug.h) \
    $(wildcard include/config/old/sunxi/kernel/compat.h) \
    $(wildcard include/config/mach/type/compat/rev.h) \
    $(wildcard include/config/build/target.h) \
    $(wildcard include/config/sys/ns16550/serial.h) \
    $(wildcard include/config/sys/ns16550/clk.h) \
    $(wildcard include/config/aw/sys/ns16550/clk.h) \
    $(wildcard include/config/sys/ns16550/reg/size.h) \
    $(wildcard include/config/sys/ns16550/com1.h) \
    $(wildcard include/config/sys/ns16550/com2.h) \
    $(wildcard include/config/sys/ns16550/com3.h) \
    $(wildcard include/config/sys/ns16550/com4.h) \
    $(wildcard include/config/sys/ns16550/com5.h) \
    $(wildcard include/config/sys/ns16550/com6.h) \
    $(wildcard include/config/foo.h) \
    $(wildcard include/config/mach/sun9i.h) \
    $(wildcard include/config/sys/sdram/base.h) \
    $(wildcard include/config/sys/load/addr.h) \
    $(wildcard include/config/spl/stack/r/addr.h) \
    $(wildcard include/config/spl/bss/start/addr.h) \
    $(wildcard include/config/mach/sun300iw1.h) \
    $(wildcard include/config/dram/para/addr.h) \
    $(wildcard include/config/spl/bss/max/size.h) \
    $(wildcard include/config/sys/init/ram/addr.h) \
    $(wildcard include/config/sys/init/ram/size.h) \
    $(wildcard include/config/sunxi/logbuffer.h) \
    $(wildcard include/config/sys/init/sp/offset.h) \
    $(wildcard include/config/sys/init/sp/addr.h) \
    $(wildcard include/config/nr/dram/banks.h) \
    $(wildcard include/config/ahci.h) \
    $(wildcard include/config/scsi/ahci/plat.h) \
    $(wildcard include/config/sunxi/ahci.h) \
    $(wildcard include/config/sys/64bit/lba.h) \
    $(wildcard include/config/sys/scsi/max/scsi/id.h) \
    $(wildcard include/config/sys/scsi/max/lun.h) \
    $(wildcard include/config/sys/scsi/max/device.h) \
    $(wildcard include/config/setup/memory/tags.h) \
    $(wildcard include/config/cmdline/tag.h) \
    $(wildcard include/config/initrd/tag.h) \
    $(wildcard include/config/serial/tag.h) \
    $(wildcard include/config/nand/sunxi.h) \
    $(wildcard include/config/sys/nand/max/eccpos.h) \
    $(wildcard include/config/sys/nand/onfi/detection.h) \
    $(wildcard include/config/sys/max/nand/device.h) \
    $(wildcard include/config/mtd/device.h) \
    $(wildcard include/config/mtd/partitions.h) \
    $(wildcard include/config/spl/spi/sunxi.h) \
    $(wildcard include/config/sys/spi/u/boot/offs.h) \
    $(wildcard include/config/mmc.h) \
    $(wildcard include/config/mmc/sunxi/slot.h) \
    $(wildcard include/config/sys/malloc/len.h) \
    $(wildcard include/config/env/size.h) \
    $(wildcard include/config/mach/sun8iw18.h) \
    $(wildcard include/config/mach/sun8iw19.h) \
    $(wildcard include/config/sys/cbsize.h) \
    $(wildcard include/config/sys/pbsize.h) \
    $(wildcard include/config/standalone/load/addr.h) \
    $(wildcard include/config/sys/monitor/len.h) \
    $(wildcard include/config/spl/board/load/image.h) \
    $(wildcard include/config/sunxi/high/sram.h) \
    $(wildcard include/config/spl/text/base.h) \
    $(wildcard include/config/spl/max/size.h) \
    $(wildcard include/config/spl/stack.h) \
    $(wildcard include/config/spl/pad/to.h) \
    $(wildcard include/config/i2c0/enable.h) \
    $(wildcard include/config/i2c1/enable.h) \
    $(wildcard include/config/i2c2/enable.h) \
    $(wildcard include/config/i2c3/enable.h) \
    $(wildcard include/config/i2c4/enable.h) \
    $(wildcard include/config/i2c5/enable.h) \
    $(wildcard include/config/r/i2c0/enable.h) \
    $(wildcard include/config/r/i2c1/enable.h) \
    $(wildcard include/config/sys/i2c/mvtwsi.h) \
    $(wildcard include/config/sys/i2c/sunxi.h) \
    $(wildcard include/config/sys/i2c.h) \
    $(wildcard include/config/sys/i2c/speed.h) \
    $(wildcard include/config/sys/i2c/slave.h) \
    $(wildcard include/config/sunxi/gpio.h) \
    $(wildcard include/config/video/sunxi.h) \
    $(wildcard include/config/sunxi/max/fb/size.h) \
    $(wildcard include/config/video/logo.h) \
    $(wildcard include/config/video/std/timings.h) \
    $(wildcard include/config/i2c/edid.h) \
    $(wildcard include/config/sun4i/emac.h) \
    $(wildcard include/config/mii.h) \
    $(wildcard include/config/sun7i/gmac.h) \
    $(wildcard include/config/phy/realtek.h) \
    $(wildcard include/config/usb/ohci/new.h) \
    $(wildcard include/config/usb/ohci/sunxi.h) \
    $(wildcard include/config/sys/usb/ohci/max/root/ports.h) \
    $(wildcard include/config/misc/init/r.h) \
    $(wildcard include/config/usb/storage.h) \
    $(wildcard include/config/default/device/tree.h) \
    $(wildcard include/config/extra/env/settings.h) \
    $(wildcard include/config/board/late/init.h) \
    $(wildcard include/config/board/early/init/r.h) \
    $(wildcard include/config/sunxi/fpga/platform.h) \
  arch/arm/include/asm/arch/cpu.h \
    $(wildcard include/config/sunxi/ncat.h) \
    $(wildcard include/config/sunxi/ncat/v2.h) \
    $(wildcard include/config/sunxi/version1.h) \
    $(wildcard include/config/mach/sun55iw3.h) \
    $(wildcard include/config/mach/sun55iw5.h) \
    $(wildcard include/config/mach/sun55iw6.h) \
    $(wildcard include/config/mach/sun60iw1.h) \
    $(wildcard include/config/mach/sun60iw2.h) \
    $(wildcard include/config/mach/sun50iw15.h) \
    $(wildcard include/config/mach/sun65iw1.h) \
  arch/arm/include/asm/arch/plat-sun60iw2p1/cpu_sun60iw2.h \
  arch/arm/include/asm/arch/plat-sun60iw2p1/cpu_autogen.h \
  include/linux/stringify.h \
  include/config_distro_bootcmd.h \
    $(wildcard include/config/cmd/distro/bootcmd/h.h) \
    $(wildcard include/config/cmd/mmc.h) \
    $(wildcard include/config/sandbox.h) \
    $(wildcard include/config/cmd/ubifs.h) \
    $(wildcard include/config/efi/loader.h) \
    $(wildcard include/config/arm.h) \
    $(wildcard include/config/x86/run/32bit.h) \
    $(wildcard include/config/x86/run/64bit.h) \
    $(wildcard include/config/arch/rv32i.h) \
    $(wildcard include/config/arch/rv64i.h) \
    $(wildcard include/config/sata.h) \
    $(wildcard include/config/scsi.h) \
    $(wildcard include/config/ide.h) \
    $(wildcard include/config/dm/pci.h) \
    $(wildcard include/config/cmd/usb.h) \
    $(wildcard include/config/cmd/dhcp.h) \
    $(wildcard include/config/x86.h) \
    $(wildcard include/config/cpu/riscv/32.h) \
    $(wildcard include/config/cpu/riscv/64.h) \
    $(wildcard include/config/cmd/pxe.h) \
    $(wildcard include/config/cmd/dhcp/or/pxe.h) \
    $(wildcard include/config/bootcommand.h) \
  arch/arm/include/asm/config.h \
    $(wildcard include/config/h/.h) \
    $(wildcard include/config/lmb.h) \
    $(wildcard include/config/sys/boot/ramdisk/high.h) \
    $(wildcard include/config/arch/ls1021a.h) \
    $(wildcard include/config/cpu/pxa27x.h) \
    $(wildcard include/config/cpu/monahans.h) \
    $(wildcard include/config/cpu/pxa25x.h) \
    $(wildcard include/config/fsl/layerscape.h) \
  include/config_fallbacks.h \
    $(wildcard include/config/fallbacks/h.h) \
    $(wildcard include/config/spl.h) \
    $(wildcard include/config/sys/baudrate/table.h) \
    $(wildcard include/config/cmd/kgdb.h) \
    $(wildcard include/config/sys/prompt.h) \
    $(wildcard include/config/sys/maxargs.h) \
  include/linux/linux_string.h \
  include/vsprintf.h \
    $(wildcard include/config/panic/hang.h) \
  include/linux/../../scripts/dtc/libfdt/libfdt.h \
  include/linux/../../scripts/dtc/libfdt/libfdt_env.h \
  include/linux/../../scripts/dtc/libfdt/fdt.h \
  include/pci.h \
    $(wildcard include/config/sys/pci/64bit.h) \
    $(wildcard include/config/pci/indirect/bridge.h) \
    $(wildcard include/config/dm/pci/compat.h) \
    $(wildcard include/config/pci/fixup/dev.h) \
    $(wildcard include/config/mpc85xx.h) \
    $(wildcard include/config/pcie/imx.h) \
  include/pci_ids.h \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/de_base.h \
  drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma_platform.h \

drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o: $(deps_drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o)

$(deps_drivers/video/drm/sunxi_device/hardware/lowlevel_de/gamma/de_gamma.o):
