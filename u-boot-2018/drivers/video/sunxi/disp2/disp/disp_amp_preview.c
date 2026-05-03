/*
 * Copyright (c) 2023 Allwinnertech Co., Ltd.
 *
 * Display driver for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/io.h>
#include <common.h>
#include <sys_config.h>
#include <sunxi_hwspinlock.h>
#include "dev_disp.h"

struct preview_display_param {
	u32 src_w;
	u32 src_h;
	u32 fmt;
	u32 src_rot;
	u32 screen_x;
	u32 screen_y;
	u32 screen_w;
	u32 screen_h;
	u32 buffer_addr;
	u32 aux_line_buffer_addr;
} __attribute__ ((packed));

struct layer_reg_info {
	u32 address_reg[3];
	u32 alpha_reg;
	u32 alpha_shift;
} __attribute__ ((packed));

struct display_info {
	u32 line_reg;
	u32 line_shift;
	u32 line_mask;
	u32 safe_line;
	u32 sleep_us;
	struct layer_reg_info rgb; /* auxline or logo layer*/
	u32 fill_color_en_reg;
	u32 fill_color_en_bit;
	u32 fill_color_reg;
	u32 logo_addr;
} __attribute__ ((packed));

struct amp_preview_driver_info {
	struct display_info disp[2];
	struct layer_reg_info layer[4];
} __attribute__ ((packed));

struct preview_exchange_param {
	u32 magic;
	struct preview_display_param from_rtos;
	u8 from_ready;
	struct amp_preview_driver_info to_rtos;
	u8 to_ready;
} __attribute__ ((packed));

#define PREVIEW_MAGIC		0x10244021
#define ROUND_DOWN(a, b)	((a) & ~((b)-1))
#define ROUND_UP(a, b)		(((a) + (b)-1) & ~((b)-1))
#define ROUND_DOWN_CACHE(a)	ROUND_DOWN(a, CONFIG_SYS_CACHELINE_SIZE)
#define ROUND_UP_CACHE(a)	ROUND_UP(a, CONFIG_SYS_CACHELINE_SIZE)
#define HWSPINLOCK_DISP_AMP	(0)

static struct preview_display_param param;

int hal_save_int_to_kernel(char *name, int value);
int get_fb_config_size(int fb_id, int *w, int *h);
int auxiliary_line_init(void);
int draw_auxiliary_line(void *base, int buffer_w, int buffer_h, int draw_x, int draw_y, int draw_w, int draw_h, int rotate, int lr);

static void disp_para_exchange(struct preview_exchange_param *p, bool get)
{
#ifndef CONFIG_DISP2_AMP_PREVIEW_DISPLAY_TMP_TEST
	bool finish = 0;
	uint32_t flush_start_addr = ROUND_DOWN_CACHE((unsigned long)CONFIG_DISP2_PARA_ELF_ADDR);
	uint32_t flush_cache_size = ROUND_UP_CACHE(sizeof(*p));
	struct preview_exchange_param *tmp = (struct preview_exchange_param *)flush_start_addr;

	do {
		if (hwspin_lock(HWSPINLOCK_DISP_AMP) == HWSPINLOCK_OK) {
			if (get) {
				flush_cache(flush_start_addr, flush_cache_size);
				memcpy(p, (void *)flush_start_addr, sizeof(*p));
				if (p->magic == PREVIEW_MAGIC && p->from_ready) {
					finish = 1;
					tmp->magic = 0;
				}
			} else {
				p->to_ready = 1;
				p->magic = PREVIEW_MAGIC;
				memcpy((void *)flush_start_addr, p, sizeof(*p));
				flush_cache(flush_start_addr, flush_cache_size);
				finish = 1;
			}
			hwspin_unlock(HWSPINLOCK_DISP_AMP);
		} else {
			pr_err("disp para get err\n");
			finish = 1;
		}
		if (!finish)
			disp_delay_ms(10);
	} while (!finish);
#else
	if (get) {
		p->from_rtos.src_w = 720;
		p->from_rtos.src_h = 480;
		p->from_rtos.fmt = DISP_FORMAT_YUV420_SP_UVUV;
		p->from_rtos.src_rot = 0;
		p->from_rtos.screen_x = 0;
		p->from_rtos.screen_y = 0;
		p->from_rtos.screen_w = 1024;
		p->from_rtos.screen_h = 600;
		p->from_rtos.buffer_addr = 0x40000000;
		p->from_rtos.aux_line_buffer_addr = 0;
	} else {
	}
#endif
}

static void get_amp_preview_config_info(struct preview_display_param *param)
{
	struct preview_exchange_param para;
	memset(&para, 0, sizeof(para));
	disp_para_exchange(&para, 1);
	memcpy(param, &para.from_rtos, sizeof(*param));
}

