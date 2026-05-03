// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "sunxi_hrc_log.h"
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "sunxi_hrc_platform.h"
#include "sunxi_hrc_reg.h"
#include "sunxi_hrc_table.h"
#include "sunxi_hrc_hardware.h"

struct hrc_private_data {
	const struct hrc_hw_desc *desc;
	void __iomem *reg_base;
	u32 irq_mask;
};

struct csc_info {
	enum hrc_color_space in_cs;
	enum hrc_quantization in_quan;
	enum hrc_color_space out_cs;
	enum hrc_quantization out_quan;
	enum csc_table_index index;
};

struct csc_info csc_info_tables[] = {
	{
		HRC_CS_BT601, HRC_QUANTIZATION_FULL,
		HRC_CS_BT601, HRC_QUANTIZATION_LIMIT,
		CSC_FULL_RGB_601_LIMIT_YUV_601
	}, {
		HRC_CS_BT709, HRC_QUANTIZATION_FULL,
		HRC_CS_BT709, HRC_QUANTIZATION_LIMIT,
		CSC_FULL_RGB_709_LIMIT_YUV_709
	}, {
		HRC_CS_BT2020, HRC_QUANTIZATION_FULL,
		HRC_CS_BT2020, HRC_QUANTIZATION_LIMIT,
		CSC_FULL_RGB_2020_LIMIT_YUV_2020
	}, {
		HRC_CS_BT601, HRC_QUANTIZATION_LIMIT,
		HRC_CS_BT601, HRC_QUANTIZATION_LIMIT,
		CSC_LIMIT_RGB_601_LIMIT_YUV_601
	}, {
		HRC_CS_BT709, HRC_QUANTIZATION_LIMIT,
		HRC_CS_BT709, HRC_QUANTIZATION_LIMIT,
		CSC_LIMIT_RGB_709_LIMIT_YUV_709
	}, {
		HRC_CS_BT2020, HRC_QUANTIZATION_LIMIT,
		HRC_CS_BT2020, HRC_QUANTIZATION_LIMIT,
		CSC_LIMIT_RGB_2020_LIMIT_YUV_2020
	}, {
		HRC_CS_BT601, HRC_QUANTIZATION_FULL,
		HRC_CS_BT601, HRC_QUANTIZATION_FULL,
		CSC_FULL_RGB_601_FULL_YUV_601
	}, {
		HRC_CS_BT709, HRC_QUANTIZATION_FULL,
		HRC_CS_BT709, HRC_QUANTIZATION_FULL,
		CSC_FULL_RGB_709_FULL_YUV_709
	}, {
		HRC_CS_BT2020, HRC_QUANTIZATION_FULL,
		HRC_CS_BT2020, HRC_QUANTIZATION_FULL,
		CSC_FULL_RGB_2020_FULL_YUV_2020
	},
};

u32 hrc_read(struct hrc_hw_handle *hrc_hdl, u32 reg)
{
	return readl(hrc_hdl->private->reg_base + reg);
}

void hrc_write(struct hrc_hw_handle *hrc_hdl, u32 reg, u32 val)
{
	writel(val, hrc_hdl->private->reg_base + reg);
}

u32 hrc_read_mask(struct hrc_hw_handle *hrc_hdl, u32 reg, u32 mask)
{
	u32 reg_val = readl(hrc_hdl->private->reg_base + reg);

	return (reg_val & mask) >> (ffs(mask) - 1);
}

void hrc_write_mask(struct hrc_hw_handle *hrc_hdl, u32 reg, u32 mask, u32 val)
{
	u32 reg_val = readl(hrc_hdl->private->reg_base + reg);

	reg_val &= ~(mask);
	reg_val |= val << (ffs(mask) - 1);
	writel(reg_val, hrc_hdl->private->reg_base + reg);
}

int hrc_reg_dump(struct hrc_hw_handle *hrc_hdl, char *buf, int n)
{
	int i, j;
	const struct hrc_hw_reg_range *reg_range = hrc_hdl->private->desc->reg_range;

	for (i = 0; i < HRC_HW_REG_RANGE_MAX_SIZE; i++) {
		for (j = reg_range[i].start; j < reg_range[i].end; j += 16) {
			if (!buf)
				hrc_inf("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
					j,
					hrc_read(hrc_hdl, j + 0),
					hrc_read(hrc_hdl, j + 4),
					hrc_read(hrc_hdl, j + 8),
					hrc_read(hrc_hdl, j + 12));
			else
				n += sprintf(buf + n, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
					     j,
					     hrc_read(hrc_hdl, j + 0),
					     hrc_read(hrc_hdl, j + 4),
					     hrc_read(hrc_hdl, j + 8),
					     hrc_read(hrc_hdl, j + 12));
		}
	}

	return n;
}

