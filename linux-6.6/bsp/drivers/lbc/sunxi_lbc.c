/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner LBC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include "sunxi_lbc.h"

//#define LBC_DEBUG

#define LBC_VERSION "1.0.3"

#define TRANSFER_MODE_DIRECT_ACCESS	(0)
#define TRANSFER_MODE_INTERRUPT		(1)
#define TRANSFER_MODE_DMA			(2)

#define TRANSFER_WIDTH_8_BIT		(0)
#define TRANSFER_WIDTH_16_BIT		(1)
#define TRANSFER_WIDTH_32_BIT		(2)

/* support power manager API */
#if IS_ENABLED(CONFIG_PM)
static int sunxi_lbc_suspend(struct device *dev)
{
	dev_info(dev, "lbc suspend okay\n");

	return 0;
}

static int sunxi_lbc_resume(struct device *dev)
{
	dev_info(dev, "lbc resume okay\n");

	return 0;
}

static const struct dev_pm_ops sunxi_lbc_pm_ops = {
	.suspend = sunxi_lbc_suspend,
	.resume = sunxi_lbc_resume,
};

#define SUNXI_MBUS_PM_OPS (&sunxi_lbc_pm_ops)
#else
#define SUNXI_MBUS_PM_OPS NULL
#endif

static void calibrate_delay_chain(void __iomem *reg_base, u32 delay_value)
{
	u32 val;

	if (delay_value <= 63 && delay_value != 0) {
		val = readl(reg_base + LOCAL_BUSR_REG_SAMP_DL) | (delay_value);
		writel(val, reg_base + LOCAL_BUSR_REG_SAMP_DL);
		mb();
		mb();
		val = readl(reg_base + LOCAL_BUSR_REG_SAMP_DL) | (1 << 7);
		writel(val, reg_base + LOCAL_BUSR_REG_SAMP_DL);

		val = readl(reg_base + LOCAL_BUSR_REG_SAMP_DL);
		val |= 0x1 << 15;
		writel(val, reg_base + LOCAL_BUSR_REG_SAMP_DL);
		mb();
		udelay(100);
	}

	val = readl(reg_base + LOCAL_BUSR_REG_SAMP_DL);
	val &= ~(0x1 << 15);
	writel(val, reg_base + LOCAL_BUSR_REG_SAMP_DL);
	mb();
	udelay(100);
}

static void tx_fifo_write(void __iomem *reg_base, u32 iodl, u8 *txdata)
{
	u32 send_data_cnt;
	for (send_data_cnt = 0; send_data_cnt < iodl; send_data_cnt++) {
		writeb(*txdata, reg_base + LOCAL_BUSR_REG_WFER);        //write data to write entrance
		mb();

		writel(0x40, reg_base + LOCAL_BUSR_REG_IRPR);			//w1c, clear tx full int
		txdata++;
	}
}

static void rx_fifo_read(struct sunxi_lbc_t *lbc, u32 rx_trig_level)
{
	u32 read_cnt;
	u32 data = 0;
	for (read_cnt = 0; read_cnt < rx_trig_level; read_cnt++) {	//trig level is byte number while read is in word
		data = readl(lbc->reg_addr + LOCAL_BUSR_REG_WLR) >> 24 ;
		if (data) {
			if (lbc->rx_index >= DMA_DATA_MAX)
				lbc->rx_index = 0;

			lbc->receive_buf[lbc->rx_index] = readb(lbc->reg_addr + LOCAL_BUSR_REG_RFER);	//read buffer from read entrance
			lbc->rx_index++;
		}
		mb();
	}
}

static void lbc_direction_ccr_ctrl(void __iomem *reg_base, u32 write_read_mode_select)
{
	int val = 0;

	val = readl(reg_base + LOCAL_BUSR_REG_CCR);

	if (write_read_mode_select == 1) {			//write
		val |= (0x01 << 1);
		writel(val, reg_base + LOCAL_BUSR_REG_CCR);
	} else if (write_read_mode_select == 0) {	//read
		val &= (~(0x01 << 1));
		writel(val, reg_base + LOCAL_BUSR_REG_CCR);
	}
}

static void cpu_transfer_start(void __iomem *reg_base)
{
	int val = 0;
	val = readl(reg_base + LOCAL_BUSR_REG_CSR);
	val |= (0x01 << 0);
	writel(val, reg_base + LOCAL_BUSR_REG_CSR);
}

static void sunxi_lbc_dump_dma_desc(struct __lbc_idma_des *desc, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		u32 *x = (u32 *)(desc + i);
		pr_info("\t%d [0x%08lx]: %08x %08x %08x %08x\n",
				i, (unsigned long)(&desc[i]),
				x[0], x[1], x[2], x[3]);

	}
	pr_info("\n");
}

static void dma_transfer_start(void __iomem *reg_base)
{
	unsigned int val = 0;
	val = readl(reg_base + LOCAL_BUSR_REG_CSR);
	val |= CFG_DMA_START;
	writel(val, reg_base + LOCAL_BUSR_REG_CSR);
}

static void sunxi_lbc_dma_set_target_addr(struct __lbc_idma_des *p, unsigned int addr)
{
	p->target_start_addr = addr;
}

static void sunxi_lbc_desc_init_chain(struct __lbc_idma_des *p, unsigned long addr, unsigned int size)
{
	unsigned long dma_phy = addr;

	p->desc0.reg.desc_31_vld_r = 1;
	p->desc0.reg.desc_30_wback_r = 0;				    //last descriptor, no write back
	p->desc0.reg.desc_29_srcsel_r = 1;					//source select = descriptor
	p->desc0.reg.desc_24_dma_fin_r = 1;					//last descriptor, finish descriptor
	p->desc0.reg.desc_19_drq_vld_r = 0;
	p->next_desc_addr = dma_phy >> 2;

	p->target_start_addr = 0;							//FPGA Data address
}

