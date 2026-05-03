# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2012 Henrik Nordstrom <henrik@henriknordstrom.net>
#
# Based on some other Makefile
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.

obj-y	+= board.o
obj-y	+= clock.o
obj-$(CONFIG_AW_PLIC) += plic.o
obj-$(CONFIG_AW_CLIC) += aw_clic.o
obj-y	+= cpu_info.o
obj-y	+= pinmux.o
obj-y	+= rtc.o
obj-n	+= usb_phy.o
obj-$(CONFIG_SUN6I_P2WI)	+= p2wi.o
obj-$(CONFIG_SUN6I_PRCM)	+= prcm.o
obj-$(CONFIG_AXP_PMIC_BUS)	+= pmic_bus.o
obj-$(CONFIG_SUN8I_RSB)		+= rsb.o
obj-$(CONFIG_MACH_SUN20IW1) += clock_sun20iw1.o
obj-$(CONFIG_MACH_SUN251IW1) += clock_sun251iw1.o board_sun251iw1.o
obj-$(CONFIG_MACH_SUN300IW1) += clock_sun300iw1.o board_sun300iw1.o
