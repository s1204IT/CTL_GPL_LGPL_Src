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
#include "../aw_hdmirx_define.h"
#include "THDMIRx_Edid.h"
#include "hardware/THDMIRx_TV_Edid_Driver.h"

/* correspond to enum hdmi_vic_e */
const char *hdmi_fmt[] = {
	"HDMI_UNKNOWN",
	"HDMI_640x480p60_4x3",
	"HDMI_720x480p60_4x3",
	"HDMI_720x480p60_16x9",
	"HDMI_1280x720p60_16x9",
	"HDMI_1920x1080i60_16x9",
	"HDMI_720x480i60_4x3",
	"HDMI_720x480i60_16x9",
	"HDMI_720x240p60_4x3",
	"HDMI_720x240p60_16x9",
	"HDMI_2880x480i60_4x3",
	"HDMI_2880x480i60_16x9",
	"HDMI_2880x240p60_4x3",
	"HDMI_2880x240p60_16x9",
	"HDMI_1440x480p60_4x3",
	"HDMI_1440x480p60_16x9",
	"HDMI_1920x1080p60_16x9",
	"HDMI_720x576p50_4x3",
	"HDMI_720x576p50_16x9",
	"HDMI_1280x720p50_16x9",
	"HDMI_1920x1080i50_16x9",
	"HDMI_720x576i50_4x3",
	"HDMI_720x576i50_16x9",
	"HDMI_720x288p_4x3",
	"HDMI_720x288p_16x9",
	"HDMI_2880x576i50_4x3",
	"HDMI_2880x576i50_16x9",
	"HDMI_2880x288p50_4x3",
	"HDMI_2880x288p50_16x9",
	"HDMI_1440x576p_4x3",
	"HDMI_1440x576p_16x9",
	"HDMI_1920x1080p50_16x9",
	"HDMI_1920x1080p24_16x9",
	"HDMI_1920x1080p25_16x9",
	"HDMI_1920x1080p30_16x9",
	"HDMI_2880x480p60_4x3",
	"HDMI_2880x480p60_16x9",
	"HDMI_2880x576p50_4x3",
	"HDMI_2880x576p50_16x9",
	"HDMI_1920x1080i_t1250_50_16x9",
	"HDMI_1920x1080i100_16x9",
	"HDMI_1280x720p100_16x9",
	"HDMI_720x576p100_4x3",
	"HDMI_720x576p100_16x9",
	"HDMI_720x576i100_4x3",
	"HDMI_720x576i100_16x9",
	"HDMI_1920x1080i120_16x9",
	"HDMI_1280x720p120_16x9",
	"HDMI_720x480p120_4x3",
	"HDMI_720x480p120_16x9",
	"HDMI_720x480i120_4x3",
	"HDMI_720x480i120_16x9",
	"HDMI_720x576p200_4x3",
	"HDMI_720x576p200_16x9",
	"HDMI_720x576i200_4x3",
	"HDMI_720x576i200_16x9",
	"HDMI_720x480p240_4x3",
	"HDMI_720x480p240_16x9",
	"HDMI_720x480i240_4x3",
	"HDMI_720x480i240_16x9",
	/* Refet to CEA 861-F */
	"HDMI_1280x720p24_16x9",
	"HDMI_1280x720p25_16x9",
	"HDMI_1280x720p30_16x9",
	"HDMI_1920x1080p120_16x9",
	"HDMI_1920x1080p100_16x9",
	"HDMI_1280x720p24_64x27",
	"HDMI_1280x720p25_64x27",
	"HDMI_1280x720p30_64x27",
	"HDMI_1280x720p50_64x27",
	"HDMI_1280x720p60_64x27",
	"HDMI_1280x720p100_64x27",
	"HDMI_1280x720p120_64x27",
	"HDMI_1920x1080p24_64x27",
	"HDMI_1920x1080p25_64x27",
	"HDMI_1920x1080p30_64x27",
	"HDMI_1920x1080p50_64x27",
	"HDMI_1920x1080p60_64x27",
	"HDMI_1920x1080p100_64x27",
	"HDMI_1920x1080p120_64x27",
	"HDMI_1680x720p24_64x27",
	"HDMI_1680x720p25_64x27",
	"HDMI_1680x720p30_64x27",
	"HDMI_1680x720p50_64x27",
	"HDMI_1680x720p60_64x27",
	"HDMI_1680x720p100_64x27",
	"HDMI_1680x720p120_64x27",
	"HDMI_2560x1080p24_64x27",
	"HDMI_2560x1080p25_64x27",
	"HDMI_2560x1080p30_64x27",
	"HDMI_2560x1080p50_64x27",
	"HDMI_2560x1080p60_64x27",
	"HDMI_2560x1080p100_64x27",
	"HDMI_2560x1080p120_64x27",
	"HDMI_3840x2160p24_16x9",
	"HDMI_3840x2160p25_16x9",
	"HDMI_3840x2160p30_16x9",
	"HDMI_3840x2160p50_16x9",
	"HDMI_3840x2160p60_16x9",
	"HDMI_4096x2160p24_256x135",
	"HDMI_4096x2160p25_256x135",
	"HDMI_4096x2160p30_256x135",
	"HDMI_4096x2160p50_256x135",
	"HDMI_4096x2160p60_256x135",
	"HDMI_3840x2160p24_64x27",
	"HDMI_3840x2160p25_64x27",
	"HDMI_3840x2160p30_64x27",
	"HDMI_3840x2160p50_64x27",
	"HDMI_3840x2160p60_64x27",
	"HDMI_RESERVED",
};

const char *_3d_structure[] = {
	/* hdmi1.4 spec, Table H-7 */
	/* value: 0x0000 */
	"Frame packing",
	"Field alternative",
	"Line alternative",
	"Side-by-Side(Full)",
	"L + depth",
	"L + depth + graphics + graphics-depth",
	"Top-and-Bottom",
	/* value 0x0111: Reserved for future use */
	"Resvrd",
	/* value 0x1000 */
	"Side-by-Side(Half) with horizontal sub-sampling",
	/* value 0x1001-0x1110:Reserved for future use */
	"Resvrd",
	"Resvrd",
	"Resvrd",
	"Resvrd",
	"Resvrd",
	"Resvrd",
	"Side-by-Side(Half) with all quincunx sub-sampling",
};

const char *_3d_detail_x[] = {
	/* hdmi1.4 spec, Table H-8 */
	/* value: 0x0000 */
	"All horizontal sub-sampling and quincunx matrix",
	"Horizontal sub-sampling",
	"Not_in_Use1",
	"Not_in_Use2",
	"Not_in_Use3",
	"Not_in_Use4",
	"All_Quincunx",
	"ODD_left_ODD_right",
	"ODD_left_EVEN_right",
	"EVEN_left_ODD_right",
	"EVEN_left_EVEN_right",
	"Resvrd1",
	"Resvrd2",
	"Resvrd3",
	"Resvrd4",
	"Resvrd5",
};

const char *aud_fmt[] = {
	"HEADER",
	"L-PCM",
	"AC3",
	"MPEG1",
	"MP3",
	"MPEG2",
	"AAC",
	"DTS",
	"ATRAC",
	"OBA",
	"DDP", /* E_AC3 */
	"DTS_HD",
	"MAT", /* TRUE_HD*/
	"DST",
	"WMAPRO",
};

/* 2.0 edid,support HDR,DOLBYVISION */
static unsigned char edid_20[] = {
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
0x05, 0xAC, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00,
0x20, 0x19, 0x01, 0x03, 0x80, 0x73, 0x41, 0x78,
0x0A, 0xCF, 0x74, 0xA3, 0x57, 0x4C, 0xB0, 0x23,
0x09, 0x48, 0x4C, 0x21, 0x08, 0x00, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x74,
0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,
0x8A, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E,
0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
0x58, 0x2C, 0x45, 0x00, 0x20, 0xC2, 0x31, 0x00,
0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x41,
0x4D, 0x4C, 0x20, 0x54, 0x56, 0x0A, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
0x00, 0x3B, 0x46, 0x1F, 0x8C, 0x3C, 0x00, 0x0A,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xDA,
0x02, 0x03, 0x62, 0xF0, 0x5B, 0x5F, 0x10, 0x1F,
0x14, 0x05, 0x13, 0x04, 0x20, 0x22, 0x3C, 0x3E,
0x12, 0x16, 0x03, 0x07, 0x11, 0x15, 0x02, 0x06,
0x01, 0x61, 0x5D, 0x64, 0x65, 0x66, 0x62, 0x60,
0x29, 0x09, 0x07, 0x03, 0x15, 0x07, 0x50, 0x57,
0x07, 0x01, 0x83, 0x01, 0x00, 0x00, 0x6E, 0x03,
0x0C, 0x00, 0x10, 0x00, 0xB8, 0x3C, 0x2F, 0x80,
0x80, 0x01, 0x02, 0x03, 0x04, 0x67, 0xD8, 0x5D,
0xC4, 0x01, 0x78, 0x88, 0x07, 0xE3, 0x05, 0x40,
0x01, 0xE5, 0x0F, 0x00, 0x00, 0x90, 0x05, 0xE3,
0x06, 0x05, 0x01, 0xEE, 0x01, 0x46, 0xD0, 0x00,
0x26, 0x0F, 0x8B, 0x00, 0xA8, 0x53, 0x4B, 0x9D,
0x27, 0x0B, 0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38,
0x2D, 0x40, 0x10, 0x2C, 0x45, 0x80, 0x30, 0xEB,
0x52, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C,
};

void HDMIRx_Edid_Getedid(U8 *pedid_data)
{
	HdmiRx_Edid_Hardware_get_edid(pedid_data);
}

