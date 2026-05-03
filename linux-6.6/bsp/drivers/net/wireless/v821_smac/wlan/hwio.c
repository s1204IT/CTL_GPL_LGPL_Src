/*
 * Hardware I/O implementation for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>

#include "xradio.h"
#include "hwio.h"
#include "sbus.h"
#if (CONFIG_WLAN_FUNC_CALL_SUPPORT)
#include "xr_func.h"
#endif

#define CHECK_ADDR_LEN  1

#if (CONFIG_HIF_BY_PASS)
#include "hifbypass.h"
#if (CONFIG_HIF_BYPASS_USE_DMA)
#include "driver/chip/hal_dma.h"
#include "kernel/os/os_semaphore.h"
XR_OS_Semaphore_t		 DMA_Sem;
DMA_Channel		  DMA_ch = DMA_CHANNEL_INVALID;
DMA_ChannelInitParam DMA_Param;
static int HIF_Bypass_DMA_Transfer(void *dst, void *src, u32 len)
{
	XR_DRV_Status ret;
	HAL_DMA_Start(DMA_ch, (u32)src, (u32)dst, len);
	ret = XR_OS_SemaphoreWait(&DMA_Sem, 5000);
	if (ret != XR_DRV_OK)
		xradio_dbg("%s sem wait failed: %d", __func__, ret);
	HAL_DMA_Stop(DMA_ch);
	return (ret != XR_DRV_OK);
}

static void HIF_Bypass_DMA_Finish(void *arg)
{
	XR_OS_SemaphoreRelease(&DMA_Sem);
}

int HIF_Bypass_DMA_Init(void)
{
	u32 dma_width_type = DMA_DATA_WIDTH_32BIT;
	u32 dma_burst_type = DMA_BURST_LEN_4;
	DMA_ch = HAL_DMA_Request();
	if (DMA_ch == DMA_CHANNEL_INVALID) {
		xradio_dbg("%s,%d\n", __func__, __LINE__);
		return -1;
	}
	DMA_Param.irqType = DMA_IRQ_TYPE_END;
	DMA_Param.endCallback = (DMA_IRQCallback)HIF_Bypass_DMA_Finish;
	DMA_Param.endArg = NULL;
	DMA_Param.cfg = HAL_DMA_MakeChannelInitCfg(DMA_WORK_MODE_SINGLE,
			DMA_WAIT_CYCLE_2,
			DMA_BYTE_CNT_MODE_REMAIN,
			dma_width_type, /*DMA0 width 32bit*/
			dma_burst_type,   /*DMA0 burst 4*/
			DMA_ADDR_MODE_INC,
			DMA_PERIPH_SRAM,
			dma_width_type,   /*DMA1 width 32bit*/
			dma_burst_type,   /*DMA1 burst 4*/
			DMA_ADDR_MODE_INC,
			DMA_PERIPH_SRAM);
	HAL_DMA_Init(DMA_ch, &DMA_Param);
	XR_OS_SemaphoreCreate(&DMA_Sem, 0, 1);
	writel(readl(0x40001008)|(1<<16), 0x40001008);
	xradio_dbg("%s width=%d, burst=%d\n", __func__, dma_width_type, dma_burst_type);
	return 0;
}

void HIF_Bypass_DMA_Deinit(void)
{
	if (DMA_CHANNEL_INVALID != DMA_ch) {
		HAL_DMA_DeInit(DMA_ch);
		HAL_DMA_Release(DMA_ch);
		DMA_ch = DMA_CHANNEL_INVALID;
		XR_OS_SemaphoreDelete(&DMA_Sem);
	}
}

static inline int HIF_Bypass_Read(void *addr, void *buf, u32 len)
{
	if (len < HIF_BYPASS_USE_DMA_SIZE)
		memcpy(buf, addr, len);
	else
		HIF_Bypass_DMA_Transfer(buf, addr, len);
	return 0;
}