static ssize_t lbc_write_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int i = 0;
	int ret = 0;
	unsigned long timeout = 0;
	unsigned int ahb_write_addr = 0;			//(10000000~17FFFFFF)&&(0x0fffffff)
	int write_fifo_cnt = 0;
	unsigned int *pw = NULL;

	struct sunxi_lbc_t *lbc_dev = NULL;

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	ret = gpio_direction_output(lbc_dev->fpga_direct_io, 1);
	if (ret < 0) {
		pr_err("%s->%d\n", __func__, __LINE__);
		return ret;
	}
	mdelay(1000);


	//if (strncmp(buf, "start", 5) == 0) {
	//	dev_info(dev, "write test start with mode %d\n", lbc_dev->transfer_mode);
	//} else {
	//	dev_err(dev, "invalid input\n");
	//	return len;
	//}


	for (i = 0; i < TEST_DATA_SIZE; i++) {
		lbc_dev->receive_buf[i] = get_random_int() % 256;
		lbc_dev->send_buf[i] = i;
	}

	if (lbc_dev->transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {

		pw = (unsigned int *)lbc_dev->send_buf;
		write_fifo_cnt = TEST_DATA_SIZE / 4;

		for (i = 0; i < write_fifo_cnt; i++) {
			writel(*pw, (lbc_dev->data_addr + ahb_write_addr));
			ahb_write_addr += 0x4;
			pw++;
		}
	} else if (lbc_dev->transfer_mode == TRANSFER_MODE_INTERRUPT) {
		lbc_direction_ccr_ctrl(lbc_dev->reg_addr, LBC_WRITE);	//TURN TO TX Direction

		cpu_transfer_start(lbc_dev->reg_addr);

		mb();
		tx_fifo_write(lbc_dev->reg_addr, TEST_DATA_SIZE, lbc_dev->send_buf);
		mb();

		timeout = wait_event_timeout(lbc_dev->tx_wait, lbc_dev->tx_result, 10*HZ);
		if (0 == timeout) {
			dev_info(dev, "%s->%d, tx time out\n", __func__, __LINE__);
		} else {
			dev_info(dev, "%s->%d , tx ok\n", __func__, __LINE__);
		}
		lbc_dev->tx_result = TRANS_NONE;
	} else if (lbc_dev->transfer_mode == TRANSFER_MODE_DMA) {
		for (i = 0; i < DMA_DATA_MAX; i++) {
			lbc_dev->dma_tx_buffer[i] = i;
		}

		writel((lbc_dev->dma_tx_phy >> 2), (lbc_dev->reg_addr + LOCAL_BUSR_REG_DESC_SADDR));
#ifdef LBC_DEBUG
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_tx, 1);
#endif
		dma_transfer_start(lbc_dev->reg_addr);

		timeout = wait_event_timeout(lbc_dev->dma_wait, lbc_dev->dma_result, 10*HZ);
		if (0 == timeout) {
			dev_err(dev, "%s[%d]: dma tx time out\n", __func__, __LINE__);
			return -EFAULT;
		} else {
			dev_info(dev, "%s->%d, tx ok\n", __func__, __LINE__);
		}
	}

	return len;
}

static ssize_t lbc_fpga_io_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int ret;
	struct sunxi_lbc_t *lbc_dev = NULL;

	//if (strncmp(buf, "reset", 5) == 0) {
	//	dev_info(dev, "start resetting fpga\n");
	//} else {
	//	dev_err(dev, "invalid input\n");
	//	return len;
	//}

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	ret = gpio_direction_output(lbc_dev->fpga_reset_io, 0);
	if (ret < 0) {
		dev_err(dev, "%s->%d\n", __func__, __LINE__);
		return ret;
	}

	mdelay(2000);

	ret = gpio_direction_output(lbc_dev->fpga_reset_io, 1);
	if (ret < 0) {
		dev_err(dev, "%s->%d\n", __func__, __LINE__);
		return ret;
	}
	mdelay(300);
	return len;
}

static ssize_t lbc_read_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	int i = 0;
	unsigned long timeout = 0;
	unsigned int ahb_read_addr = 0;
	struct sunxi_lbc_t *lbc_dev = NULL;
	int ret = 0;
	int read_fifo_cnt = 0;
	unsigned int *pr = NULL;

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	//if (strncmp(buf, "start", 5) == 0) {
	//	dev_info(dev, "read test start with mode %d\n", lbc_dev->transfer_mode);
	//} else {
	//	dev_err(dev, "invalid input\n");
	//	return len;
	//}
	ret = gpio_direction_output(lbc_dev->fpga_direct_io, 0);
	if (ret < 0) {
		pr_err("%s->%d\n", __func__, __LINE__);
		return ret;
	}
	mdelay(1000);

	if (lbc_dev->transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {

		read_fifo_cnt = TEST_DATA_SIZE/4;
		pr = (unsigned int *)lbc_dev->receive_buf;
		for (i = 0; i < read_fifo_cnt; i++) { //word num
			*pr = readl((lbc_dev->data_addr + ahb_read_addr)); //read data from direct access addr
			pr++;
			ahb_read_addr += 0x4;
		}

	} else if (lbc_dev->transfer_mode == TRANSFER_MODE_INTERRUPT) {
		lbc_direction_ccr_ctrl(lbc_dev->reg_addr, LBC_READ);
		cpu_transfer_start(lbc_dev->reg_addr);
		timeout = wait_event_timeout(lbc_dev->rx_wait, lbc_dev->rx_result, 10*HZ);
		if (0 == timeout) {
			dev_info(dev, "%s->%d, rx time out\n", __func__, __LINE__);
		} else {
			dev_info(dev, "%s->%d , rx ok\n", __func__, __LINE__);
		}
		lbc_dev->rx_result = TRANS_NONE;
		mdelay(50);
	} else if (lbc_dev->transfer_mode == TRANSFER_MODE_DMA) {
		writel((lbc_dev->dma_rx_phy >> 2), (lbc_dev->reg_addr + LOCAL_BUSR_REG_DESC_SADDR));
#ifdef LBC_DEBUG
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_rx, 1);
#endif
		dma_transfer_start(lbc_dev->reg_addr);

		timeout = wait_event_timeout(lbc_dev->dma_wait, lbc_dev->dma_result, 10*HZ);
		if (0 == timeout) {
			dev_err(dev, "%s[%d]: dma rx time out\n", __func__, __LINE__);
			return -EFAULT;
		} else  {
			dev_info(dev, "%s->%d , dma rx ok\n", __func__, __LINE__);
		}

		for (i = 0; i < 16; i++) {
			pr_info("dma_rx_buffer[%d] = 0x%x\n", i, lbc_dev->dma_rx_buffer[i]);
		}
	}

	if (lbc_dev->transfer_mode != TRANSFER_MODE_DMA) {
		for (i = 0; i < TEST_DATA_SIZE; i++) {
			if (lbc_dev->receive_buf[i] != lbc_dev->send_buf[i]) {
				dev_info(dev, "check err, [%d] receive_buf = 0x%x, send_buf = 0x%x\n", \
					i, lbc_dev->receive_buf[i], lbc_dev->send_buf[i]);
				break;
			}
		}
		if (i == TEST_DATA_SIZE) {
			dev_info(dev, "check data ok\n");
		}
	}

	return len;
}


static ssize_t lbc_config_message(struct device *dev,
					 struct device_attribute *da, char *buf)
{
	unsigned int len = 0;
	struct sunxi_lbc_t *lbc_dev = NULL;

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	dev_info(dev, "lbc version %s\n", LBC_VERSION);
	dev_info(dev, "now config message: \n"
						"transfer_mode: %d\n"
						"transfer_width: %d\n"
						"lbc_freq: %d\n",
						lbc_dev->transfer_mode,
						lbc_dev->transfer_width,
						lbc_dev->lbc_freq);

	dev_info(dev, "[tx] desc virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_tx,
						(long unsigned int)lbc_dev->dma_tx_phy);

	dev_info(dev, "[rx] desc virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_rx,
						(long unsigned int)lbc_dev->dma_rx_phy);

	dev_info(dev, "[tx] data virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_tx_buffer,
						(long unsigned int)lbc_dev->dma_tx_buffer_phy);

	dev_info(dev, "[rx] data virtual first address 0x%08lx, physical first address 0x%08lx\n",
						(long unsigned int)lbc_dev->dma_rx_buffer,
						(long unsigned int)lbc_dev->dma_rx_buffer_phy);

	return len;
}

