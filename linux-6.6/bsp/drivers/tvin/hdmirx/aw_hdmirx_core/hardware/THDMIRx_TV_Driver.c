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
#include<linux/timer.h>
#include <linux/sched.h>
#include <linux/delay.h>


#include "../../aw_hdmirx_define.h"
#include "THDMIRx_TV_Driver.h"
#include "THDMIRx_TV_Edid_Driver.h"
#include "../THDMIRx_System.h"
#include "../THDMIRx_Event.h"
#include "../THDMIRx_PortCtx.h"
#include "../THDMIRx_DisplayModuleCtx.h"
#include "ThdmiRxIPReg.h"
#include "ThdmiRxMvReg.h"

// hdmi interrupts request mask
#define HDMI_INT_REQ_MASK (HDMI_MSK_DVI_LOCKED | HDMI_MSK_HDMI_LOCKED | HDMI_MSK_PKT_LOCKED | HDMI_MSK_PHY_UNLOCKED)

// audio interrupts request mask
#define AUDIO_INT_REQ_MASK (AD_MSK_CS_DIFF | AD_MSK_FIFO_UNDERRUN | AD_MSK_FIFO_OVERRUN | AD_MSK_PLL_LOCKED)

// audio error interrupts request mask
#define AUDIO_ERROR_INT_REQ_MASK (ADERR_MSK_CHECK_SUM | ADERR_MSK_STATUS_BIT)

// packet diff 3 interrupts request mask
#define PACKET_DIFF_INT_REQ_MASK_3 (PKT_DIFF_MSK_VSI_LLC | PKT_DIFF_MSK_GCP | PKT_DIFF_MSK_ACP | PKT_DIFF_MSK_GP2)

// packet diff 4 interrupts request mask
#define PACKET_DIFF_INT_REQ_MASK_4 (PKT_DIFF_MSK_VSI | PKT_DIFF_MSK_SPDI | PKT_DIFF_MSK_AVI | PKT_DIFF_MSK_AI | PKT_DIFF_MSK_ACR)

// HDCP2.2 BCH error interrupts request mask
#define HDCP22_BCH_INT_REQ_MASK (ADERR_MSK_BCH | ADERR_MSK_BCH_HDCP22)

#define SHARE_REGISTER_WITH_MCU 0x19B00008
#define CMD_SEND_MASK _BIT31_
#define CMD_VALUE_MASK 0xFF

#define KEY_READY_TIMEOUT 50

#define MAINLINK_0_EN_MASK (MVPORT_EN_INST_0_OUTPUT | MVPORT_EN_INST_0_INPUT)
#define MAINLINK_1_EN_MASK (MVPORT_EN_INST_1_OUTPUT | MVPORT_EN_INST_1_INPUT)

static const U32 MV_Port_Base[] = {HDCPRX_MV_P0_BASE_ADDR, HDCPRX_MV_P1_BASE_ADDR, HDCPRX_MV_P2_BASE_ADDR, HDCPRX_MV_P3_BASE_ADDR};
static const U32 Link_Base[] = {HDMIRX_LINK_P0_REG_BASE, HDMIRX_LINK_P1_REG_BASE, HDMIRX_LINK_P2_REG_BASE, HDMIRX_LINK_P3_REG_BASE};

#define AEC_TAB_HDMI14 0
#define AEC_TAB_HDMI22 1
#define AEC_TAB_CNT 2
#define AEC_DATA_CNT 4
static const U32 AEC_DB_TV303[AEC_TAB_CNT][AEC_DATA_CNT] = {
	{0x0, 0xc, 0x305, 0x4},       // AEC_TAB_HDMI14
	{0x3a6, 0x3a6, 0x3a6, 0x3a6}  // update hdmi2.0 for union2
};

// Default hdmi port map
static HDMIPortMap_t HDMIPortMap[HDMI_PORT_NUM] = {
	{kSourceId_HDMI_1, HDMI_PORT1, MCU_PORT2}, {kSourceId_HDMI_2, HDMI_PORT2, MCU_PORT1}, {kSourceId_HDMI_3, HDMI_PORT3, MCU_PORT3}
};

U32 HdmiRx_GetLinkBaseByID(LinkBaseID_e link_id)
{
	return HDMIRX_LINK_P0_REG_BASE;
}

void HdmiRx_set_regbase(uintptr_t reg_base)
{
	vs_base = reg_base;
	link_base = reg_base + 0x40000;
	phy_base = reg_base + 0x800;
	audio_pll_base = reg_base + 0x80000;
}

void HdmiRx_SendHPDCMD(U8 cmd, U8 PortID)
{
	EVENT_INF("%s: port%d cmd=%d tick=%d\n", __func__, PortID, cmd, HDMIRx_GetCurrentTick());
	HdmiRx_Edid_Hardware_HotplugHandler(cmd, PortID - 1);
}

//HDCP1.4
void HdmiRx_HDCP14_EnableAKSV_ISR(void)
{
	if ((HDMIRx_ReadRegU8(link_base + HDCP_INT_MASK) & HDCP_MASK_AKSV_UPDATED) != HDCP_MASK_AKSV_UPDATED) {
		HDMIRx_WriteRegMaskU8(link_base + HDCP_INT_MASK, HDCP_MASK_AKSV_UPDATED, HDCP_MASK_AKSV_UPDATED);
	}
}

bool HdmiRx_HDCP14_LoadKey(void)
{
	U32 t0;
	U8 data;

	if (HDMIRx_ReadRegU8(link_base + HDCP_MEMORY_CONTROL_1) & KEY_READY) {
		HDCP_INF("%s: HDCP1.4 key has been loaded!\n", __func__);
		return true;
	}

	HDMIRx_WriteRegU8(link_base + HDCP_MEMORY_CONTROL_1, LOAD_HDCP_KEY | USE_KSV1);
	t0 = HDMIRx_GetCurrentTick();

	do {
		if (HDMIRx_GetCurrentTick() - t0 > KEY_READY_TIMEOUT) {
			hdmirx_err("%s(), time out!\n", __FUNCTION__);
			return false;
		}
		data = HDMIRx_ReadRegU8(link_base + HDCP_MEMORY_CONTROL_1);
	} while (!(data & KEY_READY));
	HDCP_INF("%s: HDCP1.4 key has been loaded!\n", __func__);

	return true;
}

void HdmiRx_HDCP14_ReloadKey(void)
{
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_HKEY, SYSTEM_PD_HKEY);
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_HKEY, 0);
	HDMIRx_WriteRegU8(link_base + HDCP_MEMORY_CONTROL_1, LOAD_HDCP_KEY | USE_KSV1);
}

