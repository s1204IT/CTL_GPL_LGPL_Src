# SPDX-License-Identifier: GPL-2.0
#
# Makefile for sunxi specific clk
#

ccu-y += clk-sunxi.o clk-sdm.o clk-factors.o clk-periph.o clk-cpu.o ccu_reset.o

obj-$(CONFIG_AW_CCU_LEGACY)	+= ccu.o

# Soc support
obj-${CONFIG_AW_SUN8IW17_CCU_LEGACY} += clk-sun8iw17.o
obj-$(CONFIG_AW_SUN8IW21_CCU_LEGACY) += clk-sun8iw21.o
