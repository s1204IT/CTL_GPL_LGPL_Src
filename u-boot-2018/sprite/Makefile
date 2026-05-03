# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2018www.allwinner.tech
#
# Based on some other board Makefile
#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.

obj-$(CONFIG_SUNXI_SDMMC) += sprite_card.o sprite_main.o
obj-$(CONFIG_SUNXI_SPRITE)	+= sprite_download.o sprite_verify.o sprite_privatedata.o sprite_erase.o firmware/imgdecode.o sprite_common.o
obj-$(CONFIG_SUNXI_SPRITE_LED)	+= sprite_led.o
obj-$(CONFIG_SUNXI_SECURE_STORAGE) += sprite_secstorage.o sprite_secure_object.o
obj-$(CONFIG_SUNXI_SPRITE_CARTOON)	+= cartoon/
obj-$(CONFIG_SUNXI_SPRITE_RECOVERY) += sprite_recovery.o
obj-$(CONFIG_SUNXI_BOOT_PARAM) += sunxi_boot_param.o
obj-$(CONFIG_SUNXI_MELIS_AUTO_UPDATE) += sprite_melis_auto_update.o
obj-$(CONFIG_SUNXI_AUTO_UPDATE) += sprite_auto_update.o
obj-$(CONFIG_SUNXI_PART_UPDATE) += sprite_part_update.o
obj-y += sparse/sparse.o