static void lbc_clk_init(struct sunxi_lbc_t *chip)
{
	int ret = 0;
	u32 rate = 0;
	struct device *dev = chip->dev;

	reset_control_assert(chip->lbc_rst);

	ret = reset_control_deassert(chip->lbc_rst);
	if (ret) {
		dev_err(dev, "lbc reset failed\n");
	}

	ret = clk_prepare_enable(chip->lbc_pll);
	if (ret) {
		dev_err(dev, "lbc pll enable failed\n");
	}

	ret = clk_set_parent(chip->lbc, chip->lbc_pll);
	if (ret) {
		dev_err(dev, "lbc pll parent set failed\n");
	}

	rate = clk_round_rate(chip->lbc, chip->lbc_freq);
	ret = clk_set_rate(chip->lbc, rate);
	if (ret) {
		dev_err(dev, "lbc clk rate set failed\n");
	}

	ret = clk_prepare_enable(chip->lbc_nsi_pll);
	if (ret) {
		dev_err(dev, "lbc nsi pll enable failed\n");
	}

	ret = clk_set_parent(chip->lbc_nsi_ahb, chip->lbc_nsi_pll);
	if (ret) {
		dev_err(dev, "lbc nsi pll parent set failed\n");
	}

	ret = clk_prepare_enable(chip->lbc_nsi_ahb);
	if (ret) {
		dev_err(dev, "lbc nsi ahb set failed\n");
	}

	ret = clk_prepare_enable(chip->bus_lbc);
	if (ret) {
		dev_err(dev, "lbc bus clk enalbe failed\n");
	}

	ret = clk_prepare_enable(chip->lbc);
	if (ret) {
		dev_err(dev, "lbc clk enable failed\n");
	}

	dev_info(dev, "lbc clks init done\n");
}

