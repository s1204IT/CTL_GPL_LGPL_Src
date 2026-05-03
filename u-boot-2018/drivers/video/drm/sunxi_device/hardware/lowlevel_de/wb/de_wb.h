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

#ifndef _DE_WB_H_
#define _DE_WB_H_

#include <drm/drm_framebuffer.h>
#include <de_base.h>
#include <csc/de_csc.h>

struct de_wb_handle {
	//TODO add hw feature info, format / size etc
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	const uint32_t *formats;
	unsigned int format_count;
	struct de_wb_private *private;
};

struct wb_in_config {
	unsigned int width;
	unsigned int height;
	struct de_csc_info csc_info;
};

enum eink_bit_num {
	EINK_BIT_1 = 0x01,
	EINK_BIT_2 = 0x02,
	EINK_BIT_3 = 0x03,
	EINK_BIT_4 = 0x04,
	EINK_BIT_5 = 0x05
};


enum wb_output_fmt {
	WB_FORMAT_RGB_888	 = 0x0,
	WB_FORMAT_BGR_888	 = 0x1,
	WB_FORMAT_ARGB_8888	 = 0x4,
	WB_FORMAT_ABGR_8888	 = 0x5,
	WB_FORMAT_BGRA_8888	 = 0x6,
	WB_FORMAT_RGBA_8888	 = 0x7,
	WB_FORMAT_YUV420_P	 = 0x8,
	WB_FORMAT_Y8		 = 0x9,
	WB_FORMAT_Y5		 = 0xa,
	WB_FORMAT_Y4		 = 0xb,
	WB_FORMAT_YUV420_SP_VUVU = 0xc,
	WB_FORMAT_YUV420_SP_UVUV = 0xd,
	WB_FORMAT_Y3		 = 0xe,
};


struct eink_rect {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
};

struct eink_size {
	unsigned int width;
	unsigned int height;
};

struct eink_frame {
	struct eink_rect crop;
	struct eink_size size;
	unsigned int addr;
};

enum upd_mode {
	EINK_INIT_MODE = 0x01,
	EINK_DU_MODE = 0x02,
	EINK_GC16_MODE = 0x04,
	EINK_GC4_MODE = 0x08,
	EINK_A2_MODE = 0x10,
	EINK_GL16_MODE = 0x20,
	EINK_GLR16_MODE = 0x40,
	EINK_GLD16_MODE = 0x80,
	EINK_GU16_MODE	= 0x84,
	EINK_CLEAR_MODE = 0x88,
	EINK_GC4L_MODE = 0x8c,
	EINK_GCC16_MODE = 0xa0,
	/* use self upd win not de*/
	EINK_RECT_MODE  = 0x400,
	/* AUTO MODE: auto select update mode by E-ink driver */
	/*EINK_AUTO_MODE = 0x8000,*/

/*	EINK_NO_MERGE = 0x80000000,*/
};


struct upd_win {
	u32 left;
	u32 top;
	u32 right;
	u32 bottom;
};

enum upd_pixel_fmt {
	EINK_RGB888 = 0x0,
	EINK_Y8 = 0x09,
	EINK_Y5 = 0x0a,
	EINK_Y4 = 0x0b,
	EINK_Y3 = 0x0e,
};

struct upd_pic_size {
	u32 width;
	u32 height;
	u32 align;
};



enum dither_mode {
	QUANTIZATION,
	FLOYD_STEINBERG,
	ATKINSON,
	ORDERED,
	SIERRA_LITE,
	BURKES,
};

struct eink_wb_config_t{
	enum wb_output_fmt out_fmt;
	unsigned int csc_std;
	unsigned int pitch;
	enum dither_mode dither_mode;
	bool win_en;
	struct eink_frame frame;
} ;

#define EWB_OK 0
#define EWB_OVFL 1
#define EWB_TIMEOUT 2
#define EWB_BUSY 3
#define EWB_ERR 4
#define WB_END_IE	0x1
#define WB_FINISH_IE	(0x1<<4)
#define WB_FIFO_OVERFLOW_ERROR_IE	(0x1<<5)
#define WB_TIMEOUT_ERROR_IE	(0x1<<6)

struct de_wb_handle *de_wb_create(struct module_create_info *info);
int de_wb_apply(struct de_wb_handle *handle, struct wb_in_config *in, struct drm_framebuffer *out_fb);
int de_wb_enable_irq(struct de_wb_handle *handle, u32 en);
int de_wb_set_a2_mode(struct de_wb_handle *handle, bool en);
int de_wb_set_panel_bit(struct de_wb_handle *handle, u32 panel_bit);
int de_wb_eink_set_para(struct de_wb_handle *handle, struct eink_wb_config_t *cfg);
int de_wb_set_last_img(struct de_wb_handle *handle, u32 addr);
int de_wb_stop(struct de_wb_handle *handle);

int de_wb_eink_enable(struct de_wb_handle *handle);
int de_wb_eink_disable(struct de_wb_handle *handle);
int de_wb_eink_get_status(struct de_wb_handle *handle);
int de_wb_eink_interrupt_clear(struct de_wb_handle *handle);
int de_wb_set_gray_level(struct de_wb_handle *handle, u32 gray_level);
int de_wb_eink_get_upd_win(struct de_wb_handle *handle, struct upd_win *upd_win);

#endif /* #ifndef _DE_WB_H_ */
