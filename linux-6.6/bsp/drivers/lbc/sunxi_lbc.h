/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
 /*
  * SUNXI NPD support
  *
  * Copyright (C) 2015 AllWinnertech Ltd.
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  */

#define DRIVER_NAME     "LBC"
#define LBC_MAJOR		111
#define LBC_MINOR		222

#define LBC_ON			1
#define LBC_OFF			0

#define DPPerByte		0
#define DPONE			1
#define ODDPARITY		0
#define EVENPARITY		1

#define WIDTH_SIZE_8	0x0
#define WIDTH_SIZE_16	(0x1<<0)
#define WIDTH_SIZE_32	(0x1<<1)

#define LBC_READ_MODE	0
#define LBC_WRITE_MODE	1
//Burst TYPE Select
#define SINGLE			0
#define INCR2			1
#define FIX4			2
#define INCR4			3
#define FIX16			4
#define INCR16			5
#define FIX128			6
#define INCR128			7

#define LBC_WRITE		1
#define LBC_READ		0
#define SOC_TX_DDMA_READ		0
#define SOC_RX_DDMA_WRITE		1



/*-------------basic address---------------*/
#define LOCAL_BUS_REG_BASE						(0x02810000)
#define LOCAL_BUS_DAT_BASE						(0x10000000)

/*-------------offset address--------------*/
#define LOCAL_BUSR_REG_CCR						(0x00)
#define LOCAL_BUSR_REG_CSR						(0x04)
#define LOCAL_BUSR_REG_IRER						(0x08)
#define LOCAL_BUSR_REG_IRPR						(0x0c)
#define LOCAL_BUSR_REG_IODLR					(0x10)
#define LOCAL_BUSR_REG_AR						(0x14)
#define LOCAL_BUSR_REG_TRIG_LEVEL				(0x1c)
#define LOCAL_BUSR_REG_WLR						(0x20)
#define LOCAL_BUSR_REG_SAMP_DL					(0x24)
#define LOCAL_BUSR_REG_INTAR					(0x28)
#define LOCAL_BUSR_REG_DDR_SADDR				(0x2c)
#define LOCAL_BUSR_REG_DESC_SADDR				(0x30)
#define LOCAL_BUSR_REG_PHASE_CYCLE_REGISTER     (0x38)
#define LOCAL_BUSR_REG_INTR_CONTROL             (0x3c)

#define LOCAL_BUSR_REG_READY_WAIT_CYCLE         (0x34)
#define LOCAL_BUSR_REG_DIRECT_ACCESS_START_ADDR (0x40)
#define LOCAL_BUSR_REG_DIRECT_ACCESS_END_ADDR   (0x44)
#define LOCAL_BUSR_REG_DIRECT_ACCESS_HIGH_ADDR  (0x48)
#define LOCAL_BUSR_REG_DIRECT_ACCESS_STATUS     (0x54)

#define LOCAL_BUSR_STATUS_REG					(0x58)
#define LOCAL_BUSR_REG_DMA_DESC0_DEBUG			(0x5c)
#define LOCAL_BUSR_REG_DMA_DESC1_DEBUG			(0x60)
#define LOCAL_BUSR_REG_DMA_DESC2_DEBUG			(0x64)
#define LOCAL_BUSR_REG_DMA_DESC3_DEBUG			(0x68)
#define LOCAL_BUSR_REG_FSM_DEBUG				(0x6c)
#define LOCAL_BUSR_REG_DELAY_CHAIN				(0x70)

#define DIRECT_ACCESS_START_ADDRESS_REG			(0x40)
#define DIRECT_ACCESS_END_ADDRESS_REG			(0x44)
#define DIRECT_ACCESS_HIGH_ADDRESS_REG			(0x48)
#define DIRECT_ACCESS_STATUS_REG				(0x54)

#define LOCAL_BUSR_REG_WFER						(0x100)
#define LOCAL_BUSR_REG_RFER						(0x200)

