/*
 * include/sunxi_disp_param.h
 *
 * Copyright (c) 2023 Allwinnertech Co., Ltd.
 * Author: hongyaobin <hongyaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SUNXI_DISP_PARAM_H__
#define __SUNXI_DISP_PARAM_H__

struct sunxi_disp_param {
	char *cpu_id;	    /* 区分CPU平台屏参, 取值范围：A40/A83T/RK3568/RK3588 */
	char *disp_main;	/* 主屏显示类型, 取值范围：edp/edp2/lvds/lvds2/mipi/mipi2/hdmi/hdmi2/hdmi3/hdmi4/vbo/vbo2/typec/typec2 */
	char *disp_aux;		/* 副屏显示类型, 取值范围与disp_main一样 */
	char *lcd_main_driver_name;
	char *lcd_aux_driver_name;
	unsigned int lcd_main_tcon_mode;
	unsigned int lcd_aux_tcon_mode;
	unsigned int lcd_main_tcon_en_odd_even_div;
	unsigned int lcd_aux_tcon_en_odd_even_div;
	unsigned int mipi_main_port_num;
	unsigned int mipi_aux_port_num;
	unsigned int backlight_used;
	unsigned int backlight2_used;
	unsigned int backlight_pol;
	unsigned int backlight2_pol;
	unsigned int lcd_main_start_delay;
	unsigned int lcd_aux_start_delay;
	unsigned int lcd_main_node;
	unsigned int disp_main_x;   /* 主屏水平像素 */
	unsigned int disp_main_y;   /* 主屏垂直像素 */
	unsigned int disp_main_dclk_freq_khz;   /* 主屏时钟频率（单位：khz） */
	unsigned int disp_main_hbp;     /* 主屏水平前沿 */
	unsigned int disp_main_hspw;    /* 主屏水平同步脉宽 */
	unsigned int disp_main_hfp;     /* 主屏水平后沿 */
	unsigned int disp_main_vbp;     /* 主屏垂直前沿 */
	unsigned int disp_main_vspw;    /* 主屏垂直同步脉宽 */
	unsigned int disp_main_vfp;     /* 主屏垂直后沿 */
	unsigned int disp_main_hsync;   /* 主屏水平同步信号的有效电平 */
	unsigned int disp_main_vsync;   /* 主屏垂直同步信号的有效电平 */
	unsigned int lcd_aux_node;
	unsigned int disp_aux_x;
	unsigned int disp_aux_y;
	unsigned int disp_aux_dclk_freq_khz;
	unsigned int disp_aux_hbp;
	unsigned int disp_aux_hspw;
	unsigned int disp_aux_hfp;
	unsigned int disp_aux_vbp;
	unsigned int disp_aux_vspw;
	unsigned int disp_aux_vfp;
	unsigned int disp_aux_hsync;
	unsigned int disp_aux_vsync;
	unsigned int lvds_lane_count;   /* 屏幕通道数, 取值范围：1-1lane, 2-2lane, 4-4lane */
	unsigned int edp_lane_count;
	unsigned int mipi_lane_count;
	unsigned int vbo_lane_count;
	unsigned int lvds_color_depth;  /* 屏幕颜色的位深, 取值范围：6-6 bit color, 8-8 bit color */
	unsigned int edp_color_depth;
	unsigned int mipi_color_depth;
	unsigned int vbo_color_depth;
	unsigned int lvds_fmt;          /* LVDS数据模式, 取值范围：0-vesa(spwg), 1-jeida */
	unsigned int lvds_reverse;      /* LVDS的lane0和lane1交换, 取值范围：0-不交换(LINK0-ODD LINK1-EVEN)/1-交换(LINK0-EVEN LINK1-ODD) */
	unsigned int hdmi_output;
	unsigned int backlight_level;   /* 上电启动默认背光0亮度(0-255) */
	unsigned int backlight2_level;  /* 上电启动默认背光1亮度(0-255) */
	unsigned int backlight_freq;    /* 背光的PWM频率，单位Hz */
	unsigned int backlight2_freq;   /* 背光2的PWM频率，单位Hz */
	unsigned int backlight_delayms; /* 打开背光0前的延时, 取值范围：(0-5000) */
	unsigned int backlight2_delayms;/* 打开背光2前的延时, 取值范围：(0-5000) */
	unsigned int fb0_width;         /* 主屏系统UI显示宽度 */
	unsigned int fb0_height;        /* 主屏系统UI显示高度 */
	unsigned int backlight_ch;      /* 背光调节, 取值范围：1-硬件第一路背光、2-硬件第二路背光 */
	unsigned int backlight2_ch;     /* 背光2调节, 取值范围：1-硬件第一路背光、2-硬件第二路背光 */
};

struct hdmi_spec {
    unsigned int index;
    char *detail;
};

int update_disp_param(struct sunxi_disp_param *param);
int copy_update_file(char *file_name);

#endif