static int __hrc_enable(struct hrc_hw_handle *hrc_hdl)
{
	hrc_write_mask(hrc_hdl, HRC_TOP_REG, TOP_RESET_MASK, 0);
	hrc_write_mask(hrc_hdl, HRC_TOP_REG, SRAM_GATE_MODE_MASK, 0);
	hrc_write_mask(hrc_hdl, HRC_TOP_REG, MBUS_CLK_EN_MASK, 1);

	hrc_hdl->private->irq_mask =
		  /* FRM_IRQ_EN_MASK        | */
		  CFG_FINISH_IRQ_EN_MASK | WB_FINISH_IRQ_EN_MASK | TIMEOUT_IRQ_EN_MASK |
		  OVERFLOW_IRQ_EN_MASK   | UNUSUAL_IRQ_EN_MASK;

	hrc_write(hrc_hdl, HRC_INT_CTRL_REG, hrc_hdl->private->irq_mask);
	hrc_write(hrc_hdl, HRC_INT_STAT_REG, 0xFFFFFFFF);

	hrc_write_mask(hrc_hdl, HRC_CTRL_REG, HRC_EN_MASK, 1);

	return 0;
}

static int __hrc_disable(struct hrc_hw_handle *hrc_hdl)
{
	hrc_write_mask(hrc_hdl, HRC_CTRL_REG, HRC_EN_MASK, 0);
	hrc_write(hrc_hdl, HRC_INT_CTRL_REG, 0);
	hrc_write(hrc_hdl, HRC_INT_STAT_REG, 0xFFFFFFFF);
	hrc_write_mask(hrc_hdl, HRC_TOP_REG, TOP_RESET_MASK, 1);
	return 0;
}

static int __hrc_commit(struct hrc_hw_handle *hrc_hdl)
{
	hrc_write_mask(hrc_hdl, HRC_READY_REG, REG_RDY_MASK, 1);

	/* for ddr debug: raise self vsync */
	if (hrc_read_mask(hrc_hdl, HRC_CTRL_REG, DATA_SRC_SEL_MASK) == DATA_SRC_DDR)
		hrc_write_mask(hrc_hdl, HRC_TOP_REG, SELF_VSYN_START_MASK, 1);

	return 0;
}

static int __hrc_get_irq_state(struct hrc_hw_handle *hrc_hdl, u32 *state)
{
	u32 val = hrc_read(hrc_hdl, HRC_INT_STAT_REG);
	u32 irq_mask = hrc_hdl->private->irq_mask;

	*state = 0;

	if (val & FRM_IRQ_MASK && irq_mask & FRM_IRQ_EN_MASK)
		*state |= HRC_IRQ_FRAME_VSYNC;

	if (val & CFG_FINISH_IRQ_MASK && irq_mask & CFG_FINISH_IRQ_EN_MASK)
		*state |= HRC_IRQ_CFG_FINISH;

	if (val & WB_FINISH_IRQ_MASK && irq_mask & WB_FINISH_IRQ_EN_MASK)
		*state |= HRC_IRQ_WB_FINISH;

	if (val & TIMEOUT_IRQ_MASK && irq_mask & TIMEOUT_IRQ_EN_MASK)
		*state |= HRC_IRQ_TIMEOUT;

	if (val & OVERFLOW_IRQ_MASK && irq_mask & OVERFLOW_IRQ_EN_MASK)
		*state |= HRC_IRQ_OVERFLOW;

	if (val & UNUSUAL_IRQ_MASK  && irq_mask & UNUSUAL_IRQ_EN_MASK)
		*state |= HRC_IRQ_UNUSUAL;

	return 0;
}

static int __hrc_clr_irq_state(struct hrc_hw_handle *hrc_hdl, u32 state)
{
	u32 val = 0;

	if (!state)
		return 0;

	if (state & HRC_IRQ_FRAME_VSYNC)
		val |= FRM_IRQ_MASK;

	if (state & HRC_IRQ_CFG_FINISH)
		val |= CFG_FINISH_IRQ_MASK;

	if (state & HRC_IRQ_WB_FINISH)
		val |= WB_FINISH_IRQ_MASK;

	if (state & HRC_IRQ_TIMEOUT)
		val |= TIMEOUT_IRQ_MASK;

	if (state & HRC_IRQ_OVERFLOW)
		val |= OVERFLOW_IRQ_MASK;

	if (state & HRC_IRQ_UNUSUAL)
		val |= UNUSUAL_IRQ_MASK;

	hrc_write(hrc_hdl, HRC_INT_STAT_REG, val);

	return 0;
}

static int __hrc_config_csc(struct hrc_hw_handle *hrc_hdl,
			    enum hrc_color_space in_cs,
			    enum hrc_quantization in_quan,
			    enum hrc_color_space out_cs,
			    enum hrc_quantization out_quan)
{
	int i, j;
	int index = 0;

	hrc_dbg("incs: %d outcs: %d inquan: %d outquan: %d\n", in_cs, in_quan, out_cs, out_quan);

	for (i = 0; i < ARRAY_SIZE(csc_info_tables); i++) {
		if (csc_info_tables[i].in_cs == in_cs &&
		    csc_info_tables[i].in_quan == in_quan &&
		    csc_info_tables[i].out_cs == out_cs &&
		    csc_info_tables[i].out_quan == out_quan) {
			index = csc_info_tables[i].index;
			break;
		}
	}
	if (i == ARRAY_SIZE(csc_info_tables)) {
		hrc_err("Can't find csc param index!\n");
		return -EINVAL;
	}

