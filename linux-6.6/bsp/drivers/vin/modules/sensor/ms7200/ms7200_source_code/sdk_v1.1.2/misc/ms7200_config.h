/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file    ms7200_config.h
* @author
* @version V1.0.0
* @date    15-Nov-2014
* @brief   header files
*
* Copyright (c) 2009-2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_CONFIG_H__
#define __MACROSILICON_MS7200_CONFIG_H__

//#include "ms7200_app_config.h"


#ifndef MS7200_EXT_XTAL
#define MS7200_EXT_XTAL             (27000000UL) //uint Hz
#endif

#ifndef MS7200_USE_I2CBUS
#define MS7200_USE_I2CBUS           (1)
#endif

#ifndef MS7200_I2C_ADDR
#define MS7200_I2C_ADDR             (0xB2)
#endif

#ifndef MS7200_HDMI_RX_EDID_ENABLE
#define MS7200_HDMI_RX_EDID_ENABLE  (1)
#endif

#ifndef MS7200_HDMI_TX_HDCP
#define MS7200_HDMI_TX_HDCP         (1)
#endif

#ifndef MS7200_HDMI_TX_HDCP_METHOD
#define MS7200_HDMI_TX_HDCP_METHOD  (0)
#endif

#ifndef MS7200_HDMI_TX_EDID
#define MS7200_HDMI_TX_EDID         (1)
#endif

#ifndef MS7200_HDMI_RX_INT_ENABLE
#define MS7200_HDMI_RX_INT_ENABLE   (1)
#endif

#ifndef MS7200_RXPLL_METHOD
#define MS7200_RXPLL_METHOD     (1) //new method
#endif


//RC_25M config
#ifndef MS7200_RC_CTRL1
#define MS7200_RC_CTRL1             (0x81)
#endif

#ifndef MS7200_RC_CTRL2
#define MS7200_RC_CTRL2             (0x3B)     //24M RC
#endif


#ifndef MS7200_HDMI_RX_TMDS_OVERLOAD_PROTECT_ENABLE
#define MS7200_HDMI_RX_TMDS_OVERLOAD_PROTECT_ENABLE     (0)
#endif


#ifndef MS7200_AUDIO_SAMPLE_PACKET_192BIT_BYPASS_ENABLE
#define MS7200_AUDIO_SAMPLE_PACKET_192BIT_BYPASS_ENABLE     (1)
#endif


////////////////////////////////////////////////////////////////////
#ifndef MS7200_FPGA_VERIFY
#define MS7200_FPGA_VERIFY          (0)
#endif

#ifndef MS7200_EXT_APIS
#define MS7200_EXT_APIS             (0)     //external drive
#endif

#ifndef MS7200_DEBUG_LOG
#define MS7200_DEBUG_LOG            (0)      //use uart trace log
#endif

#endif  // __MACROSILICON_MS7200_CONFIG_H__
