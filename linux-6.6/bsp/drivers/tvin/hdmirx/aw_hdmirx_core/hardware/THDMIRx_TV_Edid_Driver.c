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
#include "THDMIRx_TV_Driver.h"
#include "THDMIRx_TV_Edid_Driver.h"
#include "../THDMIRx_System.h"
#include "ThdmiRxIPReg.h"
#include "ThdmiRxMvReg.h"
#include "../aw_hdmirx_core_drv.h"
#include "../../aw_hdmirx_define.h"

U8 HOST_ASSIGNED_EDID_HDMI14[EDID_DATA_LEN] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x5e, 0x78, 0x43, 0x48, 0x21, 0x03, 0x00, 0x00, 0x0d, 0x1a, 0x01, 0x03, 0x81, 0x46,
	0x27, 0x78, 0xeb, 0xd5, 0x7c, 0xa3, 0x57, 0x49, 0x9c, 0x25, 0x11, 0x48, 0x4b, 0xbf, 0xef, 0x00, 0xa9, 0x40, 0x81, 0x59, 0x81, 0x80,
	0x61, 0x59, 0x45, 0x59, 0x31, 0x59, 0x31, 0x19, 0x31, 0xd9, 0x04, 0x74, 0x00, 0x30, 0xf2, 0x70, 0x5a, 0x80, 0xb0, 0x58, 0x8a, 0x00,
	0xc4, 0x8e, 0x21, 0x00, 0x00, 0x1e, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0xc4, 0x8e, 0x21, 0x00,
	0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x53, 0x47, 0x44, 0x20, 0x53, 0x58, 0x38, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
	0x00, 0xfd, 0x00, 0x17, 0x78, 0x0f, 0x8c, 0x1e, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x82, 0x02, 0x03, 0x52, 0xf1,
	0x55, 0x5f, 0x64, 0x5d, 0x62, 0x5e, 0x10, 0x22, 0x20, 0x1f, 0x14, 0x05, 0x04, 0x07, 0x06, 0x03, 0x02, 0x01, 0x11, 0x12, 0x13, 0x15,
	0x2c, 0x57, 0x06, 0x00, 0x3f, 0x07, 0x50, 0x09, 0x7f, 0x01, 0x15, 0x07, 0x50, 0x83, 0x4f, 0x00, 0x00, 0x70, 0x03, 0x0c, 0x00, 0x10,
	0x00, 0xb8, 0x3c, 0xaf, 0x5b, 0x5b, 0x80, 0x80, 0x01, 0x02, 0x03, 0x04, 0xeb, 0x01, 0x46, 0xd0, 0x00, 0x44, 0x6f, 0x6a, 0x8a, 0x58,
	0x65, 0xad, 0xe2, 0x00, 0xff, 0xe2, 0x0f, 0xf0, 0xe3, 0x06, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf1,
};  // HDMI 1.4 edid

