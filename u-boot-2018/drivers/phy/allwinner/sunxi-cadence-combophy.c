// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner USB3.0/PCIE/DisplayPort Combo Phy driver
 *
 * Copyright (C) 2024 Allwinner Electronics Co., Ltd.
 */
#include <phy-dp.h>
#include <dm.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <iotrace.h>
#include <dm/device.h>
#include <dm/ofnode.h>
#include <dm/of_access.h>
#include <generic-phy.h>
#include <dt-bindings/phy/phy.h>
#include <clk/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/bitops.h>

#define msleep(a)    udelay(a * 1000)
#define phy_set_mask(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define phy_clear_mask(width, shift)   (~(phy_set_mask(width, shift)))

#define phy_readl(addr) readl(addr)
#define phy_readw(addr) readw(addr)
#define phy_writel(addr, val) writel(val, addr)
#define phy_writew(addr, val) writew(val, addr)

#define MAX_LANE_CNT 4
#define MAX_SWING 4
#define MAX_PRE 4

static struct clk *clk_get_by_node(ofnode node, const char *id)
{
	int index;
	struct clk *c;

	index = ofnode_stringlist_search(node, "clock-names", id);
	if (index >= 0) {
		c = of_clk_get(ofnode_to_offset(node), index);
		return c;
	}
	return NULL;
}

static void phy_set_lbits(u32 addr_val, u32 shift, u32 width, u32 val)
{
	u32 reg_val;
	void *addr;

	addr = (void *) addr_val;
	reg_val = phy_readl(addr);
	reg_val &= phy_clear_mask(width, shift);
	reg_val |= (val << shift);
	phy_writel(addr, reg_val);
}

enum phy_type_e {
	COMBO_PHY0 = 0,
	COMBO_PHY1,
	AUX_HPD,
};

enum phy_orientation {
	ORIENTATION_NONE,
	ORIENTATION_NORMAL,
	ORIENTATION_REVERSE,
};

struct sunxi_cadence_combophy {
	const char *name;
	int type;
	ulong top_reg;
	ulong phy_reg;
	struct clk *clk;
	struct clk *bus_clk;
	struct phy *phy;
	struct sunxi_cadence_phy *sunxi_cphy;

	u32 lane_remap[MAX_LANE_CNT];
	u32 lane_invert[MAX_LANE_CNT];
	unsigned int link_rate;
	enum phy_orientation orientation;
};

struct sunxi_cadence_phy {
	ulong top_subsys_reg;
	ulong top_combo_reg;
	struct udevice *dev;
	struct clk *serdes_clk;
	struct clk *dcxo_serdes0_clk;
	struct clk *dcxo_serdes1_clk;

	struct sunxi_cadence_combophy *combo0;
	struct sunxi_cadence_combophy *combo1;
	struct sunxi_cadence_combophy *aux_hpd;

};

/* sysrtc */
#define DCXO_SERDES1_GATING			BIT(5)

/* Sub-System PCIE Bus Gating Reset Register */
#define SUBSYS_PCIE_BGR				0x0004
#define SUBSYS_PCIE_GATING			(BIT(16) | BIT(17) | BIT(18))
#define SUBSYS_DISABLE_COMBO1_AUTOGATING	BIT(29)
#define SUBSYS_COMB1_PIPE			0xc44
#define SUBSYS_COMB1_PIPE_PCIE			0x1

/* Sub-System USB3.1 Bus Gating Reset Register */
#define SUBSYS_USB3P1_BGR			0x0008
#define   USB3P1_ONLY_UTMI_CLK_SEL		BIT(21)
#define   USB3P1_ONLY_PIPE_CLK_SEL		BIT(20)
#define   USB3P1_ACLK_EN			BIT(17)
#define   USB3P1_HCLK_EN			BIT(16)
#define   USB3P1_USB2P0_PHY_RSTN		BIT(4)

/* Sub-System DBG CTRL Register */
#define SUBSYS_DBG_CTL				0x00f0
#define   COMBO1_PHY_REFCLK_AUTO_GATE_DIS	BIT(29)
#define   COMBO0_PHY_REFCLK_AUTO_GATE_DIS	BIT(28)

/* COMB0 PHY PINS Link Control Register0 */
#define COMB0_PHY_PINS_LINK_CTRL0		0x0000
#define   PHY0_TYPEC_CONN_DIR			BIT(12) /* 0: normal orientation; 1: flipped orientation */
#define   PHY0_DP_LINK_RESET_N_SOFT		BIT(8)
#define   PHY0_PIPE_LINK_RESET_N_USB3P1_MASK	BIT(5)
#define   PHY0_PIPE_LINK_RESET_N_SOFT		BIT(4)
#define   PHY0_RESET_N				BIT(0)

/* COMB0 PHY PINS Link Control Register1 */
#define COMB0_PHY_PINS_LINK_CTRL1		0x0004
#define   PHY0_PMA_CMN_PLL1_REFCLK_SEL		BIT(24)
#define   PHY0_PMA_CMN_PLL0_REFCLK_SEL		BIT(20)
#define   PHY0_PMA_CMN_REFCLK_TERM_EN		BIT(16)
#define   PHY0_PMA_CMN_REFCLK_DIG_DIV_MASK	GENMASK(13, 12)
#define   PHY0_PMA_CMN_REFCLK_DIG_DIV(n)	(((n) & 0x3) << 12)
#define   PHY0_PMA_CMN_REFCLK_INT_MODE_MASK	GENMASK(9, 8)
#define   PHY0_PMA_CMN_REFCLK_INT_MODE(n)	(((n) & 0x3) << 8)
#define   PHY0_PMA_CMN_REFCLK_MODE_MASK		GENMASK(5, 4)
#define   PHY0_PMA_CMN_REFCLK_MODE(n)		(((n) & 0x3) << 4)
#define   PHY0_PMA_CMN_REFCLK_DIG_SEL		BIT(0)

/* COMB0 PHY PINS Link Control Register2 */
#define COMB0_PHY_PINS_LINK_CTRL2		0x0008
#define   PHY0_XCVR_BSCAN_MODE_EN_SEL		BIT(21)
#define   PHY0_XCVR_BSCAN_MODE_EN_SOFT		BIT(20)
#define   PHY0_BSCAN_EXT_SELECT			BIT(16)
#define   PHY0_DBG_RX_RD_LANE_SEL_MASK		GENMASK(6, 4)
#define   PHY0_DBG_RX_RD_LANE_SEL(n)		(((n) & 0x7) << 4)
#define   PHY0_IDDQ_EN_SEL			BIT(1)
#define   PHY0_IDDQ_EN_SOFT			BIT(0)

/* COMB0 PHY PINS Link Control Register3 */
#define COMB0_PHY_PINS_LINK_CTRL3		0x000C
#define   PHY0_PMA_TX_ELEC_IDLE_LN_3		BIT(15)
#define   PHY0_PMA_TX_ELEC_IDLE_LN_2		BIT(14)
#define   PHY0_PMA_TX_ELEC_IDLE_LN_1		BIT(13)
#define   PHY0_PMA_TX_ELEC_IDLE_LN_0		BIT(12)
#define   PHY0_PMA_XCVR_POWER_STATE_REQ_LN_MASK		GENMASK(9, 4)
#define   PHY0_PMA_XCVR_POWER_STATE_REQ_LN(n)	(((n) & 0x3f) << 4)
#define   PHY0_PMA_XCVR_PLLCLK_EN_LN		BIT(0)

/* COMB0 PHY PINS Link Status Register0 */
#define COMB0_PHY_PINS_LINK_STUS0		0x0900
#define   PHY0_LINK_POWER_STATE_ACK_MASK	GENMASK(13, 8)
#define   PHY0_LINK_POWER_STATE_ACK(n)		(((n) & 0x3f) << 8)
#define   PHY0_LINK_CLOCK_EN_ACK		BIT(4)
#define   PHY0_PMA_CMN_READY			BIT(0)

/* COMB1 PHY PINS Link Control Register0 */
#define COMB1_PHY_PINS_LINK_CTRL0		0x0000
#define   PHY1_LINK_CFG_LN_MASK			GENMASK(6, 4)
#define   PHY1_LINK_CFG_LN(n)			(((n) & 0x7) << 4)
#define   PHY1_RESET_N				BIT(0)

/* COMB1 PHY PINS Link Control Register1 */
#define COMB1_PHY_PINS_LINK_CTRL1		0x0004
#define   PHY1_EN_REFCLK			BIT(28)
#define   PHY1_PMA_CMN_PLL1_REFCLK_SEL		BIT(24)
#define   PHY1_PMA_CMN_PLL0_REFCLK_SEL		BIT(20)
#define   PHY1_PMA_CMN_REFCLK_TERM_EN		BIT(16)
#define   PHY1_PMA_CMN_REFCLK_DIG_DIV_MASK	GENMASK(13, 12)
#define   PHY1_PMA_CMN_REFCLK_DIG_DIV(n)	(((n) & 0x3) << 12)
#define   PHY1_PMA_CMN_REFCLK_INT_MODE_MASK	GENMASK(9, 8)
#define   PHY1_PMA_CMN_REFCLK_INT_MODE(n)	(((n) & 0x3) << 8)
#define   PHY1_PMA_CMN_REFCLK_MODE_MASK		GENMASK(5, 4)
#define   PHY1_PMA_CMN_REFCLK_MODE(n)		(((n) & 0x3) << 4)
#define   PHY1_PMA_CMN_REFCLK_DIG_SEL		BIT(0)

