/*
 * hardware interfaces for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef XRADIO_HWIO_H_INCLUDED
#define XRADIO_HWIO_H_INCLUDED

#ifdef USE_ADDRESS_REMAP_IN_LINUX
#include "platform.h"
#endif
/* extern */ struct xradio_common;

/* DPLL initial values */
#define DPLL_INIT_VAL_XRADIO      (0x0EC4F121)

/* Hardware Type Definitions */
#define HIF_HW_TYPE_XRADIO        (1)


/* boot loader start address in SRAM */
#define DOWNLOAD_BOOT_LOADER_OFFSET   (0x00000000)
/* 32K, 0x4000 to 0xDFFF */
#define DOWNLOAD_FIFO_OFFSET          (0x00004000)
/* 32K */
#define XR819_DOWNLOAD_FIFO_SIZE      (0x00008000)
/* 128 bytes, 0xFF80 to 0xFFFF */
#define XR819_DOWNLOAD_CTRL_OFFSET    (0x0000FF80)
#define DOWNLOAD_CTRL_DATA_DWORDS     (32-6)

#define XR819S_DOWNLOAD_FIFO_SIZE      (0x00004000)
/* 128 bytes, 0xFF80 to 0xFFFF */
#define XR819S_DOWNLOAD_CTRL_OFFSET    (0x00000100)

#define V821_DOWNLOAD_FIFO_OFFSET            (0x00004000)    /* 16K, 0x4000 to 0x7FFF */
#define V821_DOWNLOAD_FIFO_SIZE              (0x00004000)    /* 16K, 0x0 to 0x4000 */
#define V821_DOWNLOAD_CTRL_OFFSET            (0x0000100)     /* correspondence with wlan bl version */


//extern u32 DOWNLOAD_FIFO_SIZE;
//extern u32 DOWNLOAD_CTRL_OFFSET;

/* Download control area */
struct download_cntl_t {
	/* size of whole firmware file (including Cheksum), host init */
	u32 ImageSize;
	/* downloading flags */
	u32 Flags;
	/* No. of bytes put into the download, init & updated by host */
	u32 Put;
	/* last traced program counter, last ARM reg_pc */
	u32 TracePc;
	/* No. of bytes read from the download, host init, device updates */
	u32 Get;
	/* r0, boot losader status, host init to pending, device updates */
	u32 Status;
	/* Extra debug info, r1 to r14 if status=r0=DOWNLOAD_EXCEPTION */
	u32 DebugData[DOWNLOAD_CTRL_DATA_DWORDS];
};

#define	DOWNLOAD_IMAGE_SIZE_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, ImageSize))
#define	DOWNLOAD_FLAGS_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, Flags))
#define DOWNLOAD_PUT_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, Put))
#define DOWNLOAD_TRACE_PC_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, TracePc))
#define	DOWNLOAD_GET_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, Get))
#define	DOWNLOAD_STATUS_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, Status))
#define DOWNLOAD_DEBUG_DATA_REG		\
	(DOWNLOAD_CTRL_OFFSET + offsetof(struct download_cntl_t, DebugData))

#define DOWNLOAD_DEBUG_DATA_LEN   (108)
#define DOWNLOAD_BLOCK_SIZE       (1024)

#define DOWNLOAD_BOOTLOADER_SIZE        (4096)
#define DOWNLOAD_SDD_SIZE               (4096)

/* For boot loader detection */
#define DOWNLOAD_ARE_YOU_HERE     (0x87654321)
#define DOWNLOAD_I_AM_HERE        (0x12345678)

/* Download error code */
#define DOWNLOAD_PENDING        (0xFFFFFFFF)
#define DOWNLOAD_SUCCESS        (0)
#define DOWNLOAD_EXCEPTION      (1)
#define DOWNLOAD_ERR_MEM_1      (2)
#define DOWNLOAD_ERR_MEM_2      (3)
#define DOWNLOAD_ERR_SOFTWARE   (4)
#define DOWNLOAD_ERR_FILE_SIZE  (5)
#define DOWNLOAD_ERR_CHECKSUM   (6)
#define DOWNLOAD_ERR_OVERFLOW   (7)
#define DOWNLOAD_ERR_IMAGE      (8)
#define DOWNLOAD_ERR_HOST       (9)
#define DOWNLOAD_ERR_ABORT      (10)

#define DOWNLOAD_F_LOAD_SIZE_NOT_CARE   0x04

