/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs HRC Driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _SUNXI_HRC_REG_H_
#define _SUNXI_HRC_REG_H_

#include <linux/bitops.h>

#define HRC_TOP_REG             0x0000
#define HRC_CTRL_REG            0x0004
#define HRC_COUNT_REG           0x0008
#define HRC_READY_REG           0x000C
#define HRC_INT_CTRL_REG        0x0010
#define HRC_INT_STAT_REG        0x0014
#define HRC_FMT_REG             0x0018
#define HRC_SIZE_REG            0x001C
#define HRC_IN_Y_BUF_ADDR_REG   0x0030
#define HRC_IN_C_BUF_ADDR_REG   0x0034
#define HRC_OUT_Y_BUF_LADDR_REG 0x0038
#define HRC_OUT_Y_BUF_HADDR_REG 0x003C
#define HRC_OUT_C_BUF_LADDR_REG 0x0040
#define HRC_OUT_C_BUF_HADDR_REG 0x0044
#define HRC_OUT_BUF_STRIDE_REG  0x0048
#define HRC_RSC_REG             0x0050
#define HRC_CSC_D0_REG          0x0054
#define HRC_CSC_D1_REG          0x0058
#define HRC_CSC_D2_REG          0x005C
#define HRC_CSC_DX_REG(x)       (HRC_CSC_D0_REG + (4 * (x)))
#define HRC_CSC_COEFF_00_REG    0x0060
#define HRC_CSC_COEFF_01_REG    0x0064
#define HRC_CSC_COEFF_02_REG    0x0068
#define HRC_CSC_CONSTANT_03_REG 0x006C
#define HRC_CSC_COEFF_10_REG    0x0070
#define HRC_CSC_COEFF_11_REG    0x0074
#define HRC_CSC_COEFF_12_REG    0x0078
#define HRC_CSC_CONSTANT_13_REG 0x007C
#define HRC_CSC_COEFF_20_REG    0x0080
#define HRC_CSC_COEFF_21_REG    0x0084
#define HRC_CSC_COEFF_22_REG    0x0088
#define HRC_CSC_CONSTANT_23_REG 0x008C
#define HRC_CSC_COEFF_XX_REG(x) (HRC_CSC_COEFF_00_REG + (4 * (x)))
#define HRC_SCALER_Y_SIZE_REG   0x0090
#define HRC_SCALER_Y_HSTEP_REG  0x0094
#define HRC_SCALER_Y_HPHASE_REG 0x00A0
#define HRC_SCALER_C_SIZE_REG   0x00B0
#define HRC_SCALER_C_HSTEP_REG  0x00B4
#define HRC_SCALER_C_HPHASE_REG 0x00C0
#define HRC_VSU_Y_COEFF_0_REG   0x0100
#define HRC_VSU_Y_COEFF_1_REG   0x0200
#define HRC_VSU_C_COEFF_0_REG   0x0400
#define HRC_VSU_C_COEFF_1_REG   0x0500

enum {
	/* HRC_TOP_REG */
	TOP_RESET_MASK            = BIT(28),
	MBUS_CLK_EN_MASK          = BIT(24),
	SRAM_GATE_MODE_MASK       = BIT(4),
	SELF_VSYN_START_MASK      = BIT(0),

	/* HRC_CTRL_REG */
	C_HSCALER_EN_MASK         = BIT(25),
	Y_HSCALER_EN_MASK         = BIT(24),
	C_VSAMPLE_EN_MASK         = BIT(20),
	CSC_EN_MASK               = BIT(16),
	FIELD_ORDER_MASK          = BIT(14),
	FIELD_ORDER_BOTTOM        = 0,
	FIELD_ORDER_TOP           = 1,
	FIELD_INVERSE_MASK        = BIT(13),
	FIELD_MODE_MASK           = BIT(12),
	FIELD_MODE_FRAME          = 0,
	FIELD_MODE_FIELD          = 1,
	DATA_SRC_SEL_MASK         = BIT(8),
	DATA_SRC_HDMI_RX          = 0,
	DATA_SRC_DDR              = 1,
	HRC_EN_MASK               = BIT(0),

	/* HRC_COUNT_REG */
	SELF_SYN_LEN_MASK         = GENMASK(31, 16),
	RESET_CYCLE_MASK          = GENMASK(15, 0),

	/* HRC_READY_REG */
	TIMEOUT_CYCLE_MASK        = GENMASK(31, 16),
	REG_RDY_MASK              = BIT(0),

	/* HRC_INT_CTRL_REG */
	UNUSUAL_IRQ_EN_MASK       = BIT(20),
	OVERFLOW_IRQ_EN_MASK      = BIT(16),
	TIMEOUT_IRQ_EN_MASK       = BIT(12),
	WB_FINISH_IRQ_EN_MASK     = BIT(8),
	CFG_FINISH_IRQ_EN_MASK    = BIT(4),
	FRM_IRQ_EN_MASK           = BIT(0),