U8 HOST_ASSIGNED_EDID_HDMI20[EDID_DATA_LEN] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x5e, 0x78, 0x43, 0x48, 0x21, 0x03, 0x00, 0x00, 0x0d, 0x1a, 0x01, 0x03, 0x81, 0x46,
	0x27, 0x78, 0xeb, 0xd5, 0x7c, 0xa3, 0x57, 0x49, 0x9c, 0x25, 0x11, 0x48, 0x4b, 0xbf, 0xef, 0x00, 0xa9, 0x40, 0x81, 0x59, 0x81, 0x80,
	0x61, 0x59, 0x45, 0x59, 0x31, 0x59, 0x31, 0x19, 0x31, 0xd9, 0x04, 0x74, 0x00, 0x30, 0xf2, 0x70, 0x5a, 0x80, 0xb0, 0x58, 0x8a, 0x00,
	0xc4, 0x8e, 0x21, 0x00, 0x00, 0x1e, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0xc4, 0x8e, 0x21, 0x00,
	0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x53, 0x47, 0x44, 0x20, 0x53, 0x58, 0x38, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
	0x00, 0xfd, 0x00, 0x17, 0x78, 0x0f, 0x8c, 0x3c, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x64, 0x02, 0x03, 0x6d, 0xf1,
	0x55, 0x5f, 0x64, 0x5d, 0x62, 0x5e, 0x10, 0x22, 0x20, 0x1f, 0x14, 0x05, 0x04, 0x07, 0x06, 0x03, 0x02, 0x01, 0x11, 0x12, 0x13, 0x15,
	0x2c, 0x57, 0x06, 0x00, 0x3f, 0x07, 0x50, 0x09, 0x7f, 0x01, 0x15, 0x07, 0x50, 0x83, 0x4f, 0x00, 0x00, 0x70, 0x03, 0x0c, 0x00, 0x10,
	0x00, 0xb8, 0x3c, 0xaf, 0x5b, 0x5b, 0x80, 0x80, 0x01, 0x02, 0x03, 0x04, 0x67, 0xd8, 0x5d, 0xc4, 0x01, 0x78, 0xc8, 0x03, 0xeb, 0x01,
	0x46, 0xd0, 0x00, 0x44, 0x6f, 0x6a, 0x8a, 0x58, 0x65, 0xad, 0xe2, 0x00, 0xff, 0xe2, 0x0f, 0xf0, 0xe3, 0x06, 0x0f, 0x01, 0xe3, 0x05,
	0xe0, 0x00, 0x68, 0x1a, 0x00, 0x00, 0x01, 0x01, 0x30, 0x78, 0x00, 0xe5, 0x01, 0x8b, 0x84, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8,
};  // HDMI 2.0 edid

Edid_state mEdid_state;

HDMI_MAP_STRUCT hdmi_map_table[3];

HDMI_MAP_STRUCT hdmi_soc_data[3] = {
	{HDMI1_PORT_MASK, HDMI1_PA, HPD_PIN_0, DDC0_PORT_MASK, HDMI_DISPLAY_P0},
	{HDMI2_PORT_MASK, HDMI2_PA, HPD_PIN_1, DDC1_PORT_MASK, HDMI_DISPLAY_P1},
	{HDMI3_PORT_MASK, HDMI3_PA, HPD_PIN_2, DDC2_PORT_MASK, HDMI_DISPLAY_P2},
};

struct timer_list hpdtimer[HDMI_PORT_NUM];

/*
 * Get hdmi port by given displaymips port
 */
U8 tdHdmiGetPortByDisplay(U8 disp_port)
{
	U8 port;
	for (port = 0; port < HDMI_PORT_NUM; port++) {
		if (hdmi_map_table[port].display == disp_port)
			return port;
	}

	return HDMI_PORT_NUM;
}

/*
 * tdHdmiGetHpdByPort
 */
static U8 tdHdmiGetHpdByPort(U8 port)
{
	return hdmi_map_table[port].hotplug_pin;
}

/*
 * tdHdmiGetMaskByPort
 */
static U8 tdHdmiGetMaskByPort(U8 port)
{
	return hdmi_map_table[port].port_mask;
}

/*
 * tdHdmiGetPaByPort
 */
static U8 tdHdmiGetPaByPort(U8 port)
{
	return hdmi_map_table[port].pa;
}

/*
 * tdHdmiGetMappedPort
 */
// static Byte tdHdmiGetMappedPort(Byte port) { return hdmi_map_table[port].hotplug_pin; }

/*
 * Get ddc mask of specific hdmi port.
 */
static U8 tdHdmiGetDdcByPort(U8 port)
{
	return hdmi_map_table[port].ddc_mask;
}

/*
 * Convert hdmi port mask bits to ddc mask bits.
 */
U8 convertDdcMask(U8 port_masks)
{
	U8 port, ddc_masks;

	ddc_masks = 0;
	for (port = 0; port < HDMI_PORT_NUM; port++) {
		if (port_masks & (1 << port)) {
			ddc_masks |= tdHdmiGetDdcByPort(port);
		}
	}

	return ddc_masks;
}