static void lbc_hw_init(struct sunxi_lbc_t *chip)
{

	struct device *dev = chip->dev;
	int transfer_mode = chip->transfer_mode;
	int transfer_width = chip->transfer_width;
	int ret = 0;

	//----------- 0x000 -----------
	//u32 ccr = 0 					; //register name
	u32 ccr_local_bus_mode_en = LBC_ON 	;
	u32 ccr_dp_pin_select = DPPerByte   ; //0:one dp per byte 1:one dp only
	u32 ccr_dp_select = ODDPARITY   ; //0:odd 1:even
	u32 ccr_dp_en = 0 				; //0:unable 1:enable
	u32 ccr_rx_negative_sel = 1     ; //0:not invert phase 1:invert phase
	u32 ccr_tx_negative_sel = 1     ; //0:not invert phase 1:invert phase
	u32 ccr_big_end = 0 			; //0:big_endian 1:little_endian
	u32 ccr_apn = 0 				; //ADDR PHASE NUM 0:1 cycle ADDR  1:2 cycle; ADDR(not valid when ADDR WIDTH is 4 bytes)
	u32 ccr_aws = WIDTH_SIZE_8 	; //ADDR WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
	u32 ccr_dws = WIDTH_SIZE_8 	; //DATA WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
	u32 ccr_lbcs = 0 				; //Local bus clock source 0:from ccu 1:from gpio
	u32 ccr_lbrwc = LBC_WRITE_MODE 	; //Local bus upcoming read/write control 0:read mode 1:write mode


	//----------- 0x008 -----------
	u32 irer = RFDIP | TCIE | TOIP | OVER_UNDER_FLOW_INT | ODPIP ;

	//----------- 0x010 -----------
	u32 iodlr = 0                   ; //register name
	u32 iodlr_drq_vld = 0           ; //0 ignore drq; 1 transer untill drq valid
	u32 iodlr_burst = SINGLE        ; //burst type
	u32 iodlr_iodl = 256            ; //bytes number; input/output data length, must align with data width;

	//----------- 0x014 -----------
	u32 ar = 0x00000000             ; //target staring address

	//----------- 0x01C -----------
	u32 trig_level = 0              ; //register name
	u32 rx_empty_trig_level = 16    ; //rx empty trigger value
	u32 tx_full_trig_level = 32     ; //tx empty trigger value

	//----------- 0x034 -----------
	u32 rwcr = 0                    ; //ready wait cycle register
	u32 drc_burst = 0               ; //direct access burst
	u32 ready_wait_cycle_num = 500  ; //time out ready wait cycle
	//----------- 0x038 -----------
	u32 phase_cycle_reg = 0 	 	; //phase cycle register
	u32 phase_cycle_reg_nrad = 0	; //read addr phase(actual = nrad+1)
	u32 phase_cycle_reg_nrdd = 0	; //read data phase
	u32 phase_cycle_reg_nwad = 0	; //write addr phase
	u32 phase_cycle_reg_nwdd = 0	; //write data phase
	u32 phase_cycle_reg_nr = 0		; //cycle num in recovery phase
	u32 phase_cycle_reg_rdsp = 0	; //read data phase sample point, should less or equal nrdd

	//----------- 0x03C -----------
	u32 intr_control = 0 				; //INTR Control register
	u32 intr_control_dwcn = 0			; //the counts of hodl cycles from DMA last signal to dma_active_high
	u32 intr_control_intr_enable = 0	; //intr enable
	u32 intr_control_intr_polarity = 0	; //intr polarity 0:rising edge 1:falling edge
	u32 intr_control_drq_enable = 0		; //drq enable
	u32 intr_control_drq_polarity = 0	; //drq polarity,0 = high level, 1= low level

	unsigned int value;



	ret = gpio_direction_output(chip->fpga_direct_io, 0);
	if (ret < 0) {
		pr_err("%s->%d\n", __func__, __LINE__);
		//return ret;
	}
	mdelay(1000);

	/*----------------------local bus master top cfg--------------------------*/
	iodlr_burst = SINGLE;
	drc_burst = 0;

	if (transfer_width == TRANSFER_WIDTH_8_BIT) {
		ccr_dp_pin_select = DPPerByte;
		ccr_aws = WIDTH_SIZE_8; //ADDR WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
		ccr_dws = WIDTH_SIZE_8; //DATA WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
		ccr_dp_en = 0;
		ccr_big_end = 0;
		phase_cycle_reg_nr = 0;
		iodlr_burst = SINGLE;
		iodlr_iodl = TEST_DATA_SIZE;
		ready_wait_cycle_num = 500;
	} else if (transfer_width == TRANSFER_WIDTH_16_BIT) {
		ccr_dp_pin_select = DPONE;
		ccr_aws = WIDTH_SIZE_16; //ADDR WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
		ccr_dws = WIDTH_SIZE_16; //DATA WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
		//ccr_dp_en = 1;
		ccr_dp_en = 0;
		ccr_big_end = 1;
		phase_cycle_reg_nr = 1;
		iodlr_burst = INCR16;
		//iodlr_iodl = 2048;
		iodlr_iodl = TEST_DATA_SIZE;
		//ready_wait_cycle_num = 400;
		ready_wait_cycle_num = 500;
	} else if (transfer_width == TRANSFER_WIDTH_32_BIT) {

		ccr_dp_pin_select = DPONE;
		ccr_aws = WIDTH_SIZE_32; //ADDR WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
		ccr_dws = WIDTH_SIZE_32; //DATA WIDTH SIZE 00:1 byte 01:2 bytes 10:4 bytes
		//ccr_dp_en = 1;
		ccr_dp_en = 0;
		ccr_big_end = 1;
		phase_cycle_reg_nr = 1;
		iodlr_burst = INCR16;
		//iodlr_iodl = 2048;
		iodlr_iodl = TEST_DATA_SIZE;
		//ready_wait_cycle_num = 400;
		ready_wait_cycle_num = 500;

	}

	if (transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		drc_burst = iodlr_burst;
	} else if (transfer_mode == TRANSFER_MODE_INTERRUPT) {
		drc_burst = 0;
	}

	value = (ccr_local_bus_mode_en<<31)|(ccr_dp_pin_select<<26)|(ccr_dp_select<<25)|(ccr_dp_en<<24)|(ccr_rx_negative_sel<<17)
			 |(ccr_tx_negative_sel<<16)|(ccr_big_end<<9)|(ccr_apn<<7)|(ccr_aws<<5)|(ccr_dws<<3)|(ccr_lbcs<<2)|(ccr_lbrwc<<1);
	writel(value, chip->reg_addr + LOCAL_BUSR_REG_CCR);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_CCR, readl(chip->reg_addr + LOCAL_BUSR_REG_CCR));

	value = readl(chip->reg_addr + LOCAL_BUSR_REG_CSR);
	value &= ~CS0VV;
	value |= CS0;
	if (transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		value &= ~CFG_DMA_START_REG;
		value &= ~CFG_DMA_START;
		value |= DIRECT_ACCESS_MODE_ON;
		value &= ~DMA_MODE_ON;
	} else if (transfer_mode == TRANSFER_MODE_INTERRUPT) {
		value &= ~CFG_DMA_START_REG;
		value &= ~CFG_DMA_START;
		value &= ~DIRECT_ACCESS_MODE_ON;
		value &= ~DMA_MODE_ON;
	} else if (transfer_mode == TRANSFER_MODE_DMA) {
		value &= ~CFG_DMA_START_REG;
		//value |= CFG_DMA_START;
		value &= ~CFG_DMA_START; /* must set bit after decs ok */
		value &= ~DIRECT_ACCESS_MODE_ON;
		value |= DMA_MODE_ON;
	}
	value &= ~SR;
	value &= ~LB_CPU_MODE_START;
	writel(value, chip->reg_addr + LOCAL_BUSR_REG_CSR);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_CSR,
			readl(chip->reg_addr + LOCAL_BUSR_REG_CSR));

	rwcr = (drc_burst<<16)|(ready_wait_cycle_num<<0);
	writel(rwcr, chip->reg_addr + LOCAL_BUSR_REG_READY_WAIT_CYCLE);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_READY_WAIT_CYCLE,
			readl(chip->reg_addr + LOCAL_BUSR_REG_READY_WAIT_CYCLE));

	iodlr = (iodlr_drq_vld<<19)|(iodlr_burst<<16)|(iodlr_iodl<<0);
	writel(iodlr, chip->reg_addr + LOCAL_BUSR_REG_IODLR);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_IODLR,
			readl(chip->reg_addr + LOCAL_BUSR_REG_IODLR));

	if (transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		u32 direct_access_start_addr = 0x10000000 	;	// bit[31:28] will be waive, using high_addr
		u32 direct_access_end_addr   = 0x17000000 	;	//
		u32 direct_access_high_addr = 0;
		writel(0x00ffffff, chip->reg_addr + LOCAL_BUSR_REG_IRPR);
		writel(direct_access_start_addr, chip->reg_addr + LOCAL_BUSR_REG_DIRECT_ACCESS_START_ADDR);
		writel(direct_access_end_addr, chip->reg_addr + LOCAL_BUSR_REG_DIRECT_ACCESS_END_ADDR);
		writel((direct_access_high_addr<<27),  chip->reg_addr + LOCAL_BUSR_REG_DIRECT_ACCESS_HIGH_ADDR);
	}

	if (transfer_mode == TRANSFER_MODE_INTERRUPT) {
		trig_level = (rx_empty_trig_level<<16)|(tx_full_trig_level<<0);
		writel(trig_level, chip->reg_addr + LOCAL_BUSR_REG_TRIG_LEVEL);
		dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_TRIG_LEVEL,
				readl(chip->reg_addr + LOCAL_BUSR_REG_TRIG_LEVEL));
	}

	phase_cycle_reg = (phase_cycle_reg_nrad<<28) | (phase_cycle_reg_nrdd<<24) | (phase_cycle_reg_nwad<<20)
					  | (phase_cycle_reg_nwdd<<16) | (phase_cycle_reg_nr<<8) | (phase_cycle_reg_rdsp);
	writel(phase_cycle_reg, chip->reg_addr + LOCAL_BUSR_REG_PHASE_CYCLE_REGISTER);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_PHASE_CYCLE_REGISTER,
			readl(chip->reg_addr + LOCAL_BUSR_REG_PHASE_CYCLE_REGISTER));

	//enable irq
	if (transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		irer = TOIP | INTR_IP| OVER_UNDER_FLOW_INT | ODPIP;
	} else if (transfer_mode == TRANSFER_MODE_INTERRUPT) {
		irer = RFDIP |TCIE |TOIP |OVER_UNDER_FLOW_INT | ODPIP;
	} else if (transfer_mode == TRANSFER_MODE_DMA) {
		irer = DES_INVLD_IP | TOIP | OVER_UNDER_FLOW_INT | ODPIP | DMA_TCIE;
	}
	writel(irer, chip->reg_addr + LOCAL_BUSR_REG_IRER);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_IRER,
			readl(chip->reg_addr + LOCAL_BUSR_REG_IRER));

	intr_control = (intr_control_dwcn<<16)|(intr_control_intr_enable<<9)|(intr_control_intr_polarity<<8)
					|(intr_control_drq_enable<<1)|(intr_control_drq_polarity);
	writel(intr_control, chip->reg_addr + LOCAL_BUSR_REG_INTR_CONTROL);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_INTR_CONTROL,
			readl(chip->reg_addr + LOCAL_BUSR_REG_INTR_CONTROL));


	writel(ar, chip->reg_addr + LOCAL_BUSR_REG_AR);
	dev_info(dev, "reg 0x%08x , value = 0x%08x\n", LOCAL_BUSR_REG_AR, readl(chip->reg_addr + LOCAL_BUSR_REG_AR));

	if (transfer_mode == TRANSFER_MODE_DMA) {
		sunxi_lbc_desc_init_chain(chip->dma_tx, (unsigned long)chip->dma_tx_phy, 1);
		chip->dma_tx->desc0.reg.desc_25_dma_wr_r = 0;
		chip->dma_tx->dma_buffer_saddr = chip->dma_tx_buffer_phy >> 2;
		chip->dma_tx->desc0.reg.desc_18t16_burst_r = (readl(chip->reg_addr + LOCAL_BUSR_REG_IODLR) & (0x70000)) >>16;
		chip->dma_tx->desc0.reg.desc_15t0_iodl = readl(chip->reg_addr + LOCAL_BUSR_REG_IODLR) & 0xffff;
		sunxi_lbc_dump_dma_desc(chip->dma_tx, 1);

		sunxi_lbc_desc_init_chain(chip->dma_rx, (unsigned long)chip->dma_rx_phy, 1);
		chip->dma_rx->desc0.reg.desc_25_dma_wr_r = 1;
		chip->dma_rx->dma_buffer_saddr = chip->dma_rx_buffer_phy >> 2;
		chip->dma_rx->desc0.reg.desc_18t16_burst_r = (readl(chip->reg_addr + LOCAL_BUSR_REG_IODLR) & (0x70000)) >>16;
		chip->dma_rx->desc0.reg.desc_15t0_iodl = readl(chip->reg_addr + LOCAL_BUSR_REG_IODLR) & 0xffff;
		sunxi_lbc_dump_dma_desc(chip->dma_rx, 1);
	}

	if (transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		calibrate_delay_chain(chip->reg_addr, 30);
	} else if (transfer_mode == TRANSFER_MODE_INTERRUPT) {
		calibrate_delay_chain(chip->reg_addr, 60);
	}
}

