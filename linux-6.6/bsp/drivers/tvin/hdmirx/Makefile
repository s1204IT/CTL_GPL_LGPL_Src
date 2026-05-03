# SPDX-License-Identifier: GPL-2.0-or-later
EXTRA_CFLAGS += -g
obj-$(CONFIG_AW_VIDEO_SUNXI_HDMIRX) += hdmirx.o
ccflags-y := -DDYNAMIC_DEBUG_MODULE

hdmirx-y := aw_hdmirx_drv.o
hdmirx-y += aw_hdmirx_dev.o
hdmirx-y += aw_hdmirx_core/aw_hdmirx_core_drv.o
hdmirx-y += aw_hdmirx_core/THDMIRx_DisplayModuleCtx.o
hdmirx-y += aw_hdmirx_core/THDMIRx_DataPath.o
hdmirx-y += aw_hdmirx_core/THDMIRx_Port.o
hdmirx-y += aw_hdmirx_core/THDMIRx_PortCtx.o
hdmirx-y += aw_hdmirx_core/THDMIRx_Event.o
hdmirx-y += aw_hdmirx_core/THDMIRx_Packet.o
hdmirx-y += aw_hdmirx_core/THDMIRx_Edid.o
hdmirx-y += aw_hdmirx_core/HDCP1.4/THDMIRx_HDCP14.o
hdmirx-y += aw_hdmirx_core/hardware/THDMIRx_TV_Driver.o
hdmirx-y += aw_hdmirx_core/hardware/THDMIRx_TV_Video_Driver.o
hdmirx-y += aw_hdmirx_core/hardware/THDMIRx_TV_Audio_Driver.o
hdmirx-y += aw_hdmirx_core/hardware/THDMIRx_TV_Edid_Driver.o
hdmirx-y += aw_hdmirx_core/THDMIRx_System.o

ifeq ($(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC),y)
hdmirx-y += aw_hdmirx_core/hardware/THDMIRx_TV_Cec_Driver.o
endif

ifeq ($(CONFIG_AW_VIDEO_SUNXI_HDMIRX_LOG_BUFFER),y)
hdmirx-y += aw_hdmirx_log.o
endif

