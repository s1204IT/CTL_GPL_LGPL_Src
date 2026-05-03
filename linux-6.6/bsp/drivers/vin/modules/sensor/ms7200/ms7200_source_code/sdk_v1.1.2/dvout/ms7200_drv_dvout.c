/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file	ms7200_drv_misc.c
* @author
* @version V1.0.0
* @date	15-Nov-2016
* @brief   misc module driver source file
* @history
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include "ms7200_comm.h"
#include "ms7200_mpi.h"
#include "ms7200_drv_dvout.h"

/* DVOUT register address map */
#define MS7200_DVOUT_REG_BASE				(0x0000)
#define REG_RST_CTRL0						(MS7200_DVOUT_REG_BASE + 0x09)
#define REG_TEST_SEL						(MS7200_DVOUT_REG_BASE + 0x16)
#define REG_DIG_CLK_SEL0					(MS7200_DVOUT_REG_BASE + 0xC0)
#define REG_DIG_CLK_SEL1					(MS7200_DVOUT_REG_BASE + 0xC1)
#define REG_DVO_CTRL_0						(MS7200_DVOUT_REG_BASE + 0xE0)
#define REG_DVO_CTRL_1						(MS7200_DVOUT_REG_BASE + 0xE1)
#define REG_DVO_CTRL_2						(MS7200_DVOUT_REG_BASE + 0xE2)
#define REG_DVO_CTRL_3						(MS7200_DVOUT_REG_BASE + 0xE3)
#define REG_DVO_CTRL_4						(MS7200_DVOUT_REG_BASE + 0xE4)
#define REG_DVO_CTRL_5						(MS7200_DVOUT_REG_BASE + 0xE5)

/* PA register address map */
#define MS7200_PA_REG_BASE					(0x1280)
#define REG_PA_S							(MS7200_PA_REG_BASE + 0x01)


VOID ms7200drv_dvout_mode_config(DVOUT_CONFIG_T *t_dvout_config)
{
	UINT8 u8_clk_ratio = 0;

	// dvo_yuv_out
	HAL_ToggleBits(REG_DVO_CTRL_1, MSRT_BIT4, t_dvout_config->u8_cs_mode != DVOUT_CS_MODE_RGB);
	// dvo_blk_clip_off
	HAL_ClrBits(REG_DVO_CTRL_1, MSRT_BIT5);

	// dvo_16b_en, dvo_8b_en
	switch (t_dvout_config->u8_cs_mode) {
	case DVOUT_CS_MODE_RGB:
	case DVOUT_CS_MODE_YUV444:
		HAL_ClrBits(REG_DVO_CTRL_2, MSRT_BITS2_1);   //24 bit
		if (t_dvout_config->u8_bw_mode == DVOUT_BW_MODE_8_12BIT) {
			// dvin_12b444
			HAL_ToggleBits(REG_DVO_CTRL_5, MSRT_BIT1, !(u8_clk_ratio & MSRT_BIT3));
			u8_clk_ratio = MSRT_BIT2;   //clk x2
		}
		break;
	case DVOUT_CS_MODE_YUV422:
		switch (t_dvout_config->u8_bw_mode) {
		case DVOUT_BW_MODE_16_24BIT:
			HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS2_1, MSRT_BIT1);   //16 bit
			HAL_WriteByte(REG_DVO_CTRL_3, 0x26);
			break;
		case DVOUT_BW_MODE_8_12BIT:
			HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS2_1, MSRT_BIT2);   //8 bit
			HAL_WriteByte(REG_DVO_CTRL_3, 0x2A);
			HAL_SetBits(REG_DVO_CTRL_4, MSRT_BIT4);   //dvo_yuv444to422_sel_div2
			u8_clk_ratio = MSRT_BIT2;   //clk x2
			break;
		}
		break;
	}

	// dvo_ddr_mode_sel
	switch (t_dvout_config->u8_dr_mode) {
	case DVOUT_DR_MODE_SDR:
		//HAL_ClrBits(REG_DVO_CTRL_4, MSRT_BIT5);   //sdr
		break;
	case DVOUT_DR_MODE_DDR:
		//HAL_SetBits(REG_DVO_CTRL_4, MSRT_BIT5);   //ddr
		u8_clk_ratio |= MSRT_BIT3;   //clk div2
		break;
	}
	HAL_ModBits(REG_DIG_CLK_SEL0, MSRT_BITS3_2, u8_clk_ratio);

	// dvo_bt1120_en, dvo_bt656_en
	switch (t_dvout_config->u8_sy_mode) {
	case DVOUT_SY_MODE_HSVSDE:
	case DVOUT_SY_MODE_BTAT1004:
		HAL_ClrBits(REG_DVO_CTRL_2, MSRT_BITS4_3);
		break;
	case DVOUT_SY_MODE_EMBANDED:
		HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS4_3, MSRT_BIT3);
		break;
	case DVOUT_SY_MODE_2XEMBANDED:
		HAL_ModBits(REG_DVO_CTRL_2, MSRT_BITS4_3, MSRT_BIT4);
		break;
	}
	// r_16b_1004_en
	HAL_ToggleBits(REG_DVO_CTRL_5, MSRT_BIT0, t_dvout_config->u8_sy_mode == DVOUT_SY_MODE_BTAT1004);
}