	if (index >= ARRAY_SIZE(hrc_csc_tables)) {
		hrc_err("csc table index %d error!\n", index);
		return -EINVAL;
	}

	hrc_dbg("csc table index: %d\n", index);

	for (i = 0; i < CSC_COEFF_CONST_LEN + CSC_CONSTANT_LEN; i++) {
		if (i < CSC_COEFF_CONST_LEN) {
			if (!((i + 1) % 4))
				hrc_write_mask(hrc_hdl, HRC_CSC_COEFF_XX_REG(i), CSC_CX3_MASK,
					       hrc_csc_tables[index][i]);
			else
				hrc_write_mask(hrc_hdl, HRC_CSC_COEFF_XX_REG(i), CSC_CXX_MASK,
					       hrc_csc_tables[index][i]);
		} else {
			j = i - CSC_COEFF_CONST_LEN;
			hrc_write_mask(hrc_hdl, HRC_CSC_DX_REG(j), CSC_DX_MASK,
				       hrc_csc_tables[index][i]);
		}
	}
	return 0;
}

static u32 __hrc_vsu_calc_fir_coef(u32 step)
{
	u32 pt_coef;
	u32 scale_ratio, int_part, float_part, fir_coef_ofst;

	scale_ratio = step >> (SCALER_Y_HSTEP_INT_OFFSET - 3);  /* Y == C */
	int_part = scale_ratio >> 3;
	float_part = scale_ratio & 0x7;
	if (int_part == 0)
		fir_coef_ofst = VSU_ZOOM0_SIZE;
	else if (int_part == 1)
		fir_coef_ofst = VSU_ZOOM0_SIZE + float_part;
	else if (int_part == 2)
		fir_coef_ofst =
		    VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE + (float_part >> 1);
	else if (int_part == 3)
		fir_coef_ofst =
		    VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE + VSU_ZOOM2_SIZE;
	else if (int_part == 4)
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
				VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE;
	else
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
				VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE +
				VSU_ZOOM4_SIZE;

	pt_coef = fir_coef_ofst * VSU_PHASE_NUM;

	return pt_coef;
}

static int __hrc_config_vsu_y(struct hrc_hw_handle *hrc_hdl, u32 step)
{
	int i;
	u32 pt_coef;

	hrc_write(hrc_hdl, HRC_SCALER_Y_HSTEP_REG, step);
	/* FIXME */
	hrc_write(hrc_hdl, HRC_SCALER_Y_HPHASE_REG, 0);

	pt_coef = __hrc_vsu_calc_fir_coef(step);
	hrc_dbg("vsu[y] step: %d pt_coef: %d\n", step, pt_coef);

	for (i = 0; i < VSU_PHASE_NUM; i++) {
		if (pt_coef + i >= ARRAY_SIZE(lan3coefftab32_left) ||
		    pt_coef + i >= ARRAY_SIZE(lan3coefftab32_right))
			return -EINVAL;

		hrc_write(hrc_hdl, HRC_VSU_Y_COEFF_0_REG + i * 4,
			  lan3coefftab32_left[pt_coef + i]);
		hrc_write(hrc_hdl, HRC_VSU_Y_COEFF_1_REG + i * 4,
			  lan3coefftab32_right[pt_coef + i]);
	}
	return 0;
}

static int __hrc_config_vsu_c(struct hrc_hw_handle *hrc_hdl, enum hrc_fmt format, u32 step)
{
	int i;
	u32 pt_coef;

	hrc_write(hrc_hdl, HRC_SCALER_C_HSTEP_REG, step);
	/* FIXME */
	hrc_write(hrc_hdl, HRC_SCALER_C_HPHASE_REG, 0);

	pt_coef = __hrc_vsu_calc_fir_coef(step);
	hrc_dbg("vsu[c] step: %d pt_coef: %d\n", step, pt_coef);

	if (format == HRC_FMT_RGB) {
		for (i = 0; i < VSU_PHASE_NUM; i++) {
			if (pt_coef + i >= ARRAY_SIZE(lan3coefftab32_left) ||
			    pt_coef + i >= ARRAY_SIZE(lan3coefftab32_right))
				return -EINVAL;

			hrc_write(hrc_hdl, HRC_VSU_C_COEFF_0_REG + i * 4,
				  lan3coefftab32_left[pt_coef + i]);
			hrc_write(hrc_hdl, HRC_VSU_C_COEFF_1_REG + i * 4,
				  lan3coefftab32_right[pt_coef + i]);
		}
	} else {
		for (i = 0; i < VSU_PHASE_NUM; i++) {
			if (pt_coef + i >= ARRAY_SIZE(bicubic8coefftab32_left) ||
			    pt_coef + i >= ARRAY_SIZE(bicubic8coefftab32_right))
				return -EINVAL;

			hrc_write(hrc_hdl, HRC_VSU_C_COEFF_0_REG + i * 4,
				  bicubic8coefftab32_left[pt_coef + i]);
			hrc_write(hrc_hdl, HRC_VSU_C_COEFF_1_REG + i * 4,
				  bicubic8coefftab32_right[pt_coef + i]);
		}
	}

	return 0;
}

