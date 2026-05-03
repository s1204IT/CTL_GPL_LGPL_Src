# SPDX-License-Identifier: GPL-2.0-or-later

obj-$(CONFIG_AW_HRC) += hrc.o

include $(srctree)/$(src)/hardware/Makefile

ccflags-y += -DDYNAMIC_DEBUG_MODULE -I. -Wall -Werror

hrc-y := sunxi_hrc.o
hrc-y += $(hrc_hardware)