	/* HRC_INT_STAT_REG */
	FIELD_FLAG_MASK           = BIT(24),
	UNUSUAL_IRQ_MASK          = BIT(20),
	OVERFLOW_IRQ_MASK         = BIT(16),
	TIMEOUT_IRQ_MASK          = BIT(12),
	WB_FINISH_IRQ_MASK        = BIT(8),
	CFG_FINISH_IRQ_MASK       = BIT(4),
	FRM_IRQ_MASK              = BIT(0),

	/* HRC_FMT_REG */
	OUT_UNCOMPACT_MASK        = BIT(16),
	OUT_COMPACT               = 0,
	OUT_UNCOMPACT             = 1,
	BIT_DEPTH_MASK            = GENMASK(9, 8),
	DEPTH_8                   = 0,
	DEPTH_10                  = 1,
	DEPTH_12                  = 2,
	OUTPUT_FORMAT_MASK        = GENMASK(5, 4),
	INPUT_FORMAT_MASK         = GENMASK(1, 0),
	FORMAT_RGB                = 0,
	FORMAT_YUV444             = 1,
	FORMAT_YUV422             = 2,
	FORMAT_YUV420             = 3,

	/* HRC_SIZE_REG */
	SRC_HEIGHT_MASK           = GENMASK(27, 16),
	SRC_WIDTH_MASK            = GENMASK(11, 0),

	/* HRC_IN_Y_BUF_ADDR_REG */
	/* HRC_IN_C_BUF_ADDR_REG */

	/* HRC_OUT_Y_BUF_LADDR_REG */
	/* HRC_OUT_Y_BUF_HADDR_REG */
	OUTPUT_Y_BUF_HADDR_MASK   = GENMASK(7, 0),

	/* HRC_OUT_C_BUF_LADDR_REG */
	/* HRC_OUT_C_BUF_HADDR_REG */
	OUTPUT_C_BUF_HADDR_MASK   = GENMASK(7, 0),

	/* HRC_OUT_BUF_STRIDE_REG */
	OUTPUT_C_STRIDE_MASK      = GENMASK(31, 16),
	OUTPUT_Y_STRIDE_MASK      = GENMASK(15, 0),

	/* HRC_RSC_REG */
	RSC_SEED_MASK             = GENMASK(31, 8),
	RSC_EN_MASK               = BIT(0),

	/* HRC_CSC_D0_REG */
	/* HRC_CSC_D1_REG */
	/* HRC_CSC_D2_REG */
	/* X: 0-2 */
	CSC_DX_MASK               = GENMASK(10, 0),

	/* HRC_CSC_COEFF_00_REG, HRC_CSC_COEFF_10_REG, HRC_CSC_COEFF_20_REG */
	/* HRC_CSC_COEFF_01_REG, HRC_CSC_COEFF_11_REG, HRC_CSC_COEFF_21_REG */
	/* HRC_CSC_COEFF_02_REG, HRC_CSC_COEFF_12_REG, HRC_CSC_COEFF_22_REG */
	/* X: 0-2 */
	CSC_CXX_MASK              = GENMASK(16, 0),
	/* HRC_CSC_CONSTANT_03_REG, HRC_CSC_CONSTANT_13_REG, HRC_CSC_CONSTANT_23_REG */
	CSC_CX3_MASK              = GENMASK(10, 0),

	/* HRC_SCALER_Y_SIZE_REG */
	SCALER_Y_OUT_HEIGHT_MASK  = GENMASK(28, 16),
	SCALER_Y_OUT_WIDTH_MASK   = GENMASK(12, 0),

	/* HRC_SCALER_Y_HSTEP_REG */
	SCALER_Y_HSTEP_INT_MASK   = GENMASK(23, 20),
	SCALER_Y_HSTEP_FRAC_MASK  = GENMASK(19, 0),
	SCALER_Y_HSTEP_INT_OFFSET = 20,

	/* HRC_SCALER_Y_HPHASE_REG */
	SCALER_Y_HPHASE_INT_MASK  = GENMASK(23, 20),
	SCALER_Y_HPHASE_FRAC_MASK = GENMASK(19, 0),

	/* HRC_SCALER_C_SIZE_REG */
	SCALER_C_OUT_HEIGHT_MASK  = GENMASK(28, 16),
	SCALER_C_OUT_WIDTH_MASK   = GENMASK(12, 0),

	/* HRC_SCALER_C_HSTEP_REG */
	SCALER_C_HSTEP_INT_MASK   = GENMASK(23, 20),
	SCALER_C_HSTEP_FRAC_MASK  = GENMASK(19, 0),
	SCALER_C_HSTEP_INT_OFFSET = 20,

	/* HRC_SCALER_C_HPHASE_REG */
	SCALER_C_HPHASE_INT_MASK  = GENMASK(23, 20),
	SCALER_C_HPHASE_FRAC_MASK = GENMASK(19, 0),

	/* HRC_VSU_Y_COEFF_0_REG */
	/* HRC_VSU_Y_COEFF_1_REG */
	/* HRC_VSU_C_COEFF_0_REG */
	/* HRC_VSU_C_COEFF_1_REG */
};

#endif  /* _SUNXI_HRC_REG_H_ */