static void HdmiRx_HDCP14_ISR_Handle_ByLink(U8 PortID, bool *pAuthResult)
{
	U8 val;
	val = HDMIRx_ReadRegU8(link_base + INTERRUPT_REASON_REGISTER);
	if (!(val & HDCP_INT)) {
		return;
	}

	val = HDMIRx_ReadRegU8(link_base + HDCP_INTERRUPT_REASON);
	HDMIRx_WriteRegU8(link_base + HDCP_INTERRUPT_REASON, val);

	// Aksv written by source interrupt
	if (val & HDCP_INT_AKSV_UPDATED) {
		HDMIRx_WriteRegMaskU8(link_base + EDID_ACCESS_CONTROL, SEL_HDCP22_DEFAULT_OFFSET, 0);

		HDMIRx_WriteRegMaskU8(link_base + HDCP22_SELECT, _BIT2_, 0);
		HDMIRx_WriteRegMaskU8(link_base + HDCP22_UPDATE_KEY, HDCP22_AUTHENTICATED, 0);
		// decryption stop interrupt or aksv updated interrupt
		// clear force the RI error bit
		HDMIRx_WriteRegU8(link_base + HDCP_CONTROL, 0);
		*pAuthResult = true;
		//hdmirx_inf("%s: port%d HDCP1.4 Pass!", __func__, PortID);
	}

	// Decryption stop interrupt
	if (val & HDCP_INT_ENC_DIS) {
		// decryption stop interrupt or aksv updated interrupt
		// clear force the RI error bit
		HDMIRx_WriteRegU8(link_base + HDCP_CONTROL, 0);
	}
	return;
}

void HdmiRx_HDCP14_ISR_Handle(U8 PortID, bool *pAuthResult)
{
	HdmiRx_HDCP14_ISR_Handle_ByLink(PortID, pAuthResult);
}

void HdmiRx_HDCP14_ReAuthReq(bool isReAuth)
{
	HDMIRx_WriteRegMaskU8(link_base + HDCP_CONTROL, FORCE_RI_ERR, (isReAuth) ? FORCE_RI_ERR : 0);
}

void HdmiRx_HDCP14_GetStatus(THdcp14Info *pHdcp14Info)
{
	int i = 0;
	U8 data = 0;

	for (i = 0; i < KSVSIZE; i++) {
		pHdcp14Info->aksv[i] = HDMIRx_ReadRegU8(link_base + AKSV0 + i);
		pHdcp14Info->bksv[i] = HDMIRx_ReadRegU8(link_base + BKSV0 + i);
	}

	pHdcp14Info->hdcpkey = HDMIRx_ReadRegU8(link_base + HDCP_STATUS) == 0x45 ? true : false;
	data = HDMIRx_ReadRegU8(link_base + HDCP_STATUS);
	if (data & HDCP_AUTH == HDCP_AUTH) {
		pHdcp14Info->auth = true;
	} else {
		pHdcp14Info->auth = false;
	}
}

// data path
void HdmiRx_MAC_Init(void)
{
	//0x6840000
	// turn off auto colorspace conv to minimize the number of csc in the system
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_CSC_CONTROL, AUTO_CSC_ON, 0);

	// set div number
	HDMIRx_WriteRegMaskU8(link_base + PHY_CONTROL_27, DIV_NUM_MASK, DEFAULT_DIV_NUM);

	// turn on EDR path connection
	HDMIRx_WriteRegMaskU8(link_base + EDR_VDE_HDE_CONFIG_0, EDR_SYNC_OUTPUT_EN | EDR_VIDEO_OUTPUT_EN,
					EDR_SYNC_OUTPUT_EN | EDR_VIDEO_OUTPUT_EN);

	// for 1080P 50HZ signal
	HDMIRx_WriteRegMaskU8(link_base + lol_fil_depth_1, 0xff, 0);
	HDMIRx_WriteRegMaskU8(link_base + lol_fil_depth_2, 0xff, 0);

	// Active edge of VSYNC used for interlace detection.
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_DEBUG_REGISTER, INTL_VEDGE_SEL_R0, 0);

	// set Lock count for hsync/vsync
	HDMIRx_WriteRegU8(link_base + LOCK_SYNC_NUM_HSYNC, LOCK_SYNC_HSYNC_COUNT);
	HDMIRx_WriteRegU8(link_base + LOCK_SYNC_NUM_VSYNC, LOCK_SYNC_VSYNC_COUNT);
}

void HdmiRx_PHY_Init(void)
{
	// use Audio PLL as audio clock source
	HDMIRx_WriteRegMaskU8(link_base + PHY_CONTROL_19, SLP_N_RXC, SLP_N_RXC);

#if (HDMIRX_USE_APLL)
	// use external Audio PLL as audio clock source
	HDMIRx_WriteRegMaskU8(link_base + PHY_CONTROL_34, SEL_EXT_A, SEL_EXT_A);
	HDMIRx_WriteRegMaskU8(link_base + HDMIAPLL_PERI_RST_REG, HDMIAPLL_POWERUP, HDMIAPLL_POWERUP);
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, SEL_PHY_AUDIO_CLK, SEL_PHY_AUDIO_CLK);
#else
	// use RX PHY clock as audio clock source
	HDMIRx_WriteRegMaskU8(link_base + PHY_CONTROL_34, SEL_EXT_A, 0);
	HDMIRx_WriteRegMaskU8(link_base + HDMIAPLL_PERI_RST_REG, HDMIAPLL_POWERUP, 0);
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, SEL_PHY_AUDIO_CLK, 0);

	// power down APLL
	HdmiRx_Audio_PowerDownAPLL();

#endif
	// sel_phy_audio_clk
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, ACLK_EN, 0);

	// don't know why phy0 audio config affect audio detect
	HDMIRx_WriteRegMaskU8(link_base + PHY_CONFIG_1, SEL_PHY_AUDIO_CLK, 0);

	// Enable BOTH_EDGE_5v BOTH_EDGE_SV BOTH_EDGE_LOL
	HDMIRx_WriteRegU8(link_base + PHY_CONTROL_11, BOTH_EDGE_5V | BOTH_EDGE_SV | BOTH_EDGE_LOL | PLL_RESET);

	// pl_div25_en: PLL VCO clock by 25 divider enable
	HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF4, PLL_DIV25_EN, 0);

	// enble frequency detection
	HDMIRx_WriteRegMaskU8(link_base + PHY_CONTROL_27, EN_FREQ_DET, EN_FREQ_DET);

	// enable path,0/1:enable/disable
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_NORMAL_PATH, ~SYSTEM_PD_NORMAL_PATH);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_420_PATH, SYSTEM_PD_420_PATH);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_EDR_PATH, ~SYSTEM_PD_EDR_PATH);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_FULL_REP_PATH, SYSTEM_PD_FULL_REP_PATH);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_POWER, SYSTEM_PD_PHY_REP_PATH, SYSTEM_PD_PHY_REP_PATH);

	// default disable short protect
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RBD, SHP_OFF, SHP_OFF);

	HDMIRx_WriteRegU8(phy_base + 0xfb, 0x0b);
}

