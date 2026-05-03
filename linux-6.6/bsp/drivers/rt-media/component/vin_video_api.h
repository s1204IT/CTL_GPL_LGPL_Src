/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef _VIN_VIDEO_API_H_
#define _VIN_VIDEO_API_H_

#include <linux/videodev2.h>
#include "../vin/vin-video/vin_video.h"


enum {
	/*encpp_ctrl*/
    // ref to enum encpp_ctrl_id in [sunxi_isp.h]
	RT_CTRL_ENCPP_EN = 0, //ISP_CTRL_ENCPP_EN
	RT_CTRL_ENCPP_STATIC_CFG,
	RT_CTRL_ENCPP_DYNAMIC_CFG,
	RT_CTRL_ENCODER_3DNR_CFG,
	RT_CTRL_ENCODER_2DNR_CFG,
	RT_CTRL_ENCPP_AE_STATS,
	RT_CTRL_ENCPP_AE_ENV_LV,
	RT_CTRL_ENCPP_AE_WEIGHT_LUM,
};


int vin_open_special(int id);
int vin_s_input_special(int id, int i);
int vin_s_parm_special(int id, void *priv, struct v4l2_streamparm *parms);
int vin_close_special(int id);
int vin_s_fmt_special(int id, struct v4l2_format *f);
int vin_g_fmt_special(int id, struct v4l2_format *f);
int vin_dqbuffer_special(int id, struct vin_buffer **buf);
int vin_qbuffer_special(int id, struct vin_buffer *buf);

struct device *vin_get_dev(int id);
int vin_rt_dqbuffer_special(int id, struct vin_buffer **buf);
int vin_rt_qbuffer_special(int id, struct vin_buffer *buf);

int vin_streamon_special(int id, enum v4l2_buf_type i);
int vin_streamoff_special(int id, enum v4l2_buf_type i);
int vin_get_encpp_cfg(int id, unsigned char ctrl_id, void *value);
void vin_register_buffer_done_callback(int id, void *func);
struct device *vin_get_dev(int id);
int vin_isp_get_lv_special(int id, int mode_flag);
void vin_server_reset_special(int id, int mode_flag, int ir_on, int ir_flash_on);
int vin_get_isp_reg(int id, int act_flag, char *save_path);
int vin_s_ctrl_special(int id, unsigned int ctrl_id, int val);
int vin_isp_get_exp_gain_special(int id, struct sensor_exp_gain *exp_gain);
int vin_isp_get_hist_special(int id, unsigned int *hist);
int vin_isp_set_attr_cfg(int id, unsigned int ctrl_id, int value);
int vin_s_fmt_overlay_special(int id, struct v4l2_format *f);
int vin_overlay_special(int id, unsigned int on);
int vin_set_isp_attr_cfg_special(int id, void *value);
int vin_get_isp_attr_cfg_special(int id, void *value);
int vin_set_ve_online_cfg_special(int id, struct csi_ve_online_cfg *cfg);
void vin_get_sensor_name_special(int id, char *name);
void vin_get_sensor_resolution_special(int id, struct sensor_resolution *sensor_resolution);//ryan todo
void vin_sensor_fps_change_callback(int id, void *func);

extern int os_mem_alloc(struct device *dev, struct vin_mm *mem_man);
extern void os_mem_free(struct device *dev, struct vin_mm *mem_man);
extern void vin_iommu_en(unsigned int mester_id, bool en);
void vin_register_tdmbuffer_done_callback(int id, void *func);
void vin_return_tdmbuffer_special(int id, struct vin_isp_tdm_event_status *status);
void vin_get_tdm_data_special(int id, struct vin_isp_tdm_data *data);
int vin_set_tdm_rxbuf_cnt_special(int id, unsigned int *count);
void vin_tunning_ctrl_special(int id, void *value);
int vin_get_sensor_reserve_addr(int id, SENSOR_ISP_CONFIG_S *sensor_isp_cfg);
int vin_set_tdm_drop_frame_special(int id, unsigned int *drop_frame_num);
//int vin_set_input_bit_width_start_special(int id, enum set_bit_width *bit_width);
//int vin_set_input_bit_width_stop_special(int id, enum set_bit_width *bit_width);
int vin_force_reset_buffer(int video_id);

#endif