/*
 * tdHdmiGetPortByMask
 */
U8 tdHdmiGetPortByMask(U8 mask)
{
	switch (mask) {
	case HDMI1_PORT_MASK:
		return 0;
	case HDMI2_PORT_MASK:
		return 1;
	case HDMI3_PORT_MASK:
		return 2;
	}

	return HDMI_PORT_NUM;
}

void hpdtimer_function(struct timer_list *t)
{
	U32 EDID_value;
	int port;
	for (port = 0; port < HDMI_PORT_NUM; port++) {
		if (t == &hpdtimer[port])
			break;
	}
	EDID_INF("%s: port = %d\n", __func__, port);
	U8 HPD_port = hdmi_map_table[port].hotplug_pin;
	EDID_value = HDMIRx_ReadRegU32(edid_top_base + HDMIRX_HPD_SET_REG);

	//Invert HPD output
	switch (HPD_port) {
	case 0:
		EDID_value &= ~HDMI1_PORT_MASK;
		break;

	case 1:
		EDID_value &= ~HDMI2_PORT_MASK;
		break;

	case 2:
		EDID_value &= ~HDMI3_PORT_MASK;
		break;

	default:
		hdmirx_err("%s: Wrong HDMI Port\n", __func__);
		break;
	}
	HDMIRx_WriteRegU32(edid_top_base + HDMIRX_HPD_SET_REG, EDID_value);
}

void HdmiRx_Edid_Hardware_set_regbase(uintptr_t edid_top)
{
	edid_top_base = edid_top;
	edid_base = edid_top + 0x800;
	hdmirx_top = edid_top + 0x10000;
}

void HdmiRx_Edid_Hardware_set_top_cfg(void)
{
	EDID_INF("%s start\n", __func__);
	HDMIRx_WriteRegU32(hdmirx_top + HDMIRX_CLOCK_GATING_CTR_REG, 0x00110103);
	HDMIRx_WriteRegU32(hdmirx_top + HDMIRX_RESET_CTR_REG, 0x00110103);
	HDMIRx_WriteRegU32(hdmirx_top + HDMIRX_RESET_CTR_REG, 0x0);
	HDMIRx_WriteRegU32(hdmirx_top + HDMIRX_RESET_CTR_REG, 0x00110103);
	HDMIRx_WriteRegU32(hdmirx_top + HDMIRX_INTERRUPT_ENABLE_CTR_REG, 0x3);
}

void HdmiRx_Edid_Hardware_set_5v_cfg(void)
{
	EDID_INF("%s start\n", __func__);
	//HDMIRx_WriteRegU32(edid_top_base + HDMIRX_5V_ON_INT_EN_REG, 0x00000003); //CONFIG +5v INT EN
	//HDMIRx_WriteRegU32(edid_top_base + HDMIRX_5V_OFF_INT_EN_REG, 0x00000003); //CONFIG +5v OFF INT EN
	HDMIRx_WriteRegU32(edid_top_base + HDMIRX_5V_Rising_DEBOUNCE_CNT_REG, 0x00000002); //CONFIG +5v RISE DEBONCE
	HDMIRx_WriteRegU32(edid_top_base + HDMIRX_5V_Falling_DEBOUNCE_CNT_REG, 0x00000002); //CONFIG +5v FALL DEBONCE
	HDMIRx_WriteRegU32(edid_top_base + HDMIRX_5V_DETECT_CTR_REG0, 0x0004e20D); //SEL +5V SCL and EN
	HDMIRx_WriteRegU32(edid_top_base + HDMIRX_5V_DETECT_CTR_REG1, 0x0004e20D); //SEL +5V SCL and EN
}