#ifdef CONFIG_ACCESS_WIFI_DIRECT
#define AHB_MEMORY_ADDRESS              0x08000000
#define PAS_MEMORY_ADDRESS              0x09000000
#define TYPE_MEMORY_ADDRESS_MASK        0x0f000000

#define XRADIO_FWMEM_FW2NET(addr)       ((addr) | 0x60000000)

#define XRADIO_FWAHBMEM_FW2NET          XRADIO_FWMEM_FW2NET(AHB_MEMORY_ADDRESS)
#define SILICON_SYS_BASE_ADDR           (XRADIO_FWMEM_FW2NET(0))
#define SILICON_AHB_MEMORY_ADDRESS      (SILICON_SYS_BASE_ADDR + 0x08000000)
#if (((defined(CONFIG_CHIP_XRADIO)) && (CONFIG_CHIP_ARCH_VER == 1)) || (defined(CONFIG_CHIP_XR819)) || (defined(CONFIG_CHIP_XR829)))
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x09000000)
#elif (defined(CONFIG_CHIP_XRADIO) && (CONFIG_CHIP_ARCH_VER == 2))
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x0800D000)  //aw1825 AHB is 52K, PAS is just follow it.
#elif (defined(CONFIG_CHIP_XRADIO) && (CONFIG_CHIP_ARCH_VER == 3))
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x0800C000)  //aw1826 AHB is 48K, PAS is just follow it.
#elif (defined(CONFIG_DRIVER_R128))
// aw1883 AHB is 96K/80K , PAS is 48K, just follow it in 96K. if AHBRAM use 80K version, the offset should be set 0x08014000
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x08018000)
#elif (defined CONFIG_DRIVER_V821)
// aw1882 AHB is 80K , PAS is 48K, PAS not follow AHB RAM.
#define SILICON_PAC_BASE_ADDRESS        (SILICON_SYS_BASE_ADDR + 0x08018000)
#define V821_PAS_RAM_SIZE               0xC000
#define V821_AHB_RAM_SIZE				0x14000
#endif

#ifdef USE_ADDRESS_REMAP_IN_LINUX
#define SILICON_PAC_SHARED_MEMORY		(PAS_SHARE_RAM_ADDR_BASE)
#else
#define SILICON_PAC_SHARED_MEMORY		(SILICON_PAC_BASE_ADDRESS)
#endif
#define SILICON_APB_ADDR(addr)			(SILICON_PAC_SHARED_MEMORY + (addr))
#else
#define SYS_BASE_ADDR_SILICON      (0)
#define AHB_MEMORY_ADDRESS         (SYS_BASE_ADDR_SILICON + 0x08000000)
#define PAC_BASE_ADDRESS_SILICON   (SYS_BASE_ADDR_SILICON + 0x09000000)
#define PAC_SHARED_MEMORY_SILICON  (PAC_BASE_ADDRESS_SILICON)
#define APB_ADDR(addr)             (PAC_SHARED_MEMORY_SILICON + (addr))
#define XRADIO_FWAHBMEM_FW2NET     (AHB_MEMORY_ADDRESS)
#endif



/* ***************************************************************
*Device register definitions
*************************************************************** */
/* WBF - SPI Register Addresses */
#define HIF_ADDR_ID_BASE             (0x0000)
/* 16/32 bits */
#define HIF_CONFIG_REG_ID            (0x0000)
/* 16/32 bits */
#define HIF_CONTROL_REG_ID           (0x0001)
/* 16 bits, Q mode W/R */
#define HIF_IN_OUT_QUEUE_REG_ID      (0x0002)
/* 32 bits, AHB bus R/W */
#define HIF_AHB_DPORT_REG_ID         (0x0003)
/* 16/32 bits */
#define HIF_SRAM_BASE_ADDR_REG_ID    (0x0004)
/* 32 bits, APB bus R/W */
#define HIF_SRAM_DPORT_REG_ID        (0x0005)
/* 32 bits, t_settle/general */
#define HIF_TSET_GEN_R_W_REG_ID      (0x0006)
/* 16 bits, Q mode read, no length */
#define HIF_FRAME_OUT_REG_ID         (0x0007)
#define HIF_ADDR_ID_MAX              (HIF_FRAME_OUT_REG_ID)

/* WBF - Control register bit set */
/* next o/p length, bit 11 to 0 */
#define HIF_CTRL_NEXT_LEN_MASK     (0x0FFF)
#define HIF_CTRL_WUP_BIT           (BIT(12))
#define HIF_CTRL_RDY_BIT           (BIT(13))
#define HIF_CTRL_IRQ_ENABLE        (BIT(14))
#define HIF_CTRL_RDY_ENABLE        (BIT(15))
#define HIF_CTRL_IRQ_RDY_ENABLE    (BIT(14)|BIT(15))

