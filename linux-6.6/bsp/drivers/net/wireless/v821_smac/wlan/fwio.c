/*
 * Firmware I/O implementation for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/firmware.h>

#include "xradio.h"
#include "fwio.h"
#include "hwio.h"
#include "sbus.h"
#include "bh.h"
#include "hifbypass.h"

/* Macroses are local. */
#ifdef CONFIG_ACCESS_WIFI_DIRECT
#define APB_WRITE(reg, val) \
		REG_WRITE_U32(SILICON_APB_ADDR(reg), val)

#define APB_READ(reg, val) \
		do { \
			val = REG_READ_U32(SILICON_APB_ADDR(reg)); \
		} while (0)
#else
#define APB_WRITE(reg, val) \
	do { \
		ret = xradio_apb_write_32(hw_priv, APB_ADDR(reg), (val)); \
		if (ret < 0) { \
			xradio_dbg(XRADIO_DBG_ERROR, \
				"%s: can't write %s at line %d.\n", \
				__func__, #reg, __LINE__); \
			goto error; \
		} \
	} while (0)
#define APB_READ(reg, val) \
	do { \
		ret = xradio_apb_read_32(hw_priv, APB_ADDR(reg), &(val)); \
		if (ret < 0) { \
			xradio_dbg(XRADIO_DBG_ERROR, \
				"%s: can't read %s at line %d.\n", \
				__func__, #reg, __LINE__); \
			goto error; \
		} \
	} while (0)
#define REG_WRITE(reg, val) \
	do { \
		ret = xradio_reg_write_32(hw_priv, (reg), (val)); \
		if (ret < 0) { \
			xradio_dbg(XRADIO_DBG_ERROR, \
				"%s: can't write %s at line %d.\n", \
				__func__, #reg, __LINE__); \
			goto error; \
		} \
	} while (0)
#define REG_READ(reg, val) \
	do { \
		ret = xradio_reg_read_32(hw_priv, (reg), &(val)); \
		if (ret < 0) { \
			xradio_dbg(XRADIO_DBG_ERROR, \
				"%s: can't read %s at line %d.\n", \
				__func__, #reg, __LINE__); \
			goto error; \
		} \
	} while (0)
#endif

static int xradio_get_hw_type(u32 config_reg_val, int *major_revision)
{
	int hw_type = -1;
	u32 hif_type = (config_reg_val >> 24) & 0xF;
	/*u32 hif_vers = (config_reg_val >> 31) & 0x1;*/

	/* Check if we have XRADIO */
	if (hif_type == 0x4 || hif_type == 0x5) {
		hw_type = HIF_HW_TYPE_XRADIO;
		*major_revision = hif_type;
	} else {
		/*hw type unknown.*/
		*major_revision = 0x0;
	}
	return hw_type;
}

/*
 * This function is called to Parse the SDD file
 * to extract some informations
 */
static int xradio_parse_sdd(struct xradio_common *hw_priv, u32 *dpll, u8 *trim)
{
	int ret = 0;
	const char *sdd_path = NULL;
	struct xradio_sdd *pElement = NULL;
	int parsedLength = 0;

	sta_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	SYS_BUG(hw_priv->sdd != NULL);

	/* select and load sdd file depend on hardware version. */
	switch (hw_priv->hw_revision) {
	case XR819_HW_REV0:
		sdd_path = XR819_SDD_FILE;
		break;
	case XR819_HW_REV1:
		sdd_path = XR819S_SDD_FILE;
		break;
	default:
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: unknown hardware version.\n", __func__);
		return ret;
	}

#ifdef CONFIG_XRADIO_ETF
	if (etf_is_connect()) {
		const char *etf_sdd = etf_get_sddpath();
		if (etf_sdd != NULL)
			sdd_path = etf_sdd;
		else
			etf_set_sddpath(sdd_path);
	}
#endif

#ifdef USE_VFS_FIRMWARE
	hw_priv->sdd = xr_request_file(sdd_path);
	if (unlikely(!hw_priv->sdd)) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't load sdd file %s.\n",
			   __func__, sdd_path);
		return -ENOENT;
	}
#else
	ret = request_firmware(&hw_priv->sdd, sdd_path, hw_priv->pdev);
	if (unlikely(ret)) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't load sdd file %s.\n",
			   __func__, sdd_path);
		return ret;
	}
#endif

	/*parse SDD config.*/
	hw_priv->is_BT_Present = false;
	pElement = (struct xradio_sdd *)hw_priv->sdd->data;
	parsedLength += (FIELD_OFFSET(struct xradio_sdd, data) + \
			 pElement->length);
	pElement = FIND_NEXT_ELT(pElement);

	while (parsedLength < hw_priv->sdd->size) {
		switch (pElement->id) {
		case SDD_PTA_CFG_ELT_ID:
			hw_priv->conf_listen_interval =
			    (*((u16 *) pElement->data + 1) >> 7) & 0x1F;
			hw_priv->is_BT_Present = true;
			xradio_dbg(XRADIO_DBG_NIY,
				   "PTA element found.Listen Interval %d\n",
				   hw_priv->conf_listen_interval);
			break;
		case SDD_XTAL_TRIM_ELT_ID:
#if (CONFIG_HIF_BY_PASS)
			hw_priv->sdd_freq_offset = *pElement->data;
#else
			*trim = *pElement->data;
#endif
			break;
		case SDD_REFERENCE_FREQUENCY_ELT_ID:
			switch (*((uint16_t *) pElement->data)) {
			case 0x32C8:
				*dpll = 0x1D89D241;
				break;
			case 0x3E80:
				*dpll = 0x1E1;
				break;
			case 0x41A0:
				*dpll = 0x124931C1;
				break;
			case 0x4B00:
				*dpll = 0x191;
				break;
			case 0x5DC0:
				if (XR819_HW_REV1 == hw_priv->hw_revision)
					*dpll = 0xe0000281;
				else
					*dpll = 0x141;
				break;
			case 0x6590:
				if (XR819_HW_REV1 == hw_priv->hw_revision)
					*dpll = 0x9D89D241;
				else
					*dpll = 0x0EC4F121;
				break;
			case 0x7D00:
				*dpll = 0xE00001E1;
				break;
			case 0x8340:
				*dpll = 0x92490E1;
				break;
			case 0x9600:
				*dpll = 0x100010C1;
				break;
			case 0x9C40:
				if (XR819_HW_REV1 == hw_priv->hw_revision)
					*dpll = 0xe0000181;
				else
					*dpll = 0xC1;
				break;
			case 0xBB80:
				*dpll = 0xA1;
				break;
			case 0xCB20:
				*dpll = 0x7627091;
				break;
			default:
				*dpll = DPLL_INIT_VAL_XRADIO;
				xradio_dbg(XRADIO_DBG_WARN,
					   "Unknown Reference clock frequency."
					   "Use default DPLL value=0x%08x.",
					    DPLL_INIT_VAL_XRADIO);
				break;
			}
			xradio_dbg(XRADIO_DBG_NIY,
					"Reference clock=%uKHz, DPLL value=0x%08x.\n",
					*((uint16_t *) pElement->data), *dpll);
		default:
			break;
		}
		parsedLength += (FIELD_OFFSET(struct xradio_sdd, data) + \
				 pElement->length);
		pElement = FIND_NEXT_ELT(pElement);
	}

	xradio_dbg(XRADIO_DBG_MSG, "sdd size=%zu parse len=%d.\n",
		   hw_priv->sdd->size, parsedLength);


	if (hw_priv->is_BT_Present == false) {
		hw_priv->conf_listen_interval = 0;
		xradio_dbg(XRADIO_DBG_NIY, "PTA element NOT found.\n");
	}
	return ret;
}

