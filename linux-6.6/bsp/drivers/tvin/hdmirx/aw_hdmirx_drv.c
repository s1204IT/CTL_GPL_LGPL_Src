/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/dma-buf.h>
#include <linux/kthread.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/notifier.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/interrupt.h>
#include <linux/irq.h>


#if IS_ENABLED(CONFIG_EXTCON)
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#endif

#include "sunxi-log.h"
#include "aw_hdmirx_drv.h"
#include "aw_hdmirx_define.h"

struct aw_hdmirx_driver *g_hdmirx_drv;

u32 gHdmirx_log_level;

bool bHdmirx_clk_enable;
bool bHdmirx_suspend_enable;
U8 bhdmirx_source_enable;
bool bHdmirx_cec_enable;
bool bHdmirx_cec_suspend_enable;

//u8 debug_log_current_port_id;

/**
 * @short List of the devices
 * Linked list that contains the installed devices
 */
static LIST_HEAD(devlist_global);

#if IS_ENABLED(CONFIG_EXTCON)
static const unsigned int aw_hdmirx_cable[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};
static struct extcon_dev *gHdmirx_extcon;
#endif

bool aw_hdmirx_drv_ishdmisource(TSourceId source_id)
{
	if (source_id > kSourceId_Image && source_id < kSourceId_HDMI_1 + HDMI_PORT_NUM) {
		return true;
	}
	return false;
}

int aw_hdmirx_drv_isvalidsource(TSourceId source_id)
{
	if (source_id > kSourceId_Dummy && source_id < kSourceId_Max) {
		return 0;
	}
	hdmirx_err("source_id is not valid: %d\n", source_id);
	return -1;
}

void aw_hdmirx_drv_setsource(TSourceId source_id)
{
	if (bhdmirx_source_enable) {
		aw_core_Disable();
		bhdmirx_source_enable = false;
	}

	if (!bhdmirx_source_enable && aw_hdmirx_drv_ishdmisource(source_id)) {
		aw_core_Enable(source_id);
		aw_core_AfterEnable(source_id);
		bhdmirx_source_enable = true;
	}
}

void aw_hdmirx_drv_sethpdtimeinterval(U32 mHpd_interval_ms)
{
	aw_core_SetHPDTimeInterval(mHpd_interval_ms);
}

void aw_hdmirx_drv_setportmap(TSourceId source_id, U8 soc_port_id)
{
	aw_core_SetPortMap(source_id, soc_port_id);
}

void aw_hdmirx_drv_setupdateedid(U8 *edid_table, U8 edid_version)
{
	aw_core_SetUpdateEdid(edid_table, edid_version);
}

void aw_hdmirx_drv_setarcenable(bool bOn)
{
	aw_core_SetARCEnable(bOn);
}

void aw_hdmirx_drv_setarctxpath(ARCTXPath_e arc_tx_path)
{
	aw_core_SetARCTXPath(arc_tx_path);
}

void aw_hdmirx_drv_setedidversion(U8 setting_version)
{
	aw_core_SetEdidVersion(setting_version);
}

void aw_hdmirx_drv_setpullhotplug(U8 port_id, U8 type)
{
	aw_core_SetPullHotplug(port_id, type);
}

void aw_hdmirx_drv_get5vstate(U32 *pb5vstate)
{
	aw_core_Get5vState(pb5vstate);
}

void aw_hdmirx_drv_write(uintptr_t addr, u32 data)
{
	writeb((u8)data, (volatile void __iomem *)(
		g_hdmirx_drv->reg_base_vs + addr));
}

u32 aw_hdmirx_drv_read(uintptr_t addr)
{
	return (u32)readb((volatile void __iomem *)(
		g_hdmirx_drv->reg_base_vs + addr));
}

ssize_t aw_hdmirx_drv_edid_parse(char *buf)
{
	return aw_core_edid_parse(buf);
}