static int __hrc_config_param(struct hrc_hw_handle *hrc_hdl,
			      struct hrc_input_param input,
			      struct hrc_output_param output)
{
	int ret;
	u64 step;
	u8 csc_en = 0;
	u8 y_hscaler_en = 0;
	u8 c_hscaler_en = 0;
	u32 output_c_width = 0;

	/* data_src */
	if (input.data_src == HRC_DATA_SRC_DDR) {
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, DATA_SRC_SEL_MASK, DATA_SRC_DDR);
		hrc_write(hrc_hdl, HRC_IN_Y_BUF_ADDR_REG, input.ddr_addr[0]);
		hrc_write(hrc_hdl, HRC_IN_C_BUF_ADDR_REG, input.ddr_addr[1]);

	} else {
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, DATA_SRC_SEL_MASK, DATA_SRC_HDMI_RX);
	}

	/* count: use defalut value */
	hrc_write_mask(hrc_hdl, HRC_COUNT_REG, RESET_CYCLE_MASK, 0x20);
	hrc_write_mask(hrc_hdl, HRC_COUNT_REG, SELF_SYN_LEN_MASK, 0x20);

	/* field */
	if (input.field_mode == HRC_FIELD_MODE_FIELD)
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, FIELD_MODE_MASK, FIELD_MODE_FIELD);
	else
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, FIELD_MODE_MASK, FIELD_MODE_FRAME);

	hrc_write_mask(hrc_hdl, HRC_CTRL_REG, FIELD_INVERSE_MASK, input.field_inverse);

	if (input.field_order == HRC_FIELD_ORDER_TOP)
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, FIELD_ORDER_MASK, FIELD_ORDER_TOP);
	else
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, FIELD_ORDER_MASK, FIELD_ORDER_BOTTOM);

	/* csc enable/disable */
	if (output.format == HRC_FMT_YUV444_1PLANE) {
		if (input.format == HRC_FMT_YUV444) {
			csc_en = 0;
			hrc_write_mask(hrc_hdl, HRC_CTRL_REG, CSC_EN_MASK, 0);
		} else {
			csc_en = 1;
			hrc_write_mask(hrc_hdl, HRC_CTRL_REG, CSC_EN_MASK, 1);
		}
	} else {
		if (input.format != output.format &&
		    input.format == HRC_FMT_RGB) {
			csc_en = 1;
			hrc_write_mask(hrc_hdl, HRC_CTRL_REG, CSC_EN_MASK, 1);
		} else {
			csc_en = 0;
			hrc_write_mask(hrc_hdl, HRC_CTRL_REG, CSC_EN_MASK, 0);
		}
	}

	/* vsample enable/disable */
	if (input.format != HRC_FMT_YUV420 && output.format == HRC_FMT_YUV420)
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, C_VSAMPLE_EN_MASK, 1);
	else
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, C_VSAMPLE_EN_MASK, 0);

	if (input.width != output.width) {
		y_hscaler_en = 1;
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, Y_HSCALER_EN_MASK, 1);
	} else {
		y_hscaler_en = 0;
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, Y_HSCALER_EN_MASK, 0);
	}

	/* scaler enable/disable */
	/* RGB/YUV444 -> YUV422/YUV420: width / 2 */
	if ((input.format == HRC_FMT_RGB || input.format == HRC_FMT_YUV444) &&
	    (output.format == HRC_FMT_YUV422 || output.format == HRC_FMT_YUV420)) {
		c_hscaler_en = 1;
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, C_HSCALER_EN_MASK, 1);
	} else {
		c_hscaler_en = 0;
		hrc_write_mask(hrc_hdl, HRC_CTRL_REG, C_HSCALER_EN_MASK, 0);
	}

	/* timeout: use default value */
	hrc_write(hrc_hdl, HRC_READY_REG, 0);

	/* input width and height */
	hrc_write_mask(hrc_hdl, HRC_SIZE_REG, SRC_WIDTH_MASK, input.width - 1);
	hrc_write_mask(hrc_hdl, HRC_SIZE_REG, SRC_HEIGHT_MASK, input.height - 1);

	/* input pixel format */
	if (input.format == HRC_FMT_YUV444)
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_YUV444);
	else if (input.format == HRC_FMT_YUV422)
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_YUV422);
	else if (input.format == HRC_FMT_YUV420)
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_YUV420);
	else
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, INPUT_FORMAT_MASK, FORMAT_RGB);

	/* input bits */
	if (input.depth == HRC_DEPTH_10)
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, BIT_DEPTH_MASK, DEPTH_10);
	if (input.depth == HRC_DEPTH_12)
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, BIT_DEPTH_MASK, DEPTH_12);
	else
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, BIT_DEPTH_MASK, DEPTH_8);

	/* uncompact: use default value */
	if (input.depth == HRC_DEPTH_10)
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, OUT_UNCOMPACT_MASK, 0);

	/* output width and height (Y channel) */
	hrc_write_mask(hrc_hdl, HRC_SCALER_Y_SIZE_REG, SCALER_Y_OUT_WIDTH_MASK, output.width - 1);
	hrc_write_mask(hrc_hdl, HRC_SCALER_Y_SIZE_REG, SCALER_Y_OUT_HEIGHT_MASK, output.height - 1);

	/* output width and height (C channel) */
	/* RGB/YUV444 -> YUV422/YUV420: width / 2 */
	if ((input.format == HRC_FMT_RGB || input.format == HRC_FMT_YUV444) &&
	    (output.format == HRC_FMT_YUV422 || output.format == HRC_FMT_YUV420))
		output_c_width = output.width / 2;
	else
		output_c_width = output.width;

	hrc_write_mask(hrc_hdl, HRC_SCALER_C_SIZE_REG, SCALER_C_OUT_WIDTH_MASK, output_c_width - 1);
	hrc_write_mask(hrc_hdl, HRC_SCALER_C_SIZE_REG, SCALER_C_OUT_HEIGHT_MASK, output.height - 1);

	/* output pixel format */
	if (output.format == HRC_FMT_YUV444_1PLANE) {
		hrc_write_mask(hrc_hdl, HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_RGB);
	} else {
		if (output.format == HRC_FMT_YUV444)
			hrc_write_mask(hrc_hdl, HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_YUV444);
		else if (output.format == HRC_FMT_YUV422)
			hrc_write_mask(hrc_hdl, HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_YUV422);
		else if (output.format == HRC_FMT_YUV420)
			hrc_write_mask(hrc_hdl, HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_YUV420);
		else
			hrc_write_mask(hrc_hdl, HRC_FMT_REG, OUTPUT_FORMAT_MASK, FORMAT_RGB);
	}

	/* output stride */
	if (output.format == HRC_FMT_RGB || output.format == HRC_FMT_YUV444_1PLANE) {
		hrc_write_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG,
			       OUTPUT_Y_STRIDE_MASK, output.width * 3);
		hrc_write_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG,
			       OUTPUT_C_STRIDE_MASK, 0);
	} else if (output.format == HRC_FMT_YUV444) {
		hrc_write_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG,
			       OUTPUT_Y_STRIDE_MASK, output.width);
		hrc_write_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG,
			       OUTPUT_C_STRIDE_MASK, output.width * 2);
	} else {
		hrc_write_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG,
			       OUTPUT_Y_STRIDE_MASK, output.width);
		hrc_write_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG,
			       OUTPUT_C_STRIDE_MASK, output.width);
	}

	/* random signal control: use default value */
	hrc_write_mask(hrc_hdl, HRC_RSC_REG, RSC_SEED_MASK, 0);
	hrc_write_mask(hrc_hdl, HRC_RSC_REG, RSC_EN_MASK, 0);

	hrc_dbg("csc: %d y_hscaler: %d c_hscaler: %d\n", csc_en, y_hscaler_en, c_hscaler_en);

	/* csc */
	if (csc_en) {
		ret = __hrc_config_csc(hrc_hdl, input.cs, input.quantization,
				       output.cs, output.quantization);
		if (ret)
			return ret;
	}

	/* scaler */
	if (y_hscaler_en) {
		step = (u64)input.width << SCALER_Y_HSTEP_INT_OFFSET;
		do_div(step, output.width);
		ret = __hrc_config_vsu_y(hrc_hdl, step);
		if (ret)
			return ret;
	}

	if (c_hscaler_en) {
		step = (u64)input.width << SCALER_C_HSTEP_INT_OFFSET;
		do_div(step, output_c_width);
		ret = __hrc_config_vsu_c(hrc_hdl, input.format, step);
		if (ret)
			return ret;
	}

	return 0;
}

