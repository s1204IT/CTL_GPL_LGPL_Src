/* SPDX-License-Identifier: GPL-2.0+ */
#include <asm/arch/plat-sun65iw1p1/clock_autogen.h>

#define  sunxi_ccm_reg CCMU_st
#define  pll1_cfg                pll_cpu0_ctrl_reg
#define  apb2_cfg                apb1_clk_reg
#define  uart_gate_reset         uart0_gar_reg
#define  cpu_axi_cfg             cpu_clk_reg
#define  pll6_cfg                pll_peri0_ctrl_reg
#define  psi_ahb1_ahb2_cfg       ahb_clk_reg
#define  apb1_cfg                apb0_clk_reg
#define  mbus_cfg                mbus_clk_reg
#define  ve_clk_cfg		 pll_ve0_ctrl_reg
#define  de_clk_cfg              de0_clk_reg
#define  mbus_gate               mbus_mat_clk_gating_reg
#define  dma_gate_reset          dma0_bgr_reg
#define  sd0_clk_cfg             smhc0_clk_reg
#define  sd1_clk_cfg             smhc1_clk_reg
#define  sd2_clk_cfg             smhc2_clk_reg
#define  sd_gate_reset           smhc0_gar_reg
#define  twi_gate_reset          twi0_gar_reg
#define  ce_mbus_master			 mbus_mat_clk_gate_en_reg /* 0x05e0 */
#define	 ce_mbus				 mbus_clk_gate_en_reg	  /* 0x05e4 */
#define  ce_sys_clk              ce_clk_reg				  /* 0x0ac0 */
#define  ce_gate_reset           ce_bgr_reg				  /* 0x0ac4 */

#define  APB2_CLK_SRC_OSC24M     (APB1_CLK_REG_CLK_SRC_SEL_SYS_24M_CLK << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_OSC32K     (APB1_CLK_REG_CLK_SRC_SEL_SYS_32K_CLK << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_PSI        (APB1_CLK_REG_CLK_SRC_SEL_RC_16M_CLK << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_PLL6       (APB1_CLK_REG_CLK_SRC_SEL_PERI0_600M_BUS << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)

#define  PLL_CPU1_CTRL_REG_PLL_OUTPUT_GATE_OFFSET      PLL_GPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET
#define  PLL_CPU1_CTRL_REG_PLL_EN_OFFSET			   PLL_GPU_CTRL_REG_PLL_EN_OFFSET
#define  PLL_CPU1_CTRL_REG_PLL_LDO_EN_OFFSET           PLL_GPU_CTRL_REG_PLL_LDO_EN_OFFSET
#define  PLL_CPU1_CTRL_REG_LOCK_ENABLE_OFFSET          PLL_GPU_CTRL_REG_LOCK_ENABLE_OFFSET

#define APB2_CLK_RATE_N_1               (0x0 << 8)
#define APB2_CLK_RATE_N_2               (0x1 << 8)
#define APB2_CLK_RATE_N_4               (0x2 << 8)
#define APB2_CLK_RATE_N_8               (0x3 << 8)
#define APB2_CLK_RATE_N_MASK            (3 << 8)
#define APB2_CLK_RATE_M(m)              (((m)-1) << APB1_CLK_REG_FACTOR_M_OFFSET)
#define APB2_CLK_RATE_M_MASK            (3 << APB1_CLK_REG_FACTOR_M_OFFSET)

/* MMC clock bit field */
#define CCM_MMC_CTRL_M(x)               (x)
#define CCM_MMC_CTRL_N(x)               ((x) << SMHC0_CLK_REG_FACTOR_N_OFFSET)
#define CCM_MMC_CTRL_OSCM24             (SMHC0_CLK_REG_CLK_SRC_SEL_SYS_24M_CLK<< SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL6X2             (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_400M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL_PERIPH2X2      (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_300M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_400M		(0x3 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_300M		(0x4 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_800M		(0x3 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_600M		(0x4 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_ENABLE             (SMHC0_CLK_REG_SMHC0_CLK_GATING_CLOCK_IS_ON << SMHC0_CLK_REG_SMHC0_CLK_GATING_OFFSET)
/* if doesn't have these delays */
#define CCM_MMC_CTRL_OCLK_DLY(a)        ((void) (a), 0)
#define CCM_MMC_CTRL_SCLK_DLY(a)        ((void) (a), 0)