ssize_t aw_hdmirx_drv_dump_SignalInfo(char *buf)
{
	return aw_core_dump_SignalInfo(buf);
}

ssize_t aw_hdmirx_drv_dump_status(char *buf)
{
	return aw_core_dump_status(buf);
}

ssize_t aw_hdmirx_drv_dump_hdcp14(char *buf)
{
	return aw_core_dump_hdcp14(buf);
}

static int aw_hdmirx_dts_parse_clk(struct platform_device *pdev)
{
	/* get vs master control clock */
	g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl = devm_clk_get(&pdev->dev, "clk_hdmi_rx_dtl");
	if (IS_ERR(g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl)) {
		hdmirx_err("%s: fail to get clk for master control\n", __func__);
		g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl = NULL;
	}

	/* get clock of tcon_tv supply data to hrc */
	g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb = devm_clk_get(&pdev->dev, "clk_hdmi_rx_cb");
	if (IS_ERR(g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb)) {
		hdmirx_err("%s: fail to get clk for colorbar\n", __func__);
		g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb = NULL;
	}

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	hdmirx_inf("cec");
	g_hdmirx_drv->aw_clock.clk_hdmi_cec_clk32k = devm_clk_get(&pdev->dev, "clk_hdmi_cec_clk32k");
	if (IS_ERR(g_hdmirx_drv->aw_clock.clk_hdmi_cec_clk32k)) {
		hdmirx_err("%s: fail to get clk for hdmirx cec clk32k\n", __func__);
		return -1;
	}

	g_hdmirx_drv->aw_clock.clk_cec_peri_gate = devm_clk_get(&pdev->dev, "clk_cec_peri_gate");
	if (IS_ERR(g_hdmirx_drv->aw_clock.clk_cec_peri_gate)) {
		hdmirx_err("%s: fail to get clk for hdmirx cec peri gate\n", __func__);
		return -1;
	}

	g_hdmirx_drv->aw_clock.clk_hdmi_cec = devm_clk_get(&pdev->dev, "clk_hdmi_cec");
	if (IS_ERR(g_hdmirx_drv->aw_clock.clk_hdmi_cec)) {
		hdmirx_err("%s: fail to get clk for hdmirx cec hdmi cec\n", __func__);
		return -1;
	}
#endif

	/* get clock for busgating reset */
	g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx = devm_clk_get(&pdev->dev, "clk_bus_hdmi_rx");
	if (IS_ERR(g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx)) {
		hdmirx_err("%s: fail to get clk busgating reset\n", __func__);
		g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx = NULL;
	}

#ifdef rst_bus_hdmi
	g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx = devm_reset_control_get(&pdev->dev, "rst_bus_hdmi_rx");
	if (IS_ERR(g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx)) {
		hdmirx_err("%s: fail to get rst_bus_hdmi_rx\n", __func__);
		return -1;
	}

	if (g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx) {
		if (reset_control_deassert(g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx)) {
			hdmirx_err("%s: deassert rst_bus_hdmi_rx failed!\n", __func__);
			return -1;
		}
	}
#endif

	return 0;
}

static int aw_hdmirx_dts_parse_basic_info(struct platform_device *pdev)
{
	uintptr_t reg_base = 0x0;
	int counter = 0;

	/* iomap hdmi register base address */
	reg_base = (uintptr_t __force)of_iomap(pdev->dev.of_node, counter);
	if (reg_base == 0) {
		hdmirx_err("%s: Unable to map hdmi registers\n", __func__);
		return -EINVAL;
	}
	g_hdmirx_drv->reg_base_vs = reg_base;
	counter++;

	/* iomap edid top register base address */
	reg_base = (uintptr_t __force)of_iomap(pdev->dev.of_node, counter);
	if (reg_base == 0) {
		hdmirx_err("%s: Unable to map edid top registers\n", __func__);
		return -EINVAL;
	}
	g_hdmirx_drv->reg_edid_top = reg_base;
	counter++;

	return 0;
}