/* COMB1 PHY PINS Link Control Register3 */
#define COMB1_PHY_PINS_LINK_CTRL3		0x000C
#define   PHY1_XCVR_BSCAN_MODE_EN		BIT(20)
#define   PHY1_BSCAN_EXT_SELECT			BIT(16)
#define   PHY1_DTB_MUX_SELECTE_MASK		GENMASK(12, 8)
#define   PHY1_DTB_MUX_SELECTE(n)		(((n) & 0x1f) << 8)
#define   PHY1_IDDQ_EN_SEL			BIT(1)
#define   PHY1_IDDQ_EN				BIT(0)

/* COMB1 PHY PINS Lane0 Control Register1 */
#define COMB1_PHY_PINS_LANE0_CTRL0		0x0100
#define   PHY1_L00_ENT_L1_X			BIT(16)
#define   PHY1_L00_PCIE_L1_SS_SEL		BIT(12)
#define   PHY1_PMA_FULLRT_DIV_LN_0_MASK		GENMASK(9, 8)
#define   PHY1_PMA_FULLRT_DIV_LN_0(n)		(((n) & 0x3) << 8)
#define   PHY1_L00_MODE_MASK			GENMASK(5, 4)
#define   PHY1_L00_MODE(n)			(((n) & 0x3) << 4)
#define   PHY1_L00_RESET_N_HW_DIS		BIT(1)
#define   PHY1_L00_RESET_N_SW			BIT(0)

/* COMB1 PHY PINS Link Status Register0 */
#define COMB1_PHY_PINS_LINK_STUS0		0x0900
#define   PHY1_PMA_CMN_CLOCK_STOP_ACK		BIT(8)
#define   PHY1_PMA_CMN_EXT_REFCLK_DETECTED_VALID	BIT(5)
#define   PHY1_PMA_CMN_EXT_REFCLK_DETECTED	BIT(4)
#define   PHY1_PMA_CMN_READY			BIT(0)

/* USB3.1 PIPE CLK MAP CTRL Register */
#define USB3P1_PIPE_CLK_MAP_CTRL		0x0100
#define   USB3P1_PIPE_CLK_MAP_MASK		GENMASK(3, 0)
#define   USB3P1_PIPE_CLK_MAP(n)		(((n) & 0xf)) /* 0x0: PHY0 Lane0 or Lane3; 0x1: PHY1 Lane0 */

/* USB3.1 PIPE RXDATA MAP CTRL Register */
#define USB3P1_PIPE_RXD_MAP_CTRL		0x0104
#define   USB3P1_PIPE_RXD_MAP_MASK		GENMASK(3, 0)
#define   USB3P1_PIPE_RXD_MAP(n)		(((n) & 0xf)) /* 0x0: PHY0 Lane0 or Lane2; 0x1: PHY1 Lane0 */

/* COMB1 PHY PIPE TXDATA MAP CTRL Register */
#define COMB1_PHY_PIPE_TXDATA_MAP_CTRL		0x0c44
#define   COMB1_PHY_LN0_PIPE_TXD_MAP_MASK	GENMASK(3, 0)
#define   COMB1_PHY_LN0_PIPE_TXD_MAP(n)		(((n) & 0xf)) /* 0x0: USB3.1 PIPE; 0x1: PCIE0 Lane0 */

/* SUBSYS COMB0 PHY Version Register */
#define COMB0_PHY_VER				0x1FE0
#define   PHY0_GEN_VER				GENMASK(31, 24)
#define   PHY0_SUB_VER				GENMASK(23, 16)
#define   PHY0_PRJ_VER				GENMASK(15, 8)

/* SUBSYS COMB1 PHY Version Register */
#define COMB1_PHY_VER				0x1FE4
#define   PHY1_GEN_VER				GENMASK(31, 24)
#define   PHY1_SUB_VER				GENMASK(23, 16)
#define   PHY1_PRJ_VER				GENMASK(15, 8)

/* SUBSYS TOP Version Register */
#define SUBSYS_TOP_VER				0x1FFC
#define   TOP_GEN_VER				GENMASK(31, 24)
#define   TOP_SUB_VER				GENMASK(23, 16)
#define   TOP_PRJ_VER				GENMASK(15, 8)

/* Registers */
#define SUBSYS_REG_PCIEBGR(top_subsys_reg)		((top_subsys_reg) \
								+ SUBSYS_PCIE_BGR)
#define SUBSYS_REG_USB3BGR(top_subsys_reg)		((top_subsys_reg) \
								+ SUBSYS_USB3P1_BGR)
#define SUBSYS_REG_DBGCTRL(top_subsys_reg)		((top_subsys_reg) \
								+ SUBSYS_DBG_CTL)

#define COMB0_REG_PHYPINS_LINK_CTRL0(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL0)

#define COMB0_REG_PHYPINS_LINK_CTRL1(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL1)

#define COMB0_REG_PHYPINS_LINK_CTRL2(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL2)

#define COMB0_REG_PHYPINS_LINK_CTRL3(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_CTRL3)

#define COMB0_REG_PHYPINS_LINK_STUS0(top_reg)			((top_reg) \
									+ COMB0_PHY_PINS_LINK_STUS0)

#define COMB1_REG_PHYPINS_LINK_CTRL0(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_CTRL0)

#define COMB1_REG_PHYPINS_LINK_CTRL1(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_CTRL1)

#define COMB1_REG_PHYPINS_LINK_CTRL3(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_CTRL3)

#define COMB1_REG_PHYPINS_LANE0_CTRL0(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LANE0_CTRL0)

#define COMB1_REG_PHYPINS_LINK_STUS0(top_reg)			((top_reg) \
									+ COMB1_PHY_PINS_LINK_STUS0)

#define USB3P1_REG_PIPE_CLK_MAP_CTRL(top_combo_reg)		((top_combo_reg) \
									+ USB3P1_PIPE_CLK_MAP_CTRL)

#define USB3P1_REG_PIPE_RXD_MAP_CTRL(top_combo_reg)		((top_combo_reg) \
									+ USB3P1_PIPE_RXD_MAP_CTRL)

#define COMB1_REG_PHYPIPE_TXDATA_MAP_CTRL(top_combo_reg)	((top_combo_reg) \
									+ COMB1_PHY_PIPE_TXDATA_MAP_CTRL)

#define COMB0_REG_PHY_VER(top_combo_reg)			((top_combo_reg) \
									+ COMB0_PHY_VER)

#define COMB1_REG_PHY_VER(top_combo_reg)			((top_combo_reg) \
									+ COMB1_PHY_VER)

#define SUBSYS_REG_TOP_VER(top_combo_reg)			((top_combo_reg) \

struct dp_signal_level {
	u16 lane_txdrv[MAX_LANE_CNT];
	u16 lane_mgnfs[MAX_LANE_CNT];
	u16 lane_cpost[MAX_LANE_CNT];
};

#define DP_TXDRV(r0, r1, r2, r3) \
	.lane_txdrv[0] = r0, \
	.lane_txdrv[1] = r1, \
	.lane_txdrv[2] = r2, \
	.lane_txdrv[3] = r3,

#define DP_MGNFS(r0, r1, r2, r3) \
	.lane_mgnfs[0] = r0, \
	.lane_mgnfs[1] = r1, \
	.lane_mgnfs[2] = r2, \
	.lane_mgnfs[3] = r3,

#define DP_CPOST(r0, r1, r2, r3) \
	.lane_cpost[0] = r0, \
	.lane_cpost[1] = r1, \
	.lane_cpost[2] = r2, \
	.lane_cpost[3] = r3,