void HdmiRx_Port_Select(U8 PortID)
{
	hdmirx_inf("%s: HdmiRx_Port_Select port %d", __func__, PortID);
	// phy port
	HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF1, PORTSEL_MASK, PortID << PORTSEL_SHIFT);
	// DDC port
	HDMIRx_WriteRegMaskU8(link_base + DDC_PORT_SELECT, DDC_PORTSEL_MASK, PortID);
}

void HdmiRx_PHY_SetOutputChannel(U8 mode)
{
	if (OutputChannelMode_Default == mode) {
		HDMIRx_WriteRegMaskU8(link_base + HDMI_20_IN_CTRL, CH2_MASK | CH1_MASK | CH0_MASK, HDMI_CHANNEL_MODE_DEF);
	} else if (OutputChannelMode_EDR_LL == mode) {
		HDMIRx_WriteRegMaskU8(link_base + HDMI_20_IN_CTRL, CH2_MASK | CH1_MASK | CH0_MASK, HDMI_CHANNEL_MODE_EDR_LL);
	}
}

void HdmiRx_PHY_Reset(void)
{
	LOG_TRACE();
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_1, SYSTEM_PD_TMDS | SYSTEM_PD_IDCLK, 0);
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, RESQ_RESET, 0x0);
	HDMIRx_WriteRegMaskU8(phy_base  + PHY_CONFIG_1, RESQ_RESET, RESQ_RESET);
}

void HdmiRx_SCDC_Reset(void)
{
	LOG_TRACE();
#if 1
	HDMIRx_WriteRegU8(link_base + SCDC_SOURCE_VERSION, 0);
	HDMIRx_WriteRegMaskU8(link_base + SCDC_UPDATE_0, STATUS_UPDATE | CED_UPDATE | RR_TEST, 0);
	HDMIRx_WriteRegMaskU8(link_base + SCDC_TMDS_CONFIG, TMDS_BIT_CLOCK_RATIO | SCRAMBLINE_ENABLE, 0);
	HDMIRx_WriteRegMaskU8(link_base + SCDC_SCRAMBLE_STATUS, SCRAMBLING_STATUS, 0);
	HDMIRx_WriteRegMaskU8(link_base + SCDC_CONFIG_0, RR_ENABLE, 0);
	HDMIRx_WriteRegMaskU8(link_base + SCDC_STATUS_0, CLOCK_DETECTED | CH0_LOCKED | CH1_LOCKED | CH2_LOCKED, 0);
	HDMIRx_WriteRegU8(link_base + SCDC_TEST_CONFIG_0, 0);

// reset EN_SCRAMBLER
	HDMIRx_WriteRegMaskU8(link_base + LFSR_CONTROL_1, EN_SCRAMBLER, 0);
// reset Rx mode
	//HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF3, RX_MODE, 0);
	HDMIRx_WriteRegU8(phy_base + CTRLA_PHY_RF3, 0);
#endif
}

void HdmiRx_MV_SCDC_Reset(U32 scdc_base)
{
	LOG_TRACE();
	HDMIRx_WriteRegU8(scdc_base + MVPORT_SCDC_SOURCE_VERSION, 0);
	HDMIRx_WriteRegMaskU8(scdc_base + MVPORT_SCDC_UPDATE_0, MVPORT_STATUS_UPDATE | MVPORT_CED_UPDATE | MVPORT_RR_TEST, 0);
	HDMIRx_WriteRegMaskU8(scdc_base + MVPORT_SCDC_TMDS_CONFIG, MVPORT_TMDS_BIT_CLOCK_RATIO | MVPORT_SCRAMBLINE_ENABLE, 0);
	HDMIRx_WriteRegMaskU8(scdc_base + MVPORT_SCDC_SCRAMBLE_STATUS, MVPORT_SCRAMBLING_STATUS, 0);
	HDMIRx_WriteRegMaskU8(scdc_base + MVPORT_SCDC_CONFIG_0, MVPORT_RR_ENABLE, 0);
	HDMIRx_WriteRegMaskU8(scdc_base + MVPORT_SCDC_STATUS_0,
						MVPORT_CLOCK_DETECTED | MVPORT_CH0_LOCKED | MVPORT_CH1_LOCKED | MVPORT_CH2_LOCKED, 0);
	HDMIRx_WriteRegU8(scdc_base + MVPORT_SCDC_TEST_CONFIG_0, 0);
}

void HdmiRx_MVMode_Enable(bool isEnable)
{
	LOG_TRACE1(isEnable);
	// enter MV mode
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVPORT_MULTIVIEW_CONTROL, MVPORT_EN_MV_MODE | MVPORT_EN_MV_CLOCKS, 0);
	// Main link
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVPORT_MULTIVIEW_MUX_CONTROL, MAINLINK_0_EN_MASK,
						isEnable ? MAINLINK_0_EN_MASK : ~MAINLINK_0_EN_MASK);
	// Sub link
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVPORT_MULTIVIEW_MUX_CONTROL, MAINLINK_1_EN_MASK,
						isEnable ? MAINLINK_1_EN_MASK : ~MAINLINK_1_EN_MASK);
}

void HdmiRx_MVMode_HDCP_PathEnable(U32 RegBase, bool isEnable)
{
}

void HdmiRx_MVMode_ConnectLink(U32 base, LinkBaseID_e linkID, U32 portID)
{
}

void HdmiRx_MVMode_DisconnectLink(U32 base, LinkBaseID_e linkID, U32 portID)
{
}

bool HdmiRx_MVMode_Link_IsConnect(U32 base, LinkBaseID_e linkID)
{
	U8 ucVal = HDMIRx_ReadRegU8(base + MVPORT_MVP_SWITCH_STATUS);
	if (LinkBaseID_0 == linkID) {
		return ucVal & MVPORT_CONN_STATUS_MAIN_CONNECT;
	} else if (LinkBaseID_1 == linkID) {
		return ucVal & MVPORT_CONN_STATUS_SUB_CONNECT;
	}

	return false;
}