int aw_hdmirx_init_resoure(struct platform_device *pdev)
{
	int ret = 0x0;
	ret = aw_hdmirx_dts_parse_basic_info(pdev);
	if (ret != 0) {
		hdmirx_err("%s: aw dts parse basic info failed!!!\n", __func__);
		return -1;
	}

	ret = aw_hdmirx_dts_parse_clk(pdev);
	if (ret != 0) {
		hdmirx_err("%s: aw dts parse clock info failed!!!\n", __func__);
		return -1;
	}

	/* get dts params */
	if (device_property_read_u32(&pdev->dev, "hdmi_num", &g_hdmirx_drv->aw_dts.hdmi_num)) {
		hdmirx_inf("parse hdmi_num from dts\n");
	} else {
		g_hdmirx_drv->aw_dts.hdmi_num = HDMI_PORT1;
	}

	return 0;
}

static void aw_hdmirx_drv_clock_enable(void)
{
	if (bHdmirx_clk_enable) {
		hdmirx_wrn("hdmi clk has been enable!\n");
		return;
	}

#ifdef rst_bus_hdmi
	if (g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx) {
		if (reset_control_deassert(g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx)) {
			hdmirx_err("%s: clock enable to de-assert rst_bus_hdmi_rx is failed!\n", __func__);
			return;
		}
	}
#endif

	if (g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl != NULL) {
		if (clk_prepare_enable(g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl) != 0)
			hdmirx_err("%s: clk_hdmi_rx_dtl enable failed!\n", __func__);

		if (clk_set_rate(g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl, 200000000)) {
			hdmirx_err("Failed to set clk_hdmi_rx_dtl rate\n");
		}
	}

	if (g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb != NULL) {
		if (clk_prepare_enable(g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb) != 0)
			hdmirx_err("%s: clk_hdmi_rx_cb enable failed!\n", __func__);
	}

	if (g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx != NULL) {
		if (clk_prepare_enable(g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx) != 0)
			hdmirx_err("%s: clk_hdmi_rx enable failed!\n", __func__);
	}

	bHdmirx_clk_enable = true;
}

static void aw_hdmirx_drv_clock_disable(void)
{
	hdmirx_inf("%s start!\n", __func__);
	if (g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl != NULL) {
		hdmirx_inf("1111");
		clk_disable_unprepare(g_hdmirx_drv->aw_clock.clk_hdmi_rx_dtl);
	}

	if (g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb != NULL) {
		hdmirx_inf("222");
		clk_disable_unprepare(g_hdmirx_drv->aw_clock.clk_hdmi_rx_cb);
	}

	if (g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx != NULL) {
		hdmirx_inf("333");
		clk_disable_unprepare(g_hdmirx_drv->aw_clock.clk_bus_hdmi_rx);
	}

#ifdef rst_bus_hdmi
	if (g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx) {
		hdmirx_inf("config assert rst_bus_hdmi_rx.\n");
		if (reset_control_assert(g_hdmirx_drv->aw_clock.rst_bus_hdmi_rx)) {
			hdmirx_err("%s: assert rst_bus_hdmi_rx failed!\n", __func__);
			return;
		}
	}
#endif

	hdmirx_inf("%s end!\n", __func__);
}

void aw_hdmirx_drv_hpd_notify(u32 status)
{
#if IS_ENABLED(CONFIG_EXTCON)
	/* hpd mask not-notify */
	extcon_set_state_sync(gHdmirx_extcon, EXTCON_DISP_HDMI,
		(status == 1) ? STATUE_OPEN : STATUE_CLOSE);
#else
	hdmirx_wrn("not config extcon node for hpd notify.\n");
#endif
}