#ifdef XR819S_WAKEUP_FIX
static void xradio_release_sdd(struct xradio_common *hw_priv)
{
	release_firmware(hw_priv->sdd);
	hw_priv->sdd = NULL;
}

static int xradio_dpll_pre_init(struct xradio_common *hw_priv, u32 *dpll)
{
	int ret = 0;
	u8 trim = 0;

	hw_priv->hw_revision = XR819_HW_REV1;
	/*load sdd file, and get config from it.*/
	ret = xradio_parse_sdd(hw_priv, dpll, &trim);
	if (ret < 0) {
		return ret;
	}

	/*set dpll initial value and check.*/
	ret = xradio_reg_write_32(hw_priv, HIF_TSET_GEN_R_W_REG_ID, *dpll);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't write DPLL register.\n",
			    __func__);
		xradio_release_sdd(hw_priv);
		return ret;
	}

	xradio_release_sdd(hw_priv);
	/* Wait for BT to wakeup WLAN and configure dpll of WLAN */
	msleep(50);
	return ret;
}
#endif /* XR819S_WAKEUP_FIX */

static int xradio_firmware(struct xradio_common *hw_priv)
{
	int ret, block, num_blocks;
	unsigned i;
	u32 val32;
	u32 put = 0, get = 0;
	u8 *buf = NULL;
	u32 DOWNLOAD_FIFO_SIZE;
	u32 DOWNLOAD_CTRL_OFFSET;

	const char *fw_path;
#ifdef USE_VFS_FIRMWARE
	const struct xr_file  *firmware = NULL;
#else
	const struct firmware *firmware = NULL;
#endif
	xradio_dbg(XRADIO_DBG_TRC, "%s\n", __func__);

	switch (hw_priv->hw_revision) {
	case XR819_HW_REV0:
#if defined(CONFIG_DRIVER_V821)
		DOWNLOAD_FIFO_SIZE   = V821_DOWNLOAD_FIFO_SIZE;
		DOWNLOAD_CTRL_OFFSET = V821_DOWNLOAD_CTRL_OFFSET;
#else
		DOWNLOAD_FIFO_SIZE   = XR819_DOWNLOAD_FIFO_SIZE;
		DOWNLOAD_CTRL_OFFSET = XR819_DOWNLOAD_CTRL_OFFSET;
#endif
		fw_path = XR819_FIRMWARE;
#ifdef CONFIG_XRADIO_ETF
		if (etf_is_connect()) {
			const char *etf_fw = etf_get_fwpath();
			if (etf_fw != NULL)
				fw_path = etf_fw;
			else {
				fw_path = XR819_ETF_FIRMWARE;
				etf_set_fwpath(fw_path);
			}
		}
#endif
		break;
	case XR819_HW_REV1:
		DOWNLOAD_FIFO_SIZE   = XR819S_DOWNLOAD_FIFO_SIZE;
		DOWNLOAD_CTRL_OFFSET = XR819S_DOWNLOAD_CTRL_OFFSET;
		fw_path = XR819S_FIRMWARE;
#ifdef CONFIG_XRADIO_ETF
		if (etf_is_connect()) {
			const char *etf_fw = etf_get_fwpath();
			if (etf_fw != NULL)
				fw_path = etf_fw;
			else {
				fw_path = XR819S_ETF_FIRMWARE;
				etf_set_fwpath(fw_path);
			}
		}
#endif
		break;
	default:
		xradio_dbg(XRADIO_DBG_ERROR, "%s: invalid silicon revision %d.\n",
			   __func__, hw_priv->hw_revision);
		return -EINVAL;
	}

#if (!CONFIG_HIF_BY_PASS)
	/*
	 * DOWNLOAD_BLOCK_SIZE must be divided exactly by sdio blocksize,
	 * otherwise it may cause bootloader error.
	 */
	val32 = hw_priv->sbus_ops->get_block_size(hw_priv->sbus_priv);
	if (val32 > DOWNLOAD_BLOCK_SIZE || DOWNLOAD_BLOCK_SIZE%val32) {
		xradio_dbg(XRADIO_DBG_WARN,
			"%s:change blocksize(%d->%d) during download fw.\n",
			__func__, val32, DOWNLOAD_BLOCK_SIZE>>1);
		hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
		ret = hw_priv->sbus_ops->set_block_size(hw_priv->sbus_priv,
			DOWNLOAD_BLOCK_SIZE>>1);
		if (ret)
			xradio_dbg(XRADIO_DBG_ERROR,
				"%s: set blocksize error(%d).\n", __func__, ret);
		hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	}
#endif
	/* Initialize common registers */
	APB_WRITE(DOWNLOAD_PUT_REG, 0);
	APB_WRITE(DOWNLOAD_GET_REG, 0);
	APB_WRITE(DOWNLOAD_STATUS_REG, DOWNLOAD_PENDING);
	APB_WRITE(DOWNLOAD_FLAGS_REG, 0);
	APB_WRITE(DOWNLOAD_IMAGE_SIZE_REG, DOWNLOAD_ARE_YOU_HERE);
#if ((defined CONFIG_LOAD_FW_SIZE_NOT_CARE) && (CONFIG_LOAD_FW_SIZE_NOT_CARE == 1))
	APB_WRITE(DOWNLOAD_FLAGS_REG, DOWNLOAD_F_LOAD_SIZE_NOT_CARE);
#endif

#if (CONFIG_HIF_BY_PASS)
	REG_CLR_BIT(HIF_OV_CTRL, OV_RESET_CPU);
	REG_CLR_BIT(HIF_OV_CTRL, OV_DISABLE_CPU_CLK);
	memset((void *)HIF_BYPASS_CTRL_ADDR, 0, sizeof(HIF_CTRL));
#else /*not HIF bypass*/
	/* Release CPU from RESET */
	REG_READ(HIF_CONFIG_REG_ID, val32);
	val32 &= ~HIF_CONFIG_CPU_RESET_BIT;
	REG_WRITE(HIF_CONFIG_REG_ID, val32);

	/* Enable Clock */
	val32 &= ~HIF_CONFIG_CPU_CLK_DIS_BIT;
	REG_WRITE(HIF_CONFIG_REG_ID, val32);
#endif

	/* Load a firmware file */
#ifdef USE_VFS_FIRMWARE
	firmware = xr_fileopen(fw_path, O_RDONLY, 0);
	if (!firmware) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't load firmware file %s.\n",
			   __func__, fw_path);
		ret = -1;
		goto error;
	}