/* SPI Config register bit set */
#define HIF_CONFIG_FRAME_BIT       (BIT(2))
#define HIF_CONFIG_WORD_MODE_BITS  (BIT(3)|BIT(4))
#define HIF_CONFIG_WORD_MODE_1     (BIT(3))
#define HIF_CONFIG_WORD_MODE_2     (BIT(4))
#define HIF_CONFIG_ERROR_0_BIT     (BIT(5))
#define HIF_CONFIG_ERROR_1_BIT     (BIT(6))
#define HIF_CONFIG_ERROR_2_BIT     (BIT(7))
/* TBD: Sure??? */
#define HIF_CONFIG_CSN_FRAME_BIT   (BIT(7))
#define HIF_CONFIG_ERROR_3_BIT     (BIT(8))
#define HIF_CONFIG_ERROR_4_BIT     (BIT(9))
/* QueueM */
#define HIF_CONFIG_ACCESS_MODE_BIT (BIT(10))
/* AHB bus */
#define HIF_CONFIG_AHB_PFETCH_BIT  (BIT(11))
#define HIF_CONFIG_CPU_CLK_DIS_BIT (BIT(12))
/* APB bus */
#define HIF_CONFIG_PFETCH_BIT      (BIT(13))
/* cpu reset */
#define HIF_CONFIG_CPU_RESET_BIT   (BIT(14))
#define HIF_CONFIG_CLEAR_INT_BIT   (BIT(15))
/*819s DPLL enable*/
#define HIF_CONFIG_DPLL_EN_BIT     (BIT(20))



/* For XRADIO the IRQ Enable and Ready Bits are in CONFIG register */
#define HIF_CONF_IRQ_RDY_ENABLE	(BIT(16)|BIT(17))

#if CONFIG_HIF_BY_PASS /*HIF bypass*/

#ifdef CONFIG_DRIVER_V821
#define WIFI_PER_BASE                   (0x70000000)
#else
#define WIFI_PER_BASE                   (0xB0000000)
#endif

#ifdef HIF_OV_CTRL
#define OV_DISABLE_CPU_CLK              (1<<0)
#define OV_RESET_CPU                    (1<<1)
#define OV_WLAN_WAKE_UP                 (1<<2)
#define OV_DISABLE_CPU_CLK_EN           (1<<4)
#define OV_RESET_CPU_EN                 (1<<5)
#define OV_WLAN_WAKE_UP_EN              (1<<6)
#define OV_WLAN_IRQ_EN                  (1<<7)
#endif

#ifdef HIF_WLAN_STATE
#define WLAN_STATE_ACTIVE               (1<<0)
#if (defined(CONFIG_DRIVER_R128) || (defined CONFIG_DRIVER_V821))
#define WLAN_CPU_DEEPSLEEP              (1<<1)
#define WLAN_CPU_SLEEP                  (1<<2)
#else
#define WLAN_CPU_SLEEP                  (1<<1)
#define WLAN_CPU_DEEPSLEEP              (1<<2)
#endif
#endif

#define HIF_OV_CTRL_RESET_VALUE        (OV_WLAN_WAKE_UP_EN   | OV_RESET_CPU_EN | \
										OV_DISABLE_CPU_CLK_EN| OV_WLAN_IRQ_EN  | \
										OV_DISABLE_CPU_CLK   | OV_RESET_CPU)


#define HIF_OV_MISC_REG_REL             (0x0AA80000 | WIFI_PER_BASE)

#ifdef USE_ADDRESS_REMAP_IN_LINUX
#define HIF_OV_MISC_REG					HIF_OV_MISC_REG_VIR
#else
#define HIF_OV_MISC_REG					HIF_OV_MISC_REG_REL
#endif
#define HIF_OV_TX0_INT_CTRL             (HIF_OV_MISC_REG + 0x80)
#define HIF_OV_TX1_INT_CTRL             (HIF_OV_MISC_REG + 0x84)
#define HIF_OV_TX2_INT_CTRL             (HIF_OV_MISC_REG + 0x88)
#define HIF_OV_TX3_INT_CTRL             (HIF_OV_MISC_REG + 0x8C)
#define HIF_OV_RX0_INT_CTRL             (HIF_OV_MISC_REG + 0x90)
#define HIF_OV_RX1_INT_CTRL             (HIF_OV_MISC_REG + 0x94)
#define HIF_OV_RX2_INT_CTRL             (HIF_OV_MISC_REG + 0x98)
#define HIF_OV_RX3_INT_CTRL             (HIF_OV_MISC_REG + 0x9C)
#define HIF_OV_RDY_INT_CTRL             (HIF_OV_MISC_REG + 0xA0)
#define HIF_OV_INT_EN                   (1<<0)
#define HIF_OV_INT_RDY_FLAG             (1<<1)