VOID ms7200drv_dvout_clk_reset_release(BOOL b_release)
{
	if (b_release) {
		HAL_SetBits(REG_TEST_SEL, MSRT_BIT1);
		HAL_SetBits(REG_RST_CTRL0, MSRT_BIT1);
	} else {
		HAL_ClrBits(REG_RST_CTRL0, MSRT_BIT1);
		HAL_ClrBits(REG_TEST_SEL, MSRT_BIT1);
	}
	HAL_ToggleBits(REG_DVO_CTRL_2, MSRT_BIT0, b_release);
}

VOID ms7200drv_dvout_data_swap_all(VOID)
{
	UINT8 u8_swap = HAL_ReadByte(REG_DVO_CTRL_2) & MSRT_BIT7;
	HAL_ToggleBits(REG_DVO_CTRL_2, MSRT_BIT7, !u8_swap);
}

VOID ms7200drv_dvout_data_swap_rb_channel(VOID)
{
	UINT8 u8_map = HAL_ReadByte(REG_DVO_CTRL_3) & MSRT_BITS5_0;
	HAL_ModBits(REG_DVO_CTRL_3, MSRT_BITS5_0, ((u8_map & MSRT_BITS1_0) << 4) | (u8_map & MSRT_BITS3_2) | ((u8_map & MSRT_BITS5_4) >> 4));
}

VOID ms7200drv_dvout_data_swap_yc_channel(VOID)
{
	UINT8 u8_map = HAL_ReadByte(REG_DVO_CTRL_3) & MSRT_BITS5_0;
	HAL_ModBits(REG_DVO_CTRL_3, MSRT_BITS5_0, ((u8_map & MSRT_BITS3_2) << 2) | ((u8_map & MSRT_BITS1_0) << 2) | ((u8_map & MSRT_BITS3_2) >> 2));
}

VOID ms7200drv_dvout_data_swap_16bit_all_y(VOID)
{
	UINT8 u8_map = HAL_ReadByte(REG_DVO_CTRL_3) & MSRT_BITS5_0;
	HAL_ModBits(REG_DVO_CTRL_3, MSRT_BITS5_0, 0x15);
	Delay_ms(100);
	HAL_ModBits(REG_DVO_CTRL_3, MSRT_BITS5_0, u8_map);
}

VOID ms7200drv_dvout_pa_adjust(BOOL b_invert, UINT8 u8_delay)
{
	HAL_ModBits(REG_PA_S, MSRT_BITS2_0, ((UINT8)b_invert << 2) | (u8_delay % 4));
}

VOID ms7200drv_dvout_clk_sel(UINT8 u8_clk_rpt, UINT16 u16_video_clk)
{
	HAL_ModBits(REG_DIG_CLK_SEL0, MSRT_BITS7_4, u8_clk_rpt << 4);
	HAL_ModBits(REG_DIG_CLK_SEL1, MSRT_BITS3_0, ((u16_video_clk <= 5000) ? 2 : 1) * (u8_clk_rpt + 1) - 1);
}