static struct dp_signal_level dp_lv_table_5g4[MAX_SWING][MAX_PRE] = {
	/* sw-0 */
	{
		{
			/* sw-0 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0025, 0x0024, 0x0024, 0x0025)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-0 pre-1 */
			DP_TXDRV(0x00A3, 0x00A3, 0x00A3, 0x00A3)
			DP_MGNFS(0x001E, 0x001D, 0x001D, 0x001E)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-0 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0011, 0x0010, 0x0010, 0x0011)
			DP_CPOST(0x001C, 0x001C, 0x001C, 0x001C)
		},
		{
			/* sw-0 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0001, 0x0000, 0x0000, 0x0001)
			DP_CPOST(0x0027, 0x0027, 0x0027, 0x0027)
		},
	},
	/* sw-1 */
	{
		{
			/* sw-1 pre-0 */
			DP_TXDRV(0x0002, 0x0002, 0x0002, 0x0002)
			DP_MGNFS(0x001D, 0x001C, 0x001C, 0x001D)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-1 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0012, 0x0011, 0x0011, 0x0012)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-1 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0001, 0x0000, 0x0000, 0x0001)
			DP_CPOST(0x001E, 0x001E, 0x001E, 0x001E)
		},
		{
			/* sw-1 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0001, 0x0000, 0x0000, 0x0001)
			DP_CPOST(0x001E, 0x001E, 0x001E, 0x001E)
		},
	},
	/* sw-2 */
	{
		{
			/* sw-2 pre-0 */
			DP_TXDRV(0x0002, 0x0002, 0x0002, 0x0002)
			DP_MGNFS(0x0010, 0x000F, 0x000F, 0x0010)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-2 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x000F, 0x000F, 0x000F, 0x000F)
		},
		{
			/* sw-2 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x000F, 0x000F, 0x000F, 0x000F)
		},
		{
			/* sw-2 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x000F, 0x000F, 0x000F, 0x000F)
		},
	},
	/* sw-3 */
	{
		{
			/* sw-3 pre-0 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
	},

};

static struct dp_signal_level dp_lv_table_2g7[MAX_SWING][MAX_PRE] = {
	/* sw-0 */
	{
		{
			/* sw-0 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0028, 0x0026, 0x0026, 0x0028)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-0 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0024, 0x0023, 0x0023, 0x0024)
			DP_CPOST(0x0008, 0x0008, 0x0008, 0x0008)
		},
		{
			/* sw-0 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x001C, 0x001B, 0x001B, 0x001C)
			DP_CPOST(0x0016, 0x0016, 0x0016, 0x0016)
		},
		{
			/* sw-0 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000B, 0x000A, 0x000A, 0x000B)
			DP_CPOST(0x0023, 0x0023, 0x0023, 0x0023)
		},
	},
	/* sw-1 */
	{
		{
			/* sw-1 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x001F, 0x001E, 0x001E, 0x001F)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-1 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x001A, 0x0019, 0x0019, 0x001A)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-1 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000A, 0x0009, 0x0009, 0x000A)
			DP_CPOST(0x0019, 0x0019, 0x0019, 0x0019)
		},
		{
			/* sw-1 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000A, 0x0009, 0x0009, 0x000A)
			DP_CPOST(0x0019, 0x0019, 0x0019, 0x0019)
		},
	},
	/* sw-2 */
	{
		{
			/* sw-2 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0010, 0x000E, 0x000E, 0x0010)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-2 pre-1 */
			DP_TXDRV(0x0082, 0x0082, 0x0082, 0x0082)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0011, 0x0011, 0x0011, 0x0011)
		},
		{
			/* sw-2 pre-2 */
			DP_TXDRV(0x0082, 0x0082, 0x0082, 0x0082)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0011, 0x0011, 0x0011, 0x0011)
		},
		{
			/* sw-2 pre-3 */
			DP_TXDRV(0x0082, 0x0082, 0x0082, 0x0082)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0011, 0x0011, 0x0011, 0x0011)
		},
	},
	/* sw-3 */
	{
		{
			/* sw-3 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-1 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-2 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-3 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
	},

};

static struct dp_signal_level dp_lv_table_1g62[MAX_SWING][MAX_PRE] = {
	/* sw-0 */
	{
		{
			/* sw-0 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0027, 0x0027, 0x0027, 0x0027)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-0 pre-1 */
			DP_TXDRV(0x00A2, 0x00A2, 0x00A2, 0x00A2)
			DP_MGNFS(0x0023, 0x0023, 0x0023, 0x0023)
			DP_CPOST(0x000A, 0x000A, 0x000A, 0x000A)
		},
		{
			/* sw-0 pre-2 */
			DP_TXDRV(0x0092, 0x0092, 0x0092, 0x0092)
			DP_MGNFS(0x001A, 0x001A, 0x001A, 0x001A)
			DP_CPOST(0x0016, 0x0016, 0x0016, 0x0016)
		},
		{
			/* sw-0 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000E, 0x000E, 0x000E, 0x000E)
			DP_CPOST(0x0020, 0x0020, 0x0020, 0x0020)
		},
	},
	/* sw-1 */
	{
		{
			/* sw-1 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0020, 0x0020, 0x0020, 0x0020)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-1 pre-1 */
			DP_TXDRV(0x00A2, 0x00A2, 0x00A2, 0x00A2)
			DP_MGNFS(0x0019, 0x0018, 0x0018, 0x0019)
			DP_CPOST(0x000C, 0x000C, 0x000C, 0x000C)
		},
		{
			/* sw-1 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000D, 0x000C, 0x000C, 0x000D)
			DP_CPOST(0x0017, 0x0017, 0x0017, 0x0017)
		},
		{
			/* sw-1 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x000D, 0x000C, 0x000C, 0x000D)
			DP_CPOST(0x0017, 0x0017, 0x0017, 0x0017)
		},
	},
	/* sw-2 */
	{
		{
			/* sw-2 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0010, 0x000F, 0x000F, 0x0010)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-2 pre-1 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0004, 0x0003, 0x0003, 0x0004)
			DP_CPOST(0x0010, 0x0010, 0x0010, 0x0010)
		},
		{
			/* sw-2 pre-2 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0004, 0x0003, 0x0003, 0x0004)
			DP_CPOST(0x0010, 0x0010, 0x0010, 0x0010)
		},
		{
			/* sw-2 pre-3 */
			DP_TXDRV(0x00B2, 0x00B2, 0x00B2, 0x00B2)
			DP_MGNFS(0x0004, 0x0003, 0x0003, 0x0004)
			DP_CPOST(0x0010, 0x0010, 0x0010, 0x0010)
		},
	},
	/* sw-3 */
	{
		{
			/* sw-3 pre-0 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-1 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-2 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
		{
			/* sw-3 pre-3 */
			DP_TXDRV(0x0003, 0x0003, 0x0003, 0x0003)
			DP_MGNFS(0x0000, 0x0000, 0x0000, 0x0000)
			DP_CPOST(0x0000, 0x0000, 0x0000, 0x0000)
		},
	},

};

/*
static void sunxi_cadence_phy_pcie_phy_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	struct sunxi_cadence_combophy *combo1 = sunxi_cphy->combo1;
	u32 val;

	writel(0x01100001, combo1->top_reg + 0x4);

	writew(0x1a, combo1->phy_reg + 0x44);

	writew(0x34, combo1->phy_reg + 0x54);
	writew(0xda, combo1->phy_reg + 0x58);

	writew(0x34, combo1->phy_reg + 0x64);
	writew(0xda, combo1->phy_reg + 0x68);

	writew(0x82, combo1->phy_reg + 0xc8);
	writew(0x82, combo1->phy_reg + 0xca);

	writew(0x1a, combo1->phy_reg + 0xe8);

	writew(0x20, combo1->phy_reg + 0x208);
	writew(0x7, combo1->phy_reg + 0x20a);
	writew(0x20, combo1->phy_reg + 0x218);
	writew(0x7, combo1->phy_reg + 0x21a);
	writew(0x30c, combo1->phy_reg + 0x228);
	writew(0x7, combo1->phy_reg + 0x22a);

	writew(0x7, combo1->phy_reg + 0x248);
	writew(0x3, combo1->phy_reg + 0x24a);
	writew(0xf, combo1->phy_reg + 0x24c);
	writew(0x132, combo1->phy_reg + 0x250);

	writew(0x208, combo1->phy_reg + 0x8180);
	writew(0x9c, combo1->phy_reg + 0x8184);

	writew(0x1a, combo1->phy_reg + 0x10088);
	writew(0x82, combo1->phy_reg + 0x1008a);
	writew(0x1a, combo1->phy_reg + 0x10098);
	writew(0x82, combo1->phy_reg + 0x1009a);

	writew(0xa28, combo1->phy_reg + 0x8246);

	val = readl(combo1->top_reg + 0x100);
	val &= ~0x30;
	writel(val, combo1->top_reg + 0x100);

	writew(0x4, combo1->phy_reg + 0x128);
	writew(0x4, combo1->phy_reg + 0x148);
	writew(0x4, combo1->phy_reg + 0x1a8);

	writew(0x509, combo1->phy_reg + 0x348);
	writew(0x509, combo1->phy_reg + 0x368);
	writew(0x509, combo1->phy_reg + 0x388);
	writew(0xf00, combo1->phy_reg + 0x34a);
	writew(0xf00, combo1->phy_reg + 0x36a);
	writew(0xf00, combo1->phy_reg + 0x38a);
	writew(0xf08, combo1->phy_reg + 0x34c);
	writew(0xf08, combo1->phy_reg + 0x36c);
	writew(0xf08, combo1->phy_reg + 0x38c);

	writew(0x180, combo1->phy_reg + 0x120);
	writew(0x133, combo1->phy_reg + 0x140);
	writew(0x133, combo1->phy_reg + 0x1a0);
	writew(0x9d8a, combo1->phy_reg + 0x122);
	writew(0xb13b, combo1->phy_reg + 0x142);
	writew(0xb13b, combo1->phy_reg + 0x1a2);
	writew(0x2, combo1->phy_reg + 0x124);
	writew(0x2, combo1->phy_reg + 0x144);
	writew(0x2, combo1->phy_reg + 0x1a4);
	writew(0x102, combo1->phy_reg + 0x126);
	writew(0xce, combo1->phy_reg + 0x146);
	writew(0xce, combo1->phy_reg + 0x1a6);

	writew(0x22, combo1->phy_reg + 0x340);
	writew(0x22, combo1->phy_reg + 0x360);
	writew(0x22, combo1->phy_reg + 0x380);

	writew(0x1, combo1->phy_reg + 0x130);
	writew(0x1, combo1->phy_reg + 0x150);
	writew(0x1, combo1->phy_reg + 0x1b0);
	writew(0x45f, combo1->phy_reg + 0x132);
	writew(0x399, combo1->phy_reg + 0x152);
	writew(0x399, combo1->phy_reg + 0x1b2);
	writew(0x6b, combo1->phy_reg + 0x134);
	writew(0x68, combo1->phy_reg + 0x154);
	writew(0x68, combo1->phy_reg + 0x1b4);
	writew(0x4, combo1->phy_reg + 0x136);
	writew(0x4, combo1->phy_reg + 0x156);
	writew(0x4, combo1->phy_reg + 0x1b6);

	writew(0x104, combo1->phy_reg + 0x108);
	writew(0x104, combo1->phy_reg + 0x188);
	writew(0x5, combo1->phy_reg + 0x10a);
	writew(0x5, combo1->phy_reg + 0x18a);
	writew(0x337, combo1->phy_reg + 0x10c);
	writew(0x337, combo1->phy_reg + 0x18c);
	writew(0x3dbe, combo1->phy_reg + 0x110);
	writew(0x3dbe, combo1->phy_reg + 0x190);
	writew(0x3, combo1->phy_reg + 0x104);
	writew(0x3, combo1->phy_reg + 0x184);

	writew(0x14, combo1->phy_reg + 0x138);
	writew(0x14, combo1->phy_reg + 0x1b8);
	writew(0x192, combo1->phy_reg + 0x13c);
	writew(0x192, combo1->phy_reg + 0x1bc);
	writew(0x6, combo1->phy_reg + 0x13e);
	writew(0x6, combo1->phy_reg + 0x1be);

	writew(0x0, combo1->phy_reg + 0x103c0);
	writew(0x19, combo1->phy_reg + 0x102e2);
	writew(0x19, combo1->phy_reg + 0x102e4);

	writew(0x1, combo1->phy_reg + 0x103fe);

	val = readl(combo1->phy_reg + 0x98);
	val &= ~0x3;
	val |= 0x2;
	writel(val, combo1->phy_reg + 0x98);

	val = readl(combo1->phy_reg + 0xa8);
	val &= ~0x3;
	val |= 0x2;
	writel(val, combo1->phy_reg + 0xa8);

	val = readl(combo1->phy_reg + 0xd8);
	val &= ~0x3;
	val |= 0x2;
	writel(val, combo1->phy_reg + 0xd8);

	val = readl(combo1->top_reg);
	writel(val | 0x1, combo1->top_reg);
	val = readl(combo1->top_reg + 0x100);
	writel(val | 0x1, combo1->top_reg + 0x100);

	val = readl(combo1->top_reg + 0x900);
	while (true) {
		udelay(100);
		val = readl(combo1->top_reg + 0x900);
		if (val & BIT(0))
			break;
	}

	val = readw(combo1->phy_reg + 0xa0);
	val |= 0x220;
	val &= ~0x2;
	val = 0x270;
	writew(val, combo1->phy_reg + 0xa0);

	val = readw(combo1->phy_reg + 0x98);
	val |= BIT(4);
	writew(val, combo1->phy_reg + 0x98);

	val = readw(combo1->phy_reg + 0x18000);
	val |= BIT(0);
	writew(val, combo1->phy_reg + 0x18000);

	val = readl(combo1->top_reg + 0x4);
	val |= BIT(28);
	writel(val, combo1->top_reg + 0x4);

}
*/
/*
static int sunxi_cadence_phy_combo1_pcie_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	int ret;
	u32 val;

	ret = clk_set_rate(sunxi_cphy->serdes_clk, 100000000);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sunxi_cphy->serdes_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sunxi_cphy->dcxo_serdes1_clk);
	if (ret)
		return ret;

	val = readl(sunxi_cphy->top_subsys_reg + SUBSYS_PCIE_BGR);
	writel(val | SUBSYS_PCIE_GATING, sunxi_cphy->top_subsys_reg + SUBSYS_PCIE_BGR);

	val = readl(sunxi_cphy->top_subsys_reg + SUBSYS_DBG_CTL);
	writel(val | SUBSYS_DISABLE_COMBO1_AUTOGATING, sunxi_cphy->top_subsys_reg + SUBSYS_DBG_CTL);

	writel(SUBSYS_COMB1_PIPE_PCIE, sunxi_cphy->top_combo_reg + SUBSYS_COMB1_PIPE);

	sunxi_cadence_phy_pcie_phy_init(sunxi_cphy);

	return 0;
}
*/

/***************************************************************************************************
 * Combo PHY0 Support.                                                                             *
 * - PIPE interface mode  USB3.1 SS and SS+                                                        *
 * - Raw SerDes interface mode  DisplayPort Tx / embedded DisplayPort Tx                           *
 * - Supports up to 4Lane                                                                          *
 **************************************************************************************************/
void combo0_dp_phy_reset(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	u32 phy0_pma_cmn_ready = 0;
	u32 reg_val;

	//PHY RESET
	phy_writel(combo0->top_reg + 0x00, 0x0);

	//PHY RESET
	phy_writel(combo0->top_reg + 0x00, 0x001);
	msleep(1);
	//DP LINK RESET
	phy_writel(combo0->top_reg + 0x00, 0x101);
	msleep(1);
	//PHY RESET
	phy_writel(combo0->top_reg + 0x00, 0x131);
	msleep(1);

	reg_val = readl(COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));
	if (combo0->orientation == ORIENTATION_REVERSE)
		reg_val |= PHY0_TYPEC_CONN_DIR;
	else
		reg_val &= ~PHY0_TYPEC_CONN_DIR;
	writel(reg_val, COMB0_REG_PHYPINS_LINK_CTRL0(combo0->top_reg));

	msleep(1);

	while (phy0_pma_cmn_ready == 0) {
		reg_val = phy_readl(combo0->top_reg+0x900);
		reg_val = reg_val & (1 << 0);
		phy0_pma_cmn_ready = reg_val;
	}

	phy_set_lbits(sunxi_cphy->top_combo_reg + 0x4, 0, 1, 1);
	phy_set_lbits(sunxi_cphy->top_combo_reg + 0x4, 4, 4, 0xf);
	phy_set_lbits(sunxi_cphy->top_combo_reg + 0x4, 8, 4, 0xf);

	phy_set_lbits(combo0->top_reg + 0x0c, 0, 1, 1);

	phy0_pma_cmn_ready = 0;
	while (phy0_pma_cmn_ready == 0) {
		reg_val = phy_readl(combo0->top_reg+0x900);
		reg_val = (reg_val & (1 << 4)) >> 4;
		phy0_pma_cmn_ready = reg_val;
	}

	msleep(1);

	phy_set_lbits(combo0->top_reg + 0x0c, 4, 6, 1);

	phy0_pma_cmn_ready = 0;
	while (phy0_pma_cmn_ready == 0) {
		reg_val = phy_readl(combo0->top_reg+0x900);
		reg_val = (reg_val & (0x3f << 8)) >> 8;
		phy0_pma_cmn_ready = reg_val;
	}
}

/*
static void sunxi_cadence_phy_combo1_pcie_exit(struct sunxi_cadence_phy *sunxi_cphy)
{
	clk_disable(sunxi_cphy->serdes_clk);
	clk_disable(sunxi_cphy->dcxo_serdes1_clk);
}
*/

static int combo0_dp_set_rate(struct sunxi_cadence_combophy *combo0,
		       struct phy_configure_opts_dp *dp_config)
{
	if (dp_config->set_rate == 1) {
		combo0->link_rate = dp_config->link_rate;
		switch (dp_config->link_rate) {
		case 1620:
			// set ref-clk to 26M
			phy_writel(combo0->top_reg + 0x4, 0x1100001);

			//CMN_SSM_BIAS_TMR
			phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001a);
			//CMN_PLLSM0_PLLPRE_TMR
			phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
			//CMN_PLLSM0_PLLLOCK_TMR
			phy_writew(combo0->phy_reg + (0x002C << 1), 0x00da);

			//CMN_PLLSM1_PLLPRE_TMR
			phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
			//CMN_PLLSM1_PLLLOCK_TMR
			phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00da);

			//CMN_BGCAL_INIT_TMR
			phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
			//CMN_BGCAL_ITER_TMR
			phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
			//CMN_IBCAL_INIT_TMR
			phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
			phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x010c << 1), 0x0020);
			phy_writew(combo0->phy_reg + (0x010d << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030c);
			phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000f);
			phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);

			phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001a);

			phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);

			phy_writew(combo0->phy_reg + (0x804c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x824c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x844c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x864c << 1), 0x001a);

			phy_writew(combo0->phy_reg + (0x804d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x824d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x844d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x864d << 1), 0x0082);

			phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0a28);

			//VCO calibration settings.
			phy_writew(combo0->phy_reg + (0x00c4 << 1), 0x0104);
			phy_writew(combo0->phy_reg + (0x00c5 << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x00c6 << 1), 0x0337);
			phy_writew(combo0->phy_reg + (0x00c8 << 1), 0x0335);
			phy_writew(combo0->phy_reg + (0x00c2 << 1), 0x0003);

			phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
			phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
			phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
			phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);

			 //PLL lock detect settings
			phy_writew(combo0->phy_reg + (0x00dc << 1), 0x00cf);
			phy_writew(combo0->phy_reg + (0x00de << 1), 0x00ce);
			phy_writew(combo0->phy_reg + (0x00dF << 1), 0x0005);

			phy_writew(combo0->phy_reg + (0x009c << 1), 0x00cf);
			phy_writew(combo0->phy_reg + (0x009e << 1), 0x00ce);
			phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);

			//DP_LANE_SET as follow
			phy_writew(combo0->phy_reg + (0xc010 << 1), 0x51d9);
			//phy DP lane enabled
			phy_writew(combo0->phy_reg + (0xc011 << 1), 0x0100);

			//PHY_PMA_ISO_PLL_CTRL0   cmn_pll1_clk_en
			phy_writew(combo0->phy_reg + (0xe003 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0xe005 << 1), 0x000B);
			//PHY_PMA_ISO_PLL_CTRL1
			phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

			//TX_PSC_A0
			phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);

			//TX_PSC_A2
			phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

			//TX_PSC_A3
			phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

			//RX all set to 0
			//RX_PSC_A0
			phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0);