#else
	ret = request_firmware(&firmware, fw_path, hw_priv->pdev);
	if (ret) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't load firmware file %s.\n",
			   __func__, fw_path);
		goto error;
	}
	SYS_BUG(!firmware->data);
#endif

	buf = xr_kmalloc(DOWNLOAD_BLOCK_SIZE, true);
	if (!buf) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: can't allocate firmware buffer.\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	/* Check if the bootloader is ready */
	for (i = 0; i < 100; i++ /*= 1 + i / 2*/) {
		APB_READ(DOWNLOAD_IMAGE_SIZE_REG, val32);
		if (val32 == DOWNLOAD_I_AM_HERE)
			break;
		mdelay(50);
	}			/* End of for loop */
	if (val32 != DOWNLOAD_I_AM_HERE) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: bootloader is not ready(0x%08x).\n",
			   __func__, val32);
		ret = -ETIMEDOUT;
		goto error;
	}

	/* Calculcate number of download blocks */
	num_blocks = (firmware->size - 1) / DOWNLOAD_BLOCK_SIZE + 1;

	/* Updating the length in Download Ctrl Area */
	val32 = firmware->size; /* Explicit cast from size_t to u32 */
	APB_WRITE(DOWNLOAD_IMAGE_SIZE_REG, val32);

	/* Firmware downloading loop */
	for (block = 0; block < num_blocks; block++) {
		size_t tx_size;
		size_t block_size;

		/* check the download status */
		APB_READ(DOWNLOAD_STATUS_REG, val32);
#if ((defined CONFIG_LOAD_FW_SIZE_NOT_CARE) && (CONFIG_LOAD_FW_SIZE_NOT_CARE == 1))
		if (val32 == DOWNLOAD_SUCCESS) {
			xradio_dbg(XRADIO_DBG_ALWY, "%s: bootloader report load sucess.\n", __func__);
			goto out_success;
		}
#endif

		if (val32 != DOWNLOAD_PENDING) {
			xradio_dbg(XRADIO_DBG_ERROR,
				   "%s: bootloader reported error %d.\n",
				   __func__, val32);
			ret = -EIO;
			goto error;
		}

		/* calculate the block size */
		tx_size = block_size = min((size_t)(firmware->size - put),
					   (size_t)DOWNLOAD_BLOCK_SIZE);
#ifdef USE_VFS_FIRMWARE
		ret = xr_fileread(firmware, buf, block_size);
		if (ret < block_size) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: xr_fileread error %d.\n",
				   __func__, ret);
			goto error;
		}
