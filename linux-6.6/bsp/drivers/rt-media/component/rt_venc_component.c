/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/module.h>
//#include <linux/g2d_driver.h>
#include <linux/fs.h>
#include "vin_video_api.h"
#define LOG_TAG "rt_venc_comp"
#include "rt_common.h"
#include "rt_venc_component.h"
#include "rt_message.h"
#include "rt_semaphore.h"
#include <uapi/media/sunxi_camera_v2.h>
#include <uapi/rt-media/uapi_rt_media.h>

#define TEST_ENCODER_BY_SELF (0)
#define DEBUG_VBV_CACHE_TIME           (4)  // unit:seconds, exp:1,2,3,4...
#define ENABLE_SAVE_NATIVE_OVERLAY_DATA (0)

//#define TEST_I_FRAME_SYNC

DEFINE_MUTEX(venc_mutex);

extern int g2d_open(struct inode *inode, struct file *file);
extern int g2d_release(struct inode *inode, struct file *file);
extern void g2d_ioctl_mutex_lock(void);
extern void g2d_ioctl_mutex_unlock(void);

typedef struct catch_jpeg_cxt {
	int enable;
	int encoder_finish_flag;
	VideoEncoder *vencoder;
	int width;
	int height;
	int qp;
	wait_queue_head_t wait_enc_finish;
	unsigned int wait_enc_finish_condition;
} catch_jpeg_cxt;

typedef struct venc_comp_ctx {
	int vencoder_init_flag;
	VideoEncoder *vencoder;
	comp_state_type state;
	struct task_struct *venc_thread;
	message_queue_t msg_queue;
	comp_callback_type *callback;
	void *callback_data;
	rt_component_type *self_comp;
	//wait_queue_head_t wait_reply[WAIT_REPLY_NUM];
	//unsigned int wait_reply_condition[WAIT_REPLY_NUM];
	venc_inbuf_manager in_buf_manager;
	venc_outbuf_manager out_buf_manager;
	comp_tunnel_info in_port_tunnel_info;
	comp_tunnel_info out_port_tunnel_info;
	venc_comp_base_config base_config;
	venc_comp_normal_config normal_config;
	VencFixQP stVencFixQp;
	int actual_en_encpp_sharp; ///< indicate if actually enable encpp sharp on venc.
	unsigned char *vbvStartAddr;
	catch_jpeg_cxt jpeg_cxt;
	int is_first_frame;
	VencCbType vencCallBack;
	VencOverlayInfoS *venc_overlayInfo;
	int mActiveDropNum; // for COMP_COMMAND_VENC_DROP_FRAME. used in offline encode.
	eCameraStatus camera_move_status;
	RTDynamicProcSettings st_dynamic_proc_set;
	struct isp_cfg_attr_data *p_rt_isp_cfg;
	VencMBModeCtrl mMBModeCtrl;
	sGetMbInfo mGetMbInfo;
	VencMBSumInfo   mMbSumInfo;
	unsigned int mMbNum;

	int nCurRecvFrameNum; //when venc_comp->base_config.nRecvFrameNum > 0, record received frame number.
	struct mutex WaitOutputStreamLock;
	bool bWaitOutputStreamFlag;

	RT_VENC_SEI_ATTR stSEIAttr;
	struct task_struct *SEIThread;
	message_queue_t stSEICmdQueue;
	rt_sem_t stSetSEISem;
	struct mutex SEILock;
	int nSetSEICount;
} venc_comp_ctx;

#define GLOBAL_OSD_MAX_NUM (3)

typedef struct native_osd_info {
	VencOverlayInfoS sOverlayInfo;

	unsigned int ori_frame_w;
	unsigned int ori_frame_h;
	struct mem_interface *memops;

	/* the g2d_in_buf will not release at all, so it can setup to vencoder immediately when start */
	unsigned char *g2d_in_ion_vir_addr[GLOBAL_OSD_MAX_NUM];
	unsigned int g2d_in_buf_size[GLOBAL_OSD_MAX_NUM];
} native_osd_info;

//static native_osd_info *global_osd_info;

static int SEIThread(void *pThreadData);
static int thread_process_venc(void *param);
int adjust_native_overlay_info(venc_comp_ctx *venc_comp);

/*
static RT_H264E_NALU_TYPE_E map_VENC_H264_NALU_TYPE_to_RT_H264E_NALU_TYPE_E(VENC_H264_NALU_TYPE eVencType)
{
	RT_H264E_NALU_TYPE_E eRTType;
	switch (eVencType) {
	case VENC_H264_NALU_PSLICE:
	{
		eRTType = RT_H264E_NALU_PSLICE;
		break;
	}
	case VENC_H264_NALU_ISLICE:
	{
		eRTType = RT_H264E_NALU_ISLICE;
		break;
	}
	case VENC_H264_NALU_SEI:
	{
		eRTType = RT_H264E_NALU_SEI;
		break;
	}
	case VENC_H264_NALU_SPS:
	{
		eRTType = RT_H264E_NALU_SPS;
		break;
	}
	case VENC_H264_NALU_PPS:
	{
		eRTType = RT_H264E_NALU_PPS;
		break;
	}
	case VENC_H264_NALU_IPSLICE:
	{
		eRTType = RT_H264E_NALU_IPSLICE;
		break;
	}
	default:
	{
		RT_LOGE("fatal error! unknown venc h264 nalu type:%d", eVencType);
		eRTType = RT_H264E_NALU_ISLICE;
		break;
	}
	}
	return eRTType;
}
static RT_H265E_NALU_TYPE_E map_VENC_H265_NALU_TYPE_to_RT_H265E_NALU_TYPE_E(VENC_H265_NALU_TYPE eVencType)
{
	RT_H265E_NALU_TYPE_E eRTType;
	switch (eVencType) {
	case VENC_H265_NALU_PSLICE:
	{
		eRTType = RT_H265E_NALU_PSLICE;
		break;
	}
	case VENC_H265_NALU_ISLICE:
	{
		eRTType = RT_H265E_NALU_ISLICE;
		break;
	}
	case VENC_H265_NALU_VPS:
	{
		eRTType = RT_H265E_NALU_VPS;
		break;
	}
	case VENC_H265_NALU_SPS:
	{
		eRTType = RT_H265E_NALU_SPS;
		break;
	}
	case VENC_H265_NALU_PPS:
	{
		eRTType = RT_H265E_NALU_PPS;
		break;
	}
	case VENC_H265_NALU_SEI:
	{
		eRTType = RT_H265E_NALU_SEI;
		break;
	}
	default:
	{
		RT_LOGE("fatal error! unknown venc h265 nalu type:%d", eVencType);
		eRTType = RT_H265E_NALU_ISLICE;
		break;
	}
	}
	return eRTType;
}
static RT_JPEGE_PACK_TYPE_E map_VENC_JPEGE_PACK_TYPE_to_RT_JPEGE_PACK_TYPE_E(VENC_JPEGE_PACK_TYPE eVencType)
{
	RT_JPEGE_PACK_TYPE_E eRTType;
	switch (eVencType) {
	case VENC_JPEG_PACK_ECS:
	{
		eRTType = RT_JPEGE_PACK_ECS;
		break;
	}
	case VENC_JPEG_PACK_APP:
	{
		eRTType = RT_JPEGE_PACK_APP;
		break;
	}
	case VENC_JPEG_PACK_VDO:
	{
		eRTType = RT_JPEGE_PACK_VDO;
		break;
	}
	case VENC_JPEG_PACK_PIC:
	{
		eRTType = RT_JPEGE_PACK_PIC;
		break;
	}
	default:
	{
		RT_LOGE("fatal error! unknown jpeg pack type:%d", eVencType);
		eRTType = RT_JPEGE_PACK_ECS;
		break;
	}
	}
	return eRTType;
}
RT_VENC_DATA_TYPE_U map_VENC_OUTPUT_PACK_TYPE_to_RT_VENC_DATA_TYPE_U(VENC_OUTPUT_PACK_TYPE UVencPackType, RT_VENC_CODEC_TYPE eRTVencCodecType)
{
	RT_VENC_DATA_TYPE_U URTPackType;
	switch (eRTVencCodecType) {
	case RT_VENC_CODEC_H264:
	{
		URTPackType.enH264EType = map_VENC_H264_NALU_TYPE_to_RT_H264E_NALU_TYPE_E(UVencPackType.eH264Type);
		break;
	}
	case RT_VENC_CODEC_H265:
	{
		URTPackType.enH265EType = map_VENC_H265_NALU_TYPE_to_RT_H265E_NALU_TYPE_E(UVencPackType.eH265Type);
		break;
	}
	case RT_VENC_CODEC_JPEG:
	{
		URTPackType.enJPEGEType = map_VENC_JPEGE_PACK_TYPE_to_RT_JPEGE_PACK_TYPE_E(UVencPackType.eJpegType);
		break;
	}
	default:
	{
		RT_LOGE("fatal error! unknown rt venc codec type:%d", eRTVencCodecType);
		URTPackType.enH264EType = RT_H264E_NALU_ISLICE;
		break;
	}
	}
	return URTPackType;
}
*/

static int config_VencOutputBuffer_by_videostream(venc_comp_ctx *venc_comp,
						  VencOutputBuffer *output_buffer,
						  video_stream_s *video_stream)
{
	if (venc_comp->vbvStartAddr == NULL) {
		RT_LOGE("venc_comp->vbvStartAddr is null");
		return -1;
	}

	output_buffer->nID    = video_stream->id;
	output_buffer->nPts   = video_stream->pts;
	output_buffer->nFlag  = video_stream->flag;
	output_buffer->nSize0 = video_stream->size0;
	output_buffer->nSize1 = video_stream->size1;
	output_buffer->nSize2 = video_stream->size2;
	output_buffer->pData0 = venc_comp->vbvStartAddr + video_stream->offset0;
	output_buffer->pData1 = venc_comp->vbvStartAddr + video_stream->offset1;
	output_buffer->pData2 = venc_comp->vbvStartAddr + video_stream->offset2;

	output_buffer->pData0 = video_stream->data0;
	output_buffer->pData1 = video_stream->data1;
	output_buffer->pData2 = video_stream->data2;
	RT_LOGD("request stream: pData0 =%px, %px, id = %d",
		video_stream->data0, output_buffer->pData0, output_buffer->nID);

	return 0;
}

static int config_videostream_by_VencOutputBuffer(venc_comp_ctx *venc_comp,
						  VencOutputBuffer *output_buffer,
						  video_stream_s *video_stream)
{
	if (venc_comp->vbvStartAddr == NULL) {
		RT_LOGE("venc_comp->vbvStartAddr is null");
		return -1;
	}

	video_stream->id      = output_buffer->nID;
	video_stream->pts     = output_buffer->nPts;
	video_stream->flag    = output_buffer->nFlag;
	video_stream->size0   = output_buffer->nSize0;
	video_stream->size1   = output_buffer->nSize1;
	video_stream->size2   = output_buffer->nSize2;
	video_stream->offset0 = output_buffer->pData0 - venc_comp->vbvStartAddr;

	if (output_buffer->pData1)
		video_stream->offset1 = output_buffer->pData1 - venc_comp->vbvStartAddr;
	if (output_buffer->pData2)
		video_stream->offset2 = output_buffer->pData2 - venc_comp->vbvStartAddr;

	video_stream->data0 = output_buffer->pData0;
	video_stream->data1 = output_buffer->pData1;
	video_stream->data2 = output_buffer->pData2;

	RT_LOGD("reqeust stream: pData0 =%px, %px, id = %d",
		video_stream->data0, output_buffer->pData0,
		output_buffer->nID);

	if (video_stream->flag & VENC_BUFFERFLAG_KEYFRAME)
		video_stream->keyframe_flag = 1;
	else
		video_stream->keyframe_flag = 0;

	video_stream->nDataNum = output_buffer->nPackNum;
	//memcpy(video_stream->stPackInfo, output_buffer->mPackInfo, sizeof(VencPackInfo)*MAX_OUTPUT_PACK_NUM);
	int i;
	for (i = 0; i < output_buffer->nPackNum; i++) {
		video_stream->stPackInfo[i].uType = output_buffer->mPackInfo[i].uType;
		video_stream->stPackInfo[i].nOffset = output_buffer->mPackInfo[i].nOffset;
		video_stream->stPackInfo[i].nLength = output_buffer->mPackInfo[i].nLength;
	}

	return 0;
}

#if NEI_TEST_CODE
static int init_default_encpp_param(sEncppSharpParamDynamic *pSharpParamDynamic,
	sEncppSharpParamStatic *pSharpParamStatic, s3DfilterParam *p3DfilterParam, s2DfilterParam *p2DfilterParam)
{
	p3DfilterParam->enable_3d_filter = 1;
	p3DfilterParam->adjust_pix_level_enable = 0;
	p3DfilterParam->smooth_filter_enable = 1;
	p3DfilterParam->max_pix_diff_th = 6;
	p3DfilterParam->max_mad_th = 8;
	p3DfilterParam->max_mv_th = 8;
	p3DfilterParam->min_coef = 15;
	p3DfilterParam->max_coef = 16;

	p2DfilterParam->enable_2d_filter = 1;
	p2DfilterParam->filter_strength_uv = 46;
	p2DfilterParam->filter_strength_y = 46;
	p2DfilterParam->filter_th_uv = 3;
	p2DfilterParam->filter_th_y = 2;

	pSharpParamDynamic->ss_ns_lw  = 0;           //[0,255];
	pSharpParamDynamic->ss_ns_hi  = 0;           //[0,255];
	pSharpParamDynamic->ls_ns_lw  = 0;           //[0,255];
	pSharpParamDynamic->ls_ns_hi  = 0;           //[0,255];
	pSharpParamDynamic->ss_lw_cor = 180;           //[0,255];
	pSharpParamDynamic->ss_hi_cor = 255;           //[0,255];
	pSharpParamDynamic->ls_lw_cor = 120;           //[0,255];
	pSharpParamDynamic->ls_hi_cor = 255;           //[0,255];
	pSharpParamDynamic->ss_blk_stren = 552;      //[0,4095];
	pSharpParamDynamic->ss_wht_stren = 423;      //[0,4095];
	pSharpParamDynamic->ls_blk_stren = 1408;      //[0,4095];
	pSharpParamDynamic->ls_wht_stren = 952;      //[0,4095];
	pSharpParamDynamic->ss_avg_smth  = 0;         //[0,255];
	pSharpParamDynamic->ss_dir_smth  = 0;       //[0,16];
	pSharpParamDynamic->dir_smth[0] = 0;         //[0,16];
	pSharpParamDynamic->dir_smth[1] = 0;         //[0,16];
	pSharpParamDynamic->dir_smth[2] = 0;         //[0,16];
	pSharpParamDynamic->dir_smth[3] = 0;         //[0,16];
	pSharpParamDynamic->hfr_smth_ratio  = 0;      //[0,32];
	pSharpParamDynamic->hfr_hf_wht_stren = 0;    //[0,4095];
	pSharpParamDynamic->hfr_hf_blk_stren  = 0;    //[0,4095];
	pSharpParamDynamic->hfr_mf_wht_stren = 0;    //[0,4095];
	pSharpParamDynamic->hfr_mf_blk_stren  = 0;    //[0,4095];
	pSharpParamDynamic->hfr_hf_cor_ratio  = 0;    //[0,255];
	pSharpParamDynamic->hfr_mf_cor_ratio = 0;    //[0,255];
	pSharpParamDynamic->hfr_hf_mix_ratio = 390;  //[0,1023];
	pSharpParamDynamic->hfr_mf_mix_ratio  = 390;  //[0,1023];
	pSharpParamDynamic->hfr_hf_mix_min_ratio = 0;   //[0,255];
	pSharpParamDynamic->hfr_mf_mix_min_ratio = 0;   //[0,255];
	pSharpParamDynamic->hfr_hf_wht_clp  = 32;     //[0,255];
	pSharpParamDynamic->hfr_hf_blk_clp  = 32;     //[0,255];
	pSharpParamDynamic->hfr_mf_wht_clp  = 32;     //[0,255];
	pSharpParamDynamic->hfr_mf_blk_clp  = 32;     //[0,255];
	pSharpParamDynamic->wht_clp_para = 256;       //[0,1023];
	pSharpParamDynamic->blk_clp_para = 256;       //[0,1023];
	pSharpParamDynamic->wht_clp_slp  = 16;        //[0,63];
	pSharpParamDynamic->blk_clp_slp  = 8;         //[0,63];
	pSharpParamDynamic->max_clp_ratio  = 64;        //[0,255];

	pSharpParamDynamic->sharp_edge_lum[0] = 0;
	pSharpParamDynamic->sharp_edge_lum[1] = 14;
	pSharpParamDynamic->sharp_edge_lum[2] = 25;
	pSharpParamDynamic->sharp_edge_lum[3] = 32;
	pSharpParamDynamic->sharp_edge_lum[4] = 39;
	pSharpParamDynamic->sharp_edge_lum[5] = 51;
	pSharpParamDynamic->sharp_edge_lum[6] = 65;
	pSharpParamDynamic->sharp_edge_lum[7] = 80;
	pSharpParamDynamic->sharp_edge_lum[8] = 99;
	pSharpParamDynamic->sharp_edge_lum[9] = 124;
	pSharpParamDynamic->sharp_edge_lum[10] = 152;
	pSharpParamDynamic->sharp_edge_lum[11] = 179;
	pSharpParamDynamic->sharp_edge_lum[12] = 203;
	pSharpParamDynamic->sharp_edge_lum[13] = 224;
	pSharpParamDynamic->sharp_edge_lum[14] = 237;
	pSharpParamDynamic->sharp_edge_lum[15] = 245;
	pSharpParamDynamic->sharp_edge_lum[16] = 271;
	pSharpParamDynamic->sharp_edge_lum[17] = 336;
	pSharpParamDynamic->sharp_edge_lum[18] = 428;
	pSharpParamDynamic->sharp_edge_lum[19] = 526;
	pSharpParamDynamic->sharp_edge_lum[20] = 623;
	pSharpParamDynamic->sharp_edge_lum[21] = 709;
	pSharpParamDynamic->sharp_edge_lum[22] = 767;
	pSharpParamDynamic->sharp_edge_lum[23] = 782;
	pSharpParamDynamic->sharp_edge_lum[24] = 772;
	pSharpParamDynamic->sharp_edge_lum[25] = 761;
	pSharpParamDynamic->sharp_edge_lum[26] = 783;
	pSharpParamDynamic->sharp_edge_lum[27] = 858;
	pSharpParamDynamic->sharp_edge_lum[28] = 941;
	pSharpParamDynamic->sharp_edge_lum[29] = 988;
	pSharpParamDynamic->sharp_edge_lum[30] = 1006;
	pSharpParamDynamic->sharp_edge_lum[31] = 1016;
	pSharpParamDynamic->sharp_edge_lum[32] = 1023;

	pSharpParamStatic->ss_shp_ratio = 0;         //[0,255];
	pSharpParamStatic->ls_shp_ratio = 0;         //[0,255];
	pSharpParamStatic->ss_dir_ratio = 64;         //[0,1023];
	pSharpParamStatic->ls_dir_ratio = 255;         //[0,1023];
	pSharpParamStatic->ss_crc_stren = 0;        //[0,1023];
	pSharpParamStatic->ss_crc_min = 16;        //[0,255];
	pSharpParamStatic->wht_sat_ratio = 64;
	pSharpParamStatic->blk_sat_ratio = 64;
	pSharpParamStatic->wht_slp_bt = 1;
	pSharpParamStatic->blk_slp_bt = 1;

	pSharpParamStatic->sharp_ss_value[0] = 262;
	pSharpParamStatic->sharp_ss_value[1] = 375;
	pSharpParamStatic->sharp_ss_value[2] = 431;
	pSharpParamStatic->sharp_ss_value[3] = 395;
	pSharpParamStatic->sharp_ss_value[4] = 316;
	pSharpParamStatic->sharp_ss_value[5] = 250;
	pSharpParamStatic->sharp_ss_value[6] = 206;
	pSharpParamStatic->sharp_ss_value[7] = 179;
	pSharpParamStatic->sharp_ss_value[8] = 158;
	pSharpParamStatic->sharp_ss_value[9] = 132;
	pSharpParamStatic->sharp_ss_value[10] = 107;
	pSharpParamStatic->sharp_ss_value[11] = 91;
	pSharpParamStatic->sharp_ss_value[12] = 79;
	pSharpParamStatic->sharp_ss_value[13] = 67;
	pSharpParamStatic->sharp_ss_value[14] = 56;
	pSharpParamStatic->sharp_ss_value[15] = 50;
	pSharpParamStatic->sharp_ss_value[16] = 39;
	pSharpParamStatic->sharp_ss_value[17] = 19;
	pSharpParamStatic->sharp_ss_value[18] = 0;
	pSharpParamStatic->sharp_ss_value[19] = 0;
	pSharpParamStatic->sharp_ss_value[20] = 0;
	pSharpParamStatic->sharp_ss_value[21] = 1;
	pSharpParamStatic->sharp_ss_value[22] = 0;
	pSharpParamStatic->sharp_ss_value[23] = 0;
	pSharpParamStatic->sharp_ss_value[24] = 0;
	pSharpParamStatic->sharp_ss_value[25] = 0;
	pSharpParamStatic->sharp_ss_value[26] = 0;
	pSharpParamStatic->sharp_ss_value[27] = 0;
	pSharpParamStatic->sharp_ss_value[28] = 0;
	pSharpParamStatic->sharp_ss_value[29] = 0;
	pSharpParamStatic->sharp_ss_value[30] = 0;
	pSharpParamStatic->sharp_ss_value[31] = 0;
	pSharpParamStatic->sharp_ss_value[32] = 0;

	pSharpParamStatic->sharp_ls_value[0] = 341;
	pSharpParamStatic->sharp_ls_value[1] = 489;
	pSharpParamStatic->sharp_ls_value[2] = 564;
	pSharpParamStatic->sharp_ls_value[3] = 515;
	pSharpParamStatic->sharp_ls_value[4] = 395;
	pSharpParamStatic->sharp_ls_value[5] = 254;
	pSharpParamStatic->sharp_ls_value[6] = 90;
	pSharpParamStatic->sharp_ls_value[7] = 56;
	pSharpParamStatic->sharp_ls_value[8] = 54;
	pSharpParamStatic->sharp_ls_value[9] = 52;
	pSharpParamStatic->sharp_ls_value[10] = 51;
	pSharpParamStatic->sharp_ls_value[11] = 48;
	pSharpParamStatic->sharp_ls_value[12] = 42;
	pSharpParamStatic->sharp_ls_value[13] = 36;
	pSharpParamStatic->sharp_ls_value[14] = 31;
	pSharpParamStatic->sharp_ls_value[15] = 28;
	pSharpParamStatic->sharp_ls_value[16] = 23;
	pSharpParamStatic->sharp_ls_value[17] = 11;
	pSharpParamStatic->sharp_ls_value[18] = 0;
	pSharpParamStatic->sharp_ls_value[19] = 0;
	pSharpParamStatic->sharp_ls_value[20] = 0;
	pSharpParamStatic->sharp_ls_value[21] = 1;
	pSharpParamStatic->sharp_ls_value[22] = 0;
	pSharpParamStatic->sharp_ls_value[23] = 0;
	pSharpParamStatic->sharp_ls_value[24] = 0;
	pSharpParamStatic->sharp_ls_value[25] = 0;
	pSharpParamStatic->sharp_ls_value[26] = 0;
	pSharpParamStatic->sharp_ls_value[27] = 0;
	pSharpParamStatic->sharp_ls_value[28] = 0;
	pSharpParamStatic->sharp_ls_value[29] = 0;
	pSharpParamStatic->sharp_ls_value[30] = 0;
	pSharpParamStatic->sharp_ls_value[31] = 0;
	pSharpParamStatic->sharp_ls_value[32] = 0;

	pSharpParamStatic->sharp_hsv[0] = 287;
	pSharpParamStatic->sharp_hsv[1] = 286;
	pSharpParamStatic->sharp_hsv[2] = 283;
	pSharpParamStatic->sharp_hsv[3] = 276;
	pSharpParamStatic->sharp_hsv[4] = 265;
	pSharpParamStatic->sharp_hsv[5] = 256;
	pSharpParamStatic->sharp_hsv[6] = 256;
	pSharpParamStatic->sharp_hsv[7] = 269;
	pSharpParamStatic->sharp_hsv[8] = 290;
	pSharpParamStatic->sharp_hsv[9] = 314;
	pSharpParamStatic->sharp_hsv[10] = 336;
	pSharpParamStatic->sharp_hsv[11] = 361;
	pSharpParamStatic->sharp_hsv[12] = 397;
	pSharpParamStatic->sharp_hsv[13] = 446;
	pSharpParamStatic->sharp_hsv[14] = 492;
	pSharpParamStatic->sharp_hsv[15] = 514;
	pSharpParamStatic->sharp_hsv[16] = 498;
	pSharpParamStatic->sharp_hsv[17] = 457;
	pSharpParamStatic->sharp_hsv[18] = 411;
	pSharpParamStatic->sharp_hsv[19] = 376;
	pSharpParamStatic->sharp_hsv[20] = 352;
	pSharpParamStatic->sharp_hsv[21] = 336;
	pSharpParamStatic->sharp_hsv[22] = 324;
	pSharpParamStatic->sharp_hsv[23] = 315;
	pSharpParamStatic->sharp_hsv[24] = 307;
	pSharpParamStatic->sharp_hsv[25] = 299;
	pSharpParamStatic->sharp_hsv[26] = 290;
	pSharpParamStatic->sharp_hsv[27] = 281;
	pSharpParamStatic->sharp_hsv[28] = 272;
	pSharpParamStatic->sharp_hsv[29] = 261;
	pSharpParamStatic->sharp_hsv[30] = 247;
	pSharpParamStatic->sharp_hsv[31] = 230;
	pSharpParamStatic->sharp_hsv[32] = 215;
	pSharpParamStatic->sharp_hsv[33] = 209;
	pSharpParamStatic->sharp_hsv[34] = 215;
	pSharpParamStatic->sharp_hsv[35] = 229;
	pSharpParamStatic->sharp_hsv[36] = 243;
	pSharpParamStatic->sharp_hsv[37] = 252;
	pSharpParamStatic->sharp_hsv[38] = 257;
	pSharpParamStatic->sharp_hsv[39] = 258;
	pSharpParamStatic->sharp_hsv[40] = 257;
	pSharpParamStatic->sharp_hsv[41] = 256;
	pSharpParamStatic->sharp_hsv[42] = 256;
	pSharpParamStatic->sharp_hsv[43] = 260;
	pSharpParamStatic->sharp_hsv[44] = 266;
	pSharpParamStatic->sharp_hsv[45] = 289;

	return 0;
}
#endif