static inline int HIF_Bypass_Write(void *addr, const void *buf, u32 len)
{
	if (len < HIF_BYPASS_USE_DMA_SIZE)
		memcpy(addr, buf, len);
	else
		HIF_Bypass_DMA_Transfer(addr, (void *)buf, len);
	return 0;
}
#else
#define HIF_Bypass_Read(addr, buf, len)   memcpy(buf, addr, len)
#define HIF_Bypass_Write(addr, buf, len)  memcpy(addr, buf, len)
#endif

#include "wsm.h"
HIF_QUEUE *pHif_bypass = NULL;
volatile s32 rx_data_cnt;
volatile s32 exception_flag;

u32 FW_ADDR_TO_APP_ADDR(u32 addr)
{
	if ((addr & 0xFFF00000) == 0xFFF00000) {
		addr = (SILICON_AHB_MEMORY_ADDRESS + (addr & 0x1FFFF)); /*max 128K*/
#ifdef USE_ADDRESS_REMAP_IN_LINUX
		addr = (u32)AHB_SHARE_RAM_ADDR_BASE + (addr & 0x1FFFF);
		return addr;
#endif
	} else if ((addr & PAS_MEMORY_ADDRESS) == PAS_MEMORY_ADDRESS) {
		u32 offset = (addr & 0x3FFFF);
		if ((addr & 0x09400000) == 0x09400000) {
			if (!pHif_bypass) {
				xradio_dbg(XRADIO_DBG_ERROR, "%s:HIF_QUEUE is not init! Error Addr=0x%08x.\n", __func__, addr);
				return (u32)NULL;
			}
			if (offset >= pHif_bypass->rx_fifo_len)
				offset = offset - pHif_bypass->rx_fifo_len;
			addr = pHif_bypass->rx_fifo_addr + offset;
			xradio_dbg(XRADIO_DBG_MSG, "%s: Rx Addr=0x%08x(offset=%d).\n", __func__, addr, offset);
#ifdef USE_ADDRESS_REMAP_IN_LINUX
			return addr;
#endif
		} else {
#ifdef USE_ADDRESS_REMAP_IN_LINUX
			addr = (u32)(PAS_SHARE_RAM_ADDR_BASE + offset);
			return addr;
#else
			addr = (SILICON_PAC_BASE_ADDRESS + offset);
#endif
		}
	}
#ifdef USE_ADDRESS_REMAP_IN_LINUX
	else if ((addr & AHB_MEMORY_ADDRESS) == AHB_MEMORY_ADDRESS) {
		addr = (u32)AHB_SHARE_RAM_ADDR_BASE + (addr & 0x1FFFF);
		return addr;
	}
#endif
	return XRADIO_FWMEM_FW2NET(addr);
}

extern int get_etf_mode(void);

int xradio_hif_bypass_enable(void)
{
	xradio_dbg(XRADIO_DBG_MSG, "%s: HIF_OV_CTRL=0x%08x.\n", __func__, REG_READ_U32(HIF_OV_CTRL));
	REG_WRITE_U32(HIF_OV_CTRL, HIF_OV_CTRL_RESET_VALUE);

#if (CONFIG_HIF_BYPASS_USE_DMA)
	HIF_Bypass_DMA_Init();
#endif
#if (CONFIG_WLAN_FUNC_CALL_SUPPORT)
	if (get_etf_mode())
		xradio_func_call_init_etf();
	else
		xradio_func_call_init();
#endif
	return 0;
}