void HdmiRx_Edid_Hardware_get_edid(U8 *pedid_data)
{
	U32 EDID_value;
	U8 SRAM_EDID[EDID_DATA_LEN] = {0};
	int i;
	HdmiRx_Edid_Hardware_EdidEnable(HDMI1_PORT_MASK, false);
	for (i = 0; i < EDID_Array_LEN; i++) {
		EDID_value = HDMIRx_ReadRegU32(edid_base + EDID_HDMI1_ADDRESS + (i * 4));
		SRAM_EDID[(i * 4)] = (U8)EDID_value;
		SRAM_EDID[(i * 4 + 1)] = (U8)(EDID_value >> 8);
		SRAM_EDID[(i * 4 + 2)] = (U8)(EDID_value >> 16);
		SRAM_EDID[(i * 4 + 3)] = (U8)(EDID_value >> 24);
	}
	HdmiRx_Edid_Hardware_EdidEnable(HDMI1_PORT_MASK, true);
	memcpy(pedid_data, SRAM_EDID, EDID_DATA_LEN);
	//return HOST_ASSIGNED_EDID_HDMI14;
}

void HdmiRx_Edid_Hardware_Edidreset(void)
{
	HDMIRx_WriteRegMaskU32(edid_top_base + HDMIRX_ASSIST_CTR_REG, _BIT3_, 0);
	HDMIRx_WriteRegMaskU32(edid_top_base + HDMIRX_ASSIST_CTR_REG, _BIT3_, _BIT3_);
}

void HdmiRx_Edid_Hardware_InitHpdtimer(U8 port_id)
{
	timer_setup(&hpdtimer[port_id], hpdtimer_function, 0);
}

void HdmiRx_Edid_Hardware_DeinitHpdtimer(U8 port_id)
{
	del_timer(&hpdtimer[port_id]);
}

void HdmiRx_Edid_Hardware_SysGPIOSetHotplug(U8 port, bool bOn)
{
	U32 EDID_value;
	U8 HPD_port = hdmi_map_table[port].hotplug_pin;

	if (!mEdid_state.bEdidRamReady) {
		bOn = false;  // EDID is not ready, keep HPD pin low to avoid HDMI device load wrong EDID data.
		EDID_INF("%s: EDID-HPD %d LOW\n", __func__, HPD_port);
	}
	EDID_value = HDMIRx_ReadRegU32(edid_top_base + HDMIRX_HPD_SET_REG);

	//Invert HPD output
	switch (HPD_port) {
	case 0:
		if (!bOn) {
			EDID_value |= HDMI1_PORT_MASK;
		} else {
			EDID_value &= ~HDMI1_PORT_MASK;
		}
		EDID_INF("%s: Chip HPD RX0 %d\n", __func__, bOn);
		break;

	case 1:
		if (!bOn) {
			EDID_value |= HDMI2_PORT_MASK;
		} else {
			EDID_value &= ~HDMI2_PORT_MASK;
		}
		EDID_INF("%s: Chip HPD RX1 %d\n", __func__, bOn);
		break;

	case 2:
		if (!bOn) {
			EDID_value |= HDMI3_PORT_MASK;
		} else {
			EDID_value &= ~HDMI3_PORT_MASK;
		}
		EDID_INF("%s: Chip HPD RX2 %d\n", __func__, bOn);
		break;

	default:
		hdmirx_err("%s: Wrong HDMI Port\n", __func__);
		break;
	}
	HDMIRx_WriteRegU32(edid_top_base + HDMIRX_HPD_SET_REG, EDID_value);
}

