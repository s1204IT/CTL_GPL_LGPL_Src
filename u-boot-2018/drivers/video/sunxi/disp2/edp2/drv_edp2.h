/* drv_edp2.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __DRV_EDP_H__
#define __DRV_EDP_H__

#include <common.h>
#include <sunxi_display2.h>
#include <sunxi_edp.h>
#include "edp_core/edp_core.h"
#include "edp_core/edp_edid.h"
#include "edp_configs.h"
#include "../include/disp_edid.h"
#include "panels/edp_panels.h"

extern uintptr_t disp_getprop_regbase(char *main_name, char *sub_name,
									u32 index);
extern u32 disp_getprop_irq(char *main_name, char *sub_name,
									u32 index);
s32 edp2_init(void);

#define EDP_NUM_MAX 2
#define EDP_POWER_STR_LEN 32

struct edp_debug {
	unsigned long aux_i2c_addr;
	unsigned long aux_i2c_len;
	unsigned long aux_read_start;
	unsigned long aux_read_end;
	u32 aux_write_start;
	u32 aux_write_len;
	u32 aux_write_val[16];
	u32 aux_write_val_before[16];
	u32 lane_debug_en;
	u32 hpd_mask;
	u32 hpd_mask_pre;
};

/**
 * save some info here for every edp module
 */
struct edp_info_t {
	u32 id;
	u32 enable;
	u32 irq;
	uintptr_t base_addr;
	struct clk *clk_bus;
	struct clk *clk;
	struct clk *clk_24m;
	char *vdd_regulator;
	char *vcc_regulator;
	struct reset_control *rst_bus;
	s32 rst_gpio;

	char dpcd_rx_buf[576];
	char dpcd_ext_rx_buf[32];
	bool hpd_state;
	bool hpd_state_now;
	bool suspend;
	bool training_done;
	bool timings_fixed;
	bool edid_timings_prefer;
	bool fps_limit_60;
	bool sink_capacity_prefer;
	bool use_def_timings;
	bool use_edid_timings;

	bool use_debug_para;

	struct mutex mlock;
	struct edp_tx_core edp_core;
	struct edp_tx_cap source_cap;
	struct edp_rx_cap sink_cap;
	struct edp_debug edp_debug;
	struct sunxi_edp_output_desc *desc;
};

enum sunxi_edp_output_type {
	EDP_OUTPUT = 0,
	DP_OUTPUT  = 1,
};
typedef enum sunxi_edp_output_type sunxi_edp_output_type;

struct sunxi_edp_output_match {
	const char *compatible;
	struct sunxi_edp_output_desc *desc;
};

struct sunxi_edp_output_desc {
	enum sunxi_edp_output_type type;
	s32 (*init)(u32 sel);
	s32 (*exit)(u32 sel);
	s32 (*sw_enable_early)(u32 sel);
	s32 (*sw_enable)(u32 sel);
	s32 (*enable_early)(u32 sel);
	s32 (*enable)(u32 sel);
	s32 (*disable)(u32 sel);
	s32 (*plugin)(u32 sel);
	s32 (*plugout)(u32 sel);
	s32 (*runtime_suspend)(u32 sel);
	s32 (*runtime_resume)(u32 sel);
	s32 (*suspend)(u32 sel);
	s32 (*resume)(u32 sel);
	void (*soft_reset)(u32 sel);
};


extern s32 disp_set_edp_func(struct disp_tv_func *func);
extern s32 disp_deliver_edp_dev(struct device *dev);
extern s32 disp_edp_drv_init(int sel);
extern s32 disp_edp_drv_deinit(int sel);
extern bool disp_init_ready(void);

#endif /*End of file*/