static ssize_t lbc_init_ctrl(struct device *dev,
					 struct device_attribute *da, const char *buf, size_t len)
{
	struct sunxi_lbc_t *lbc_dev;

	lbc_dev = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return len;
	}

	//if (strncmp(buf, "init", strlen("init")) == 0) {
	//	dev_info(dev, "init lbc%d\n", lbc_dev->transfer_mode);
	//} else {
	//	dev_err(dev, "invalid input\n");
	//	return len;
	//}

	lbc_clk_init(lbc_dev);
	lbc_hw_init(lbc_dev);

	return len;
}

static ssize_t lbc_clk_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho number > clk_freq"
			"\nnow clk_freq: %d\n", lbc_dev->lbc_freq);
}

static ssize_t lbc_clk_freq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	struct sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = kstrtou32(buf, 0, &lbc_dev->lbc_freq);
	if (ret) {
		lbc_dev->lbc_freq = 100000000;
		dev_err(dev, "set to lbc_clk_freq fail, use default value %d\n", lbc_dev->lbc_freq);
		return ret;
	}
	return count;
}

static ssize_t lbc_transfer_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~2] > transfer_mode"
			"\nnow transfer_mode: %d\n", lbc_dev->transfer_mode);
}

static ssize_t lbc_transfer_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	struct sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = kstrtou32(buf, 0, &lbc_dev->transfer_mode);
	if (ret) {
		lbc_dev->transfer_mode = 0;
		dev_err(dev, "set to transfer_mode fail, use default value %d\n", \
				lbc_dev->transfer_mode);
		return ret;
	}
	return count;
}

static ssize_t lbc_transfer_width_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);

	return sprintf(buf, "Usage:\necho [0~2] > transfer_width"
			"\nnow transfer_mode: %d\n", lbc_dev->transfer_width);
}

static ssize_t lbc_transfer_width_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	struct sunxi_lbc_t *lbc_dev = dev_get_drvdata(dev);
	int ret;

	if (IS_ERR_OR_NULL(lbc_dev)) {
		dev_err(dev, "failed to get lbc dev\n");
		return count;
	}

	ret = kstrtou32(buf, 0, &lbc_dev->transfer_width);
	if (ret) {
		lbc_dev->transfer_width = 0;
		dev_err(dev, "set to transfer_width fail, use default value %d\n", \
				lbc_dev->transfer_width);
		return ret;
	}
	return count;
}

static struct device_attribute dev_attr_lbc_init =
	__ATTR(lbc_init, 0644, NULL, lbc_init_ctrl);

static struct device_attribute dev_attr_lbc_fpga_io_ctrl =
	__ATTR(fpga_io_ctrl, 0644, NULL, lbc_fpga_io_ctrl);

static struct device_attribute dev_attr_lbc_write_ctrl =
	__ATTR(lbc_write, 0644, NULL, lbc_write_ctrl);

static struct device_attribute dev_attr_lbc_read_ctrl =
	__ATTR(lbc_read, 0644, NULL, lbc_read_ctrl);

static struct device_attribute dev_attr_lbc_msg =
	__ATTR(message, 0444, lbc_config_message, NULL);

static struct device_attribute dev_attr_lbc_clk_freq =
	__ATTR(clk_freq, 0664, lbc_clk_freq_show, lbc_clk_freq_store);

static struct device_attribute dev_attr_lbc_transfer_mode =
	__ATTR(transfer_mode, 0664, lbc_transfer_mode_show, lbc_transfer_mode_store);

static struct device_attribute dev_attr_lbc_transfer_width =
	__ATTR(transfer_width, 0664, lbc_transfer_width_show, lbc_transfer_width_store);


static struct attribute *lbc_attributes[] = {
	&dev_attr_lbc_init.attr,
	&dev_attr_lbc_fpga_io_ctrl.attr,
	&dev_attr_lbc_write_ctrl.attr,
	&dev_attr_lbc_read_ctrl.attr,
	&dev_attr_lbc_msg.attr,
	&dev_attr_lbc_clk_freq.attr,
	&dev_attr_lbc_transfer_mode.attr,
	&dev_attr_lbc_transfer_width.attr,
	NULL,
};

static struct attribute_group lbc_group = {
	.attrs = lbc_attributes,
};

static const struct attribute_group *lbc_groups[] = {
	&lbc_group,
	NULL,
};

static int lbc_open(struct inode *inode, struct file *file)
{
	struct sunxi_lbc_t *lbc_dev = container_of(inode->i_cdev, struct sunxi_lbc_t, cdev);
	file->private_data = lbc_dev;
	lbc_clk_init(lbc_dev);
	lbc_hw_init(lbc_dev);

	return 0;
}

static loff_t sunxi_lbc_llseek(struct file *file, loff_t off, int whence)
{
	struct sunxi_lbc_t *lbc_dev = file->private_data;
	loff_t newpos = 0;
    int offset = off;

	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = lbc_dev->dma_target_addr + offset;
		break;
	/*case SEEK_END:
		break; */
	default:
		return -1;
    }

    if (newpos < 0)
		return -1;

	//lbc_dev->dma_target_addr = newpos;
	lbc_dev->dma_target_addr = 0;
	//pr_info("newpos 0x%x, off 0x%x , whence 0x%x\n", newpos, off, whence);
	return newpos;
}


static int lbc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct sunxi_lbc_t *lbc_dev = file->private_data;

	vma->vm_flags |= VM_IO;
	/* addr 4K align */
	if (lbc_dev->transfer_mode == TRANSFER_MODE_DIRECT_ACCESS) {
		vma->vm_pgoff = lbc_dev->data_phy_addr >> PAGE_SHIFT;
	} else if (lbc_dev->transfer_mode == TRANSFER_MODE_DMA) {
		if (lbc_dev->dma_mmap_dir == CMD_MMAP_DMA_TX_BUF) {
			vma->vm_pgoff = lbc_dev->dma_tx_buffer_phy >> PAGE_SHIFT;
			pr_err("%s->%d, lbc_dev->dma_tx_buffer_phy 0x%08lx\n",
					__func__, __LINE__, (long unsigned int)lbc_dev->dma_tx_buffer_phy);
		} else if (lbc_dev->dma_mmap_dir == CMD_MMAP_DMA_RX_BUF) {
			vma->vm_pgoff = lbc_dev->dma_rx_buffer_phy >> PAGE_SHIFT;
			pr_err("%s->%d, lbc_dev->dma_rx_buffer_phy 0x%08lx\n",
					__func__, __LINE__, (long unsigned int)lbc_dev->dma_rx_buffer_phy);
		}
	}

	if (remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) {
		return -EAGAIN;
	}

	return 0;
}

