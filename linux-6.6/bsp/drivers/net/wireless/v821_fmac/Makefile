# SPDX-License-Identifier: GPL-2.0
CONFIG_MODULE_NAME:=v821_fmac

CONFIG_PLATFORM_INTERFACE_RPBUF:=y
CONFIG_CHECKSUM:=n
CONFIG_XRADIO_RPBUF_PERF_TEST:=n
CONFIG_STA_AP_COEXIST:=y

v821_fmac-y := \
	low_cmd.o \
	xradio_main.o \
	xradio_platform.o \
	queue.o \
	xradio_txrx.o \
	xradio_firmware.o \
	debug.o \
	up_cmd.o \
	os_dep/os_intf.o

ifeq ($(CONFIG_CHECKSUM), y)
v821_fmac-$(CONFIG_CHECKSUM) += checksum.o
EXTRA_CFLAGS += -DXRADIO_CHECKSUM
endif

EXTRA_CFLAGS += -Wno-error=date-time -Wno-date-time
EXTRA_CFLAGS += -I$(src)/include -I$(src)/os_dep -I$(src)/platform -I$(src)/test

ifeq ($(CONFIG_PLATFORM_INTERFACE_RPBUF), y)
v821_fmac-$(CONFIG_PLATFORM_INTERFACE_RPBUF) += platform/xradio_rpbuf.o
v821_fmac-$(CONFIG_PLATFORM_INTERFACE_RPBUF) += platform/xradio_rpmsg.o
EXTRA_CFLAGS += -DXRADIO_RPBUF_SUPPORT
endif
ifeq ($(CONFIG_XRADIO_RPBUF_PERF_TEST), y)
EXTRA_CFLAGS += -DXRADIO_RPBUF_PERF_TEST
endif

ifeq ($(CONFIG_STA_AP_COEXIST), y)
EXTRA_CFLAGS += -DSTA_AP_COEXIST
endif

obj-m += $(CONFIG_MODULE_NAME).o