int aw_hdmirx_init_notify_node(void)
{
#if IS_ENABLED(CONFIG_EXTCON)
	/* creat /sys/class/extcon/extcon%d */
	gHdmirx_extcon = devm_extcon_dev_allocate(g_hdmirx_drv->parent_dev, aw_hdmirx_cable);
/* 	gHdmi_extcon->name = "hdmi"; */ /* fix me */
	devm_extcon_dev_register(g_hdmirx_drv->parent_dev, gHdmirx_extcon);

	aw_hdmirx_drv_hpd_notify(false);
	return 0;
#else
	hdmirx_wrn("not init hdmi notify node!\n");
	return 0;
#endif
}

static void aw_hdmirx_drv_resource_release(void)
{
	aw_hdmirx_drv_clock_disable();
}

int aw_hdmirx_init_mutex_lock(void)
{
	if (!g_hdmirx_drv) {
		hdmirx_err("%s: g_hdmi_drv is null!!!\n", __func__);
		return -1;
	}

	mutex_init(&g_hdmirx_drv->aw_mutex.lock_ctrl);
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	mutex_init(&g_hdmirx_drv->aw_mutex.lock_cec);
	mutex_init(&g_hdmirx_drv->aw_mutex.lock_cec_notifier);
#endif
	return 0;
}

void aw_hdmirx_init_global_value(void)
{
	gHdmirx_log_level = 0;

	bHdmirx_clk_enable = false;
	bHdmirx_suspend_enable = false;
	bhdmirx_source_enable = false;

	//debug_log_current_port_id = 1;
}

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
static void _aw_hdmirx_cec_drv_clock_enable(void)
{
	if (g_hdmirx_drv->aw_clock.clk_hdmi_cec_clk32k != NULL) {
		if (clk_prepare_enable(g_hdmirx_drv->aw_clock.clk_hdmi_cec_clk32k) != 0)
			hdmirx_err("%s: hdmi cec clk32k enable failed!\n", __func__);
	}

	if (g_hdmirx_drv->aw_clock.clk_hdmi_cec != NULL) {
		if (clk_prepare_enable(g_hdmirx_drv->aw_clock.clk_hdmi_cec) != 0)
			hdmirx_err("%s: hdmi cec clk gate enable failed!\n", __func__);
	}

	if (g_hdmirx_drv->aw_clock.clk_cec_peri_gate != NULL) {
		if (clk_prepare_enable(g_hdmirx_drv->aw_clock.clk_cec_peri_gate) != 0)
			hdmirx_err("%s: hdmi cec clk peri gate enable failed!\n", __func__);
	}
}

static void _aw_hdmirx_cec_drv_clock_disable(void)
{
	if (g_hdmirx_drv->aw_clock.clk_hdmi_cec != NULL)
		clk_disable_unprepare(g_hdmirx_drv->aw_clock.clk_hdmi_cec);

	if (g_hdmirx_drv->aw_clock.clk_hdmi_cec_clk32k != NULL)
		clk_disable_unprepare(g_hdmirx_drv->aw_clock.clk_hdmi_cec_clk32k);

	if (g_hdmirx_drv->aw_clock.clk_cec_peri_gate != NULL)
		clk_disable_unprepare(g_hdmirx_drv->aw_clock.clk_cec_peri_gate);
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
ssize_t aw_hdmirx_cec_dump_info(char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "\n[cec]\n");
	n += sprintf(buf + n, " - status: %s\n", bHdmirx_cec_enable ? "enable" : "disable");
	n += sprintf(buf + n, " - suspend: %d\n", bHdmirx_cec_suspend_enable);
	n += sprintf(buf + n, " - logical address: 0x%04x\n",
			HdmiRx_Cec_Hardware_get_logical_addr());
	n += sprintf(buf + n, "\n");

	return n;
}

void aw_hdmirx_cec_msg_debug(int tx_rx, unsigned char *msg, int len)
{
	struct cec_msg rx_msg;

	if (tx_rx == 0) {
		aw_core_Cec_Send(msg, len, 0);
	} else {
		memset(&rx_msg, 0, sizeof(struct cec_msg));
		rx_msg.len = len;
		memcpy(rx_msg.msg, msg, len);
		cec_received_msg(g_hdmirx_drv->adap, &rx_msg);
	}
}