#else
		memcpy(buf, &firmware->data[put], block_size);
#endif
		if (block_size < DOWNLOAD_BLOCK_SIZE) {
			memset(&buf[block_size], 0, DOWNLOAD_BLOCK_SIZE - block_size);
			tx_size = DOWNLOAD_BLOCK_SIZE;
		}

		/* loop until put - get <= 24K */
		for (i = 0; i < 100; i++) {
			APB_READ(DOWNLOAD_GET_REG, get);
			if ((put - get) <= (DOWNLOAD_FIFO_SIZE - DOWNLOAD_BLOCK_SIZE))
				break;
			mdelay(i);
		}

		if ((put - get) > (DOWNLOAD_FIFO_SIZE - DOWNLOAD_BLOCK_SIZE)) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: Timeout waiting for FIFO.\n",
				   __func__);
			ret = -ETIMEDOUT;
			goto error;
		}
#if (CONFIG_HIF_BY_PASS)
		block_size = min((size_t)(firmware->size - put), (size_t)DOWNLOAD_BLOCK_SIZE);
#endif
		/* send the block to sram */
#ifdef CONFIG_ACCESS_WIFI_DIRECT
		memcpy((void *)SILICON_APB_ADDR(DOWNLOAD_FIFO_OFFSET + (put & (DOWNLOAD_FIFO_SIZE - 1))),
		       buf, block_size);