static void send_amp_preview_driver_info(unsigned long fb_addr)
{
	struct amp_preview_driver_info info;
	struct disp_amp_preview_channel_info ch;
	const int logo_channel = 1;
	const int preview_channel = 0;
	struct disp_manager *mgr = g_disp_drv.mgr[0];
	struct disp_device_info dev;
	struct disp_device *dispdev = mgr->device;
	struct preview_exchange_param para;
	int i;

	memset(&dev, 0,  sizeof(dev));
	memset(&info, 0,  sizeof(info));
	de_get_amp_preview_info(0, preview_channel, &ch);
	info.layer[0].address_reg[0] = ch.addr_reg;
	info.layer[0].address_reg[1] = info.layer[0].address_reg[0] + 0x4;
	info.layer[0].address_reg[2] = info.layer[0].address_reg[1] + 0x4;
	info.layer[0].alpha_reg = ch.alpha_reg;
	info.layer[0].alpha_shift = ch.alpha_shift;

	for (i = 1; i <= 3; i++) {
		info.layer[i].address_reg[0] = info.layer[i - 1].address_reg[0] + ch.layer_reg_size;
		info.layer[i].address_reg[1] = info.layer[i - 1].address_reg[1] + ch.layer_reg_size;
		info.layer[i].address_reg[2] = info.layer[i - 1].address_reg[2] + ch.layer_reg_size;
		info.layer[i].alpha_reg = ch.alpha_reg;
		info.layer[i].alpha_shift = ch.alpha_shift;
	}

	de_get_amp_preview_info(0, logo_channel, &ch);
	info.disp[0].rgb.address_reg[0] = ch.addr_reg;
	info.disp[0].rgb.alpha_reg = ch.alpha_reg;
	info.disp[0].rgb.alpha_shift = ch.alpha_shift;
	info.disp[0].fill_color_en_reg = ch.fill_color_en_reg;
	info.disp[0].fill_color_en_bit = ch.fill_color_en_bit;
	info.disp[0].fill_color_reg = ch.fill_color_reg;

	info.disp[0].logo_addr = fb_addr;
#ifdef SUPPORT_AMP_PREVIEW_REG
	if (dispdev->get_preview_device_info) {
		dispdev->get_preview_device_info(dispdev, &dev);
	}
	info.disp[0].line_reg = dev.line_reg;
	info.disp[0].line_shift = dev.line_shift;
	info.disp[0].line_mask = dev.line_mask;
	info.disp[0].safe_line = dev.safe_line;
	info.disp[0].sleep_us = dev.sleep_us;
#endif

	memset(&para, 0, sizeof(para));
	memcpy(&para.to_rtos.disp[0], &info.disp[0], sizeof(info.disp[0]));
	memcpy(&para.to_rtos.layer[0], &info.layer[0], 4 * sizeof(info.layer[0]));
	disp_para_exchange(&para, 0);
}

void preview_display_invisible(struct preview_display_param *param)
{
	u32 w, h;
	bool swap = param->src_rot == 1 || param->src_rot == 3;
	int fb_w, fb_h;
	unsigned long buffer_addr = param->buffer_addr;
	struct disp_manager *mgr = g_disp_drv.mgr[0];
	struct disp_layer_config config[2];

	get_fb_config_size(0, &fb_w, &fb_h);
	memset(config, 0, sizeof(config[0]) * 2);
	if (mgr && mgr->device && mgr->device->get_resolution)
		mgr->device->get_resolution(mgr->device, &w, &h);
	else {
		printf("mgr not connect with device\n");
		return;
	}

	config[0].enable = 1;
	config[0].channel = 1;
	config[0].layer_id = 0;
	config[0].info.zorder = 1;
	config[0].info.mode = LAYER_MODE_COLOR;
	config[0].info.alpha_mode = 1;// global
	config[0].info.alpha_value = 255;
	config[0].info.fb.format = DISP_FORMAT_ARGB_8888;
	config[0].info.color = 0xff000000; /* black */
	config[0].info.screen_win.width = w;
	config[0].info.screen_win.height = h;
	config[0].info.screen_win.x = 0;
	config[0].info.screen_win.y = 0;
	config[0].info.fb.crop.x = 0;
	config[0].info.fb.crop.y = 0;
	config[0].info.fb.crop.width =
	    ((long long)(fb_w) << 32);
	config[0].info.fb.crop.height =
	    ((long long)(fb_h) << 32);

	w = swap ? param->src_h : param->src_w;
	h = swap ? param->src_w : param->src_h;
	config[1].enable = 1;
	config[1].info.zorder = 0;
	config[1].info.mode = LAYER_MODE_BUFFER;
	config[1].info.alpha_mode = 1;// global
	config[1].info.alpha_value = 255;
	config[1].channel = 0;
	config[1].layer_id = 0;
	config[1].info.fb.format = param->fmt;
	config[1].info.fb.size[0].width = w;
	config[1].info.fb.size[0].height = h;
	config[1].info.fb.size[1].width = w / 2;
	config[1].info.fb.size[1].height = h / 2;
	config[1].info.fb.size[2].width = w / 2;
	config[1].info.fb.size[2].height = h / 2;
	config[1].info.fb.addr[0] = buffer_addr;
	config[1].info.fb.addr[1] = buffer_addr + w * h;
	config[1].info.fb.addr[2] = buffer_addr + w * h * 5 / 4;
	config[1].info.screen_win.x = param->screen_x;
	config[1].info.screen_win.y = param->screen_y;
	config[1].info.screen_win.width = param->screen_w;
	config[1].info.screen_win.height = param->screen_h;
	config[1].info.fb.crop.x = 0;
	config[1].info.fb.crop.y = 0;
	config[1].info.fb.crop.width = (long long)(w) << 32;
	config[1].info.fb.crop.height = (long long)(h) << 32;

	mgr->set_layer_config(mgr, config, 2);
}