			//RX_PSC_A0
			phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0);

			//RX_PSC_A0
			phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0);

			//RX_PSC_CAL
			phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0);

			//RX_REE_GCSM1_CTRL
			phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0);

			//RX_REE_GCSM2_CTRL
			phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0);

			//RX_REE_PERGCSM_CTRL
			phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0);

			//XCVR_DIAG_BIDI_CTRL  diefferent mode with TX/RX enable
			phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);

			//TX_TXCC_CTRL
			phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);

			//DRV_DIAG_TX_DRV
			phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);

			phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);

			phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);

			//CMN-pull up-down res cal
			phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
			phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

			phy_writew(combo0->phy_reg + (0x4140 << 1), 0x0801);
			phy_writew(combo0->phy_reg + (0x4340 << 1), 0x0801);
			phy_writew(combo0->phy_reg + (0x4540 << 1), 0x0801);
			phy_writew(combo0->phy_reg + (0x4740 << 1), 0x0801);

			//ssc
			phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
			phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
			phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);

			phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x044A);
			phy_writew(combo0->phy_reg + (0x009A << 1), 0x006b);
			phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);

			// pll configure
			phy_writew(combo0->phy_reg + (0x00d4 << 1), 0x0004);
			phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
			phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
			phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);

			phy_writew(combo0->phy_reg + (0x00d0 << 1), 0x0175);
			phy_writew(combo0->phy_reg + (0x00d1 << 1), 0xD89E);
			phy_writew(combo0->phy_reg + (0x00d2 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x00d3 << 1), 0x00FA);
			phy_writew(combo0->phy_reg + (0x01c0 << 1), 0x0002);

			//SET common  PLL1_CLOCK0
			phy_writew(combo0->phy_reg + (0x1c1 << 1), 0x0F01);
			phy_writew(combo0->phy_reg + (0x004B << 1), 0x0);

			//dp XCVR(LANE) pll1 seting
			phy_writew(combo0->phy_reg + (0x40e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x42e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x44e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x46e6 << 1), 0x0001);

			phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x40e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x42e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x44e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x46e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

			//PLL0 configure
			phy_writew(combo0->phy_reg + (0x1A1 << 1), 0x8600);
			phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
			phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
			phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
			phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
			phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
			phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9DBA);
			phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
			phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
			break;
		case 2160:
			break;
		case 2430:
			break;
		case 2700:
			phy_writel(combo0->top_reg + 0x4, 0x1100001);

			//CMN_SSM_BIAS_TMR
			phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001a);

			//CMN_PLLSM0_PLLPRE_TMR
			phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
			//CMN_PLLSM0_PLLLOCK_TMR
			phy_writew(combo0->phy_reg + (0x002C << 1), 0x00da);

			//CMN_PLLSM1_PLLPRE_TMR
			phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
			//CMN_PLLSM1_PLLLOCK_TMR
			phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00da);

			//CMN_BGCAL_INIT_TMR
			phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
			//CMN_BGCAL_ITER_TMR
			phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
			//CMN_IBCAL_INIT_TMR
			phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
			phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x010c << 1), 0x0020);
			phy_writew(combo0->phy_reg + (0x010d << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030c);
			phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000f);
			phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);

			phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001a);

			phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);

			phy_writew(combo0->phy_reg + (0x804c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x824c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x844c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x864c << 1), 0x001a);

			phy_writew(combo0->phy_reg + (0x804d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x824d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x844d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x864d << 1), 0x0082);

			phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0a28);

			//VCO calibration settings.
			phy_writew(combo0->phy_reg + (0x00c4 << 1), 0x0104);
			phy_writew(combo0->phy_reg + (0x00c5 << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x00c6 << 1), 0x0337);
			phy_writew(combo0->phy_reg + (0x00c8 << 1), 0x0335);
			phy_writew(combo0->phy_reg + (0x00c2 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
			phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
			phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
			phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);

			//PLL lock detect settings
			phy_writew(combo0->phy_reg + (0x00dc << 1), 0x00cf);
			phy_writew(combo0->phy_reg + (0x00de << 1), 0x00ce);
			phy_writew(combo0->phy_reg + (0x00dF << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x009c << 1), 0x00cf);
			phy_writew(combo0->phy_reg + (0x009e << 1), 0x00ce);
			phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);

			//DP_LANE_SET
			phy_writew(combo0->phy_reg + (0xc010 << 1), 0x51d9);
			phy_writew(combo0->phy_reg + (0xc011 << 1), 0x0100);
			//PHY_PMA_ISO_PLL_CTRL0   cmn_pll1_clk_en
			phy_writew(combo0->phy_reg + (0xe003 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0xe005 << 1), 0x000B);
			//PHY_PMA_ISO_PLL_CTRL1
			phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);
			//TX_PSC_A0
			phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);

			//TX_PSC_A2
			phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

			//TX_PSC_A3
			phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

			//RX all set to 0
			phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0);

			//XCVR_DIAG_BIDI_CTRL
			phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);

			//TX_TXCC_CTRL
			phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);

			//DRV_DIAG_TX_DRV
			phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);

			phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);

			phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);

			//CMN-pull up-down res cal
			phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
			phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

			//ssc
			phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
			phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
			phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);

			phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x045f);
			phy_writew(combo0->phy_reg + (0x009A << 1), 0x006b);
			phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);

			// pll configure
			phy_writew(combo0->phy_reg + (0x00d4 << 1), 0x0004);
			phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
			phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
			phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);

			phy_writew(combo0->phy_reg + (0x00d0 << 1), 0x019f);
			phy_writew(combo0->phy_reg + (0x00d1 << 1), 0x6276);
			phy_writew(combo0->phy_reg + (0x00d2 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x00d3 << 1), 0x0116);
			phy_writew(combo0->phy_reg + (0x01c0 << 1), 0x0002);

			phy_writew(combo0->phy_reg + (0x1c1 << 1), 0x0701);
			phy_writew(combo0->phy_reg + (0x004B << 1), 0x0);

			//dp XCVR(LANE) pll1 seting
			phy_writew(combo0->phy_reg + (0x40e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x42e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x44e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x46e6 << 1), 0x0001);

			phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0001);

			phy_writew(combo0->phy_reg + (0x40e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x42e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x44e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x46e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

			// pll0 configure
			phy_writew(combo0->phy_reg + (0x1A1 << 1), 0x8600);
			phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
			phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
			phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
			phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
			phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
			phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9DBA);
			phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
			phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
			break;
		case 3240:
			break;
		case 4320:
			break;
		case 5400:
			phy_writel(combo0->top_reg + 0x4, 0x1100001);

			//CMN_SSM_BIAS_TMR
			phy_writew(combo0->phy_reg + (0x0022 << 1), 0x001a);
			//CMN_PLLSM0_PLLPRE_TMR
			phy_writew(combo0->phy_reg + (0x002A << 1), 0x0034);
			//CMN_PLLSM0_PLLLOCK_TMR
			phy_writew(combo0->phy_reg + (0x002C << 1), 0x00da);
			//CMN_PLLSM1_PLLPRE_TMR
			phy_writew(combo0->phy_reg + (0x0032 << 1), 0x0034);
			//CMN_PLLSM1_PLLLOCK_TMR
			phy_writew(combo0->phy_reg + (0x0034 << 1), 0x00da);
			//CMN_BGCAL_INIT_TMR
			phy_writew(combo0->phy_reg + (0x0064 << 1), 0x0082);
			//CMN_BGCAL_ITER_TMR
			phy_writew(combo0->phy_reg + (0x0065 << 1), 0x0082);
			//CMN_IBCAL_INIT_TMR
			phy_writew(combo0->phy_reg + (0x0074 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x0104 << 1), 0x0020);
			phy_writew(combo0->phy_reg + (0x0105 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x010c << 1), 0x0020);
			phy_writew(combo0->phy_reg + (0x010d << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0114 << 1), 0x030c);
			phy_writew(combo0->phy_reg + (0x0115 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0124 << 1), 0x0007);
			phy_writew(combo0->phy_reg + (0x0125 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x0126 << 1), 0x000f);
			phy_writew(combo0->phy_reg + (0x0128 << 1), 0x0132);
			phy_writew(combo0->phy_reg + (0x8044 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8244 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8444 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8644 << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x8045 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8245 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8445 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x8645 << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x804c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x824c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x844c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x864c << 1), 0x001a);
			phy_writew(combo0->phy_reg + (0x804d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x824d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x844d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x864d << 1), 0x0082);
			phy_writew(combo0->phy_reg + (0x4123 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4323 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4523 << 1), 0x0a28);
			phy_writew(combo0->phy_reg + (0x4723 << 1), 0x0a28);

			//VCO calibration settings.
			phy_writew(combo0->phy_reg + (0x00c4 << 1), 0x0104);
			phy_writew(combo0->phy_reg + (0x00c5 << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x00c6 << 1), 0x0337);
			phy_writew(combo0->phy_reg + (0x00c8 << 1), 0x0335);
			phy_writew(combo0->phy_reg + (0x00c2 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x0084 << 1), 0x0104);
			phy_writew(combo0->phy_reg + (0x0085 << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x0086 << 1), 0x0337);
			phy_writew(combo0->phy_reg + (0x0088 << 1), 0x0335);
			phy_writew(combo0->phy_reg + (0x0082 << 1), 0x0003);

			//PLL lock detect settings
			phy_writew(combo0->phy_reg + (0x00dc << 1), 0x00cf);
			phy_writew(combo0->phy_reg + (0x00de << 1), 0x00ce);
			phy_writew(combo0->phy_reg + (0x00dF << 1), 0x0005);
			phy_writew(combo0->phy_reg + (0x009c << 1), 0x00cf);
			phy_writew(combo0->phy_reg + (0x009e << 1), 0x00ce);
			phy_writew(combo0->phy_reg + (0x009F << 1), 0x0005);

			//DP_LANE_SET
			//lane mapping PHY_PMA_LANE_MAP
			phy_writew(combo0->phy_reg + (0xc010 << 1), 0x51d9);
			//phy DP lane enabled
			phy_writew(combo0->phy_reg + (0xc011 << 1), 0x0100);
			//PHY_PMA_ISO_PLL_CTRL0   cmn_pll1_clk_en
			phy_writew(combo0->phy_reg + (0xe003 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0xe005 << 1), 0x000B);
			//PHY_PMA_ISO_PLL_CTRL1
			phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);
			//TX_PSC_A0
			phy_writew(combo0->phy_reg + (0x4100 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4300 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4500 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4700 << 1), 0x00FB);
			phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4102 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4302 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4502 << 1), 0x04AA);
			phy_writew(combo0->phy_reg + (0x4702 << 1), 0x04AA);

			//RX all set to 0
			phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8200 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8400 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8600 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8000 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8202 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8402 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8602 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8002 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8203 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8403 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8603 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8006 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8206 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8406 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8606 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8108 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8308 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8508 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8708 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8110 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8310 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8510 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8710 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8118 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8318 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8518 << 1), 0x0);
			phy_writew(combo0->phy_reg + (0x8718 << 1), 0x0);

			//XCVR_DIAG_BIDI_CTRL
			phy_writew(combo0->phy_reg + (0x40EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x42EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x44EA << 1), 0x000F);
			phy_writew(combo0->phy_reg + (0x46EA << 1), 0x000F);

			//TX_TXCC_CTRL
			phy_writew(combo0->phy_reg + (0x4040 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4240 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4440 << 1), 0x08A4);
			phy_writew(combo0->phy_reg + (0x4640 << 1), 0x08A4);

			//DRV_DIAG_TX_DRV
			phy_writew(combo0->phy_reg + (0x40C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x42C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x44C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x46C6 << 1), 0x0003);
			phy_writew(combo0->phy_reg + (0x4050 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4250 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4450 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x4650 << 1), 0x002A);
			phy_writew(combo0->phy_reg + (0x404C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x424C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x444C << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x464C << 1), 0x0000);

			//CMN-pull-up-down res cal
			phy_writew(combo0->phy_reg + (0x0103 << 1), 0x007F);
			phy_writew(combo0->phy_reg + (0x010B << 1), 0x007F);

			//ssc
			phy_writew(combo0->phy_reg + (0x00D8 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x04C4);
			phy_writew(combo0->phy_reg + (0x00DA << 1), 0x006A);
			phy_writew(combo0->phy_reg + (0x00DB << 1), 0x0004);

			phy_writew(combo0->phy_reg + (0x0098 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x00D9 << 1), 0x045f);
			phy_writew(combo0->phy_reg + (0x009A << 1), 0x006b);
			phy_writew(combo0->phy_reg + (0x009B << 1), 0x0004);

			// pll configure
			phy_writew(combo0->phy_reg + (0x00d4 << 1), 0x0004);
			phy_writew(combo0->phy_reg + (0x01C4 << 1), 0x0509);
			phy_writew(combo0->phy_reg + (0x01C5 << 1), 0x0F00);
			phy_writew(combo0->phy_reg + (0x01C6 << 1), 0x0F08);

			phy_writew(combo0->phy_reg + (0x00d0 << 1), 0x019f);
			phy_writew(combo0->phy_reg + (0x00d1 << 1), 0x6276);
			phy_writew(combo0->phy_reg + (0x00d2 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x00d3 << 1), 0x0116);
			phy_writew(combo0->phy_reg + (0x01c0 << 1), 0x0002);

			//SET common  PLL1_CLOCK0
			phy_writew(combo0->phy_reg + (0x1c1 << 1), 0x0601);
			phy_writew(combo0->phy_reg + (0x004B << 1), 0x0);

			//dp XCVR(LANE) pll1 seting
			phy_writew(combo0->phy_reg + (0x40e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x42e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x44e6 << 1), 0x0001);
			phy_writew(combo0->phy_reg + (0x46e6 << 1), 0x0001);

			phy_writew(combo0->phy_reg + (0x40e7 << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x42e7 << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x44e7 << 1), 0x0000);
			phy_writew(combo0->phy_reg + (0x46e7 << 1), 0x0000);

			phy_writew(combo0->phy_reg + (0x40e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x42e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x44e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0x46e5 << 1), 0x0019);
			phy_writew(combo0->phy_reg + (0xe006 << 1), 0x2224);

			phy_writew(combo0->phy_reg + (0x1A1 << 1), 0x8600);
			phy_writew(combo0->phy_reg + (0x0094 << 1), 0x0004);
			phy_writew(combo0->phy_reg + (0x01A4 << 1), 0x0509);
			phy_writew(combo0->phy_reg + (0x01A5 << 1), 0x0F00);
			phy_writew(combo0->phy_reg + (0x01A6 << 1), 0x0F08);
			phy_writew(combo0->phy_reg + (0x0090 << 1), 0x0180);
			phy_writew(combo0->phy_reg + (0x0091 << 1), 0x9DBA);
			phy_writew(combo0->phy_reg + (0x0092 << 1), 0x0002);
			phy_writew(combo0->phy_reg + (0x0093 << 1), 0x0102);
			phy_writew(combo0->phy_reg + (0x01A0 << 1), 0x0002);
			break;
		case 8100:
			break;
		default:
			pr_err("[sunxi cadence phy]: not support rate(%d Mb/s) for dp\n", dp_config->link_rate);
			return -1;
		}

		combo0_dp_phy_reset(combo0);
	}

	return 0;
}