#define LOCAL_BUSR_SLAVE_REG_1_FPGA				(0x80)
#define LOCAL_BUSR_SLAVE_REG_2_FPGA				(0x84)
#define LOCAL_BUSR_SLAVE_REG_3_FPGA				(0x88)
#define LOCAL_BUSR_SLAVE_REG_4_FPGA				(0x8C)

#define TUEIP				(1<<0) //TX FIFO Underflow Error Interrupt Pending
#define TOEIP				(1<<1) //TX OVERFLOW
#define RUEIP				(1<<2) //RX UNDERFLOW
#define ROEIP				(1<<3) //RX OVERFLOW

#define TCIE				(1<<4) //Transmission Complete
#define TOIP				(1<<5) //Time Out
#define TFDIP				(1<<6) //TX FIFO DATA INT --- not using this interrupt
#define RFDIP				(1<<7) //RX FIFO DATA INT
#define DMA_TCIP			(1<<8) //DMA Transmission Complete

#define DMA_TCIE			(1<<8)

#define INTR_IP				(1<<10) //INTR INT
#define DES_INVLD_IP		(1<<11) //DES INVALID
#define ODPIP				(1<<12) //One-bit data parity pending
#define DPIP				(1<<13) //Data parity pending

#define RD_BUF_H_UNDERFLOW	(1<<14) //
#define RD_BUF_H_OVERFLOW	(1<<15) //
#define WR_BUF_H_UNDERFLOW	(1<<16) //
#define WR_BUF_H_OVERFLOW	(1<<17) //
#define CMD_FIFO_UNDERFLOW	(1<<18) //
#define CMD_FIFO_OVERFLOW	(1<<19) //

#define RD_BUF_L_UNDERFLOW  (1<<20) //
#define RD_BUF_L_OVERFLOW	(1<<21) //
#define WR_BUF_L_UNDERFLOW	(1<<22) //
#define WR_BUF_L_OVERFLOW	(1<<23) //

/*CCR*/
#define LOCAL_BUS_MODE_EN	BIT(31)
#define DP_PIN_SELECT		BIT(26)
#define DP_SELECT			BIT(25)
#define DP_EN				BIT(24)
#define LB_RX_NEGATIVE_SEL	BIT(17)
#define LB_TX_NEGATIVE_SEL	BIT(16)
#define BIG_END				BIT(9)
#define APN					BIT(7)
#define LBCS				BIT(2)
#define LBRWC				BIT(1)

#define AWS_1BYTE			(0x00)
#define AWS_2BYTE			(0x01)
#define AWS_4BYTE			(0x10)
#define DWS_1BYTE			(0x00)
#define DWS_2BYTE			(0x01)
#define DWS_4BYTE			(0x10)

/*CSR*/
#define CS3VV					BIT(19)
#define CS2VV					BIT(18)
#define CS1VV					BIT(17)
#define CS0VV					BIT(16)
#define CS3						BIT(15)
#define CS2						BIT(14)
#define CS1						BIT(13)
#define CS0						BIT(12)
#define CFG_DMA_START_REG		BIT(9)
#define CFG_DMA_START			BIT(8)
#define DIRECT_ACCESS_MODE_ON	BIT(3)
#define DMA_MODE_ON				BIT(2)
#define SR						BIT(1)
#define LB_CPU_MODE_START		BIT(0)

#define OVER_UNDER_FLOW_INT		(0xffc00f)

#define LBC_IS_WRITE_DIRECTION  ((get_wvalue(LOCAL_BUSR_REG_CCR)) & (0x1 << 1))
#define LBC_IS_READ_DIRECTION 	(!((get_wvalue(LOCAL_BUSR_REG_CCR)) & (0x1 << 1)))
#define LBC_RX_BUFFER_EMPTY     ((get_wvalue(LOCAL_BUSR_REG_WLR) >> 24) == 0)
#define RX_TRIG_LEVEL 			(get_wvalue(LOCAL_BUSR_REG_TRIG_LEVEL) >> 16)
#define TRX_IODL				(get_wvalue(LOCAL_BUSR_REG_IODLR) & 0xffff)