#define WLAN_APB_PWR_CTRL_BASE_REL		((0x0A000000 + 0xC80000) | WIFI_PER_BASE)

#ifdef USE_ADDRESS_REMAP_IN_LINUX
#define WLAN_APB_PWR_CTRL_BASE			WLAN_APB_PWR_CTRL_BASE_VIR
#else
#define WLAN_APB_PWR_CTRL_BASE			WLAN_APB_PWR_CTRL_BASE_REL
#endif
#define WLAN_APB_PWR_SWREADY            (WLAN_APB_PWR_CTRL_BASE + 0x0034)
#define WLAN_APB_PWR_GOINTOSLEEP        (WLAN_APB_PWR_CTRL_BASE + 0x0048)
#define WLAN_APB_PWR_GOINTOSTANDBY      (WLAN_APB_PWR_CTRL_BASE + 0x004C)

#if (defined(CONFIG_CHIP_XRADIO) && (CONFIG_CHIP_ARCH_VER == 3))
#define APB_PWR_RFIPTRIM_0              (WLAN_APB_PWR_CTRL_BASE + 0x005C + 0)
#define APB_PWR_RFIPTRIM_1              (WLAN_APB_PWR_CTRL_BASE + 0x00E8 + 0)
#endif

#define REG_READ_U32(reg)                    (*(volatile u32 *)(reg))
#define REG_WRITE_U32(reg, v)                (*(volatile u32 *)(reg) = (v))
#define REG_SET_BIT(reg, mask)              (*(volatile u32 *)(reg) |= (mask))
#define REG_CLR_BIT(reg, mask)              (*(volatile u32 *)(reg) &= ~(mask))
#define REG_GET_BIT(reg, mask)              (*(volatile u32 *)(reg) & (mask))
#define REG_GET_BIT_VAL(reg, shift, vmask)  (((*(volatile u32 *)(reg))>>(shift)) & (vmask))

#define TX_QUEUE_MAX  32
#define RX_QUEUE_MAX  64
#define TX_QUEUE_MASK  (TX_QUEUE_MAX-1)
#define RX_QUEUE_MASK  (RX_QUEUE_MAX-1)
typedef struct __queue_ctrl {
	volatile u32 read_idx;
	volatile u32 write_idx;
	volatile u32 buffer_idx;
} QUEUE_CTRL;

typedef struct __hif_queue {
	QUEUE_CTRL tx_ctrl;
	QUEUE_CTRL rx_ctrl;
	u32 tx_queue[TX_QUEUE_MAX];
	u32 rx_queue[RX_QUEUE_MAX];
	volatile u32 host_state;
	volatile u32 exception;
	volatile u32 rx_fifo_addr;
	volatile u32 rx_fifo_len;
} HIF_QUEUE;

typedef struct __hif_ctrl {
	volatile u32 state_magic;
	volatile u32 queue_addr;
} HIF_CTRL;


#ifdef USE_ADDRESS_REMAP_IN_LINUX
#define HIF_BYPASS_CTRL_ADDR (PAS_SHARE_RAM_ADDR_BASE + V821_DOWNLOAD_CTRL_OFFSET - sizeof(HIF_CTRL))
#else
#define HIF_BYPASS_CTRL_ADDR (SILICON_PAC_SHARED_MEMORY + V821_DOWNLOAD_CTRL_OFFSET - sizeof(HIF_CTRL))
#endif
#define HIF_BYPASS_FW_RDY     DOWNLOAD_I_AM_HERE
#define HIF_BYPASS_HOST_RDY   DOWNLOAD_ARE_YOU_HERE

#define INDEX_DIFF(a, b)      ((s32)(a) - (s32)(b))
#define INDEX_BEFORE(a, b)    ((s32)(a) - (s32)(b) < 0)
#define INDEX_BEFORE_EQ(a, b) ((s32)(a) - (s32)(b) <= 0)