static void MBParamInit(VencMBModeCtrl *pMBModeCtrl, sGetMbInfo *pGetMbInfo, venc_comp_base_config *pConfigPara, unsigned int *pMbNum)
{
	unsigned int pic_mb_width = RT_ALIGN(pConfigPara->dst_width, 16) >> 4;
	unsigned int pic_mb_height = RT_ALIGN(pConfigPara->dst_height, 16) >> 4;
	unsigned int mb_num = pic_mb_width * pic_mb_height;
	unsigned int mb_info_len = mb_num * sizeof(sH264GetMbInfo);
	unsigned int map_info_len = mb_num * sizeof(VencMbMap);

	if ((NULL == pMBModeCtrl) || (NULL == pGetMbInfo) || (NULL == pConfigPara) || (NULL == pMbNum)) {
		RT_LOGE("fatal error! invalid input params! %p,%p,%p,%p", pMBModeCtrl, pGetMbInfo, pConfigPara, pMbNum);
		return;
	}
	if (RT_VENC_CODEC_H264 != pConfigPara->codec_type && RT_VENC_CODEC_H265 != pConfigPara->codec_type) {
		RT_LOGE("fatal error! Unexpected venc type:0x%x, QPMAP only for H264/H265!", pConfigPara->codec_type);
		return;
	}

	// calc mb num and malloc GetMbInfo buffer
	if (NULL == pGetMbInfo->mb) {
		pGetMbInfo->mb = (sH264GetMbInfo *)kmalloc(mb_info_len, GFP_KERNEL);
		if (NULL == pGetMbInfo->mb) {
			RT_LOGE("fatal error! malloc mb fail!");
			return;
		}
		memset(pGetMbInfo->mb, 0, mb_info_len);
		RT_LOGD("pGetMbInfo->mb=%p", pGetMbInfo->mb);
	} else {
		RT_LOGW("init mb_ctu repeated? pGetMbInfo->mb is not NULL!");
	}

	// malloc mb mode p_map_info
	if (NULL == pMBModeCtrl->p_map_info) {
		pMBModeCtrl->p_map_info = kmalloc(map_info_len, GFP_KERNEL);
		if (NULL == pMBModeCtrl->p_map_info) {
			RT_LOGE("fatal error! malloc pMBModeCtrl->p_map_info fail! size=%d", map_info_len);
			return;
		}
		RT_LOGD("p_map_info malloc success, vir:%p, len:%d", pMBModeCtrl->p_map_info, map_info_len);
	} else {
		RT_LOGW("init mb mode repeated? pMBModeCtrl->p_map_info is not NULL!");
	}
	memset(pMBModeCtrl->p_map_info, 0, map_info_len);

	*pMbNum = mb_num;

	pMBModeCtrl->mode_ctrl_en = 1;
	pMBModeCtrl->qp_map_en = pConfigPara->mb_mode_ctl.qp_map_en;
	pMBModeCtrl->lambda_map_en = pConfigPara->mb_mode_ctl.lambda_map_en;
	pMBModeCtrl->mode_map_en = pConfigPara->mb_mode_ctl.mode_map_en;
	pMBModeCtrl->split_map_en = pConfigPara->mb_mode_ctl.split_map_en;
	RT_LOGW("mb_num=%d, pMBModeCtrl->p_map_info=%p", mb_num, pMBModeCtrl->p_map_info);
}

static void MBParamDeinit(VencMBModeCtrl *pMBModeCtrl, sGetMbInfo *pGetMbInfo, venc_comp_base_config *pConfigPara)
{
	if ((NULL == pMBModeCtrl) || (NULL == pGetMbInfo) || (NULL == pConfigPara)) {
		RT_LOGE("fatal error! invalid input params! %p,%p,%p", pMBModeCtrl, pGetMbInfo, pConfigPara);
		return;
	}

	if (pMBModeCtrl->p_map_info) {
		kfree(pMBModeCtrl->p_map_info);
		pMBModeCtrl->p_map_info = NULL;
	}
	pMBModeCtrl->mode_ctrl_en = 0;

	if (RT_VENC_CODEC_H264 == pConfigPara->codec_type || RT_VENC_CODEC_H265 == pConfigPara->codec_type) {
		if (pGetMbInfo->mb) {
			kfree(pGetMbInfo->mb);
			pGetMbInfo->mb = NULL;
		}
	} else {
		RT_LOGE("fatal error! Unexpected venc type:0x%x, QPMAP only for H264/H265!", pConfigPara->codec_type);
		return;
	}
}

static void ParseMbMadQpSSE(unsigned char *p_mb_mad_qp_sse, venc_comp_base_config *pConfigPara, sGetMbInfo *pGetMbInfo)
{
	if ((NULL == p_mb_mad_qp_sse) || (NULL == pConfigPara) || (NULL == pGetMbInfo)) {
		RT_LOGE("fatal error! invalid input params! %p,%p,%p", p_mb_mad_qp_sse, pConfigPara, pGetMbInfo);
		return;
	}

	VencMbInfoParcel *mb_addr = (VencMbInfoParcel *)p_mb_mad_qp_sse;

	if (RT_VENC_CODEC_H264 == pConfigPara->codec_type || RT_VENC_CODEC_H265 == pConfigPara->codec_type) {
		unsigned int pic_mb_width = RT_ALIGN(pConfigPara->dst_width, 16) >> 4;
		unsigned int pic_mb_height = RT_ALIGN(pConfigPara->dst_height, 16) >> 4;
		unsigned int total_mb_num = pic_mb_width * pic_mb_height;

		sH264GetMbInfo *mb = (sH264GetMbInfo *)pGetMbInfo->mb;
		unsigned int cnt = 0;
		for (cnt = 0; cnt < total_mb_num; cnt++) {
			mb[cnt].mad  = mb_addr->mad;
			mb[cnt].qp   = mb_addr->qp;
			mb[cnt].type = mb_addr->mb_type;
			mb[cnt].sse  = mb_addr->sse;
			mb_addr++;
		}
	} else {
		RT_LOGE("fatal error! Unexpected venc type:0x%x, QPMAP only for H264/H265!", pConfigPara->codec_type);
		return;
	}
}

static void ParseMbMv(unsigned char *p_mb_mv, venc_comp_base_config *pConfigPara, sGetMbInfo *pGetMbInfo)
{
	if ((NULL == p_mb_mv) || (NULL == pConfigPara) || (NULL == pGetMbInfo)) {
		RT_LOGE("fatal error! invalid input params! %p,%p,%p", p_mb_mv, pConfigPara, pGetMbInfo);
		return;
	}

	if (RT_VENC_CODEC_H264 == pConfigPara->codec_type || RT_VENC_CODEC_H265 == pConfigPara->codec_type) {
		VencMvInfoParcel *parcel = (VencMvInfoParcel *)p_mb_mv;
		unsigned int pic_mb_width = RT_ALIGN(pConfigPara->dst_width, 16) >> 4;
		unsigned int pic_mb_height = RT_ALIGN(pConfigPara->dst_height, 16) >> 4;

		sH264GetMbInfo *mb = (sH264GetMbInfo *)pGetMbInfo->mb;
		unsigned int cnt = 0, x = 0, y = 0, w = 0, k = 0;
		for (y = 0; y < pic_mb_height; y++) {
			for (x = 0; x < pic_mb_width; x++) {
				if (x < pic_mb_width) {
					mb[cnt].mv_x = parcel->max_mvx;
					mb[cnt].mv_y = parcel->max_mvy;
				}
				parcel++;
				cnt++;
			}
		}
	} else {
		RT_LOGE("fatal error! Unexpected venc type:0x%x, QPMAP only for H264/H265!", pConfigPara->codec_type);
		return;
	}
}

static void FillQpMapInfo(VencMBModeCtrl *pMBModeCtrl, venc_comp_base_config *pConfigPara, sGetMbInfo *pGetMbInfo)
{
	if ((NULL == pMBModeCtrl) || (NULL == pConfigPara) || (NULL == pGetMbInfo)) {
		RT_LOGE("fatal error! invalid input params! %p,%p,%p", pMBModeCtrl, pConfigPara, pGetMbInfo);
		return;
	}

	if ((NULL == pMBModeCtrl->p_map_info) || (0 == pMBModeCtrl->mode_ctrl_en)) {
		RT_LOGE("fatal error! invalid input param! p_map_info=%p, mode_ctrl_en=%d",
			pMBModeCtrl->p_map_info, pMBModeCtrl->mode_ctrl_en);
		return;
	}

	if (RT_VENC_CODEC_H264 == pConfigPara->codec_type || RT_VENC_CODEC_H265 == pConfigPara->codec_type) {
		unsigned int cnt = 0, x = 0, y = 0;
		unsigned int pic_mb_width = RT_ALIGN(pConfigPara->dst_width, 16) >> 4;
		unsigned int pic_mb_height = RT_ALIGN(pConfigPara->dst_height, 16) >> 4;

		int en_low32 = pMBModeCtrl->mode_map_en || pMBModeCtrl->lambda_map_en || pMBModeCtrl->qp_map_en;
		int en_high32 = pMBModeCtrl->split_map_en;

		if (!en_low32 && !en_high32) {
			RT_LOGW("Both map function is disable!");
		} else if (en_low32 && !en_high32) {
			VencMbModeLambdaQpMap *parcel = (VencMbModeLambdaQpMap *)pMBModeCtrl->p_map_info;
			for (y = 0; y < pic_mb_height; y++) {
				for (x = 0; x < pic_mb_width; x++) {
					parcel[cnt].qp =            pConfigPara->mMapLow.qp;
					parcel[cnt].skip =          pConfigPara->mMapLow.skip;
					parcel[cnt].qp_skip_en =    pConfigPara->mMapLow.qp_skip_en;
					parcel[cnt].lambda =        pConfigPara->mMapLow.lambda;
					parcel[cnt].skip_bias =     pConfigPara->mMapLow.skip_bias;
					parcel[cnt].skip_bias_en =  pConfigPara->mMapLow.skip_bias_en;
					parcel[cnt].inter_bias =    pConfigPara->mMapLow.inter_bias;
					parcel[cnt].inter_bias_en = pConfigPara->mMapLow.inter_bias_en;
					cnt++;
				}
			}
		} else if (!en_low32 && !en_high32) {
			VencMbSplitMap *parcel = (VencMbSplitMap *)pMBModeCtrl->p_map_info;
			for (y = 0; y < pic_mb_height; y++) {
				for (x = 0; x < pic_mb_width; x++) {
					parcel[cnt].inter8_cost_factor =    pConfigPara->mMapHigh.inter8_cost_factor;
					parcel[cnt].inter16_cost_factor =   pConfigPara->mMapHigh.inter16_cost_factor;
					parcel[cnt].inter_cost_factor_en =  pConfigPara->mMapHigh.inter_cost_factor_en;
					parcel[cnt].intra4_cost_factor =    pConfigPara->mMapHigh.intra4_cost_factor;
					parcel[cnt].intra8_cost_factor =    pConfigPara->mMapHigh.intra8_cost_factor;
					parcel[cnt].intra_cost_factor_en =  pConfigPara->mMapHigh.intra_cost_factor_en;
					cnt++;
				}
			}
		} else {
			VencMbMap *parcel = (VencMbMap *)pMBModeCtrl->p_map_info;
			for (y = 0; y < pic_mb_height; y++) {
				for (x = 0; x < pic_mb_width; x++) {
					parcel[cnt].low32.qp =                     pConfigPara->mMapLow.qp;
					parcel[cnt].low32.skip =                   pConfigPara->mMapLow.skip;
					parcel[cnt].low32.qp_skip_en =             pConfigPara->mMapLow.qp_skip_en;
					parcel[cnt].low32.lambda =                 pConfigPara->mMapLow.lambda;
					parcel[cnt].low32.skip_bias =              pConfigPara->mMapLow.skip_bias;
					parcel[cnt].low32.skip_bias_en =           pConfigPara->mMapLow.skip_bias_en;
					parcel[cnt].low32.inter_bias =             pConfigPara->mMapLow.inter_bias;
					parcel[cnt].low32.inter_bias_en =          pConfigPara->mMapLow.inter_bias_en;
					parcel[cnt].high32.inter8_cost_factor =    pConfigPara->mMapHigh.inter8_cost_factor;
					parcel[cnt].high32.inter16_cost_factor =   pConfigPara->mMapHigh.inter16_cost_factor;
					parcel[cnt].high32.inter_cost_factor_en =  pConfigPara->mMapHigh.inter_cost_factor_en;
					parcel[cnt].high32.intra4_cost_factor =    pConfigPara->mMapHigh.intra4_cost_factor;
					parcel[cnt].high32.intra8_cost_factor =    pConfigPara->mMapHigh.intra8_cost_factor;
					parcel[cnt].high32.intra_cost_factor_en =  pConfigPara->mMapHigh.intra_cost_factor_en;
					cnt++;
				}
			}
		}
	} else {
		RT_LOGE("fatal error! Unexpected venc type:0x%x, QPMAP only for H264/H265!", pConfigPara->codec_type);
		return;
	}
}

static void QpMapInfoUserProcessFunc(venc_comp_base_config *pConfigPara, sGetMbInfo *pGetMbInfo)
{
	if ((NULL == pConfigPara) || (NULL == pGetMbInfo)) {
		RT_LOGE("fatal error! invalid input params! %p,%p", pConfigPara, pGetMbInfo);
		return;
	}

	if (RT_VENC_CODEC_H264 == pConfigPara->codec_type || RT_VENC_CODEC_H265 == pConfigPara->codec_type) {
		sH264GetMbInfo *mb = (sH264GetMbInfo *)pGetMbInfo->mb;
		// The user can modify QP of a macro block. At the same time, user can set whether to
		// skip this macro block, or whether to enable this macro block.
		// The specific modification strategy needs to be judged by the user.
	} else {
		RT_LOGE("fatal error! Unexpected venc type:0x%x, QPMAP only for H264/H265!", pConfigPara->codec_type);
		return;
	}
}