static int __hrc_refresh_output_address(struct hrc_hw_handle *hrc_hdl,
					struct hrc_output_param output)
{
	hrc_write(hrc_hdl, HRC_OUT_Y_BUF_LADDR_REG, output.out_addr[0][0]);
	hrc_write_mask(hrc_hdl, HRC_OUT_Y_BUF_HADDR_REG,
		       OUTPUT_Y_BUF_HADDR_MASK, output.out_addr[0][1]);

	hrc_write(hrc_hdl, HRC_OUT_C_BUF_LADDR_REG, output.out_addr[1][0]);
	hrc_write_mask(hrc_hdl, HRC_OUT_C_BUF_HADDR_REG,
		       OUTPUT_C_BUF_HADDR_MASK, output.out_addr[1][1]);
	return 0;
}

static bool __hrc_check_size(struct hrc_hw_handle *hrc_hdl, u32 input_width, u32 input_height,
			     u32 output_width, u32 output_height)
{
	const struct hrc_hw_desc *desc = hrc_hdl->private->desc;

	if (input_width < desc->min_width || input_width > desc->max_width)
		return false;
	if (output_width < desc->min_width || output_width > desc->max_width)
		return false;
	if (input_height < desc->min_height || input_height > desc->max_height)
		return false;
	if (output_height < desc->min_height || output_height > desc->max_height)
		return false;

	if (input_width != output_width && !(desc->scaler & HRC_SCALER_HOR))
		return false;
	if (input_height != output_height && !(desc->scaler & HRC_SCALER_VER))
		return false;

	if (hrc_hdl->private->desc->version == 0x110) {
		/* width scale down (2x) */
		if (input_width != output_width && input_width != output_width * 2)
			return false;
	}

	return true;
}