void HdmiRx_MVMode_EnableOutput(bool isEnable)
{
	LOG_TRACE1(isEnable);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_0, EN_PHY_CLK, isEnable ? (EN_PHY_CLK) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_MUX_CONTROL_3, 0x0F, isEnable ? (0x0F) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_7, 0x0F, isEnable ? (0x0F) : 0x0);
	// port0 ICG
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_1, 0x7F, isEnable ? (0x7F) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_2, 0xFF, isEnable ? (0xFF) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_3, 0x3F, isEnable ? (0x3F) : 0x0);
	// port1 ICG
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_4, 0x7F, isEnable ? (0x7F) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_5, 0xFF, isEnable ? (0xFF) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_6, 0x3F, isEnable ? (0x3F) : 0x0);
	// port2 ICG
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_8, 0x7F, isEnable ? (0x7F) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_9, 0xFF, isEnable ? (0xFF) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_10, 0x3F, isEnable ? (0x3F) : 0x0);
	// port3 ICG
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_11, 0x7F, isEnable ? (0x7F) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_12, 0xFF, isEnable ? (0xFF) : 0x0);
	HDMIRx_WriteRegMaskU8(HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_13, 0x3F, isEnable ? (0x3F) : 0x0);
}

// enable/disable top icg, used to power saving
void HdmiRx_MVTOP_ICGEnable(U8 port_id, bool isEnable)
{
	LOG_TRACE2(port_id, isEnable);
	U8 ICG_PORT_ADDR[] = {MVTOP_ICG_CONTROL_1, MVTOP_ICG_CONTROL_4, MVTOP_ICG_CONTROL_8, MVTOP_ICG_CONTROL_11};
	uintptr_t ICG_BASE_ADDR = link_base + HDCPRX_MV_TOP_BASE_ADDR + ICG_PORT_ADDR[port_id];
	HDMIRx_WriteRegMaskU8(ICG_BASE_ADDR, 0x7F, isEnable ? (0x7F) : 0x0);
	HDMIRx_WriteRegMaskU8(ICG_BASE_ADDR + 1, 0xFF, isEnable ? (0xFF) : 0x0);
	HDMIRx_WriteRegMaskU8(ICG_BASE_ADDR + 2, 0x3F, isEnable ? (0x3F) : 0x0);

	HDMIRx_WriteRegMaskU8(link_base + HDCPRX_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_0, EN_PHY_CLK, isEnable ? (EN_PHY_CLK) : 0x0);

	HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF1, BI_STANDBY, isEnable ? (0x0) : BI_STANDBY);
}

void HdmiRx_VideoEnableOutput(bool isEnable)
{
	LOG_TRACE1(isEnable);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, VIDEO_OUTPUT_EN, isEnable);
}

void HdmiRx_AudioEnableOutput(bool isEnable)
{
	LOG_TRACE1(isEnable);
	HDMIRx_WriteRegMaskU8(link_base + AUDIO_FORMAT_CONTROL, AUDIO_ENABLE_MASK, isEnable ? AUDIO_ENABLE_MASK : ~AUDIO_ENABLE_MASK);
}

U8 HdmiRx_GetAVMuteState(U32 base)
{
	return HDMIRx_ReadRegU8(base + GENERAL_CONTROL_PACKET_1);
}

void HdmiRx_SYNCEnableOutput(bool isEnable)
{
	LOG_TRACE1(isEnable);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_OUTPUT_PIN_CONTROL_1, SYNC_OUTPUT_EN, isEnable ? SYNC_OUTPUT_EN : 0);
}

bool HdmiRx_GetPort_HpdState(U8 port_id)
{
	return HdmiRx_Edid_Hardware_GetPort_HpdState(port_id);
}

bool HdmiRx_GetPort_5VState(U8 PortID)
{
	U8 port_status = HDMIRx_ReadRegU8(link_base + HDMI_5V_STATUS_REG) & 0xF;
	bool b5vPreset = HDMIRx_ReadBit(port_status, PortID);

	return b5vPreset;
}

bool HdmiRx_GetPort_SignalValid(void)
{
	bool bSignalValid = false;

	bSignalValid = (HDMIRx_ReadRegU8(link_base + SYSTEM_CONTROL_2) & SIGNAL_VALID) ? true : false;
	bSignalValid &= ((HDMIRx_ReadRegU8(link_base + PHY_PLL_STATE) & PLL_LOCK) ? true : true);

	return bSignalValid;
}

bool HdmiRx_GetPort_PhyCDRLock(void)
{
	bool bPhyCdrLocked = false;

	bPhyCdrLocked = (HDMIRx_ReadRegU8(link_base + SYSTEM_CONTROL_2) & PHY_CDR_LOCKED) ? true : false;

	return bPhyCdrLocked;
}

void HdmiRx_ISR_Enable(bool isEnable)
{
	LOG_TRACE1(isEnable);
	if (!isEnable) {
		// disable hdmi interrups mask
		HDMIRx_WriteRegU8(link_base + HDMI_INTERRUPT_MASK, 0);

		// disable hdmi audio interrupts mask
		HDMIRx_WriteRegU8(link_base + AUDIO_INTERRUPT_MASK, 0);

		// disable hdmi audio error interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_MASK, AUDIO_ERROR_INT_REQ_MASK, 0);

		// disable packet diff 3 interrups mask
		HDMIRx_WriteRegMaskU8(link_base + PACKET_DIFF_INTERRUPT_MASK_3, PACKET_DIFF_INT_REQ_MASK_3, 0);

		// disable packet diff 4 interrups mask
		HDMIRx_WriteRegMaskU8(link_base + PACKET_DIFF_INTERRUPT_MASK_4, PACKET_DIFF_INT_REQ_MASK_4, 0);

		// disable phy interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + PHY_INTERRUPT_MASK, PHY_MSK_FREQ_DET, 0);

		// HDCP22 BCH error interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_MASK, HDCP22_BCH_INT_REQ_MASK, 0);
	} else {
		// enable hdmi interrups mask
		HDMIRx_WriteRegMaskU8(link_base + HDMI_INTERRUPT_MASK, HDMI_INT_REQ_MASK, HDMI_INT_REQ_MASK);

		// enable hdmi audio interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_INTERRUPT_MASK, AUDIO_INT_REQ_MASK, AUDIO_INT_REQ_MASK);

		// enable hdmi audio error interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_MASK, AUDIO_ERROR_INT_REQ_MASK, AUDIO_ERROR_INT_REQ_MASK);

		// enable packet diff 3 interrups mask
		HDMIRx_WriteRegMaskU8(link_base + PACKET_DIFF_INTERRUPT_MASK_3, PACKET_DIFF_INT_REQ_MASK_3, PACKET_DIFF_INT_REQ_MASK_3);

		// enable packet diff 4 interrups mask
		HDMIRx_WriteRegMaskU8(link_base + PACKET_DIFF_INTERRUPT_MASK_4, PACKET_DIFF_INT_REQ_MASK_4, PACKET_DIFF_INT_REQ_MASK_4);

		// enable packet vsi avrived interrups mask
		HDMIRx_WriteRegMaskU8(link_base + PACKET_ARV_INTERRUPT_MASK, PKT_ARV_MSK_VSI, PKT_ARV_MSK_VSI);

		// enable phy interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + PHY_INTERRUPT_MASK, PHY_MSK_FREQ_DET, PHY_MSK_FREQ_DET);

		// HDCP22 BCH error interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_MASK, HDCP22_BCH_INT_REQ_MASK, HDCP22_BCH_INT_REQ_MASK);
	}
}