static int venc_comp_event_handler(VideoEncoder *pEncoder, void *pAppData, VencEventType eEvent,
		unsigned int nData1, unsigned int nData2, void *pEventData)
{
	venc_comp_ctx *venc_comp = (venc_comp_ctx *)pAppData;

	if (VencEvent_UpdateIspToVeParam == eEvent) {
		if (!venc_comp->base_config.enable_isp2ve_linkage)
			return -1;
		int i = 0;
		int vipp_id = venc_comp->base_config.channel_id;
		int en_encpp = 0;
		int final_en_encpp_sharp = 0;
		int ae_env_lv, ae_weight_lum;
		int ret;

		VencIsp2VeParam *isp2ve_param = (VencIsp2VeParam *)pEventData;
		sEncppSharpParamDynamic *dynamic_sharp = &isp2ve_param->mSharpParam.mDynamicParam;
		sEncppSharpParamStatic *static_sharp = &isp2ve_param->mSharpParam.mStaticParam;
		int encpp_sharp_atten_coef_per = 0;

		if (venc_comp->base_config.skip_sharp_param_frame_num <= 0) {
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);
			RT_LOGI("vencoder:%px, codec_type:%d, en_encpp %d %d", venc_comp->vencoder, venc_comp->base_config.codec_type,
				en_encpp, venc_comp->base_config.en_encpp_sharp);
			final_en_encpp_sharp = en_encpp && venc_comp->base_config.en_encpp_sharp;
			if (venc_comp->actual_en_encpp_sharp != final_en_encpp_sharp) {
				RT_LOGW("Be careful! vencoder:%px, codec_type:%d, en_encpp change:%d->%d(%d,%d)", venc_comp->vencoder,
					venc_comp->base_config.codec_type, venc_comp->actual_en_encpp_sharp, final_en_encpp_sharp, en_encpp,
					venc_comp->base_config.en_encpp_sharp);
				venc_comp->actual_en_encpp_sharp = final_en_encpp_sharp;
				VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, (void *)&venc_comp->actual_en_encpp_sharp);
			}
			if (venc_comp->actual_en_encpp_sharp) {
				memset(dynamic_sharp, 0, sizeof(sEncppSharpParamDynamic));
				memset(static_sharp, 0, sizeof(sEncppSharpParamStatic));
				vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_DYNAMIC_CFG, dynamic_sharp);
				vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_STATIC_CFG, static_sharp);

				struct sensor_config stSensorConfig;
				ret = vin_get_sensor_info_special(vipp_id, &stSensorConfig);
				if (0 == ret) {
					//RT_LOGD("vin get_sensor_info special:[%dx%d]-%d", stSensorConfig.width, stSensorConfig.height, venc_comp->base_config.src_width);
					encpp_sharp_atten_coef_per = venc_comp->base_config.src_width*100/stSensorConfig.width;
				} else {
					RT_LOGE("fatal error! vipp[%d] get sensor info fail[%d]", vipp_id, ret);
				}

				RT_LOGI("hfr_hf_wht_clp %d max_clp_ratio %d", dynamic_sharp->hfr_hf_wht_clp, dynamic_sharp->max_clp_ratio);
				RT_LOGI("ls_dir_ratio %d ss_dir_ratio %d", static_sharp->ls_dir_ratio, static_sharp->ss_dir_ratio);
				if (encpp_sharp_atten_coef_per) {
					dynamic_sharp->ss_blk_stren = dynamic_sharp->ss_blk_stren * encpp_sharp_atten_coef_per / 100;
					dynamic_sharp->ss_wht_stren = dynamic_sharp->ss_wht_stren * encpp_sharp_atten_coef_per / 100;
					dynamic_sharp->ls_blk_stren = dynamic_sharp->ls_blk_stren * encpp_sharp_atten_coef_per / 100;
					dynamic_sharp->ls_wht_stren = dynamic_sharp->ls_wht_stren * encpp_sharp_atten_coef_per / 100;
					RT_LOGI("enable encpp sharp param stren decrease");
				}
			}
		} else {
			venc_comp->base_config.skip_sharp_param_frame_num--;
			RT_LOGI("skip set ve encpp sharp param");
		}

		//vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_AE_STATS, isp2ve_param->mIspAeStatus.avg);
		vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_AE_ENV_LV, &ae_env_lv);
		//vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_AE_WEIGHT_LUM, &ae_weight_lum);
		RT_LOGI("ve isp linkage EnvLv %d AeWeightLum %d CameraMoveStatus %d", ae_env_lv, ae_weight_lum,
			venc_comp->camera_move_status);
		isp2ve_param->mEnvLv = ae_env_lv;
		//isp2ve_param->mAeWeightLum = ae_weight_lum;
		isp2ve_param->mEnCameraMove = venc_comp->camera_move_status;
	} else if (VencEvent_UpdateVeToIspParam == eEvent) {
		if (!venc_comp->base_config.enable_ve2isp_linkage)
			return -1;
		VencVe2IspParam *s_ve2isp_param = (VencVe2IspParam *)pEventData;
		struct isp_cfg_attr_data *p_rt_isp_cfg = venc_comp->p_rt_isp_cfg;
		RT_LOGI("d2d_level %d %d", s_ve2isp_param->d2d_level, s_ve2isp_param->d3d_level);
		//set 2d param
		/*p_rt_isp_cfg->cfg_id = RT_ISP_CTRL_DN_STR;
		p_rt_isp_cfg->denoise_level = s_ve2isp_param->d2d_level;
		vin_set_isp_attr_cfg_special(venc_comp->base_config.channel_id, p_rt_isp_cfg);*/

		//set 3d param
		/*p_rt_isp_cfg->cfg_id = RT_ISP_CTRL_3DN_STR;
		p_rt_isp_cfg->tdf_level = s_ve2isp_param->d3d_level;
		vin_set_isp_attr_cfg_special(venc_comp->base_config.channel_id, p_rt_isp_cfg);*/

		p_rt_isp_cfg->cfg_id = ISP_CTRL_VENC2ISP_PARAM;
		if (sizeof(struct enc_VencVe2IspParam) != sizeof(VencVe2IspParam)) {
			RT_LOGE("fatal error! rt_media and libcedarc Ve2ispParam size is not same, check code!");
		}
		memcpy(&p_rt_isp_cfg->VencVe2IspParam, s_ve2isp_param, sizeof(VencVe2IspParam));
//		if (sizeof(RTIspCfgAttrData) != sizeof(struct isp_cfg_attr_data)) {
//			RT_LOGE("fatal error! rt_media and vin isp_cfg_attr data size is not same, check code!");
//		}
		vin_set_isp_attr_cfg_special(venc_comp->base_config.channel_id, p_rt_isp_cfg);
	} else if (VencEvent_UpdateMbModeInfo == eEvent) {
		RT_LOGI("VencEvent_UpdateMbModeInfo");
		// The encoding has just been completed, but the encoding of the next frame has not yet started.
		// Before encoding, set the encoding method of the frame to be encoded.
		// The user can modify the encoding parameters before setting.
		QpMapInfoUserProcessFunc(&venc_comp->base_config, &venc_comp->mGetMbInfo);

		FillQpMapInfo(&venc_comp->mMBModeCtrl, &venc_comp->base_config, &venc_comp->mGetMbInfo);

		VencMBModeCtrl *pMBModeCtrl = (VencMBModeCtrl *)pEventData;
		RT_LOGI("pMBModeCtrl 0x%px &venc_comp->mMBModeCtrl 0x%px", pMBModeCtrl, &venc_comp->mMBModeCtrl);
		memcpy(pMBModeCtrl, &venc_comp->mMBModeCtrl, sizeof(VencMBModeCtrl));

		if ((0 == pMBModeCtrl->mode_ctrl_en) || (NULL == pMBModeCtrl->p_map_info)) {
			RT_LOGW("Venc ModeCtrl: mode_ctrl_en=%d, p_map_info=%p",
				pMBModeCtrl->mode_ctrl_en, pMBModeCtrl->p_map_info);
		}
	} else if (VencEvent_UpdateMbStatInfo == eEvent) {
		RT_LOGI("VencEvent_UpdateMbStatInfo");
		// The encoding has just been completed, but the encoding of the next frame has not yet started.
		// and the QpMap information of the frame that has just been encoded is obtained.
		VencMBSumInfo *pMbSumInfo = (VencMBSumInfo *)pEventData;
		memcpy(&venc_comp->mMbSumInfo, pMbSumInfo, sizeof(VencMBSumInfo));

		if ((0 == pMbSumInfo->avg_qp) || (NULL == pMbSumInfo->p_mb_mad_qp_sse) || (NULL == pMbSumInfo->p_mb_mv)) {
			RT_LOGW("Venc MBSumInfo: avg_mad=%d, avg_sse=%d, avg_qp=%d, avg_psnr=%lf, "
				"p_mb_mad_qp_sse=%p, p_mb_mv=%p",
				pMbSumInfo->avg_mad, pMbSumInfo->avg_sse, pMbSumInfo->avg_qp, pMbSumInfo->avg_psnr,
				pMbSumInfo->p_mb_mad_qp_sse, pMbSumInfo->p_mb_mv);
		}

		ParseMbMadQpSSE(venc_comp->mMbSumInfo.p_mb_mad_qp_sse, &venc_comp->base_config, &venc_comp->mGetMbInfo);
		ParseMbMv(venc_comp->mMbSumInfo.p_mb_mv, &venc_comp->base_config, &venc_comp->mGetMbInfo);
	}
	return 0;
}
static int setup_vbv_buffer_size(venc_comp_ctx *venc_comp)
{
	unsigned int vbv_size = venc_comp->base_config.vbv_buf_size;
	int min_size;
	unsigned int thresh_size = venc_comp->base_config.vbv_thresh_size;
	unsigned int bit_rate = venc_comp->base_config.bit_rate;

	min_size    = venc_comp->base_config.dst_width * venc_comp->base_config.dst_height * 3 / 2;
	RT_LOGD("vbv_size %d %d", vbv_size, thresh_size);
	if (!vbv_size || !thresh_size) {
		if (bit_rate) {
			thresh_size = bit_rate / 8 / venc_comp->base_config.frame_rate * 15;
		} else {
			thresh_size = venc_comp->base_config.dst_width * venc_comp->base_config.dst_height;
		}
		if (thresh_size > 7 * 1024 * 1024) {
			RT_LOGW("Be careful! threshSize[%d]bytes too large, reduce to 7MB", thresh_size);
			thresh_size = 7 * 1024 * 1024;
		}

		if (bit_rate > 0) {
			vbv_size    = bit_rate / 8 * DEBUG_VBV_CACHE_TIME + thresh_size;
		} else {
			vbv_size = min_size;
		}
	}
	vbv_size = RT_ALIGN(vbv_size, 1024);

	if (vbv_size <= thresh_size) {
		RT_LOGE("fatal error! vbv_size[%d] <= thresh_size[%d]", vbv_size, thresh_size);
	}

	if (vbv_size > 12 * 1024 * 1024) {
		RT_LOGE("Be careful! vbv_size[%d] too large, exceed 24M byte", vbv_size);
		vbv_size = 12 * 1024 * 1024;
	}

	RT_LOGD("bit rate is %d bytes, set encode vbv size %d, frame length threshold %d, minSize = %d",
		bit_rate, vbv_size, thresh_size, min_size);

	if (venc_comp->base_config.dst_width >= 3840) {
		vbv_size    = 2 * 1024 * 1024;
		thresh_size = 1 * 1024 * 1024;
		RT_LOGW("the size is too large[%d x %d], so we reset vbv size and thresh_size to: %d, %d",
			venc_comp->base_config.dst_width, venc_comp->base_config.dst_height,
			vbv_size, thresh_size);
	}

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetVbvSize, &vbv_size);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetFrameLenThreshold, &thresh_size);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int vencoder_create(venc_comp_ctx *venc_comp)
{
	int ret = 0;
	VENC_CODEC_TYPE type = VENC_CODEC_H264;

	/* map codec type */
	switch (venc_comp->base_config.codec_type) {
	case RT_VENC_CODEC_H264: {
		type = VENC_CODEC_H264;
		break;
	}
	case RT_VENC_CODEC_H265: {
		type = VENC_CODEC_H265;
		break;
	}
	case RT_VENC_CODEC_JPEG: {
		type = VENC_CODEC_JPEG;
		break;
	}
	default: {
		RT_LOGE("nor support codec type: %d", venc_comp->base_config.codec_type);
		return ERROR_TYPE_ILLEGAL_PARAM;
		break;
	}
	}
	LOCK_MUTEX(&venc_mutex);
	venc_comp->vencoder = VencCreate(type);
	cdc_log_set_level(CONFIG_RT_MEDIA_CDC_LOG_LEVEL);
	if (venc_comp->vencoder) {
		if (VENC_CODEC_H264 == type || VENC_CODEC_H265 == type) {
			if (PRODUCT_NUM <= venc_comp->base_config.product_mode ||
				0 == venc_comp->base_config.bit_rate || 0 == venc_comp->base_config.frame_rate ||
				0 == venc_comp->base_config.dst_width || 0 == venc_comp->base_config.dst_height) {
				RT_LOGW("fatal error! channel[%d] invalid params, product_mode=%d, bitrate=%d, frame_rate=%d, dst_wxh=%dx%d",
					venc_comp->base_config.channel_id, venc_comp->base_config.product_mode, venc_comp->base_config.bit_rate,
					venc_comp->base_config.frame_rate, venc_comp->base_config.dst_width, venc_comp->base_config.dst_height);
				UNLOCK_MUTEX(&venc_mutex);
				return ERROR_TYPE_ILLEGAL_PARAM;
			}
			RT_LOGW("channel[%d] set product_mode=%d, bitrate=%d, frame_rate=%d, dst_wxh=%dx%d",
				venc_comp->base_config.channel_id, venc_comp->base_config.product_mode, venc_comp->base_config.bit_rate,
				venc_comp->base_config.frame_rate, venc_comp->base_config.dst_width, venc_comp->base_config.dst_height);
			VencProductModeInfo ProductInfo;
			memset(&ProductInfo, 0, sizeof(VencProductModeInfo));
			ProductInfo.eProductMode = venc_comp->base_config.product_mode;
			ProductInfo.nBitrate = venc_comp->base_config.bit_rate;
			ProductInfo.nFrameRate = venc_comp->base_config.frame_rate;
			ProductInfo.nDstWidth = venc_comp->base_config.dst_width;
			ProductInfo.nDstHeight = venc_comp->base_config.dst_height;
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamProductCase, (void *)&ProductInfo);
			if (ret)
				RT_LOGE("fatal error! channel[%d] set ProductCase failed, ret=%d", venc_comp->base_config.channel_id, ret);

			VencIspVeLinkParam stIspVeLink;
			memset(&stIspVeLink, 0, sizeof(stIspVeLink));
			stIspVeLink.isp2VeEn = (unsigned char)venc_comp->base_config.enable_isp2ve_linkage;
			stIspVeLink.ve2IspEn = (unsigned char)venc_comp->base_config.enable_ve2isp_linkage;
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamIspVeLinkParam, (void *)&stIspVeLink);
			if (ret) {
				RT_LOGE("fatal error! rt_media_chn[%d] set ispveLink param fail[%d]", venc_comp->base_config.channel_id, ret);
			}

			//if ve2isp is disable, no need enable RegionDetectLinkParam.
			if (!venc_comp->base_config.enable_ve2isp_linkage) {
				sRegionLinkParam stRegionLinkParam;
				memset(&stRegionLinkParam, 0, sizeof(stRegionLinkParam));
				VencSetParameter(venc_comp->vencoder, VENC_IndexParamRegionDetectLinkParam, (void *)&stRegionLinkParam);
			}
		}
	}
	UNLOCK_MUTEX(&venc_mutex);
	return ret;
}

int rt_which_sensor(int vipp_num)
{
	switch (vipp_num) {
	case CSI_SENSOR_0_VIPP_0:
	case CSI_SENSOR_0_VIPP_1:
	case CSI_SENSOR_0_VIPP_2:
	case CSI_SENSOR_0_VIPP_3:
		return 0;
	case CSI_SENSOR_1_VIPP_0:
	case CSI_SENSOR_1_VIPP_1:
	case CSI_SENSOR_1_VIPP_2:
	case CSI_SENSOR_1_VIPP_3:
		return 1;
	case CSI_SENSOR_2_VIPP_0:
	case CSI_SENSOR_2_VIPP_1:
	case CSI_SENSOR_2_VIPP_2:
	case CSI_SENSOR_2_VIPP_3:
		return 2;
	default:
		return 0;//default sensor 0
	}

	return 0;//default sensor 0
}

static int rt_which_bk_id(int vipp_num)
{
	switch (vipp_num) {
	case CSI_SENSOR_0_VIPP_0:
	case CSI_SENSOR_1_VIPP_0:
	case CSI_SENSOR_2_VIPP_0:
		return 0;
	case CSI_SENSOR_0_VIPP_1:
	case CSI_SENSOR_1_VIPP_1:
	case CSI_SENSOR_2_VIPP_1:
		return 1;
	default:
		RT_LOGW("vipp_num %d not config bk id!!", vipp_num);
		return -1;
	}
	RT_LOGW("vipp_num %d not config bk id!!", vipp_num);
	return -1;
}
static int vencoder_init(venc_comp_ctx *venc_comp)
{
	VencBaseConfig base_config;
	int en_encpp = 0;
	int vipp_id = venc_comp->base_config.channel_id;

	memset(&base_config, 0, sizeof(VencBaseConfig));

	base_config.bOnlineMode		   = 1; //venc_comp->base_config.bOnlineMode;
	base_config.bOnlineChannel	 = venc_comp->base_config.bOnlineChannel;
	base_config.nOnlineShareBufNum = venc_comp->base_config.share_buf_num;
	base_config.bEncH264Nalu	   = 0;
	base_config.nInputWidth		   = venc_comp->base_config.src_width;
	base_config.nInputHeight	   = venc_comp->base_config.src_height;
	base_config.nDstWidth		   = venc_comp->base_config.dst_width;
	base_config.nDstHeight		   = venc_comp->base_config.dst_height;
	base_config.nStride		   = RT_ALIGN(base_config.nInputWidth, 16);
	base_config.eInputFormat	   = venc_comp->base_config.pixelformat;
	base_config.eOutputFormat      = venc_comp->base_config.outputformat;
	base_config.bOnlyWbFlag		   = 0;
	base_config.memops		   = NULL;
	base_config.veOpsS		   = NULL;
	base_config.bLbcLossyComEnFlag2x   = 0;
	base_config.bLbcLossyComEnFlag2_5x = 0;
	base_config.bIsVbvNoCache = 1;
	base_config.channel_id = venc_comp->base_config.channel_id;
	base_config.sensor_id = rt_which_sensor(base_config.channel_id);
	base_config.bk_id = rt_which_bk_id(base_config.channel_id);

	if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2_5x = 1;
	} else if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2x = 1;
	}

	RT_LOGD("channel_id %d bOnlineChannel %d input format = %d, 2_5x_flag = %d, enable_ve_isp_linkage = %d-%d",
		venc_comp->base_config.channel_id, base_config.bOnlineChannel, base_config.eInputFormat,
		base_config.bLbcLossyComEnFlag2_5x, venc_comp->base_config.enable_isp2ve_linkage,
		venc_comp->base_config.enable_ve2isp_linkage);

	if (venc_comp->base_config.enable_isp2ve_linkage) {
		vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);
		RT_LOGD("vencoder:%px, codec_type:%d, en_encpp %d %d", venc_comp->vencoder, venc_comp->base_config.codec_type,
			en_encpp, venc_comp->base_config.en_encpp_sharp);
		venc_comp->actual_en_encpp_sharp = en_encpp && venc_comp->base_config.en_encpp_sharp;
		RT_LOGD("vencoder:%px skip_sharp_param_frame_num %d", venc_comp->vencoder,
			venc_comp->base_config.skip_sharp_param_frame_num);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, (void *)&venc_comp->actual_en_encpp_sharp);
	}

	/*int64_t time_start = get_cur_time();*/
	LOCK_MUTEX(&venc_mutex);
	VencInit(venc_comp->vencoder, &base_config);
	UNLOCK_MUTEX(&venc_mutex);

	if (venc_comp->base_config.s_wbyuv_param.bEnableWbYuv) {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableWbYuv, &venc_comp->base_config.s_wbyuv_param);
		UNLOCK_MUTEX(&venc_mutex);
	}

	/*
	int64_t time_end = get_cur_time();
	RT_LOGD("time of VideoEncInit: %lld",(time_end - time_start));
	*/

	RT_LOGD("VencInit finish");

#if TEST_ENCODER_BY_SELF
	VencAllocateBufferParam bufferParam;
	memset(&bufferParam, 0, sizeof(VencAllocateBufferParam));
	bufferParam.nSizeY     = base_config.nInputWidth * base_config.nInputHeight;
	bufferParam.nSizeC     = base_config.nInputWidth * base_config.nInputHeight / 2;
	bufferParam.nBufferNum = 1;
	AllocInputBuffer(venc_comp->vencoder, &bufferParam);
#endif

	return 0;
}

//static VENC_H264PROFILETYPE match_h264_profile(int src_profile)
//{
//	VENC_H264PROFILETYPE h264_profile = VENC_H264ProfileHigh;

//	switch (src_profile) {
//	case AW_Video_H264ProfileBaseline:
//	case AW_Video_H264ProfileMain:
//	case AW_Video_H264ProfileHigh:
//		h264_profile = src_profile;
//		break;
//	default: {
//		RT_LOGW("can not match the h264 profile: %d, use defaut: %d", src_profile, h264_profile);
//		break;
//	}
//	}

//	return h264_profile;
//}

//static VENC_H264LEVELTYPE match_h264_level(int src_level)
//{
//	VENC_H264LEVELTYPE h264_level = VENC_H264Level51;

//	switch (src_level) {
//	case AW_Video_H264Level1:
//	case AW_Video_H264Level11:
//	case AW_Video_H264Level12:
//	case AW_Video_H264Level13:
//	case AW_Video_H264Level2:
//	case AW_Video_H264Level21:
//	case AW_Video_H264Level22:
//	case AW_Video_H264Level3:
//	case AW_Video_H264Level31:
//	case AW_Video_H264Level32:
//	case AW_Video_H264Level4:
//	case AW_Video_H264Level41:
//	case AW_Video_H264Level42:
//	case AW_Video_H264Level5:
//	case AW_Video_H264Level51:
//		h264_level = src_level;
//		break;
//	default: {
//		RT_LOGW("can not match the h264 level: %d, defaut: %d", src_level, h264_level);
//		break;
//	}
//	}

//	return h264_level;
//}

//static VENC_H265PROFILETYPE match_h265_profile(int src_profile)
//{
//	VENC_H265PROFILETYPE h265_profile = VENC_H265ProfileMain;

//	switch (src_profile) {
//	case AW_Video_H265ProfileMain:
//	case AW_Video_H265ProfileMain10:
//	case AW_Video_H265ProfileMainStill:
//		h265_profile = src_profile;
//		break;
//	default: {
//		RT_LOGW("can not match the h265 profile: %d, defaut: %d", src_profile, h265_profile);
//		break;
//	}
//	}

//	return h265_profile;
//}

//static VENC_H265LEVELTYPE match_h265_level(int src_level)
//{
//	VENC_H265LEVELTYPE h265_level = VENC_H265Level51;

//	switch (src_level) {
//	case AW_Video_H265Level1:
//	case AW_Video_H265Level2:
//	case AW_Video_H265Level21:
//	case AW_Video_H265Level3:
//	case AW_Video_H265Level31:
//	case AW_Video_H265Level41:
//	case AW_Video_H265Level5:
//	case AW_Video_H265Level51:
//	case AW_Video_H265Level52:
//	case AW_Video_H265Level6:
//	case AW_Video_H265Level61:
//	case AW_Video_H265Level62:
//		h265_level = src_level;
//		break;
//	default: {
//		RT_LOGW("can not match the h265 level: %d, defaut: %d", src_level, h265_level);
//		break;
//	}
//	}

//	return h265_level;
//}

static int set_param_h264(venc_comp_ctx *venc_comp)
{
	VencH264VideoTiming video_time;
	VencH264Param param_h264;

	memset(&param_h264, 0, sizeof(VencH264Param));

	param_h264.sProfileLevel.nProfile  = (VENC_H264PROFILETYPE)venc_comp->base_config.profile;
	param_h264.sProfileLevel.nLevel    = (VENC_H264LEVELTYPE)venc_comp->base_config.level;
	param_h264.bEntropyCodingCABAC     = 1;
	param_h264.sQPRange.nMinqp	 = venc_comp->base_config.qp_range.nMinqp;
	param_h264.sQPRange.nMaxqp	 = venc_comp->base_config.qp_range.nMaxqp;
	param_h264.sQPRange.nMinPqp	= venc_comp->base_config.qp_range.nMinPqp;
	param_h264.sQPRange.nMaxPqp	= venc_comp->base_config.qp_range.nMaxPqp;
	param_h264.sQPRange.nQpInit	= venc_comp->base_config.qp_range.nQpInit;
	param_h264.sQPRange.bEnMbQpLimit = venc_comp->base_config.qp_range.bEnMbQpLimit;
	param_h264.nFramerate		   = venc_comp->base_config.frame_rate;
	param_h264.nSrcFramerate	   = venc_comp->base_config.frame_rate;
	param_h264.nBitrate		   = venc_comp->base_config.bit_rate;
	param_h264.nMaxKeyInterval	 = venc_comp->base_config.max_keyframe_interval;
	param_h264.nCodingMode		   = VENC_FRAME_CODING;
	param_h264.sGopParam.bUseGopCtrlEn = 1;
	param_h264.sGopParam.eGopMode      = AW_NORMALP;
	param_h264.breduce_refrecmem = venc_comp->base_config.breduce_refrecmem;
	param_h264.sRcParam.eRcMode = venc_comp->base_config.mRcMode;
	if (venc_comp->base_config.mRcMode == AW_VBR) {
		memcpy(&param_h264.sRcParam.sVbrParam, &venc_comp->base_config.vbr_param, sizeof(VencVbrParam));
	} else if (venc_comp->base_config.mRcMode == AW_FIXQP) {
		memcpy(&param_h264.sRcParam.sFixQp, &venc_comp->stVencFixQp, sizeof(VencFixQP));
	}

	RT_LOGI("h264 param: profile = %d, level = %d, min_qp = %d, max_qp = %d",
		param_h264.sProfileLevel.nProfile,
		param_h264.sProfileLevel.nLevel,
		param_h264.sQPRange.nMinqp,
		param_h264.sQPRange.nMaxqp);

	memset(&video_time, 0, sizeof(VencH264VideoTiming));

	video_time.num_units_in_tick     = 1;
	video_time.time_scale		 = 32;
	video_time.fixed_frame_rate_flag = 1;

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264Param, &param_h264);
	//VideoEncSetParameter(venc_comp->vencoder, VENC_IndexParamH264VideoTiming, &video_time);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264VideoSignal, &venc_comp->base_config.venc_video_signal);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int set_param_h265(venc_comp_ctx *venc_comp)
{
	VencH265Param h265Param;

	memset(&h265Param, 0, sizeof(VencH265Param));

	h265Param.sGopParam.bUseGopCtrlEn = 1;
	h265Param.sGopParam.eGopMode      = AW_NORMALP;
	h265Param.nBitrate		  = venc_comp->base_config.bit_rate;
	h265Param.nFramerate		  = venc_comp->base_config.frame_rate;
	h265Param.nQPInit		  = 35;
	h265Param.idr_period		  = venc_comp->base_config.max_keyframe_interval;
	h265Param.nGopSize		  = h265Param.idr_period;
	h265Param.nIntraPeriod		  = h265Param.idr_period;
	h265Param.sProfileLevel.nProfile  = (VENC_H265PROFILETYPE)venc_comp->base_config.profile;
	h265Param.sProfileLevel.nLevel    = (VENC_H265LEVELTYPE)venc_comp->base_config.level;

	h265Param.sQPRange.nMinqp  = venc_comp->base_config.qp_range.nMinqp;
	h265Param.sQPRange.nMaxqp  = venc_comp->base_config.qp_range.nMaxqp;
	h265Param.sQPRange.nMinPqp = venc_comp->base_config.qp_range.nMinPqp;
	h265Param.sQPRange.nMaxPqp = venc_comp->base_config.qp_range.nMaxPqp;
	h265Param.sQPRange.nQpInit = venc_comp->base_config.qp_range.nQpInit;
	h265Param.sQPRange.bEnMbQpLimit	= venc_comp->base_config.qp_range.bEnMbQpLimit;
	h265Param.breduce_refrecmem = venc_comp->base_config.breduce_refrecmem;

	h265Param.sRcParam.eRcMode = venc_comp->base_config.mRcMode;
	if (venc_comp->base_config.mRcMode == AW_VBR)
		memcpy(&h265Param.sRcParam.sVbrParam, &venc_comp->base_config.vbr_param, sizeof(VencVbrParam));

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH265Param, &h265Param);

	VencSetParameter(venc_comp->vencoder, VENC_IndexParamVUIVideoSignal, &venc_comp->base_config.venc_video_signal);
	UNLOCK_MUTEX(&venc_mutex);
	return 0;
}