void HDMIRx_Edid_Dumpedid(U8 *buff, char *buf, ssize_t *lenth)
{
	if (!buff)
		return;

	int i, j;
	*lenth += sprintf(buf + *lenth, "__________________________________________________________________\n\n");
	*lenth += sprintf(buf + *lenth, "edid   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");
	*lenth += sprintf(buf + *lenth, "=======================================================\n");
	for (i = 0; i < EDID_DATA_LEN; i += 16) {
		*lenth += sprintf(buf + *lenth, "0x%02x | ", i);
		for (j = 0; j < 16; j++) {
			*lenth += sprintf(buf + *lenth, "%02x ", buff[i + j]);
		}
		*lenth += sprintf(buf + *lenth, "\n");
	}
	*lenth += sprintf(buf + *lenth, "=======================================================\n");
}

void rx_edid_print_vic_fmt(unsigned char i,
	unsigned char hdmi_vic, char *buf, ssize_t *lenth)
{
	/* CTA-861-G: Table-18 */
	/* SVD = 128, 254, 255 are reserved */
	if ((hdmi_vic >= 1) && (hdmi_vic <= 64)) {
		*lenth += sprintf(buf + *lenth, "\tSVD#%2d: vic(%3d), format: %s\n",
			i+1, hdmi_vic, hdmi_fmt[hdmi_vic]);
	} else if ((hdmi_vic >= 65) && (hdmi_vic <= 107)) {
		/* from first new set */
		*lenth += sprintf(buf + *lenth, "\tSVD#%2d: vic(%3d), format: %s\n",
			i+1, hdmi_vic, hdmi_fmt[hdmi_vic]);
	} else if ((hdmi_vic >= 108) && (hdmi_vic <= 127)) {
		/* from first new set: 8bit VIC */
	} else if ((hdmi_vic >= 129) && (hdmi_vic <= 192)) {
		hdmi_vic &= 0x7F;
		*lenth += sprintf(buf + *lenth, "\tSVD#%2d: vic(%3d), native format: %s\n",
			i+1, hdmi_vic, hdmi_fmt[hdmi_vic]);
	} else if ((hdmi_vic >= 193) && (hdmi_vic <= 253)) {
		/* from second new set: 8bit VIC */
	}
}

/* edid header of base block
 * offset 0x00 ~ 0x07
 */
static bool check_edid_header_valid(unsigned char *buff)
{
	int i;
	bool ret = true;

	if (!(buff[0] | buff[7])) {
		for (i = 1; i < 7; i++) {
			if (buff[i] != 0xFF) {
				ret = false;
				break;
			}
		}
	} else {
		ret = false;
	}
	return ret;
}

/* manufacturer name
 * offset 0x8~0x9
 */
static void get_edid_manufacturer_name(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{
	int i;
	unsigned char uppercase[26] = { 0 };
	unsigned char brand[3];

	if (!edid_info || !buff)
		return;
	/* Fill array uppercase with 'A' to 'Z' */
	for (i = 0; i < 26; i++)
		uppercase[i] = 'A' + i;
	/* three 5-byte AscII code, such as 'A' = 00001, 'C' = 00011,*/
	brand[0] = buff[start] >> 2;
	brand[1] = ((buff[start] & 0x3) << 3)+(buff[start+1] >> 5);
	brand[2] = buff[start+1] & 0x1f;
	for (i = 0; i < 3; i++)
		edid_info->manufacturer_name[i] = uppercase[brand[i] - 1];
}

/* product code
 * offset 0xA~0xB
 */
static void get_edid_product_code(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	edid_info->product_code = buff[start+1] << 8 | buff[start];
}

/* serial number
 * offset 0xC~0xF
 */
static void get_edid_serial_number(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	edid_info->serial_number = (buff[start+3] << 24) |
		(buff[start+2] << 16) |
		(buff[start+1] << 8) |
		buff[start];
}

/* manufacture date
 * offset 0x10~0x11
 */
static void get_edid_manufacture_date(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	/* week of manufacture:
	 * 0: not specified
	 * 0x1~0x35: valid week
	 * 0x36~0xfe: reserved
	 * 0xff: model year is specified
	 */
	if ((buff[start] == 0) ||
		((buff[start] >= 0x36) && (buff[start] <= 0xfe)))
		edid_info->product_week = 0;
	else
		edid_info->product_week = buff[start];
	/* year of manufacture,
	 * or model year (if specified by week=0xff)
	 */
	edid_info->product_year = buff[start+1] + 1990;
}

/* The version of EDID version
 * offset 0x12 and 0x13
 */
static void get_edid_version(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	edid_info->edid_version = buff[start];
	edid_info->edid_revision = buff[start+1];
}

/* Basic Display Parameters and Features
 * offset 0x14~0x18
 */
static void get_edid_display_parameters(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{}

/* Color Characteristics. Color Characteristics provides information about
 * the display device's chromaticity and color temperature parameters
 * (white temperature in degrees Kelvin)
 * offset 0x19~0x22
 */
static void get_edid_color_characteristics(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info)
{}

/* established timings are computer display timings recognized by VESA.
 * offset 0x23~0x25
 */
static void get_edid_established_timings(unsigned char *buff,
	unsigned char start, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	if (!edid_info || !buff)
		return;

	*lenth += sprintf(buf + *lenth, "established timing:\n");
	/* each bit for an established timing */
	if (buff[start] & (1 << 5))
		*lenth += sprintf(buf + *lenth, "\t640*480p60hz\n");
	if (buff[start] & (1 << 0))
		*lenth += sprintf(buf + *lenth, "\t800*600p60hz\n");
	if (buff[start+1] & (1 << 3))
		*lenth += sprintf(buf + *lenth, "\t1024*768p60hz\n");
}

/* Standard timings are those either recognized by VESA
 * through the VESA Discrete Monitor Timing or Generalized
 * Timing Formula standards. Each timing is two bytes.
 * offset 0x26~0x35
 */
static void get_edid_standard_timing(unsigned char *buff, unsigned char start,
	unsigned int length, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	unsigned char  i, img_aspect_ratio;
	int hactive_pixel, vactive_pixel, refresh_rate;
	int asp_ratio[] = {
		80*10/16,
		80*3/4,
		80*4/5,
		80*9/16,
	};/*multiple 80 first*/
	if (!edid_info || !buff)
		return;
	*lenth += sprintf(buf + *lenth, "standard timing:\n");
	for (i = 0; i < length; i = i+2) {
		if ((buff[start+i] != 0x01) && (buff[start+i+1] != 0x01)) {
			hactive_pixel = (int)((buff[start+i] + 31)*8);
			/* image aspect ratio:
			 * 0 -> 16:10
			 * 1 -> 4:3
			 * 2 -> 5:4
			 * 3 -> 16:9
			 */
			img_aspect_ratio = (buff[start+i+1] >> 6) & 0x3;
			vactive_pixel =
				hactive_pixel*asp_ratio[img_aspect_ratio]/80;
			refresh_rate = (int)(buff[start+i+1] & 0x3F) + 60;
			*lenth += sprintf(buf + *lenth, "\t%d*%dP%dHz\n", hactive_pixel,
				vactive_pixel, refresh_rate);
		}
	}
}

static void get_edid_monitor_name(unsigned char *p_edid,
	unsigned char start, struct edid_info_s *edid_info)
{
	unsigned char i, j;
	/* CEA861-F Table83 */
	for (i = 0; i < 4; i++) {
		/* 0xFC denotes that last 13 bytes of this
		 * descriptor block contain Monitor name
		 */
		if (p_edid[start+i*18+3] == 0xFC)
			break;
	}
	/* if name < 13 bytes, terminate name with 0x0A
	 * and fill remainder of 13 bytes with 0x20
	 */
	for (j = 0; j < 13; j++) {
		if (p_edid[start+i*18+5+j] == 0x0A)
			break;
		edid_info->monitor_name[j] = p_edid[0x36+i*18+5+j];
	}

}

static void get_edid_range_limits(unsigned char *p_edid,
	unsigned char start, struct edid_info_s *edid_info)
{
	unsigned char i;

	for (i = 0; i < 4; i++) {
		/* 0xFD denotes that last 13 bytes of this
		 * descriptor block contain monitor range limits
		 */
		if (p_edid[start+i*18+3] == 0xFD)
			break;
	}
	/*maxmium supported pixel clock*/
	edid_info->max_sup_pixel_clk = p_edid[0x36+i*18+9]*10;
}


/* edid parse
 * Descriptor data
 * 0xff monitor S/N
 * 0xfe ASCII data string
 * 0xfd monitor range limits
 * 0xfc monitor name
 * 0xfb color point
 * 0xfa standard timing ID
 */
void HDMIRx_Edid_Parse_Block0(uint8_t *p_edid, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	bool edid_header_valid;

	if (!p_edid || !edid_info)
		return;
	edid_header_valid = check_edid_header_valid(p_edid);
	if (!edid_header_valid) {
		hdmirx_inf("edid block0 header invalid!\n");
		return;
	}
	/* manufacturer name offset 8~9 */
	get_edid_manufacturer_name(p_edid, 8, edid_info);
	/* product code offset 10~11 */
	get_edid_product_code(p_edid, 10, edid_info);
	/* serial number offset 12~15 */
	get_edid_serial_number(p_edid, 12, edid_info);

	/* product date offset 0x10~0x11 */
	get_edid_manufacture_date(p_edid, 0x10, edid_info);
	/* EDID version offset 0x12~0x13*/
	get_edid_version(p_edid, 0x12, edid_info);
	/* Basic Display Parameters and Features offset 0x14~0x18 */
	get_edid_display_parameters(p_edid, 0x14, edid_info);
	/* Color Characteristics offset 0x19~0x22 */
	get_edid_color_characteristics(p_edid, 0x19, edid_info);
	/* established timing offset 0x23~0x25 */
	get_edid_established_timings(p_edid, 0x23, edid_info, buf, lenth);
	/* standard timing offset 0x26~0x35*/
	get_edid_standard_timing(p_edid, 0x26, 16, edid_info, buf, lenth);
	/* best resolution: hactive*vactive*/
	edid_info->descriptor1.hactive =
		p_edid[0x38] + (((p_edid[0x3A] >> 4) & 0xF) << 8);
	edid_info->descriptor1.vactive =
		p_edid[0x3B] + (((p_edid[0x3D] >> 4) & 0xF) << 8);
	edid_info->descriptor1.pixel_clk =
		(p_edid[0x37] << 8) + p_edid[0x36];
	/* monitor name */
	get_edid_monitor_name(p_edid, 0x36, edid_info);
	get_edid_range_limits(p_edid, 0x36, edid_info);
	edid_info->extension_flag = p_edid[0x7e];
	edid_info->block0_chk_sum = p_edid[0x7f];
}

static void get_edid_video_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info)
{
	unsigned char i;

	edid_info->video_db.svd_num = len;
	for (i = 0; i < len; i++) {
		edid_info->video_db.hdmi_vic[i] =
			buff[i+start];
	}
}

static void get_edid_audio_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info)
{
	enum edid_audio_format_e fmt;
	int i = start;
	struct pcm_t *pcm;

	do {
		fmt = (buff[i] & 0x78) >> 3;/* bit6~3 */
		edid_info->audio_db.aud_fmt_sup[fmt] = 1;
		/* CEA-861F page 82: audio data block*/
		switch (fmt) {
		case AUDIO_FORMAT_LPCM:
			pcm = &(edid_info->audio_db.sad[fmt].bit_rate.pcm);
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x7);
			if (buff[i+1] & 0x40)
				edid_info->audio_db.sad[fmt].freq_192khz = 1;
			if (buff[i+1] & 0x20)
				edid_info->audio_db.sad[fmt].freq_176_4khz = 1;
			if (buff[i+1] & 0x10)
				edid_info->audio_db.sad[fmt].freq_96khz = 1;
			if (buff[i+1] & 0x08)
				edid_info->audio_db.sad[fmt].freq_88_2khz = 1;
			if (buff[i+1] & 0x04)
				edid_info->audio_db.sad[fmt].freq_48khz = 1;
			if (buff[i+1] & 0x02)
				edid_info->audio_db.sad[fmt].freq_44_1khz = 1;
			if (buff[i+1] & 0x01)
				edid_info->audio_db.sad[fmt].freq_32khz = 1;
			if (buff[i+2] & 0x04)
				pcm->size_24bit = 1;
			if (buff[i+2] & 0x02)
				pcm->size_20bit = 1;
			if (buff[i+2] & 0x01)
				pcm->size_16bit = 1;
			break;
		/* CEA861F table50 fmt2~8 byte 3:
		 * Maximum bit rate divided by 8 kHz
		 */
		case AUDIO_FORMAT_AC3:
		case AUDIO_FORMAT_MPEG1:
		case AUDIO_FORMAT_MP3:
		case AUDIO_FORMAT_MPEG2:
		case AUDIO_FORMAT_AAC:
		case AUDIO_FORMAT_DTS:
		case AUDIO_FORMAT_ATRAC:
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x07);
			if (buff[i+1] & 0x40)
				edid_info->audio_db.sad[fmt].freq_192khz = 1;
			if (buff[i+1] & 0x20)
				edid_info->audio_db.sad[fmt].freq_176_4khz = 1;
			if (buff[i+1] & 0x10)
				edid_info->audio_db.sad[fmt].freq_96khz = 1;
			if (buff[i+1] & 0x08)
				edid_info->audio_db.sad[fmt].freq_88_2khz = 1;
			if (buff[i+1] & 0x04)
				edid_info->audio_db.sad[fmt].freq_48khz = 1;
			if (buff[i+1] & 0x02)
				edid_info->audio_db.sad[fmt].freq_44_1khz = 1;
			if (buff[i+1] & 0x01)
				edid_info->audio_db.sad[fmt].freq_32khz = 1;
			edid_info->audio_db.sad[fmt].bit_rate.others =
				buff[i+2];
			break;
		/* for audio format code 9~13:
		 * byte3 is dependent on Audio Format Code
		 */
		case AUDIO_FORMAT_OBA:
		case AUDIO_FORMAT_DDP:
		case AUDIO_FORMAT_DTSHD:
		case AUDIO_FORMAT_MAT:
		case AUDIO_FORMAT_DST:
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x07);
			if (buff[i+1] & 0x40)
				edid_info->audio_db.sad[fmt].freq_192khz = 1;
			if (buff[i+1] & 0x20)
				edid_info->audio_db.sad[fmt].freq_176_4khz = 1;
			if (buff[i+1] & 0x10)
				edid_info->audio_db.sad[fmt].freq_96khz = 1;
			if (buff[i+1] & 0x08)
				edid_info->audio_db.sad[fmt].freq_88_2khz = 1;
			if (buff[i+1] & 0x04)
				edid_info->audio_db.sad[fmt].freq_48khz = 1;
			if (buff[i+1] & 0x02)
				edid_info->audio_db.sad[fmt].freq_44_1khz = 1;
			if (buff[i+1] & 0x01)
				edid_info->audio_db.sad[fmt].freq_32khz = 1;
			edid_info->audio_db.sad[fmt].bit_rate.others
				= buff[i+2];
			break;
		/* audio format code 14:
		 * last 3 bits of byte3: profile
		 */
		case AUDIO_FORMAT_WMAPRO:
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x07);
			if (buff[i+1] & 0x40)
				edid_info->audio_db.sad[fmt].freq_192khz = 1;
			if (buff[i+1] & 0x20)
				edid_info->audio_db.sad[fmt].freq_176_4khz = 1;
			if (buff[i+1] & 0x10)
				edid_info->audio_db.sad[fmt].freq_96khz = 1;
			if (buff[i+1] & 0x08)
				edid_info->audio_db.sad[fmt].freq_88_2khz = 1;
			if (buff[i+1] & 0x04)
				edid_info->audio_db.sad[fmt].freq_48khz = 1;
			if (buff[i+1] & 0x02)
				edid_info->audio_db.sad[fmt].freq_44_1khz = 1;
			if (buff[i+1] & 0x01)
				edid_info->audio_db.sad[fmt].freq_32khz = 1;
			edid_info->audio_db.sad[fmt].bit_rate.others
				= buff[i+2];
			break;
		/* case 15: audio coding extension coding type */
		default:
			break;
		}
		i += 3; /*next audio fmt*/
	} while (i < (start + len));
}

