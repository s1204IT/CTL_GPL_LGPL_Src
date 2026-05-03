cmd_arch/arm/dts/sun60iw2p1-soc-system.dtb := mkdir -p arch/arm/dts/ ; (cat arch/arm/dts/sun60iw2p1-soc-system.dts; ) > arch/arm/dts/.sun60iw2p1-soc-system.dtb.pre.tmp; ./../tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -E -Wp,-MD,arch/arm/dts/.sun60iw2p1-soc-system.dtb.d.pre.tmp -nostdinc -I./arch/arm/dts -I./arch/arm/dts/include -Iinclude -I./include -I./arch/arm/include -include ./include/linux/kconfig.h -D__ASSEMBLY__ -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/dts/.sun60iw2p1-soc-system.dtb.dts.tmp arch/arm/dts/.sun60iw2p1-soc-system.dtb.pre.tmp ; ./scripts/dtc/dtc -O dtb -o arch/arm/dts/sun60iw2p1-soc-system.dtb -b 0 -i arch/arm/dts/  -Wno-unit_address_vs_reg -Wno-simple_bus_reg -Wno-unit_address_format -Wno-pci_bridge -Wno-pci_device_bus_num -Wno-pci_device_reg  -d arch/arm/dts/.sun60iw2p1-soc-system.dtb.d.dtc.tmp arch/arm/dts/.sun60iw2p1-soc-system.dtb.dts.tmp ; cat arch/arm/dts/.sun60iw2p1-soc-system.dtb.d.pre.tmp arch/arm/dts/.sun60iw2p1-soc-system.dtb.d.dtc.tmp > arch/arm/dts/.sun60iw2p1-soc-system.dtb.d

source_arch/arm/dts/sun60iw2p1-soc-system.dtb := arch/arm/dts/.sun60iw2p1-soc-system.dtb.pre.tmp

deps_arch/arm/dts/sun60iw2p1-soc-system.dtb := \
  arch/arm/dts/include/dt-bindings/interrupt-controller/arm-gic.h \
  arch/arm/dts/include/dt-bindings/interrupt-controller/irq.h \
  arch/arm/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/dts/sun60iw2p1-clk.dtsi \
  arch/arm/dts/include/dt-bindings/thermal/thermal.h \
  arch/arm/dts/include/dt-bindings/phy/phy.h \
  include/media_bus_format.h \
  include/drm/drm_mipi_dsi.h \
  arch/arm/dts/.board-uboot.dts \

arch/arm/dts/sun60iw2p1-soc-system.dtb: $(deps_arch/arm/dts/sun60iw2p1-soc-system.dtb)

$(deps_arch/arm/dts/sun60iw2p1-soc-system.dtb):