void HdmiRx_StateMachine_Sleep_PreAction(void)
{
	// Clear VRR mode
	HDMIRx_WriteRegMaskU8(link_base + NEW_FORMAT_DET_30, ALLOW_VRR_MODE, 0);

	// reset DDC and HDCP block
	//hdmirx_inf("Set SYSTEM_PD_HDCP SYSTEM_PD_DDC reset!\n");
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_PIXEL_CONVERSION_CONTROL, SYSTEM_PD_HDCP | SYSTEM_PD_DDC | AUTO_PC_ON,
							SYSTEM_PD_HDCP | SYSTEM_PD_DDC | AUTO_PC_ON);

	// disable interrupts
	HdmiRx_ISR_Enable(false);

	// disable AEC
	HdmiRx_AEC_Enable(false);
}

void HdmiRx_StateMachine_Idle_PreAction(void)
{
	// clear reset DDC and HDCP block
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_PIXEL_CONVERSION_CONTROL, SYSTEM_PD_HDCP | SYSTEM_PD_DDC, 0);

	// reset IDCLK and TMDS CLK
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_1, SYSTEM_PD_TMDS, SYSTEM_PD_TMDS);

	// enable AEC
	HdmiRx_AEC_Enable(true);
}

void HdmiRx_StateMachine_Active_PreAction(void)
{
	// Enable interrupts
	HdmiRx_ISR_Enable(true);
}

void HdmiRx_StateMachine_Running_PreAction(void)
{
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_1, SYSTEM_PD_TMDS, 0);

	HdmiRx_Toggle_PD_IDCLK_Reset();
}

void HdmiRx_SetARCEnable(bool bOn)
{
	LOG_TRACE1(bOn);
	HDMIRx_WriteRegMaskU8(vs_base + ARC_CONTROL, ARC_EN, bOn ? ARC_EN : 0);
}

void HdmiRx_SetARCTXPath(ARCTXPath_e arc_tx_path)
{
	LOG_TRACE();
	//HDMIRx_WriteRegMaskU8(HDMIRX_ARC_TX_PATH_ADDR, SPDIF_MUX_SEL, arc_tx_path);
}

void HdmiRx_Reset_HDCP_DDC(void)
{
	// reset DDC and HDCP block, please make sure don't effect hdcp process.
	// It must reset when switch source and hotplug out
	LOG_TRACE();
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_PIXEL_CONVERSION_CONTROL, SYSTEM_PD_HDCP | SYSTEM_PD_DDC, SYSTEM_PD_HDCP | SYSTEM_PD_DDC);
	HDMIRx_WriteRegMaskU8(link_base + VIDEO_PIXEL_CONVERSION_CONTROL, SYSTEM_PD_HDCP | SYSTEM_PD_DDC, 0);
}

// MV_LIB EQ
void HdmiRx_MV_LIB_EQ_Init(void)
{
	LOG_TRACE();
	// MV TOP
	HDMIRx_WriteRegU8(vs_base + MV_TOP_BASE_ADDR + 0x17, 0x20);
	HDMIRx_WriteRegU8(vs_base + MV_TOP_BASE_ADDR + 0x1a, 0x20);
	HDMIRx_WriteRegU8(vs_base + MV_TOP_BASE_ADDR + 0x1c, 0x15);

	// MV LIB Top
	HDMIRx_WriteRegMaskU8(vs_base + LIB_MV_TOP_BASE_ADDR + MVPORT_MULTIVIEW_MUX_CONTROL, MVPORT_EN_INST_0_INPUT, MVPORT_EN_INST_0_INPUT);
	HDMIRx_WriteRegMaskU8(vs_base + LIB_MV_TOP_BASE_ADDR + MVPORT_MULTIVIEW_MUX_CONTROL, MVPORT_EN_INST_0_OUTPUT, MVPORT_EN_INST_0_OUTPUT);
	HDMIRx_WriteRegMaskU8(vs_base + LIB_MV_TOP_BASE_ADDR + MVTOP_ICG_CONTROL_0, EN_PHY_CLK, EN_PHY_CLK);
	HDMIRx_WriteRegMaskU8(vs_base + LIB_MV_TOP_BASE_ADDR + MVTOP_MUX42_MASTER_SELECT, USE_MASTER_SELECT_1 | USE_MASTER_SELECT_0,
						USE_MASTER_SELECT_1 | USE_MASTER_SELECT_0);
	HDMIRx_WriteRegU8(vs_base + LIB_MV_TOP_BASE_ADDR + 0x15, 0xff);
	HDMIRx_WriteRegU8(vs_base + LIB_MV_TOP_BASE_ADDR + 0x16, 0xff);
	HDMIRx_WriteRegU8(vs_base + LIB_MV_TOP_BASE_ADDR + 0x17, 0xff);
}

