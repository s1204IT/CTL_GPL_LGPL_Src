# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the remoteproc drivers.
#
ccflags-y += -I $(srctree)/drivers/remoteproc
obj-$(CONFIG_AW_REMOTEPROC_E906_BOOT) += sunxi_rproc_e906_boot.o
obj-$(CONFIG_AW_REMOTEPROC_E907_BOOT) += sunxi_remoteproc_e907_boot.o
sunxi_remoteproc_e907_boot-y += sunxi_rproc_e907_boot.o
sunxi_remoteproc_e907_boot-$(CONFIG_ARCH_SUN55IW6) += rproc_e907-sun55iw6.o
sunxi_remoteproc_e907_boot-$(CONFIG_ARCH_SUN8IW21) += rproc_e907-sun8iw21.o
sunxi_remoteproc_e907_boot-$(CONFIG_ARCH_SUN300IW1) += rproc_e907-sun300iw1.o
obj-$(CONFIG_AW_REMOTEPROC_HIFI4_BOOT) += sunxi_rproc_hifi4_boot.o
obj-$(CONFIG_AW_REMOTEPROC_ARM64_BOOT) += sunxi_rproc_arm64_boot.o
obj-$(CONFIG_AW_REMOTEPROC)		+= sunxi_remoteproc.o
sunxi_remoteproc-y			+= sunxi_rproc.o
sunxi_remoteproc-y			+= sunxi_rproc_boot.o
sunxi_remoteproc-y			+= sunxi_rproc_standby.o
sunxi_remoteproc-y			+= sunxi_rproc_common_boot.o
sunxi_remoteproc-y			+= sunxi_rproc_firmware.o
obj-$(CONFIG_AW_RPROC_ENHANCED_TRACE) += sunxi_rproc_trace.o
obj-$(CONFIG_SUNXI_RPROC_SHARE_IRQ)	+= sunxi_share_interrupt/sunxi_share_interrupt.o
obj-$(CONFIG_AW_RPROC_SUBDEV_WDT)	+= subdev/sunxi_rproc_wdt.o
obj-$(CONFIG_AW_RPROC_SUBDEV_WDT)	+= subdev/sunxi_rproc_wdt_dev.o
obj-$(CONFIG_AW_RPROC_SUBDEV_RSC_HELPER) += subdev/sunxi_rproc_rsc_helper.o
obj-$(CONFIG_AW_RPROC_SUBDEV_STANDBY) += subdev/sunxi_rproc_standby_mgr.o
obj-$(CONFIG_AW_RPROC_SUBDEV_ECC) += subdev/sunxi_rproc_ecc_dev.o
obj-$(CONFIG_AW_RPROC_USER_RESOURCE)	+= subdev/sunxi_rproc_user_resource.o
obj-$(CONFIG_AW_RPROC_SUBDEV)	+= sunxi_rproc_subdev.o