static int set_param_jpeg(venc_comp_ctx *venc_comp)
{
	VencJpegVideoSignal sVideoSignal;
	int rotate_angle = venc_comp->base_config.rotate_angle;
	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegEncMode, &venc_comp->base_config.jpg_mode);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegQuality, &venc_comp->base_config.quality);
	if (venc_comp->base_config.jpg_mode) {//mjpg
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamBitrate, &venc_comp->base_config.bit_rate);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamFramerate, &venc_comp->base_config.frame_rate);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetBitRateRange, &venc_comp->base_config.bit_rate_range);
	}

	memset(&sVideoSignal, 0, sizeof(VencJpegVideoSignal));
	sVideoSignal.src_colour_primaries = venc_comp->base_config.venc_video_signal.src_colour_primaries;
	sVideoSignal.dst_colour_primaries = venc_comp->base_config.venc_video_signal.dst_colour_primaries;
	RT_LOGD("src_colour_primaries %d %d", sVideoSignal.src_colour_primaries, sVideoSignal.dst_colour_primaries);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegVideoSignal, &sVideoSignal);
	if (rotate_angle == 90 || rotate_angle == 180 || rotate_angle == 270) {
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRotation, &rotate_angle);
	}
	RT_LOGI("jpg param %d %d %d %d %d %d", venc_comp->base_config.jpg_mode, venc_comp->base_config.quality,
		venc_comp->base_config.frame_rate, venc_comp->base_config.bit_rate,
		venc_comp->base_config.bit_rate_range.bitRateMin, venc_comp->base_config.bit_rate_range.bitRateMax);
	UNLOCK_MUTEX(&venc_mutex);
	return 0;
}

static void check_qp_value(VencQPRange *qp_range)
{
	int min_qp = qp_range->nMinqp;
	int max_qp = qp_range->nMaxqp;

	if (!(min_qp >= 1 && min_qp <= 51)) {
		min_qp = 25;
	}
	if (!(max_qp >= min_qp && max_qp >= 1 && max_qp <= 51)) {
		max_qp = 45;
	}

	qp_range->nMinqp = min_qp;
	qp_range->nMaxqp = max_qp;

	min_qp = qp_range->nMinPqp;
	max_qp = qp_range->nMaxPqp;
	if (!(min_qp >= 1 && min_qp <= 51)) {
		min_qp = 25;
	}
	if (!(max_qp >= min_qp && max_qp >= 1 && max_qp <= 51)) {
		max_qp = 45;
	}

	qp_range->nMinPqp = min_qp;
	qp_range->nMaxPqp = max_qp;

	if (qp_range->nQpInit <= 0 || qp_range->nQpInit > 51)
		qp_range->nQpInit = 35;
}

static int vencoder_set_param(venc_comp_ctx *venc_comp)
{
	// file g2d_file;
	check_qp_value(&venc_comp->base_config.qp_range);

	RT_LOGI("i_qp = %d~%d, p_qp = %d~%d",
		venc_comp->base_config.qp_range.nMinqp, venc_comp->base_config.qp_range.nMaxqp,
		venc_comp->base_config.qp_range.nMinPqp, venc_comp->base_config.qp_range.nMaxPqp);

	if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H264)
		set_param_h264(venc_comp);
	else if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H265)
		set_param_h265(venc_comp);
	else if (venc_comp->base_config.codec_type == RT_VENC_CODEC_JPEG)
		set_param_jpeg(venc_comp);

#if defined(CONFIG_DEBUG_FS)
	VeProcSet sVeProcInfo;
	int channel_id = 0;

	memset(&sVeProcInfo, 0, sizeof(VeProcSet));
	sVeProcInfo.bProcEnable = 1;
	sVeProcInfo.nProcFreq = 10;
	channel_id = venc_comp->base_config.channel_id;
	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamChannelNum, &channel_id);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamProcSet, &sVeProcInfo);
	UNLOCK_MUTEX(&venc_mutex);
#endif

	setup_vbv_buffer_size(venc_comp);

	if (venc_comp->vencoder_init_flag == 0) {
		vencoder_init(venc_comp);
		venc_comp->vencoder_init_flag = 1;
	}

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
	RT_LOGW("enable_overlay = %d", venc_comp->base_config.enable_overlay);
	if (venc_comp->base_config.enable_overlay == 1) {
		if (adjust_native_overlay_info(venc_comp) == 0) {
			LOCK_MUTEX(&venc_mutex);
			g2d_open(0, &g2d_file);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, &global_osd_info->sOverlayInfo);
			g2d_release(0, &g2d_file);
			UNLOCK_MUTEX(&venc_mutex);
		}
	}
#endif

	return 0;
}

/* the input buffer had been used, return it*/
/* valid_list --> empty_list */
static int venc_empty_in_buffer_done(void *venc_comp, video_frame_s *frame_info)
{
	struct venc_comp_ctx *pvenc_comp = (struct venc_comp_ctx *)venc_comp;
	video_frame_s *pframe_info       = (video_frame_s *)frame_info;

	comp_buffer_header_type buffer_header;
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));
	buffer_header.private = pframe_info;

	if (pvenc_comp->in_port_tunnel_info.valid_flag == 0) {
		pvenc_comp->callback->empty_in_buffer_done(
			pvenc_comp->self_comp,
			pvenc_comp->callback_data,
			&buffer_header);
	} else {
		comp_fill_this_out_buffer(pvenc_comp->in_port_tunnel_info.tunnel_comp, &buffer_header);
	}

	RT_LOGI("venc_comp %px channel id %d release input FrameId[%d]", venc_comp, pvenc_comp->base_config.channel_id,
		pframe_info->id);
	return ERROR_TYPE_OK;
}

static int commond_process(struct venc_comp_ctx *venc_comp, message_t *msg)
{
	int cmd_error		      = 0;
	bool bNeedResponse = true;
	int cmd			      = msg->command;
	MessageReply *wait_reply = (MessageReply *)msg->pReply;

	RT_LOGI("cmd process: cmd = %d, state = %d, wait_reply = %p", cmd, venc_comp->state, wait_reply);

	if (cmd == COMP_COMMAND_INIT) {
		if (venc_comp->state != COMP_STATE_IDLE) {
			cmd_error = 1;
		} else {
			vencoder_create(venc_comp);
			venc_comp->state = COMP_STATE_INITIALIZED;
		}
	} else if (cmd == COMP_COMMAND_START) {
		if (venc_comp->state != COMP_STATE_INITIALIZED && venc_comp->state != COMP_STATE_PAUSE) {
			cmd_error = 1;
		} else {
			venc_comp->state = COMP_STATE_EXECUTING;
		}
	} else if (cmd == COMP_COMMAND_PAUSE) {
		venc_comp->state = COMP_STATE_PAUSE;
	} else if (cmd == COMP_COMMAND_STOP) {
		if ((COMP_STATE_EXECUTING == venc_comp->state) || (COMP_STATE_PAUSE == venc_comp->state)) {
			mutex_lock(&venc_comp->in_buf_manager.mutex);
			if (!list_empty(&venc_comp->in_buf_manager.using_frame_list)) {
				int cnt = 0; struct list_head *pList; list_for_each(pList, &venc_comp->in_buf_manager.using_frame_list) { cnt++; }
				RT_LOGE("fatal error! rt_media_chn[%d] venc comp stop, but need wait [%d] frames sent to venclib return!",
					venc_comp->base_config.channel_id, cnt);
			}
			if (!list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
				int cnt = 0; struct list_head *pList; list_for_each(pList, &venc_comp->in_buf_manager.valid_frame_list) { cnt++; }
				RT_LOGW("Be careful! rt_media_chn[%d] venc comp stop, but need release [%d] frames in valid_frame_list!",
					venc_comp->base_config.channel_id, cnt);
				video_frame_node *pFrameNode, *pTmpNode;
				list_for_each_entry_safe(pFrameNode, pTmpNode, &venc_comp->in_buf_manager.valid_frame_list, mList) {
					mutex_unlock(&venc_comp->in_buf_manager.mutex);
					venc_empty_in_buffer_done(venc_comp, &pFrameNode->video_frame);
					mutex_lock(&venc_comp->in_buf_manager.mutex);
					list_move_tail(&pFrameNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
					venc_comp->in_buf_manager.empty_num++;
				}
			}
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			venc_comp->state = COMP_STATE_IDLE;
		} else {
			RT_LOGW("Be careful! rt_venc_comp state[%d]->idle.", venc_comp->state);
			venc_comp->state = COMP_STATE_IDLE;
		}
	} else if (cmd == COMP_COMMAND_VENC_DROP_FRAME) {
		if (0 == venc_comp->base_config.bOnlineChannel) {
			if (venc_comp->mActiveDropNum != 0) {
				RT_LOGW("Be careful! activeDropNum[%d] still != 0, increase %d frames again!", venc_comp->mActiveDropNum, msg->para0);
			}
			venc_comp->mActiveDropNum += msg->para0;
		} else {
			VENC_RESULT_TYPE eVencRet;
			if (venc_comp->vencoder) {
				unsigned int nDropNum = msg->para0;
				eVencRet = VencSetParameter(venc_comp->vencoder, VENC_IndexParamDropFrame, (void *)&nDropNum);
				if (VENC_RESULT_OK == eVencRet) {
					RT_LOGD("rt_media_chn[%d] venc comp drop frame num:%d", venc_comp->base_config.channel_id, nDropNum);
				} else {
					RT_LOGE("fatal error! rt_media_chn[%d] venc comp call dropFrame fail[%d]", venc_comp->base_config.channel_id, eVencRet);
					cmd_error = 1;
				}
			} else {
				RT_LOGE("fatal error! rt_media_chn[%d] venc comp NULL", venc_comp->base_config.channel_id);
				cmd_error = 1;
			}
		}
		bNeedResponse = false;
	} else if (cmd == COMP_COMMAND_VENC_INPUT_FRAME_VALID) {
		bNeedResponse = false;
	} else if (cmd == COMP_COMMAND_VENC_OUTPUT_STREAM_AVAILABLE) {
		bNeedResponse = false;
	} else {
		RT_LOGE("fatal error! rt_media_chn[%d] vencComp unknown cmd=%d", venc_comp->base_config.channel_id, cmd);
	}

	if (bNeedResponse) {
		if (cmd_error == 1) {
			venc_comp->callback->EventHandler(
				venc_comp->self_comp,
				venc_comp->callback_data,
				COMP_EVENT_CMD_ERROR,
				cmd,
				0,
				NULL);
		} else {
			venc_comp->callback->EventHandler(
				venc_comp->self_comp,
				venc_comp->callback_data,
				COMP_EVENT_CMD_COMPLETE,
				cmd,
				0,
				NULL);
		}
	}

	if (wait_reply) {
		RT_LOGD("wait up: cmd = %d, state = %d", cmd, venc_comp->state);
		wait_reply->ReplyResult = cmd_error;
		rt_sem_up(&wait_reply->ReplySem);
	}

	return 0;
}

static int VencCompInputBufferDone(VideoEncoder *pEncoder, void *pAppData, VencCbInputBufferDoneInfo *pBufferDoneInfo)
{
	struct venc_comp_ctx *pvenc_comp = NULL;
	VencInputBuffer *pInputBuffer = NULL;
	struct vin_buffer *vin_buf = NULL;
	rt_component_type *pcomp_type = NULL;
	video_frame_s mvideo_stream;
	int ret = ERROR_TYPE_OK;

	if (pAppData == NULL || pBufferDoneInfo == NULL) {
		RT_LOGE("parama is null");
		return ERROR_TYPE_ERROR;
	}
	pvenc_comp = (struct venc_comp_ctx *)pAppData;
	pInputBuffer = pBufferDoneInfo->pInputBuffer;

	if (0 == pvenc_comp->base_config.bOnlineChannel) {
		mutex_lock(&pvenc_comp->in_buf_manager.mutex);
		if (!list_empty(&pvenc_comp->in_buf_manager.using_frame_list)) {
			video_frame_node *pFrameNode = list_first_entry(&pvenc_comp->in_buf_manager.using_frame_list, video_frame_node, mList);
			if (pInputBuffer->nID != pFrameNode->video_frame.id) {
				RT_LOGE("fatal error! rtVencChn[%d] input buf ID is NOT match[%ld!=%d]", pvenc_comp->base_config.channel_id,
					pInputBuffer->nID, pFrameNode->video_frame.id);
			}
			mutex_unlock(&pvenc_comp->in_buf_manager.mutex);

			ret = venc_empty_in_buffer_done(pvenc_comp, &pFrameNode->video_frame);
			//static int sBufferCnt = 0; sBufferCnt++; if(sBufferCnt%40 == 0) {
			if (pBufferDoneInfo->nResult != VENC_RESULT_OK) {
				static int stream_full_cnt;

				if (pBufferDoneInfo->nResult == VENC_RESULT_BITSTREAM_IS_FULL) {
					RT_LOGD("bitstream is full , please save or drop it.");
				} else if (pBufferDoneInfo->nResult == VENC_RESULT_CONTINUE) {
					RT_LOGE("fatal error! rt_media_chn[%d] offline encode fail:%d, maybe some source not ready, contine.",
						pvenc_comp->base_config.channel_id, pBufferDoneInfo->nResult);
				} else {
					RT_LOGE("Be careful! rt_media_chn[%d] encode fail:%d", pvenc_comp->base_config.channel_id,
						pBufferDoneInfo->nResult);
				}

				if (pBufferDoneInfo->nResult == VENC_RESULT_BITSTREAM_IS_FULL) {
					stream_full_cnt++;
					if (stream_full_cnt % 500 == 0)
						RT_LOGW("rt_media_chn[%d] bitstream is full 500 cnt, please check! toal cnt %d", pvenc_comp->base_config.channel_id,
							stream_full_cnt);
				} else if (pBufferDoneInfo->nResult == VENC_RESULT_DROP_FRAME) {
					pvenc_comp->callback->EventHandler(
							pvenc_comp->self_comp,
							pvenc_comp->callback_data,
							COMP_EVENT_VENC_DROP_FRAME,
							0,
							0,
							NULL);
				} else if (VENC_RESULT_USER_DROP_FRAME == pBufferDoneInfo->nResult) {
					/*#ifdef TEST_I_FRAME_SYNC
					if (0 == pvenc_comp->base_config.channel_id) {
						pvenc_comp->callback->EventHandler(
							pvenc_comp->self_comp,
							pvenc_comp->callback_data,
							COMP_EVENT_VENC_DROP_FRAME,
							0,
							0,
							NULL);
					}
					#endif*/
					RT_LOGE("fatal error! rt_media_chn[%d] offline encode result=%d(user_drop_frame), impossible, check code!",
						pvenc_comp->base_config.channel_id, pBufferDoneInfo->nResult);
				}
			}

			mutex_lock(&pvenc_comp->in_buf_manager.mutex);
			list_move_tail(&pFrameNode->mList, &pvenc_comp->in_buf_manager.empty_frame_list);
			pvenc_comp->in_buf_manager.empty_num++;
		}
		mutex_unlock(&pvenc_comp->in_buf_manager.mutex);
	} else {
		if (pBufferDoneInfo->nResult != VENC_RESULT_OK) {
			if (VENC_RESULT_DROP_FRAME == pBufferDoneInfo->nResult) {
				RT_LOGW("rt_media_chn[%d] venc online encode result=%d(drop_frame)!", pvenc_comp->base_config.channel_id,
					pBufferDoneInfo->nResult);
				pvenc_comp->callback->EventHandler(pvenc_comp->self_comp, pvenc_comp->callback_data, COMP_EVENT_VENC_DROP_FRAME, 0,
					0, NULL);
			} else if (VENC_RESULT_USER_DROP_FRAME == pBufferDoneInfo->nResult) {
				#ifdef TEST_I_FRAME_SYNC
				if (0 == pvenc_comp->base_config.channel_id) {
					pvenc_comp->callback->EventHandler(
						pvenc_comp->self_comp,
						pvenc_comp->callback_data,
						COMP_EVENT_VENC_DROP_FRAME,
						0,
						0,
						NULL);
				}
				#endif
				RT_LOGD("rt_media_chn[%d] online encode result=%d(user_drop_frame)", pvenc_comp->base_config.channel_id,
					pBufferDoneInfo->nResult);
			} else if (pBufferDoneInfo->nResult == VENC_RESULT_CONTINUE) {
				RT_LOGE("fatal error! rt_media_chn[%d] online encode fail:%d, maybe some source not ready, contine.",
					pvenc_comp->base_config.channel_id, pBufferDoneInfo->nResult);
			}
		}
	}

	mutex_lock(&pvenc_comp->WaitOutputStreamLock);
	if (pvenc_comp->bWaitOutputStreamFlag) {
		pvenc_comp->bWaitOutputStreamFlag = false;
		message_t msg;
		msg.command = COMP_COMMAND_VENC_OUTPUT_STREAM_AVAILABLE;
		put_message(&pvenc_comp->msg_queue, &msg);
		RT_LOGD("rt_media_chn[%d] vencComp send message out stream available", pvenc_comp->base_config.channel_id);
	}
	mutex_unlock(&pvenc_comp->WaitOutputStreamLock);

	mutex_lock(&pvenc_comp->SEILock);
	if (pvenc_comp->stSEIAttr.bEnable) {
		rt_sem_up_unique(&pvenc_comp->stSetSEISem);
		pvenc_comp->nSetSEICount++;
	}
	mutex_unlock(&pvenc_comp->SEILock);

	return ret;
}
/* empty_list --> valid_list */
static int venc_fill_out_buffer_done(struct venc_comp_ctx *venc_comp,
					 VencOutputBuffer *out_buffer)
{
	video_stream_node *stream_node = NULL;
	error_type error	       = ERROR_TYPE_OK;
	comp_buffer_header_type buffer_header;
	video_stream_s mvideo_stream;

	memset(&mvideo_stream, 0, sizeof(video_stream_s));
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));

	mutex_lock(&venc_comp->out_buf_manager.mutex);
	stream_node = list_first_entry(&venc_comp->out_buf_manager.empty_stream_list, video_stream_node, mList);

	config_videostream_by_VencOutputBuffer(venc_comp, out_buffer, &stream_node->video_stream);
	memcpy(&mvideo_stream, &stream_node->video_stream, sizeof(video_stream_s));

	venc_comp->out_buf_manager.empty_num--;
	list_move_tail(&stream_node->mList, &venc_comp->out_buf_manager.valid_stream_list);
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	buffer_header.private = &mvideo_stream;
	if (venc_comp->out_port_tunnel_info.valid_flag == 0) {
		venc_comp->callback->fill_out_buffer_done(
			venc_comp->self_comp,
			venc_comp->callback_data,
			&buffer_header);
	} else {
		comp_empty_this_in_buffer(venc_comp->out_port_tunnel_info.tunnel_comp, &buffer_header);
	}

	RT_LOGD(" id = %d", mvideo_stream.id);
	return error;
}

#if NEI_TEST_CODE
static void init_jpeg_exif(EXIFInfo *exifinfo)
{
	exifinfo->ThumbWidth = 640;
	exifinfo->ThumbHeight = 480;

	strcpy((char *)exifinfo->CameraMake,        "allwinner make test");
	strcpy((char *)exifinfo->CameraModel,        "allwinner model test");
	strcpy((char *)exifinfo->DateTime,         "2014:02:21 10:54:05");
	strcpy((char *)exifinfo->gpsProcessingMethod,  "allwinner gps");

	exifinfo->Orientation = 0;

	exifinfo->ExposureTime.num = 2;
	exifinfo->ExposureTime.den = 1000;

	exifinfo->FNumber.num = 20;
	exifinfo->FNumber.den = 10;
	exifinfo->ISOSpeed = 50;

	exifinfo->ExposureBiasValue.num = -4;
	exifinfo->ExposureBiasValue.den = 1;

	exifinfo->MeteringMode = 1;
	exifinfo->FlashUsed = 0;

	exifinfo->FocalLength.num = 1400;
	exifinfo->FocalLength.den = 100;

	exifinfo->DigitalZoomRatio.num = 4;
	exifinfo->DigitalZoomRatio.den = 1;

	exifinfo->WhiteBalance = 1;
	exifinfo->ExposureMode = 1;

	exifinfo->enableGpsInfo = 1;

	exifinfo->gps_latitude = 23.2368;
	exifinfo->gps_longitude = 24.3244;
	exifinfo->gps_altitude = 1234.5;

	exifinfo->gps_timestamp = 0;

	strcpy((char *)exifinfo->CameraSerialNum,  "123456789");
	strcpy((char *)exifinfo->ImageName,  "exif-name-test");
	strcpy((char *)exifinfo->ImageDescription,  "exif-descriptor-test");
}
#endif

