/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file    ms7200_drv_misc.c
* @author
* @version V1.0.0
* @date    15-Nov-2016
* @brief   misc module driver source file
* @history
*
* Copyright (c) 2009 - 2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include "ms7200_comm.h"
#include "ms7200_mpi.h"
#include "ms7200_drv_misc.h"

/* TOP MISC register address map */
#define MS7200_MISC_REG_BASE                     (0x0000)
#define REG_CHIPID0_REG                          (MS7200_MISC_REG_BASE + 0x0000)
#define REG_CHIPID1_REG                          (MS7200_MISC_REG_BASE + 0x0001)
#define REG_CHIPID2_REG                          (MS7200_MISC_REG_BASE + 0x0002)
#define REG_BDOP_REG                             (MS7200_MISC_REG_BASE + 0x00a3)
#define REG_AUDIO_INPUT_OUTPUT_CNTRL             (MS7200_MISC_REG_BASE + 0x00c2)
#define REG_PINMUX                               (MS7200_MISC_REG_BASE + 0x00a5)
#define REG_MISC_DRIV                            (MS7200_MISC_REG_BASE + 0x00a8)
#define REG_MISC_TSTPAT0                         (MS7200_MISC_REG_BASE + 0x00b0)
#define REG_MISC_TSTPAT1                         (MS7200_MISC_REG_BASE + 0x00b1)
#define REG_MISC_TSTPAT8                         (MS7200_MISC_REG_BASE + 0x00b8)
#define REG_MISC_TSTPAT9                         (MS7200_MISC_REG_BASE + 0x00b9)
#define REG_MISC_TSTPAT10                        (MS7200_MISC_REG_BASE + 0x00ba)

/* CSC register address map */
#define MS7200_CSC_REG_BASE                      (0x00f0)
#define REG_CSC_CTRL1                            (MS7200_CSC_REG_BASE + 0x00)

#define MS7200_CHIP_ID1         (0x20)


UINT8 ms7200drv_misc_package_sel_get(VOID)
{
    UINT8 u8_package_sel = (HAL_ReadByte(REG_BDOP_REG) & MSRT_BITS3_2) >> 2;
    return u8_package_sel;
}

BOOL ms7200drv_misc_chipisvalid(VOID)
{
    UINT8 u8_chipid1;
    u8_chipid1 = HAL_ReadByte(REG_CHIPID1_REG);

    if (u8_chipid1 == MS7200_CHIP_ID1)
		return TRUE;

	return FALSE;
}

VOID ms7200drv_misc_audio_out_pad_enable(BOOL b_enable)
{
    HAL_ToggleBits(REG_AUDIO_INPUT_OUTPUT_CNTRL, MSRT_BIT2, b_enable);
}

VOID ms7200drv_misc_audio_pad_output_spdif(BOOL b_spdif)
{
    HAL_ToggleBits(REG_PINMUX, MSRT_BIT2 | MSRT_BIT0, b_spdif);
}

VOID ms7200drv_misc_audio_mclk_pad_enable(BOOL b_enable)
{
    HAL_ToggleBits(REG_AUDIO_INPUT_OUTPUT_CNTRL, MSRT_BIT4, b_enable);
}

VOID ms7200drv_misc_audio_mclk_div(UINT8 u8_div)
{
    HAL_ModBits(REG_AUDIO_INPUT_OUTPUT_CNTRL, MSRT_BITS7_6, u8_div << 6);
}

VOID ms7200drv_misc_vclk_driver_set(UINT8 u8_drv_sel)
{
    HAL_ModBits(REG_MISC_DRIV, MSRT_BITS1_0, u8_drv_sel);
}

VOID ms7200drv_csc_config_input(HDMI_CS_E e_cs)
{
    UINT8 u8_cs = 3;   //0:RGB; 1:YUV444; 2:YUV422; 3:bypass
    switch (e_cs) {
    case HDMI_RGB:
		u8_cs = 0;
		break;
    case HDMI_YCBCR444:
		u8_cs = 1;
		break;
    case HDMI_YCBCR422:
		u8_cs = 2;
		break;
    default:
		break;
    }
    HAL_ModBits(REG_CSC_CTRL1, MSRT_BITS1_0, u8_cs);
}

VOID ms7200drv_csc_config_output(DVOUT_CS_MODE_E e_cs)
{
    HAL_ModBits(REG_CSC_CTRL1, MSRT_BITS4_2, 0x10 | ((UINT8)e_cs << 2));
}

VOID ms7200drv_tp_pure_color_set(UINT8 u8_vr, UINT8 u8_yg, UINT8 u8_ub)
{
    HAL_WriteByte(REG_MISC_TSTPAT8, u8_yg);
    HAL_WriteByte(REG_MISC_TSTPAT9, u8_ub);
    HAL_WriteByte(REG_MISC_TSTPAT10, u8_vr);
    HAL_SetBits(REG_MISC_TSTPAT1, MSRT_BITS6_4);
}

VOID ms7200drv_tp_output_enable(BOOL b_enable)
{
    HAL_ToggleBits_Ex(REG_MISC_TSTPAT0, MSRT_BIT0, b_enable);
}