static bool __hrc_check_format(struct hrc_hw_handle *hrc_hdl,
			       enum hrc_fmt input_format,
			       enum hrc_fmt output_format)
{
	int i, j;
	const struct hrc_hw_fmt *fmt_table = hrc_hdl->private->desc->fmt;
	u32 fmt_table_size = hrc_hdl->private->desc->fmt_size;

	for (i = 0; i < fmt_table_size; i++) {
		if (input_format != fmt_table[i].input_format)
			continue;

		for (j = 0; j < fmt_table[i].output_format_size; j++)
			if (output_format == fmt_table[i].output_format[j])
				return true;
	}

	return false;
}

static bool __hrc_check_color_space(struct hrc_hw_handle *hrc_hdl,
				    enum hrc_color_space input_cs,
				    enum hrc_color_space output_cs)
{
	int i, j;
	const struct hrc_hw_color_space *cs_table = hrc_hdl->private->desc->cs;
	u32 cs_table_size = hrc_hdl->private->desc->cs_size;

	for (i = 0; i < cs_table_size; i++) {
		if (input_cs != cs_table[i].input_cs)
			continue;

		for (j = 0; j < cs_table[i].output_cs_size; j++)
			if (output_cs == cs_table[i].output_cs[j])
				return true;
	}

	return false;
}

static bool __hrc_check_quantization(struct hrc_hw_handle *hrc_hdl,
				     enum hrc_quantization input_quan,
				     enum hrc_quantization output_quan)
{
	int i, j;
	const struct hrc_hw_quantization *quan_table = hrc_hdl->private->desc->quan;
	u32 quan_table_size = hrc_hdl->private->desc->quan_size;

	for (i = 0; i < quan_table_size; i++) {
		if (input_quan != quan_table[i].input_quan)
			continue;

		for (j = 0; j < quan_table[i].output_quan_size; j++)
			if (output_quan == quan_table[i].output_quan[j])
				return true;
	}

	return false;
}