int aw_hdmirx_cec_suspend(struct device *dev)
{
	hdmirx_inf("%s start.\n", __func__);
	return 0;
}

int aw_hdmirx_cec_resume(struct device *dev)
{
	hdmirx_inf("%s start.\n", __func__);
	return 0;
}
#endif

static int _aw_hdmirx_cec_enable(struct cec_adapter *adap, bool enable)
{
	CEC_INF("%s start!\n", __func__);

	int ret = 0;

	if (enable) {
		mutex_lock(&g_hdmirx_drv->aw_mutex.lock_cec);
		_aw_hdmirx_cec_drv_clock_enable();
		aw_core_Cec_Enable();
		bHdmirx_cec_enable = true;
		mutex_unlock(&g_hdmirx_drv->aw_mutex.lock_cec);
	} else {
		mutex_lock(&g_hdmirx_drv->aw_mutex.lock_cec);
		aw_core_Cec_Disable();
		_aw_hdmirx_cec_drv_clock_disable();
		bHdmirx_cec_enable = false;
		mutex_unlock(&g_hdmirx_drv->aw_mutex.lock_cec);
	}

	return ret;
}

static int _aw_hdmirx_cec_log_addr(struct cec_adapter *adap, u8 logical_addr)
{
	CEC_INF("%s start!\n", __func__);
	struct aw_hdmirx_cec *cec = cec_get_drvdata(adap);

	if (logical_addr == CEC_LOG_ADDR_INVALID)
		cec->logical_addr = 0;
	else
		cec->logical_addr |= BIT(logical_addr);

	aw_core_Cec_Set_Logical_Addr(cec->logical_addr);
	return 0;
}

static int _aw_hdmirx_cec_transmit(struct cec_adapter *adap, u8 attempts,
				u32 signal_free_time, struct cec_msg *msg)
{
	CEC_INF("%s start!\n", __func__);
	unsigned int frame_type;

	switch (signal_free_time) {
	case CEC_SIGNAL_FREE_TIME_RETRY:
		frame_type = AW_HDMI_CEC_FRAME_TYPE_RETRY;

		/* The reason for failed retry may be that other devices are sending,
		 * so you need to give some time to receive before sending.
		 */
		msleep(50);
		break;
	case CEC_SIGNAL_FREE_TIME_NEW_INITIATOR:
	default:
		frame_type = AW_HDMI_CEC_FRAME_TYPE_NORMAL;
		break;
	case CEC_SIGNAL_FREE_TIME_NEXT_XFER:
		frame_type = AW_HDMI_CEC_FRAME_TYPE_IMMED;
		break;
	}

	aw_core_Cec_Send(msg->msg, msg->len, frame_type);
	return 0;
}

static irqreturn_t _aw_hdmirx_cec_irq_handler(int irq, void *data)
{
	LOG_TRACE();

	struct cec_adapter *adap = data;
	struct aw_hdmirx_cec *cec = cec_get_drvdata(adap);

	irqreturn_t ret = IRQ_HANDLED;
	unsigned int stat = aw_core_Cec_interrupt_get_state();

	aw_core_Cec_interrupt_clear_state(stat);

	if (stat & AW_HDMI_CEC_STAT_ERROR_INIT) {
		cec->tx_status = CEC_TX_STATUS_ERROR;
		cec->tx_done = true;
		ret = IRQ_WAKE_THREAD;
	} else if (stat & AW_HDMI_CEC_STAT_DONE) {
		cec->tx_status = CEC_TX_STATUS_OK;
		cec->tx_done = true;
		ret = IRQ_WAKE_THREAD;
	} else if (stat & AW_HDMI_CEC_STAT_NACK) {
		cec->tx_status = CEC_TX_STATUS_NACK;
		cec->tx_done = true;
		ret = IRQ_WAKE_THREAD;
	}

	if (stat & AW_HDMI_CEC_STAT_EOM) {
		unsigned int len;

		aw_core_Cec_Receive(cec->rx_msg.msg, &len);

		cec->rx_msg.len = len;
		smp_wmb();
		cec->rx_done = true;

		ret = IRQ_WAKE_THREAD;
	}

	return ret;
}