extern HIF_QUEUE   *pHif_bypass;
extern volatile s32 rx_data_cnt;
#define HOST_WAKE_UP         0x1
#define RX_WAKE_UP           0x2
static inline bool is_device_awake(void)
{
	return (REG_GET_BIT(HIF_OV_CTRL, OV_WLAN_WAKE_UP) || (rx_data_cnt > 0));
}
static __always_inline void rx_set_cnt(int cnt)
{
	rx_data_cnt = cnt;
}
u32 FW_ADDR_TO_APP_ADDR(u32 addr);
int xradio_hif_bypass_enable(void);
void xradio_hif_bypass_disable(void);
int xradio_hif_bypass_init(void);
int xradio_hif_bypass_irq_enable(void);
int xradio_check_rx(void);

int xradio_data_read(struct xradio_common *hw_priv, void *buf, size_t buf_len);
int xradio_data_write(struct xradio_common *hw_priv, const void *buf, size_t buf_len);

#endif

#if !(CONFIG_HIF_BY_PASS) /*HIF bypass*/
int xradio_data_read(struct xradio_common *hw_priv, void *buf,
					 size_t buf_len);
int xradio_data_write(struct xradio_common *hw_priv, const void *buf,
					  size_t buf_len);
int xradio_reg_read(struct xradio_common *hw_priv, u16 addr, void *buf,
					size_t buf_len);
int xradio_reg_write(struct xradio_common *hw_priv, u16 addr,
					 const void *buf, size_t buf_len);
int xradio_indirect_read(struct xradio_common *hw_priv, u32 addr, void *buf,
						 size_t buf_len, u32 prefetch, u16 port_addr);
int xradio_apb_write(struct xradio_common *hw_priv, u32 addr,
					 const void *buf, size_t buf_len);
int xradio_ahb_write(struct xradio_common *hw_priv, u32 addr,
					 const void *buf, size_t buf_len);


static inline int xradio_reg_read_16(struct xradio_common *hw_priv,
				     u16 addr, u16 *val)
{
	int ret    = 0;
	u32 bigVal = 0;
	ret = xradio_reg_read(hw_priv, addr, &bigVal, sizeof(bigVal));
	*val = (u16)bigVal;
	return ret;
}

static inline int xradio_reg_write_16(struct xradio_common *hw_priv,
				      u16 addr, u16 val)
{
	u32 bigVal = (u32)val;
	return xradio_reg_write(hw_priv, addr, &bigVal, sizeof(bigVal));
}

static inline int xradio_reg_read_32(struct xradio_common *hw_priv,
				      u16 addr, u32 *val)
{
	return xradio_reg_read(hw_priv, addr, val, sizeof(*val));
}

static inline int xradio_reg_write_32(struct xradio_common *hw_priv,
				      u16 addr, u32 val)
{
	return xradio_reg_write(hw_priv, addr, &val, sizeof(val));
}

static inline int xradio_apb_read(struct xradio_common *hw_priv, u32 addr,
				  void *buf, size_t buf_len)
{
	return xradio_indirect_read(hw_priv, addr, buf, buf_len,
				    HIF_CONFIG_PFETCH_BIT,
				    HIF_SRAM_DPORT_REG_ID);
}

static inline int xradio_apb_read_32(struct xradio_common *hw_priv,
				      u32 addr, u32 *val)
{
	return xradio_apb_read(hw_priv, addr, val, sizeof(*val));
}

static inline int xradio_apb_write_32(struct xradio_common *hw_priv,
				      u32 addr, u32 val)
{
	return xradio_apb_write(hw_priv, addr, &val, sizeof(val));
}

static inline int xradio_ahb_read_32(struct xradio_common *hw_priv,
				      u32 addr, u32 *val)
{
	return xradio_indirect_read(hw_priv, addr, val, sizeof(*val),
				    HIF_CONFIG_AHB_PFETCH_BIT,
				    HIF_AHB_DPORT_REG_ID);

}

static inline int xradio_ahb_write_32(struct xradio_common *hw_priv,
				      u32 addr, u32 val)
{
	return xradio_ahb_write(hw_priv, addr, &val, sizeof(val));
}
#endif
#ifdef CONFIG_DRIVER_V821
int xradio_get_freq_offset_from_efuse(u8 *freq_offset);
int xradio_set_freq_offset(struct xradio_common *hw_priv, u8 freq_offset);
int xradio_get_freq_offset(struct xradio_common *hw_priv, u8 *freq_offset, u8 *dcxo_from_a_die);
#endif
#endif /* XRADIO_HWIO_H_INCLUDED */