static int __hrc_dump(struct hrc_hw_handle *hrc_hdl, char *buf, int n)
{
	const char * const src_name[] = {"HDMI", "DDR"};
	const char * const field_mode_name[] = {"FRAME", "FIELD"};
	const char * const field_order_name[] = {"BOTTOM", "TOP"};
	const char * const fmt_name[] = {"RGB888", "YUV444", "YUV422", "YUV420"};
	const char * const depth_name[] = {"8BITS", "10BITS", "12BITS"};

#define SHOW_NAME(x, i) ((i) < ARRAY_SIZE(x) ? x[(i)] : "UNKNOWN")
#define SHOW_BOOL(i)	((i) ? "TRUE" : "FALSE")

	n += sprintf(buf + n, "\n[CTRL]\n");
	n += sprintf(buf + n, "  - data_src: %s\n",
		     SHOW_NAME(src_name,
			       hrc_read_mask(hrc_hdl, HRC_CTRL_REG, DATA_SRC_SEL_MASK)));
	n += sprintf(buf + n, "  - field mode: %s inverse: %s order: %s\n",
		     SHOW_NAME(field_mode_name,
			       hrc_read_mask(hrc_hdl, HRC_CTRL_REG, FIELD_MODE_MASK)),
		     SHOW_BOOL(hrc_read_mask(hrc_hdl, HRC_CTRL_REG, FIELD_INVERSE_MASK)),
		     SHOW_NAME(field_order_name,
			       hrc_read_mask(hrc_hdl, HRC_CTRL_REG, FIELD_ORDER_MASK)));
	n += sprintf(buf + n, "  - csc: %d c_vsample: %d y_hscaler: %d c_hscaler: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_CTRL_REG, CSC_EN_MASK),
		     hrc_read_mask(hrc_hdl, HRC_CTRL_REG, C_VSAMPLE_EN_MASK),
		     hrc_read_mask(hrc_hdl, HRC_CTRL_REG, Y_HSCALER_EN_MASK),
		     hrc_read_mask(hrc_hdl, HRC_CTRL_REG, C_HSCALER_EN_MASK));
	n += sprintf(buf + n, "  - reset: %d self_vsync: %d timeout: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_COUNT_REG, RESET_CYCLE_MASK),
		     hrc_read_mask(hrc_hdl, HRC_COUNT_REG, SELF_SYN_LEN_MASK),
		     hrc_read_mask(hrc_hdl, HRC_READY_REG, TIMEOUT_CYCLE_MASK));

	n += sprintf(buf + n, "\n[INPUT]\n");
	n += sprintf(buf + n, "  - width: %d height: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SIZE_REG, SRC_WIDTH_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SIZE_REG, SRC_HEIGHT_MASK));
	n += sprintf(buf + n, "  - format: %s depth: %s\n",
		     SHOW_NAME(fmt_name,
			       hrc_read_mask(hrc_hdl, HRC_FMT_REG, INPUT_FORMAT_MASK)),
		     SHOW_NAME(depth_name,
			       hrc_read_mask(hrc_hdl, HRC_FMT_REG, BIT_DEPTH_MASK)));

	n += sprintf(buf + n, "\n[OUTPUT]\n");
	n += sprintf(buf + n, "  - [Y Channel]:\n");
	n += sprintf(buf + n, "    - width: %d height: %d stride: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SCALER_Y_SIZE_REG, SCALER_Y_OUT_WIDTH_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SCALER_Y_SIZE_REG, SCALER_Y_OUT_HEIGHT_MASK),
		     hrc_read_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG, OUTPUT_Y_STRIDE_MASK));
	n += sprintf(buf + n, "    - h_step_int: %d h_step_frac: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SCALER_Y_HSTEP_REG, SCALER_Y_HSTEP_INT_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SCALER_Y_HSTEP_REG, SCALER_Y_HSTEP_FRAC_MASK));
	n += sprintf(buf + n, "    - h_phase_int: %d h_phase_frac: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SCALER_Y_HPHASE_REG, SCALER_Y_HPHASE_INT_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SCALER_Y_HPHASE_REG, SCALER_Y_HPHASE_FRAC_MASK));
	n += sprintf(buf + n, "  - [C Channel]:\n");
	n += sprintf(buf + n, "    - width: %d height: %d stride: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SCALER_C_SIZE_REG, SCALER_C_OUT_WIDTH_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SCALER_C_SIZE_REG, SCALER_C_OUT_HEIGHT_MASK),
		     hrc_read_mask(hrc_hdl, HRC_OUT_BUF_STRIDE_REG, OUTPUT_C_STRIDE_MASK));
	n += sprintf(buf + n, "    - h_step_int: %d h_step_frac: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SCALER_C_HSTEP_REG, SCALER_C_HSTEP_INT_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SCALER_C_HSTEP_REG, SCALER_C_HSTEP_FRAC_MASK));
	n += sprintf(buf + n, "    - h_phase_int: %d h_phase_frac: %d\n",
		     hrc_read_mask(hrc_hdl, HRC_SCALER_C_HPHASE_REG, SCALER_C_HPHASE_INT_MASK),
		     hrc_read_mask(hrc_hdl, HRC_SCALER_C_HPHASE_REG, SCALER_C_HPHASE_FRAC_MASK));
	n += sprintf(buf + n, "  - format: %s depth: (same as input)\n",
		     SHOW_NAME(fmt_name,
			       hrc_read_mask(hrc_hdl, HRC_FMT_REG, OUTPUT_FORMAT_MASK)));

	return n;
}

int sunxi_hrc_hardware_enable(struct hrc_hw_handle *hrc_hdl)
{
	if (IS_ERR_OR_NULL(hrc_hdl))
		return -EINVAL;

	return __hrc_enable(hrc_hdl);
}

int sunxi_hrc_hardware_disable(struct hrc_hw_handle *hrc_hdl)
{
	if (IS_ERR_OR_NULL(hrc_hdl))
		return -EINVAL;

	return __hrc_disable(hrc_hdl);
}

int sunxi_hrc_hardware_get_size_range(struct hrc_hw_handle *hrc_hdl, struct hrc_size_range *size)
{
	const struct hrc_hw_desc *desc;

	if (IS_ERR_OR_NULL(hrc_hdl))
		return -EINVAL;

	desc = hrc_hdl->private->desc;
	size->min_width    = desc->min_width;
	size->min_height   = desc->min_height;
	size->max_width    = desc->max_width;
	size->max_height   = desc->max_height;
	size->align_width  = desc->align_width;
	size->align_height = desc->align_height;
	return 0;
}

int sunxi_hrc_hardware_get_format(struct hrc_hw_handle *hrc_hdl,
				  enum hrc_fmt in_format,
				  enum hrc_fmt *out_format)
{
	if (IS_ERR_OR_NULL(hrc_hdl) || IS_ERR_OR_NULL(out_format))
		return -EINVAL;

	if (!__hrc_check_format(hrc_hdl, in_format, *out_format))
		/* returns the recommended value */
		*out_format = in_format;

	return 0;
}

int sunxi_hrc_hardware_get_color_space(struct hrc_hw_handle *hrc_hdl,
				       enum hrc_color_space input_cs,
				       enum hrc_color_space *output_cs)
{
	if (IS_ERR_OR_NULL(hrc_hdl) || IS_ERR_OR_NULL(output_cs))
		return -EINVAL;

	if (!__hrc_check_color_space(hrc_hdl, input_cs, *output_cs))
		/* returns the recommended value */
		*output_cs = input_cs;

	return 0;
}

int sunxi_hrc_hardware_get_quantization(struct hrc_hw_handle *hrc_hdl,
					enum hrc_quantization input_quan,
					enum hrc_quantization *output_quan)
{
	if (IS_ERR_OR_NULL(hrc_hdl) || IS_ERR_OR_NULL(output_quan))
		return -EINVAL;

	if (!__hrc_check_quantization(hrc_hdl, input_quan, *output_quan))
		/* returns the recommended value */
		*output_quan = input_quan;

	return 0;
}

int sunxi_hrc_hardware_check(struct hrc_hw_handle *hrc_hdl, struct hrc_apply_data *data)
{
	int ret;

	if (IS_ERR_OR_NULL(hrc_hdl) || IS_ERR_OR_NULL(data))
		return -EINVAL;

	if (data->dirty & HRC_APPLY_PARAM_DIRTY || data->dirty & HRC_APPLY_ALL_DIRTY) {
		ret = __hrc_check_size(hrc_hdl, data->input.width, data->input.height,
				       data->output.width, data->output.height);
		if (!ret) {
			hrc_err("check size error! [IN]: %dx%d [OUT]: %dx%d\n",
				data->input.width, data->input.height,
				data->output.width, data->output.height);
			return -EINVAL;
		}

		ret = __hrc_check_format(hrc_hdl, data->input.format, data->output.format);
		if (!ret) {
			hrc_err("check format error! [IN]: %d [OUT]: %d\n",
				data->input.format, data->output.format);
			return -EINVAL;
		}

		ret = __hrc_check_color_space(hrc_hdl, data->input.cs, data->output.cs);
		if (!ret) {
			hrc_err("check color space error! [IN]: %d [OUT]: %d\n",
				data->input.cs, data->output.cs);
			return -EINVAL;
		}

		ret = __hrc_check_quantization(hrc_hdl, data->input.quantization,
					       data->output.quantization);
		if (!ret) {
			hrc_err("check quantization error! [IN]: %d [OUT]: %d\n",
				data->input.quantization, data->output.quantization);
			return -EINVAL;
		}
	}

	return 0;
}

int sunxi_hrc_hardware_apply(struct hrc_hw_handle *hrc_hdl, struct hrc_apply_data *data)
{
	int ret;

	if (IS_ERR_OR_NULL(hrc_hdl))
		return -EINVAL;

	ret = sunxi_hrc_hardware_check(hrc_hdl, data);
	if (ret)
		return ret;

	if (data->dirty & HRC_APPLY_PARAM_DIRTY || data->dirty & HRC_APPLY_ALL_DIRTY) {
		ret = __hrc_config_param(hrc_hdl, data->input, data->output);
		if (ret)
			return ret;
	}

	if (data->dirty & HRC_APPLY_ADDR_DIRTY || data->dirty & HRC_APPLY_ALL_DIRTY) {
		ret = __hrc_refresh_output_address(hrc_hdl, data->output);
		if (ret)
			return ret;
	}

	data->dirty = 0;
	return 0;
}

int sunxi_hrc_hardware_commit(struct hrc_hw_handle *hrc_hdl)
{
	if (IS_ERR_OR_NULL(hrc_hdl))
		return -EINVAL;

	hrc_dbg("\n");
	return __hrc_commit(hrc_hdl);
}

int sunxi_hrc_hardware_get_irq_state(struct hrc_hw_handle *hrc_hdl, u32 *state)
{
	if (IS_ERR_OR_NULL(hrc_hdl) || IS_ERR_OR_NULL(state))
		return -EINVAL;

	return __hrc_get_irq_state(hrc_hdl, state);
}

int sunxi_hrc_hardware_clr_irq_state(struct hrc_hw_handle *hrc_hdl, u32 state)
{
	if (IS_ERR_OR_NULL(hrc_hdl))
		return -EINVAL;

	return __hrc_clr_irq_state(hrc_hdl, state);
}

int sunxi_hrc_hardware_dump(struct hrc_hw_handle *hrc_hdl, char *buf, int n)
{
	if (IS_ERR_OR_NULL(hrc_hdl) || IS_ERR_OR_NULL(buf))
		return -EINVAL;

	return __hrc_dump(hrc_hdl, buf, n);
}

struct hrc_hw_handle *sunxi_hrc_handle_create(struct hrc_hw_create_info *info)
{
	struct resource *iores;
	const struct hrc_hw_desc *desc;
	struct hrc_hw_handle *hrc_hdl = NULL;

	if (IS_ERR_OR_NULL(info))
		return NULL;

	desc = get_hrc_desc(info->version);
	if (IS_ERR_OR_NULL(desc))
		return NULL;

	hrc_hdl = kmalloc(sizeof(*hrc_hdl), GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(hrc_hdl))
		return ERR_PTR(-ENOMEM);

	hrc_hdl->private = kmalloc(sizeof(*hrc_hdl), GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(hrc_hdl))
		return ERR_PTR(-ENOMEM);

	memcpy(&hrc_hdl->info, info, sizeof(hrc_hdl->info));
	hrc_hdl->private->desc = desc;

	iores = platform_get_resource(info->pdev, IORESOURCE_MEM, 0);
	hrc_hdl->private->reg_base = devm_ioremap_resource(&info->pdev->dev, iores);
	if (IS_ERR(hrc_hdl->private->reg_base)) {
		kfree(hrc_hdl->private);
		kfree(hrc_hdl);
		return NULL;
	}

	return hrc_hdl;
}

void sunxi_hrc_handle_destory(struct hrc_hw_handle *hrc_hdl)
{
	if (IS_ERR_OR_NULL(hrc_hdl))
		return;

	devm_iounmap(&hrc_hdl->info.pdev->dev, hrc_hdl->private->reg_base);
	kfree(hrc_hdl->private);
	kfree(hrc_hdl);
}
