/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _RT_VI_COMPONENT_H_
#define _RT_VI_COMPONENT_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>

#include "vin_video_api.h"
#include <uapi/rt-media/uapi_rt_media.h>
#include "rt_common.h"
#include "vencoder.h"

#define ENABLE_SAVE_VIN_OUT_DATA 0

typedef struct adapter_venc_fps_info {
    int enable;
    int dst_fps;
    uint64_t base_pts;   //unit:us
    uint64_t cur_wanted_pts; //unit:us
    uint64_t frame_cnt;
} adapter_venc_fps_info;

typedef struct vi_comp_base_config {
	unsigned int         width;
	unsigned int         height;
	int                  frame_rate; //source vin fps
	int                  channel_id; //vipp number, scope:[0,15]
	rt_pixelformat_type  pixelformat;
	enum VIDEO_OUTPUT_MODE output_mode;
	int                    drop_frame_num;
	int                    enable_wdr;
	int 				   enable_high_fps_transfor;
	int bonline_channel;
	int share_buf_num;
	VencH264VideoSignal venc_video_signal;
	int en_16_align_fill_data;
	int vin_buf_num;
	adapter_venc_fps_info st_adapter_venc_fps;
	int dst_fps; //dst software fps, vi comp outputs frame using this dst_fps.
	int enable_tdm_raw;
	unsigned int tdm_rxbuf_cnt;
	enum vi_dma_stitch_mode_t dma_merge_mode;
	struct dma_overlay_para dma_overlay_ctx;
	struct dma_merge_scaler_cfg dma_merge_scaler_cfg;
} vi_comp_base_config;

typedef struct vi_comp_normal_config {
	int tmp;
} vi_comp_normal_config;

/* dynamic config */

typedef struct vi_outbuf_manager {
	struct list_head    empty_frame_list; //video_frame_node
	struct list_head    valid_frame_list;
	struct mutex        mutex;
	//int                 empty_num;
	//int                 no_empty_frame_flag;
} vi_outbuf_manager;

typedef struct vi_lbc_fill_param {
    unsigned char *mLbcFillDataAddr;
    unsigned int mLbcFillDataLen;
    unsigned int line_tar_bits[2];
} vi_lbc_fill_param;

typedef struct VI_DMA_LBC_PARAM_S {
    unsigned int frm_wth;
    unsigned int frm_hgt;
    unsigned int frm_x;
    unsigned int frm_y;
    unsigned int line_tar_bits[2];
    unsigned int line_bits;
    unsigned int frm_bits;
    unsigned int mb_wth;
} VI_DMA_LBC_PARAM_S;

typedef struct VI_DMA_LBC_BS_S {
    unsigned char *cur_buf_ptr;
    unsigned int left_bits;
    unsigned int cnt;
    unsigned int sum;
} VI_DMA_LBC_BS_S;

typedef struct VI_DMA_LBC_CFG_S {
    unsigned int frm_wth;
    unsigned int frm_hgt;
    unsigned int is_lossy;
    unsigned int bit_depth;
    unsigned int glb_enable;
    unsigned int dts_enable;
    unsigned int ots_enable;
    unsigned int msq_enable;
    unsigned int line_tar_bits[2];
    unsigned int mb_min_bits[2];
    unsigned int rc_adv[4];
    unsigned int lmtqp_en;
    unsigned int lmtqp_min;
    unsigned int updataadv_en;
    unsigned int updataadv_ratio;
} VI_DMA_LBC_CFG_S;

typedef struct VI_DMA_LBC_STM_S {
    unsigned char *bs;
} VI_DMA_LBC_STM_S;

typedef struct vi_bkbuf_info {
	int buf_num;
	int buf_size;
} vi_bkbuf_info;

error_type vi_comp_component_init(PARAM_IN comp_handle component, const rt_media_config_s *pmedia_config);

#endif