void xradio_hif_bypass_disable(void)
{
#if (CONFIG_WLAN_SUPPORT_BLE_CONFIG_RFIP_TRIM)
	extern enum suspend_state_t pm_get_mode(void);
	if (xradio_support_ble_config_rfip_trim_en) {
		if ((pm_get_mode() == PM_MODE_HIBERNATION) ||
			(xradio_get_ble_state() == BLE_STATE_FLAG_PWR_DOWN)) {
			REG_WRITE_U32(HIF_OV_CTRL, OV_DISABLE_CPU_CLK|OV_RESET_CPU);
		}
	} else {
		REG_WRITE_U32(HIF_OV_CTRL, OV_DISABLE_CPU_CLK|OV_RESET_CPU);
	}
#else
	REG_WRITE_U32(HIF_OV_CTRL, OV_DISABLE_CPU_CLK|OV_RESET_CPU);
#endif

#if (CONFIG_HIF_BYPASS_USE_DMA)
	HIF_Bypass_DMA_Deinit();
#endif
#if (CONFIG_WLAN_FUNC_CALL_SUPPORT)
	if (get_etf_mode())
		xradio_func_call_deinit_etf();
	else
		xradio_func_call_deinit();

#endif
}

int xradio_hif_bypass_init(void)
{
	int wait_hif_rdy = 500;
	HIF_CTRL *pCtrl = (HIF_CTRL *)HIF_BYPASS_CTRL_ADDR;

	while (pCtrl->state_magic != HIF_BYPASS_FW_RDY) {
		msleep(1);
		if (--wait_hif_rdy < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: HIF Bypass not ready, Magic(0x%08x)=0x%08x.\n", __func__,
					(u32)&pCtrl->state_magic, pCtrl->state_magic);
			return -1;
		}
	}
	pHif_bypass = (HIF_QUEUE *)FW_ADDR_TO_APP_ADDR(pCtrl->queue_addr);
	if (NULL == pHif_bypass)
		return -1;

	pHif_bypass->rx_fifo_addr = FW_ADDR_TO_APP_ADDR(pHif_bypass->rx_fifo_addr);
	xradio_dbg(XRADIO_DBG_MSG, "%s: HIF_BYPASS_ADDR=0x%08x, Rx Addr=0x%08x(len=%d).\n", __func__,
		(u32)pHif_bypass, pHif_bypass->rx_fifo_addr, pHif_bypass->rx_fifo_len);
	/* tell fw, host is rdy.*/
	pCtrl->state_magic = HIF_BYPASS_HOST_RDY;

	/*Enable TX/RX irq*/
	REG_SET_BIT(HIF_OV_TX0_INT_CTRL, HIF_OV_INT_EN);
	REG_SET_BIT(HIF_OV_TX1_INT_CTRL, HIF_OV_INT_EN);
	REG_SET_BIT(HIF_OV_RX0_INT_CTRL, HIF_OV_INT_EN);
#if (CONFIG_WLAN_FUNC_CALL_SUPPORT)
	REG_SET_BIT(HIF_OV_TX2_INT_CTRL, HIF_OV_INT_EN);
	REG_SET_BIT(HIF_OV_RX1_INT_CTRL, HIF_OV_INT_EN);
#endif
	//REG_SET_BIT(HIF_OV_RDY_INT_CTRL, HIF_OV_INT_EN);
	xradio_dbg(XRADIO_DBG_MSG, "%s: HIF_OV_RX0_INT_CTRL=0x%08x.\n", __func__, REG_READ_U32(HIF_OV_RX0_INT_CTRL));

	return 0;
}