void HdmiRx_Edid_Hardware_HotplugHandler(U8 Type, U8 port)
{
	switch (Type) {
	case CMD_HOTPLUG_PULLUP:
		/* pull up hotplug pin by timer, to avoid conflict.
		* the delay is about 10ms which can be ignored. */
		msleep(10);
		HdmiRx_Edid_Hardware_SysGPIOSetHotplug(port, 1);
		EDID_INF("%s: hpd %d UP\n", __func__, port);
		break;

	case CMD_HOTPLUG_PULLDOWN:
		HdmiRx_Edid_Hardware_SysGPIOSetHotplug(port, 0);  // pull-down hotplug pin immediately
		EDID_INF("%s: hpd %d DOWN\n", __func__, port);
		 break;

	case CMD_HOTPLUG_RESET:
		HdmiRx_Edid_Hardware_SysGPIOSetHotplug(port, 0);  // pull-down hotplug pin immediately
		EDID_INF("%s: 11111\n", __func__);
		msleep(TIME_OUT_RESET_HPD);  // pull-up hotplug pin after timeout
		HdmiRx_Edid_Hardware_SysGPIOSetHotplug(port, 1);
		//mod_timer(&hpdtimer[port], jiffies + msecs_to_jiffies(TIME_OUT_RESET_HPD));
		EDID_INF("%s: hpd %d RESET\n", __func__, port);
		break;

	default:
		hdmirx_err("%s: Unknown Hotplug Type %d\n", __func__, Type);
		break;
	}
}

/*
 * Init hdmi_map_table
 */
void HdmiRx_Edid_Hardware_MapInit(void)
{
	U8 port;
	for (port = 0; port < HDMI_PORT_NUM; port++) {
		hdmi_map_table[port].port_mask = hdmi_soc_data[port].port_mask;
		hdmi_map_table[port].pa = hdmi_soc_data[port].pa;
		hdmi_map_table[port].hotplug_pin = hdmi_soc_data[port].hotplug_pin;
		hdmi_map_table[port].ddc_mask = hdmi_soc_data[port].ddc_mask;
		hdmi_map_table[port].display = hdmi_soc_data[port].display;
	}
}

bool HdmiRx_Edid_Hardware_GetPort_HpdState(U8 port)
{
	U32 HpdStates = HDMIRx_ReadRegU32(edid_top_base + HDMIRX_HPD_SET_REG);
	return !((HpdStates >> port) & 0x1);
}

/*****************************************************************************
*  Name        : setSingleEdidTable
*  Description :

*****************************************************************************/
static void HdmiRx_Edid_Hardware_setSingleEdidTable(U8 hdmi_port_mask, U8 *pEdidData)
{
	U16 i;
	U8 checksum;
	U8 rsvAudioBlockOffset = 0;
	U8 pEdidBuffer[EDID_DATA_LEN] = {0};
	U8 cecPhyAddrOffset = 0xFF, ddc_port, hdmi_port;
	U32 EDID_value;
	U16 data_block_end_offset = 128 + *(pEdidData + 130);
	uintptr_t EdidRegAddress;

	hdmi_port = tdHdmiGetPortByMask(hdmi_port_mask);
	ddc_port = tdHdmiGetDdcByPort(hdmi_port);
	/* check which port will be updated */
	if (ddc_port == DDC0_PORT_MASK) {
		EdidRegAddress = edid_base + EDID_HDMI1_ADDRESS;
	} else if (ddc_port == DDC1_PORT_MASK) {
		EdidRegAddress = edid_base + EDID_HDMI2_ADDRESS;
	} else if (ddc_port == DDC2_PORT_MASK) {
		EdidRegAddress = edid_base + EDID_HDMI3_ADDRESS;
	} else {
		hdmirx_err("%s: ERROR port_mask=0x%x\n", __func__, hdmi_port);
		return;  // didn't find a correct EDID target, do nothing.
	}

	for (i = DATA_BLOCK_START_OFFSET; i < data_block_end_offset;) {
		U8 blockType = *(pEdidData + i) & BLOCK_TYPE_MASK;
		U8 blockLength = *(pEdidData + i) & BLOCK_LENGTH_MASK;
		// LOG("BlockType 0x%x  ", blockType);
		// LOG("Length 0x%x\n", blockLength);

		switch (blockType) {
		case BLOCK_TYPE_VIDEO:
		case BLOCK_TYPE_SPEAKER:
		case BLOCK_TYPE_VESA_DTC:
		case BLOCK_TYPE_EXTENDED:
			i += blockLength + 1;  // jump to next block
			break;

		case BLOCK_TYPE_VENDOR:
			/* Compare 24bit IEEE Registration Identifier (0x000C03) */
			if ((*(pEdidData + i + 1) == RegistrationID_0) && (*(pEdidData + i + 2) == RegistrationID_1) &&
			(*(pEdidData + i + 3) == RegistrationID_2)) {
				cecPhyAddrOffset = i + 4;  // found CEC physical address offset
			}
			i += blockLength + 1;  // jump to next block
			break;

		case BLOCK_TYPE_AUDIO:
			if (blockLength >= RESERVED_BLOCK_LENGTH) {
				/* to support DD+, we put 57 06 00 in reserved block, do not check empty block. */
				rsvAudioBlockOffset = i + 1;  // RESERVED AUDIO BLOCK FOUND !!! (i is block head, i+1 is 1st data)
			}

			i += blockLength + 1;  // jump to next block
			break;

		default:
			//hdmirx_err("%s: WRONG EDID BLOCK TYPE 0x%x\n", __func__, blockType);
			i = 256;  // exit
			break;
		}
	}

	/* Load 0~127 bytes: Basic block */
	/* Load 128~255 bytes: EIA/CEA-861 extension block */
	for (i = 0; i < 255; i++) {
		*(pEdidBuffer + i) = *(pEdidData + i);
	}

	/* Update CEC physical address */
	//*(pEdidBuffer + cecPhyAddrOffset) = convertHdmiPortToPA(hdmi_port);
	*(pEdidBuffer + cecPhyAddrOffset) = tdHdmiGetPaByPort(hdmi_port);

	// calculate checksum for block1
	checksum = 0;
	for (i = 128; i < 255; i++) {
		checksum -= *(pEdidBuffer + i);
	}
	*(pEdidBuffer + 255) = checksum;

	for (i = 0; i < EDID_Array_LEN; i++) {
		EDID_value =
			pEdidBuffer[(i * 4)] | (pEdidBuffer[(i * 4 + 1)] << 8) | (pEdidBuffer[(i * 4 + 2)] << 16) | (pEdidBuffer[(i * 4 + 3)] << 24);
		HDMIRx_WriteRegU32(EdidRegAddress + (i * 4), EDID_value);
	}
}