static irqreturn_t _aw_hdmirx_cec_irq_thread(int irq, void *data)
{
	LOG_TRACE();
	struct cec_adapter *adap = data;
	struct aw_hdmirx_cec *cec = cec_get_drvdata(adap);

	if (cec->tx_done) {
		cec->tx_done = false;
		cec_transmit_attempt_done(adap, cec->tx_status);
	}
	if (cec->rx_done) {
		CEC_INF("%s start!\n", __func__);
		cec->rx_done = false;
		smp_rmb();
		cec_received_msg(adap, &cec->rx_msg);
	}
	return IRQ_HANDLED;
}

static const struct cec_adap_ops aw_hdmirx_cec_ops = {
	.adap_enable   = _aw_hdmirx_cec_enable,
	.adap_log_addr = _aw_hdmirx_cec_log_addr,
	.adap_transmit = _aw_hdmirx_cec_transmit,
};

static void _aw_hdmirx_cec_del(void *data)
{
	struct aw_hdmirx_cec *cec = data;
	cec_delete_adapter(cec->adap);
}

static int _aw_hdmirx_cec_init(void)
{
	struct platform_device_info pdevinfo;

	hdmirx_inf("%s start!\n", __func__);

	memset(&pdevinfo, 0, sizeof(pdevinfo));
	pdevinfo.parent = g_hdmirx_drv->parent_dev;
	pdevinfo.id = PLATFORM_DEVID_AUTO;
	pdevinfo.name = "aw-hdmirx-cec";
	pdevinfo.dma_mask = 0;

	g_hdmirx_drv->cec = platform_device_register_full(&pdevinfo);

	hdmirx_inf("%s finish!\n", __func__);
	return 0;
}

static void _aw_hdmirx_cec_exit(void)
{
	hdmirx_inf("%s: start!!!\n", __func__);

	if (g_hdmirx_drv->cec) {
		platform_device_unregister(g_hdmirx_drv->cec);
	}
	hdmirx_inf("%s: end!!!\n", __func__);
}