int xradio_check_rx(void)
{
	if (is_device_awake() && NULL != pHif_bypass) {
		u32 read_idx = pHif_bypass->rx_ctrl.read_idx;
		if (pHif_bypass->exception) { /*FW exception*/
			struct wsm_hdr *msg = (struct wsm_hdr *)FW_ADDR_TO_APP_ADDR(pHif_bypass->exception);
			exception_flag = 1;
			return msg->len;
		} else {
			exception_flag = 0;
		}

		if (INDEX_BEFORE(read_idx, pHif_bypass->rx_ctrl.write_idx)) {
			struct wsm_hdr *msg = (struct wsm_hdr *)FW_ADDR_TO_APP_ADDR(pHif_bypass->rx_queue[read_idx & RX_QUEUE_MASK]);
			xradio_dbg(XRADIO_DBG_MSG, "%s: msg=0x%08x, id=0x%04x, len=%d.\n",
				__func__, (u32)msg, msg->id, msg->len);

			if (INDEX_DIFF(pHif_bypass->rx_ctrl.write_idx, pHif_bypass->rx_ctrl.buffer_idx) >= 60)
				xradio_dbg(XRADIO_DBG_ERROR, "%s: buffer_idx=%d, write_idx=%d.\n",
					__func__, pHif_bypass->rx_ctrl.buffer_idx, pHif_bypass->rx_ctrl.write_idx);
			//if (msg->id != 0x0804 || msg->len > 1600 || msg->len < 8)
			//  xradio_dbg("%s: msg=0x%08x, id=0x%04x, len=%d.\n",
			// 	 __func__, (u32)msg, msg->id, msg->len);
			return msg->len;
		} else {
			if (!INDEX_BEFORE_EQ(read_idx, pHif_bypass->rx_ctrl.write_idx))
				xradio_dbg(XRADIO_DBG_ERROR, "%s: read_idx=%d(%d), write_idx=%d.\n", __func__,
					read_idx, pHif_bypass->rx_ctrl.read_idx, pHif_bypass->rx_ctrl.write_idx);
			else
				xradio_dbg(XRADIO_DBG_MSG, "%s: read_idx=%d, write_idx=%d.\n", __func__,
						  pHif_bypass->rx_ctrl.read_idx, pHif_bypass->rx_ctrl.write_idx);
		}
	}
	return 0;
}

#if PATCH_NOT_DISABLE_WLAN_IRQ_IN_HANDLER
extern u32 hifbypass_irq_num;
#endif
int xradio_data_read(struct xradio_common *hw_priv, void *buf, size_t buf_len)
{
	if (buf) {
		u32 read_idx = pHif_bypass->rx_ctrl.read_idx;
		if (INDEX_BEFORE(read_idx, pHif_bypass->rx_ctrl.write_idx) || exception_flag) {
			int copy_size	 = 0;
			u32 rx_end_addr = (pHif_bypass->rx_fifo_addr + pHif_bypass->rx_fifo_len);
			struct wsm_hdr *msg = NULL;
			if (exception_flag) {
				msg = (struct wsm_hdr *)FW_ADDR_TO_APP_ADDR(pHif_bypass->exception);
				pHif_bypass->exception = 0;
			} else {
				msg = (struct wsm_hdr *)FW_ADDR_TO_APP_ADDR(pHif_bypass->rx_queue[read_idx & RX_QUEUE_MASK]);
			}
			if (!msg)
				return -3;

			copy_size = (msg->len + 3) & (~0x3);
			if (buf_len < msg->len) {
				xradio_dbg(XRADIO_DBG_ERROR, "%s: buffer is too small=%d, data=%d.\n",
					__func__, buf_len,  msg->len);
				copy_size = buf_len;
			}

			/*for rx fifo wrap.*/
			if (((u32)msg + copy_size) > rx_end_addr && (u32)msg < rx_end_addr) {
				int first_len =  rx_end_addr - (u32)msg;
				HIF_Bypass_Read((void *)msg, buf, first_len);
				msg = (struct wsm_hdr *)pHif_bypass->rx_fifo_addr;
				buf = (void *)((u32)buf + first_len);
				copy_size -= first_len;
				xradio_dbg(XRADIO_DBG_MSG, "%s: first_len=%d, id=0x%04x, len=%d.\n",
					__func__, first_len, msg->id, msg->len);
			}
			HIF_Bypass_Read((void *)msg, buf, copy_size);

			if (!exception_flag) {
				xradio_dbg(XRADIO_DBG_MSG, "%s: read_idx=%d, id=0x%04x, len=%d.\n",
					__func__, read_idx, msg->id, msg->len);
				read_idx++;
				pHif_bypass->rx_ctrl.read_idx = read_idx;
#if PATCH_NOT_DISABLE_WLAN_IRQ_IN_HANDLER
				s32 diff = INDEX_DIFF(pHif_bypass->rx_ctrl.write_idx, read_idx);
				u32 flags = hifbypass_irq_num;
				disable_irq(flags);
				rx_set_cnt(diff);
				enable_irq(flags);
#else /* wlan irq is disable, so there is no need to disable irq */
				rx_set_cnt(INDEX_DIFF(pHif_bypass->rx_ctrl.write_idx, read_idx));
#endif
				if (INDEX_DIFF(read_idx, pHif_bypass->rx_ctrl.buffer_idx) == 1)
					REG_SET_BIT(HIF_OV_TX1_INT_CTRL, HIF_OV_INT_RDY_FLAG); //set hif rx complete.
			}
		} else {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: read_idx=%d, write_idx=%d, exception=%d.\n", __func__,
				read_idx,	pHif_bypass->rx_ctrl.write_idx, exception_flag);
			return -2;
		}
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: buffer is NULL.\n", __func__);
		return -1;
	}
	return 0;
}

