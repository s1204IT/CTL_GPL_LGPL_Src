/* SPDX-License-Identifier: GPL-2.0+ */

#include <asm/arch/plat-sun300iw1p1/clock_autogen_app.h>
#include <asm/arch/plat-sun300iw1p1/cpu_autogen.h>

#define sunxi_ccm_reg		CCU_APP_st
#define mbus_gate		bus_clk_gating2_reg
#define dma_gate_reset		bus_reset0_reg
#define sd0_clk_cfg		smhc_ctrl0_clk_reg
#define sd1_clk_cfg		smhc_ctrl1_clk_reg
#define sd_gate_reset		bus_reset1_reg
#define spi0_clk_cfg		spi_clk_reg
#define spif_clk_cfg		spif_clk_reg
#define ahb_gate0		bus_clk_gating1_reg
#define ahb_reset0_cfg		bus_reset1_reg
#define ce_clk_cfg		ss_clk_reg
#define ce_gate_reset		bus_reset0_reg

#define CCMU_UART_BGR_REG        (SUNXI_CCM_APP_BASE + BUS_CLK_GATING0_REG)
#define CCMU_UART_RST_REG        (SUNXI_CCM_APP_BASE + BUS_Reset0_REG)
#define CCMU_BUS_CLK_GATING0_REG (SUNXI_CCM_APP_BASE + BUS_CLK_GATING0_REG)
#define CCMU_BUS_CLK_GATING1_REG (SUNXI_CCM_APP_BASE + BUS_CLK_GATING1_REG)
#define CCMU_BUS_Reset0_REG      (SUNXI_CCM_APP_BASE + BUS_Reset0_REG)
#define CCMU_BUS_Reset1_REG      (SUNXI_CCM_APP_BASE + BUS_Reset1_REG)

/* pll1 bit field */
#define CCM_PLL1_CTRL_EN		BIT(31)
#define CCM_PLL1_LOCK_EN		BIT(29)
#define CCM_PLL1_LOCK			BIT(28)
#define CCM_PLL1_CLOCK_TIME_2		(2 << 24)
#define CCM_PLL1_CTRL_P(p)		((p) << 16)
#define CCM_PLL1_CTRL_N(n)		((n) << 8)

/* pll5 bit field */
#define CCM_PLL5_CTRL_EN		BIT(31)
#define CCM_PLL5_LOCK_EN		BIT(29)
#define CCM_PLL5_LOCK			BIT(28)
#define CCM_PLL5_CTRL_N(n)		((n) << 8)
#define CCM_PLL5_CTRL_DIV1(div1)	((div1) << 0)
#define CCM_PLL5_CTRL_DIV2(div0)	((div0) << 1)

/* pll6 bit field */
#define CCM_PLL6_CTRL_EN		BIT(31)
#define CCM_PLL6_LOCK_EN		BIT(29)
#define CCM_PLL6_LOCK			BIT(28)
#define CCM_PLL6_CTRL_N_SHIFT		8
#define CCM_PLL6_CTRL_N_MASK		(0xff << CCM_PLL6_CTRL_N_SHIFT)
#define CCM_PLL6_CTRL_DIV1_SHIFT	0
#define CCM_PLL6_CTRL_DIV1_MASK		(0x1 << CCM_PLL6_CTRL_DIV1_SHIFT)
#define CCM_PLL6_CTRL_DIV2_SHIFT	1
#define CCM_PLL6_CTRL_DIV2_MASK		(0x1 << CCM_PLL6_CTRL_DIV2_SHIFT)
#define CCM_PLL6_DEFAULT		0xa0006300

/* cpu_axi bit field*/
#define CCM_CPU_AXI_MUX_MASK		(0x3 << 24)
#define CCM_CPU_AXI_MUX_OSC24M		(0x0 << 24)
#define CCM_CPU_AXI_MUX_PLL_CPUX	(0x3 << 24)

/* psi_ahb1_ahb2 bit field */
#define CCM_PSI_AHB1_AHB2_DEFAULT	0x03000102

/* ahb3 bit field */
#define CCM_AHB3_DEFAULT		0x03000002

/* apb1 bit field */
#define CCM_APB1_DEFAULT		0x03000102

/* apb2 bit field */
#define APB2_CLK_SRC_OSC24M		(0x0 << 24)
#define APB2_CLK_SRC_OSC32K		(0x1 << 24)
#define APB2_CLK_SRC_PSI		(0x2 << 24)
#define APB2_CLK_SRC_PLL6		(0x3 << 24)
#define APB2_CLK_SRC_MASK		(0x3 << 24)
#define APB2_CLK_RATE_N_1		(0x0 << 8)
#define APB2_CLK_RATE_N_2		(0x1 << 8)
#define APB2_CLK_RATE_N_4		(0x2 << 8)
#define APB2_CLK_RATE_N_8		(0x3 << 8)
#define APB2_CLK_RATE_N_MASK		(3 << 8)
#define APB2_CLK_RATE_M(m)		(((m)-1) << 0)
#define APB2_CLK_RATE_M_MASK            (3 << 0)