static int catch_jpeg_encoder(venc_comp_ctx *venc_comp, VencInputBuffer *in_buf)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	int result		 = 0;

	result = VencQueueInputBuf(jpeg_cxt->vencoder, in_buf);
	if (result != 0) {
		RT_LOGE("fatal error! VencQueueInputBuf fail[%d]", result);
	}

	LOCK_MUTEX(&venc_mutex);
	result = encodeOneFrame(jpeg_cxt->vencoder);
	UNLOCK_MUTEX(&venc_mutex);

	RT_LOGI("encoder result = %d", result);

	jpeg_cxt->wait_enc_finish_condition = 1;
	wake_up(&jpeg_cxt->wait_enc_finish);

	jpeg_cxt->encoder_finish_flag = 1;

	return 0;
}

static int catch_jpeg_start(venc_comp_ctx *venc_comp, catch_jpeg_config *jpeg_config)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	VENC_CODEC_TYPE type     = VENC_CODEC_JPEG;
	VencBaseConfig base_config;
	unsigned int vbv_size = jpeg_config->width * jpeg_config->height / 2;
	unsigned int quality  = jpeg_config->qp;

	jpeg_cxt->width  = jpeg_config->width;
	jpeg_cxt->height = jpeg_config->height;
	jpeg_cxt->qp     = jpeg_config->qp;

	LOCK_MUTEX(&venc_mutex);
	jpeg_cxt->vencoder = VencCreate(type);
	UNLOCK_MUTEX(&venc_mutex);

	if (!jpeg_cxt->vencoder) {
		RT_LOGE("create vencoder failed");
		return -1;
	}

/* setup exif */
#if NEI_TEST_CODE
	EXIFInfo exif_info;
	memset(&exif_info, 0, sizeof(EXIFInfo));
	init_jpeg_exif(&exif_info);
	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamJpegExifInfo, &exif_info);
#endif

	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamSetVbvSize, &vbv_size);
	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamJpegQuality, &quality);

	memset(&base_config, 0, sizeof(VencBaseConfig));

	base_config.bEncH264Nalu	   = 0;
	base_config.nInputWidth		   = venc_comp->base_config.src_width;
	base_config.nInputHeight	   = venc_comp->base_config.src_height;
	base_config.nDstWidth		   = jpeg_cxt->width;
	base_config.nDstHeight		   = jpeg_cxt->height;
	base_config.nStride		       = RT_ALIGN(base_config.nInputWidth, 16);
	base_config.eInputFormat	   = venc_comp->base_config.pixelformat;
	base_config.eOutputFormat	   = venc_comp->base_config.outputformat;
	base_config.bOnlyWbFlag		   = 0;
	base_config.memops		   = NULL;
	base_config.veOpsS		   = NULL;
	base_config.bLbcLossyComEnFlag2x   = 0;
	base_config.bLbcLossyComEnFlag2_5x = 0;

	if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2_5x = 1;
	} else if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2x = 1;
	}

	LOCK_MUTEX(&venc_mutex);
	VencInit(jpeg_cxt->vencoder, &base_config);
	UNLOCK_MUTEX(&venc_mutex);

	/* timeout is 2000 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
	jpeg_cxt->wait_enc_finish_condition = 0;
	jpeg_cxt->enable		    = 1;
	wait_event_timeout(jpeg_cxt->wait_enc_finish, jpeg_cxt->wait_enc_finish_condition, msecs_to_jiffies(2000));

	if (jpeg_cxt->wait_enc_finish_condition == 0) {
		RT_LOGE("wait for enc finish timeout");
		return -1;
	}

	return 0;
}

static int catch_jpeg_stop(venc_comp_ctx *venc_comp)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;

	if (jpeg_cxt->vencoder) {
		LOCK_MUTEX(&venc_mutex);
		VencDestroy(jpeg_cxt->vencoder);
		jpeg_cxt->vencoder = NULL;
		UNLOCK_MUTEX(&venc_mutex);
	} else {
		RT_LOGE("the jpeg_cxt->vencoder is null");
		return -1;
	}

	jpeg_cxt->enable = 0;

	return 0;
}

static int catch_jpeg_get_data(venc_comp_ctx *venc_comp, void *user_buf_info)
{
	int result = 0;
	VencOutputBuffer out_buffer;
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	catch_jpeg_buf_info src_buf_info;

	if (jpeg_cxt->enable == 0 || !jpeg_cxt->vencoder) {
		RT_LOGE("error: enable = %d, vencoder = %px", jpeg_cxt->enable, jpeg_cxt->vencoder);
		return -1;
	}

	memset(&src_buf_info, 0, sizeof(catch_jpeg_buf_info));
	memset(&out_buffer, 0, sizeof(VencOutputBuffer));

	if (copy_from_user(&src_buf_info, (void __user *)user_buf_info, sizeof(struct catch_jpeg_buf_info))) {
		RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
		return -EFAULT;
	}

	result = VencDequeueOutputBuf(jpeg_cxt->vencoder, &out_buffer);
	if (result != 0) {
		RT_LOGE("have no bitstream");
		return -1;
	}

	RT_LOGI("data_size = %d, max_size = %d",
		out_buffer.nSize0, src_buf_info.buf_size);

	if (out_buffer.nSize0 > src_buf_info.buf_size) {
		RT_LOGE("buf_size overflow: %d > %d", out_buffer.nSize0, src_buf_info.buf_size);
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

	if (copy_to_user((void *)src_buf_info.buf, out_buffer.pData0, out_buffer.nSize0)) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

	if (copy_to_user((void *)user_buf_info, &out_buffer.nSize0, sizeof(unsigned int))) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

catch_jpeg_get_data_exit:

	VencQueueOutputBuf(jpeg_cxt->vencoder, &out_buffer);

	return result;
}

typedef struct osd_convert_dst_info {
	unsigned int dst_ext_buf_size;
	unsigned int dst_w_ext;
	unsigned int dst_h_ext;
	unsigned int dst_crop_x;
	unsigned int dst_crop_y;
	unsigned int dst_crop_w;
	unsigned int dst_crop_h;
	unsigned int dst_start_x;
	unsigned int dst_start_y;
} osd_convert_dst_info;

typedef struct osd_convert_src_info {
	unsigned int start_x;
	unsigned int start_y;
	unsigned int widht;
	unsigned int height;
} osd_convert_src_info;

#if NEI_TEST_CODE
static int convert_osd_pos_info(venc_comp_ctx *venc_comp, osd_convert_dst_info *dst_info, osd_convert_src_info *src_info)
{
	/* 1X: 100 */
	unsigned int w_ratio	= 100;
	unsigned int h_ratio	= 100;
	unsigned int is_left_corner = 0;

	if (global_osd_info->ori_frame_w != venc_comp->base_config.dst_width || global_osd_info->ori_frame_h != venc_comp->base_config.dst_height) {
		w_ratio = venc_comp->base_config.dst_width * 100 / global_osd_info->ori_frame_w;
		h_ratio = venc_comp->base_config.dst_height * 100 / global_osd_info->ori_frame_h;
		RT_LOGI("ori_w&h = %d, %d, cur_w&h = %d, %d; w_ratio = %d, h_ratio = %d",
			global_osd_info->ori_frame_w, global_osd_info->ori_frame_h,
			venc_comp->base_config.dst_width, venc_comp->base_config.dst_height,
			w_ratio, h_ratio);
	}

	if (src_info->start_x > 160)
		is_left_corner = 0;
	else
		is_left_corner = 1;

	/* compulte the scale ratio */
	if (is_left_corner == 1) {
		dst_info->dst_start_x = 0;
		dst_info->dst_start_y = 0;
		dst_info->dst_w_ext   = src_info->start_x + src_info->widht;
		dst_info->dst_h_ext   = src_info->start_y + src_info->height;
		dst_info->dst_crop_x  = src_info->start_x;
		dst_info->dst_crop_y  = src_info->start_y;
		dst_info->dst_crop_w  = src_info->widht;
		dst_info->dst_crop_h  = src_info->height;

		dst_info->dst_w_ext  = dst_info->dst_w_ext * w_ratio / 100;
		dst_info->dst_h_ext  = dst_info->dst_h_ext * h_ratio / 100;
		dst_info->dst_crop_x = dst_info->dst_crop_x * w_ratio / 100;
		dst_info->dst_crop_y = dst_info->dst_crop_y * h_ratio / 100;
		dst_info->dst_crop_w = dst_info->dst_crop_w * w_ratio / 100;
		dst_info->dst_crop_h = dst_info->dst_crop_h * h_ratio / 100;
	} else {
		dst_info->dst_start_y = 0;
		dst_info->dst_start_x = src_info->start_x;
		dst_info->dst_start_x = (dst_info->dst_start_x / 16) * 16;

		dst_info->dst_w_ext = global_osd_info->ori_frame_w - dst_info->dst_start_x;
		dst_info->dst_h_ext = src_info->start_y + src_info->height;

		dst_info->dst_crop_x = src_info->start_x - dst_info->dst_start_x;
		dst_info->dst_crop_y = src_info->start_y;
		dst_info->dst_crop_w = src_info->widht;
		dst_info->dst_crop_h = src_info->height;

		if (w_ratio != 100) {
			unsigned int dst_start_x_ori = 0;

			dst_start_x_ori     = dst_info->dst_start_x * w_ratio / 100;
			dst_info->dst_w_ext = dst_info->dst_w_ext * w_ratio / 100;
			dst_info->dst_h_ext = dst_info->dst_h_ext * h_ratio / 100;

			dst_info->dst_start_x = (dst_start_x_ori / 16) * 16;
			dst_info->dst_w_ext   = venc_comp->base_config.dst_width - dst_info->dst_start_x;

			dst_info->dst_crop_x = dst_info->dst_crop_x * w_ratio / 100;
			dst_info->dst_crop_y = dst_info->dst_crop_y * h_ratio / 100;
			dst_info->dst_crop_w = dst_info->dst_crop_w * w_ratio / 100;
			dst_info->dst_crop_h = dst_info->dst_crop_h * h_ratio / 100;

			dst_info->dst_crop_x += dst_start_x_ori - dst_info->dst_start_x;
		}
	}

	dst_info->dst_w_ext = RT_ALIGN(dst_info->dst_w_ext, 16);
	dst_info->dst_h_ext = RT_ALIGN(dst_info->dst_h_ext, 16);

	dst_info->dst_ext_buf_size = dst_info->dst_w_ext * dst_info->dst_h_ext * 4;

	RT_LOGI("is_left = %d, dst_w&h_e = %d, %d; crop = %d, %d, %d, %d; ratio = %d, %d",
		is_left_corner, dst_info->dst_w_ext, dst_info->dst_h_ext,
		dst_info->dst_crop_x, dst_info->dst_crop_y, dst_info->dst_crop_w, dst_info->dst_crop_h,
		w_ratio, h_ratio);

	return 0;
}
#endif
#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
int convert_overlay_info_native(venc_comp_ctx *venc_comp, VencOverlayInfoS *pvenc_osd, VideoInputOSD *user_osd_info)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	OverlayItemInfo *src_item    = NULL;
	int align_end_x		     = 0;
	int align_end_y		     = 0;
	struct mem_param param;
	unsigned int in_phy_addr = 0;
	osd_convert_dst_info osd_dst_info;
	osd_convert_src_info osd_src_info;

	memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
	memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

	if (global_osd_info == NULL) {
		RT_LOGW("the global_osd_info is null");
		return -1;
	}

	if (global_osd_info->memops == NULL) {
		global_osd_info->memops = mem_create(MEM_TYPE_ION, param);
		if (global_osd_info->memops == NULL) {
			RT_LOGE("mem_create failed\n");
			return -1;
		}
	}

	if (global_osd_info->ori_frame_w == 0 || global_osd_info->ori_frame_h == 0) {
		global_osd_info->ori_frame_w = venc_comp->base_config.dst_width;
		global_osd_info->ori_frame_h = venc_comp->base_config.dst_height;
	}

	pvenc_osd->blk_num	  = user_osd_info->osd_num;
	pvenc_osd->argb_type	= user_osd_info->argb_type;

	for (i = 0; i < pvenc_osd->blk_num; i++) {
		if (i >= GLOBAL_OSD_MAX_NUM) {
			RT_LOGW("osd num overlay: blk_num = %d, GLOBAL_OSD_MAX_NUM = %d",
				pvenc_osd->blk_num, GLOBAL_OSD_MAX_NUM);
			break;
		}
		dst_item = &pvenc_osd->overlayHeaderList[i];
		src_item = &user_osd_info->item_info[i];
		/* copy osd data to native */
		if (global_osd_info->g2d_in_ion_vir_addr[i] == NULL || global_osd_info->g2d_in_buf_size[i] < src_item->data_size) {
			if (global_osd_info->g2d_in_ion_vir_addr[i])
				cdc_mem_pfree(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i]);

			global_osd_info->g2d_in_ion_vir_addr[i] = cdc_mem_palloc(global_osd_info->memops, src_item->data_size);
			global_osd_info->g2d_in_buf_size[i]     = src_item->data_size;
		}
		if (copy_from_user(global_osd_info->g2d_in_ion_vir_addr[i], (void __user *)src_item->data_buf, src_item->data_size)) {
			RT_LOGE("g2d_scale copy_from_user fail\n");
			return -1;
		}

		cdc_mem_flush_cache(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i], src_item->data_size);

		in_phy_addr = cdc_mem_get_phy(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i]);

		memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
		memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

		osd_src_info.start_x = src_item->start_x;
		osd_src_info.start_y = src_item->start_y;
		osd_src_info.widht   = src_item->widht;
		osd_src_info.height  = src_item->height;

		convert_osd_pos_info(venc_comp, &osd_dst_info, &osd_src_info);

		align_end_x = osd_dst_info.dst_start_x + osd_dst_info.dst_w_ext;
		align_end_y = osd_dst_info.dst_start_y + osd_dst_info.dst_h_ext;

		dst_item->start_mb_x = osd_dst_info.dst_start_x / 16;
		dst_item->start_mb_y = osd_dst_info.dst_start_y / 16;
		dst_item->end_mb_x   = align_end_x / 16 - 1;
		dst_item->end_mb_y   = align_end_y / 16 - 1;

		dst_item->overlay_blk_addr    = src_item->data_buf;
		dst_item->bitmap_size	 = osd_dst_info.dst_ext_buf_size;
		dst_item->overlay_type	= NORMAL_OVERLAY;
		dst_item->extra_alpha_flag    = 0;
		dst_item->bforce_reverse_flag = 0;
		RT_LOGI("osd item[%d]: s_x&y = %d, %d; e_x&y = %d, %d, size = %d, buf = %px",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}

	return 0;
}

static int adjust_native_overlay_info(venc_comp_ctx *venc_comp)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	int align_end_x		     = 0;
	int align_end_y		     = 0;
	osd_convert_dst_info osd_dst_info;
	osd_convert_src_info osd_src_info;

	memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
	memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

	if (global_osd_info == NULL) {
		RT_LOGW("the global_osd_info is null");
		return -1;
	}

	for (i = 0; i < global_osd_info->sOverlayInfo.blk_num; i++) {
		dst_item = &global_osd_info->sOverlayInfo.overlayHeaderList[i];

		memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
		memset(&osd_src_info, 0, sizeof(osd_convert_src_info));
		convert_osd_pos_info(venc_comp, &osd_dst_info, &osd_src_info);

		align_end_x = osd_dst_info.dst_start_x + osd_dst_info.dst_w_ext;
		align_end_y = osd_dst_info.dst_start_y + osd_dst_info.dst_h_ext;

		dst_item->start_mb_x = osd_dst_info.dst_start_x / 16;
		dst_item->start_mb_y = osd_dst_info.dst_start_y / 16;
		dst_item->end_mb_x   = align_end_x / 16 - 1;
		dst_item->end_mb_y   = align_end_y / 16 - 1;

		dst_item->bitmap_size = osd_dst_info.dst_ext_buf_size;
		RT_LOGI("osd item[%d]: s_x&y = %d, %d; e_x&y = %d, %d, size = %d, buf = %px",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}
	return 0;
}