int xradio_data_write(struct xradio_common *hw_priv, const void *buf, size_t buf_len)
{
	int copy_size = buf_len;
	if (buf_len < sizeof(struct wsm_hdr) ||
		buf_len > hw_priv->wsm_caps.sizeInpChBuf) {
		copy_size = hw_priv->wsm_caps.sizeInpChBuf;
		xradio_dbg(XRADIO_DBG_ERROR, "%s: msg length invalid = %d.\n", __func__, buf_len);
	}
	if (buf && copy_size) {
		u32 write_idx =  pHif_bypass->tx_ctrl.write_idx;
		if (INDEX_BEFORE(write_idx, pHif_bypass->tx_ctrl.buffer_idx)) {
			void *dst = (void *)FW_ADDR_TO_APP_ADDR(pHif_bypass->tx_queue[write_idx & TX_QUEUE_MASK]);
			if (!dst)
				return -3;
			HIF_Bypass_Write(dst, buf, copy_size);
			xradio_dbg(XRADIO_DBG_MSG, "%s: write_idx=%d, msg_id=0x%04x, len=%d.\n",
				__func__, write_idx, ((struct wsm_hdr *)dst)->id, ((struct wsm_hdr *)dst)->len);
			write_idx++;
			pHif_bypass->tx_ctrl.write_idx = write_idx;
			if (INDEX_DIFF(write_idx, pHif_bypass->tx_ctrl.read_idx) == 1)
				REG_SET_BIT(HIF_OV_TX0_INT_CTRL, HIF_OV_INT_RDY_FLAG); //set hif tx.
		} else {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: FW no buffer, write_idx=%d buffer_idx=%d.\n",
				__func__, write_idx, pHif_bypass->tx_ctrl.buffer_idx);

			return -2;
		}
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: buffer=0x%0x, copy_size=%d.\n", __func__, (u32)buf, copy_size);
		return -1;
	}
	return 0;
}
#endif


#if (!CONFIG_HIF_BY_PASS)
 /* Sdio addr is 4*spi_addr */
#define SPI_REG_ADDR_TO_SDIO(spi_reg_addr) ((spi_reg_addr) << 2)
#define SDIO_ADDR17BIT(buf_id, mpf, rfu, reg_id_ofs) \
				((((buf_id)    & 0x1F) << 7) \
				| (((mpf)        & 1) << 6) \
				| (((rfu)        & 1) << 5) \
				| (((reg_id_ofs) & 0x1F) << 0))
#define MAX_RETRY		3


static int __xradio_read(struct xradio_common *hw_priv, u16 addr,
			 void *buf, size_t buf_len, int buf_id)
{
	u16 addr_sdio;
	u32 sdio_reg_addr_17bit ;

#if (CHECK_ADDR_LEN)
	/* Check if buffer is aligned to 4 byte boundary */
	if (SYS_WARN(((unsigned long)buf & 3) && (buf_len > 4))) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: buffer is not aligned.\n",
			    __func__);
		return -EINVAL;
	}
#endif

	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);
	SYS_BUG(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_data_read(hw_priv->sbus_priv,
						 sdio_reg_addr_17bit,
						 buf, buf_len);
}