// EQ
void HdmiRx_EQ_Init(void)
{
	LOG_TRACE();
	U8 i;
	// reset
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, CPURST_N_RESET, CPURST_N_RESET);
	LOOP_DELAY(6);

	// init AEC block
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB5, AEC_EN | AEC_MODE | AEC_CYCLE_LENGTH, AEC_MODE | AEC_CYCLE_LENGTH);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB4, AEC_USE_AVRGPH | AEC_EDGE_ZERO_REQ | AEC_INSYNC_REQ | AEC_SETTING_SHORT,
						AEC_USE_AVRGPH | AEC_EDGE_ZERO_REQ | AEC_INSYNC_REQ | AEC_SETTING_SHORT);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CTRL, _BIT1_);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);

	// init HDMI1.4 AEC data to register fifo as default
	for (i = 0; i < AEC_DATA_CNT; i++) {
		HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB2, AEC_REGADR, (i * 4) << AEC_REGADR_SHIFT);
		HDMIRx_WriteRegU8(phy_base + CTRLD_PHY_RB3, AEC_DB_TV303[AEC_TAB_HDMI14][i] & 0xFF);
		HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB4, AEC_DATA, AEC_DB_TV303[AEC_TAB_HDMI14][i] >> 8);
		HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_WR_REG);
		LOOP_DELAY(16);

		HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
		LOOP_DELAY(16);
	}

	// init fixed EQ ...

	// init termination resistor
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RBB, INSYNC_TIMEOUT, INSYNC_TIMEOUT);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF2, RT_RCTL | RT_BYP, TERMINATION_RESISTOR_47);

	// select default channel and default AEC
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CHSEL, DEFAULT_AEC_CHSEL);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB2, AEC_REGADR, 4 << AEC_REGADR_SHIFT);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB5, AEC_EN, 0);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_LD_CH_SET);

	// reset termination resistor
	HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF2, RT_RESQ, 0);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLA_PHY_RF2, RT_RESQ, RT_RESQ);

	// enable AEC
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB5, AEC_EN, AEC_EN);

	// only reset the digital part of the PHY
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, RESQ_RESET, 0);
	LOOP_DELAY(16);
	HDMIRx_WriteRegMaskU8(phy_base + PHY_CONFIG_1, RESQ_RESET, RESQ_RESET);
}

void HdmiRx_AEC_Update(bool b6G)
{
	// see UpdateEQValue
	U32 index = b6G ? AEC_TAB_HDMI22 : AEC_TAB_HDMI14;
	U8 i;
	LOOP_DELAY(6);
	// disable AEC
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB5, AEC_EN, 0);
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);

	// update 1st AEC data
	for (i = 0; i < AEC_DATA_CNT; i++) {
		HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB2, AEC_REGADR, (i * 4) << AEC_REGADR_SHIFT);
		HDMIRx_WriteRegU8(link_base + CTRLD_PHY_RB3, AEC_DB_TV303[index][i] & 0xFF);
		HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB4, AEC_DATA, AEC_DB_TV303[index][i] >> 8);
		HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_WR_REG);
		LOOP_DELAY(16);

		HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
		LOOP_DELAY(16);
	}

	// enable AEC
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB5, AEC_EN, AEC_EN);
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);

	// manually triggers the AEC
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_TRIG_AEC);
}

void HdmiRx_AEC_Enable(bool bEnable)
{
	LOG_TRACE1(bEnable);
	// enable AEC
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB5, AEC_EN, bEnable ? AEC_EN : 0);
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);

	// manually triggers the AEC
	HDMIRx_WriteRegMaskU8(link_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_TRIG_AEC);
}

// update fixed EQ value
void HdmiRxFixedEQ_Update(U16 wEQ)
{
	LOG_TRACE1(wEQ);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB5, AEC_EN, 0x0);
	LOOP_DELAY(16);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB2, AEC_REGADR, (6 << AEC_REGADR_SHIFT));
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB3, 0xFF, wEQ);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB4, AEC_DATA, wEQ >> 8);  // high
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_WR_REG);
	LOOP_DELAY(16);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CHSEL, DEFAULT_AEC_CHSEL);  // use Register 0 for all channel
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB2, AEC_REGADR, 6 << AEC_REGADR_SHIFT);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_NOP);
	LOOP_DELAY(16);
	HDMIRx_WriteRegMaskU8(phy_base + CTRLD_PHY_RB1, AEC_CMD, AEC_CMD_LD_CH_SET);
	LOOP_DELAY(16);

	if ((HDMIRx_ReadRegU8(phy_base + 0x01ac) & 0x3C) == 0x18) {
		hdmirx_err("%s: All Channel  Write EQ (0x%x) success", __func__, wEQ);
	} else {
		hdmirx_err("%s: All Channel	Write EQ (0x%x) fail", __func__, wEQ);
	}
}

HDMIPortMap_t *HdmiRx_GetPortMap(void)
{
	return &HDMIPortMap[0];
}

U8 HDMIPortMapToMCU(U8 PortID)
{
	U8 uc = 0x0f;
	U8 i;

	for (i = 0; i < HDMI_PORT_NUM; i++) {
		if (HDMIPortMap[i].hdmi_port_id == PortID) {
			uc = HDMIPortMap[i].mcu_port_id;
			break;
		}
	}
	return uc;
}

// Display Module
U8 DisplayModule_GetPolarity(void)
{
	U8 ucVal;
	ucVal = ((HDMIRx_ReadRegU8(_PAGE_DATA_SYNC_ + 0xE7) & 0x80) >> 7);
	ucVal |= ((HDMIRx_ReadRegU8(_PAGE_DATA_SYNC_ + 0xDF) & 0x80) >> 6);
	return (ucVal & 0x03);
}

U8 DisplayModule_GetPhase(void)
{
	return ((HDMIRx_ReadRegU8(_PAGE_ADC_ + 0x80) & 0x7E) >> 1);
}

void DisplayModule_SetPhase(U8 dwPhase)
{
	HDMIRx_WriteRegMaskU8(_PAGE_ADC_ + 0x80, 0x7E, dwPhase << 1);
}

void DisplayModule_SetPLL(U32 dwPLL)
{
	HDMIRx_WriteRegMaskU8(_PAGE_ADC_ +  0x7C, 0xFE, (U8)(dwPLL << 1));
	HDMIRx_WriteRegU8(_PAGE_ADC_ +  0x7B, (U8)((dwPLL) >> 7));
	//HDMIRx_WriteRegMaskU32(_PAGE_ADC_ + 0x75, _BIT5_, (TRUE == TRUE) ? _BIT5_ : ~_BIT5_);
	mdelay(5);
	//HDMIRx_WriteRegMaskU32(_PAGE_ADC_ + 0xB5, _BIT5_, (FALSE == TRUE) ? _BIT5_ : ~_BIT5_);
}