static int combo0_dp_set_lanes(struct sunxi_cadence_combophy *combo0,
			       struct phy_configure_opts_dp *dp_config)
{
	if (dp_config->set_lanes == 1) {
		switch (dp_config->lanes) {
		case 1:
			// close lane 321
			phy_writew(combo0->phy_reg + (0x4011 << 1), 0x010E);
			break;
		case 2:
			// close lane 32
			phy_writew(combo0->phy_reg + (0x4011 << 1), 0x010C);
			break;
		case 4:
			phy_writew(combo0->phy_reg + (0x4011 << 1), 0x0100);
			break;
		default:
			pr_err("[sunxi cadence phy]: not support lanes(%d) for dp\n", dp_config->lanes);
			return -1;;
		}
	}

	return 0;
}

static int combo0_dp_set_sw_pre(struct sunxi_cadence_combophy *combo0,
			       struct phy_configure_opts_dp *dp_config)
{
	int i = 0;
	u16 lane_txdrv = 0;
	u16 lane_mgnfs = 0;
	u16 lane_cpost = 0;
	unsigned int sw = 0;
	unsigned int pre = 0;

	if (dp_config->set_voltages == 1) {
		for (i = 0; i < dp_config->lanes; i++) {
			sw = dp_config->voltage[i];
			pre = dp_config->pre[i];

			switch (combo0->link_rate) {
			case 5400:
				lane_txdrv = dp_lv_table_5g4[sw][pre].lane_txdrv[i];
				lane_mgnfs = dp_lv_table_5g4[sw][pre].lane_mgnfs[i];
				lane_cpost = dp_lv_table_5g4[sw][pre].lane_cpost[i];
				break;
			case 2700:
				lane_txdrv = dp_lv_table_2g7[sw][pre].lane_txdrv[i];
				lane_mgnfs = dp_lv_table_2g7[sw][pre].lane_mgnfs[i];
				lane_cpost = dp_lv_table_2g7[sw][pre].lane_cpost[i];
				break;
			case 1620:
				lane_txdrv = dp_lv_table_1g62[sw][pre].lane_txdrv[i];
				lane_mgnfs = dp_lv_table_1g62[sw][pre].lane_mgnfs[i];
				lane_cpost = dp_lv_table_1g62[sw][pre].lane_cpost[i];
				break;
			default:
				pr_err("[sunxi cadence phy]: not support rate(%d Mb/s) for dp signal level\n",
				       dp_config->link_rate);
				return -1;
			}
			phy_writew(combo0->phy_reg + (0x818C + (i * 0x400)), lane_txdrv);
			phy_writew(combo0->phy_reg + (0x80A0 + (i * 0x400)), lane_mgnfs);
			phy_writew(combo0->phy_reg + (0x8098 + (i * 0x400)), lane_cpost);
		}
	}