/*****************************************************************************
*  Name        : loadHostEdidTable
*  Description :
		- load HDMI1.4/2.0 EDID table which is downloaded from ARM.
*****************************************************************************/
void HdmiRx_Edid_Hardware_loadHostEdidTable(U8 port_mask)
{
	U8 port;
	EDID_INF("%s: mEdid_state.bEdidDataReady = %d\n", __func__, mEdid_state.bEdidDataReady);

	if (mEdid_state.bEdidDataReady) {
		// Load HDMI1/2/3 EDID
		for (port = 0; port < HDMI_PORT_NUM; port++) {
			if (port_mask & tdHdmiGetMaskByPort(port)) {
				if (mEdid_state.ucEdidHdmi20Setting & tdHdmiGetMaskByPort(port)) {
					HdmiRx_Edid_Hardware_setSingleEdidTable(tdHdmiGetMaskByPort(port), HOST_ASSIGNED_EDID_HDMI20);  // hdmi2.0 edid
				} else {
					HdmiRx_Edid_Hardware_setSingleEdidTable(tdHdmiGetMaskByPort(port), HOST_ASSIGNED_EDID_HDMI14);  // hdmi1.4 edid
				}
			}
		}
	}
}

/*****************************************************************************
*  Name        : tdHdmiEdidUpdate
*  Description :
*  Params      :
*  Returns     :
*****************************************************************************/
void HdmiRx_Edid_Hardware_EdidUpdate(U8 hdmi_port_masks)
{
	if (mEdid_state.bEdidDataReady) {
		// host edid is ready
		/* Start reset hotplug pins before update EDID table */
		//SysGPIOResetHotplug(hdmi_port_masks, _TRUE_, TIME_OUT_RELOAD_EDID);
		//LOG("Reset HPD, update edid, mask=0x%x\n", hdmi_port_masks);
		HdmiRx_Edid_Hardware_EdidEnable(hdmi_port_masks, false);
		HdmiRx_Edid_Hardware_loadHostEdidTable(hdmi_port_masks);
		EDID_INF("%s: edid updated\n", __func__);
		HdmiRx_Edid_Hardware_EdidEnable(hdmi_port_masks, true);
		mEdid_state.bEdidRamReady = true;
	}
}

