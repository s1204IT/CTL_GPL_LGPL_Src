/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_TOP_H_
#define _DE_TOP_H_

#include <linux/types.h>
#include "de_base.h"

struct de_top_handle {
	struct module_create_info cinfo;
	bool share_scaler;
	bool support_offline;
	struct de_top_private *private;
};

struct de_top_display_cfg {
	unsigned int display_id;
	bool enable;
	unsigned int w;
	unsigned int h;
	unsigned int device_index;
	unsigned long rcq_header_addr;
	unsigned int rcq_header_byte;
	unsigned int pixel_mode;
};

enum de_rtwb_mode {
	TIMING_FROM_TCON = 0,
	SELF_GENERATED_TIMING = 1,
};

enum de_rt_wb_pos {
	FROM_BLENER,
	FROM_DISP,
};

struct de_offline_get_info {
	void *vir_addr;
	unsigned long buff_size;
};

struct de_top_wb_cfg {
	unsigned int disp;
	bool enable;
	enum de_rtwb_mode mode;
	enum de_rt_wb_pos pos;
};

enum de_offline_mode {
	ONE_FRAME_DELAY = 0,
	CURRENT_FRAME = 1,
};

struct offline_cfg {
	bool enable;
	enum de_offline_mode mode;
	unsigned int w;
	unsigned int h;
};

struct dfs_cfg {
	bool enable;
	unsigned int display_id;
	unsigned int de_clk;
	unsigned int dclk;
};

int de_top_request_rcq_fifo_update(struct de_top_handle *hdl, u32 disp, unsigned long rcq_header_addr, unsigned int rcq_header_byte);

int de_top_display_config(struct de_top_handle *hdl, const struct de_top_display_cfg *cfg);

int de_top_set_chn_mux(struct de_top_handle *hdl, u32 disp, u32 port, u32 chn_type_id, bool is_video);

bool de_top_check_display_update_finish_with_clear(struct de_top_handle *hdl, u32 disp);

int de_top_set_rcq_update(struct de_top_handle *hdl, u32 disp, bool update);

int de_top_set_double_buffer_ready(struct de_top_handle *hdl, u32 disp);

int de_top_wb_config(struct de_top_handle *hdl, const struct de_top_wb_cfg *cfg);

struct de_top_handle *de_top_create(const struct module_create_info *info);

s32 de_top_offline_mode_config(struct de_top_handle *hdl, struct offline_cfg *cfg);
s32 de_top_get_offline_mode_status(struct de_top_handle *hdl);
s32 de_top_get_offline_info(struct de_top_handle *hdl, struct de_offline_get_info *offline_info);
enum de_offline_mode_status de_top_offline_mode_query_state_with_clear(struct de_top_handle *hdl, enum de_offline_mode_status offline_state);

s32 de_top_dfs_config_enable(struct de_top_handle *hdl, struct dfs_cfg *cfg);

bool de_top_query_de_busy_state(struct de_top_handle *hdl, u32 disp);

#endif /* #ifndef _DE_TOP_H_ */