/* Module gate/reset shift*/
#define RESET_SHIFT                     (16)
#define GATING_SHIFT                    (0)

/* CE CLK Register */
#define SUNXI_CE_MBUS_MAT_CLK_REG (SUNXI_CCM_BASE + MBUS_MAT_CLK_GATE_EN_REG)
#define SUNXI_CE_GATING_ON 1
#define SUNXI_CE_MBUS_MAT_CLK_GATE_OFFSET MBUS_MAT_CLK_GATE_EN_REG_CE_SYS_AXI_GATE_SW_CFG_OFFSET

#define SUNXI_CE_MBUS_CLK_REG	(SUNXI_CCM_BASE + MBUS_CLK_GATE_EN_REG)
#define SUNXI_CE_MBUS_CLK_GATE_OFFSET MBUS_CLK_GATE_EN_REG_CE_SYS_AXI_CLK_EN_OFFSET

#define SUNXI_CE_SYS_CLK_REG (SUNXI_CCM_BASE + CE_SYS_CLK_REG)
#define SUNXI_CE_SYS_GATING_OFFSET	CE_SYS_CLK_REG_CE_SYS_CLK_GATING_OFFSET
#define	SUNXI_CE_SYS_CLK_SRC_SEL_OFFSET CE_SYS_CLK_REG_CLK_SRC_SEL_OFFSET
#define SUNXI_CE_SRC_600M	CE_SYS_CLK_REG_CLK_SRC_SEL_PERI0_600M
#define SUNXI_CE_SYS_CLK_FACTOR_M_OFFSET CE_SYS_CLK_REG_FACTOR_M_OFFSET
#define SUNXI_CE_FACTOR_0 0b0

#define SUNXI_CE_SYS_GATING_RESET_REG (SUNXI_CCM_BASE + CE_SYS_GAR_REG)
#define SUNXI_CE_GATING_OFFSET CE_SYS_GAR_REG_CE_SYS_IP_AHB_CLK_EN_MASK
#define SUNXI_CE_SYS_GATING_RESET_OFFSET CE_SYS_GAR_REG_CE_SYS_RST_N_OFFSET

/* GIC */
#define  GIC_CLK_REG	CPUX_GIC_CLK_REG

#define CCMU_PLL_CPU0_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE)
#define CCMU_PLL_CPU1_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x100)
#define CCMU_PLL_CPU2_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x200)

struct sunxi_cpu_pll_reg{
	uint32_t pll_cpu0_ctrl_reg; /*0x00*/
	uint32_t pll_cpu0_pat0_reg;
	uint32_t pll_cpu0_pat1_reg;
	uint32_t pll_cpu0_bias_reg;
	uint32_t pll_cpu0_tun1_reg; /*0x10*/
	uint32_t pll_cpu0_ssc_reg;
	uint32_t clu0_clk_reg;
	uint32_t clu0_gating_reg;
	uint32_t clu0_div_cfg_reg; /*0x20*/

	uint32_t pad_until_0x0100[55];

	uint32_t pll_cpu1_ctrl_reg;
	uint32_t pll_cpu1_pat0_reg;
	uint32_t pll_cpu1_pat1_reg;
	uint32_t pll_cpu1_bias_reg; /* 0x30 */
	uint32_t pll_cpu1_tun1_reg;
	uint32_t pll_cpu1_ssc_reg;
	uint32_t clu1_clk_reg;
	uint32_t clu1_gating_reg; /* 0x40 */
	uint32_t clu1_div_cfg_reg;
	uint32_t pll_test_clk_sel;
};