U8 DisplayModule_GetAllPort_5VState(void)
{
	return HDMIRx_ReadRegU8(link_base + HDMI_5V_STATUS_REG) & 0xF;
}

// packet
// packet
bool HdmiRx_Packet_Read(eHdmiRxPacketType type, ThdmiRxPacket *pPacket)
{
	U8 i = 0, len = 0;
	char buf[128];
	char *p = buf;
	HDMIRx_WriteRegMaskU8(link_base + GENERAL_PACKET_READOUT_SELECT, GEN0_PACKET_SELECT, type);

	// read header byte
	for (i = 0; i < HB_SIZE; i++) {
		pPacket->HeaderByte[i] = HDMIRx_ReadRegU8(link_base + GENERAL_PACKET_READOUT_HB0 + i);
	}

	// dump header byte
	PACK_INF("------------------------\n");
	PACK_INF("Packet Type = 0x%x\n", pPacket->HeaderByte[0]);
	PACK_INF("Version = %d\n", pPacket->HeaderByte[1]);
	PACK_INF("Length = %d\n", pPacket->HeaderByte[2]);

	// check packet length
	len = pPacket->HeaderByte[2] + 1;
	if (len > PB_SIZE) {
		hdmirx_err("Invalid packet length!\n");
		return false;
	}

	p += sprintf(p, "%s", "data");
	// read packet byte
	for (i = 0; i < len; i++) {
		pPacket->PacketByte[i] = HDMIRx_ReadRegU8(link_base + GENERAL_PACKET_READOUT_PB0 + i);
		p += sprintf(p, " %02x", pPacket->PacketByte[i]);
	}
	PACK_INF("%s\n", buf);
	PACK_INF("------------------------\n");
	return true;
}

bool HdmiRx_Packet_ReadGen2(eHdmiRxPacketType type, ThdmiRxPacket *pPacket)
{
	CHECK_POINTER(pPacket, false);

	U8 i = 0, len = 0;
	HDMIRx_WriteRegMaskU8(link_base + GENERAL_PACKET2_SELECT, GEN2_PACKET_SELECT, type);

	// read header byte
	for (i = 0; i < HB_SIZE; i++) {
		pPacket->HeaderByte[i] = HDMIRx_ReadRegU8(link_base + GENERAL_PACKET2_READOUT_HB0 + i);
	}
	/*
	// dump header byte
	hdmirx_inf("------------------------");
	hdmirx_inf("Packet Type = 0x%x", pPacket->HeaderByte[0]);
	hdmirx_inf("Version = %d", pPacket->HeaderByte[1]);
	hdmirx_inf("Length = %d", pPacket->HeaderByte[2]);
	hdmirx_inf("------------------------");
	*/
	// check packet length
	len = pPacket->HeaderByte[2] + 1;
	if (len > PB_SIZE) {
		hdmirx_err("Invalid packet length!\n");
		return false;
	}

	// read packet byte
	for (i = 0; i < len; i++) {
		pPacket->PacketByte[i] = HDMIRx_ReadRegU8(link_base + GENERAL_PACKET2_READOUT_PB0 + i);
	}
	return true;
}

bool HdmiRx_Packet_ReadGen1(eHdmiRxPacketType type, ThdmiRxPacket *pPacket)
{
	CHECK_POINTER(pPacket, false);

	U8 i = 0, len = 0;

	if (type != HDMIRx_ReadRegU8(link_base + GENERAL_PACKET1_SELECT)) {
		HDMIRx_WriteRegMaskU8(link_base + GENERAL_PACKET1_SELECT, GEN1_PACKET_SELECT, type);
	}

	// read header byte
	for (i = 0; i < HB_SIZE; i++) {
		pPacket->HeaderByte[i] = HDMIRx_ReadRegU8(link_base + GENERAL_PACKET1_READOUT_HB0 + i);
	}

	/*
	// dump header byte
	hdmirx_inf("------------------------");
	hdmirx_inf("Packet Type = 0x%x", pPacket->HeaderByte[0]);
	hdmirx_inf("Version = %d", pPacket->HeaderByte[1]);
	hdmirx_inf("Length = %d", pPacket->HeaderByte[2]);
	hdmirx_inf("------------------------");
	*/

	// check packet length
	len = pPacket->HeaderByte[2] + 1;
	if (len > PB_SIZE) {
		hdmirx_err("Invalid packet length!\n");
		return false;
	}

	// read packet byte
	for (i = 0; i < len; i++) {
		pPacket->PacketByte[i] = HDMIRx_ReadRegU8(link_base + GENERAL_PACKET1_READOUT_PB0 + i);
	}

	return true;
}

static void HdmiRx_SendISREvent(eHdmiRxMsgID msg, HDMIRxEvent_t *pEvent)
{
	EVENT_INF("%s: eHdmiRxMsgID msg : %x, pEvent type : %d PortID : %d\n", __func__, msg, pEvent->type, pEvent->PortID);
	pEvent->param = msg;
	HDMIRx_Event_SendEvent(*pEvent);
}

static void HdmiRx_ScanISR_PHY(HDMIRxEvent_t *pEvent)
{
	U8 val = 0;
	val = HDMIRx_ReadRegU8(link_base + PHY_INTERRUPT_REASON);
	HDMIRx_WriteRegU8(link_base + PHY_INTERRUPT_REASON, val);
	// Aksv written by source interrupt
	if (val & PHY_INT_FREQ_DET) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PHY_INT_FREQ_DET, pEvent);
		// disable phy interrupts mask
		HDMIRx_WriteRegMaskU8(link_base + PHY_INTERRUPT_MASK, PHY_MSK_FREQ_DET, 0);
	}
}

static void HdmiRx_ScanISR_HDMI(HDMIRxEvent_t *pEvent)
{
	U8 val = 0;
	val = HDMIRx_ReadRegU8(link_base + HDMI_INTERRUPT_REASON);
	HDMIRx_WriteRegU8(link_base + HDMI_INTERRUPT_REASON, val);

	if (!(HDMIRx_ReadRegU8(link_base + NEW_FORMAT_DET_30) & ALLOW_VRR_MODE)) {
		// PHY_UNLOCKED
		if (val & HDMI_REASON_PHY_UNLOCKED) {
			// HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PHY_UNLOCKED,pEvent,pISREventHandle);
		}
	}

	// DVI_LOCKED
	if (val & HDMI_REASON_DVI_LOCKED) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_DVI_LOCKED, pEvent);
	}

	// HDMI_LOCKED
	if (val & HDMI_REASON_HDMI_LOCKED) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_HDMI_LOCKED, pEvent);
	}

	// PACKET_LOCKED
	if (val & HDMI_REASON_PKT_LOCKED) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_LOCKED, pEvent);
	}

	// if(bSX8B){
	// VTOTAL_CHANGED
	if (val & HDMI_REASON_VTOTAL_CHANGED) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_VTOTAL_CHANGED, pEvent);
	}
}

