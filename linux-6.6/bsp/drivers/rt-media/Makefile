# SPDX-License-Identifier: GPL-2.0

codec_dir = $(LICHEE_BSP_DIR)/drivers/ve/cedar-ve/codec/libcedarc

EXTRA_CFLAGS += -I$(LICHEE_BSP_DIR)/drivers/ve/cedar-ve \
				-I$(codec_dir)/include \
				-I$(codec_dir)/vencoder \
				-I$(codec_dir)/base/include \
				-I$(codec_dir)/memory/kernelMemory \
				-I$(LICHEE_BSP_DIR)/drivers/ve/cedar-ve/codec/lib/include \
				-I$(LICHEE_BSP_DIR)/drivers/vin/vin-video \
				-I$(LICHEE_BSP_DIR)/include/uapi \
				-I$(LICHEE_BSP_DIR)/include/uapi/media \
				-Wno-undef -Wno-declaration-after-statement -Wno-unused-variable -Wno-unused-function -Wno-unused-label

obj-$(CONFIG_VIDEO_RT_MEDIA) += rt-media.o

rt-media-objs := rt_media.o
rt-media-objs += component/rt_message.o
rt-media-objs += component/rt_semaphore.o
rt-media-objs += component/rt_component.o
rt-media-objs += component/rt_venc_component.o
rt-media-objs += component/rt_vi_component.o
rt-media-objs += ../ve/cedar-ve/codec/lib/libmem
rt-media-objs += ../ve/cedar-ve/codec/lib/libvencoder
rt-media-objs += ../ve/cedar-ve/codec/lib/libbase
rt-media-objs += ../ve/cedar-ve/codec/lib/libve
rt-media-objs += ../ve/cedar-ve/codec/lib/libgcc
rt-media-objs += ../ve/cedar-ve/codec/lib/libmath
ifeq (${CONFIG_VIDEO_RT_MEDIA},m)
rt-media-objs += ./libmath
endif