/*****************************************************************************
*  Name        : tdHdmiEdidEnable
*  Description :
		- 1: SRAM can only br read by I2C host(tx).
		- 0: AHB(ARM) can read/write access SRAM.
*****************************************************************************/
void HdmiRx_Edid_Hardware_EdidEnable(U8 port_masks, bool bEnable)
{
	U32 HDMIEDID_CTL;
	U8 ddc_ports = convertDdcMask(port_masks);

	HDMIEDID_CTL = HDMIRx_ReadRegU32(edid_base + I2C_CTRL_REG);

	if (bEnable && mEdid_state.bEdidDataReady) {
		// HDMIEDID_CTL |= HDMI3EDID_Enable | HDMI2EDID_Enable |HDMI1EDID_Enable;
		if (ddc_ports & DDC0_PORT_MASK) {
			HDMIEDID_CTL |= HDMI1EDID_Enable;
		}
		if (ddc_ports & DDC0_PORT_MASK) {
			HDMIEDID_CTL |= HDMI2EDID_Enable;
		}
		if (ddc_ports & DDC0_PORT_MASK) {
			HDMIEDID_CTL |= HDMI3EDID_Enable;
		}

		EDID_INF("%s: enable EDID\n", __func__);
	} else {
		// HDMIEDID_CTL &= ~(HDMI3EDID_Enable | HDMI2EDID_Enable |HDMI1EDID_Enable);
		if (ddc_ports & HDMI1_PORT_MASK) {
			HDMIEDID_CTL &= ~HDMI1EDID_Enable;
		}
		if (ddc_ports & DDC0_PORT_MASK) {
			HDMIEDID_CTL &= ~HDMI2EDID_Enable;
		}
		if (ddc_ports & DDC0_PORT_MASK) {
			HDMIEDID_CTL &= ~HDMI3EDID_Enable;
		}
	}
	HDMIRx_WriteRegU32(edid_base + I2C_CTRL_REG, HDMIEDID_CTL);
}

/*****************************************************************************
*  Name        : tdHdmiEdidPrint
*  Description : This function will print EDID table on UART.
*  Params      :
*  Returns     :
******************************************************************************/
void HdmiRx_Edid_Hardware_EdidPrint(void)
{
	int i;

	mEdid_state.bEdidRamReady = false;
	HdmiRx_Edid_Hardware_EdidEnable(HDMI_ALL_PORT_MASK, false);
	hdmirx_inf("\nEDID HDMI 1:\n");
	for (i = 0; i < EDID_Array_LEN; i++) {
		printk("0x%x\n", HDMIRx_ReadRegU32(edid_base + EDID_HDMI1_ADDRESS + (i * 4)));
	}
	hdmirx_inf("\nEDID HDMI 2:\n");
	for (i = 0; i < EDID_Array_LEN; i++) {
		printk("0x%x\n", HDMIRx_ReadRegU32(edid_base + EDID_HDMI2_ADDRESS + (i * 4)));
	}
	hdmirx_inf("\nEDID HDMI 3:\n");
	for (i = 0; i < EDID_Array_LEN; i++) {
		printk("0x%x\n", HDMIRx_ReadRegU32(edid_base + EDID_HDMI3_ADDRESS + (i * 4)));
	}

	HdmiRx_Edid_Hardware_EdidEnable(HDMI_ALL_PORT_MASK, true);
	mEdid_state.bEdidRamReady = true;
	printk("\n");
}