	return 0;
}

static void combo0_dp_phy_init(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	if (sunxi_cphy->serdes_clk)
		clk_prepare_enable(sunxi_cphy->serdes_clk);
	if (sunxi_cphy->dcxo_serdes0_clk)
		clk_prepare_enable(sunxi_cphy->dcxo_serdes0_clk);
	if (combo0->clk)
		clk_prepare_enable(combo0->clk);
}

static void combo0_dp_phy_exit(struct sunxi_cadence_combophy *combo0)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;

	if (combo0->clk)
		clk_disable(combo0->clk);

	if (sunxi_cphy->serdes_clk)
		clk_disable(sunxi_cphy->serdes_clk);

	if (sunxi_cphy->dcxo_serdes0_clk)
		clk_disable(sunxi_cphy->dcxo_serdes0_clk);
}

static void combo0_dp_lane_remap(struct sunxi_cadence_combophy *combo0,
				 enum phy_orientation orientation)
{
	struct sunxi_cadence_phy *sunxi_cphy = combo0->sunxi_cphy;
	int val = 0;

	//set phy's lane remap
	if (orientation != ORIENTATION_REVERSE) {
		val |= combo0->lane_remap[0] << 0;
		val |= combo0->lane_remap[1] << 4;
		val |= combo0->lane_remap[2] << 8;
		val |= combo0->lane_remap[3] << 12;
	} else {
		val |= combo0->lane_remap[3] << 0;
		val |= combo0->lane_remap[2] << 4;
		val |= combo0->lane_remap[1] << 8;
		val |= combo0->lane_remap[0] << 12;
	}
	phy_writel(sunxi_cphy->top_combo_reg + 0xc24, val);
}