#else
		ret = xradio_apb_write(hw_priv, APB_ADDR(DOWNLOAD_FIFO_OFFSET + \
				       (put & (DOWNLOAD_FIFO_SIZE - 1))),
				       buf, tx_size);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR,
				   "%s: can't write block at line %d.\n",
				   __func__, __LINE__);
			goto error;
		}
#endif

		/* update the put register */
		put += block_size;
		APB_WRITE(DOWNLOAD_PUT_REG, put);
	} /* End of firmware download loop */

	/* Wait for the download completion */
	for (i = 0; i < 300; i += 1 + i / 2) {
		APB_READ(DOWNLOAD_STATUS_REG, val32);
		if (val32 != DOWNLOAD_PENDING)
			break;
		mdelay(i);
	}
	if (val32 != DOWNLOAD_SUCCESS) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: wait for download completion failed. " \
			   "Read: 0x%.8X\n", __func__, val32);
		ret = -ETIMEDOUT;
		goto error;
	} else {
		xradio_dbg(XRADIO_DBG_ALWY, "Firmware completed.\n");
		ret = 0;
	}

#if ((defined CONFIG_LOAD_FW_SIZE_NOT_CARE) && (CONFIG_LOAD_FW_SIZE_NOT_CARE == 1))
out_success:
	if (val32 != DOWNLOAD_SUCCESS) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: wait for download completion failed. Read: 0x%.8X\n", __func__, val32);
		ret = -ETIMEDOUT;
		goto error;
	} else {
		xradio_dbg(XRADIO_DBG_NIY, "Firmware download completed.\n");
		ret = 0;
	}
#endif
error:
	if (buf)
		kfree(buf);
	if (firmware) {
#ifdef USE_VFS_FIRMWARE
		xr_fileclose(firmware);
#else
		release_firmware(firmware);
#endif
	}
	return ret;
}

static int xradio_bootloader(struct xradio_common *hw_priv)
{
	int ret = -1;
	u32 i = 0;
	const char *bl_path;
#ifdef USE_ADDRESS_REMAP_IN_LINUX
	u32 addr = (u32)AHB_SHARE_RAM_ADDR_BASE;
#else
	u32 addr = AHB_MEMORY_ADDRESS;
#endif
	u32 *data = NULL;
#ifdef USE_VFS_FIRMWARE
	const struct xr_file *bootloader = NULL;
#else
	const struct firmware *bootloader = NULL;
#endif
	xradio_dbg(XRADIO_DBG_TRC, "%s\n", __func__);

	if (hw_priv->hw_revision == XR819_HW_REV0)
		bl_path = XR819_BOOTLOADER;
	else
		bl_path = XR819S_BOOTLOADER;

#ifdef USE_VFS_FIRMWARE
	bootloader = xr_request_file(bl_path);
	if (!bootloader) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: can't load bootloader file %s.\n",
			   __func__, bl_path);
		goto error;
	}
#else
	/* Load a bootloader file */
	ret = request_firmware(&bootloader, bl_path, hw_priv->pdev);
	if (ret) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: can't load bootloader file %s.\n",
			   __func__, bl_path);
		goto error;
	}
#endif

	xradio_dbg(XRADIO_DBG_NIY, "%s: bootloader size = %zu, loopcount = %zu\n",
		   __func__, bootloader->size, (bootloader->size) / 4);

	/* Down bootloader. */
	data = (u32 *) bootloader->data;
	for (i = 0; i < (bootloader->size) / 4; i++) {
#ifdef CONFIG_ACCESS_WIFI_DIRECT
		REG_WRITE_U32(addr, data[i]);
#else
		REG_WRITE(HIF_SRAM_BASE_ADDR_REG_ID, addr);
		REG_WRITE(HIF_AHB_DPORT_REG_ID, data[i]);
#endif
		if (i == 100 || i == 200 || i == 300 ||
		   i == 400 || i == 500 || i == 600) {
			xradio_dbg(XRADIO_DBG_NIY,
				   "%s: addr = 0x%x, data = 0x%08x\n",
				   __func__, addr, data[i]);
		}
		addr += 4;
	}
	xradio_dbg(XRADIO_DBG_ALWY, "Bootloader complete\n");