#else
static int convert_overlay_info(VencOverlayInfoS *pvenc_osd, VideoInputOSD *user_osd_info)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	OverlayItemInfo *src_item    = NULL;
	int align_start_x	    = 0;
	int align_start_y	    = 0;
	int align_end_x		     = 0;
	int align_end_y		     = 0;

	pvenc_osd->blk_num	  = user_osd_info->osd_num;
	pvenc_osd->argb_type	= user_osd_info->argb_type;
	pvenc_osd->invert_mode = user_osd_info->invert_mode;
	pvenc_osd->invert_threshold = user_osd_info->invert_threshold;

	for (i = 0; i < pvenc_osd->blk_num; i++) {
		dst_item = &pvenc_osd->overlayHeaderList[i];
		src_item = &user_osd_info->item_info[i];

		align_start_x = RT_ALIGN(src_item->start_x, 16);
		align_start_y = RT_ALIGN(src_item->start_y, 16);
		align_end_x   = align_start_x + src_item->widht;
		align_end_y   = align_start_y + src_item->height;

		if (src_item->data_buf && src_item->data_size
			&& src_item->osd_type != COVER_OVERLAY) {
			if (dst_item->overlay_blk_addr
				&& src_item->data_size > dst_item->bitmap_size) {
				vfree(dst_item->overlay_blk_addr);
				dst_item->overlay_blk_addr = NULL;
				RT_LOGI("update overlay[%d] buf size[%d]->[%d]",
					i, dst_item->bitmap_size, src_item->data_size);
			}
			if (!dst_item->overlay_blk_addr) {
				dst_item->overlay_blk_addr = vmalloc(src_item->data_size);
				memset(dst_item->overlay_blk_addr, 0, src_item->data_size);
			}
			if (dst_item->overlay_blk_addr) {
				if (copy_from_user(dst_item->overlay_blk_addr, (void __user *)src_item->data_buf, src_item->data_size)) {
					RT_LOGE("copy_from_user fail\n");
					return -EFAULT;
				}
			}
		} else {
			dst_item->overlay_blk_addr = NULL;
		}
		dst_item->bitmap_size = src_item->data_size;

		dst_item->start_mb_x       = align_start_x / 16;
		dst_item->start_mb_y       = align_start_y / 16;
		dst_item->end_mb_x	 = align_end_x / 16 - 1;
		dst_item->end_mb_y	 = align_end_y / 16 - 1;

		dst_item->overlay_type	= src_item->osd_type;
		if (dst_item->overlay_type == COVER_OVERLAY)
			memcpy(&dst_item->cover_yuv, &src_item->cover_yuv, sizeof(VencOverlayCoverYuvS));

		dst_item->extra_alpha_flag    = 0;
		dst_item->bforce_reverse_flag = 0;
		RT_LOGI("osd item[%d]: s_x = %d, s_y = %d, e_x = %d, e_y = %d, size = %d, buf = %px",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}
	return 0;
}
#endif
/**
  release frames in valid_frame_list.
  frames in using_frame_list will not be released because vencLib are using them.
*/
static int flush_in_buffer(venc_comp_ctx *venc_comp)
{
	video_frame_node *frame_node = NULL;
	video_frame_s cur_video_frame;

	memset(&cur_video_frame, 0, sizeof(video_frame_s));

	mutex_lock(&venc_comp->in_buf_manager.mutex);
	if (!list_empty(&venc_comp->in_buf_manager.using_frame_list)) {
		int cnt = 0; struct list_head *pList; list_for_each(pList, &venc_comp->in_buf_manager.using_frame_list) { cnt++; }
		RT_LOGW("Be careful! vencChn[%d] flush frames, but [%d] frames have been sent to venclib, these frames are not released here!",
			venc_comp->base_config.channel_id, cnt);
	}
	while (!list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));
		list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;

		RT_LOGW("flush in frame buf, frame_node = %px", frame_node);

		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		venc_empty_in_buffer_done(venc_comp, &cur_video_frame);

		mutex_lock(&venc_comp->in_buf_manager.mutex);
	}
	mutex_unlock(&venc_comp->in_buf_manager.mutex);

	return 0;
}
static error_type config_dynamic_param(
	venc_comp_ctx *venc_comp,
	comp_index_type index,
	void *param_data)
{
	error_type error = ERROR_TYPE_OK;
	int ret		 = 0;

	if (venc_comp->vencoder == NULL) {
		RT_LOGW("vencoder is not be created when config dynamic param");
		return ERROR_TYPE_ERROR;
	}

	switch (index) {
	case COMP_INDEX_VENC_CONFIG_Dynamic_ForceKeyFrame: {
		RT_LOGI("*****ForceKeyFrame***** ");
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamForceKeyFrame, param_data);
		UNLOCK_MUTEX(&venc_mutex);

		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START: {
		catch_jpeg_config *jpeg_config = (catch_jpeg_config *)param_data;
		if (catch_jpeg_start(venc_comp, jpeg_config) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_STOP: {
		if (catch_jpeg_stop(venc_comp) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA: {
		if (catch_jpeg_get_data(venc_comp, param_data) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_OSD: {
#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
		VencOverlayInfoS *pOverlayInfo = &global_osd_info->sOverlayInfo;
		VideoInputOSD *pinputOsd = (VideoInputOSD *)param_data;
		struct file g2d_file;

		memset(pOverlayInfo, 0, sizeof(VencOverlayInfoS));

		convert_overlay_info_native(venc_comp, pOverlayInfo, pinputOsd);

		LOCK_MUTEX(&venc_mutex);
		g2d_open(0, &g2d_file);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, pOverlayInfo);
		g2d_release(0, &g2d_file);
		UNLOCK_MUTEX(&venc_mutex);
#else
		VideoInputOSD *pinputOsd = (VideoInputOSD *)param_data;
		int i = 0;

		if (!venc_comp->venc_overlayInfo) {
			venc_comp->venc_overlayInfo = kmalloc(sizeof(VencOverlayInfoS), GFP_KERNEL);
			memset(venc_comp->venc_overlayInfo, 0, sizeof(VencOverlayInfoS));
		}

		if (!pinputOsd->osd_num) {
			RT_LOGI("user want to disable all venc osd");
			for (i = 0; i < venc_comp->venc_overlayInfo->blk_num; i++) {
				if (venc_comp->venc_overlayInfo->overlayHeaderList[i].overlay_blk_addr) {
					venc_comp->venc_overlayInfo->overlayHeaderList[i].bitmap_size = 0;
					vfree(venc_comp->venc_overlayInfo->overlayHeaderList[i].overlay_blk_addr);
					venc_comp->venc_overlayInfo->overlayHeaderList[i].overlay_blk_addr = NULL;
				}
			}
		}
		convert_overlay_info(venc_comp->venc_overlayInfo, pinputOsd);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, venc_comp->venc_overlayInfo);
		UNLOCK_MUTEX(&venc_mutex);
#endif
		RT_LOGI("ENABLE_SAVE_NATIVE_OVERLAY_DATA = %d", ENABLE_SAVE_NATIVE_OVERLAY_DATA);
		break;
	}
//	case COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE: {
//		rt_venc_bin_image_param *bin_param = (rt_venc_bin_image_param *)param_data;
//		VencBinImageParam venc_bin_param;
//
//		venc_bin_param.enable = bin_param->enable;
//		venc_bin_param.moving_th = bin_param->moving_th;
//
//		RT_LOGI("COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE: enable = %d, th = %d",
//				venc_bin_param.enable, venc_bin_param.moving_th);
//		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableGetBinImage, &venc_bin_param);
//		break;
//	}
//	case COMP_INDEX_VENC_CONFIG_ENABLE_MV_INFO: {
//		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableMvInfo, param_data);
//		break;
//	}
	case COMP_INDEX_VENC_CONFIG_FLUSH_IN_BUFFER: {
		flush_in_buffer(venc_comp);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_QP_RANGE: {
		//VencQPRange mVencQpRange;
		VencQPRange *pQp_range = (VencQPRange *)param_data;

		RT_LOGW("SET_QP_RANGE: i_qp = %d ~ %d, p_qp = %d ~ %d, bEnMbQpLimit = %d",
			pQp_range->nMinqp, pQp_range->nMaxqp,
			pQp_range->nMinPqp, pQp_range->nMaxPqp, pQp_range->bEnMbQpLimit);

//		mVencQpRange.nMinqp       = pQp_range->i_min_qp;
//		mVencQpRange.nMaxqp       = pQp_range->i_max_qp;
//		mVencQpRange.nMinPqp      = pQp_range->p_min_qp;
//		mVencQpRange.nMaxPqp      = pQp_range->p_max_qp;
//		mVencQpRange.nQpInit      = pQp_range->i_init_qp;
//		mVencQpRange.bEnMbQpLimit = pQp_range->enable_mb_qp_limit;

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264QPRange, (void *)pQp_range);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_H264_TIMING: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264VideoTiming, param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_H265_TIMING: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH265Timing, param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_FIX_QP: {
		LOCK_MUTEX(&venc_mutex);
		memcpy(&venc_comp->stVencFixQp, param_data, sizeof(VencFixQP));
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264FixQP, param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_JPG_QUALITY: {
		int nQuality = (int)param_data;

		RT_LOGI("nQuality = %d", nQuality);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegQuality, &nQuality);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_MB_MOVE_STATUS: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnIFrmMbRcMoveStatus, param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_BITRATE: {
		int bitrate = (int)param_data;

		RT_LOGI("SET_BITRATE: bitrate = %d", bitrate);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamBitrate, &bitrate);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS: {
		int fps = (int)param_data;

		RT_LOGI("SET_FPS: fps = %d", fps);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamFramerate, &fps);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_VBR_PARAM: {
		VencVbrParam *rt_vbr_param = (VencVbrParam *)param_data;
		VencVbrParam mVbrParam;

		memset(&mVbrParam, 0, sizeof(VencVbrParam));

		mVbrParam.uMaxBitRate   = rt_vbr_param->uMaxBitRate;
		mVbrParam.nMovingTh     = rt_vbr_param->nMovingTh;
		mVbrParam.nQuality      = rt_vbr_param->nQuality;
		mVbrParam.nIFrmBitsCoef = rt_vbr_param->nIFrmBitsCoef;
		mVbrParam.nPFrmBitsCoef = rt_vbr_param->nPFrmBitsCoef;

		RT_LOGI("SET_VBR_PARAM: uMaxBitRate = %d, nMovingTh = %d, nQuality = %d, coef = %d, %d",
			mVbrParam.uMaxBitRate, mVbrParam.nMovingTh,
			mVbrParam.nQuality, mVbrParam.nIFrmBitsCoef, mVbrParam.nPFrmBitsCoef);

		LOCK_MUTEX(&venc_mutex);
		ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetVbrParam, &mVbrParam);
		if (ret != 0)
			error = ERROR_TYPE_ERROR;
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_SUM_MB_INFO: {
		VencMBSumInfo *p_venc_mb_sum = (VencMBSumInfo *)param_data;

		memset(p_venc_mb_sum, 0, sizeof(VencMBSumInfo));

		ret = VencGetParameter(venc_comp->vencoder, VENC_IndexParamMBSumInfoOutput, (void *)p_venc_mb_sum);
		if (ret != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE: {
		int is_night_case = *(int *)param_data;

		RT_LOGW("is_night_case = %d", is_night_case);

		if (venc_comp->vencoder) {
			LOCK_MUTEX(&venc_mutex);
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamIsNightCaseFlag, &is_night_case);
			if (ret != 0)
				error = ERROR_TYPE_ERROR;
			UNLOCK_MUTEX(&venc_mutex);
		}

		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_SUPER_FRAME_PARAM: {
		VencSuperFrameConfig *pconfig = (VencSuperFrameConfig *)param_data;

		if (venc_comp->vencoder) {
			LOCK_MUTEX(&venc_mutex);
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSuperFrameConfig, pconfig);
			if (ret != 0)
				error = ERROR_TYPE_ERROR;
			UNLOCK_MUTEX(&venc_mutex);
		}

		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_SHARP: {
//		RTsEncppSharp mSharp;
//		RTsEncppSharpParamDynamic dynamic_sharp;
//		RTsEncppSharpParamStatic static_sharp;
//
//		if (copy_from_user(&mSharp, (void __user *)param_data, sizeof(RTsEncppSharp))) {
//			RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_SHARP copy_from_user fail\n");
//			return -EFAULT;
//		}
//
//		if (copy_from_user(&dynamic_sharp, (void __user *)mSharp.sEncppSharpParam.pDynamicParam, sizeof(sEncppSharpParamDynamic))) {
//			RT_LOGE("pDynamicParam copy_from_user fail\n");
//			return -EFAULT;
//		}
//		if (copy_from_user(&static_sharp, (void __user *)mSharp.sEncppSharpParam.pStaticParam, sizeof(sEncppSharpParamStatic))) {
//			RT_LOGE("pStaticParam copy_from_user fail\n");
//			return -EFAULT;
//		}
//
//		mSharp.sEncppSharpParam.pDynamicParam = &dynamic_sharp;
//		mSharp.sEncppSharpParam.pStaticParam = &static_sharp;
//
//		RT_LOGD("hfr_hf_wht_clp %d max_clp_ratio %d", mSharp.sEncppSharpParam.pDynamicParam->hfr_hf_wht_clp, mSharp.sEncppSharpParam.pDynamicParam->max_clp_ratio);
//		RT_LOGD("ls_dir_ratio %d ss_dir_ratio %d", mSharp.sEncppSharpParam.pStaticParam->ls_dir_ratio, mSharp.sEncppSharpParam.pStaticParam->ss_dir_ratio);
//
//		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSharpConfig, &mSharp.sEncppSharpParam);

		int bSharp = *(int *)param_data;
		if (venc_comp->base_config.en_encpp_sharp != bSharp) {
			int vipp_id = venc_comp->base_config.channel_id;
			RT_LOGW("vipp:%d,venc_type:%d: en_encpp_sharp base config change:%d->%d", vipp_id, venc_comp->base_config.codec_type, venc_comp->base_config.en_encpp_sharp, bSharp);
			venc_comp->base_config.en_encpp_sharp = bSharp;
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, (void *)&venc_comp->base_config.en_encpp_sharp);
		}
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CAMERA_MOVE_STATUS: {
		if (!venc_comp->base_config.enable_isp2ve_linkage)
			break;
		eCameraStatus camera_move_status = *(eCameraStatus *)param_data;
		RT_LOGI("set camera move status %d", camera_move_status);
		venc_comp->camera_move_status = camera_move_status;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_TARGET_BITS_CLIP_PARAM: {
		//VencTargetBitsClipParam venc_target_bits_clip_param;
		VencTargetBitsClipParam *p_venc_target_bits_clip_param = (VencTargetBitsClipParam *)param_data;
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamTargetBitsClipParam, p_venc_target_bits_clip_param);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_MB_MAP_PARAM: {
		RTVencMbMapParam *mb_map_param = (RTVencMbMapParam *)param_data;
		memcpy(&venc_comp->base_config.mb_mode_ctl, &mb_map_param->mb_md_ctl, sizeof(VencMBModeCtrl));
		memcpy(&venc_comp->base_config.mMapLow, &mb_map_param->low32, sizeof(VencMbModeLambdaQpMap));
		memcpy(&venc_comp->base_config.mMapHigh, &mb_map_param->high32, sizeof(VencMbSplitMap));
		break;
	}
	default: {
		RT_LOGE("nor support config index");
		return ERROR_TYPE_ILLEGAL_PARAM;
		break;
	}
	}

	return error;
}

static int post_msg_and_wait(venc_comp_ctx *venc_comp, int cmd_id)
{
	int result = 0;
	message_t cmd_msg;
	InitMessage(&cmd_msg);
	cmd_msg.command	= cmd_id;
	cmd_msg.pReply = ConstructMessageReply();
	putMessageWithData(&venc_comp->msg_queue, &cmd_msg);

	int ret;
	while (1) {
		ret = rt_sem_down_timedwait(&cmd_msg.pReply->ReplySem, 5000);
		if (ret != 0) {
			RT_LOGE("fatal error! rt_venc cmd[%d] wait reply fail[0x%x]", cmd_id, ret);
		} else {
			break;
		}
	}
	result = cmd_msg.pReply->ReplyResult;
	RT_LOGD("rt_venc cmd[%d] receive reply ret: %d!", cmd_id, result);
	DestructMessageReply(cmd_msg.pReply);
	cmd_msg.pReply = NULL;

	return result;
}

error_type vencComp_ConfigSEI(venc_comp_ctx *venc_comp, RT_VENC_SEI_ATTR *pAttr)
{
	error_type ret = ERROR_TYPE_OK;
	int err;
	if (false == venc_comp->stSEIAttr.bEnable) {
		if (pAttr->bEnable) { //disable -> enable
			mutex_lock(&venc_comp->SEILock);
			memcpy(&venc_comp->stSEIAttr, pAttr, sizeof(RT_VENC_SEI_ATTR));
			err = message_create(&venc_comp->stSEICmdQueue);
			if (err != 0) {
				RT_LOGE("fatal error! rt_venc[%p] message create error!", venc_comp);
			}
			rt_sem_reset(&venc_comp->stSetSEISem);
			venc_comp->nSetSEICount = 0;
			mutex_unlock(&venc_comp->SEILock);

			venc_comp->SEIThread = kthread_create(SEIThread, venc_comp, "VEncSEIThd[%p]", venc_comp);
			err = wake_up_process(venc_comp->SEIThread);
			if (1 == err) {
				RT_LOGD("rt_media_chn[%d] rt_venc[%p] wake up SEI thread:%d", venc_comp->base_config.channel_id, venc_comp, err);
			} else if (0 == err) {
				RT_LOGE("fatal error! rt_media_chn[%d] rt_venc[%p] wake up SEI thread:%d, but it was already running",
					venc_comp->base_config.channel_id, venc_comp, err);
			} else {
				RT_LOGE("fatal error! rt_media_chn[%d] rt_venc[%p] wake up SEI thread error:%d", venc_comp->base_config.channel_id,
					venc_comp, err);
			}
		} else { //disable -> disable
			mutex_lock(&venc_comp->SEILock);
			memcpy(&venc_comp->stSEIAttr, pAttr, sizeof(RT_VENC_SEI_ATTR));
			mutex_unlock(&venc_comp->SEILock);
		}
	} else {
		if (pAttr->bEnable) { //enable ->enable
			mutex_lock(&venc_comp->SEILock);
			memcpy(&venc_comp->stSEIAttr, pAttr, sizeof(RT_VENC_SEI_ATTR));
			mutex_unlock(&venc_comp->SEILock);
		} else { //enable -> disable
			mutex_lock(&venc_comp->SEILock);
			memcpy(&venc_comp->stSEIAttr, pAttr, sizeof(RT_VENC_SEI_ATTR));
			mutex_unlock(&venc_comp->SEILock);

			message_t msg;
			InitMessage(&msg);
			msg.command = COMP_COMMAND_EXIT;
			put_message(&venc_comp->stSEICmdQueue, &msg);
			rt_sem_up_unique(&venc_comp->stSetSEISem);
			RT_LOGI("rt_venc[%p] send SEI message Stop", venc_comp);
			if (venc_comp->SEIThread) {
				kthread_stop(venc_comp->SEIThread);
				venc_comp->SEIThread = NULL;
			}
			message_destroy(&venc_comp->stSEICmdQueue);
		}
	}
	return ret;
}

error_type venc_comp_init(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_init: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	int ret = post_msg_and_wait(venc_comp, COMP_COMMAND_INIT);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_venc init fail:%d", ret);
	}
	return error;
}

error_type venc_comp_start(
	PARAM_IN comp_handle component)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	int ret = post_msg_and_wait(venc_comp, COMP_COMMAND_PAUSE);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_venc pause fail:%d", ret);
	}

	/* set param for vencoder here as:
	 *	vencoder_set_param will palloc vbv-buf, and we must make sure the pid
	 *	of 'palloc vbv-buf' code is the same as ioctl-calling, to valid ion share
	 *	between user and kenrl.
	 */
	vencoder_set_param(venc_comp);

	venc_comp->vencCallBack.EventHandler = venc_comp_event_handler;
	//if (venc_comp->base_config.bOnlineChannel == 0)
		venc_comp->vencCallBack.InputBufferDone = VencCompInputBufferDone;
	VencSetCallbacks(venc_comp->vencoder, &venc_comp->vencCallBack, venc_comp);
	if (venc_comp->base_config.mb_mode_ctl.mode_ctrl_en) {
		RT_LOGI("venc_comp->base_config.mb_mode_ctl.mode_ctrl_en %d", venc_comp->base_config.mb_mode_ctl.mode_ctrl_en);
		RT_LOGI("venc_comp->base_config.mb_mode_ctl.lambda_map_en %d", venc_comp->base_config.mb_mode_ctl.lambda_map_en);
		RT_LOGI("venc_comp->base_config.mMapLow.qp %d", venc_comp->base_config.mMapLow.qp);
		RT_LOGI("venc_comp->base_config.mMapHigh.intra8_cost_factor %d", venc_comp->base_config.mMapHigh.intra8_cost_factor);
		MBParamInit(&venc_comp->mMBModeCtrl, &venc_comp->mGetMbInfo, &venc_comp->base_config, &venc_comp->mMbNum);
	}
	VencStart(venc_comp->vencoder);
	ret = post_msg_and_wait(venc_comp, COMP_COMMAND_START);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_venc start fail:%d", ret);
	}
	return error;
}

error_type venc_comp_pause(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp pause: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("Ryan VencPause");
	VencPause(venc_comp->vencoder);

	int ret = post_msg_and_wait(venc_comp, COMP_COMMAND_PAUSE);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_venc pause fail:%d", ret);
	}

	return error;
}

error_type venc_comp_stop(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_stop: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("Ryan VencDestroy");
	//VencDestroy(venc_comp->vencoder);

	int ret = post_msg_and_wait(venc_comp, COMP_COMMAND_STOP);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_venc stop fail:%d", ret);
	}
	return error;
}

error_type venc_comp_destroy(
	PARAM_IN comp_handle component)
{
	int i = 0;
	int free_frame_cout		= 0;
	int free_stream_cout		= 0;
	video_stream_node *stream_node  = NULL;
	video_frame_node *frame_node    = NULL;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_destroy: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	/*should make sure the thread do nothing importent */
	if (venc_comp->venc_thread)
		kthread_stop(venc_comp->venc_thread);

	message_destroy(&venc_comp->msg_queue);

	//disable sei thread
	RT_VENC_SEI_ATTR stVencSeiAttr;
	memset(&stVencSeiAttr, 0, sizeof(stVencSeiAttr));
	vencComp_ConfigSEI(venc_comp, &stVencSeiAttr);

	//destroy venclib. warning: here not care if vbv buffer stream nodes are all returned, free them directly.
	//so app should return all stream nodes first, then destroy rt_media recorder.
	if (venc_comp->vencoder) {
		LOCK_MUTEX(&venc_mutex);
		VencDestroy(venc_comp->vencoder);
		UNLOCK_MUTEX(&venc_mutex);
	}

	while ((!list_empty(&venc_comp->in_buf_manager.empty_frame_list))) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.empty_frame_list, video_frame_node, mList);
		if (frame_node) {
			list_del(&frame_node->mList);
			free_frame_cout++;
			kfree(frame_node);
		}
	}

	if (!list_empty(&venc_comp->in_buf_manager.using_frame_list)) {
		int nUsingNum = 0;
		video_frame_node *tmp_node;
		list_for_each_entry_safe (frame_node, tmp_node, &venc_comp->in_buf_manager.using_frame_list, mList) {
			if (frame_node) {
				list_del(&frame_node->mList);
				free_frame_cout++;
				nUsingNum++;
				kfree(frame_node);
			}
		}
		RT_LOGE("fatal error! vencChn[%d] in_buf using frame list has %d frames", venc_comp->base_config.channel_id, nUsingNum);
	}

	int nValidNum = 0;
	while ((!list_empty(&venc_comp->in_buf_manager.valid_frame_list))) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		if (frame_node) {
			list_del(&frame_node->mList);
			free_frame_cout++;
			kfree(frame_node);
		}
		nValidNum++;
	}
	if (nValidNum > 0) {
		RT_LOGE("fatal error! vencChn[%d] in_buf valid frame list has %d frames", venc_comp->base_config.channel_id, nValidNum);
	}
	if (free_frame_cout != VENC_IN_BUFFER_LIST_NODE_NUM)
		RT_LOGE("fatal error! free num of frame node is not match: %d, %d", free_frame_cout, VENC_IN_BUFFER_LIST_NODE_NUM);

	RT_LOGD("free_frame_cout = %d", free_frame_cout);

	while ((!list_empty(&venc_comp->out_buf_manager.empty_stream_list))) {
		stream_node = list_first_entry(&venc_comp->out_buf_manager.empty_stream_list, video_stream_node, mList);
		if (stream_node) {
			list_del(&stream_node->mList);
			free_stream_cout++;
			kfree(stream_node);
		}
	}

	int nValidStreamNum = 0;
	while ((!list_empty(&venc_comp->out_buf_manager.valid_stream_list))) {
		stream_node = list_first_entry(&venc_comp->out_buf_manager.valid_stream_list, video_stream_node, mList);
		if (stream_node) {
			list_del(&stream_node->mList);
			free_stream_cout++;
			nValidStreamNum++;
			kfree(stream_node);
		}
	}
	if (nValidStreamNum > 0) {
		RT_LOGE("fatal error! vencChn[%d] out_buf has %d valid stream nodes which have been released in VencDestroy before."
			"We will improve in future",
			venc_comp->base_config.channel_id, nValidStreamNum);
	}
	if (free_stream_cout != VENC_OUT_BUFFER_LIST_NODE_NUM)
		RT_LOGE("fatal error! free num of stream node is not match: %d, %d", free_stream_cout, VENC_OUT_BUFFER_LIST_NODE_NUM);

	if (venc_comp->venc_overlayInfo) {
		for (i = 0; i < venc_comp->venc_overlayInfo->blk_num; i++) {
			if (venc_comp->venc_overlayInfo->overlayHeaderList[i].overlay_blk_addr) {
				vfree(venc_comp->venc_overlayInfo->overlayHeaderList[i].overlay_blk_addr);
				venc_comp->venc_overlayInfo->overlayHeaderList[i].overlay_blk_addr = NULL;
			}
		}
		kfree(venc_comp->venc_overlayInfo);
	}

	RT_LOGD("free_stream_cout = %d", free_stream_cout);

	if (venc_comp->p_rt_isp_cfg) {
		vfree(venc_comp->p_rt_isp_cfg);
		venc_comp->p_rt_isp_cfg = NULL;
	}

	if (venc_comp->base_config.mb_mode_ctl.mode_ctrl_en)
		MBParamDeinit(&venc_comp->mMBModeCtrl, &venc_comp->mGetMbInfo, &venc_comp->base_config);

	rt_sem_deinit(&venc_comp->stSetSEISem);
	mutex_destroy(&venc_comp->WaitOutputStreamLock);
	mutex_destroy(&venc_comp->SEILock);

	kfree(venc_comp);

	return error;
}