/* MBUS clock bit field */
#define MBUS_ENABLE			BIT(31)
#define MBUS_RESET			BIT(30)
#define MBUS_CLK_SRC_MASK		GENMASK(25, 24)
#define MBUS_CLK_SRC_OSCM24		(0 << 24)
#define MBUS_CLK_SRC_PLL6X2		(1 << 24)
#define MBUS_CLK_SRC_PLL5		(2 << 24)
#define MBUS_CLK_SRC_PLL6X4		(3 << 24)
#define MBUS_CLK_M(m)			(((m)-1) << 0)

/* Module gate/reset shift*/
#define RESET_SHIFT			(16)
#define GATING_SHIFT			(0)

/* MMC clock bit field */
#define CCM_MMC_CTRL_M(x)		((x))
#define CCM_MMC_CTRL_N(x)		((x) << 16)
#define CCM_MMC_CTRL_OSCM24		(0x0 << 24)
#define CCM_MMC_CTRL_PLL6X2		(0x2 << 24)
#define CCM_MMC_CTRL_PLL_PERIPH2X2	(0x2 << 24)
#define SMHC_CTRL0_CLK_SEL_PERI_192M_freq (192000000)
#define SMHC_CTRL0_CLK_SEL_PERI_219M_freq (219000000)
#define SMHC_CTRL0_CLK_SEL_PERI_192M_OFFSET ((SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_SEL_PERI_192M) << (SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_SEL_OFFSET))
#define SMHC_CTRL0_CLK_SEL_PERI_219M_OFFSET ((SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_SEL_PERI_219M) << (SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_SEL_OFFSET))
#define CCM_MMC_CTRL_ENABLE		(0x1 << 31)
/* if doesn't have these delays */
#define CCM_MMC_CTRL_OCLK_DLY(a)	((void) (a), 0)
#define CCM_MMC_CTRL_SCLK_DLY(a)	((void) (a), 0)

/*CE*/
#define CE_CLK_SRC_MASK                   (0x1)
#define CE_CLK_SRC_SEL_BIT                (24)
#define CE_CLK_SRC                        (0x01)

#define CE_CLK_DIV_RATION_N_BIT           (8)
#define CE_CLK_DIV_RATION_N_MASK          (0x3)
#define CE_CLK_DIV_RATION_N               (0)

#define CE_CLK_DIV_RATION_M_BIT           (0)
#define CE_CLK_DIV_RATION_M_MASK          (0xF)
#define CE_CLK_DIV_RATION_M               (3)

#define CE_SCLK_ONOFF_BIT                 (31)
#define CE_SCLK_ON                        (1)

#define CE_GATING_BASE                    CCMU_CE_BGR_REG
#define CE_GATING_PASS                    (1)
#define CE_GATING_BIT                     (0)

#define CE_RST_REG_BASE                   CCMU_CE_BGR_REG
#define CE_RST_BIT                        (16)
#define CE_DEASSERT                       (1)

#define CE_MBUS_GATING_MASK               (1)
#define CE_MBUS_GATING_BIT		  (2)
#define CE_MBUS_GATING			  (1)

//usb
#define USBEHCI0_RST_BIT 20
#define USBEHCI0_GATIING_BIT 4
#define USBPHY0_RST_BIT 30
#define USBPHY0_SCLK_GATING_BIT 29

#define USBEHCI1_RST_BIT 21
#define USBEHCI1_GATIING_BIT 5
#define USBPHY1_RST_BIT 30
#define USBPHY1_SCLK_GATING_BIT 29

//spi
#define GATING_RESET_SHIFT	 (4)

//* SPIF clock bit field */
#define CCM_SPIF_CTRL_M(x)		((x) - 1)
#define CCM_SPIF_CTRL_N(x)		((x) << 16)
#define CCM_SPIF_CTRL_HOSC		(0x0 << 24)
#define CCM_SPIF_CTRL_PERI512M		(0x1 << 24)
#define CCM_SPIF_CTRL_PERI384M		(0x2 << 24)
#define CCM_SPIF_CTRL_PERI307M		(0x3 << 24)
#define CCM_SPIF_CTRL_ENABLE		(0x1 << 31)
#define GET_SPIF_CLK_SOURECS(x)		(x == CCM_SPIF_CTRL_PERI512M ? 512000000 : 384000000)
#define CCM_SPIF_CTRL_PERI		CCM_SPIF_CTRL_PERI384M
#define SPIF_GATING_RESET_SHIFT		(5)

/* E907 */
#define CCMU_E907_CFG_RST           (0x1 << 0)
#define CCMU_E907_SYS_APB_RST       (0x1 << 1)
#define CCMU_E907_CFG_CLK_GATING    (0x1 << 0)
#define CCMU_E907_RSTN_REG          (SUNXI_CCM_APP_BASE + E907_RSTN_REG)
#define E907_CFG_BASE               (0x43030000)
#define E907_STA_ADD_REG            (E907_CFG_BASE + 0x0204)