error:
	if (bootloader) {
#ifdef USE_VFS_FIRMWARE
		xr_fileclose(bootloader);
#else
		release_firmware(bootloader);
#endif
	}
	return ret;
}

int xradio_load_firmware(struct xradio_common *hw_priv)
{
	int ret = 0;
	int i;
	u32 val32 = 0;
	u16 val16 = 0;
	u32 dpll = 0;
	u8 trim = 0;

#if (CONFIG_HIF_BY_PASS)
	/*set hw type and verion*/
	hw_priv->hw_type = HIF_HW_TYPE_XRADIO;
	hw_priv->hw_revision = XR819_HW_REV0;

#if !defined(CONFIG_CHIP_FPGA_AW1882) || (defined(CONFIG_CHIP_FPGA_AW1882) && !defined(CONFIG_BITFILE_VERSION_V2))
	/* Set wakeup bit in device and Wait for wakeup*/

	REG_SET_BIT(HIF_OV_CTRL, OV_WLAN_WAKE_UP);

	ret = REG_GET_BIT(HIF_OV_CTRL, OV_WLAN_WAKE_UP);

	/* Wait for wakeup */
	for (i = 0 ; i < 300 ; i += 1 + i / 2) {
		ret = REG_GET_BIT(HIF_WLAN_STATE, WLAN_STATE_ACTIVE);
		if (ret) {
			break;
		}
		msleep(i);
	}

	if (!ret) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: Wait for wakeup:"
			   "device is not responding %d.\n", __func__, ret);
		ret = -ETIMEDOUT;
		goto out;
	}

	xradio_dbg(XRADIO_DBG_ALWY, "WLAN device is ready.\n"); /* WLAN device is ready. */

#elif (defined(CONFIG_CHIP_FPGA_AW1882) && defined(CONFIG_BITFILE_VERSION_V2) && defined(CONFIG_BITFILE_VERSION_R8))
	/* Set wakeup bit in device and Wait for wakeup*/
	REG_SET_BIT(HIF_OV_CTRL, OV_WLAN_WAKE_UP);
#endif
	/* note:
	 * In R128 platform, dpll,	DXCO freq trim, wlan clock src(rfip0_dpll or rfip1_dpll)
	 * all are config by SDK system init, such as in sys_clk_init(),
	 * AS HOSC type is 40MHz, the dpll1/2 reg(0x4004c48c/0x4004c490) value
	 * both should be 0xe0000301
	 * In V821, trim is not used here too.
	 */
	ret = xradio_parse_sdd(hw_priv, &dpll, &trim);
	if (ret)
		xradio_dbg(XRADIO_DBG_WARN, "%s:xradio_parse_sdd err=%d.\n", __func__, ret);
#else /* HIF bypass */
	int major_revision;

#ifdef XR819S_WAKEUP_FIX
	int wakeup_retry = 0;
#endif

	xradio_dbg(XRADIO_DBG_TRC, "%s\n", __func__);
	SYS_BUG(!hw_priv);

#ifdef XR819S_WAKEUP_FIX
	ret = xradio_dpll_pre_init(hw_priv, &dpll);
	if (ret) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s error!\n", __func__);
		return ret;
	}
