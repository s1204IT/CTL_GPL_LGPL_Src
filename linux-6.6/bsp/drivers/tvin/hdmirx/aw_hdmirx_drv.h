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
#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include <linux/version.h>
#include <linux/of_gpio.h>
#include <media/cec-notifier.h>
#include "aw_hdmirx_core/aw_hdmirx_core_drv.h"

//#define rst_bus_hdmi

enum hpd_status {
	STATUE_CLOSE = 0,
	STATUE_OPEN  = 1,
};

struct file_ops {
	int ioctl_cmd;
};

struct aw_hdmirx_mutex_s {
	struct mutex lock_ctrl;
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	struct mutex lock_cec;
	struct mutex lock_cec_notifier;
#endif
};

struct aw_hdmirx_clock_s {
	struct clk  *clk_hdmi_rx_dtl;
	struct clk  *clk_hdmi_rx_cb;
	struct clk  *clk_hdmi_cec_clk32k;
	struct clk  *clk_cec_peri_gate;
	struct clk  *clk_hdmi_cec;
	struct clk  *clk_bus_hdmi_rx;
	struct reset_control  *rst_bus_hdmi_rx;
};

struct aw_hdmirx_dts_s {
	int irq_vs;
	int irq_cec;
	int hdmi_num;
};

/**
 * @desc: allwinner hdmirx driver main struct
 */
struct aw_hdmirx_driver {
	struct platform_device  *pdev;
	struct device           *parent_dev;
	struct list_head         devlist;

	/* HDMI RX Controller */
	uintptr_t          reg_base_vs;
	/* EDID_TOP */
	uintptr_t          reg_edid_top;
	/* HDMIRX_TOP */
	uintptr_t          reg_hdmirx_top;

	U8 hdmirx_eanble;

	struct aw_hdmirx_dts_s    aw_dts;
	struct aw_hdmirx_clock_s  aw_clock;
	struct aw_hdmirx_mutex_s  aw_mutex;

	struct aw_hdmirx_core_s  *hdmirx_core;

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	struct platform_device *cec;
	struct cec_notifier    *cec_notifier;
	struct cec_adapter     *adap;
#endif
};

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
struct aw_hdmirx_cec {
	struct device *dev;
	struct cec_adapter *adap;
	struct cec_notifier *notify;

	struct cec_msg rx_msg;
	unsigned int tx_status;
	bool tx_done;
	bool rx_done;
	u32 logical_addr;
};

ssize_t aw_hdmirx_cec_dump_info(char *buf);
void aw_hdmirx_cec_msg_debug(int tx_rx, unsigned char *msg, int len);
int aw_hdmirx_cec_probe(struct platform_device *pdev);
int aw_hdmirx_cec_remove(struct platform_device *pdev);
#if IS_ENABLED(CONFIG_PM_SLEEP)
int aw_hdmirx_cec_suspend(struct device *dev);
int aw_hdmirx_cec_resume(struct device *dev);
#endif
#endif

bool aw_hdmirx_drv_ishdmisource(TSourceId source_id);
int aw_hdmirx_drv_isvalidsource(TSourceId source_id);
void aw_hdmirx_drv_setsource(TSourceId source_id);
void aw_hdmirx_drv_sethpdtimeinterval(U32 mHpd_interval_ms);
void aw_hdmirx_drv_setportmap(TSourceId source_id, U8 soc_port_id);
void aw_hdmirx_drv_setupdateedid(U8 *edid_table, U8 edid_version);
void aw_hdmirx_drv_setarcenable(bool bOn);
void aw_hdmirx_drv_setarctxpath(ARCTXPath_e arc_tx_path);
void aw_hdmirx_drv_setedidversion(U8 version);
void aw_hdmirx_drv_setpullhotplug(U8 port_id, U8 type);
void aw_hdmirx_drv_get5vstate(U32 *pb5vstate);

ssize_t aw_hdmirx_drv_edid_parse(char *buf);
ssize_t aw_hdmirx_drv_dump_SignalInfo(char *buf);
ssize_t aw_hdmirx_drv_dump_status(char *buf);
ssize_t aw_hdmirx_drv_dump_hdcp14(char *buf);

static int aw_hdmirx_dts_parse_clk(struct platform_device *pdev);
static int aw_hdmirx_dts_parse_basic_info(struct platform_device *pdev);
static void aw_hdmirx_drv_clock_enable(void);
static void aw_hdmirx_drv_clock_disable(void);
static void aw_hdmirx_drv_resource_release(void);

int aw_hdmirx_init_resoure(struct platform_device *pdev);
int aw_hdmirx_init_mutex_lock(void);
void aw_hdmirx_init_global_value(void);
int aw_hdmirx_init_notify_node(void);
void aw_hdmirx_drv_hpd_notify(u32 status);

int aw_hdmirx_drv_resume(struct device *dev);
int aw_hdmirx_drv_suspend(struct device *dev);

int aw_hdmirx_drv_remove(struct platform_device *pdev);
int aw_hdmirx_drv_probe(struct platform_device *pdev);

void aw_hdmirx_drv_write(uintptr_t addr, u32 data);

u32 aw_hdmirx_drv_read(uintptr_t addr);

#endif