int aw_hdmirx_cec_probe(struct platform_device *pdev)
{
	struct aw_hdmirx_cec *cec;
	int ret;

	hdmirx_inf("%s: start!!!\n", __func__);

	cec = devm_kzalloc(&pdev->dev, sizeof(*cec), GFP_KERNEL);
	if (!cec) {
		hdmirx_err("%s: cec kzalloc failed!\n", __func__);
		return -ENOMEM;
	}

	cec->dev = &pdev->dev;
	bHdmirx_cec_enable = false;

	platform_set_drvdata(pdev, cec);

	cec->adap = cec_allocate_adapter(&aw_hdmirx_cec_ops, cec, "aw_hdmirx",
					 CEC_CAP_DEFAULTS |
					 CEC_CAP_CONNECTOR_INFO,
					 CEC_MAX_LOG_ADDRS);
	if (IS_ERR(cec->adap)) {
		hdmirx_err("%s: cec_allocate_adapter error!\n", __func__);
		return PTR_ERR(cec->adap);
	}

	cec->adap->owner = THIS_MODULE;

	ret = devm_add_action(&pdev->dev, _aw_hdmirx_cec_del, cec);
	if (ret) {
		hdmirx_err("%s: devm_add_action error! ret: %d\n", __func__, ret);
		cec_delete_adapter(cec->adap);
		return ret;
	}

	g_hdmirx_drv->cec_notifier = cec_notifier_cec_adap_register(pdev->dev.parent,
			NULL, cec->adap);
	if (!g_hdmirx_drv->cec_notifier) {
		hdmirx_err("%s: cec_notifier_cec_adap_register error!\n", __func__);
		return -ENOMEM;
	}

	ret = cec_register_adapter(cec->adap, pdev->dev.parent);
	if (ret < 0) {
		hdmirx_err("%s: cec_register_adapter error! ret: %d\n", __func__, ret);
		cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
		return ret;
	}

	ret = devm_request_threaded_irq(&pdev->dev, g_hdmirx_drv->aw_dts.irq_cec,
					_aw_hdmirx_cec_irq_handler,
					_aw_hdmirx_cec_irq_thread,
					IRQF_SHARED,
					"hdmirx-cec", cec->adap);
	if (ret < 0) {
		hdmirx_err("%s: devm_request_threaded_irq error! ret: %d\n", __func__, ret);
		goto free_mem;
	}

	devm_remove_action(&pdev->dev, _aw_hdmirx_cec_del, cec);

	mutex_lock(&g_hdmirx_drv->aw_mutex.lock_cec_notifier);
	cec_notifier_set_phys_addr(g_hdmirx_drv->cec_notifier, 0x0000);
	mutex_unlock(&g_hdmirx_drv->aw_mutex.lock_cec_notifier);

	g_hdmirx_drv->adap = cec->adap;
	hdmirx_inf("%s finish!\n", __func__);

	return 0;

free_mem:
	devm_remove_action(&pdev->dev, _aw_hdmirx_cec_del, cec);
	cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
	cec_unregister_adapter(cec->adap);
	devm_kfree(&pdev->dev, cec);
	return ret;

}

int aw_hdmirx_cec_remove(struct platform_device *pdev)
{
	struct aw_hdmirx_cec *cec = platform_get_drvdata(pdev);

	mutex_lock(&g_hdmirx_drv->aw_mutex.lock_cec_notifier);
	cec_notifier_phys_addr_invalidate(g_hdmirx_drv->cec_notifier);
	mutex_unlock(&g_hdmirx_drv->aw_mutex.lock_cec_notifier);

	cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
	cec_unregister_adapter(cec->adap);
	devm_kfree(&pdev->dev, cec);

	return 0;
}
#endif

int aw_hdmirx_drv_resume(struct device *dev)
{
	int ret = 0;

	hdmirx_inf("%s: start.\n", __func__);

	if (!bHdmirx_suspend_enable) {
		hdmirx_wrn("hdmi has been resume!\n");
		return 0;
	}

	aw_hdmirx_drv_clock_enable();

	aw_hdmirx_drv_hpd_notify(0x0);

	HdmiRx_Edid_Hardware_set_top_cfg();
	HdmiRx_Edid_Hardware_set_5v_cfg();

	ret = aw_hdmirx_core_init_thread();
	if (ret != 0) {
		hdmirx_err("%s: aw hdmi initial thread failed!!!\n", __func__);
		return -1;
	}

	bHdmirx_suspend_enable = false;
	hdmirx_inf("%s finish.\n", __func__);
	return 0;
}

int aw_hdmirx_drv_suspend(struct device *dev)
{
	hdmirx_inf("%s: start.\n", __func__);

	if (bHdmirx_suspend_enable) {
		hdmirx_wrn("hdmirx has been suspend!\n");
		return 0;
	}

	aw_core_statemachine_thread_exit();
	aw_core_hdcp_thread_exit();
	aw_core_scan_thread_exit();

	aw_hdmirx_drv_resource_release();

	bHdmirx_suspend_enable = true;
	hdmirx_inf("%s finish.\n", __func__);
	return 0;
}

int aw_hdmirx_drv_probe(struct platform_device *pdev)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_LOG_BUFFER)
	aw_hdmirx_log_init(pdev->dev.of_node);