// detect & dispatch all packet interrupts
static void HdmiRx_ScanISR_Packet(HDMIRxEvent_t *pEvent)
{
	U8 val;

	// diff. interrupts reason 1
	val = HDMIRx_ReadRegU8(link_base + PACKET_DIFF_INTERRUPT_REASON_1);
	HDMIRx_WriteRegU8(link_base + PACKET_DIFF_INTERRUPT_REASON_1, val);

	// GCP Packet
	if (val & PKT_DIFF_INT_GCP) {
		// HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_GCP,pEvent,pISREventHandle);
	}

	// VSI_LLC Packet
	if (val & PKT_DIFF_INT_VSI_LLC) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_VSI_LLC, pEvent);
	}

	// GP2 diff
	if (val & PKT_DIFF_INT_GP2) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_GP2, pEvent);
	}

	// diff. interrupts reason 2
	val = HDMIRx_ReadRegU8(link_base + PACKET_DIFF_INTERRUPT_REASON_2);
	HDMIRx_WriteRegU8(link_base + PACKET_DIFF_INTERRUPT_REASON_2, val);

	// VSI Packet
	if (val & PKT_DIFF_INT_VSI) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_VSI_LLC, pEvent);
	}

	// SPD Infoframe
	if (val & PKT_DIFF_INT_SPDI) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_SPDI, pEvent);
	}

	// AVI Infoframe
	if (val & PKT_DIFF_INT_AVI) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_AVI, pEvent);
	}

	// Audio Infoframe
	if (val & PKT_DIFF_INT_AI) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_AI, pEvent);
	}

	// Audio Clock Regeneration
	if (val & PKT_DIFF_INT_ACR) {
		// HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_ACR,pEvent,pISREventHandle);
	}

	// Audio Content Protection
	if (val & PKT_DIFF_INT_ACP) {
		// HdmiRx_SendISREvent(MSG_HDMIRX_INTR_PKT_DIFF_ACP,pEvent,pISREventHandle);
	}
}

static void HdmiRx_ScanISR_Audio(HDMIRxEvent_t *pEvent)
{
	U8 val = 0;
	val = HDMIRx_ReadRegU8(link_base + AUDIO_INTERRUPT_REASON);
	HDMIRx_WriteRegU8(link_base + AUDIO_INTERRUPT_REASON, val);

	// Channel status bit change
	if (val & AD_INT_CS_DIFF) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_AUDIO_CS_DIFF, pEvent);
	}

	// Audio FIFO underrun
	if (val & AD_INT_FIFO_UNDERRUN) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_AUDIO_FIFO_UNDERRUN, pEvent);
	}

	// Audio FIFO overrun
	if (val & AD_INT_FIFO_OVERRUN) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_AUDIO_FIFO_OVERRUN, pEvent);
	}

	// Audio locked
	if (val & AD_INT_PLL_LOCKED) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_AUDIO_PLL_LOCKED, pEvent);
	}
}

static void HdmiRx_ScanISR_AudioError(HDMIRxEvent_t *pEvent)
{
	U8 val;
	val = HDMIRx_ReadRegU8(link_base + AUDIO_ERROR_INTERRUPT_REASON);
	HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_REASON, AUDIO_ERROR_INT_REQ_MASK, val);

	// Audio Check Sum error
	if (val & ADERR_INT_CHECK_SUM) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_AUDIO_CHECKSUM_ERROR, pEvent);
	}

	// Audio Status Bit error
	if (val & ADERR_INT_STATUS_BIT) {
		HdmiRx_SendISREvent(MSG_HDMIRX_INTR_AUDIO_STATUS_BIT_ERROR, pEvent);
	}
}

static void HdmiRx_ScanISR_HDCP22_BCHError(HDMIRxEvent_t *pEvent)
{
	U8 val;
	val = HDMIRx_ReadRegU8(link_base + AUDIO_ERROR_INTERRUPT_REASON);
	HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_REASON, HDCP22_BCH_INT_REQ_MASK, val);

	// BCH50 error
	if (val & ADERR_MSK_BCH_HDCP22) {
		HdmiRx_SendISREvent(MSG_HDCPRX_INTR_HDCP22_BCH50_ERROR, pEvent);
	}
}

static void HdmiRx_ScanISR_HDCP14_BCHError(HDMIRxEvent_t *pEvent)
{
	U8 val;
	val = HDMIRx_ReadRegU8(link_base + HDCP22_UPDATE_KEY);
	if (val & HDCP22_AUTHENTICATED) { // if HDCP22
		return;
	}

	val = HDMIRx_ReadRegU8(link_base + AUDIO_ERROR_INTERRUPT_REASON);
	HDMIRx_WriteRegMaskU8(link_base + AUDIO_ERROR_INTERRUPT_REASON, ADERR_MSK_BCH, val);

	// BCH error
	if (val & ADERR_MSK_BCH) {
		HdmiRx_SendISREvent(MSG_HDCPRX_INTR_HDCP14_BCH_ERROR, pEvent);
	}
}

void HdmiRx_ScanISR(U8 PortID)
{
	U8 data = 0;
	HDMIRxEvent_t event;
	event.PortID = PortID;
	event.type = HDMIRxEventType_ISREvent;
	data = HDMIRx_ReadRegU8(link_base + INTERRUPT_REASON_REGISTER);

	// PHY interrupts
	if (data & PHY_INT) {
		HdmiRx_ScanISR_PHY(&event);
	}

	// HDMI interrupts
	if (data & HDMI_INT) {
		HdmiRx_ScanISR_HDMI(&event);
	}

	// Packet Received interrupts
	if (data & PKT_RCVD_INT) {
		HdmiRx_ScanISR_Packet(&event);
	}

	// Audio Interrupts
	if (data & AUDIO_INT) {
		HdmiRx_ScanISR_Audio(&event);
	}

	// Audio Error Interrupts
	if (data & AUDIO_ERR_INT) {
		HdmiRx_ScanISR_AudioError(&event);
		HdmiRx_ScanISR_HDCP22_BCHError(&event);
		HdmiRx_ScanISR_HDCP14_BCHError(&event);
	}
}