static void get_edid_speaker_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	int i;

	/* speaker allocation is 3 bytes long*/
	if (len != 3) {
		*lenth += sprintf(buf + *lenth, "invalid length for speaker allocation data block: %d\n",
			len);
		return;
	}
	for (i = 1; i <= 0x80; i = i << 1) {
		switch (buff[start] & i) {
		case 0x80:
			edid_info->speaker_alloc.flw_frw = 1;
			break;
		case 0x40:
			edid_info->speaker_alloc.rlc_rrc = 1;
			break;
		case 0x20:
			edid_info->speaker_alloc.flc_frc = 1;
			break;
		case 0x10:
			edid_info->speaker_alloc.rc = 1;
			break;
		case 0x08:
			edid_info->speaker_alloc.rl_rr = 1;
			break;
		case 0x04:
			edid_info->speaker_alloc.fc = 1;
			break;
		case 0x02:
			edid_info->speaker_alloc.lfe = 1;
			break;
		case 0x01:
			edid_info->speaker_alloc.fl_fr = 1;
			break;
		default:
			break;
		}
	}
	for (i = 1; i <= 0x4; i = i << 1) {
		switch (buff[start+1] & i) {
		case 0x4:
			edid_info->speaker_alloc.fch = 1;
			break;
		case 0x2:
			edid_info->speaker_alloc.tc = 1;
			break;
		case 0x1:
			edid_info->speaker_alloc.flh_frh = 1;
			break;
		default:
			break;
		}
	}
}