static ssize_t sunxi_lbc_write(struct file *file,
				const char __user *data,
				size_t count, loff_t *ppos)
{
	struct sunxi_lbc_t *lbc_dev = file->private_data;
	struct device *dev = lbc_dev->dev;
	unsigned long timeout = 0;

	if (*ppos != 0)
		return -EINVAL;

	if (copy_from_user((void *)lbc_dev->dma_tx_buffer, (void __user *)data, count)) {
		return -EFAULT;
	}
	writel((lbc_dev->dma_tx_phy >> 2), (lbc_dev->reg_addr + LOCAL_BUSR_REG_DESC_SADDR));
	sunxi_lbc_dma_set_target_addr(lbc_dev->dma_tx, lbc_dev->dma_target_addr);
#ifdef LBC_DEBUG
	sunxi_lbc_dump_dma_desc(lbc_dev->dma_tx, 1);
#endif
	dma_transfer_start(lbc_dev->reg_addr);

	timeout = wait_event_timeout(lbc_dev->dma_wait, lbc_dev->dma_result, 10*HZ);
	if (0 == timeout) {
		dev_err(dev, "%s[%d]: dma tx time out\n", __func__, __LINE__);
		return -EFAULT;
	}

	lbc_dev->dma_result = TRANS_NONE;
	return 0;
}

static ssize_t sunxi_lbc_read(struct file *file,
				char __user *data,
				size_t count, loff_t *ppos)
{
	struct sunxi_lbc_t *lbc_dev = file->private_data;
	struct device *dev = lbc_dev->dev;
	unsigned long timeout = 0;

	writel((lbc_dev->dma_rx_phy >> 2), (lbc_dev->reg_addr + LOCAL_BUSR_REG_DESC_SADDR));
	sunxi_lbc_dma_set_target_addr(lbc_dev->dma_tx, lbc_dev->dma_target_addr);
#ifdef LBC_DEBUG
	sunxi_lbc_dump_dma_desc(lbc_dev->dma_rx, 1);
#endif
	dma_transfer_start(lbc_dev->reg_addr);

	timeout = wait_event_timeout(lbc_dev->dma_wait, lbc_dev->dma_result, 10*HZ);
	if (0 == timeout) {
		dev_err(dev, "%s[%d]: dma rx time out\n", __func__, __LINE__);
		return -EFAULT;
	}

	lbc_dev->dma_result = TRANS_NONE;
	if (copy_to_user((void __user *)data, (void *)lbc_dev->dma_rx_buffer, count)) {
		return -EFAULT;
	}
	return 0;
}

static long sunxi_lbc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	int ret = 1;
	struct sunxi_lbc_t *lbc_dev = file->private_data;
	struct device *dev = lbc_dev->dev;
	int buf[2];
	unsigned int irpr_tcip;
	int time_cnt;
	int i = 0;

	switch (cmd) {
	case CMD_DMA_TX_START:
		if (copy_from_user((void *)&buf, (void __user *)arg, 2*sizeof(int))) {
			return -EFAULT;
		}

		for (i = 0; i < 16; i++) {
			pr_info("%s->%d, dma_tx_buffer[%d] = 0x%x", __func__, __LINE__, i, lbc_dev->dma_tx_buffer[i]);
		}

		writel((lbc_dev->dma_tx_phy >> 2), (lbc_dev->reg_addr + LOCAL_BUSR_REG_DESC_SADDR));
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_tx, 1);
		dma_transfer_start(lbc_dev->reg_addr);

		while (1) {
			irpr_tcip = readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR) & DMA_TCIP;
			if (irpr_tcip == DMA_TCIP) {
				writel((DMA_TCIP | TCIE), (lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR));
				dev_err(dev, "%s->%d , dma tx ok\n", __func__, __LINE__);
				break;
			}
			if (++time_cnt >= 100) {
				dev_err(dev, "%s->%d, dma tx time out,  irpr_tcip 0x%x\n", __func__, __LINE__, irpr_tcip);
				break;
			}
			mdelay(10);
		}

		break;
	case CMD_DMA_RX_START:
		if (copy_from_user((void *)&buf, (void __user *)arg, 2*sizeof(int))) {
			return -EFAULT;
		}
		writel((lbc_dev->dma_rx_phy >> 2), (lbc_dev->reg_addr + LOCAL_BUSR_REG_DESC_SADDR));
		sunxi_lbc_dump_dma_desc(lbc_dev->dma_rx, 1);
		dma_transfer_start(lbc_dev->reg_addr);
		while (1) {
			irpr_tcip = readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR) & DMA_TCIP;
			if (irpr_tcip == DMA_TCIP) {
				writel((DMA_TCIP | TCIE), (lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR));
				dev_info(dev, "%s->%d , dma rx ok\n", __func__, __LINE__);
				break;
			}
			if (++time_cnt >= 100) {
				dev_info(dev, "%s->%d, dma rx time out,  irpr_tcip 0x%x\n", __func__, __LINE__, irpr_tcip);
				break;
			}
			mdelay(10);
		}

		for (i = 0; i < 16; i++) {
			pr_info("%s->%d, dma_rx_buffer[%d] = 0x%x", __func__, __LINE__, i, lbc_dev->dma_rx_buffer[i]);
		}
		break;
	case CMD_MMAP_DMA_TX_BUF:
		lbc_dev->dma_mmap_dir = CMD_MMAP_DMA_TX_BUF;
		break;
	case CMD_MMAP_DMA_RX_BUF:
		lbc_dev->dma_mmap_dir = CMD_MMAP_DMA_RX_BUF;
		break;
	default:
		break;
	}

	dev_info(dev, "cmd 0x%x , buf[0] = 0x%x, buf[1] = 0x%x\n", cmd, buf[0], buf[1]);
	return ret;
}

static struct file_operations lbc_fops = {
	.owner = THIS_MODULE,
	.open = lbc_open,
	.write = sunxi_lbc_write,
	.read = sunxi_lbc_read,
	.llseek = sunxi_lbc_llseek,
	.mmap = lbc_mmap,
	.unlocked_ioctl = sunxi_lbc_ioctl,
};

static irqreturn_t sunxi_lbc_handler(int irq, void *dev_id)
{
	u32 irq_status;
	u32 data;
	u32 data1;
	struct sunxi_lbc_t *lbc_dev = (struct sunxi_lbc_t *)dev_id;

	irq_status = readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR);

	/* dma */
	if (irq_status & DMA_TCIP) {
		writel((DMA_TCIP | TCIE), (lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR));
		lbc_dev->dma_result = TRANS_COMPLETED;
		wake_up(&lbc_dev->dma_wait);
	}

	/* Rrite Direction Transmission interrupt finish*/
	if ((irq_status & RFDIP) && (readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR) &RFDIP)) {
		data = (!((readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_CCR)) & (0x1<<1)));
		if (data) {
			data1 = readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_TRIG_LEVEL) >> 16 ;
			rx_fifo_read(lbc_dev, data1);
			writel(RFDIP, lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR);
		}
	}

	/* Write Direction Transmission interrupt finish*/
	if ((irq_status & TCIE)) {	//Transmitt finish
		writel(TCIE, lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR);
		mb();
		data = readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_CCR) & (0x1<<1);
		mb();
		data1 = !((readl(lbc_dev->reg_addr + LOCAL_BUSR_REG_CCR)) & (0x1<<1));
		mb();
		if (data) {	//if current transmit it is write direction
			lbc_dev->tx_result = TRANS_COMPLETED;
			wake_up(&lbc_dev->tx_wait);
		} else if (data1) { //if current transmit it is read direction
			lbc_dev->rx_result = TRANS_COMPLETED;
			wake_up(&lbc_dev->rx_wait);
		}
	}

	/* Ready wait cycle Over time interrupt*/
	if (irq_status & TOIP) {	//TIME OUT
		writel(TOIP, lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR);
	}

	/* OVERFLOW_UNDERFLOW error*/
	if (irq_status & OVER_UNDER_FLOW_INT) {
		writel(OVER_UNDER_FLOW_INT, lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR);
	}

	/* Data parity error*/
	if ((irq_status & DPIP) || (irq_status & ODPIP)) {
		writel((DPIP|ODPIP), lbc_dev->reg_addr + LOCAL_BUSR_REG_IRPR);
	}
	return IRQ_HANDLED;
}