void HdmiRx_Edid_Hardware_EdidInit(void)
{
	//   Byte i;
	U8 I2C_ADDR_0 = 0xA0;
	U8 I2C_ADDR_1 = 0xA0;
	U8 I2C_ADDR_2 = 0xA0;
	U32 value = 0;
	U32 reg_val;

	/* set I2C_CTRL_DEBUG_REG(0x07091b08) to 0xC0C0C0C0 */
	HDMIRx_WriteRegU32(edid_base + I2C_CTRL_DEBUG_REG, 0xc0c0c0c0);

	/* set I2C_CTRL_REG(0x07091b04) bit[31:24] to 0 , not surpport segment*/
	reg_val = HDMIRx_ReadRegU32(edid_base + I2C_CTRL_REG);
	reg_val &= ~(0xff << 24);
	HDMIRx_WriteRegU32(edid_base + I2C_CTRL_REG, reg_val);

	HdmiRx_Edid_Hardware_MapInit();

	mEdid_state.bEdidDataReady = false;  // EDID data buffer is empty after AC power on, must get data from host cpu.
	mEdid_state.bEdidRamReady = false;   // EDID SRAM is empty after AC power on, must be filled with EDID data
	// g_Data.ucEdidDynamicMode = 0;       // DTS support: all ports default OFF
	mEdid_state.ucEdidHdmi20Setting = 0x0;  // all ports use HDMI1.4 EDID by default.
	HdmiRx_Edid_Hardware_EdidEnable(HDMI_ALL_PORT_MASK, true);

	value = (I2C_ADDR_0 | (I2C_ADDR_1 << 8) | (I2C_ADDR_2 << 16));
	HDMIRx_WriteRegU32(edid_base + I2C_ADDR_REG, value);
	mEdid_state.bEdidDataReady = true;  // EDID data is downloaded from ARM.
	HdmiRx_Edid_Hardware_EdidUpdate(HDMI_ALL_PORT_MASK);

	//HdmiRx_Edid_Hardware_EdidPrint();
}

void HdmiRx_Edid_Hardware_UpdateEdid(U8 *edid_table, U8 edid_version)
{
	mEdid_state.bEdidDataReady = false;
	if ((EDID_Version)edid_version == HDMI_EDID_Version_14) {
		memcpy(HOST_ASSIGNED_EDID_HDMI14, edid_table, 256);
	} else if ((EDID_Version)edid_version == HDMI_EDID_Version_20) {
		memcpy(HOST_ASSIGNED_EDID_HDMI20, edid_table, 256);
	}
	mEdid_state.bEdidDataReady = true;
	HdmiRx_Edid_Hardware_EdidUpdate(HDMI_ALL_PORT_MASK);
}

void HdmiRx_Edid_Hardware_SetEdidVersion(U8 setting_version)
{
	/*
		Bit3: HDMI4 use HDMI2.0
		Bit2: HDMI3 use HDMI2.0
		Bit1: HDMI2 use HDMI2.0
		Bit0: HDMI1 use HDMI2.0
	*/
	EDID_INF("%s Host Set EDID Version.  0x%x\n", __func__, setting_version);
	if (mEdid_state.ucEdidHdmi20Setting != setting_version) {
		U8 mask = mEdid_state.ucEdidHdmi20Setting ^ setting_version;
		mEdid_state.ucEdidHdmi20Setting = setting_version & 0x0F;
		HdmiRx_Edid_Hardware_EdidUpdate(mask & 0x0F);
	}
}

void HdmiRx_Edid_Hardware_SetPullHotplug(U8 port_id, U8 type)
{
	EDID_INF("%s Host pull Hotplug port %d, value = %d\n", __func__, port_id, type);
	HdmiRx_Edid_Hardware_HotplugHandler(type, port_id);
}

void HdmiRx_Edid_Hardware_Get5VState(U32 *pb5vstate)
{
	*pb5vstate = HDMIRx_ReadRegU32(edid_top_base + HDMIRX_5V_ON_STATUS_REG);
	EDID_INF("%s 5vstate = 0x%x\n", __func__, *pb5vstate);
}