#endif /* XR819S_WAKEUP_FIX */

	/* Read CONFIG Register Value - We will read 32 bits */
	ret = xradio_reg_read_32(hw_priv, HIF_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: can't read config register, err=%d.\n",
			   __func__, ret);
		return ret;
	}
	/*check hardware type and revision.*/
	hw_priv->hw_type = xradio_get_hw_type(val32, &major_revision);
	switch (hw_priv->hw_type) {
	case HIF_HW_TYPE_XRADIO:
		val32 |= HIF_CONFIG_DPLL_EN_BIT;
		xradio_reg_write_32(hw_priv, HIF_CONFIG_REG_ID, val32);
		xradio_reg_read_32(hw_priv, HIF_CONFIG_REG_ID, &val32);
		if (val32 & HIF_CONFIG_DPLL_EN_BIT)
			major_revision = 5;
		xradio_dbg(XRADIO_DBG_NIY, "%s: HW_TYPE_XRADIO Ver%d detected.\n",
			   __func__, major_revision);
		break;
	default:
		xradio_dbg(XRADIO_DBG_ERROR, "%s: Unknown hardware: %d.\n",
			   __func__, hw_priv->hw_type);
		return -ENOTSUPP;
	}
	if (major_revision == 4) {
		hw_priv->hw_revision = XR819_HW_REV0;
		xradio_dbg(XRADIO_DBG_ALWY, "XRADIO_HW_REV 1.0 detected.\n");
	} else if (major_revision == 5) {
		hw_priv->hw_revision = XR819_HW_REV1;
		xradio_dbg(XRADIO_DBG_ALWY, "XRADIO_HW_REV 1.1 detected.\n");
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: Unsupported major revision %d.\n",
			   __func__, major_revision);
		return -ENOTSUPP;
	}

	/*load sdd file, and get config from it.*/
	ret = xradio_parse_sdd(hw_priv, &dpll, &trim);
	if (ret < 0) {
		return ret;
	}

	/*set dpll initial value and check.*/
	ret = xradio_reg_write_32(hw_priv, HIF_TSET_GEN_R_W_REG_ID, dpll);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't write DPLL register.\n",
			   __func__);
		goto out;
	}
	msleep(5);
	ret = xradio_reg_read_32(hw_priv, HIF_TSET_GEN_R_W_REG_ID, &val32);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't read DPLL register.\n",
			   __func__);
		goto out;
	}
	if (val32 != dpll) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: unable to initialise " \
			   "DPLL register. Wrote 0x%.8X, read 0x%.8X.\n",
			   __func__, dpll, val32);
		ret = -EIO;
		goto out;
	}

#ifdef XR819S_WAKEUP_FIX
wakeup:
#endif
	/* Set wakeup bit in device */
	ret = xradio_reg_read_16(hw_priv, HIF_CONTROL_REG_ID, &val16);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: set_wakeup: can't read control register.\n",
			   __func__);
		goto out;
	}
	ret = xradio_reg_write_16(hw_priv, HIF_CONTROL_REG_ID,
				  val16 | HIF_CTRL_WUP_BIT);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: set_wakeup: can't write control register.\n",
			   __func__);
		goto out;
	}

	/* Wait for wakeup */
	for (i = 0 ; i < 300 ; i += 1 + i / 2) {
		ret = xradio_reg_read_16(hw_priv, HIF_CONTROL_REG_ID, &val16);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: Wait_for_wakeup: "
				   "can't read control register.\n", __func__);
			goto out;
		}
		if (val16 & HIF_CTRL_RDY_BIT) {
			break;
		}
		msleep(i);
	}
	if ((val16 & HIF_CTRL_RDY_BIT) == 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: Wait for wakeup:"
			   "device is not responding.\n", __func__);
		ret = -ETIMEDOUT;
#ifdef XR819S_WAKEUP_FIX
		if (wakeup_retry < 5) {
			wakeup_retry++;
			xradio_dbg(XRADIO_DBG_ALWY, "wakup retry: %d.\n", wakeup_retry);

			if (wakeup_retry <= 3)
				msleep(wakeup_retry * 10);
			else
				msleep(wakeup_retry * 100);

			goto wakeup;
		}
#endif /* XR819S_WAKEUP_FIX */
		goto out;
	} else {
		xradio_dbg(XRADIO_DBG_NIY, "WLAN device is ready.\n");
	}

	/*set trim initial value and check.*/
	xradio_dbg(XRADIO_DBG_ALWY, "Get DCXO Trimm from SDD: 0x%x\n", trim);
	ret = xradio_apb_write_32(hw_priv, DCXO_TRIM_ADDR, DCXO_TRIM_KEY | trim);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s:APB write Trim error!\n", __func__);
		goto out;
	}
	ret = xradio_apb_read_32(hw_priv, DCXO_TRIM_ADDR, &val32);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s:APB read Trim error!\n", __func__);
		goto out;
	}
	if (val32 != (DCXO_TRIM_KEY | trim)) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: unable to initialise " \
			"Trim wrote 0x%.8X, read 0x%.8X.\n",
			__func__, dpll, val32);
		ret = -EIO;
		goto out;
	}
	/* Checking for access mode and download firmware. */
	ret = xradio_reg_read_32(hw_priv, HIF_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: check_access_mode: "
			   "can't read config register.\n", __func__);
		goto out;
	}
	if (val32 & HIF_CONFIG_ACCESS_MODE_BIT) {
#endif	/* CONFIG_HIF_BY_PASS */
		/* Down bootloader. */
		ret = xradio_bootloader(hw_priv);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR,
				   "%s: can't download bootloader.\n", __func__);
			goto out;
		}
		/* Down firmware. */
		ret = xradio_firmware(hw_priv);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR,
				   "%s: can't download firmware.\n", __func__);
			goto out;
		}