static void get_edid_vsdb(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	unsigned char _3d_present_offset;
	unsigned char hdmi_vic_len;
	unsigned char hdmi_vic_offset;
	unsigned char i, j;
	unsigned int ieee_oui;
	unsigned char _3d_struct_all_offset;
	unsigned char hdmi_3d_len;
	unsigned char _3d_struct;
	unsigned char _2d_vic_order_offset;
	unsigned char temp_3d_len;

	/* basic 5 bytes; others: extension fields */
	if (len < 5) {
		*lenth += sprintf(buf + *lenth, "invalid VSDB length: %d!\n", len);
		return;
	}
	ieee_oui = (buff[start+2] << 16) |
		(buff[start+1] << 8) |
		buff[start];
	if ((ieee_oui != 0x000C03) &&
		(ieee_oui != 0xC45DD8)) {
		*lenth += sprintf(buf + *lenth, "invalid IEEE OUI\n");
		return;
	} else if (ieee_oui == 0xC45DD8) {
		goto hf_vsdb;
	}
	edid_info->vsdb.ieee_oui = ieee_oui;
	/* phy addr: 2 bytes*/
	edid_info->vsdb.a = (buff[start+3] >> 4) & 0xf;
	edid_info->vsdb.b = buff[start+3] & 0xf;
	edid_info->vsdb.c = (buff[start+4] >> 4) & 0xf;
	edid_info->vsdb.d = buff[start+4] & 0xf;
	/* after first 5 bytes: vsdb1.4 extension fileds */
	if (len > 5) {
		edid_info->vsdb.support_AI = (buff[start+5] >> 7) & 0x1;
		edid_info->vsdb.DC_48bit = (buff[start+5] >> 6) & 0x1;
		edid_info->vsdb.DC_36bit = (buff[start+5] >> 5) & 0x1;
		edid_info->vsdb.DC_30bit = (buff[start+5] >> 4) & 0x1;
		edid_info->vsdb.DC_y444 = (buff[start+5] >> 3) & 0x1;
		edid_info->vsdb.dvi_dual = buff[start+5] & 0x1;
	}
	if (len > 6)
		edid_info->vsdb.max_tmds_clk = buff[start+6];
	if (len > 7) {
		edid_info->vsdb.latency_fields_present =
			(buff[start+7] >> 7) & 0x1;
		edid_info->vsdb.i_latency_fields_present =
			(buff[start+7] >> 6) & 0x1;
		edid_info->vsdb.hdmi_video_present =
			(buff[start+7] >> 5) & 0x1;
		edid_info->vsdb.cnc3 = (buff[start+7] >> 3) & 0x1;
		edid_info->vsdb.cnc2 = (buff[start+7] >> 2) & 0x1;
		edid_info->vsdb.cnc1 = (buff[start+7] >> 1) & 0x1;
		edid_info->vsdb.cnc0 = buff[start+7] & 0x1;
	}
	if (edid_info->vsdb.latency_fields_present) {
		if (len < 10) {
			*lenth += sprintf(buf + *lenth, "invalid vsdb len for latency: %d\n", len);
			return;
		}
		edid_info->vsdb.video_latency = buff[start+8];
		edid_info->vsdb.audio_latency = buff[start+9];
		_3d_present_offset = 10;
	} else {
		*lenth += sprintf(buf + *lenth, "latency fields not present\n");
		_3d_present_offset = 8;
	}
	if (edid_info->vsdb.i_latency_fields_present) {
		/* I_Latency_Fields_Present shall be zero
		 * if Latency_Fields_Present is zero.
		 */
		if (edid_info->vsdb.latency_fields_present)
			*lenth += sprintf(buf + *lenth, "i_latency_fields should not be set\n");
		else if (len < 12) {
			*lenth += sprintf(buf + *lenth, "invalid vsdb len for i_latency: %d\n", len);
			return;
		}
		edid_info->vsdb.interlaced_video_latency
			= buff[start+10];
		edid_info->vsdb.interlaced_audio_latency
			= buff[start+11];
		_3d_present_offset = 12;
	}
	/* HDMI_Video_present: If set then additional video format capabilities
	 * are described by using the fields starting after the Latency
	 * area. This consists of 4 parts with the order described below:
	 * 1 byte containing the 3D_present flag and other flags
	 * 1 byte with length fields HDMI_VIC_LEN and HDMI_3D_LEN
	 * zero or more bytes for information about HDMI_VIC formats supported
	 * (length of this field is indicated by HDMI_VIC_LEN).
	 * zero or more bytes for information about 3D formats supported
	 * (length of this field is indicated by HDMI_3D_LEN)
	 */
	if (edid_info->vsdb.hdmi_video_present) {
		/* if hdmi video present,
		 * 2 additonal bytes at least will present
		 * 1 byte containing the 3D_present flag and other flags
		 * 1 byte with length fields HDMI_VIC_LEN and HDMI_3D_LEN
		 * 0 or more bytes for info about HDMI_VIC formats supported
		 * 0 or more bytes for info about 3D formats supported
		 */
		if (len < _3d_present_offset + 2) {
			*lenth += sprintf(buf + *lenth, "invalid vsdb length for hdmi video: %d\n", len);
			return;
		}
		edid_info->vsdb._3d_present =
			(buff[start+_3d_present_offset] >> 7) & 0x1;
		edid_info->vsdb._3d_multi_present =
			(buff[start+_3d_present_offset] >> 5) & 0x3;
		edid_info->vsdb.image_size =
			(buff[start+_3d_present_offset] >> 3) & 0x3;
		edid_info->vsdb.hdmi_vic_len =
			(buff[start+_3d_present_offset+1] >> 5) & 0x7;
		edid_info->vsdb.hdmi_3d_len =
			(buff[start+_3d_present_offset+1]) & 0x1f;
		/* parse 4k2k video format, 4 4k2k format maximum*/
		hdmi_vic_offset = _3d_present_offset + 2;
		hdmi_vic_len = edid_info->vsdb.hdmi_vic_len;
		if (hdmi_vic_len > 4) {
			*lenth += sprintf(buf + *lenth, "invalid hdmi vic len: %d\n",
				edid_info->vsdb.hdmi_vic_len);
			return;
		}

		/* HDMI_VIC_LEN may be 0 */
		if (len < hdmi_vic_offset + hdmi_vic_len) {
			*lenth += sprintf(buf + *lenth, "invalid length for 4k2k: %d\n", len);
			return;
		}
		for (i = 0; i < hdmi_vic_len; i++) {
			if (buff[start+hdmi_vic_offset+i] == 1)
				edid_info->vsdb.hdmi_4k2k_30hz_sup = 1;
			else if (buff[start+hdmi_vic_offset+i] == 2)
				edid_info->vsdb.hdmi_4k2k_25hz_sup = 1;
			else if (buff[start+hdmi_vic_offset+i] == 3)
				edid_info->vsdb.hdmi_4k2k_24hz_sup = 1;
			else if (buff[start+hdmi_vic_offset+i] == 4)
				edid_info->vsdb.hdmi_smpte_sup = 1;
		}

		/* 3D info parse */
		_3d_struct_all_offset =
			hdmi_vic_offset + hdmi_vic_len;
		hdmi_3d_len = edid_info->vsdb.hdmi_3d_len;
		/* there may be additional 0 present after 3D info  */
		if (len < _3d_struct_all_offset + hdmi_3d_len) {
			*lenth += sprintf(buf + *lenth, "invalid vsdb length for 3d: %d\n", len);
			return;
		}
		/* 3d_multi_present: hdmi1.4 spec page155:
		 * 0: neither structure or mask present,
		 * 1: only 3D_Structure_ALL_15 is present
		 *    and assigns 3D formats to all of the
		 *    VICs listed in the first 16 entries
		 *    in the EDID
		 * 2: 3D_Structure_ALL_15 and 3D_MASK_15
		 *    are present and assign 3D formats to
		 *    some of the VICs listed in the first
		 *    16 entries in the EDID.
		 * 3: neither structure or mask present,
		 *    Reserved for future use.
		 */
		if (edid_info->vsdb._3d_multi_present == 1) {
			edid_info->vsdb._3d_struct_all =
				(buff[start+_3d_struct_all_offset] << 8) +
				buff[start+_3d_struct_all_offset+1];
			_2d_vic_order_offset =
				_3d_struct_all_offset + 2;
			temp_3d_len = 2;
		} else if (edid_info->vsdb._3d_multi_present == 2) {
			edid_info->vsdb._3d_struct_all =
				(buff[start+_3d_struct_all_offset] << 8) +
				buff[start+_3d_struct_all_offset+1];
			edid_info->vsdb._3d_mask_15_0 =
				(buff[start+_3d_struct_all_offset+2] << 8) +
				buff[start+_3d_struct_all_offset+3];
			_2d_vic_order_offset =
				_3d_struct_all_offset + 4;
			temp_3d_len = 4;
		} else {
			_2d_vic_order_offset =
				_3d_struct_all_offset;
			temp_3d_len = 0;
		}
		i = _2d_vic_order_offset;
		for (j = 0; (temp_3d_len < hdmi_3d_len)
			&& (j < 16); j++) {
			edid_info->vsdb._2d_vic[j]._2d_vic_order =
				(buff[start+i] >> 4) & 0xF;
			edid_info->vsdb._2d_vic[j]._3d_struct =
				buff[start+i] & 0xF;
			_3d_struct =
				edid_info->vsdb._2d_vic[j]._3d_struct;
			/* hdmi1.4 spec page156
			 * if 3D_Structure_X is 0000~0111,
			 * 3D_Detail_X shall not be present,
			 * otherwise shall be present
			 */
			if ((_3d_struct	>= 0x8) &&
				(_3d_struct <= 0xF)) {
				edid_info->vsdb._2d_vic[j]._3d_detail =
					(buff[start+i+1] >> 4) & 0xF;
				i += 2;
				temp_3d_len += 2;
				if (temp_3d_len > hdmi_3d_len) {
					*lenth += sprintf(buf + *lenth, "invalid len for 3d_detail: %d\n",
						len);
					break;
				}
			} else {
				i++;
				temp_3d_len++;
			}
		}
		edid_info->vsdb._2d_vic_num = j;
		return;
	}
hf_vsdb:
	/* hdmi spec2.0 Table10-6 */
	if (len < 7) {
		*lenth += sprintf(buf + *lenth, "invalid HF_VSDB length: %d!\n", len);
		return;
	}
	edid_info->contain_hf_vsdb = true;
	edid_info->hf_vsdb.ieee_oui = ieee_oui;
	edid_info->hf_vsdb.version =
		buff[start+3];
	edid_info->hf_vsdb.max_tmds_rate =
		buff[start+4];
	edid_info->hf_vsdb.scdc_present =
		(buff[start+5] >> 7) & 0x1;
	edid_info->hf_vsdb.rr_cap =
		(buff[start+5] >> 6) & 0x1;
	edid_info->hf_vsdb.lte_340m_scramble =
		(buff[start+5] >> 3) & 0x1;
	edid_info->hf_vsdb.independ_view =
		(buff[start+5] >> 2) & 0x1;
	edid_info->hf_vsdb.dual_view =
		(buff[start+5] >> 1) & 0x1;
	edid_info->hf_vsdb._3d_osd_disparity =
		buff[start+5] & 0x1;

	edid_info->hf_vsdb.dc_48bit_420 =
		(buff[start+6] >> 2) & 0x1;
	edid_info->hf_vsdb.dc_36bit_420 =
		(buff[start+6] >> 1) & 0x1;
	edid_info->hf_vsdb.dc_30bit_420 =
		buff[start+6] & 0x1;
}