static void combo0_dp_lane_invert(struct sunxi_cadence_combophy *combo0)
{
	int val = 0, i = 0;

	for (i = 0; i < MAX_LANE_CNT; i++) {
		val = phy_readw(combo0->phy_reg + ((0xF000 + (i * 0x100)) << 1));
		val &= ~(1 << 8);
		val |= combo0->lane_invert[i] ? (1 << 8) : (0 << 8);
		phy_writew(combo0->phy_reg + ((0xF000 + (i * 0x100)) << 1), val);
	}
}

static void combo0_dp_phy_power_on(struct sunxi_cadence_combophy *combo0)
{
	// set lane remap
	combo0_dp_lane_remap(combo0, ORIENTATION_NONE);

	// set lane invert
	combo0_dp_lane_invert(combo0);
}

static void combo0_dp_phy_power_off(struct sunxi_cadence_combophy *combo0)
{
}

static int sunxi_cadence_combo0_init(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	combo0_dp_phy_init(sunxi_cphy->combo0);

	return 0;

}

static int sunxi_cadence_combo0_exit(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	combo0_dp_phy_exit(sunxi_cphy->combo0);

	return 0;

}

static int sunxi_cadence_combo0_power_on(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	combo0_dp_phy_power_on(sunxi_cphy->combo0);

	return 0;
}

static int sunxi_cadence_combo0_power_off(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	combo0_dp_phy_power_off(sunxi_cphy->combo0);

	return 0;
}

static int sunxi_cadence_combo0_configure(struct phy *phy, void *opts)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);
	struct phy_configure_opts_dp *dp_config = opts;
	int ret;

	ret = combo0_dp_set_lanes(sunxi_cphy->combo0, dp_config);
	if (ret < 0)
		return ret;

	ret = combo0_dp_set_rate(sunxi_cphy->combo0, dp_config);
	if (ret < 0)
		return ret;

	ret = combo0_dp_set_sw_pre(sunxi_cphy->combo0, dp_config);
	if (ret < 0)
		return ret;

	return 0;
}
/*
static int sunxi_cadence_phy_combo1_init(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);
	int ret = 0;

	ret = sunxi_cadence_phy_combo1_pcie_init(sunxi_cphy);

	return ret;
}

static int sunxi_cadence_phy_combo1_exit(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	sunxi_cadence_phy_combo1_pcie_exit(sunxi_cphy);

	return 0;
}
*/
static int sunxi_cadence_aux_hpd_power_on(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);
	struct sunxi_cadence_combophy *aux_phy = sunxi_cphy->aux_hpd;

	msleep(10);
	/* aux phy init*/
	phy_set_lbits(aux_phy->top_reg + 0x00, 24, 4, 0x3);
	phy_set_lbits(aux_phy->top_reg + 0x00, 20, 2, 0x3);
	phy_set_lbits(aux_phy->top_reg + 0x00, 16, 2, 0x3);
	phy_set_lbits(aux_phy->top_reg + 0x00, 12, 2, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x00, 8, 1, 0x1);

	phy_set_lbits(aux_phy->top_reg + 0x04, 28, 1, 0x0);
	phy_set_lbits(aux_phy->top_reg + 0x04, 24, 1, 0x0);
	phy_set_lbits(aux_phy->top_reg + 0x04, 16, 1, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x04, 8, 5, 0xe);
	phy_set_lbits(aux_phy->top_reg + 0x04, 0, 5, 0xd);

	phy_writel(aux_phy->top_reg + 0x08, 0);

	phy_set_lbits(aux_phy->top_reg + 0x00, 4, 1, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x00, 5, 1, 0x1);
	phy_set_lbits(aux_phy->top_reg + 0x00, 0, 1, 0x1);

	return 0;
}

static int sunxi_cadence_aux_hpd_power_off(struct phy *phy)
{
	return 0;
}

static int sunxi_cadence_aux_hpd_init(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	msleep(10);
	if (sunxi_cphy->serdes_clk)
		clk_prepare_enable(sunxi_cphy->serdes_clk);
	return 0;
}

static int sunxi_cadence_aux_hpd_exit(struct phy *phy)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(phy->dev);

	if (sunxi_cphy->serdes_clk)
		clk_disable(sunxi_cphy->serdes_clk);

	return 0;
}

static int sunxi_cadence_aux_hpd_configure(struct phy *phy, void *opts)
{
	return 0;
}

static fdt_addr_t new_fdtdec_get_addr_size_fixed(const void *blob, int node,
		const char *prop_name, int index, int na,
		int ns, fdt_size_t *sizep,
		bool translate)
{
	const fdt32_t *prop, *prop_end;
	const fdt32_t *prop_addr, *prop_size, *prop_after_size;
	int len;
	fdt_addr_t addr;

	prop = fdt_getprop(blob, node, prop_name, &len);
	if (!prop) {
		pr_err("(not found)\n");
		return FDT_ADDR_T_NONE;
	}
	prop_end = prop + (len / sizeof(*prop));

	prop_addr = prop + (index * (na + ns));
	prop_size = prop_addr + na;
	prop_after_size = prop_size + ns;
	if (prop_after_size > prop_end) {
		pr_err("(not enough data: expected >= %d cells, got %d cells)\n",
		      (u32)(prop_after_size - prop), ((u32)(prop_end - prop)));
		return FDT_ADDR_T_NONE;
	}

#if CONFIG_IS_ENABLED(OF_TRANSLATE)
	if (translate)
		addr = fdt_translate_address(blob, node, prop_addr);
	else
#endif
		addr = fdtdec_get_number(prop_addr, na);

	if (sizep) {
		*sizep = fdtdec_get_number(prop_size, ns);
		pr_debug("addr=%08llx, size=%llx\n", (unsigned long long)addr,
		      (unsigned long long)*sizep);
	} else {
		pr_debug("addr=%08llx\n", (unsigned long long)addr);
	}

	return addr;
}