error_type venc_comp_get_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_INOUT void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_get_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("venc_comp_get_config : %d, start", index);
	switch (index) {
	case COMP_INDEX_VENC_CONFIG_GET_WBYUV: {
		RTVencThumbInfo *p_wb_info = (RTVencThumbInfo *)param_data;
		VencThumbInfo s_venc_thumb_info;
		memset(&s_venc_thumb_info, 0, sizeof(VencThumbInfo));

		s_venc_thumb_info.nThumbSize = p_wb_info->nThumbSize;
		s_venc_thumb_info.pThumbBuf = p_wb_info->pThumbBuf;

		LOCK_MUTEX(&venc_mutex);
		error = VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetThumbYUV, &s_venc_thumb_info);
		UNLOCK_MUTEX(&venc_mutex);
		RT_LOGD("nThumbSize %d %px", p_wb_info->nThumbSize, p_wb_info->pThumbBuf);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO: {
		KERNEL_VBV_BUFFER_INFO *param_vbv = (KERNEL_VBV_BUFFER_INFO *)param_data;
		VbvInfo vbv_info;
		int share_fd = 0;

		memset(&vbv_info, 0, sizeof(VbvInfo));

		if (venc_comp->vencoder_init_flag == 0) {
			RT_LOGD("get vbv info: vencoder not init");
			return ERROR_TYPE_ERROR;
		}

		VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbvInfo, &vbv_info);
		param_vbv->size = vbv_info.vbv_size;

		VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetVbvShareFd, &share_fd);
		param_vbv->share_fd = share_fd;

		RT_LOGW("get vbv info: share_fd = %d, size = %d", param_vbv->share_fd, param_vbv->size);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_STREAM_HEADER: {
		venc_comp_header_data *header_data = (venc_comp_header_data *)param_data;
		VencHeaderData sps_pps_data;
		memset(&sps_pps_data, 0, sizeof(VencHeaderData));

		if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H264) {
			VencGetParameter(venc_comp->vencoder, VENC_IndexParamH264SPSPPS, &sps_pps_data);
		} else if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H265) {
			VencGetParameter(venc_comp->vencoder, VENC_IndexParamH265Header, &sps_pps_data);
		} else {
			RT_LOGW("not support COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO yet");
			return ERROR_TYPE_ERROR;
		}
		RT_LOGD("1 get stream header = %d, data = %x %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[0],
			sps_pps_data.pBuffer[1],
			sps_pps_data.pBuffer[2],
			sps_pps_data.pBuffer[3],
			sps_pps_data.pBuffer[4],
			sps_pps_data.pBuffer[5],
			sps_pps_data.pBuffer[6],
			sps_pps_data.pBuffer[7]);
		RT_LOGD("2 get stream header size = %d, data = %x %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[8],
			sps_pps_data.pBuffer[9],
			sps_pps_data.pBuffer[10],
			sps_pps_data.pBuffer[11],
			sps_pps_data.pBuffer[12],
			sps_pps_data.pBuffer[13],
			sps_pps_data.pBuffer[14],
			sps_pps_data.pBuffer[15]);
		RT_LOGD("3 get stream header size = %d, data = %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[16],
			sps_pps_data.pBuffer[17],
			sps_pps_data.pBuffer[18],
			sps_pps_data.pBuffer[19],
			sps_pps_data.pBuffer[20],
			sps_pps_data.pBuffer[21],
			sps_pps_data.pBuffer[22]);

		header_data->pBuffer = sps_pps_data.pBuffer;
		header_data->nLength = sps_pps_data.nLength;

		break;
	}
#if NEI_TEST_CODE
	case COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA:
	{
		VencGetBinImageBufInfo venc_bin_buf_info;
		VideoGetBinImageBufInfo bin_buf_info;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA ");

		memset(&bin_buf_info, 0, sizeof(VideoGetBinImageBufInfo));
		memset(&venc_bin_buf_info, 0, sizeof(VencGetBinImageBufInfo));
		if (copy_from_user(&bin_buf_info, (void __user *)param_data, sizeof(struct VideoGetBinImageBufInfo))) {
			RT_LOGE("COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA copy_from_user fail\n");
			return -EFAULT;
		}
		venc_bin_buf_info.buf = bin_buf_info.buf;
		venc_bin_buf_info.max_size = bin_buf_info.max_size;

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetBinImageData, &venc_bin_buf_info);
	}
	case COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA:
	{
		VencGetMvInfoBufInfo venc_mv_buf_info;
		VideoGetMvInfoBufInfo mv_buf_info;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA");

		memset(&mv_buf_info, 0, sizeof(VideoGetMvInfoBufInfo));
		memset(&venc_mv_buf_info, 0, sizeof(VencGetMvInfoBufInfo));

		if (copy_from_user(&mv_buf_info, (void __user *)param_data, sizeof(struct VideoGetMvInfoBufInfo))) {
			RT_LOGE("COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA copy_from_user fail\n");
			return -EFAULT;
		}

		venc_mv_buf_info.buf = mv_buf_info.buf;
		venc_mv_buf_info.max_size = mv_buf_info.max_size;

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetMvInfoData, &venc_mv_buf_info);
	}