static int __xradio_write(struct xradio_common *hw_priv, u16 addr,
			      const void *buf, size_t buf_len, int buf_id)
{
	u16 addr_sdio;
	u32 sdio_reg_addr_17bit ;

#if (CHECK_ADDR_LEN)
	/* Check if buffer is aligned to 4 byte boundary */
	if (SYS_WARN(((unsigned long)buf & 3) && (buf_len > 4))) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: buffer is not aligned.\n",
			    __func__);
		return -EINVAL;
	}
#endif

	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);

	SYS_BUG(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_data_write(hw_priv->sbus_priv,
						  sdio_reg_addr_17bit,
						  buf, buf_len);
}

static inline int __xradio_read_reg32(struct xradio_common *hw_priv,
				       u16 addr, u32 *val)
{
	int ret = 0;

	*hw_priv->sbus_priv->val32_r = 0;
	ret = __xradio_read(hw_priv, addr, hw_priv->sbus_priv->val32_r, sizeof(u32), 0);
	*val = *hw_priv->sbus_priv->val32_r;

	return ret;
}

static inline int __xradio_write_reg32(struct xradio_common *hw_priv,
					u16 addr, u32 val)
{
	int ret = 0;

	*hw_priv->sbus_priv->val32_w = val;
	ret = __xradio_write(hw_priv, addr, hw_priv->sbus_priv->val32_w, sizeof(u32), 0);
	*hw_priv->sbus_priv->val32_w = 0;

	return ret;
}