#endif

	hdmirx_inf("%s: start.\n", __func__);

	if (g_hdmirx_drv) {
		kfree(g_hdmirx_drv);
		g_hdmirx_drv = NULL;
	}

	g_hdmirx_drv = kzalloc(sizeof(struct aw_hdmirx_driver), GFP_KERNEL);
	if (!g_hdmirx_drv) {
		hdmirx_err("%s: aw hdmi alloc hdmirx drv memory failed!!!\n", __func__);
		return -1;
	}

	g_hdmirx_drv->hdmirx_core = kzalloc(sizeof(struct aw_hdmirx_core_s), GFP_KERNEL);
	if (!g_hdmirx_drv->hdmirx_core) {
		hdmirx_err("%s: aw hdmi alloc hdmirx core memory failed!!!\n", __func__);
		goto free_mem;
	}
	g_hdmirx_drv->pdev = pdev;
	g_hdmirx_drv->parent_dev = &pdev->dev;

	/* 1. base global value init */
	aw_hdmirx_init_global_value();

	/* 2. base use reosurce init and malloc */
	if (aw_hdmirx_init_resoure(pdev) != 0)
		goto free_mem;

	if (aw_hdmirx_init_mutex_lock() != 0)
		goto free_mem;

	if (aw_hdmirx_init_notify_node() != 0)
		goto free_mem;

	aw_hdmirx_drv_clock_enable();

	/* Init hdmi core and core params */
	g_hdmirx_drv->hdmirx_core->reg_base_vs = g_hdmirx_drv->reg_base_vs;
	g_hdmirx_drv->hdmirx_core->reg_edid_top = g_hdmirx_drv->reg_edid_top;
	hdmirx_inf("g_aw_core->reg_base_vs = %p, g_aw_core->reg_edid_top = %p\n", (void *)g_hdmirx_drv->hdmirx_core->reg_base_vs, (void *)g_hdmirx_drv->hdmirx_core->reg_edid_top);
	ret = aw_hdmirx_core_init(g_hdmirx_drv->hdmirx_core);
	if (ret) {
		hdmirx_err("%s: aw hdmi core init failed!!!\n", __func__);
		goto free_mem;
	}

	/* Now that everything is fine, let's add it to device list */
	list_add_tail(&g_hdmirx_drv->devlist, &devlist_global);

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	aw_hdmirx_core_set_cecbase();
	g_hdmirx_drv->aw_dts.irq_cec = platform_get_irq(pdev, 0);
	if (g_hdmirx_drv->aw_dts.irq_cec < 0) {
		hdmirx_err("%s: platform_get_irq error!\n", __func__);
	} else {
		ret = _aw_hdmirx_cec_init();
		if (ret != 0) {
			hdmirx_err("%s: aw hdmi initial cec failed!!!\n", __func__);
			goto free_mem;
		}
	}
#endif
	return 0;

free_mem:
	kfree(g_hdmirx_drv->hdmirx_core);
	kfree(g_hdmirx_drv);
	hdmirx_err("%s: check log and free core and drv memory!!!\n", __func__);
	return -1;

}

int aw_hdmirx_drv_remove(struct platform_device *pdev)
{
	struct aw_hdmirx_driver *dev;
	struct list_head *list;

	hdmirx_inf("%s: 1111\n", __func__);
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
		_aw_hdmirx_cec_exit();
#endif
	hdmirx_inf("%s: 2222\n", __func__);

	aw_core_statemachine_thread_exit();
	aw_core_hdcp_thread_exit();
	aw_core_scan_thread_exit();

	aw_hdmirx_drv_resource_release();

	while (!list_empty(&devlist_global)) {
		list = devlist_global.next;
		list_del(list);
		dev = list_entry(list, struct aw_hdmirx_driver, devlist);

		if (dev == NULL)
			continue;
	}

	hdmirx_inf("%s: end.\n", __func__);
	return 0;
}