#if (!CONFIG_HIF_BY_PASS)
	} else {
		xradio_dbg(XRADIO_DBG_WARN, "%s: check_access_mode: "
			   "device is already in QUEUE mode.\n", __func__);
		/* TODO: verify this branch. Do we need something to do? */
	}

	/* Register Interrupt Handler */
	ret = hw_priv->sbus_ops->irq_subscribe(hw_priv->sbus_priv,
					      (sbus_irq_handler)xradio_irq_handler,
					       hw_priv);
#else
	ret = hifbypass_irq_subscribe(hw_priv->hifbypass_priv,
							(hifbypass_irq_handler)xradio_irq_handler,
							hw_priv); //hifbypass rx irq subscribe
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: can't register IRQ handler.\n",
			   __func__);
		goto out;
	}
#endif
#if (CONFIG_HIF_BY_PASS)
	ret = xradio_hif_bypass_init();
#else
	if (HIF_HW_TYPE_XRADIO == hw_priv->hw_type) {
		/* If device is XRADIO the IRQ enable/disable bits
		 * are in CONFIG register */
		ret = xradio_reg_read_32(hw_priv, HIF_CONFIG_REG_ID, &val32);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: enable_irq: can't read " \
				   "config register.\n", __func__);
			goto unsubscribe;
		}
		ret = xradio_reg_write_32(hw_priv, HIF_CONFIG_REG_ID,
			val32 | HIF_CONF_IRQ_RDY_ENABLE);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: enable_irq: can't write " \
				   "config register.\n", __func__);
			goto unsubscribe;
		}
	} else {
		/* If device is XRADIO the IRQ enable/disable bits
		 * are in CONTROL register */
		/* Enable device interrupts - Both DATA_RDY and WLAN_RDY */
		ret = xradio_reg_read_16(hw_priv, HIF_CONFIG_REG_ID, &val16);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: enable_irq: can't read " \
				   "control register.\n", __func__);
			goto unsubscribe;
		}
		ret = xradio_reg_write_16(hw_priv, HIF_CONFIG_REG_ID,
					  val16 | HIF_CTRL_IRQ_RDY_ENABLE);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "%s: enable_irq: can't write " \
				   "control register.\n", __func__);
			goto unsubscribe;
		}

	}

	/* Configure device for MESSSAGE MODE */
	ret = xradio_reg_read_32(hw_priv, HIF_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: set_mode: can't read config register.\n",
			   __func__);
		goto unsubscribe;
	}
	ret = xradio_reg_write_32(hw_priv, HIF_CONFIG_REG_ID,
				  val32 & ~HIF_CONFIG_ACCESS_MODE_BIT);
	if (ret < 0) {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: set_mode: can't write config register.\n",
			       __func__);
		goto unsubscribe;
	}

	/* Unless we read the CONFIG Register we are
	 * not able to get an interrupt */
	mdelay(10);
	xradio_reg_read_32(hw_priv, HIF_CONFIG_REG_ID, &val32);
#endif //CONFIG_HIF_BY_PASS

	return 0;

unsubscribe:
#if CONFIG_HIF_BY_PASS
	hifbypass_irq_unsubscribe(hw_priv->hifbypass_priv);
#else
	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
#endif
out:
	if (hw_priv->sdd) {
#ifdef USE_VFS_FIRMWARE
		xr_fileclose(hw_priv->sdd);
#else
		release_firmware(hw_priv->sdd);
#endif
		hw_priv->sdd = NULL;
	}
	return ret;
}

int xradio_dev_deinit(struct xradio_common *hw_priv)
{
#if (CONFIG_HIF_BY_PASS)
	hifbypass_irq_unsubscribe(hw_priv->hifbypass_priv);
#else
	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
#endif
	if (hw_priv->sdd) {
#ifdef USE_VFS_FIRMWARE
		xr_fileclose(hw_priv->sdd);
#else
		release_firmware(hw_priv->sdd);
#endif
		hw_priv->sdd = NULL;
	}
	return 0;
}

#undef APB_WRITE
#undef APB_READ
#undef REG_WRITE
#undef REG_READ