int xradio_reg_read(struct xradio_common *hw_priv, u16 addr,
		    void *buf, size_t buf_len)
{
	int ret;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	*hw_priv->sbus_priv->val32_r = 0;
	ret = __xradio_read(hw_priv, addr, hw_priv->sbus_priv->val32_r, buf_len, 0);
	*(u32 *)buf = *hw_priv->sbus_priv->val32_r;
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_reg_write(struct xradio_common *hw_priv, u16 addr,
		     const void *buf, size_t buf_len)
{
	int ret;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	*hw_priv->sbus_priv->val32_w = *(u32 *)buf;
	ret = __xradio_write(hw_priv, addr, hw_priv->sbus_priv->val32_w, buf_len, 0);
	*hw_priv->sbus_priv->val32_w = 0;
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_data_read(struct xradio_common *hw_priv, void *buf, size_t buf_len)
{
	int ret, retry = 1;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	{
		int buf_id_rx = hw_priv->buf_id_rx;
		while (retry <= MAX_RETRY) {
			ret = __xradio_read(hw_priv, HIF_IN_OUT_QUEUE_REG_ID, buf,
					    buf_len, buf_id_rx + 1);
			if (!ret) {
				buf_id_rx = (buf_id_rx + 1) & 3;
				hw_priv->buf_id_rx = buf_id_rx;
				break;
			} else {
				retry++;
				mdelay(1);
				sbus_printk(XRADIO_DBG_ERROR, "%s, error :[%d]\n",
					    __func__, ret);
			}
		}
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_data_write(struct xradio_common *hw_priv, const void *buf,
		      size_t buf_len)
{
	int ret, retry = 1;
	SYS_BUG(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	{
		int buf_id_tx = hw_priv->buf_id_tx;
		while (retry <= MAX_RETRY) {
			ret = __xradio_write(hw_priv, HIF_IN_OUT_QUEUE_REG_ID, buf,
					     buf_len, buf_id_tx);
			if (!ret) {
				buf_id_tx = (buf_id_tx + 1) & 31;
				hw_priv->buf_id_tx = buf_id_tx;
				break;
			} else {
				retry++;
				mdelay(1);
				sbus_printk(XRADIO_DBG_ERROR, "%s,error :[%d]\n",
					    __func__, ret);
			}
		}
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_indirect_read(struct xradio_common *hw_priv, u32 addr, void *buf,
			 size_t buf_len, u32 prefetch, u16 port_addr)
{
	u32 val32 = 0;
	int i, ret;

	if ((buf_len / 2) >= 0x1000) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't read more than 0xfff words.\n",
			    __func__);
		return -EINVAL;
		goto out;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	/* Write address */
	ret = __xradio_write_reg32(hw_priv, HIF_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write address register.\n", __func__);
		goto out;
	}

	/* Read CONFIG Register Value - We will read 32 bits */
	ret = __xradio_read_reg32(hw_priv, HIF_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't read config register.\n", __func__);
		goto out;
	}

	/* Set PREFETCH bit */
	ret = __xradio_write_reg32(hw_priv, HIF_CONFIG_REG_ID, val32 | prefetch);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write prefetch bit.\n", __func__);
		goto out;
	}

	/* Check for PRE-FETCH bit to be cleared */
	for (i = 0; i < 20; i++) {
		ret = __xradio_read_reg32(hw_priv, HIF_CONFIG_REG_ID, &val32);
		if (ret < 0) {
			sbus_printk(XRADIO_DBG_ERROR,
				    "%s: Can't check prefetch bit.\n", __func__);
			goto out;
		}
		if (!(val32 & prefetch))
			break;
		mdelay(i);
	}

	if (val32 & prefetch) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Prefetch bit is not cleared.\n", __func__);
		goto out;
	}

	/* Read data port */
	if (buf_len == sizeof(u32)) {
		*hw_priv->sbus_priv->val32_r = 0;
		ret = __xradio_read(hw_priv, port_addr, hw_priv->sbus_priv->val32_r, buf_len, 0);
		*(u32 *)buf = *hw_priv->sbus_priv->val32_r;
	} else {
		ret = __xradio_read(hw_priv, port_addr, buf, buf_len, 0);
	}

	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't read data port.\n", __func__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_apb_write(struct xradio_common *hw_priv, u32 addr, const void *buf,
		     size_t buf_len)
{
	int ret;

	if ((buf_len / 2) >= 0x1000) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't wrire more than 0xfff words.\n", __func__);
		return -EINVAL;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);

	/* Write address */
	ret = __xradio_write_reg32(hw_priv, HIF_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write address register.\n", __func__);
		goto out;
	}

	/* Write data port */
	if (buf_len == sizeof(u32)) {
		memcpy(hw_priv->sbus_priv->val32_w, buf, sizeof(u32));
		ret = __xradio_write(hw_priv, HIF_SRAM_DPORT_REG_ID,
				hw_priv->sbus_priv->val32_w, buf_len, 0);
		memset(hw_priv->sbus_priv->val32_w, 0, sizeof(u32));
	} else {
		ret = __xradio_write(hw_priv, HIF_SRAM_DPORT_REG_ID, buf, buf_len, 0);
	}

	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR, "%s: Can't write data port.\n",
			    __func__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int xradio_ahb_write(struct xradio_common *hw_priv, u32 addr, const void *buf,
		     size_t buf_len)
{
	int ret;

	if ((buf_len / 2) >= 0x1000) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't wrire more than 0xfff words.\n",
			     __func__);
		return -EINVAL;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);

	/* Write address */
	ret = __xradio_write_reg32(hw_priv, HIF_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write address register.\n", __func__);
		goto out;
	}

	/* Write data port */
	if (buf_len == sizeof(u32)) {
		memcpy(hw_priv->sbus_priv->val32_w, buf, sizeof(u32));
		ret = __xradio_write(hw_priv, HIF_AHB_DPORT_REG_ID,
				hw_priv->sbus_priv->val32_w, buf_len, 0);
		memset(hw_priv->sbus_priv->val32_w, 0, sizeof(u32));
	} else {
		ret = __xradio_write(hw_priv, HIF_AHB_DPORT_REG_ID, buf, buf_len, 0);
	}

	if (ret < 0) {
		sbus_printk(XRADIO_DBG_ERROR,
			    "%s: Can't write data port.\n", __func__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}
#endif /* !CONFIG_HIF_BY_PASS */