static void get_edid_vcdb(unsigned char *buff, unsigned char start,
	 unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth) {
	 /* vcdb only contain 3 bytes data block. the source should
	  * ignore additional bytes (when present) and continue to
	  * parse the single byte as defined in CEA861-F Table 59.
	  */
	if (len != 2-1) {
		*lenth += sprintf(buf + *lenth, "invalid length for video cap data blcok: %d!\n", len);
		/* return; */
	}
	edid_info->contain_vcdb = true;
	edid_info->vcdb.quanti_range_ycc = (buff[start] >> 7) & 0x1;
	edid_info->vcdb.quanti_range_rgb = (buff[start] >> 6) & 0x1;
	edid_info->vcdb.s_PT = (buff[start] >> 4) & 0x3;
	edid_info->vcdb.s_IT = (buff[start] >> 2) & 0x3;
	edid_info->vcdb.s_CE = buff[start] & 0x3;
}

static void get_edid_dv_data(unsigned char *buff, unsigned char start,
	 unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	unsigned int ieee_oui;

	if ((len != 0xE - 1) &&
		(len != 0x19 - 1)) {
		*lenth += sprintf(buf + *lenth, "invalid length for dolby vision vsvdb:%d\n",
			len);
		return;
	}
	ieee_oui = (buff[start+2] << 16) |
		(buff[start+1] << 8) |
		buff[start];
	if (ieee_oui != 0x00D046) {
		*lenth += sprintf(buf + *lenth, "invalid dolby vision ieee oui\n");
		return;
	}
	edid_info->contain_vsvdb = true;
	edid_info->dv_vsvdb.ieee_oui = ieee_oui;
	edid_info->dv_vsvdb.version = buff[start+3] >> 5;
	if (edid_info->dv_vsvdb.version == 0x0) {
		/* length except extend code */
		if (len != 0x19 - 1) {
			*lenth += sprintf(buf + *lenth, "invalid length for dolby vision ver0\n");
			return;
		}
		edid_info->dv_vsvdb.sup_global_dimming =
			(buff[start+3] >> 2) & 0x1;
		edid_info->dv_vsvdb.sup_2160p60hz =
			(buff[start+3] >> 1) & 0x1;
		edid_info->dv_vsvdb.sup_yuv422_12bit =
			buff[start+3] & 0x1;
		edid_info->dv_vsvdb.Rx =
			((buff[start+4] >> 4) & 0xF) + (buff[start+5] << 4);
		edid_info->dv_vsvdb.Ry =
			(buff[start+4] & 0xF) + (buff[start+6] << 4);
		edid_info->dv_vsvdb.Gx =
			((buff[start+7] >> 4) & 0xF) + (buff[start+8] << 4);
		edid_info->dv_vsvdb.Gy =
			(buff[start+7] & 0xF) + (buff[start+9] << 4);
		edid_info->dv_vsvdb.Bx =
			((buff[start+10] >> 4) & 0xF) + (buff[start+11] << 4);
		edid_info->dv_vsvdb.By =
			(buff[start+10] & 0xF) + (buff[start+12] << 4);
		edid_info->dv_vsvdb.Wx =
			((buff[start+13] >> 4) & 0xF) + (buff[start+14] << 4);
		edid_info->dv_vsvdb.Wy =
			(buff[start+13] & 0xF) + (buff[start+15] << 4);
		edid_info->dv_vsvdb.tminPQ =
			((buff[start+16] >> 4) & 0xF) + (buff[start+17] << 4);
		edid_info->dv_vsvdb.tmaxPQ =
			(buff[start+16] & 0xF) + (buff[start+18] << 4);
		edid_info->dv_vsvdb.dm_major_ver =
			(buff[start+19] >> 4) & 0xF;
		edid_info->dv_vsvdb.dm_minor_ver =
			buff[start+19] & 0xF;
	} else if (edid_info->dv_vsvdb.version == 0x1) {
		/*length except extend code*/
		if (len != 0xE - 1) {
			*lenth += sprintf(buf + *lenth, "invalid length for dolby vision ver1\n");
			return;
		}
		edid_info->dv_vsvdb.DM_version =
			(buff[start+3] >> 2) & 0x7;
		edid_info->dv_vsvdb.sup_2160p60hz =
			(buff[start+3] >> 1) & 0x1;
		edid_info->dv_vsvdb.sup_yuv422_12bit =
			buff[start+3] & 1;
		edid_info->dv_vsvdb.target_max_lum =
			(buff[start+4] >> 1) & 0x7F;
		edid_info->dv_vsvdb.sup_global_dimming =
			buff[start+4] & 0x1;
		edid_info->dv_vsvdb.target_min_lum =
			(buff[start+5] >> 1) & 0x7F;
		edid_info->dv_vsvdb.colormetry =
			buff[start+5] & 0x1;
		edid_info->dv_vsvdb.Rx = buff[start+7];
		edid_info->dv_vsvdb.Ry = buff[start+8];
		edid_info->dv_vsvdb.Gx = buff[start+9];
		edid_info->dv_vsvdb.Gy = buff[start+10];
		edid_info->dv_vsvdb.Bx = buff[start+11];
		edid_info->dv_vsvdb.By = buff[start+12];
	}
}

static void get_edid_colorimetry_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth) {
	/* colorimetry DB only contain 3 bytes data block */
	if (len != 3-1) {
		*lenth += sprintf(buf + *lenth, "invalid length for colorimetry data block:%d\n",
			len);
		return;
	}
	edid_info->contain_cdb = true;
	edid_info->color_db.BT2020_RGB = (buff[start] >> 7) & 0x1;
	edid_info->color_db.BT2020_YCC = (buff[start] >> 6) & 0x1;
	edid_info->color_db.BT2020_cYCC = (buff[start] >> 5) & 0x1;
	edid_info->color_db.Adobe_RGB = (buff[start] >> 4) & 0x1;
	edid_info->color_db.Adobe_YCC601 = (buff[start] >> 3) & 0x1;
	edid_info->color_db.sYCC601 = (buff[start] >> 2) & 0x1;
	edid_info->color_db.xvYCC709 = (buff[start] >> 1) & 0x1;
	edid_info->color_db.xvYCC601 = buff[start] & 0x1;

	edid_info->color_db.MD3 = (buff[start+1] >> 3) & 0x1;
	edid_info->color_db.MD2 = (buff[start+1] >> 2) & 0x1;
	edid_info->color_db.MD1 = (buff[start+1] >> 1) & 0x1;
	edid_info->color_db.MD0 = buff[start+1] & 0x1;
}

static void get_hdr_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	/* Bytes 5-7 are optional to declare. 3 bytes payload at least */
	if (len < 3-1) {
		*lenth += sprintf(buf + *lenth, "invalid hdr length: %d!\n", len);
		return;
	}
	edid_info->contain_hdr_db = true;
	edid_info->hdr_db.eotf_hlg = (buff[start] >> 3) & 0x1;
	edid_info->hdr_db.eotf_smpte_st_2084 = (buff[start] >> 2) & 0x1;
	edid_info->hdr_db.eotf_hdr = (buff[start] >> 1) & 0x1;
	edid_info->hdr_db.eotf_sdr = buff[start] & 0x1;
	edid_info->hdr_db.hdr_SMD_type1 =  buff[start+1] & 0x1;
}

static void get_edid_y420_vid_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	int i;

	if (len > 6) {
		*lenth += sprintf(buf + *lenth, "y420vdb support only 4K50/60hz, smpte50/60hz, len:%d\n",
			len);
		return;
	}
	edid_info->contain_y420_vdb = true;
	edid_info->y420_vic_len = len;
	for (i = 0; i < len; i++)
		edid_info->y420_vdb_vic[i] = buff[start+i];
}

static void get_edid_y420_cap_map_data(unsigned char *buff, unsigned char start,
	unsigned char len, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	unsigned int i, bit_map = 0;
	/* 31 SVD maxmium, 4 bytes needed */
	if (len > 4) {
		*lenth += sprintf(buf + *lenth, "31 SVD maxmium, all-zero data not needed\n");
		len = 4;
	}
	edid_info->contain_y420_cmdb = true;
	/* When the Length field is set to L==1, the Y420CMDB does not
	 * include a YCBCR 4:2:0 Capability Bit Map and all the SVDs in
	 * the regular Video Data Block support YCBCR 4:2:0 samplin mode.
	 */
	if (len == 0) {
		edid_info->y420_all_vic = 1;
		return;
	}
	/* bit0: first SVD, bit 1:the second SVD, and so on */
	for (i = 0; i < len; i++)
		bit_map |= (buff[start+i] << (i*8));
	/* '1' bit in the bit map indicate corresponding SVD support Y420 */
	for (i = 0; (i < len*8) && (i < 31); i++) {
		if ((bit_map >> i) & 0x1) {
			edid_info->y420_cmdb_vic[i] =
				edid_info->video_db.hdmi_vic[i];
		}
	}
}