void amp_preview_init(unsigned long fb_addr)
{
	bool rotate_swap;
	int fb_w, fb_h;
	int w, h;
	get_amp_preview_config_info(&param);
	preview_display_invisible(&param);
	rotate_swap = param.src_rot == 1 || param.src_rot == 3;
	if (param.aux_line_buffer_addr) {
		get_fb_config_size(0, &fb_w, &fb_h);
		w = rotate_swap ? fb_h : fb_w;
		h = rotate_swap ? fb_w : fb_h;
		auxiliary_line_init();
		draw_auxiliary_line((void *)param.aux_line_buffer_addr, w, h, 0, 0, w, h, 0, 0);
	}
	/* remote processor may use preview after this call */
	send_amp_preview_driver_info(fb_addr);
}

#ifndef CONFIG_DISP2_AMP_PREVIEW_DISPLAY_TMP_TEST
void amp_preview_save(void)
{
	int prop_len;
	const void *prop;
	struct disp_layer_config lyr_cfg;
	u32 prop_tmp[32] = {0};
	struct disp_manager *mgr = g_disp_drv.mgr[0];
	int mem_reg_node = fdt_path_offset(working_fdt, "/reserved-memory/car_reverse_reserved");
	int disp_node = fdt_path_offset(working_fdt, "/soc/disp");

	if (mem_reg_node > 0) {
		prop = fdt_getprop(working_fdt, mem_reg_node, "reg", &prop_len);
		memcpy(prop_tmp, prop, prop_len);
		fdt_appendprop(working_fdt, disp_node, "sunxi-iova-premap", prop_tmp, prop_len);
	}

	mem_reg_node = fdt_path_offset(working_fdt, "/reserved-memory/video_data");
	if (mem_reg_node > 0) {
		prop = fdt_getprop(working_fdt, mem_reg_node, "reg", &prop_len);
		memcpy(prop_tmp, prop, prop_len);
		fdt_appendprop(working_fdt, disp_node, "sunxi-iova-premap", prop_tmp, prop_len);
	}

	lyr_cfg.channel = 0;
	lyr_cfg.layer_id = 0;
	mgr->get_layer_config(mgr, &lyr_cfg, 1);

	hal_save_int_to_kernel("screen0_amp_preview", 1);
	hal_save_int_to_kernel("preview_src_width", lyr_cfg.info.fb.size[0].width);
	hal_save_int_to_kernel("preview_src_height", lyr_cfg.info.fb.size[0].height);
	hal_save_int_to_kernel("preview_aux_line_en", 0); // not used
	hal_save_int_to_kernel("preview_screen_x", lyr_cfg.info.screen_win.x);
	hal_save_int_to_kernel("preview_screen_y", lyr_cfg.info.screen_win.y);
	hal_save_int_to_kernel("preview_screen_w", lyr_cfg.info.screen_win.width);
	hal_save_int_to_kernel("preview_screen_h", lyr_cfg.info.screen_win.height);
	hal_save_int_to_kernel("preview_format", param.fmt);
}
#else
void amp_preview_save(void)
{
//	int prop_len;
//	const void *prop;
	struct disp_layer_config lyr_cfg;
//	u32 prop_tmp[32] = {0};
	struct disp_manager *mgr = g_disp_drv.mgr[0];
//	int mem_reg_node = fdt_path_offset(working_fdt, "/reserved-memory/e906_car_reverse_reserved");
//	int disp_node = fdt_path_offset(working_fdt, "/soc/disp");

//	prop = fdt_getprop(working_fdt, mem_reg_node, "reg", &prop_len);
//	memcpy(prop_tmp, prop, prop_len);
//	fdt_setprop(working_fdt, disp_node, "sunxi-iova-premap", prop_tmp, prop_len);

	lyr_cfg.channel = 0;
	lyr_cfg.layer_id = 0;
	mgr->get_layer_config(mgr, &lyr_cfg, 1);

	hal_save_int_to_kernel("screen0_amp_preview", 1);
	hal_save_int_to_kernel("preview_src_width", lyr_cfg.info.fb.size[0].width);
	hal_save_int_to_kernel("preview_src_height", lyr_cfg.info.fb.size[0].height);
	hal_save_int_to_kernel("preview_aux_line_en", 0); // not used
	hal_save_int_to_kernel("preview_screen_x", lyr_cfg.info.screen_win.x);
	hal_save_int_to_kernel("preview_screen_y", lyr_cfg.info.screen_win.y);
	hal_save_int_to_kernel("preview_screen_w", lyr_cfg.info.screen_win.width);
	hal_save_int_to_kernel("preview_screen_h", lyr_cfg.info.screen_win.height);
	hal_save_int_to_kernel("preview_format", param.fmt);
}
#endif