static int lbc_analy_dts(struct platform_device *pdev, struct sunxi_lbc_t *lbc_dev)
{
	int ret = 0;
	struct resource *res;
	enum of_gpio_flags config;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "reg_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find reg in dts\n");
		return -1;
	}

	lbc_dev->reg_addr = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->reg_addr) {
		dev_err(dev, "reg_addr memory mapping failed\n");
		return -1;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "data_addr");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find data reg in dts\n");
		return -1;
	}

	lbc_dev->data_addr = devm_ioremap_resource(&pdev->dev, res);
	if (!lbc_dev->reg_addr) {
		dev_err(dev, "data_addr memory mapping failed\n");
		return -1;
	}
	lbc_dev->data_phy_addr = res->start;
	lbc_dev->data_addr_size = resource_size(res);

	dev_info(dev, "start addr 0x%x , size 0x%x\n", lbc_dev->data_phy_addr, lbc_dev->data_addr_size);

	lbc_dev->fpga_reset_io = of_get_named_gpio_flags(pdev->dev.of_node, "reset-fpga-gpios", 0, &config);
	if (!gpio_is_valid(lbc_dev->fpga_reset_io)) {
		dev_err(dev, "get fpga reset io %d err \n", lbc_dev->fpga_reset_io);
		return -1;
	}

	ret = devm_gpio_request(dev, lbc_dev->fpga_reset_io, "reset-fpga-gpios");
	if (ret < 0) {
		dev_err(dev, "fpga reset io request failed\n");
		return -1;
	}

	lbc_dev->fpga_direct_io = of_get_named_gpio_flags(pdev->dev.of_node, "direct-gpios", 0, &config);
	if (!gpio_is_valid(lbc_dev->fpga_direct_io)) {
		dev_err(dev, "fpga direct io %d err \n", lbc_dev->fpga_direct_io);
		return -EINVAL;
	}

	ret = devm_gpio_request(dev, lbc_dev->fpga_direct_io, "direct-gpios");
	if (ret < 0) {
		dev_err(dev, "fpga direct io request failed\n");
		return ret;
	}

	lbc_dev->io_supply = devm_regulator_get(&pdev->dev, "lbc-io");
	if (IS_ERR_OR_NULL(lbc_dev->io_supply)) {
		dev_err(dev, "Could not get lbc supply\n");
		return -1;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "lbc-io-vol", &lbc_dev->io_vol);
	if (ret < 0) {
		lbc_dev->io_vol = 3300000;
		dev_err(dev, "Could not get lbc-io-vol, use default value %d\n", lbc_dev->io_vol);
	}

	lbc_dev->lbc_pll = devm_clk_get(&pdev->dev, "lbc_pll");
	if (IS_ERR_OR_NULL(lbc_dev->lbc_pll)) {
		dev_err(dev, "Could not get lbc_pll\n");
	}

	lbc_dev->lbc_nsi_pll = devm_clk_get(&pdev->dev, "lbc_nsi_pll");
	if (IS_ERR_OR_NULL(lbc_dev->lbc_nsi_pll)) {
		dev_err(dev, "Could not get lbc_nsi_pll\n");
	}

	lbc_dev->lbc_nsi_ahb = devm_clk_get(&pdev->dev, "lbc_nsi_ahb");
	if (IS_ERR_OR_NULL(lbc_dev->lbc_nsi_ahb)) {
		dev_err(dev, "Could not get lbc_nsi_ahb\n");
	}

	lbc_dev->bus_lbc = devm_clk_get(&pdev->dev, "bus_lbc");
	if (IS_ERR_OR_NULL(lbc_dev->bus_lbc)) {
		dev_err(dev, "Could not get bus_lbc\n");
	}

	lbc_dev->lbc = devm_clk_get(&pdev->dev, "lbc");
	if (IS_ERR_OR_NULL(lbc_dev->lbc)) {
		dev_err(dev, "Could not get lbc clk\n");
	}

	lbc_dev->lbc_rst = devm_reset_control_get(&pdev->dev, "lbc_rst");
	if (IS_ERR_OR_NULL(lbc_dev->lbc_rst)) {
		dev_err(dev, "Could not get lbc rst\n");
	}

	if (of_property_read_u32(np, "clock-frequency", &lbc_dev->lbc_freq)) {
		dev_err(dev, "Could not get clock frequency\n");
		return -1;
	}

	if (of_property_read_u32(np, "transfer_mode", &lbc_dev->transfer_mode)) {
		dev_err(dev, "Could not get transfer_mode\n");
		return -1;
	}

	if (lbc_dev->transfer_mode > TRANSFER_MODE_DMA) {
		dev_err(dev, "Set transfer_mode value %d err, max value %d\n", \
				lbc_dev->transfer_mode, TRANSFER_MODE_DMA);
		return -1;
	}

	if (of_property_read_u32(np, "transfer_width", &lbc_dev->transfer_width)) {
		dev_err(dev, "Could not get transfer_width\n");
		return -1;
	}

	if (lbc_dev->transfer_width > TRANSFER_WIDTH_32_BIT) {
		dev_err(dev, "Set transfer_width value %d err, max value %d\n", \
				lbc_dev->transfer_width, TRANSFER_WIDTH_32_BIT);
		return -1;
	}

	return 0;
}

static int do_dma_alloc(struct sunxi_lbc_t *lbc_dev)
{
	int ret = 0;
	struct device *dev = lbc_dev->dev;

	lbc_dev->dma_tx_buffer = dma_alloc_coherent(lbc_dev->dev,
									DMA_DATA_MAX,
									&lbc_dev->dma_tx_buffer_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_tx_buffer) {
		dev_err(dev, "failed to alloc tx buffer dma coherent\n");
		ret = -1;
		goto out_err;
	}

	lbc_dev->dma_rx_buffer = dma_alloc_coherent(lbc_dev->dev,
									DMA_DATA_MAX,
									&lbc_dev->dma_rx_buffer_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_rx_buffer) {
		dev_err(dev, "failed to alloc rx buffer dma coherent\n");
		ret = -1;
		goto tx_buffer_del;
	}

	lbc_dev->dma_tx = dma_alloc_coherent(lbc_dev->dev,
									DMA_DESCS_SIZE,
									&lbc_dev->dma_tx_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_tx) {
		dev_err(dev, "failed to alloc tx dma desc coherent\n");
		ret = -1;
		goto rx_buffer_del;
	}

	lbc_dev->dma_rx = dma_alloc_coherent(lbc_dev->dev,
									DMA_DESCS_SIZE,
									&lbc_dev->dma_rx_phy,
									GFP_KERNEL);
	if (!lbc_dev->dma_rx) {
		dev_err(dev, "failed to alloc rx dma desc coherent\n");
		ret = -1;
		goto tx_desc_del;
	}

	dev_info(dev, "dma alloc coherent done\n");
	return 0;

tx_desc_del:
	dma_free_coherent(lbc_dev->dev, DMA_DESCS_SIZE, lbc_dev->dma_tx, lbc_dev->dma_tx_phy);
rx_buffer_del:
	dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX, lbc_dev->dma_rx_buffer, lbc_dev->dma_rx_buffer_phy);
