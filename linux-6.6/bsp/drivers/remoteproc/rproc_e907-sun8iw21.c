/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include "sunxi_rproc_boot.h"

/*
 * RISC-V CFG Peripheral Register
 */
#define RV_CFG_VER_REG			(0x0000) /* RV_CFG Version Register */
#define RV_CFG_RF1P_CFG_REG		(0x0010) /* RV_CFG Control Register0 */
#define RV_CFG_TS_TMODE_SEL_REG		(0x0040) /* RV_CFG Test Mode Select Register */
#define RV_CFG_STA_ADD_REG		(0x0204) /* RV_CFG Boot Address Register */
#define RV_CFG_WAKEUP_EN_REG		(0x0220) /* RV_CFG WakeUp Enable Register */
#define RV_CFG_WAKEUP_MASK0_REG		(0x0224) /* RV_CFG WakeUp Mask0 Register */
#define RV_CFG_WAKEUP_MASK1_REG		(0x0228) /* RV_CFG WakeUp Mask1 Register */
#define RV_CFG_WAKEUP_MASK2_REG		(0x022C) /* RV_CFG WakeUp Mask2 Register */
#define RV_CFG_WAKEUP_MASK3_REG		(0x0230) /* RV_CFG WakeUp Mask3 Register */
#define RV_CFG_WAKEUP_MASK4_REG		(0x0234) /* RV_CFG WakeUp Mask4 Register */
#define RV_CFG_WORK_MODE_REG		(0x0248) /* RV_CFG Worke Mode Register */

#define RPROC_NAME "e907"

#define RV_DTS_PROPERTY_CORE_CLK_FREQ "core-clock-frequency"
#define RV_DTS_PROPERTY_AXI_CLK_FREQ "axi-clock-frequency"

#define RV_CORE_GATE_CLK_NAME "core-gate"
#define RV_IO_RES_NAME "rv-cfg"

static struct rproc_common_res common_res[] = {
	/* clk */
	{ .res_name = "input", .type = RPROC_COMMON_CLK_RES },
	{ .res_name = "core-div", .type = RPROC_COMMON_CLK_RES },
	{ .res_name = "core-gate", .type = RPROC_COMMON_CLK_RES },
	{ .res_name = "axi-div", .type = RPROC_COMMON_CLK_RES },
	{ .res_name = "rv-cfg-gate", .type = RPROC_COMMON_CLK_RES },
	{ .res_name = "core-rst", .type = RPROC_COMMON_CLK_RES },
	{ .res_name = "dbg-rst", .type = RPROC_COMMON_CLK_RES },
	/* rst */
	{ .res_name = "rv-cfg-rst", .type = RPROC_COMMON_RST_RES },
	/* reg */
	{ .res_name = RV_IO_RES_NAME, .type = RPROC_COMMON_REG_RES }
};

static struct rproc_clk_opration common_start_seq[]  = {
	{ .name = "core-rst",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "dbg-rst",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "rv-cfg-rst", .option = RPROC_COMMON_OP(ASSERT) },
	{ .name = "rv-cfg-gate", .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "core-div",   .option = RPROC_COMMON_OP(SETPARENT), .parent = "input" },
	{ .name = "core-div",   .option = RPROC_COMMON_OP(SETRATE),
			.prop_rate = RV_DTS_PROPERTY_CORE_CLK_FREQ, .default_freq = 600000000 },
	{ .name = "axi-div",   .option = RPROC_COMMON_OP(SETRATE),
			.prop_rate = RV_DTS_PROPERTY_AXI_CLK_FREQ, .default_freq = 300000000 },
	{ .name = "core-gate",  .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "rv-cfg-rst", .option = RPROC_COMMON_OP(DEASSERT) },
	{ .name = RV_IO_RES_NAME, .option = RPROC_COMMON_OP(WRITEREG), .offset = RV_CFG_STA_ADD_REG },
	{ .name = "dbg-rst",  .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "core-rst",  .option = RPROC_COMMON_OP(ENABLE) },
};
static struct rproc_clk_opration common_stop_seq[]  = {
	{ .name = "core-rst",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "dbg-rst",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "rv-cfg-rst", .option = RPROC_COMMON_OP(ASSERT) },
	{ .name = "core-gate",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "rv-cfg-gate",  .option = RPROC_COMMON_OP(DISABLE) },
};
static struct rproc_clk_opration common_attach_seq[]  = {
	{ .name = "rv-cfg-gate", .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "core-div",   .option = RPROC_COMMON_OP(SETPARENT), .parent = "input" },
	{ .name = "core-div",   .option = RPROC_COMMON_OP(SETRATE),
			.prop_rate = RV_DTS_PROPERTY_CORE_CLK_FREQ, .default_freq = 600000000 },
	{ .name = "axi-div",   .option = RPROC_COMMON_OP(SETRATE),
			.prop_rate = RV_DTS_PROPERTY_AXI_CLK_FREQ, .default_freq = 300000000 },
	{ .name = "core-gate",  .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "rv-cfg-rst", .option = RPROC_COMMON_OP(DEASSERT) },
	{ .name = RV_IO_RES_NAME, .option = RPROC_COMMON_OP(WRITEREG), .offset = RV_CFG_STA_ADD_REG },
	{ .name = "dbg-rst",  .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "core-rst",  .option = RPROC_COMMON_OP(ENABLE) },
};

static struct rproc_clk_opration common_reset_seq[]  = {
	{ .name = "core-rst",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "dbg-rst",  .option = RPROC_COMMON_OP(DISABLE) },
	{ .name = "rv-cfg-rst", .option = RPROC_COMMON_OP(ASSERT) },
	{ .name = "core-rst",  .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "dbg-rst",  .option = RPROC_COMMON_OP(ENABLE) },
	{ .name = "rv-cfg-rst", .option = RPROC_COMMON_OP(DEASSERT) },
};

struct rproc_common_boot common_boot = {
	.name = RPROC_NAME,
	.res = common_res, .nr_res = ARRAY_SIZE(common_res),
	.start_seq = common_start_seq, .nr_start_seq = ARRAY_SIZE(common_start_seq),
	.stop_seq = common_stop_seq, .nr_stop_seq = ARRAY_SIZE(common_stop_seq),
	.attach_seq = common_attach_seq, .nr_attach_seq = ARRAY_SIZE(common_attach_seq),
	.reset_seq = common_reset_seq, .nr_reset_seq = ARRAY_SIZE(common_reset_seq),
	.suspend_seq = common_stop_seq, .nr_suspend_seq = ARRAY_SIZE(common_stop_seq),
	.resume_seq = common_start_seq, .nr_resume_seq = ARRAY_SIZE(common_start_seq),
};