#endif
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT: {
		VencMotionSearchResult *motion_result = (VencMotionSearchResult *)param_data;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT");

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamMotionSearchResult, motion_result);
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT: {
		// VencRegionD3DResult *pRegionD3DResult = (VencRegionD3DResult *)param_data;
		// error = VencGetParameter(venc_comp->vencoder, VENC_IndexParamRegionD3DResult, pRegionD3DResult);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_INSERT_DATA_BUF_STATUS: {
		VENC_BUF_STATUS *pStatus = (VENC_BUF_STATUS *)param_data;
		error = VencGetParameter(venc_comp->vencoder, VENC_IndexParamInsertDataBufStatus, pStatus);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_REGION_DETECT_LINK: {
		LOCK_MUTEX(&venc_mutex);
		VencGetParameter(venc_comp->vencoder, VENC_IndexParamRegionDetectLinkParam, (void *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_VBR_OPT_PARAM: {
		VencVbrOptParam *pVbrOptParam = (VencVbrOptParam *)param_data;
		error = VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbrOptParam, pVbrOptParam);
		break;
	}
	default: {
		RT_LOGW("get config: not support the index = 0x%x", index);
		break;
	}
	}
	RT_LOGD("venc_comp_get_config : %d, finish", index);
	return error;
}

error_type venc_comp_set_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_IN void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_set_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	switch (index) {
	case COMP_INDEX_VENC_CONFIG_RESET_VE_INOUT_BUFFER: {
		RT_LOGD("VencBufferReset");
		VencReset(venc_comp->vencoder);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Base: {
		venc_comp_base_config *base_config = (venc_comp_base_config *)param_data;
		memcpy(&venc_comp->base_config, base_config, sizeof(venc_comp_base_config));
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Normal: {
		venc_comp_normal_config *normal_config = (venc_comp_normal_config *)param_data;
		memcpy(&venc_comp->normal_config, normal_config, sizeof(venc_comp_normal_config));
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM: {
		if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H264
			|| venc_comp->base_config.codec_type == RT_VENC_CODEC_H265) {
			LOCK_MUTEX(&venc_mutex);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamMotionSearchParam, (VencMotionSearchParam *)param_data);
			UNLOCK_MUTEX(&venc_mutex);
		}
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_ROI: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamROIConfig, (VencROIConfig *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_GDC: {
		if (rt_is_format422(venc_comp->base_config.pixelformat)) {
			RT_LOGE("422 not support gdc, please disable gdc");
			break;
		}
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamGdcConfig, (sGdcParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_ROTATE: {
		if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
			RT_LOGW("lbc format not support rotate!");
			break;
		}
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRotation, (unsigned int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_REC_REF_LBC_MODE: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetRecRefLbcMode, (eVeLbcMode *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_WEAK_TEXT_TH: {
		LOCK_MUTEX(&venc_mutex);
		venc_comp->st_dynamic_proc_set.weak_text_th = *((float *)param_data);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamWeakTextTh, (float *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM: {
		LOCK_MUTEX(&venc_mutex);
		// VencSetParameter(venc_comp->vencoder, VENC_IndexParamRegionD3DParam, (VencRegionD3DParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_CHROMA_QP_OFFSET: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamChromaQPOffset, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_ENC_AND_DEC_CASE: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEncAndDecCase, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_P_SKIP: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetPSkip, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_NULL_FRAME: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetNullFrame, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264ConstraintFlag, (VencH264ConstraintFlag *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamVe2IspD2DLimit, (VencVe2IspD2DLimit *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_EN_SMALL_SEARCH_RANGE: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnSmallSearchRange, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_FORCE_CONF_WIN: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamForceConfWin, (VencForceConfWin *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_ROT_VE2ISP: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRotVe2Isp, (VencRotVe2Isp *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_INSERT_DATA: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamInsertData, (VencInsertData *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_GRAY: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamChmoraGray, (unsigned int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_WBYUV: {
		sWbYuvParam *p_wbyuv_param = (sWbYuvParam *)param_data;
		if (p_wbyuv_param->bEnableWbYuv && !venc_comp->vencoder_init_flag) {//en wbyuv will set int venc_init.
			memcpy(&venc_comp->base_config.s_wbyuv_param, p_wbyuv_param, sizeof(sWbYuvParam));
		} else {
			LOCK_MUTEX(&venc_mutex);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableWbYuv, (void *)param_data);
			UNLOCK_MUTEX(&venc_mutex);
		}
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_2DNR: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam2DFilter, (s2DfilterParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_3DNR: {
		Venc3dFilterParam st3dFilterParam;
		memset(&st3dFilterParam, 0, sizeof(Venc3dFilterParam));
		st3dFilterParam.e3dEn = *((int *)param_data);
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam3DFilter, &st3dFilterParam);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_CYCLE_INTRA_REFRESH: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264CyclicIntraRefresh, (VencCyclicIntraRefresh *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_P_FRAME_INTRA: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamPFrameIntraEn, (unsigned int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_DROP_FRAME: {
		message_t msg;
		memset(&msg, 0, sizeof(message_t));
		msg.command = COMP_COMMAND_VENC_DROP_FRAME;
		msg.para0 = *(int *)param_data;
		put_message(&venc_comp->msg_queue, &msg);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_CROP: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamCropCfg, (void *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SEI: {
		error = vencComp_ConfigSEI(venc_comp, (RT_VENC_SEI_ATTR *)param_data);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_REGION_DETECT_LINK: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRegionDetectLinkParam, (void *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_VBR_OPT_PARAM: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamVbrOptParam, (VencVbrOptParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	default: {
		error = config_dynamic_param(venc_comp, index, param_data);
		break;
	}
	}

	return error;
}

error_type venc_comp_get_state(
	PARAM_IN comp_handle component,
	PARAM_OUT comp_state_type *pState)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_get_state: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	*pState = venc_comp->state;
	return error;
}

/* empty_list --> valid_list */
error_type venc_comp_empty_this_in_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *buffer_header)
{
	video_frame_node *pNode		= NULL;
	video_frame_s *src_frame	= NULL;
	video_frame_node *first_node    = NULL;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;
	message_t msg;
	static int cnt_1 = 1;

	memset(&msg, 0, sizeof(message_t));

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_empty_this_in_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	if (venc_comp->state != COMP_STATE_EXECUTING) {
		RT_LOGW("channel %d submit inbuf when venc state[0x%x] isn not executing",
			venc_comp->base_config.channel_id, venc_comp->state);
	}

	//reach max recv number
	if (venc_comp->base_config.nRecvFrameNum > 0 && venc_comp->nCurRecvFrameNum >= venc_comp->base_config.nRecvFrameNum) {
		RT_LOGD("rt_media_chn[%d]-rt_venc[%p] codecType:%d, had received [%d-%d] frame. It is enough!",
			venc_comp->base_config.channel_id, venc_comp, venc_comp->base_config.codec_type,
			venc_comp->nCurRecvFrameNum, venc_comp->base_config.nRecvFrameNum);
		error = ERROR_TYPE_VENC_REFUSE;
		goto submit_exit;
	}

	src_frame = (video_frame_s *)buffer_header->private;

	if (src_frame == NULL) {
		RT_LOGE("fatal error! submit in buf: frame is null");
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto submit_exit;
	}

	RT_LOGI("empty buf: pts = %lld", src_frame->pts);

	mutex_lock(&venc_comp->in_buf_manager.mutex);

	if (list_empty(&venc_comp->in_buf_manager.empty_frame_list)) {
		RT_LOGW("Low probability! venc idle frame is empty!");
		if (venc_comp->in_buf_manager.empty_num != 0) {
			RT_LOGE("fatal error! empty_num must be zero!");
		}

		pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			goto submit_exit;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;
	}

	first_node = list_first_entry(&venc_comp->in_buf_manager.empty_frame_list, video_frame_node, mList);

	memcpy(&first_node->video_frame, src_frame, sizeof(video_frame_s));
	venc_comp->in_buf_manager.empty_num--;

	list_move_tail(&first_node->mList, &venc_comp->in_buf_manager.valid_frame_list);
#if 1
	if (cnt_1) {
		cnt_1 = 0;
		RT_LOGW("channel %d first time add buffer_node to valid_frame_list, time: %lld",
			venc_comp->base_config.channel_id, get_cur_time());
	}
#endif
	if (venc_comp->in_buf_manager.no_frame_flag) {
		venc_comp->in_buf_manager.no_frame_flag = 0;
		msg.command				      = COMP_COMMAND_VENC_INPUT_FRAME_VALID;
		put_message(&venc_comp->msg_queue, &msg);
	}

	venc_comp->nCurRecvFrameNum++;
	mutex_unlock(&venc_comp->in_buf_manager.mutex);
submit_exit:

	return error;
}

/* valid_list --> empty_list */
error_type venc_comp_fill_this_out_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;
	video_stream_s *src_stream      = NULL;
	video_stream_node *stream_node  = NULL;
	VencOutputBuffer output_buffer;

	memset(&output_buffer, 0, sizeof(VencOutputBuffer));

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_fill_this_out_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	src_stream = (video_stream_s *)pBuffer->private;

	RT_LOGD(" id = %d", src_stream->id);

	/* get the ready frame */
	mutex_lock(&venc_comp->out_buf_manager.mutex);

	if (list_empty(&venc_comp->out_buf_manager.valid_stream_list)) {
		RT_LOGE("the valid stream list is empty when return out buffer");
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return ERROR_TYPE_ERROR;
	}
	stream_node = list_first_entry(&venc_comp->out_buf_manager.valid_stream_list, video_stream_node, mList);

	if (src_stream->id != stream_node->video_stream.id) {
		RT_LOGE("fill this out buf: buf is not match, id = %d ,%d",
			src_stream->id, stream_node->video_stream.id);
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return -1;
	}

	memcpy(&stream_node->video_stream, src_stream, sizeof(video_stream_s));

	/* kfree bitstream frame to vencoder */
	config_VencOutputBuffer_by_videostream(venc_comp, &output_buffer, &stream_node->video_stream);

	LOCK_MUTEX(&venc_mutex);
	VencQueueOutputBuf(venc_comp->vencoder, &output_buffer);
	UNLOCK_MUTEX(&venc_mutex);

	list_move_tail(&stream_node->mList, &venc_comp->out_buf_manager.empty_stream_list);
	venc_comp->out_buf_manager.empty_num++;
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	return error;
}

error_type venc_comp_set_callbacks(
	PARAM_IN comp_handle component,
	PARAM_IN comp_callback_type *callback,
	PARAM_IN void *callback_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	venc_comp->callback      = callback;
	venc_comp->callback_data = callback_data;
	return error;
}

error_type venc_comp_tunnel_request(
	PARAM_IN comp_handle component,
	PARAM_IN comp_port_type port_type,
	PARAM_IN comp_handle tunnel_comp,
	PARAM_IN int connect_flag)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL || tunnel_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks, param error: %px, %px, %px",
			rt_component, venc_comp, tunnel_comp);
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto setup_tunnel_exit;
	}

	if (port_type == COMP_INPUT_PORT) {
		if (connect_flag == 1) {
			if (venc_comp->in_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("in port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				venc_comp->in_port_tunnel_info.valid_flag  = 1;
				venc_comp->in_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (venc_comp->in_port_tunnel_info.valid_flag == 1 && venc_comp->in_port_tunnel_info.tunnel_comp == tunnel_comp) {
				venc_comp->in_port_tunnel_info.valid_flag  = 0;
				venc_comp->in_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect in tunnel failed:  valid_flag = %d, tunnel_comp = %px, %px",
					venc_comp->in_port_tunnel_info.valid_flag,
					venc_comp->in_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	} else if (port_type == COMP_OUTPUT_PORT) {
		if (connect_flag == 1) {
			if (venc_comp->out_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("out port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				venc_comp->out_port_tunnel_info.valid_flag  = 1;
				venc_comp->out_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (venc_comp->out_port_tunnel_info.valid_flag == 1 && venc_comp->out_port_tunnel_info.tunnel_comp == tunnel_comp) {
				venc_comp->out_port_tunnel_info.valid_flag  = 0;
				venc_comp->out_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect out tunnel failed:	valid_flag = %d, tunnel_comp = %px, %px",
					venc_comp->out_port_tunnel_info.valid_flag,
					venc_comp->out_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	}

setup_tunnel_exit:

	return error;
}

error_type venc_comp_component_init(PARAM_IN comp_handle component, const rt_media_config_s *pmedia_config)
{
	int i				= 0;
	int ret				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = NULL;

	venc_comp = kmalloc(sizeof(struct venc_comp_ctx), GFP_KERNEL);
	if (venc_comp == NULL) {
		RT_LOGE("kmalloc for venc_comp failed");
		error = ERROR_TYPE_NOMEM;
		goto EXIT;
	}
	memset(venc_comp, 0, sizeof(struct venc_comp_ctx));
	venc_comp->p_rt_isp_cfg = vmalloc(sizeof(struct isp_cfg_attr_data));
	memset(venc_comp->p_rt_isp_cfg, 0, sizeof(struct isp_cfg_attr_data));
#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
	if (global_osd_info == NULL) {
		global_osd_info = kmalloc(sizeof(struct native_osd_info), GFP_KERNEL);
		if (global_osd_info == NULL) {
			RT_LOGW("kmalloc for global_osd_info failed, size = %d", sizeof(struct native_osd_info));
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		} else
			memset(global_osd_info, 0, sizeof(struct native_osd_info));

		/* force set ori size to 2560x1440*/
		global_osd_info->ori_frame_w = 2560;
		global_osd_info->ori_frame_h = 1440;
		RT_LOGI("ori_frame_w = %d, h = %d", global_osd_info->ori_frame_w, global_osd_info->ori_frame_h);
	}
#endif

	ret = message_create(&venc_comp->msg_queue);
	if (ret < 0) {
		RT_LOGE("message create failed");
		error = ERROR_TYPE_ERROR;
		goto EXIT;
	}

	rt_sem_init(&venc_comp->stSetSEISem, 0);
	mutex_init(&venc_comp->WaitOutputStreamLock);
	mutex_init(&venc_comp->SEILock);

	rt_component->component_private    = venc_comp;
	rt_component->init		   = venc_comp_init;
	rt_component->start		   = venc_comp_start;
	rt_component->pause		   = venc_comp_pause;
	rt_component->stop		   = venc_comp_stop;
	rt_component->destroy		   = venc_comp_destroy;
	rt_component->get_config	   = venc_comp_get_config;
	rt_component->set_config	   = venc_comp_set_config;
	rt_component->get_state		   = venc_comp_get_state;
	rt_component->empty_this_in_buffer = venc_comp_empty_this_in_buffer;
	rt_component->fill_this_out_buffer = venc_comp_fill_this_out_buffer;
	rt_component->set_callbacks	= venc_comp_set_callbacks;
	rt_component->tunnel_request	 = venc_comp_tunnel_request;

	venc_comp->base_config.channel_id = pmedia_config->channelId;
	venc_comp->self_comp = rt_component;
	venc_comp->state     = COMP_STATE_IDLE;

	init_waitqueue_head(&venc_comp->jpeg_cxt.wait_enc_finish);

	/* init in buf manager*/
	mutex_init(&venc_comp->in_buf_manager.mutex);
	venc_comp->in_buf_manager.empty_num = VENC_IN_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.empty_frame_list);
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.valid_frame_list);
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.using_frame_list);
	for (i = 0; i < VENC_IN_BUFFER_LIST_NODE_NUM; i++) {
		video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
	}

	/* init out buf manager*/
	mutex_init(&venc_comp->out_buf_manager.mutex);
	venc_comp->out_buf_manager.empty_num = VENC_OUT_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&venc_comp->out_buf_manager.empty_stream_list);
	INIT_LIST_HEAD(&venc_comp->out_buf_manager.valid_stream_list);
	for (i = 0; i < VENC_OUT_BUFFER_LIST_NODE_NUM; i++) {
		video_stream_node *pNode = kmalloc(sizeof(video_stream_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_stream_node));
		list_add_tail(&pNode->mList, &venc_comp->out_buf_manager.empty_stream_list);
	}
	venc_comp->camera_move_status = CAMERA_ADAPTIVE_STATIC;
	RT_LOGS("channel_id %d venc_comp %px create thread_process_venc", venc_comp->base_config.channel_id, venc_comp);
	venc_comp->venc_thread = kthread_create(thread_process_venc,
						venc_comp, "venc comp thread");
	wake_up_process(venc_comp->venc_thread);

EXIT:

	return error;
}

#if TEST_ENCODER_BY_SELF
static void loop_encode_one_frame(struct venc_comp_ctx *venc_comp)
{
	VencInputBuffer in_buf;
	video_frame_s cur_video_frame;
	video_frame_node *frame_node = NULL;
	int result		     = 0;

	if (venc_comp->is_first_frame == 0) {
		mutex_lock(&venc_comp->in_buf_manager.mutex);
		if (list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
			RT_LOGD("valid frame list is null, wait for it  start");
			venc_comp->in_buf_manager.no_frame_flag = 1;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 200);
			RT_LOGD("valid frame list is null, wait for it finish");
			return;
		}
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));
		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		GetOneAllocInputBuffer(venc_comp->vencoder, &in_buf);
		memcpy(in_buf.pAddrVirY, cur_video_frame.vir_addr[0], cur_video_frame.buf_size);
		FlushCacheAllocInputBuffer(venc_comp->vencoder, &in_buf);

		VencQueueInputBuf(venc_comp->vencoder, &in_buf);
		result = encodeOneFrame(venc_comp->vencoder);

		RT_LOGW("first encode, ret = %d, buf_size = %d, vir_addr = %px, %px",
			result, cur_video_frame.buf_size,
			in_buf.pAddrVirY, cur_video_frame.vir_addr[0]);

		mutex_lock(&venc_comp->in_buf_manager.mutex);
		list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;
		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		venc_empty_in_buffer_done(venc_comp, &cur_video_frame);
		venc_comp->is_first_frame = 1;
	} else {
		GetOneAllocInputBuffer(venc_comp->vencoder, &in_buf);
		//int y_size = venc_comp->base_config.src_width*venc_comp->base_config.src_height;
		//memset(in_buf.pAddrVirY, 0x80, y_size);
		//memset(in_buf.pAddrVirC, 0x10, y_size/2);

		//FlushCacheAllocInputBuffer(venc_comp->vencoder, &in_buf);
		VencQueueInputBuf(venc_comp->vencoder, &in_buf);
		result = encodeOneFrame(venc_comp->vencoder);

		if (result != 0)
			msleep(20);
		RT_LOGD("**test copy encoder, result = %d", result);
	}
}
#endif


static int venc_comp_stream_buffer_to_valid_list(struct venc_comp_ctx *venc_comp, VencOutputBuffer *p_out_buffer)
{
	int result = 0;

	mutex_lock(&venc_comp->out_buf_manager.mutex);
	if (list_empty(&venc_comp->out_buf_manager.empty_stream_list)) {
		if (list_empty(&venc_comp->out_buf_manager.empty_stream_list))
			RT_LOGI("venc_comp %px channel_id %d empty list is null", venc_comp, venc_comp->base_config.channel_id);
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return -1;
	}
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	LOCK_MUTEX(&venc_mutex);
	result = VencDequeueOutputBuf(venc_comp->vencoder, p_out_buffer);
	UNLOCK_MUTEX(&venc_mutex);
	if (result != 0) {
		RT_LOGD(" have no bitstream, result = %d", result);
		return -1;
	}
	RT_LOGI(" get bitstream , result = %d, pts = %lld pData0 0x%px %d", result, p_out_buffer->nPts, p_out_buffer->pData0,
		p_out_buffer->nSize0);

	if (venc_comp->vbvStartAddr == NULL) {
		VbvInfo vbv_info;

		memset(&vbv_info, 0, sizeof(VbvInfo));
		VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbvInfo, &vbv_info);
		venc_comp->vbvStartAddr = vbv_info.start_addr;
	}

	venc_fill_out_buffer_done(venc_comp, p_out_buffer);
	return 0;
}

/**
  Sei data structure:

  ----------------------------------------------------------
  |   header  | ISPSeiInfoLevel1 | ISPSeiInfoLevel2 | ...
  |--4 bytes--|---------------------------------------------

  header description:
  bit31-bit24: special flag, 0xA5.
  bit23-bit16: sei info type(ISP, VIPP, VENC)
  bit15-bit10:sei venc subtype. (VencInfoType_1Sec | VencInfoType_10Sec | VencInfoType_1Times)<<10;
  bit9-bit6: sei vipp subtype.
  bit5-bit0: sei isp subtype. (ISPInfoType_Level1 | ISPInfoType_Level2 | ISPInfoType_Level3)

  It will realloc memory for pSeiData->pBuffer if it is not enough.
*/
#define SEIDataSubType_ISP_Bits (6)
#define SEIDataSubType_VIPP_Bits (4)
#define SEIDataSubType_VENC_Bits (6)
static error_type ConfigSeiData(VencSeiData *pSeiData, ISPSeiInfo *pISPSeiInfo, VIPPSeiInfo *pVIPPSeiInfo, VencSeiInfo *pVencSeiInfo)
{
	int nDataLen = 4;
	int nSEITypeBits = 0;
	int nSEISubTypeBits = 0;
	if (pISPSeiInfo != NULL) {
		nSEITypeBits |= RTSEIDataType_ISP;
		nSEISubTypeBits |= pISPSeiInfo->nInfoBitFlags;
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level1) {
			nDataLen += sizeof(pISPSeiInfo->mInfoLevel1);
		}
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level2) {
			nDataLen += sizeof(pISPSeiInfo->mInfoLevel2);
		}
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level3) {
			nDataLen += sizeof(pISPSeiInfo->mInfoLevel3);
		}
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level4) {
			nDataLen += sizeof(pISPSeiInfo->mInfoLevel4);
		}
	}
	if (pVIPPSeiInfo != NULL) {
		nSEITypeBits |= RTSEIDataType_VIPP;
		nDataLen += sizeof(VIPPSeiInfo);
	}
	if (pVencSeiInfo != NULL) {
		nSEITypeBits |= RTSEIDataType_VENC;
		nSEISubTypeBits |= (pVencSeiInfo->nInfoBitFlags << (SEIDataSubType_ISP_Bits+SEIDataSubType_VIPP_Bits));
		if (pVencSeiInfo->nInfoBitFlags & VencInfoType_1Sec) {
			nDataLen += sizeof(pVencSeiInfo->mInfo1Sec);
		}
		if (pVencSeiInfo->nInfoBitFlags & VencInfoType_10Sec) {
			nDataLen += sizeof(pVencSeiInfo->mInfo10Sec);
		}
		if (pVencSeiInfo->nInfoBitFlags & VencInfoType_1Times) {
			nDataLen += sizeof(pVencSeiInfo->mInfo1Times);
		}
	}
	int SeiHeader = 0xA5<<24 | nSEITypeBits<<16 | nSEISubTypeBits;
	//alogd("dataLen:%d, SeiHeader:0x%x", nDataLen, SeiHeader);
	if (pSeiData->nBufLen < nDataLen) {
		vfree(pSeiData->pBuffer);
		pSeiData->pBuffer = vmalloc(nDataLen);
		if (NULL == pSeiData->pBuffer) {
			RT_LOGE("fatal error! vmalloc [%d]bytes fail", nDataLen);
		}
		pSeiData->nBufLen = nDataLen;
	}
	memset(pSeiData->pBuffer, 0, nDataLen);
	pSeiData->nDataLen = nDataLen;
	pSeiData->nType = 5;
	unsigned char *pBuf = pSeiData->pBuffer;
	memcpy(pBuf, &SeiHeader, sizeof(SeiHeader));
	pBuf += sizeof(SeiHeader);
	if (pISPSeiInfo != NULL) {
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level1) {
			memcpy(pBuf, &pISPSeiInfo->mInfoLevel1, sizeof(pISPSeiInfo->mInfoLevel1));
			pBuf += sizeof(pISPSeiInfo->mInfoLevel1);
		}
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level2) {
			memcpy(pBuf, &pISPSeiInfo->mInfoLevel2, sizeof(pISPSeiInfo->mInfoLevel2));
			pBuf += sizeof(pISPSeiInfo->mInfoLevel2);
		}
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level3) {
			memcpy(pBuf, &pISPSeiInfo->mInfoLevel3, sizeof(pISPSeiInfo->mInfoLevel3));
			pBuf += sizeof(pISPSeiInfo->mInfoLevel3);
		}
		if (pISPSeiInfo->nInfoBitFlags & ISPInfoType_Level4) {
			memcpy(pBuf, &pISPSeiInfo->mInfoLevel4, sizeof(pISPSeiInfo->mInfoLevel4));
			pBuf += sizeof(pISPSeiInfo->mInfoLevel4);
		}
	}
	if (pVIPPSeiInfo != NULL) {
		memcpy(pBuf, pVIPPSeiInfo, sizeof(VIPPSeiInfo));
		pBuf += sizeof(VIPPSeiInfo);
	}
	if (pVencSeiInfo != NULL) {
		if (pVencSeiInfo->nInfoBitFlags & VencInfoType_1Sec) {
			memcpy(pBuf, &pVencSeiInfo->mInfo1Sec, sizeof(pVencSeiInfo->mInfo1Sec));
			pBuf += sizeof(pVencSeiInfo->mInfo1Sec);
		}
		if (pVencSeiInfo->nInfoBitFlags & VencInfoType_10Sec) {
			memcpy(pBuf, &pVencSeiInfo->mInfo10Sec, sizeof(pVencSeiInfo->mInfo10Sec));
			pBuf += sizeof(pVencSeiInfo->mInfo10Sec);
		}
		if (pVencSeiInfo->nInfoBitFlags & VencInfoType_1Times) {
			memcpy(pBuf, &pVencSeiInfo->mInfo1Times, sizeof(pVencSeiInfo->mInfo1Times));
			pBuf += sizeof(pVencSeiInfo->mInfo1Times);
		}
	}

	return ERROR_TYPE_OK;
}

static int SEIThread(void *pThreadData)
{
	venc_comp_ctx *venc_comp = (venc_comp_ctx *)pThreadData;
	message_t stCmdMsg;
	int ret;
	error_type eError = ERROR_TYPE_OK;

	ISPSeiInfo *pISPSeiInfo = (ISPSeiInfo *)vmalloc(sizeof(ISPSeiInfo));
	if (NULL == pISPSeiInfo) {
		RT_LOGE("fatal error! vmalloc fail");
	}
	VIPPSeiInfo *pVIPPSeiInfo = (VIPPSeiInfo *)vmalloc(sizeof(VIPPSeiInfo));
	if (NULL == pVIPPSeiInfo) {
		RT_LOGE("fatal error! vmalloc fail");
	}
	VencSeiInfo *pVencSeiInfo = (VencSeiInfo *)vmalloc(sizeof(VencSeiInfo));
	if (NULL == pVencSeiInfo) {
		RT_LOGE("fatal error! vmalloc fail");
	}
	unsigned char *pSeiDataBuf = NULL;
	unsigned int nSeiDataBufLen = 0;

	while (1) {
	PROCESS_MESSAGE:
		if (kthread_should_stop()) {
			break;
		}
		if (get_message(&venc_comp->stSEICmdQueue, &stCmdMsg) == 0) {
			if (COMP_COMMAND_EXIT == stCmdMsg.command) {
				RT_LOGD("rt_venc[%p] SEI thread receive exit command", venc_comp);
				// Kill thread
				goto EXIT;
			} else {
				RT_LOGE("fatal error! unknown command:0x%x", stCmdMsg.command);
			}
			goto PROCESS_MESSAGE;
		}
		ret = rt_sem_down_timedwait(&venc_comp->stSetSEISem, 1*1000);
		if (0 == ret) {
			mutex_lock(&venc_comp->SEILock);
			if (venc_comp->stSEIAttr.bEnable) {
				VENC_RESULT_TYPE eVencRet;
				VencSeiParam stVencSeiParam;
				memset(&stVencSeiParam, 0, sizeof(stVencSeiParam));
				int nNum = 0;
				if (venc_comp->stSEIAttr.nSeiDataTypeFlags | RTSEIDataType_ISP) {
					int nISPBitFlags = 0;
					if (0 == venc_comp->nSetSEICount%venc_comp->stSEIAttr.nFrameIntervalForISPLevel1) {
						nISPBitFlags |= ISPInfoType_Level1;
					}
					if (0 == venc_comp->nSetSEICount%venc_comp->stSEIAttr.nFrameIntervalForISPLevel2) {
						nISPBitFlags |= ISPInfoType_Level2;
					}
					if (0 == venc_comp->nSetSEICount%venc_comp->stSEIAttr.nFrameIntervalForISPLevel3) {
						nISPBitFlags |= (ISPInfoType_Level3 | ISPInfoType_Level4);
					}
					if (nISPBitFlags != 0) {
						memset(pISPSeiInfo, 0xA2, sizeof(ISPSeiInfo));
						pISPSeiInfo->nInfoBitFlags = nISPBitFlags;

						if (venc_comp->in_port_tunnel_info.valid_flag) {
							eError = comp_get_config(venc_comp->in_port_tunnel_info.tunnel_comp, COMP_INDEX_VI_CONFIG_Dynamic_ISP_SEI_INFO, pISPSeiInfo);
							if (eError != ERROR_TYPE_OK) {
								RT_LOGE("fatal error! rt_venc[%p] get isp sei info fail[%d]", venc_comp, eError);
							}
						} else {
							RT_LOGE("fatal error! rt_venc[%p] can't get sei info in non-tunnel mode", venc_comp);
							eError = ERROR_TYPE_ERROR;
						}

						if (ERROR_TYPE_OK == eError) {
							nNum++;
						}
					}
				}
				/*
				if (pVideoEncData->stSEIAttr.nSeiDataTypeFlags | SEIDataType_VIPP) {
					if (0 == pVideoEncData->nSetSEICount%pVideoEncData->stSEIAttr.nFrameIntervalForVIPP) {
						pVIPPSeiInfo = (VIPPSeiInfo *)malloc(sizeof(VIPPSeiInfo));
						if (NULL == pVIPPSeiInfo) {
							aloge("fatal error! malloc fail");
						}
						memset(pVIPPSeiInfo, 0xA2, sizeof(VIPPSeiInfo));
						eError = AW_MPI_VI_GetSEIInfo(pVideoEncData->stSEIAttr.nVipp, pVIPPSeiInfo);
						if (SUCCESS == eError) {
							nNum++;
						} else {
							aloge("fatal error! VencChn[%d] Get VIPP sei info fail[0x%x]", pVideoEncData->mMppChnInfo.mChnId, eError);
							if (pVIPPSeiInfo) {
								free(pVIPPSeiInfo);
								pVIPPSeiInfo = NULL;
							}
						}
					}
				}
				*/
				if (venc_comp->stSEIAttr.nSeiDataTypeFlags | RTSEIDataType_VENC) {
					int nVencBitFlags = 0;
					if (0 == venc_comp->nSetSEICount%venc_comp->stSEIAttr.nFrameIntervalForVencLevel1) {
						nVencBitFlags |= VencInfoType_1Sec;
					}
					if (0 == venc_comp->nSetSEICount%venc_comp->stSEIAttr.nFrameIntervalForVencLevel2) {
						nVencBitFlags |= (VencInfoType_10Sec | VencInfoType_1Times);
					}
					if (nVencBitFlags != 0) {
						memset(pVencSeiInfo, 0xA2, sizeof(VencSeiInfo));
						pVencSeiInfo->nInfoBitFlags = nVencBitFlags;
						eVencRet = VencGetParameter(venc_comp->vencoder, VENC_IndexParamSeiInfo, (void *)pVencSeiInfo);
						if (VENC_RESULT_OK == eVencRet) {
							nNum++;
						} else {
							RT_LOGE("fatal error! rt_venc[%p] get venc sei info fail[0x%x]", venc_comp, eVencRet);
						}
					}
				}
				if (nNum > 0) {
					VencSeiData stVencSeiData;
					memset(&stVencSeiData, 0, sizeof(stVencSeiData));
					stVencSeiData.pBuffer = pSeiDataBuf;
					stVencSeiData.nBufLen = nSeiDataBufLen;
					ConfigSeiData(&stVencSeiData, pISPSeiInfo, pVIPPSeiInfo, pVencSeiInfo);
					pSeiDataBuf = stVencSeiData.pBuffer;
					nSeiDataBufLen = stVencSeiData.nBufLen;

					stVencSeiParam.nSeiNum = 1;
					stVencSeiParam.pSeiData = &stVencSeiData;
					eVencRet = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSeiParam, (void *)&stVencSeiParam);
					if (eVencRet != VENC_RESULT_OK) {
						RT_LOGE("fatal error! rt_venc[%p] set sei param fail[0x%x]", venc_comp, eVencRet);
					}
				}
			}
			mutex_unlock(&venc_comp->SEILock);
		} else if (ETIMEDOUT == ret) {
		} else {
			RT_LOGE("fatal error! rt_venc[%p] wait setSei error[%d]!", venc_comp, ret);
		}
	}
EXIT:
	if (pISPSeiInfo != NULL) {
		vfree(pISPSeiInfo);
	}
	if (pVIPPSeiInfo != NULL) {
		vfree(pVIPPSeiInfo);
	}
	if (pVencSeiInfo != NULL) {
		vfree(pVencSeiInfo);
	}
	if (pSeiDataBuf != NULL) {
		vfree(pSeiDataBuf);
	}
	RT_LOGD("rt_venc[%p] SEI Thread exit", venc_comp);
	return ERROR_TYPE_OK;
}

static int thread_process_venc(void *param)
{
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)param;
	message_t cmd_msg;
	int result		     = 0;
	video_frame_node *frame_node = NULL;
	VencOutputBuffer out_buffer;
	VencInputBuffer in_buf;
	//int64_t time_start  = 0;
	//int64_t time_finish = 0;
	video_frame_s cur_video_frame;

	memset(&cur_video_frame, 0, sizeof(video_frame_s));
	memset(&out_buffer, 0, sizeof(VencOutputBuffer));

	RT_LOGS("channel_id %d thread_process_venc start !", venc_comp->base_config.channel_id);
	while (1) {
	process_message:
		if (kthread_should_stop())
			break;

		if (get_message(&venc_comp->msg_queue, &cmd_msg) == 0) {
			commond_process(venc_comp, &cmd_msg);
			/* continue process message */
			continue;
		}

		if (venc_comp->state == COMP_STATE_EXECUTING) {
/* get the ready frame */
#if TEST_ENCODER_BY_SELF
			loop_encode_one_frame(venc_comp);
#else

		#if DEBUG_SHOW_ENCODE_TIME
			static int cnt_0 = 1;
			if (cnt_0 && venc_comp->base_config.channel_id == 0) {
				cnt_0 = 0;
				RT_LOGW("channel %d first time in execution state, time: %lld", venc_comp->base_config.channel_id, get_cur_time());
			}
			static int cnt_01 = 1;
			if (cnt_01 && venc_comp->base_config.channel_id == 1) {
				cnt_01 = 0;
				RT_LOGW("channel %d first time in execution state, time: %lld", venc_comp->base_config.channel_id, get_cur_time());
			}
		#endif

			if (venc_comp->base_config.codec_type == RT_VENC_CODEC_JPEG && VencGetUnReadOutputBufNum(venc_comp->vencoder)) {
				result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
				RT_LOGD("ValidOutputBufNum:%d, UnreadOutputBufNum:%d, result:%d", VencGetValidOutputBufNum(venc_comp->vencoder),
					VencGetUnReadOutputBufNum(venc_comp->vencoder), result);
			}
			/* get frame from valid_list */
			if (venc_comp->base_config.bOnlineChannel == 0) {
				mutex_lock(&venc_comp->in_buf_manager.mutex);
				if (list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
//					static int count;
//					if (count % 100 == 0)
//						RT_LOGD("valid frame list is null, wait for it  start");
					venc_comp->in_buf_manager.no_frame_flag = 1;
					mutex_unlock(&venc_comp->in_buf_manager.mutex);

//					if (VencGetUnReadOutputBufNum(venc_comp->vencoder)) {
//						result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
//					}

//					TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 200);
//					if (count % 100 == 0) {
//						RT_LOGD("valid frame list is null, wait for it finish");
//					}
//					count++;
//					goto process_message;
				} else {
				#if DEBUG_SHOW_ENCODE_TIME
					static int cnt_1 = 1;
					if (cnt_1 && venc_comp->base_config.channel_id == 0) {
						cnt_1 = 0;
						RT_LOGW("channel %d first time found valid frame buffer, time: %lld",
							venc_comp->base_config.channel_id, get_cur_time());
					}
					static int cnt_11 = 1;
					if (cnt_11 && venc_comp->base_config.channel_id == 1) {
						cnt_11 = 0;
						RT_LOGW("channel %d first time found valid frame buffer, time: %lld",
							venc_comp->base_config.channel_id, get_cur_time());
					}
				#endif
					mutex_unlock(&venc_comp->in_buf_manager.mutex);

					//send all video frames to vencoder
					while (1) {
						mutex_lock(&venc_comp->in_buf_manager.mutex);
						frame_node = list_first_entry_or_null(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
						if (NULL == frame_node) {
							venc_comp->in_buf_manager.no_frame_flag = 1;
							mutex_unlock(&venc_comp->in_buf_manager.mutex);
							break;
						}
						memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));

						list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.using_frame_list);
						//venc_comp->in_buf_manager.empty_num++;
						mutex_unlock(&venc_comp->in_buf_manager.mutex);

						if (0 == venc_comp->mActiveDropNum) {
							/*process frame start*/
							memset(&in_buf, 0, sizeof(VencInputBuffer));
							in_buf.nID       = cur_video_frame.id;
							in_buf.nPts      = cur_video_frame.pts;
							in_buf.nFlag     = 0;
							in_buf.pAddrPhyY = (unsigned char *)cur_video_frame.phy_addr[0];
							in_buf.pAddrPhyC = (unsigned char *)cur_video_frame.phy_addr[1];
							in_buf.pAddrVirY = (unsigned char *)cur_video_frame.vir_addr[0];
							in_buf.pAddrVirC = (unsigned char *)cur_video_frame.vir_addr[1];
							RT_LOGI("phyY = %px, phyC = %px, %px, %px", in_buf.pAddrPhyY, in_buf.pAddrPhyC,
								in_buf.pAddrVirY, in_buf.pAddrVirC);
							if (venc_comp->jpeg_cxt.enable == 1 && venc_comp->jpeg_cxt.encoder_finish_flag == 0) {
								catch_jpeg_encoder(venc_comp, &in_buf);
							}

							result = VencQueueInputBuf(venc_comp->vencoder, &in_buf);
							if (result != 0) {
								RT_LOGE("fatal error! rt_media_chn[%d] VencQueueInputBuf fail[%d]. must return frame",
									venc_comp->base_config.channel_id, result);
								venc_empty_in_buffer_done(venc_comp, &cur_video_frame);
								mutex_lock(&venc_comp->in_buf_manager.mutex);
								list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
								venc_comp->in_buf_manager.empty_num++;
								mutex_unlock(&venc_comp->in_buf_manager.mutex);
							}
							/* process frame end */
						} else {
							venc_empty_in_buffer_done(venc_comp, &cur_video_frame);
							mutex_lock(&venc_comp->in_buf_manager.mutex);
							list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
							venc_comp->in_buf_manager.empty_num++;
							mutex_unlock(&venc_comp->in_buf_manager.mutex);
							venc_comp->mActiveDropNum--;
							#ifdef TEST_I_FRAME_SYNC
							if (0 == venc_comp->base_config.channel_id) {
								venc_comp->callback->EventHandler(
									venc_comp->self_comp,
									venc_comp->callback_data,
									COMP_EVENT_VENC_DROP_FRAME,
									0,
									0,
									NULL);
							}
							#endif
							RT_LOGD("rt_media_chn[%d] actively drop one frame, left [%d]frames to drop", venc_comp->base_config.channel_id,
								venc_comp->mActiveDropNum);
						}
					}
				}
			}
#endif
			/* get bitstream */
			while (1) {
				result = -1;
				if (VencGetUnReadOutputBufNum(venc_comp->vencoder)) {
					result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
				}
				if (result != 0) {
//					if (venc_comp->base_config.frame_rate) {
//						RT_LOGI("no output buff, will wait for one frame half time %d ms fps %d.",
//							1000 / venc_comp->base_config.frame_rate / 2, venc_comp->base_config.frame_rate);
//						TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 1000 / venc_comp->base_config.frame_rate / 2);
//					} else {
//						RT_LOGI("no output buff, will wait for one frame time 25 ms fps %d.", venc_comp->base_config.frame_rate);
//						TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 25);
//					}
					mutex_lock(&venc_comp->WaitOutputStreamLock);
					venc_comp->bWaitOutputStreamFlag = true;
					mutex_unlock(&venc_comp->WaitOutputStreamLock);
					int nNum = TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 5*1000);
					if (0 == nNum) {
						RT_LOGW("Be careful! rt_media_chn[%d] wait out stream or input frame timeout?", venc_comp->base_config.channel_id);
					}
					break;
				}
			}
		} else {
			static int count;
			count++;
			if (count % 50 == 0) {
				RT_LOGD("TMessage_WaitQueueNotEmpty  !");
			}
			TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 200);
		}
	}

	RT_LOGD("thread_process_venc finish !");
	return 0;
}