int sunxi_cadence_phy_create(struct udevice *dev, ofnode np,
			     struct sunxi_cadence_combophy *combophy, enum phy_type_e type)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(dev);
	fdt_size_t size = 0;
	fdt_addr_t addr = 0;
	u32 prop_val[MAX_LANE_CNT], i;
	int ret;

	switch (type) {
	case COMBO_PHY0:
		combophy->name = strdup("combophy0");
		combophy->type = PHY_TYPE_DP;
		break;
	case COMBO_PHY1:
		combophy->name = strdup("combophy1");
		combophy->type = PHY_TYPE_PCIE;
		break;
	case AUX_HPD:
		combophy->name = strdup("aux_hpd");
		combophy->type = PHY_TYPE_AUX;
		break;
	default:
		pr_err("not support phy type (%d)\n", type);
		return -EINVAL;
	}

	combophy->clk = clk_get_by_node(np, "phy-clk");
	if (!(combophy->clk)) {
		combophy->clk = NULL;
		pr_debug("Maybe there is no clk for phy (%s)\n", combophy->name);
	}

	combophy->bus_clk = clk_get_by_node(np, "phy-bus-clk");
	if (!(combophy->bus_clk)) {
		combophy->bus_clk = NULL;
		pr_debug("Maybe there is no bus clk for phy (%s)\n", combophy->name);
	}

	addr = new_fdtdec_get_addr_size_fixed((const void *)gd->fdt_blob,
		ofnode_to_offset(np), "reg", 0, 2, 2, &size, false);
	if (addr == FDT_ADDR_T_NONE)
		return -ENOMEM;

	combophy->top_reg = addr;
	addr = new_fdtdec_get_addr_size_fixed((const void *)gd->fdt_blob,
		ofnode_to_offset(np), "reg", 1, 2, 2, &size, false);

	if (addr == FDT_ADDR_T_NONE) {
		combophy->phy_reg = 0; /* equal to NULL */
		pr_debug("Maybe there is no phy reg for %s\n", combophy->name);
	} else
		combophy->phy_reg = addr;

	combophy->sunxi_cphy = sunxi_cphy;

	ret = ofnode_read_u32_array(np, "lane_invert", prop_val, MAX_LANE_CNT);
	if (ret == 0) {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_invert[i] = prop_val[i];
	} else {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_invert[i] = 0;
	}

	ret = ofnode_read_u32_array(np, "lane_remap", prop_val, MAX_LANE_CNT);
	if (ret == 0) {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_remap[i] = prop_val[i];
	} else {
		for (i = 0; i < MAX_LANE_CNT; i++)
			combophy->lane_remap[i] = i;
	}

	return 0;
}

static int sunxi_cadence_phy_serdes_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	return 0;
}

static int sunxi_cadence_phy_parse_dt(struct udevice *dev)
{
	int ret;
	fdt_addr_t addr;
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_priv(dev);
	ofnode child;

	/* parse top register, which determide general configuration such as mode */
	addr = dev_read_addr_index(dev, 0);
	if (addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "fail to get addr for sunxi_cphy top_subsys_reg!\n");
		return -ENXIO;
	} else {
		sunxi_cphy->top_subsys_reg = (ulong)addr;
	}
	addr = dev_read_addr_index(dev, 1);
	if (addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "fail to get addr for sunxi_cphy top_combo_reg!\n");
		return -ENXIO;
	} else {
		sunxi_cphy->top_combo_reg = (ulong)addr;
	}

	sunxi_cphy->serdes_clk = clk_get_by_name(dev, "serdes-clk");
	if (!(sunxi_cphy->serdes_clk)) {
		dev_err(dev, "failed to get serdes clock for sunxi cadence phy\n");
		return -ENXIO;
	}

	sunxi_cphy->dcxo_serdes0_clk = clk_get_by_name(dev, "dcxo-serdes0-clk");
	if (!(sunxi_cphy->dcxo_serdes0_clk)) {
		dev_err(dev, "failed to get dcxo serdes0 clock for sunxi cadence phy\n");
		return -ENXIO;
	}

	sunxi_cphy->dcxo_serdes1_clk = clk_get_by_name(dev, "dcxo-serdes1-clk");
	if (!(sunxi_cphy->dcxo_serdes1_clk)) {
		dev_err(dev, "failed to get dcxo serdes1 clock for sunxi cadence phy\n");
		return -ENXIO;
	}

	ofnode_for_each_subnode(child, dev_ofnode(dev)) {
		if (strstr(ofnode_get_name(child), "combo-phy0")) {
			sunxi_cphy->combo0 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo0), GFP_KERNEL);
			if (!sunxi_cphy->combo0)
				return -ENOMEM;

			/* create combophy0 */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->combo0, COMBO_PHY0);
			if (ret) {
				dev_err(dev, "failed to create cadence combophy0, ret:%d\n", ret);
				goto err_node_put;
			}
		} else if (strstr(ofnode_get_name(child), "combo-phy1")) {
			sunxi_cphy->combo1 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo1), GFP_KERNEL);
			if (!sunxi_cphy->combo1)
				return -ENOMEM;

			/* create combophy1 */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->combo1, COMBO_PHY1);
			if (ret) {
				dev_err(dev, "failed to create cadence combophy1, ret:%d\n", ret);
				goto err_node_put;
			}
		} else if (strstr(ofnode_get_name(child), "aux-hpd")) {
			sunxi_cphy->aux_hpd = devm_kzalloc(dev, sizeof(*sunxi_cphy->aux_hpd), GFP_KERNEL);
			if (!sunxi_cphy->aux_hpd)
				return -ENOMEM;

			/* create aux phy */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->aux_hpd, AUX_HPD);
			if (ret) {
				dev_err(dev, "failed to create cadence aux-hpd phy, ret:%d\n", ret);
				goto err_node_put;
			}
		}
	}

	return 0;

err_node_put:

	return ret;
}

static int sunxi_cadence_phy_probe(struct udevice *dev)
{
	int ret;
	struct sunxi_cadence_phy *sunxi_cphy;

	sunxi_cphy = devm_kzalloc(dev, sizeof(*sunxi_cphy), GFP_KERNEL);
	if (!sunxi_cphy)
		return -ENOMEM;

	sunxi_cphy->dev = dev;

	ret = sunxi_cadence_phy_parse_dt(dev);
	if (ret)
		return -EINVAL;

	ret = sunxi_cadence_phy_serdes_init(sunxi_cphy);
	if (ret)
		return -EINVAL;

	return 0;
}

/*******************************************************************
 * Note:
 * The features support by phy are listed as follow. From the table,
 * we can know that PCIE/DP/DP_AUX has their only one specific conmophy
 * the use, and USB3.1 can select between combo0 and combo1.
 *
 * So, we assume: DP only use combo0, PCIE only use combo1, USB may
 * choose combo0 firstly.
 *
 * Some complex situation need to be solved later:
 * How to compliance with all three module exist, USB/PCIE/DP ?
 *  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
 * |             | USB3.1 | PCIE | DisplayPort | DP_AUX  |
 * |_ _ _ _ _ _ _|_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 * | Combophy0   |   O    |  X   |      O      |    X    |
 * |_ _ _ _ _ _ _|_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 * | Combophy1   |   O    |  O   |      X      |    X    |
 * |_ _ _ _ _ _ _|_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 * | AUX_HPD_PHY |   X    |  X   |      X      |    O    |
 * |_ _ _ _ _ _  |_ _ _ _ |_ _ _ |_ _ _ _ _ _ _|_ _ _ _ _|
 *
 *******************************************************************/
static int sunxi_cadence_phy_xlate(struct phy *phy,
					  struct ofnode_phandle_args *args)
{
	if (args->args_count != 1) {
		dev_err(dev, "invalid number of arguments\n");
		return -EINVAL;
	}

	if (args->args_count)
		phy->id = args->args[0];
	else
		phy->id = PHY_NONE;

	pr_info("%s: phy_id = %ld\n", __func__, phy->id);

	return 0;
}

static int sunxi_cadence_phy_power_off(struct phy *phy)
{
	if (phy->id == PHY_TYPE_DP)
		return sunxi_cadence_combo0_power_off(phy);
	else if (phy->id == PHY_TYPE_AUX)
		return sunxi_cadence_aux_hpd_power_off(phy);

	return 0;
}

static int sunxi_cadence_phy_power_on(struct phy *phy)
{
	if (phy->id == PHY_TYPE_DP)
		return sunxi_cadence_combo0_power_on(phy);
	else if (phy->id == PHY_TYPE_AUX)
		return sunxi_cadence_aux_hpd_power_on(phy);

	return 0;
}

static int sunxi_cadence_phy_init(struct phy *phy)
{
	if (phy->id == PHY_TYPE_DP)
		return sunxi_cadence_combo0_init(phy);
	else if (phy->id == PHY_TYPE_AUX)
		return sunxi_cadence_aux_hpd_init(phy);

	return 0;
}

static int sunxi_cadence_phy_exit(struct phy *phy)
{
	if (phy->id == PHY_TYPE_DP)
		return sunxi_cadence_combo0_exit(phy);
	else if (phy->id == PHY_TYPE_AUX)
		return sunxi_cadence_aux_hpd_exit(phy);

	return 0;
}

static int sunxi_cadence_phy_configure(struct phy *phy, void *opts)
{
	if (phy->id == PHY_TYPE_DP)
		return sunxi_cadence_combo0_configure(phy, opts);
	else if (phy->id == PHY_TYPE_AUX)
		return sunxi_cadence_aux_hpd_configure(phy, opts);

	return 0;
}

static const struct phy_ops sunxi_cadence_phy_ops = {
	.power_on	= sunxi_cadence_phy_power_on,
	.power_off	= sunxi_cadence_phy_power_off,
	.init		= sunxi_cadence_phy_init,
	.exit		= sunxi_cadence_phy_exit,
	.configure	= sunxi_cadence_phy_configure,
	.of_xlate       = sunxi_cadence_phy_xlate,
};

static const struct udevice_id sunxi_cadence_phy_of_match_table[] = {
	{
		.compatible = "allwinner,cadence-combophy",
	},
	{},
};

U_BOOT_DRIVER(sunxi_cadence_combophy) = {
	.name	= "sunxi_cadence_combophy",
	.id	= UCLASS_PHY,
	.of_match = sunxi_cadence_phy_of_match_table,
	.ops = &sunxi_cadence_phy_ops,
	.probe = sunxi_cadence_phy_probe,
	.priv_auto_alloc_size = sizeof(struct sunxi_cadence_phy),
};