#define BYTES_10M				(100*1024) //100KB

#define INCR_CLR				1
#define NOT_INCR_CLR			0


#define TEST_DATA_SIZE	(256)
#define DMA_DESCS_SIZE  (1 * sizeof(struct __lbc_idma_des))
#define DMA_DATA_MAX	(4*1024)

#define TRANS_NONE		0
#define TRANS_COMPLETED	1
#define TRANS_ERROR		2



/* kernel & user cmd */
#define CMD_DMA_TX_START 0x0a
#define CMD_DMA_RX_START 0x0b
#define CMD_MMAP_DMA_TX_BUF 0x0c
#define CMD_MMAP_DMA_RX_BUF 0x0d

typedef union {
	struct {
		unsigned int desc_15t0_iodl:16;			//how many bytes data to w/r
		unsigned int desc_18t16_burst_r:3;      //burst
		unsigned int desc_19_drq_vld_r:1;       //drq valid
		unsigned int reversed0:4;    			//23:20 bit not use
		unsigned int desc_24_dma_fin_r:1;       //dma finish flag
		unsigned int desc_25_dma_wr_r:1;        //dma wr select 0:read 1:write
		unsigned int reversed1:3;     			//28:26 bit not use
		unsigned int desc_29_srcsel_r:1;		//parameter config source from 0:reg 1:dma_desc
		unsigned int desc_30_wback_r:1;         //dma write back flag
		unsigned int desc_31_vld_r:1;			//dma description is/not valid
	} reg;
	unsigned int all;
} sunxi_lbc_desc0_u;

/* IDMC structure */
typedef struct __lbc_idma_des {
	sunxi_lbc_desc0_u desc0;
	unsigned int dma_buffer_saddr;              //dram start addr
	unsigned int next_desc_addr;
	unsigned int target_start_addr;             //localbus start addr (fpag)
} __attribute__((packed)) lbc_idma_des;

struct sunxi_lbc_t {
	struct cdev cdev;
	struct device *dev_lbc;
	dev_t devid;
	struct class *lbc_class;
	int major;

	struct platform_device *pdev;
	struct device *dev;

	struct clk *lbc_pll;
	struct clk *lbc_nsi_pll;
	struct clk *lbc_nsi_ahb;
	struct clk *bus_lbc;
	struct clk *lbc;
	struct reset_control *lbc_rst;
	int lbc_freq;
	int transfer_mode;
	int transfer_width;

	struct regulator *io_supply;
	unsigned int io_vol;

	void __iomem *reg_addr;
	void __iomem *data_addr;
	unsigned int data_phy_addr;
	unsigned int data_addr_size;

	void __iomem *data_len;

	unsigned int ahb_rate;
	uint32_t period;
	uint32_t cycle;
	spinlock_t bwlock;
	int irq;

	int fpga_reset_io;
	int fpga_direct_io;

	struct __lbc_idma_des *dma_tx;
	dma_addr_t dma_tx_phy;

	struct __lbc_idma_des *dma_rx;
	dma_addr_t dma_rx_phy;

	u8 *dma_tx_buffer;
	dma_addr_t dma_tx_buffer_phy;

	u8 *dma_rx_buffer;
	dma_addr_t dma_rx_buffer_phy;

	unsigned int dma_target_addr;
	int dma_mmap_dir;

	u8 receive_buf[TEST_DATA_SIZE];
	u8 send_buf[TEST_DATA_SIZE];
	int rx_index;
	volatile int tx_result;
	volatile int rx_result;
	volatile int dma_result;

	wait_queue_head_t tx_wait;
	wait_queue_head_t rx_wait;
	wait_queue_head_t dma_wait;
};
