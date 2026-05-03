/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _THDMIRx_TV_EDID_DRIVER_
#define _THDMIRx_TV_EDID_DRIVER_

#define EDID_DATA_LEN 256
#define EDID_Array_LEN (EDID_DATA_LEN / 4)
#define Extended_EDID_Array_LEN (EDID_Array_LEN / 2)
#define EDID_DWORD_PER_PACK 16
#define EDID_DATA_PER_PACK 64
#define EDID_PACK_NUM 4


#define RESERVED_AUDIO_BLOCK_SIZE 12
#define DATA_BLOCK_START_OFFSET (128 + 4)
#define RESERVED_BLOCK_LENGTH RESERVED_AUDIO_BLOCK_SIZE

#define BLOCK_TYPE_MASK 0xE0    // Bit7:5
#define BLOCK_LENGTH_MASK 0x1F  // Bit4:0
#define BLOCK_TYPE_AUDIO 0x20
#define BLOCK_TYPE_VIDEO 0x40
#define BLOCK_TYPE_VENDOR 0x60
#define BLOCK_TYPE_SPEAKER 0x80
#define BLOCK_TYPE_VESA_DTC 0xA0
#define BLOCK_TYPE_EXTENDED 0xE0

#define RegistrationID_0 0x03
#define RegistrationID_1 0x0C
#define RegistrationID_2 0x00

// HDMI HPD CMD
#define TIME_OUT_RESET_HPD 830  // use 830ms same as Samsung TV

#define MCU_BUSY_TIMEOUT 500
#define HPD_VALID_INTERVAL 200  // default HPD TIME
#define CMD_PULL_UP (0x90)
#define CMD_PULL_DOWN (0xA0)
#define CMD_HPD (0xB0)  //先拉低830ms，再拉高

#define HDMI1_PORT_MASK _BIT0_
#define HDMI2_PORT_MASK _BIT1_
#define HDMI3_PORT_MASK _BIT2_
#define HDMI_ALL_PORT_MASK (HDMI1_PORT_MASK + HDMI2_PORT_MASK + HDMI3_PORT_MASK)

#define HDMI1EDID_Enable _BIT0_
#define HDMI2EDID_Enable _BIT1_
#define HDMI3EDID_Enable _BIT2_

#define HDMI1_PA 0x10  // HDMI1 has physical address 0x1000
#define HDMI2_PA 0x20  // HDMI2 has physical address 0x2000
#define HDMI3_PA 0x30  // HDMI3 has physical address 0x3000

/*
 * Display port no. for HDMI channel
 */
#define HDMI_DISPLAY_P0 0
#define HDMI_DISPLAY_P1 1
#define HDMI_DISPLAY_P2 2

/*
 * DDC channel mask
 */
#define DDC0_PORT_MASK _BIT0_
#define DDC1_PORT_MASK _BIT1_
#define DDC2_PORT_MASK _BIT2_

/*
 * HPD pin no. for HDMI channel
 */
#define HPD_PIN_0 0
#define HPD_PIN_1 1
#define HPD_PIN_2 2

typedef enum _tagHDMI_HOTPLUG {
	CMD_HOTPLUG_PULLUP = 0x1,
	CMD_HOTPLUG_PULLDOWN = 0x2,
	CMD_HOTPLUG_RESET = 0x3,
} HDMI_HOTPLUG;

typedef struct _HDMI_MAP_STRUCT {
	/* port number follow UI sequence: "HDMI-1", "HDMI-2", "HDMI-3" */
	U8 port_mask;   /* port mask for bit shift: HDMI1_PORT_MASK ... */
	U8 pa;          /* physical address: 0x10 0x20 0x30 */
	U8 hotplug_pin; /* hotplug pin: 0 1 2  */
	U8 ddc_mask;    /* ddc port: DDC0_PORT_MASK ... */
	U8 display;     /* display port: P0, P1, P2 */
} HDMI_MAP_STRUCT;

typedef enum _EDID_Version {  // HDMI_FUNC_ID_SET_EDID_VERSION
	HDMI_EDID_Version_14,
	HDMI_EDID_Version_20,
} EDID_Version;

void HdmiRx_Edid_Hardware_set_regbase(uintptr_t edid_top);

void HdmiRx_Edid_Hardware_set_top_cfg(void);

void HdmiRx_Edid_Hardware_set_5v_cfg(void);

U8 tdHdmiGetPortByMask(U8 mask);

void HdmiRx_Edid_Hardware_get_edid(U8 *pedid_data);

bool HdmiRx_Edid_Hardware_GetPort_HpdState(U8 port);

void HdmiRx_Edid_Hardware_Edidreset(void);

void HdmiRx_Edid_Hardware_InitHpdtimer(U8 port_id);

void HdmiRx_Edid_Hardware_DeinitHpdtimer(U8 port_id);

void HdmiRx_Edid_Hardware_UpdateEdid(U8 *edid_table, U8 edid_version);

void HdmiRx_Edid_Hardware_SetEdidVersion(U8 setting_version);

void HdmiRx_Edid_Hardware_SetPullHotplug(U8 port_id, U8 type);

void HdmiRx_Edid_Hardware_Get5VState(U32 *pb5vstate);

void HdmiRx_Edid_Hardware_EdidUpdate(U8 hdmi_port_masks);

void HdmiRx_Edid_Hardware_EdidEnable(U8 port_masks, bool bEnable);

void HdmiRx_Edid_Hardware_HotplugHandler(U8 Type, U8 port);

void HdmiRx_Edid_Hardware_EdidPrint(void);

void HdmiRx_Edid_Hardware_EdidInit(void);

#endif