/* parse CEA extension block */
void HDMIRx_Edid_Parse_Cea_Block(uint8_t *p_edid, struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	/* unsigned int audio_block; */
	unsigned int max_offset;
	unsigned int tag_offset;
	unsigned char tag_data;
	unsigned char tag_code;
	unsigned char extend_tag_code;
	unsigned char data_blk_len;

	if (!p_edid || !edid_info)
		return;
	if (p_edid[0] != 0x02) {
		*lenth += sprintf(buf + *lenth, "not a valid CEA block!\n");
		return;
	}
	edid_info->cea_tag = p_edid[0];
	edid_info->cea_revision = p_edid[1];
	max_offset = p_edid[2];
	edid_info->dtd_offset = max_offset;
	edid_info->underscan_sup = (p_edid[3] >> 7) & 0x1;
	edid_info->basic_aud_sup = (p_edid[3] >> 6) & 0x1;
	edid_info->ycc444_sup = (p_edid[3] >> 5) & 0x1;
	edid_info->ycc422_sup = (p_edid[3] >> 4) & 0x1;
	edid_info->native_dtd_num = p_edid[3] & 0xF;

	tag_offset = 4;/* block offset */
	do {
		tag_data = p_edid[tag_offset];
		tag_code = (tag_data & 0xE0) >> 5;
		/* data block playload length */
		data_blk_len = tag_data & 0x1F;
		/* length beyond max offset, force to break */
		if ((tag_offset + data_blk_len) >= max_offset)
			break;
		switch (tag_code) {
		/* data block playload offset: tag_offset+1
		 * length: payload length
		 */
		case VIDEO_TAG:
			get_edid_video_data(p_edid, tag_offset+1,
				data_blk_len, edid_info);
			break;
		case AUDIO_TAG:
			get_edid_audio_data(p_edid, tag_offset+1,
				data_blk_len, edid_info);
			break;
		case SPEAKER_TAG:
			get_edid_speaker_data(p_edid, tag_offset+1,
				data_blk_len, edid_info, buf, lenth);
			break;
		case VENDOR_TAG:
			get_edid_vsdb(p_edid, tag_offset+1,
				data_blk_len, edid_info, buf, lenth);
			break;
		case VESA_TAG:
			break;
		case USE_EXTENDED_TAG:
			extend_tag_code = p_edid[tag_offset+1];
			switch (extend_tag_code) {
			/* offset: start after extended tag code
			 * length: payload length except extend tag
			 */
			case VCDB_TAG:
				get_edid_vcdb(p_edid, tag_offset+2,
					data_blk_len-1, edid_info, buf, lenth);
				break;
			case VSVDB_TAG:
				get_edid_dv_data(p_edid, tag_offset+2,
					data_blk_len-1, edid_info, buf, lenth);
				break;
			case CDB_TAG:
				get_edid_colorimetry_data(p_edid, tag_offset+2,
					data_blk_len-1, edid_info, buf, lenth);
				break;
			case HDR_TAG:
				get_hdr_data(p_edid, tag_offset+2,
					data_blk_len-1, edid_info, buf, lenth);
				break;
			case Y420VDB_TAG:
				get_edid_y420_vid_data(p_edid, tag_offset+2,
					data_blk_len-1, edid_info, buf, lenth);
				break;
			case Y420CMDB_TAG:
				get_edid_y420_cap_map_data(p_edid, tag_offset+2,
					data_blk_len-1, edid_info, buf, lenth);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		/* next tag offset */
		tag_offset += ((tag_data & 0x1F)+1);
	} while (tag_offset < max_offset);
}

void HDMIRx_Edid_Parse_Print(struct edid_info_s *edid_info, char *buf, ssize_t *lenth)
{
	unsigned char i;
	unsigned char hdmi_vic;
	enum edid_audio_format_e fmt;
	union bit_rate_u *bit_rate;
	unsigned char svd_num;
	unsigned char _2d_vic_order;
	unsigned char _3d_struct;
	unsigned char _3d_detail;

	if (!edid_info)
		return;
	*lenth += sprintf(buf + *lenth, "****EDID Basic Block****\n");
	*lenth += sprintf(buf + *lenth, "manufacturer_name: %s\n", edid_info->manufacturer_name);
	*lenth += sprintf(buf + *lenth, "product code: 0x%04x\n", edid_info->product_code);
	*lenth += sprintf(buf + *lenth, "serial_number: 0x%08x\n", edid_info->serial_number);
	*lenth += sprintf(buf + *lenth, "product week: %d\n", edid_info->product_week);
	*lenth += sprintf(buf + *lenth, "product year: %d\n", edid_info->product_year);
	*lenth += sprintf(buf + *lenth, "descriptor1.hactive: %d\n", edid_info->descriptor1.hactive);
	*lenth += sprintf(buf + *lenth, "descriptor1.vactive: %d\n", edid_info->descriptor1.vactive);
	*lenth += sprintf(buf + *lenth, "descriptor1.pix clk: %dKHz\n",
		edid_info->descriptor1.pixel_clk*10);
	*lenth += sprintf(buf + *lenth, "monitor name: %s\n", edid_info->monitor_name);
	*lenth += sprintf(buf + *lenth, "max support pixel clock: %dMhz\n",
		edid_info->max_sup_pixel_clk);
	*lenth += sprintf(buf + *lenth, "extension_flag: %d\n", edid_info->extension_flag);
	*lenth += sprintf(buf + *lenth, "block0_chk_sum: 0x%x\n", edid_info->block0_chk_sum);

	*lenth += sprintf(buf + *lenth, "****CEA block header****\n");
	*lenth += sprintf(buf + *lenth, "underscan_sup: %d\n", edid_info->underscan_sup);
	*lenth += sprintf(buf + *lenth, "basic_aud_sup: %d\n", edid_info->basic_aud_sup);
	*lenth += sprintf(buf + *lenth, "ycc444_sup: %d\n", edid_info->ycc444_sup);
	*lenth += sprintf(buf + *lenth, "ycc422_sup: %d\n", edid_info->ycc422_sup);
	*lenth += sprintf(buf + *lenth, "native_dtd_num: %d\n", edid_info->native_dtd_num);

	*lenth += sprintf(buf + *lenth, "****Video Data Block****\n");
	*lenth += sprintf(buf + *lenth, "support SVD list:\n");
	svd_num = edid_info->video_db.svd_num;
	for (i = 0; i < edid_info->video_db.svd_num; i++) {
		hdmi_vic = edid_info->video_db.hdmi_vic[i];
		rx_edid_print_vic_fmt(i, hdmi_vic, buf, lenth);
	}

	*lenth += sprintf(buf + *lenth, "****Audio Data Block****\n");
	for (fmt = AUDIO_FORMAT_LPCM; fmt <= AUDIO_FORMAT_WMAPRO; fmt++) {
		if (edid_info->audio_db.aud_fmt_sup[fmt]) {
			*lenth += sprintf(buf + *lenth, "audio fmt: %s\n", aud_fmt[fmt]);
			*lenth += sprintf(buf + *lenth, "\tmax channel: %d\n",
				edid_info->audio_db.sad[fmt].max_channel+1);
			if (edid_info->audio_db.sad[fmt].freq_192khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_192khz\n");
			if (edid_info->audio_db.sad[fmt].freq_176_4khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_176.4khz\n");
			if (edid_info->audio_db.sad[fmt].freq_96khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_96khz\n");
			if (edid_info->audio_db.sad[fmt].freq_88_2khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_88.2khz\n");
			if (edid_info->audio_db.sad[fmt].freq_48khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_48khz\n");
			if (edid_info->audio_db.sad[fmt].freq_44_1khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_44.1khz\n");
			if (edid_info->audio_db.sad[fmt].freq_32khz)
				*lenth += sprintf(buf + *lenth, "\tfreq_32khz\n");
			bit_rate = &(edid_info->audio_db.sad[fmt].bit_rate);
			if (fmt == AUDIO_FORMAT_LPCM) {
				*lenth += sprintf(buf + *lenth, "sample size:\n");
				if (bit_rate->pcm.size_16bit)
					*lenth += sprintf(buf + *lenth, "\t16bit\n");
				if (bit_rate->pcm.size_20bit)
					*lenth += sprintf(buf + *lenth, "\t20bit\n");
				if (bit_rate->pcm.size_24bit)
					*lenth += sprintf(buf + *lenth, "\t24bit\n");
			} else if ((fmt >= AUDIO_FORMAT_AC3) &&
				(fmt <= AUDIO_FORMAT_ATRAC)) {
				*lenth += sprintf(buf + *lenth, "\tmax bit rate: %dkHz\n",
					bit_rate->others*8);
			} else {
				*lenth += sprintf(buf + *lenth, "\tformat dependent value: 0x%x\n",
					bit_rate->others);
			}
		}
	}

	*lenth += sprintf(buf + *lenth, "****Speaker Allocation Data Block****\n");
	if (edid_info->speaker_alloc.flw_frw)
		*lenth += sprintf(buf + *lenth, "FLW/FRW\n");
	if (edid_info->speaker_alloc.rlc_rrc)
		*lenth += sprintf(buf + *lenth, "RLC/RRC\n");
	if (edid_info->speaker_alloc.flc_frc)
		*lenth += sprintf(buf + *lenth, "FLC/FRC\n");
	if (edid_info->speaker_alloc.rc)
		*lenth += sprintf(buf + *lenth, "RC\n");
	if (edid_info->speaker_alloc.rl_rr)
		*lenth += sprintf(buf + *lenth, "RL/RR\n");
	if (edid_info->speaker_alloc.fc)
		*lenth += sprintf(buf + *lenth, "FC\n");
	if (edid_info->speaker_alloc.lfe)
		*lenth += sprintf(buf + *lenth, "LFE\n");
	if (edid_info->speaker_alloc.fl_fr)
		*lenth += sprintf(buf + *lenth, "FL/FR\n");
	if (edid_info->speaker_alloc.fch)
		*lenth += sprintf(buf + *lenth, "FCH\n");
	if (edid_info->speaker_alloc.tc)
		*lenth += sprintf(buf + *lenth, "TC\n");
	if (edid_info->speaker_alloc.flh_frh)
		*lenth += sprintf(buf + *lenth, "FLH_FRH\n");

	*lenth += sprintf(buf + *lenth, "****Vender Specific Data Block****\n");
	*lenth += sprintf(buf + *lenth, "IEEE OUI: %06X\n",
		edid_info->vsdb.ieee_oui);
	*lenth += sprintf(buf + *lenth, "phy addr: %d.%d.%d.%d\n",
		edid_info->vsdb.a, edid_info->vsdb.b,
		edid_info->vsdb.c, edid_info->vsdb.d);
	if (edid_info->vsdb.support_AI)
		*lenth += sprintf(buf + *lenth, "support AI\n");
	*lenth += sprintf(buf + *lenth, "support deep clor:\n");
	if (edid_info->vsdb.DC_48bit)
		*lenth += sprintf(buf + *lenth, "\t16bit\n");
	if (edid_info->vsdb.DC_36bit)
		*lenth += sprintf(buf + *lenth, "\t12bit\n");
	if (edid_info->vsdb.DC_30bit)
		*lenth += sprintf(buf + *lenth, "\t10bit\n");
	if (edid_info->vsdb.dvi_dual)
		*lenth += sprintf(buf + *lenth, "support dvi dual channel\n");
	if (edid_info->vsdb.max_tmds_clk > 0)
		*lenth += sprintf(buf + *lenth, "max tmds clk supported: %dMHz\n",
			edid_info->vsdb.max_tmds_clk*5);
	*lenth += sprintf(buf + *lenth, "hdmi_video_present: %d\n",
		edid_info->vsdb.hdmi_video_present);
	*lenth += sprintf(buf + *lenth, "Content types:\n");
	if (edid_info->vsdb.cnc3)
		*lenth += sprintf(buf + *lenth, "\tcnc3: Game\n");
	if (edid_info->vsdb.cnc2)
		*lenth += sprintf(buf + *lenth, "\tcnc2: Cinema\n");
	if (edid_info->vsdb.cnc1)
		*lenth += sprintf(buf + *lenth, "\tcnc1: Photo\n");
	if (edid_info->vsdb.cnc0)
		*lenth += sprintf(buf + *lenth, "\tcnc0: Grahpics(text)\n");

	if (edid_info->vsdb.hdmi_vic_len > 0)
		*lenth += sprintf(buf + *lenth, "Supproted 4k2k format:\n");
	if (edid_info->vsdb.hdmi_4k2k_30hz_sup)
		*lenth += sprintf(buf + *lenth, "\thdmi vic1: 4k30hz\n");
	if (edid_info->vsdb.hdmi_4k2k_25hz_sup)
		*lenth += sprintf(buf + *lenth, "\thdmi vic2: 4k25hz\n");
	if (edid_info->vsdb.hdmi_4k2k_24hz_sup)
		*lenth += sprintf(buf + *lenth, "\thdmi vic3: 4k24hz\n");
	if (edid_info->vsdb.hdmi_smpte_sup)
		*lenth += sprintf(buf + *lenth, "\thdmi vic4: smpte\n");
	/* Mandatory 3D format: HDMI1.4 spec page157 */
	if (edid_info->vsdb._3d_present) {
		*lenth += sprintf(buf + *lenth, "Basic(Mandatory) 3D formats supported\n");
		*lenth += sprintf(buf + *lenth, "Image Size:\n");
		switch (edid_info->vsdb.image_size) {
		case 0:
			*lenth += sprintf(buf + *lenth, "\tNo additional information\n");
			break;
		case 1:
			*lenth += sprintf(buf + *lenth, "\tOnly indicate correct aspect ratio\n");
			break;
		case 2:
			*lenth += sprintf(buf + *lenth, "\tCorrect size: Accurate to 1(cm)\n");
			break;
		case 3:
			*lenth += sprintf(buf + *lenth, "\tCorrect size: multiply by 5(cm)\n");
			break;
		default:
			break;
		}
	} else
		*lenth += sprintf(buf + *lenth, "No 3D support\n");
	if (edid_info->vsdb._3d_multi_present == 1) {
		/* For each bit in _3d_struct which is set (=1),
		 * Sink supports the corresponding 3D_Structure
		 * for all of the VICs listed in the first 16
		 * entries in the EDID.
		 */
		*lenth += sprintf(buf + *lenth, "General 3D format, on the first 16 SVDs:\n");
		for (i = 0; i < 16; i++) {
			if ((edid_info->vsdb._3d_struct_all >> i) & 0x1)
				*lenth += sprintf(buf + *lenth, "\t%s\n",	_3d_structure[i]);
		}
	} else if (edid_info->vsdb._3d_multi_present == 2) {
		/* Where a bit is set (=1), for the corresponding
		 * VIC within the first 16 entries in the EDID,
		 * the Sink indicates 3D support as designate
		 * by the 3D_Structure_ALL_15 field.
		 */
		*lenth += sprintf(buf + *lenth, "General 3D format, on the SVDs below:\n");
		for (i = 0; i < 16; i++) {
			if ((edid_info->vsdb._3d_struct_all >> i) & 0x1)
				*lenth += sprintf(buf + *lenth, "\t%s\n",	_3d_structure[i]);
		}
		*lenth += sprintf(buf + *lenth, "For SVDs:\n");
		for (i = 0; (i < svd_num) && (i < 16); i++) {
			hdmi_vic = edid_info->video_db.hdmi_vic[i];
			if ((edid_info->vsdb._3d_mask_15_0 >> i) & 0x1)
				rx_edid_print_vic_fmt(i, hdmi_vic, buf, lenth);
		}
	}

	if (edid_info->vsdb._2d_vic_num > 0)
		*lenth += sprintf(buf + *lenth, "Specific VIC 3D information:\n");
	for (i = 0; (i < edid_info->vsdb._2d_vic_num)
		&& (i < svd_num) && (i < 16); i++) {
		_2d_vic_order =
			edid_info->vsdb._2d_vic[i]._2d_vic_order;
		_3d_struct =
			edid_info->vsdb._2d_vic[i]._3d_struct;
		hdmi_vic =
			edid_info->video_db.hdmi_vic[_2d_vic_order];
		rx_edid_print_vic_fmt(_2d_vic_order, hdmi_vic, buf, lenth);
		*lenth += sprintf(buf + *lenth, "\t\t3d format: %s\n",
			_3d_structure[_3d_struct]);
		if ((_3d_struct >= 0x8) &&
			(_3d_struct <= 0xF)) {
			_3d_detail =
				edid_info->vsdb._2d_vic[i]._3d_detail;
			*lenth += sprintf(buf + *lenth, "\t\t3d_detail: %s\n",
				_3d_detail_x[_3d_detail]);
		}
	}

	if (edid_info->contain_hf_vsdb) {
		*lenth += sprintf(buf + *lenth, "****HF-VSDB****\n");
		*lenth += sprintf(buf + *lenth, "IEEE OUI: %06X\n",
			edid_info->hf_vsdb.ieee_oui);
		*lenth += sprintf(buf + *lenth, "hf-vsdb version: %d\n",
			edid_info->hf_vsdb.version);
		*lenth += sprintf(buf + *lenth, "max_tmds_rate: %dMHz\n",
			edid_info->hf_vsdb.max_tmds_rate*5);
		*lenth += sprintf(buf + *lenth, "scdc_present: %d\n",
			edid_info->hf_vsdb.scdc_present);
		*lenth += sprintf(buf + *lenth, "rr_cap: %d\n",
			edid_info->hf_vsdb.rr_cap);
		*lenth += sprintf(buf + *lenth, "lte_340m_scramble: %d\n",
			edid_info->hf_vsdb.lte_340m_scramble);
		*lenth += sprintf(buf + *lenth, "independ_view: %d\n",
			edid_info->hf_vsdb.independ_view);
		*lenth += sprintf(buf + *lenth, "dual_view: %d\n",
			edid_info->hf_vsdb.dual_view);
		*lenth += sprintf(buf + *lenth, "_3d_osd_disparity: %d\n",
			edid_info->hf_vsdb._3d_osd_disparity);

		*lenth += sprintf(buf + *lenth, "48bit 420 endode: %d\n",
			edid_info->hf_vsdb.dc_48bit_420);
		*lenth += sprintf(buf + *lenth, "36bit 420 endode: %d\n",
			edid_info->hf_vsdb.dc_36bit_420);
		*lenth += sprintf(buf + *lenth, "30bit 420 endode: %d\n",
			edid_info->hf_vsdb.dc_30bit_420);
	}

	if (edid_info->contain_vcdb) {
		*lenth += sprintf(buf + *lenth, "****Video Cap Data Block****\n");
		*lenth += sprintf(buf + *lenth, "YCC Quant Range:\n");
		if (edid_info->vcdb.quanti_range_ycc)
			*lenth += sprintf(buf + *lenth, "\tSelectable(via AVI YQ)\n");
		else
			*lenth += sprintf(buf + *lenth, "\tNo Data\n");

		*lenth += sprintf(buf + *lenth, "RGB Quant Range:\n");
		if (edid_info->vcdb.quanti_range_rgb)
			*lenth += sprintf(buf + *lenth, "\tSelectable(via AVI Q)\n");
		else
			*lenth += sprintf(buf + *lenth, "\tNo Data\n");

		*lenth += sprintf(buf + *lenth, "PT Scan behavior:\n");
		switch (edid_info->vcdb.s_PT) {
		case 0:
			*lenth += sprintf(buf + *lenth, "\trefer to CE/IT fields\n");
			break;
		case 1:
			*lenth += sprintf(buf + *lenth, "\tAlways Overscanned\n");
			break;
		case 2:
			*lenth += sprintf(buf + *lenth, "\tAlways Underscanned\n");
			break;
		case 3:
			*lenth += sprintf(buf + *lenth, "\tSupport both over and underscan\n");
			break;
		default:
			break;
		}
		*lenth += sprintf(buf + *lenth, "IT Scan behavior:\n");
		switch (edid_info->vcdb.s_IT) {
		case 0:
			*lenth += sprintf(buf + *lenth, "\tIT video format not support\n");
			break;
		case 1:
			*lenth += sprintf(buf + *lenth, "\tAlways Overscanned\n");
			break;
		case 2:
			*lenth += sprintf(buf + *lenth, "\tAlways Underscanned\n");
			break;
		case 3:
			*lenth += sprintf(buf + *lenth, "\tSupport both over and underscan\n");
			break;
		default:
			break;
		}
		*lenth += sprintf(buf + *lenth, "CE Scan behavior:\n");
		switch (edid_info->vcdb.s_CE) {
		case 0:
			*lenth += sprintf(buf + *lenth, "\tCE video format not support\n");
			break;
		case 1:
			*lenth += sprintf(buf + *lenth, "\tAlways Overscanned\n");
			break;
		case 2:
			*lenth += sprintf(buf + *lenth, "\tAlways Underscanned\n");
			break;
		case 3:
			*lenth += sprintf(buf + *lenth, "\tSupport both over and underscan\n");
			break;
		default:
			break;
		}
	}

	if (edid_info->contain_vsvdb) {
		*lenth += sprintf(buf + *lenth, "****VSVDB(dolby vision)****\n");
		*lenth += sprintf(buf + *lenth, "IEEE_OUI: %06X\n",
			edid_info->dv_vsvdb.ieee_oui);
		*lenth += sprintf(buf + *lenth, "vsvdb version: %d\n",
			edid_info->dv_vsvdb.version);
		*lenth += sprintf(buf + *lenth, "sup_global_dimming: %d\n",
			edid_info->dv_vsvdb.sup_global_dimming);
		*lenth += sprintf(buf + *lenth, "sup_2160p60hz: %d\n",
			edid_info->dv_vsvdb.sup_2160p60hz);
		*lenth += sprintf(buf + *lenth, "sup_yuv422_12bit: %d\n",
			edid_info->dv_vsvdb.sup_yuv422_12bit);
		*lenth += sprintf(buf + *lenth, "Rx: 0x%x\n", edid_info->dv_vsvdb.Rx);
		*lenth += sprintf(buf + *lenth, "Ry: 0x%x\n", edid_info->dv_vsvdb.Ry);
		*lenth += sprintf(buf + *lenth, "Gx: 0x%x\n", edid_info->dv_vsvdb.Gx);
		*lenth += sprintf(buf + *lenth, "Gy: 0x%x\n", edid_info->dv_vsvdb.Gy);
		*lenth += sprintf(buf + *lenth, "Bx: 0x%x\n", edid_info->dv_vsvdb.Bx);
		*lenth += sprintf(buf + *lenth, "By: 0x%x\n", edid_info->dv_vsvdb.By);
		if (edid_info->dv_vsvdb.version == 0) {
			*lenth += sprintf(buf + *lenth, "target max pq: 0x%x\n",
				edid_info->dv_vsvdb.tmaxPQ);
			*lenth += sprintf(buf + *lenth, "target min pq: 0x%x\n",
				edid_info->dv_vsvdb.tminPQ);
			*lenth += sprintf(buf + *lenth, "dm_major_ver: 0x%x\n",
				edid_info->dv_vsvdb.dm_major_ver);
			*lenth += sprintf(buf + *lenth, "dm_minor_ver: 0x%x\n",
				edid_info->dv_vsvdb.dm_minor_ver);
		} else if (edid_info->dv_vsvdb.version == 1) {
			*lenth += sprintf(buf + *lenth, "DM_version: 0x%x\n",
				edid_info->dv_vsvdb.DM_version);
			*lenth += sprintf(buf + *lenth, "target_max_lum: 0x%x\n",
				edid_info->dv_vsvdb.target_max_lum);
			*lenth += sprintf(buf + *lenth, "target_min_lum: 0x%x\n",
				edid_info->dv_vsvdb.target_min_lum);
			*lenth += sprintf(buf + *lenth, "colormetry: 0x%x\n",
				edid_info->dv_vsvdb.colormetry);
		}
	}

	if (edid_info->contain_cdb) {
		*lenth += sprintf(buf + *lenth, "****Colorimetry Data Block****\n");
		*lenth += sprintf(buf + *lenth, "supported colorimetry:\n");
		if (edid_info->color_db.BT2020_RGB)
			*lenth += sprintf(buf + *lenth, "\tBT2020_RGB\n");
		if (edid_info->color_db.BT2020_YCC)
			*lenth += sprintf(buf + *lenth, "\tBT2020_YCC\n");
		if (edid_info->color_db.BT2020_cYCC)
			*lenth += sprintf(buf + *lenth, "\tBT2020_cYCC\n");
		if (edid_info->color_db.Adobe_RGB)
			*lenth += sprintf(buf + *lenth, "\tAdobe_RGB\n");
		if (edid_info->color_db.Adobe_YCC601)
			*lenth += sprintf(buf + *lenth, "\tAdobe_YCC601\n");
		if (edid_info->color_db.sYCC601)
			*lenth += sprintf(buf + *lenth, "\tsYCC601\n");
		if (edid_info->color_db.xvYCC709)
			*lenth += sprintf(buf + *lenth, "\txvYCC709\n");
		if (edid_info->color_db.xvYCC601)
			*lenth += sprintf(buf + *lenth, "\txvYCC601\n");

		*lenth += sprintf(buf + *lenth, "supported colorimetry metadata:\n");
		if (edid_info->color_db.MD3)
			*lenth += sprintf(buf + *lenth, "\tMD3\n");
		if (edid_info->color_db.MD2)
			*lenth += sprintf(buf + *lenth, "\tMD2\n");
		if (edid_info->color_db.MD1)
			*lenth += sprintf(buf + *lenth, "\tMD1\n");
		if (edid_info->color_db.MD0)
			*lenth += sprintf(buf + *lenth, "\tMD0\n");
	}

	if (edid_info->contain_hdr_db) {
		*lenth += sprintf(buf + *lenth, "****HDR Static Metadata Data Block****\n");
		*lenth += sprintf(buf + *lenth, "eotf_hlg: %d\n",
			edid_info->hdr_db.eotf_hlg);
		*lenth += sprintf(buf + *lenth, "eotf_smpte_st_2084: %d\n",
			edid_info->hdr_db.eotf_smpte_st_2084);
		*lenth += sprintf(buf + *lenth, "eotf_hdr: %d\n",
			edid_info->hdr_db.eotf_hdr);
		*lenth += sprintf(buf + *lenth, "eotf_sdr: %d\n",
			edid_info->hdr_db.eotf_sdr);
		*lenth += sprintf(buf + *lenth, "hdr_SMD_type1: %d\n",
			edid_info->hdr_db.hdr_SMD_type1);
	}

	if (edid_info->contain_y420_vdb) {
		*lenth += sprintf(buf + *lenth, "****Y420 Video Data Block****\n");
		for (i = 0; i < edid_info->y420_vic_len; i++)
			rx_edid_print_vic_fmt(i,
				edid_info->y420_vdb_vic[i], buf, lenth);
	}

	if (edid_info->contain_y420_cmdb) {
		*lenth += sprintf(buf + *lenth, "****Yc420 capability map****\n");
		if (edid_info->y420_all_vic)
			*lenth += sprintf(buf + *lenth, "all vic support y420\n");
		else {
			for (i = 0; i < 31; i++) {
				hdmi_vic = edid_info->y420_cmdb_vic[i];
				if (hdmi_vic)
					rx_edid_print_vic_fmt(i, hdmi_vic, buf, lenth);
			}
		}
	}
}

void HDMIRx_Edid_SetUpdateEdid(U8 *edid_table, U8 edid_version)
{
	HdmiRx_Edid_Hardware_UpdateEdid(edid_table, edid_version);
}

void HDMIRx_Edid_SetEdidVersion(U8 setting_version)
{
	HdmiRx_Edid_Hardware_SetEdidVersion(setting_version);
}

void HDMIRx_Edid_SetPullHotplug(U8 port_id, U8 type)
{
	HdmiRx_Edid_Hardware_SetPullHotplug(port_id, type);
}

void HdmiRx_Edid_Get_5VState(U32 *pb5vstate)
{
	HdmiRx_Edid_Hardware_Get5VState(pb5vstate);
}

void HDMIRx_Edid_InitHpdtimer(U8 port_id)
{
	HdmiRx_Edid_Hardware_InitHpdtimer(port_id);
}

void THDMIRx_Edidinit(void)
{
	int port_id;
	for (port_id = 0; port_id < HDMI_PORT_NUM; port_id++) {
		HdmiRx_Edid_Hardware_InitHpdtimer(port_id);
	}
	HdmiRx_Edid_Hardware_EdidInit();
}

void HDMIRx_Edid_DeinitHpdtimer(U8 port_id)
{
	HdmiRx_Edid_Hardware_DeinitHpdtimer(port_id);
}