tx_buffer_del:
	dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX, lbc_dev->dma_tx_buffer, lbc_dev->dma_tx_buffer_phy);
out_err:
	return ret;
}

static int lbc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	char *name_str = "lbc";
	struct sunxi_lbc_t *sunx_lbc_dev = NULL;

	dev_info(dev, "local bus driver probe, version %s\n", LBC_VERSION);

	sunx_lbc_dev = devm_kzalloc(dev, sizeof(*sunx_lbc_dev), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunx_lbc_dev)) {
		dev_err(dev, "sunx_lbc_dev err\n");
		goto out_err;
	}

	sunx_lbc_dev->pdev = pdev;
	sunx_lbc_dev->dev = &pdev->dev;
	sunx_lbc_dev->rx_index = 0;

	ret = register_chrdev_region(MKDEV(LBC_MAJOR, 0), LBC_MINOR, name_str);
	if (ret)
		goto lbc_del;

	cdev_init(&sunx_lbc_dev->cdev, &lbc_fops);
	sunx_lbc_dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&sunx_lbc_dev->cdev, MKDEV(LBC_MAJOR, 0), 1);
	if (ret) {
		dev_err(dev, "add cdev fail\n");
		goto region_del;
	}

	sunx_lbc_dev->lbc_class = class_create(THIS_MODULE, name_str);
	if (IS_ERR(sunx_lbc_dev->lbc_class)) {
		ret = PTR_ERR(sunx_lbc_dev->lbc_class);
		goto cdev_del;
	}

	sunx_lbc_dev->dev_lbc = device_create_with_groups(sunx_lbc_dev->lbc_class,
						      dev, MKDEV(LBC_MAJOR, 0), NULL,
						      lbc_groups, "%s", name_str);

	//device_create_with_groups
	if (IS_ERR(sunx_lbc_dev->dev_lbc)) {
		dev_err(dev, "failed to create device with groups\n");
		goto class_del;
	}

	ret = lbc_analy_dts(pdev, sunx_lbc_dev);
	if (ret < 0)
		goto group_del;

	sunx_lbc_dev->irq = platform_get_irq(pdev, 0);
	if (sunx_lbc_dev->irq < 0) {
		dev_err(dev, "Could not get irq \n");
		goto group_del;
	}

	ret = devm_request_irq(&pdev->dev, sunx_lbc_dev->irq, sunxi_lbc_handler, \
							IRQF_SHARED, dev_name(&pdev->dev), sunx_lbc_dev);
	if (ret) {
		 dev_err(dev, "Could not request irq %d\n", sunx_lbc_dev->irq);
		 goto group_del;
	}

	ret = do_dma_alloc(sunx_lbc_dev);
	if (ret < 0)
		goto group_del;

	regulator_set_voltage(sunx_lbc_dev->io_supply, \
						sunx_lbc_dev->io_vol, sunx_lbc_dev->io_vol);
	ret = regulator_enable(sunx_lbc_dev->io_supply);
	if (ret) {
		dev_err(dev, "failed to enable regulator\n");
	}

	sunx_lbc_dev->tx_result = TRANS_NONE;
	sunx_lbc_dev->rx_result = TRANS_NONE;
	sunx_lbc_dev->dma_result = TRANS_NONE;

	init_waitqueue_head(&sunx_lbc_dev->tx_wait);
	init_waitqueue_head(&sunx_lbc_dev->rx_wait);
	init_waitqueue_head(&sunx_lbc_dev->dma_wait);

	dev_set_drvdata(sunx_lbc_dev->dev_lbc, sunx_lbc_dev);
	platform_set_drvdata(pdev, sunx_lbc_dev);

	dev_info(dev, "local bus driver probe ok ...\n");
	return 0;

group_del:
	device_remove_groups(sunx_lbc_dev->dev_lbc, lbc_groups);
	device_destroy(sunx_lbc_dev->lbc_class, MKDEV(LBC_MAJOR, 0));
class_del:
	class_destroy(sunx_lbc_dev->lbc_class);
cdev_del:
	cdev_del(&sunx_lbc_dev->cdev);
region_del:
	unregister_chrdev_region(MKDEV(LBC_MAJOR, 0), LBC_MINOR);
lbc_del:
	devm_kfree(dev, sunx_lbc_dev);
out_err:
	dev_err(dev, "probed failed\n");
	return ret;
}

static int lbc_remove(struct platform_device *pdev)
{
	struct sunxi_lbc_t *lbc_dev = platform_get_drvdata(pdev);


	if (lbc_dev->dma_rx)
		dma_free_coherent(lbc_dev->dev, DMA_DESCS_SIZE,
						  lbc_dev->dma_rx, lbc_dev->dma_rx_phy);

	if (lbc_dev->dma_tx)
		dma_free_coherent(lbc_dev->dev, DMA_DESCS_SIZE,
						  lbc_dev->dma_tx, lbc_dev->dma_tx_phy);

	if (lbc_dev->dma_rx_buffer)
		dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX,
						  lbc_dev->dma_rx_buffer, lbc_dev->dma_rx_buffer_phy);

	if (lbc_dev->dma_tx_buffer)
		dma_free_coherent(lbc_dev->dev, DMA_DATA_MAX,
						  lbc_dev->dma_tx_buffer, lbc_dev->dma_tx_buffer_phy);


	if (lbc_dev->dev_lbc) {
		device_remove_groups(lbc_dev->dev_lbc, lbc_groups);
		device_destroy(lbc_dev->lbc_class, MKDEV(LBC_MAJOR, 0));
		lbc_dev->dev_lbc = NULL;
	}

	class_destroy(lbc_dev->lbc_class);
	cdev_del(&lbc_dev->cdev);
	platform_set_drvdata(pdev, NULL);
	unregister_chrdev_region(MKDEV(LBC_MAJOR, 0), LBC_MINOR);

	return 0;
}

static const struct of_device_id sunxi_lbc_matches[] = {
	{.compatible = "allwinner,sunxi-lbc", },
	{},
};

static struct platform_driver lbc_driver = {
	.driver = {
		.name   = "sunxi-lbc",
		.pm     = SUNXI_MBUS_PM_OPS,
		.of_match_table = sunxi_lbc_matches,
	},
	.probe = lbc_probe,
	.remove = lbc_remove,
};

module_platform_driver(lbc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SUNXI localbus support");
MODULE_AUTHOR("wujiayi");
MODULE_VERSION(LBC_VERSION);
