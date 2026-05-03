/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#define LOG_TAG "rt_vi_comp"

#include "rt_common.h"
#include "rt_vi_component.h"
#include "rt_message.h"
#include <uapi/linux/sched/types.h>
#define RT_VI_SHOW_TIME_COST (0)

#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

struct vi_comp_ctx *g_vi_comp[VIDEO_INPUT_CHANNEL_NUM];

static int64_t time_vi_start;
static int64_t time_first_cb;

static int callback_count;

typedef struct vi_comp_ctx {
	comp_state_type state;
	struct task_struct *vi_thread;
	message_queue_t msg_queue;
	comp_callback_type *callback;
	void *callback_data;
	rt_component_type *self_comp;
	//wait_queue_head_t wait_reply[WAIT_REPLY_NUM];
	//unsigned int wait_reply_condition[WAIT_REPLY_NUM];
	vi_outbuf_manager out_buf_manager;
	comp_tunnel_info in_port_tunnel_info;
	comp_tunnel_info out_port_tunnel_infos[RT_MAX_VENC_COMP_NUM];
	struct mutex tunnelLock;
	vi_comp_base_config base_config;
	vi_comp_normal_config normal_config;
	config_yuv_buf_info yuv_buf_info;
	int config_yuv_buf_flag;
	int align_width;
	int align_height;
	int buf_size;
	int buf_num; // actual frame numfer, considering base_config.share_buf_num and base_config.vin_buf_num.
	struct vin_buffer *vin_buf; //malloc array, pass them to vin driver to use. number is vi_comp_ctx->buf_num
	//struct vin_buffer *vin_buf_user; //for OUTPUT_MODE_YUV.
	int vipp_id;
	struct mem_interface *memops;

	spinlock_t vi_spin_lock;
	wait_queue_head_t wait_in_buf;
	unsigned int wait_in_buf_condition;
	//int catch_jpeg_flag;
	int max_fps; //same to base_config.frame_rate, source vin fps.

	//user frame list is designed for app to get frame directly.
	struct mutex UserFrameLock;
	bool bUserReqFlag;
	bool bUserFrameReadyCondition;
	wait_queue_head_t   stUserFrameReadyWaitQueue; //wait_queue_t
	struct list_head    idleUserFrameList; //video_frame_node
	struct list_head    readyUserFrameList;
	struct list_head    usingUserFrameList;

	vi_lbc_fill_param s_vi_lbc_fill_param;
	rt_yuv_info *s_yuv_info; // elem number is not equal to [CONFIG_YUV_BUF_NUM], it is equal to vin_buf's number now.
	int vipp_is_init;
} vi_comp_ctx;

int thread_process_vi(void *param);

int config_videostream_by_vinbuf(vi_comp_ctx *vi_comp,
					video_frame_s *pvideo_stream,
					struct vin_buffer *pvin_buf)
{
	vi_comp_ctx *pvi_comp = (vi_comp_ctx *)vi_comp;
	int y_size		= pvi_comp->align_width * pvi_comp->align_height;
	unsigned char *phy_addr = pvin_buf->paddr;
	unsigned char *vir_addr = pvin_buf->vir_addr;

	pvideo_stream->pts = (uint64_t)(pvin_buf->vb.vb2_buf.timestamp/1000);
	pvideo_stream->phy_addr[0] = (unsigned int)phy_addr;
	pvideo_stream->phy_addr[1] = (unsigned int)(phy_addr + y_size);
	pvideo_stream->vir_addr[0] = (void *)vir_addr;
	pvideo_stream->vir_addr[1] = (void *)(vir_addr + y_size);
	pvideo_stream->private     = (void *)pvin_buf;
	pvideo_stream->buf_size    = pvi_comp->buf_size;
	if (pvi_comp->vipp_id == 0)
		RT_LOGI("boot id = %d, video pts = %llu us, cur_time = %llu us,",
			pvi_comp->vipp_id, pvideo_stream->pts, ktime_get_ns() / 1000);
	return 0;
}

int rt_is_lbc_format(vi_comp_ctx *vi_comp)
{
	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X
		|| vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X)
		return 1;
	else
		return 0;
}

int rt_need_fill_data(vi_comp_ctx *vi_comp)
{
	int vipp_id = vi_comp->base_config.channel_id;
	int en_encpp = 0;

	vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);

	if (en_encpp && vi_comp->base_config.en_16_align_fill_data
		&& vi_comp->base_config.height != vi_comp->align_height) {
			return 1;
	} else {
		return 0;
	}
}
void vin_buffer_callback(int id)
{
	vi_comp_ctx *vi_comp = g_vi_comp[id];

	RT_LOGI("callback, id = %d", id);

	time_first_cb = get_cur_time();
	callback_count++;

	if (callback_count == 1)
		RT_LOGW("callback, time = %lld", time_first_cb - time_vi_start);

	vi_comp->wait_in_buf_condition = 1;
	wake_up(&vi_comp->wait_in_buf);

	return;
}

#if 1//lbc fill data function.
void lbcLine_StmWrBit(VI_DMA_LBC_BS_S *bs, unsigned int word, unsigned int len)
{
    if (NULL == bs) {
		return;
    }
    bs->cnt++;
    bs->sum = bs->sum + len;

    while (len > 0) {
		if (len < 32) {
		    word = word & ((1 << len) - 1);
		}

		if (bs->left_bits > len) {
		    *bs->cur_buf_ptr = *bs->cur_buf_ptr | (word << (8 - bs->left_bits));
		    bs->left_bits = bs->left_bits - len;
		    break;
		} else {
		    *bs->cur_buf_ptr = *bs->cur_buf_ptr | (word << (8 - bs->left_bits));
		    len = len - bs->left_bits;
		    word = word >> bs->left_bits;
		    bs->left_bits = 8;
		    bs->cur_buf_ptr++;
		}
    }
}

void lbcLine_StmInit(VI_DMA_LBC_BS_S *bs, unsigned char *bs_buf_ptr)
{
    if (NULL == bs || NULL == bs_buf_ptr) {
		return;
    }
    bs->cur_buf_ptr = bs_buf_ptr;
    bs->left_bits = 8;
    bs->cnt = 0;
    bs->sum = 0;
}

void lbcLine_ParaInit(VI_DMA_LBC_PARAM_S *para, VI_DMA_LBC_CFG_S *cfg)
{
    if (NULL == para || NULL == cfg) {
		return;
    }
    para->mb_wth = 16;
    para->frm_wth = (cfg->frm_wth + 31) / 32 * 32;
    para->frm_hgt = cfg->frm_hgt;
    para->line_tar_bits[0] = cfg->line_tar_bits[0];
    para->line_tar_bits[1] = cfg->line_tar_bits[1];
    para->frm_bits = 0;
}

void lbcLine_Align(VI_DMA_LBC_PARAM_S *para, VI_DMA_LBC_BS_S *bs)
{
    unsigned int align_bit = 0;
    unsigned int align_bit_1 = 0;
    unsigned int align_bit_2 = 0;
    unsigned int i = 0;

    if (NULL == para || NULL == bs) {
		return;
    }

    align_bit = para->line_tar_bits[para->frm_y % 2] - para->line_bits;
    align_bit_1 = align_bit / 1024;
    align_bit_2 = align_bit % 1024;

    for (i = 0; i < align_bit_1; i++) {
		lbcLine_StmWrBit(bs, 0, 1024);
    }
    lbcLine_StmWrBit(bs, 0, align_bit_2);
    para->frm_bits += para->line_tar_bits[para->frm_y % 2];
}

void lbcLine_Enc(VI_DMA_LBC_PARAM_S *para, VI_DMA_LBC_BS_S *bs, unsigned int frm_x)
{
    unsigned int i = 0;
    unsigned int bits = 0;

    if (NULL == para || NULL == bs) {
		return;
    }

    if (para->frm_y % 2 == 0) {
		lbcLine_StmWrBit(bs, 1, 2); //mode-dts
		if (para->frm_x == 0) { //qp_code
		    lbcLine_StmWrBit(bs, 0, 3);
		    bits = 3;
		} else {
		    lbcLine_StmWrBit(bs, 1, 1);
		    bits = 1;
		}
		lbcLine_StmWrBit(bs, 0, 3);
		para->line_bits += bits + 5;
    } else {
		for (i = 0; i < 2; i++) {
		    lbcLine_StmWrBit(bs, 1, 2); //mode-dts
		    if (i == 1) { //qp_code
				lbcLine_StmWrBit(bs, 0, 1);
				bits = 1;
		    } else {
				if (para->frm_x == 0) { //qp_code
				    lbcLine_StmWrBit(bs, 0, 3);
				    bits = 3;
				} else {
				    lbcLine_StmWrBit(bs, 1, 1);
				    bits = 1;;
				}
		    }
		    lbcLine_StmWrBit(bs, 0, 3);
		    para->line_bits += bits + 5;
		}
    }
}

unsigned int lbcLine_Cmp(VI_DMA_LBC_CFG_S *cfg, VI_DMA_LBC_STM_S *stm)
{
    VI_DMA_LBC_PARAM_S stLbcPara;
    VI_DMA_LBC_BS_S stLbcBs;
    unsigned int frm_x = 0;
    unsigned int frm_bits = 0;

    if (NULL == cfg || NULL == stm) {
		RT_LOGE("fatal error! null pointer");
		return 0;
    }

    memset(&stLbcPara, 0, sizeof(VI_DMA_LBC_PARAM_S));
    memset(&stLbcBs, 0, sizeof(VI_DMA_LBC_BS_S));

    lbcLine_ParaInit(&stLbcPara, cfg);
    lbcLine_StmInit(&stLbcBs, stm->bs);

    for (stLbcPara.frm_y = 0; stLbcPara.frm_y < stLbcPara.frm_hgt; stLbcPara.frm_y = stLbcPara.frm_y + 1) {
		stLbcPara.line_bits = 0;
		for (stLbcPara.frm_x = 0; stLbcPara.frm_x < stLbcPara.frm_wth; stLbcPara.frm_x = stLbcPara.frm_x + stLbcPara.mb_wth) {
		    lbcLine_Enc(&stLbcPara, &stLbcBs, stLbcPara.frm_x);
		}
		lbcLine_Align(&stLbcPara, &stLbcBs);
    }
    frm_bits = (stLbcPara.frm_bits + 7) / 8;

    return frm_bits;
}
#endif

#if ENABLE_SAVE_VIN_OUT_DATA
void vi_comp_save_outdata(struct vi_comp_ctx *pvi_comp, video_frame_s *pvideo_frame)
{
	//struct mem_interface *_memops = pvi_comp->memops;
    int nSaveLen = pvideo_frame->buf_size;
	unsigned char *pVirBuf = pvideo_frame->vir_addr[0];
	int width = pvi_comp->align_width;
	int height = pvi_comp->align_height;
    int ret = 0;
    mm_segment_t old_fs;
	char name[128];
	struct file *fp;
	static int count;
	if (pvi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X
		|| pvi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		sprintf(name, "/mnt/extsd/lbc_%d_%d_%d.bin", width, height, count++);
	} else {
		sprintf(name, "/mnt/extsd/yuv_%d_%d_%d.yuv", width, height, count++);
	}
	if (count > 5)
		count = 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    fp = filp_open(name, O_CREAT|O_RDWR, 0600);
	if (IS_ERR(fp)) {
		RT_LOGE("open file failed");
		set_fs(old_fs);
		return ;
	}
	RT_LOGW("file name = %s, nSaveLen = %d, fp = %px, id = %d pVirBuf %px",
	     name, nSaveLen, fp, pvideo_frame->id, pVirBuf);

	//cdc_mem_flush_cache(_memops, pVirBuf, nSaveLen);
	ret = vfs_write(fp, (char __user *)pVirBuf, nSaveLen, &fp->f_pos);
	RT_LOGW("vfs_write ret: %d", ret);
	filp_close(fp, NULL);
    set_fs(old_fs);
	return ;
}
#endif

/**
  increase refCnt.
  pFrame must belong to vi_comp->out_buf_manager list.
*/
static error_type viComp_RefIncreaseVideoFrame(vi_comp_ctx *vi_comp, video_frame_s *pFrame, bool bLock)
{
	if (bLock) {
		mutex_lock(&vi_comp->out_buf_manager.mutex);
	}
	pFrame->refCnt++;
	if (bLock) {
		mutex_unlock(&vi_comp->out_buf_manager.mutex);
	}
	return ERROR_TYPE_OK;
}

/**
  reduce refCnt, if refCnt == 0, move node to empty frame list, then release vin_buf to vin driver.
  pFrame must belong to vi_comp->out_buf_manager list.
*/
static error_type viComp_RefReduceVideoFrame(vi_comp_ctx *vi_comp, video_frame_s *pFrame)
{
	error_type ret = ERROR_TYPE_OK;
	mutex_lock(&vi_comp->out_buf_manager.mutex);
	if (pFrame->refCnt <= 0) {
		RT_LOGE("fatal error! video frame ref cnt[%d] wrong, check code!", pFrame->refCnt);
	}
	pFrame->refCnt--;

	if (0 == pFrame->refCnt) {
		video_frame_node *pDstNode = NULL;
		video_frame_node *pEntry;
		list_for_each_entry (pEntry, &vi_comp->out_buf_manager.valid_frame_list, mList) {
			if (&pEntry->video_frame == pFrame) {
				pDstNode = pEntry;
				break;
			}
		}
		video_frame_node *pFrameNode = container_of(pFrame, video_frame_node, video_frame);
		if (pFrameNode != pDstNode) {
			RT_LOGE("fatal error! frame is not in list, check rt_vi_comp out_buf list![%p!=%p]", pFrameNode, pDstNode);
		}
		//I fear sth is wrong, so add above redundant code to verify. In normal, container_of() is enough.

		//RT_LOGD("rt_media_chn[%d] rt_vi_comp will release frameId[%d] to vipp[%d]", vi_comp->base_config.channel_id, pFrame->id, vi_comp->vipp_id);
		//release frame to vipp.
		struct vin_buffer *vin_buf = (struct vin_buffer *)pFrame->private;
		pFrame->private = NULL;
		list_move_tail(&pFrameNode->mList, &vi_comp->out_buf_manager.empty_frame_list);
		mutex_unlock(&vi_comp->out_buf_manager.mutex);

		RT_LOGI("vin_buf = %p", vin_buf);

		if (vin_buf == NULL)
			RT_LOGE("fatal error: vin_buf is null");
		else
			vin_qbuffer_special(vi_comp->vipp_id, vin_buf);

		return ret;
	} else {
		mutex_unlock(&vi_comp->out_buf_manager.mutex);
		return ret;
	}
}

static int vi_comp_fill_buffer_to_16_align(struct vi_comp_ctx *vi_comp, video_frame_s *src_frame)
{
	void *src_vir_addr = src_frame->vir_addr[0];
	unsigned int width = vi_comp->base_config.width;
	unsigned int width_align = vi_comp->align_width;
	unsigned int height = vi_comp->base_config.height;
	unsigned int height_align = vi_comp->align_height;
	unsigned int height_diff = height_align - height;
	unsigned int offset_src = width_align*height - (width_align*1);
	unsigned int offset_dst = 0;
	int i = 0;
	unsigned int mDataLen = 0;
	char *tmp = NULL;
	int ySize = 0;
	int uvData = 0;

	RT_LOGD("width %dx%d align %dx%d height_diff %d", width, height, width_align, height_align, height_diff);
	if (rt_is_lbc_format(vi_comp)) {
		unsigned char *mLbcFillDataAddr = vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr;
		unsigned int mLbcFillDataLen = vi_comp->s_vi_lbc_fill_param.mLbcFillDataLen;
		unsigned int line_bits_sum = (vi_comp->s_vi_lbc_fill_param.line_tar_bits[0] + vi_comp->s_vi_lbc_fill_param.line_tar_bits[1]) / 8;
		unsigned int offset_dst = line_bits_sum/2*height;

		RT_LOGD("ViCh:%d, LBC pix:0x%x, src_vir_addr:0x%px, dst:%d, src:%px, len:%d, %d", vi_comp->base_config.channel_id, vi_comp->base_config.pixelformat,
			src_vir_addr, offset_dst, mLbcFillDataAddr, mLbcFillDataLen, src_frame->buf_size);
		if (!mLbcFillDataAddr) {
			RT_LOGE("mLbcFillDataAddr is null!");
			return -1;
		}
		mDataLen = line_bits_sum*height_diff/2;
		if (mLbcFillDataLen != mDataLen) {
			RT_LOGW("error! wrong data len %d != %d", mLbcFillDataLen, mDataLen);
		}
		memcpy(src_vir_addr + offset_dst, mLbcFillDataAddr, mLbcFillDataLen);
		//cdc_mem_flush_cache(vi_comp->memops, src_vir_addr + offset_dst, mLbcFillDataLen);
	} else {
		for (i = 0; i < height_diff; i++) {
			offset_dst = width_align*height + (width_align*i);
			memcpy(src_vir_addr + offset_dst, src_vir_addr + offset_src, width_align);
		}
		//cdc_mem_flush_cache(vi_comp->memops, src_vir_addr + width_align*height, width_align*height_diff);

		tmp = (char *)src_vir_addr;
		ySize = width_align * height_align;
		if (RT_PIXEL_YUV420SP == vi_comp->base_config.pixelformat || RT_PIXEL_YVU420SP == vi_comp->base_config.pixelformat) {
			// fill UV
			uvData = width_align * height / 2;
			offset_src = ySize + uvData - (width_align*1);
			for (i = 0; i < height_diff/2; i++) {
				offset_dst = ySize + uvData + (width_align*i);
				RT_LOGI("i:%d, src_vir_addr:0x%x, offset_src:%d, offset_dst:%d, width_align:%d", i, src_vir_addr, offset_src, offset_dst, width_align);
				memcpy(src_vir_addr + offset_dst, src_vir_addr + offset_src, width_align);
			}
			//cdc_mem_flush_cache(vi_comp->memops, src_vir_addr + ySize + uvData, width_align*height_diff/2);
		} else if (RT_PIXEL_YUV420P == vi_comp->base_config.pixelformat || RT_PIXEL_YVU420P == vi_comp->base_config.pixelformat) {
			// fill U
			int uData = width_align * height / 4;
			offset_src = ySize + uData - (width_align/4 * 1);
			for (i = 0; i < height_diff; i++) {
				offset_dst = ySize + uData + (width_align/4 * i);
				memcpy(src_vir_addr + offset_dst, src_vir_addr + offset_src, width_align/4);
			}
			//cdc_mem_flush_cache(vi_comp->memops, src_vir_addr + ySize + uData, width_align/4*height_diff);
			// fill V
			uvData = width_align * height / 2;
			offset_src = ySize + uvData - (width_align/4 * 1);
			for (i = 0; i < height_diff; i++) {
				offset_dst = ySize + uvData + (width_align/4 * i);
				memcpy(src_vir_addr + offset_dst, src_vir_addr + offset_src, width_align/4);
			}
			//cdc_mem_flush_cache(vi_comp->memops, src_vir_addr + ySize + uvData, width_align/4*height_diff);
		} else {
			RT_LOGW("pixel format 0x%x is not support!", vi_comp->base_config.pixelformat);
		}
	}

	return 0;
}

/* empty_list --> valid_list */
static int vi_fill_out_buffer_done(struct vi_comp_ctx *vi_comp, struct vin_buffer *vin_buf)
{
	video_frame_node *first_node = NULL;
	comp_buffer_header_type buffer_header;
	int i;

	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));
	mutex_lock(&vi_comp->out_buf_manager.mutex);
	if (list_empty(&vi_comp->out_buf_manager.empty_frame_list)) {
		RT_LOGE("error: empty_frame_list is empty");
		mutex_unlock(&vi_comp->out_buf_manager.mutex);
		vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
		return -1;
	}

	first_node = list_first_entry(&vi_comp->out_buf_manager.empty_frame_list, video_frame_node, mList);
	config_videostream_by_vinbuf(vi_comp, &first_node->video_frame, vin_buf);

	//vi_comp->out_buf_manager.empty_num--;
	list_move_tail(&first_node->mList, &vi_comp->out_buf_manager.valid_frame_list);

	mutex_unlock(&vi_comp->out_buf_manager.mutex);

	buffer_header.private = &first_node->video_frame;

	#if ENABLE_SAVE_VIN_OUT_DATA
		vi_comp_save_outdata(vi_comp, &first_node->video_frame);
	#endif

	if (rt_need_fill_data(vi_comp))
		vi_comp_fill_buffer_to_16_align(vi_comp, &first_node->video_frame);

	//send out_buf to all tunnel components
	int nValidNum = 0;
	error_type ret;
	first_node->video_frame.refCnt = 1;
	mutex_lock(&vi_comp->tunnelLock);
	for (i = 0; i < RT_MAX_VENC_COMP_NUM; i++) {
		if (vi_comp->out_port_tunnel_infos[i].valid_flag == 1 && vi_comp->out_port_tunnel_infos[i].tunnel_comp != NULL) {
			nValidNum++;
			viComp_RefIncreaseVideoFrame(vi_comp, &first_node->video_frame, true);
			ret = comp_empty_this_in_buffer(vi_comp->out_port_tunnel_infos[i].tunnel_comp, &buffer_header);
			if (ret != ERROR_TYPE_OK) {
				if (ERROR_TYPE_VENC_REFUSE == ret) {
					//RT_LOGD("rt_venc_comp[%d-%p] refuses frame, maybe limited frame number", i, vi_comp->out_port_tunnel_infos[i].tunnel_comp);
				} else {
					RT_LOGE("fatal error! send to tunnel comp[%d-%p] fail[%d]", i, vi_comp->out_port_tunnel_infos[i].tunnel_comp, ret);
				}
				viComp_RefReduceVideoFrame(vi_comp, &first_node->video_frame);
			}
		}
	}
	mutex_unlock(&vi_comp->tunnelLock);
	if (0 == nValidNum) {
		if (vi_comp->base_config.output_mode != OUTPUT_MODE_YUV) {
			RT_LOGE("fatal error! no tunnel_comp, return to isp");
		}
	}

	//if user want to get frame directly, ref one this frame and operate user frame lists to record this frame info.
	mutex_lock(&vi_comp->UserFrameLock);
	if (vi_comp->bUserReqFlag) {
		if (!list_empty(&vi_comp->readyUserFrameList)) {
			RT_LOGE("fatal error! why valid user frame list not empty? check code!");
		}
		if (list_empty(&vi_comp->idleUserFrameList)) {
			RT_LOGE("fatal error! why idle user frame list empty? malloc one!");
			video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
			if (NULL == pNode) {
				RT_LOGE("fatal error! kmalloc fail!");
			}
			memset(pNode, 0, sizeof(video_frame_node));
			list_add_tail(&pNode->mList, &vi_comp->idleUserFrameList);
		}
		viComp_RefIncreaseVideoFrame(vi_comp, &first_node->video_frame, true);

		video_frame_node *pUserFrameNode = list_first_entry(&vi_comp->idleUserFrameList, video_frame_node, mList);
		memcpy(&pUserFrameNode->video_frame, &first_node->video_frame, sizeof(video_frame_s));
		list_move_tail(&pUserFrameNode->mList, &vi_comp->readyUserFrameList);
		vi_comp->bUserReqFlag = false;

		//RT_LOGD("rt_media_chn[%d] rt_vi_comp prepare one user frame for user to get, vin_buf[%p]",
		//	vi_comp->base_config.channel_id, pUserFrameNode->video_frame.private);
		vi_comp->bUserFrameReadyCondition = true;
		wake_up(&vi_comp->stUserFrameReadyWaitQueue);
	}
	mutex_unlock(&vi_comp->UserFrameLock);

	viComp_RefReduceVideoFrame(vi_comp, &first_node->video_frame);

	return 0;
}

static int check_adt_fps_reduce_frame(vi_comp_ctx *vi_comp, uint64_t pts)
{
	adapter_venc_fps_info *venc_fps_info = &vi_comp->base_config.st_adapter_venc_fps;
	uint64_t videoFramerate = (uint64_t)venc_fps_info->dst_fps;

	RT_LOGI("all frame, pts %lld %lld", pts, venc_fps_info->cur_wanted_pts);

	if (videoFramerate == 0) {
		RT_LOGE("err, dst_fps is 0!");
		return 0;
	}
	if (-1 == venc_fps_info->frame_cnt) {
		venc_fps_info->base_pts = pts;
		venc_fps_info->cur_wanted_pts = venc_fps_info->base_pts;
		venc_fps_info->frame_cnt = 0;
	}
	if (pts >= venc_fps_info->cur_wanted_pts) {
		venc_fps_info->frame_cnt++;
		venc_fps_info->cur_wanted_pts = venc_fps_info->base_pts + venc_fps_info->frame_cnt * (1000 * 1000) * 1000 / videoFramerate;
		RT_LOGI("cur_wanted_pts %lld cnt %d", venc_fps_info->cur_wanted_pts, venc_fps_info->frame_cnt);
	} else {
		return 1;
	}
	return 0;
}

static int dequeue_count;

int dequeue_buffer(vi_comp_ctx *vi_comp)
{
	struct vin_buffer *vin_buf = NULL;
	int ret			   = 0;

	if (vi_comp->base_config.bonline_channel) {
		RT_LOGD(" chanel_id %d online mode not need this", vi_comp->base_config.channel_id);
		return -1;
	}
	ret = vin_dqbuffer_special(vi_comp->vipp_id, &vin_buf);

	if (ret != 0) {
		RT_LOGI("channel %d vin_dqbuffer_special failed.", vi_comp->vipp_id);
		return -1;
	}
	RT_LOGD("vipp%d paddr %px", vi_comp->vipp_id, vin_buf->paddr);
	dequeue_count++;

	if (dequeue_count == 1) {
		int64_t time_cur = get_cur_time();
		RT_LOGW("wait first dequeue time = %lld", time_cur - time_first_cb);
	}

	//vi_comp->base_config.output_mode == OUTPUT_MODE_YUV is not important now.
	if (vi_comp->base_config.drop_frame_num > 0) {
		RT_LOGI("drop_frame_num = %d", vi_comp->base_config.drop_frame_num);
		vi_comp->base_config.drop_frame_num--;
		vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
	} else {
		int badt_fps_drop_flag = 0;
		if (vi_comp->base_config.st_adapter_venc_fps.enable)
			badt_fps_drop_flag = check_adt_fps_reduce_frame(vi_comp, vin_buf->vb.vb2_buf.timestamp);

		if (badt_fps_drop_flag)
			vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
		else
			vi_fill_out_buffer_done(vi_comp, vin_buf);
	}

	return 0;
}

static int vipp_create(vi_comp_ctx *vi_comp)
{
	RT_LOGI("vipp create not support");

	return -1;
}

static int lbc_cal_fill_16_align_data(vi_comp_ctx *vi_comp)
{
	unsigned int width =  vi_comp->base_config.width;
	unsigned int width_32_align =  RT_ALIGN(vi_comp->base_config.width, 32);
	unsigned int height =  vi_comp->base_config.height;
	unsigned int height_16_align =  RT_ALIGN(vi_comp->base_config.height, 16);
	VI_DMA_LBC_CFG_S stLbcCfg;
	VI_DMA_LBC_STM_S stLbcStm;
	unsigned int bs_len = 0;
	unsigned int frm_bit = 0;

	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		vi_comp->s_vi_lbc_fill_param.line_tar_bits[0] = RT_ALIGN(LBC_2_5X_COM_RATIO_EVEN * width_32_align * LBC_BIT_DEPTH/1000, 512);
		vi_comp->s_vi_lbc_fill_param.line_tar_bits[1] = RT_ALIGN(LBC_2_5X_COM_RATIO_ODD * width_32_align * LBC_BIT_DEPTH/500, 512);
	} else if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		vi_comp->s_vi_lbc_fill_param.line_tar_bits[0] = RT_ALIGN(LBC_2X_COM_RATIO_EVEN * width_32_align * LBC_BIT_DEPTH/1000, 512);
		vi_comp->s_vi_lbc_fill_param.line_tar_bits[1] = RT_ALIGN(LBC_2X_COM_RATIO_ODD * width_32_align * LBC_BIT_DEPTH/500, 512);
	} else {
		RT_LOGW("rt_media now only support LBC_25X/2X.");
	}

	memset(&stLbcCfg, 0, sizeof(VI_DMA_LBC_CFG_S));
	stLbcCfg.frm_wth = width;
	stLbcCfg.frm_hgt = height_16_align - height;
	stLbcCfg.line_tar_bits[0] = vi_comp->s_vi_lbc_fill_param.line_tar_bits[0];
	stLbcCfg.line_tar_bits[1] = vi_comp->s_vi_lbc_fill_param.line_tar_bits[1];

	memset(&stLbcStm, 0, sizeof(VI_DMA_LBC_STM_S));
	bs_len = stLbcCfg.frm_wth * stLbcCfg.frm_hgt * 4;
	stLbcStm.bs = (unsigned char *)kmalloc(bs_len, GFP_KERNEL);
	if (NULL == stLbcStm.bs) {
		RT_LOGE("fatal error! malloc stLbcStm.bs failed! size=%d", bs_len);
		return -1;
	}
	memset(stLbcStm.bs, 0, bs_len);

	frm_bit = lbcLine_Cmp(&stLbcCfg, &stLbcStm);
	if (frm_bit > bs_len) {
		RT_LOGE("fatal error! wrong frm_bit:%d > bs_len:%d", frm_bit, bs_len);
		if (stLbcStm.bs) {
			kfree(stLbcStm.bs);
			stLbcStm.bs = NULL;
		}
		return -1;
	}

	if (vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr) {
		RT_LOGW("LbcFillDataAddr %px is not NULL! free it before malloc.", vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr);
		kfree(vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr);
		vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr = NULL;
	}

	vi_comp->s_vi_lbc_fill_param.mLbcFillDataLen = frm_bit;
	vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr = (unsigned char *)kmalloc(vi_comp->s_vi_lbc_fill_param.mLbcFillDataLen, GFP_KERNEL);
	if (NULL == vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr) {
		RT_LOGE("fatal error! malloc LbcFillDataAddr failed! size=%d", vi_comp->s_vi_lbc_fill_param.mLbcFillDataLen);
		if (stLbcStm.bs) {
			kfree(stLbcStm.bs);
			stLbcStm.bs = NULL;
		}
		return -1;
	}
	memcpy(vi_comp->s_vi_lbc_fill_param.mLbcFillDataAddr, stLbcStm.bs, vi_comp->s_vi_lbc_fill_param.mLbcFillDataLen);

	if (stLbcStm.bs) {
		kfree(stLbcStm.bs);
		stLbcStm.bs = NULL;
	}
	return 0;
}

static int alloc_vin_buf(vi_comp_ctx *vi_comp)
{
	int i = 0;
	int ret = 0;
	int buf_size		  = vi_comp->align_width * vi_comp->align_height * 3 / 2;
	unsigned int lbc_ext_size = 0;
	int buf_num = 0;

	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		int y_stride		   = 0;
		int yc_stride		   = 0;
		int bit_depth		   = 8;
		int com_ratio_even	 = 0;
		int com_ratio_odd	  = 0;
		int pic_width_32align      = (vi_comp->base_config.width + 31) & ~31;
		int pic_height_16align     = (vi_comp->base_config.height + 15) & ~15;
		int bLbcLossyComEnFlag2x   = 0;
		int bLbcLossyComEnFlag2_5x = 0;

		if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
			bLbcLossyComEnFlag2_5x = 1;
		} else if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
			bLbcLossyComEnFlag2x = 1;
		}

		if (rt_need_fill_data(vi_comp) && rt_is_lbc_format(vi_comp)) {
			lbc_cal_fill_16_align_data(vi_comp);
		}

		RT_LOGD("bLbcLossyComEnFlag2x %d bLbcLossyComEnFlag2_5x %d", bLbcLossyComEnFlag2x, bLbcLossyComEnFlag2_5x);
		if (bLbcLossyComEnFlag2x == 1) {
			com_ratio_even = LBC_2X_COM_RATIO_EVEN;
			com_ratio_odd  = LBC_2X_COM_RATIO_ODD;
			y_stride       = ((com_ratio_even * pic_width_32align * bit_depth / 1000 + 511) & (~511)) >> 3;
			yc_stride      = ((com_ratio_odd * pic_width_32align * bit_depth / 500 + 511) & (~511)) >> 3;
		} else if (bLbcLossyComEnFlag2_5x == 1) {
			com_ratio_even = LBC_2_5X_COM_RATIO_EVEN;
			com_ratio_odd  = LBC_2_5X_COM_RATIO_ODD;
			y_stride       = ((com_ratio_even * pic_width_32align * bit_depth / 1000 + 511) & (~511)) >> 3;
			yc_stride      = ((com_ratio_odd * pic_width_32align * bit_depth / 500 + 511) & (~511)) >> 3;
		} else {
			y_stride  = ((pic_width_32align * bit_depth + pic_width_32align / 16 * 2 + 511) & (~511)) >> 3;
			yc_stride = ((pic_width_32align * bit_depth * 2 + pic_width_32align / 16 * 4 + 511) & (~511)) >> 3;
		}

		buf_size = (y_stride + yc_stride) * pic_height_16align / 2;

		//* add more 1KB to fix ve-lbc-error
		lbc_ext_size = LBC_EXT_SIZE;
		buf_size += lbc_ext_size;

		RT_LOGD("LBC in buf:com_ratio: %d, %d, w32alin = %d, h16alin = %d, y_s = %d, yc_s = %d, total_len = %d,\n",
			com_ratio_even, com_ratio_odd, pic_width_32align, pic_height_16align, y_stride, yc_stride, buf_size);
	} else if (rt_is_format422(vi_comp->base_config.pixelformat)) {
		buf_size = vi_comp->align_width * vi_comp->align_height * 2;
		if (vi_comp->align_width % 64 != 0)
			buf_size += 64;
	} else {//default as yuv420, add other format please additionally add
		buf_size = vi_comp->align_width * vi_comp->align_height * 3 / 2;
		if (vi_comp->align_width % 64 != 0)
			buf_size += 64;
	}

	vi_comp->buf_size = buf_size - lbc_ext_size;
	RT_LOGD("buf_size = %d, align_w = %d, align_h = %d", buf_size, vi_comp->align_width, vi_comp->align_height);
	// if (vi_comp->memops == NULL) {
	// 	vi_comp->memops = mem_create(MEM_TYPE_ION, param);
	// 	if (vi_comp->memops == NULL) {
	// 		RT_LOGE("mem_create failed\n");
	// 		return -1;
	// 	}
	//}
	{
//		RT_LOGD("channel %d vin_buf_num %d", vi_comp->base_config.channel_id, vi_comp->base_config.vin_buf_num);
//		int buf_num = vi_comp->base_config.vin_buf_num;

//		if (vi_comp->base_config.bonline_channel)
//			buf_num = vi_comp->base_config.share_buf_num;

//		if (vi_comp->base_config.output_mode == OUTPUT_MODE_YUV)
//			buf_num = CONFIG_YUV_BUF_NUM;

//		vi_comp->buf_num = buf_num;
		for (i = 0; i < vi_comp->buf_num; i++) {
			vi_comp->vin_buf[i].dmabuf = ion_alloc(buf_size, ION_HEAP_CUSTOM_START_MASK, 0);
			if (vi_comp->vin_buf[i].dmabuf == NULL) {
				RT_LOGE("ion_alloc ION_HEAP_CUSTOM_START_MASK failed, buf size=%d\n", buf_size);
				return -1;
			}
			RT_LOGD("vipp%d vin_buf[%d] alloc dmabuf %p, buf_size %d", vi_comp->vipp_id, i, vi_comp->vin_buf[i].dmabuf, buf_size);

			if ((vi_comp->base_config.output_mode == OUTPUT_MODE_STREAM && vi_comp->base_config.en_16_align_fill_data) ||
					(vi_comp->base_config.dma_merge_mode >= DMA_STITCH_HORIZONTAL)) {
				vi_comp->vin_buf[i].vir_addr = dma_buf_vmap(vi_comp->vin_buf[i].dmabuf);
			}

			ret = vin_qbuffer_special(vi_comp->vipp_id, &vi_comp->vin_buf[i]);

			//if (vi_comp->base_config.output_mode == OUTPUT_MODE_YUV) {
				vi_comp->s_yuv_info[i].phy_addr = vi_comp->vin_buf[i].paddr;
				//vi_comp->vin_buf_user[i].paddr = vi_comp->vin_buf[i].paddr;
			//}

			RT_LOGD("palloc vin buf: vir = 0x%px, phy = 0x%px, i = %d/%d",
				vi_comp->vin_buf[i].vir_addr, vi_comp->vin_buf[i].paddr, i, vi_comp->buf_num);
		}
	}
	return 0;
}

static void free_vin_buf(vi_comp_ctx *vi_comp)
{
	int i = 0;

	for (i = 0; i < vi_comp->buf_num; i++) {
		if (vi_comp->vin_buf[i].paddr) {
			RT_LOGD("vipp%d vin_buf[%d] free dmabuf %p, paddr %p", vi_comp->vipp_id, i,
				vi_comp->vin_buf[i].dmabuf, vi_comp->vin_buf[i].paddr);
			if (vi_comp->vin_buf[i].vir_addr) {
				dma_buf_vunmap(vi_comp->vin_buf[i].dmabuf, vi_comp->vin_buf[i].vir_addr);
				vi_comp->vin_buf[i].vir_addr = NULL;
			}
			dma_buf_put(vi_comp->vin_buf[i].dmabuf);
		}
	}
}

int convert_to_v4l2_color_space(VencH264VideoSignal *venc_video_signal)
{
	if (venc_video_signal->src_colour_primaries == VENC_BT709) {
		if (venc_video_signal->full_range_flag)
			return V4L2_COLORSPACE_REC709;
		else
			return V4L2_COLORSPACE_REC709_PART_RANGE;
	}

	if (VENC_YCC == venc_video_signal->src_colour_primaries)
		return V4L2_COLORSPACE_JPEG;

	RT_LOGW("not support color space %d set default to V4L2_COLORSPACE_REC709", venc_video_signal->src_colour_primaries);
	return V4L2_COLORSPACE_REC709;
}

int rt_is_format422(rt_pixelformat_type  pixelformat)
{
	if (pixelformat >= RT_PIXEL_YUV422SP && pixelformat <= RT_PIXEL_VYUY422)
		return 1;
	else
		return 0;
}

__u32 vi_change_pixelformat_2_vin(rt_pixelformat_type  pixelformat)
{
	__u32 v4l2_pix_fmt = 0;

	switch (pixelformat) {
	case RT_PIXEL_YUV420SP:
		v4l2_pix_fmt = V4L2_PIX_FMT_NV12;
		break;
	case RT_PIXEL_YVU420SP:
		v4l2_pix_fmt = V4L2_PIX_FMT_NV21;
		break;
	case RT_PIXEL_YUV420P:
		v4l2_pix_fmt = V4L2_PIX_FMT_YUV420;
		break;
	case RT_PIXEL_YVU420P:
		v4l2_pix_fmt = V4L2_PIX_FMT_YVU420;
		break;
	case RT_PIXEL_LBC_25X:
		v4l2_pix_fmt = V4L2_PIX_FMT_LBC_2_5X;
		break;
	case RT_PIXEL_LBC_2X:
		v4l2_pix_fmt = V4L2_PIX_FMT_LBC_2_0X;
		break;
	case RT_PIXEL_YUV422SP:
		v4l2_pix_fmt = V4L2_PIX_FMT_NV16;
		break;
	case RT_PIXEL_YVU422SP:
		v4l2_pix_fmt = V4L2_PIX_FMT_NV61;
		break;
	//blow 422 format csi not support, reserve.
	case RT_PIXEL_YUV422P:
		v4l2_pix_fmt = V4L2_PIX_FMT_YUV422P;
		break;
	case RT_PIXEL_YVU422P:
		v4l2_pix_fmt = V4L2_PIX_FMT_YYUV;
		break;

	case RT_PIXEL_YUYV422:
		v4l2_pix_fmt = V4L2_PIX_FMT_YUYV;
		break;
	case RT_PIXEL_UYVY422:
		v4l2_pix_fmt = V4L2_PIX_FMT_UYVY;
		break;
	case RT_PIXEL_YVYU422:
		v4l2_pix_fmt = V4L2_PIX_FMT_YVYU;
		break;
	case RT_PIXEL_VYUY422:
		v4l2_pix_fmt = V4L2_PIX_FMT_VYUY;
		break;
	default:
		v4l2_pix_fmt = V4L2_PIX_FMT_YUV420M;
		break;
	}
	return v4l2_pix_fmt;
}

int vipp_init(vi_comp_ctx *vi_comp)
{
	int ret = 0;
	int id  = vi_comp->base_config.channel_id;
	struct v4l2_streamparm parms;
	struct v4l2_format fmt;
	int wdr_param = 0;
	struct csi_ve_online_cfg online_cfg;

	memset(&online_cfg, 0, sizeof(struct csi_ve_online_cfg));

	RT_LOGW("vipp init: widht = %d, height = %d, fps = %d, id = %d, format = %d, dma_merge_mode = %d",
		vi_comp->base_config.width, vi_comp->base_config.height,
		vi_comp->base_config.frame_rate, id, vi_comp->base_config.pixelformat, vi_comp->base_config.dma_merge_mode);

	callback_count = 0;
	dequeue_count  = 0;
	time_vi_start  = 0;
	time_first_cb  = 0;

	if (vi_comp->base_config.enable_high_fps_transfor == 0)
		ret = vin_open_special(id);
	RT_LOGI("vin open, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_start = get_cur_time();
#endif

	/* Set dma_merge_demo for stitch */
	if (vi_comp->base_config.dma_merge_mode > DMA_STITCH_NONE) {
		vin_s_dma_merge_mode_special(id, vi_comp->base_config.dma_merge_mode);
	}

	if (vi_comp->base_config.enable_high_fps_transfor == 0)
		ret = vin_s_input_special(id, 0);
	RT_LOGI("vin s input, ret = %d, id = %d", ret, id);
	// if (vi_comp->base_config.enable_aiisp && vi_comp->base_config.tdm_rxbuf_cnt)
	// 	vin_set_tdm_rxbuf_cnt_special(vi_comp->base_config.channel_id, &vi_comp->base_config.tdm_rxbuf_cnt);
	if (vi_comp->base_config.width == 0 || vi_comp->base_config.height == 0) {
		struct sensor_resolution Sensor_resolution;
		memset(&Sensor_resolution, 0, sizeof(struct sensor_resolution));

		vin_get_sensor_resolution_special(id, &Sensor_resolution);
		RT_LOGW("width_max %d height_max %d", Sensor_resolution.width_max, Sensor_resolution.height_max);
		if (Sensor_resolution.width_max && Sensor_resolution.height_max) {
			if (Sensor_resolution.width_max > 1920) {
				vi_comp->base_config.width = 1920;
				vi_comp->base_config.height = 1088;
			} else {
				vi_comp->base_config.width = Sensor_resolution.width_max;
				vi_comp->base_config.height = Sensor_resolution.height_max;
			}
		} else {
			vi_comp->base_config.width = 640;
			vi_comp->base_config.height = 480;
		}
	}

	online_cfg.ve_online_en = vi_comp->base_config.bonline_channel;
	online_cfg.dma_buf_num = vi_comp->base_config.share_buf_num;
	ret = vin_set_ve_online_cfg_special(id, &online_cfg);
	RT_LOGD("vin_set_ve_online_cfg_special, ret = %d, id = %d", ret, id);
#if RT_VI_SHOW_TIME_COST
	int64_t time3_end = get_cur_time();
	RT_LOGE("time of s input: %lld", (time3_end - time3_start));
#endif

	if (vi_comp->base_config.enable_wdr == 1)
		wdr_param = 1;
	else
		wdr_param = 0;
	RT_LOGD("wdr_param = %d, enable_wdr = %d", wdr_param, vi_comp->base_config.enable_wdr);

	/* set parameter */
	CLEAR(parms);
	parms.type				    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	parms.parm.capture.timeperframe.numerator   = 1;
	parms.parm.capture.timeperframe.denominator = vi_comp->base_config.frame_rate;
	parms.parm.capture.capturemode		    = V4L2_MODE_VIDEO;
	/* parms.parm.capture.capturemode = V4L2_MODE_IMAGE; */
	/*when different video have the same sensor source, 1:use sensor current win, 0:find the nearest win*/
	parms.parm.capture.reserved[0] = 0;
	parms.parm.capture.reserved[1] = wdr_param; /*2:command, 1: wdr, 0: normal*/

	ret = vin_s_parm_special(id, NULL, &parms);
	RT_LOGD("vin s parm, ret = %d, id = %d", ret, id);

	/* set format */
	CLEAR(fmt);
	fmt.type	      = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width  = vi_comp->base_config.width;
	fmt.fmt.pix_mp.height = vi_comp->base_config.height;
	fmt.fmt.pix_mp.colorspace =
		convert_to_v4l2_color_space(&vi_comp->base_config.venc_video_signal);
	RT_LOGI("fmt.fmt.pix_mp.colorspace %d %d", fmt.fmt.pix_mp.colorspace, V4L2_COLORSPACE_REC709);
	fmt.fmt.pix_mp.pixelformat = vi_change_pixelformat_2_vin(vi_comp->base_config.pixelformat);
	fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;

#if RT_VI_SHOW_TIME_COST
	int64_t time2_start = get_cur_time();
#endif

	ret = vin_s_fmt_special(id, &fmt);
	RT_LOGD("vin s fmt, ret = %d, id = %d", ret, id);

	if (vi_comp->base_config.dma_merge_scaler_cfg.scaler_en) {
		RT_LOGI("SensorA: %dx%d, SensorB: %dx%d",
			vi_comp->base_config.dma_merge_scaler_cfg.sensorA_scaler_cfg.nWidth,
			vi_comp->base_config.dma_merge_scaler_cfg.sensorA_scaler_cfg.nHeight,
			vi_comp->base_config.dma_merge_scaler_cfg.sensorB_scaler_cfg.nWidth,
			vi_comp->base_config.dma_merge_scaler_cfg.sensorB_scaler_cfg.nHeight);
		vin_set_sclaer_resolution_special(id, &vi_comp->base_config.dma_merge_scaler_cfg);
	}

#if RT_VI_SHOW_TIME_COST
	int64_t time2_end = get_cur_time();
	RT_LOGE("time of s fmt: %lld", (time2_end - time2_start));
#endif

	vin_g_fmt_special(id, &fmt);
	RT_LOGD("vin g fmt, ret = %d, id = %d", ret, id);

	RT_LOGD("resolution got from sensor = %d*%d num_planes = %d\n",
		fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
		fmt.fmt.pix_mp.num_planes);

	/* queue buffer */
	vi_comp->vipp_id = id;
	g_vi_comp[id]    = vi_comp;

	vin_register_buffer_done_callback(id, vin_buffer_callback);

#if RT_VI_SHOW_TIME_COST
	int64_t time_start = get_cur_time();
#endif

#if RT_VI_SHOW_TIME_COST
	int64_t time_end = get_cur_time();
	RT_LOGE("time of alloc buf: %lld", (time_end - time_start));
#endif

/* stream on */
#if RT_VI_SHOW_TIME_COST
	int64_t time1_start = get_cur_time();
#endif

	time_vi_start = get_cur_time();

#if RT_VI_SHOW_TIME_COST
	int64_t time1_end = get_cur_time();

	RT_LOGE("time of stream on: %lld", (time1_end - time1_start));
#endif
	RT_LOGD("ret %d", ret);
	return ret;
}

/**
  first check out_buf list, if find none, then notify and wait rt_vi_thread to get new frame.
  @return
    <0: error
    >=0: yuv frame size.
*/
static int request_yuv_frame(struct vi_comp_ctx *vi_comp, rt_yuv_info *p_yuv_info)
{
	unsigned long flags;
	int ret			   = -1;
	int result = 0;
	struct vin_buffer *vin_buf = NULL;
	int data_size		   = vi_comp->align_width * vi_comp->base_config.height * 3 / 2;
	int nTimeOut = 5000; //unit:ms
	int i = 0;

	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X)
		data_size = vi_comp->buf_size;

	//find frame in out_buf list first.
	video_frame_node *pFrameNode = NULL;
	mutex_lock(&vi_comp->out_buf_manager.mutex);
	if (!list_empty(&vi_comp->out_buf_manager.valid_frame_list)) {
		//if out_buf valid frame list has nodes, we get latest node from it.
		pFrameNode = list_last_entry(&vi_comp->out_buf_manager.valid_frame_list, video_frame_node, mList);
		viComp_RefIncreaseVideoFrame(vi_comp, &pFrameNode->video_frame, false);
	}
	mutex_unlock(&vi_comp->out_buf_manager.mutex);
	if (pFrameNode != NULL) {
		mutex_lock(&vi_comp->UserFrameLock);
		if (!list_empty(&vi_comp->readyUserFrameList)) {
			RT_LOGE("fatal error! ready user frame list not empty? check code!");
		}
		if (list_empty(&vi_comp->idleUserFrameList)) {
			RT_LOGE("fatal error! why idle user frame list is empty?");
			video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
			if (NULL == pNode) {
				RT_LOGE("fatal error! kmalloc fail!");
			}
			memset(pNode, 0, sizeof(video_frame_node));
			list_add_tail(&pNode->mList, &vi_comp->idleUserFrameList);
		}
		video_frame_node *pUserFrameNode = list_first_entry(&vi_comp->idleUserFrameList, video_frame_node, mList);
		memcpy(&pUserFrameNode->video_frame, &pFrameNode->video_frame, sizeof(video_frame_s));
		list_move_tail(&pUserFrameNode->mList, &vi_comp->usingUserFrameList);
		vin_buf = (struct vin_buffer *)pUserFrameNode->video_frame.private;
		mutex_unlock(&vi_comp->UserFrameLock);
	} else {
		//if not find frame in out_buf list, then notify and wait rt_vi_thread to get new frame.
		mutex_lock(&vi_comp->UserFrameLock);
		if (!list_empty(&vi_comp->readyUserFrameList)) {
			RT_LOGE("fatal error! why valid user frame list not empty? check code!");
		}
		vi_comp->bUserReqFlag = true;
		vi_comp->bUserFrameReadyCondition = false;
		mutex_unlock(&vi_comp->UserFrameLock);
		ret = wait_event_timeout(vi_comp->stUserFrameReadyWaitQueue, (true == vi_comp->bUserFrameReadyCondition),
			msecs_to_jiffies(nTimeOut));
		mutex_lock(&vi_comp->UserFrameLock);

		if (ret != 0) {
			if (vi_comp->bUserReqFlag != false) {
				RT_LOGE("fatal error! user req flag should be false!");
				vi_comp->bUserReqFlag = false;
			}
			if (!list_empty(&vi_comp->readyUserFrameList)) {
				video_frame_node *pNode = list_first_entry(&vi_comp->readyUserFrameList, video_frame_node, mList);
				vin_buf = (struct vin_buffer *)pNode->video_frame.private;
				list_move_tail(&pNode->mList, &vi_comp->usingUserFrameList);
			} else {
				RT_LOGE("fatal error! why valid user frame list empty? check code!");
				result = -1;
			}
		} else {
			RT_LOGE("fatal error! request yuv frame timeout[%d]?", ret);
			if (vi_comp->bUserReqFlag != true) {
				RT_LOGE("fatal error! But possible! user req flag is not true");
			}
			vi_comp->bUserReqFlag = false;
			if (!list_empty(&vi_comp->readyUserFrameList)) {
				RT_LOGE("fatal error! But possible, valid user frame list is filled with one node after unlock mutex!");
				video_frame_node *pNode = list_first_entry(&vi_comp->readyUserFrameList, video_frame_node, mList);
				vin_buf = (struct vin_buffer *)pNode->video_frame.private;
				list_move_tail(&pNode->mList, &vi_comp->usingUserFrameList);
			} else {
				result = -1;
			}
		}
		mutex_unlock(&vi_comp->UserFrameLock);
	}

	if (NULL == vin_buf) {
		RT_LOGE("request yuv frame: dequeue buf failed");
		return -1;
	}
	RT_LOGI("request yuvFrame ret = %d, data_size = %d, vin_buf[%p][addr:0x%px-0x%px] from [%s]", result,
		data_size, vin_buf, vin_buf->paddr, vin_buf->vir_addr,
		pFrameNode?"vi_out_buf_manager":"vin_driver");
	p_yuv_info->phy_addr = (unsigned char *)vin_buf->paddr;

	for (i = 0; i < vi_comp->buf_num; i++) { //CONFIG_YUV_BUF_NUM
		//RT_LOGD("vi_comp->s_yuv_info[i].fd %d", vi_comp->s_yuv_info[i].fd);
		if (p_yuv_info->phy_addr == vi_comp->s_yuv_info[i].phy_addr) {
			if (vin_buf != &vi_comp->vin_buf[i]) {
				RT_LOGE("fatal error! vin_buf[%p!=%p-%d]", vin_buf, &vi_comp->vin_buf[i], i);
			}
			if (vi_comp->s_yuv_info[i].bset)
				p_yuv_info->fd = vi_comp->s_yuv_info[i].fd;
			else {
				vi_comp->s_yuv_info[i].fd = dma_buf_fd(vi_comp->vin_buf[i].dmabuf, O_RDWR | O_CLOEXEC);
				//vi_comp->vin_buf_user[i].dmabuf_fd = vi_comp->s_yuv_info[i].fd;
				if (vi_comp->s_yuv_info[i].fd < 0) {
					RT_LOGE("get fd error!! vi_comp->s_yuv_info[i].fd %d", vi_comp->s_yuv_info[i].fd);
				}
				p_yuv_info->fd = vi_comp->s_yuv_info[i].fd;
				vi_comp->s_yuv_info[i].bset = 1;
			}
			break;
		}
	}
	p_yuv_info->width = vi_comp->base_config.width;
	p_yuv_info->height = vi_comp->base_config.height;
	if (i >= vi_comp->buf_num)
		RT_LOGE("fatal error! get yuv mem fd err.");
	return data_size;
}

static int return_yuv_frame(struct vi_comp_ctx *vi_comp, unsigned char *phy_addr)
{
	int ret = 0;
	int rc;
	error_type error;
	video_frame_node *pFrameNode = NULL;
	struct vin_buffer *vin_buf = NULL;
	mutex_lock(&vi_comp->UserFrameLock);
	if (!list_empty(&vi_comp->usingUserFrameList)) {
		video_frame_node *pEntry;
		list_for_each_entry (pEntry, &vi_comp->usingUserFrameList, mList) {
			vin_buf = (struct vin_buffer *)pEntry->video_frame.private;
			if (vin_buf->paddr == phy_addr) {
				pFrameNode = pEntry;
				break;
			}
		}
		if (pFrameNode != NULL) {
			pFrameNode->video_frame.private = NULL;
			list_move_tail(&pFrameNode->mList, &vi_comp->idleUserFrameList);
		} else {
			RT_LOGE("fatal error! can not match phy_addr: %p", phy_addr);
			ret = -1;
		}
	} else {
		RT_LOGE("fatal error! why using user frame list empty? check code!");
		ret = -1;
	}
	mutex_unlock(&vi_comp->UserFrameLock);

	if (vin_buf != NULL) {
		//find matching node in out buf manager
		video_frame_node *pOutBufFrameNode = NULL;
		int num = 0;
		video_frame_node *pEntry = NULL;
		mutex_lock(&vi_comp->out_buf_manager.mutex);
		list_for_each_entry (pEntry, &vi_comp->out_buf_manager.valid_frame_list, mList) {
			num++;
			if (pEntry->video_frame.private == vin_buf) {
				pOutBufFrameNode = pEntry;
				break;
			}
		}
		mutex_unlock(&vi_comp->out_buf_manager.mutex);
		if (NULL == pOutBufFrameNode) {
			RT_LOGE("fatal error! not find matching node[%p] in out_buf manager, valid frame num[%d]", vin_buf, num);
		}
		//ref reduce frame.
		error = viComp_RefReduceVideoFrame(vi_comp, &pOutBufFrameNode->video_frame);
		if (error != ERROR_TYPE_OK) {
			RT_LOGE("fatal error! rt_media_chn[%d] rt_vi_comp ref reduce frame id[%d] fail:%d", vi_comp->base_config.channel_id,
				pOutBufFrameNode->video_frame.id, error);
		}
	}
	return ret;
}

static int vi_conifg_dma_overlay(struct vi_comp_ctx *vi_comp)
{
	int ret;

	if (!vi_comp) {
		RT_LOGE("vi_comp is NULL!!!\n");
		return -1;
	}

	if (vi_comp->base_config.dma_merge_mode == DMA_STITCH_VERTICAL) {
		if (vi_comp->base_config.dma_overlay_ctx.dma_overlay_en) {
			if (vi_comp->base_config.dma_overlay_ctx.overlay_width != 0 && vi_comp->base_config.dma_overlay_ctx.overlay_height) {
				if (vi_comp->base_config.dma_overlay_ctx.overlay_data) {
					vin_set_dma_overlay_special(vi_comp->base_config.channel_id, &vi_comp->base_config.dma_overlay_ctx);
				} else {
					RT_LOGE("dma_overlay_en = %d, but overlay_data is NULL, it will disable dma_overlay_en\n",
						vi_comp->base_config.dma_overlay_ctx.dma_overlay_en);
					vi_comp->base_config.dma_overlay_ctx.dma_overlay_en = 0;
				}
			} else {
				RT_LOGE("dma_overlay is %d*%d, it will disable dma_overlay_en, please check it!!!\n",
					vi_comp->base_config.dma_overlay_ctx.overlay_width, vi_comp->base_config.dma_overlay_ctx.overlay_height);
				vi_comp->base_config.dma_overlay_ctx.dma_overlay_en = 0;
			}
		} else {
			RT_LOGD("dma_merge_mode is DMA_STITCH_VERTICAL, but not to set dma_overlay\n");
		}
	} else {
		RT_LOGD("It will not insert dma overlay!\n");
	}

	return 0;
}

int commond_process_vi(struct vi_comp_ctx *vi_comp, message_t *msg)
{
	int ret = 0;
	int cmd_error		      = 0;
	int cmd			      = msg->command;
	MessageReply *wait_reply = (MessageReply *)msg->pReply;

	RT_LOGD("cmd process: cmd = %d, state = %d, wait_reply = %p", cmd, vi_comp->state, wait_reply);

	if (cmd == COMP_COMMAND_INIT) {
		if (vi_comp->state != COMP_STATE_IDLE) {
			cmd_error = 1;
		} else {
			vi_comp->state = COMP_STATE_INITIALIZED;
		}

	} else if (cmd == COMP_COMMAND_START) {
		if (vi_comp->state != COMP_STATE_INITIALIZED && vi_comp->state != COMP_STATE_PAUSE) {
			cmd_error = 1;
		} else {
			if (vi_comp->state == COMP_STATE_INITIALIZED) {
#if RT_VI_SHOW_TIME_COST
				int64_t time_start = get_cur_time();
#endif
				alloc_vin_buf(vi_comp);
				vi_conifg_dma_overlay(vi_comp);
				ret = vin_streamon_special(vi_comp->base_config.channel_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
				RT_LOGW("vin streamon, ret = %d, id = %d", ret, vi_comp->base_config.channel_id);
#if RT_VI_SHOW_TIME_COST
				int64_t time_end = get_cur_time();
				RT_LOGW("time of vipp_init: %lld time: %lld", (time_end - time_start), time_end);
#endif
			}

			vi_comp->state = COMP_STATE_EXECUTING;
		}

	} else if (cmd == COMP_COMMAND_PAUSE) {
		if (COMP_STATE_EXECUTING == vi_comp->state) {
			vi_comp->state = COMP_STATE_PAUSE;
		} else if (COMP_STATE_PAUSE == vi_comp->state) {
			RT_LOGD("rt_vi_comp already in pause state");
		} else {
			RT_LOGE("fatal error! rt_vi_comp state:%d is not expected", vi_comp->state);
			cmd_error = 1;
		}
	} else if (cmd == COMP_COMMAND_STOP) {
		vin_streamoff_special(vi_comp->vipp_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
		vi_comp->state = COMP_STATE_IDLE;
	} else if (cmd == COMP_COMMAND_RESET_HIGH_FPS) {
		RT_LOGW("*** process COMP_COMMAND_RESET_HIGH_FPS ***");
		vi_comp->callback->EventHandler(
			vi_comp->self_comp,
			vi_comp->callback_data,
			COMP_EVENT_CMD_RESET_ISP_HIGH_FPS,
			0,
			0,
			NULL);
	} else if (cmd == COMP_COMMAND_EXIT) {
		if (vi_comp->state != COMP_STATE_IDLE) {
			cmd_error = 1;
		} else {
			vi_comp->state = COMP_STATE_EXIT;
		}
	}

	if (cmd_error == 1) {
		vi_comp->callback->EventHandler(
			vi_comp->self_comp,
			vi_comp->callback_data,
			COMP_EVENT_CMD_ERROR,
			COMP_COMMAND_INIT,
			0,
			NULL);
	} else {
		vi_comp->callback->EventHandler(
			vi_comp->self_comp,
			vi_comp->callback_data,
			COMP_EVENT_CMD_COMPLETE,
			cmd,
			0,
			NULL);
	}

	if (wait_reply) {
		RT_LOGD("wait up: cmd = %d", cmd);
		wait_reply->ReplyResult = cmd_error;
		rt_sem_up(&wait_reply->ReplySem);
	}
	return 0;
}

error_type config_dynamic_param(
	vi_comp_ctx *vi_comp,
	comp_index_type index,
	void *param_data)
{
	error_type error = ERROR_TYPE_OK;

	RT_LOGW("not support config dynamic param yet");

	return error;
}

static int post_msg_and_wait(vi_comp_ctx *vi_comp, int cmd_id, int msg_param)
{
	int result = 0;
	message_t cmd_msg;
	InitMessage(&cmd_msg);
	cmd_msg.command	= cmd_id;
	cmd_msg.para0	  = msg_param;
	cmd_msg.pReply = ConstructMessageReply();
	putMessageWithData(&vi_comp->msg_queue, &cmd_msg);

	int ret;
	while (1) {
		ret = rt_sem_down_timedwait(&cmd_msg.pReply->ReplySem, 5000);
		if (ret != 0) {
			RT_LOGE("fatal error! rt_vi cmd[%d] wait reply fail[%d]", cmd_id, ret);
		} else {
			break;
		}
	}
	result = cmd_msg.pReply->ReplyResult;
	RT_LOGD("rt_vi cmd[%d] receive reply ret: %d!", cmd_id, result);
	DestructMessageReply(cmd_msg.pReply);
	cmd_msg.pReply = NULL;

	return result;
}

error_type vi_comp_init(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;
	int ret = 0;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_init: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	int buf_num = vi_comp->base_config.vin_buf_num;
	if (vi_comp->base_config.bonline_channel) {
		buf_num = vi_comp->base_config.share_buf_num;
	}
	vi_comp->buf_num = buf_num;
	vi_comp->vin_buf = kzalloc(sizeof(struct vin_buffer) * vi_comp->buf_num, GFP_KERNEL);
	//vi_comp->vin_buf_user = kzalloc(sizeof(struct vin_buffer) * vi_comp->buf_num, GFP_KERNEL);
	vi_comp->s_yuv_info = kzalloc(sizeof(rt_yuv_info) * vi_comp->buf_num, GFP_KERNEL);

	vipp_create(vi_comp);
	ret = vipp_init(vi_comp);
	if (ret != 0) {
		RT_LOGE("vipp_init error ret %d", ret);
		return ERROR_TYPE_VIN_ERR;
	}
	ret = post_msg_and_wait(vi_comp, COMP_COMMAND_INIT, 0);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_vi init fail:%d", ret);
	}
	return error;
}

error_type vi_comp_start(
	PARAM_IN comp_handle component)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("vi_comp start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	int ret = post_msg_and_wait(vi_comp, COMP_COMMAND_START, 0);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_vi start fail:%d", ret);
	}
	return error;
}

error_type vi_comp_pause(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("vi_comp start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	int ret = post_msg_and_wait(vi_comp, COMP_COMMAND_PAUSE, 0);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_vi pause fail:%d", ret);
	}
	return error;
}

error_type vi_comp_stop(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_stop: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	int ret = post_msg_and_wait(vi_comp, COMP_COMMAND_STOP, 0);
	if (ret != 0) {
		RT_LOGE("fatal error! rt_vi stop fail:%d", ret);
	}
	return error;
}

error_type vi_comp_destroy(PARAM_IN comp_handle component)
{
	int i				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;
	video_frame_node *frame_node    = NULL;
	int free_frame_cout		= 0;
	struct vin_buffer *vin_buf = NULL;
	int ret;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_destroy: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	//free vin hold buffer.
	vin_force_reset_buffer(vi_comp->vipp_id);
	while (vin_dqbuffer_special(vi_comp->vipp_id, &vin_buf) == 0) {
		RT_LOGD("vin_dqbuffer_special succuss!");
	}

	vin_close_special(vi_comp->vipp_id);
	/*should make sure the thread do nothing importent */
	if (vi_comp->vi_thread) {
		ret = post_msg_and_wait(vi_comp, COMP_COMMAND_EXIT, 0);
		//kthread_stop(vi_comp->vi_thread);
		if (ret != 0) {
			RT_LOGE("fatal error! rt_vi exit fail:%d", ret);
		}
	}
	message_destroy(&vi_comp->msg_queue);

	while ((!list_empty(&vi_comp->out_buf_manager.empty_frame_list))) {
		frame_node = list_first_entry(&vi_comp->out_buf_manager.empty_frame_list, video_frame_node, mList);
		if (frame_node) {
			free_frame_cout++;
			list_del(&frame_node->mList);
			kfree(frame_node);
		}
	}

	while ((!list_empty(&vi_comp->out_buf_manager.valid_frame_list))) {
		frame_node = list_first_entry(&vi_comp->out_buf_manager.valid_frame_list, video_frame_node, mList);
		if (frame_node) {
			free_frame_cout++;
			list_del(&frame_node->mList);
			kfree(frame_node);
		}
	}
	RT_LOGD("free_frame_cout = %d", free_frame_cout);
	if (free_frame_cout != vi_comp->base_config.vin_buf_num)
		RT_LOGE("free num of frame node is not match: %d, %d", free_frame_cout, vi_comp->base_config.vin_buf_num);

	free_vin_buf(vi_comp);
	if (vi_comp->vin_buf != NULL) {
		kfree(vi_comp->vin_buf);
		vi_comp->vin_buf = NULL;
	}

//	if (vi_comp->vin_buf_user != NULL) {
//		kfree(vi_comp->vin_buf_user);
//		vi_comp->vin_buf_user = NULL;
//	}

	if (vi_comp->s_yuv_info != NULL) {
		kfree(vi_comp->s_yuv_info);
		vi_comp->s_yuv_info = NULL;
	}

	int nUsingNum = 0, nReadyNum = 0, nIdleNum = 0;
	video_frame_node *pEntry, *pTemp;
	mutex_lock(&vi_comp->UserFrameLock);
	if (!list_empty(&vi_comp->usingUserFrameList)) {
		RT_LOGE("fatal error! why using user frame list not empty?");
		list_for_each_entry_safe (pEntry, pTemp, &vi_comp->usingUserFrameList, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
			nUsingNum++;
		}
		RT_LOGE("fatal error! using user frame list has [%d] nodes", nUsingNum);
	}
	if (!list_empty(&vi_comp->readyUserFrameList)) {
		RT_LOGE("fatal error! why ready user frame list not empty?");
		list_for_each_entry_safe (pEntry, pTemp, &vi_comp->readyUserFrameList, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
			nReadyNum++;
		}
		RT_LOGE("fatal error! ready user frame list has [%d] nodes", nReadyNum);
	}
	if (!list_empty(&vi_comp->idleUserFrameList)) {
		list_for_each_entry_safe (pEntry, pTemp, &vi_comp->idleUserFrameList, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
			nIdleNum++;
		}
	}
	mutex_unlock(&vi_comp->UserFrameLock);

	kfree(vi_comp);

	return error;
}

error_type vi_comp_get_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_INOUT void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("vi_comp_get_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	switch (index) {
	case COMP_INDEX_VI_CONFIG_Dynamic_ISP_SEI_INFO: {
		struct isp_sei_info *pInfo = (struct isp_sei_info *)param_data;

		vin_get_isp_sei_info_special(vi_comp->vipp_id, pInfo);

		RT_LOGD("get isp sei info");

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_ISP_ARRT_CFG: {
		struct isp_cfg_attr_data *p_isp_attr_cfg = (struct isp_cfg_attr_data *)param_data;

		vin_get_isp_attr_cfg_special(vi_comp->vipp_id, (void *)p_isp_attr_cfg);

		RT_LOGD("set isp attr cfg");

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_SENSOR_NAME: {
		char *sensor_name = (char *)param_data;

		vin_get_sensor_name_special(vi_comp->vipp_id, &sensor_name[0]);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_SENSOR_RESOLUTION: {
		struct sensor_resolution *sensor_resolution = (struct sensor_resolution *)param_data;

		vin_get_sensor_resolution_special(vi_comp->vipp_id, (struct sensor_resolution *)sensor_resolution);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_TUNNING_CTL_DATA: {
		struct tunning_ctl_data *TunningCtlData = (struct tunning_ctl_data *)param_data;

		vin_tunning_ctrl_special(vi_comp->vipp_id, (void *)TunningCtlData);
		break;
	}
	case COMP_INDEX_VI_CONFIG_GET_BASE_CONFIG: {
		vi_comp_base_config *base_config = (vi_comp_base_config *)param_data;
		memcpy(base_config, &vi_comp->base_config, sizeof(vi_comp_base_config));
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_BKBUF_INFO: {
		vi_bkbuf_info *bkbuf_info = (vi_bkbuf_info *)param_data;
		if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
			bkbuf_info->buf_size = vi_comp->buf_size + LBC_EXT_SIZE;
		} else {
			bkbuf_info->buf_size = vi_comp->buf_size;
		}
		bkbuf_info->buf_num = vi_comp->buf_num;
		break;
	}
	case COMP_INDEX_VI_CONFIG_GET_SENSOR_RESERVE_ADDR: {
		SENSOR_ISP_CONFIG_S *sensor_isp_cfg = (SENSOR_ISP_CONFIG_S *)param_data;
		//vin_get_sensor_reserve_addr(vi_comp->vipp_id, sensor_isp_cfg);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_LV: {
		int lv = 0;

		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when get lv: %d", vi_comp->state);
			break;
		}

		//lv = vin_isp_get_lv_special(vi_comp->vipp_id, 0);
		RT_LOGI("get lv = %d\n", lv);

		return lv;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_EXP_GAIN: {
		struct sensor_exp_gain *exp_gain = (struct sensor_exp_gain *)param_data;

		vin_isp_get_exp_gain_special(vi_comp->vipp_id, (struct sensor_exp_gain *)exp_gain);

		RT_LOGI("get exp&gain = %d/%d/%d/%d", exp_gain->exp_val, exp_gain->gain_val,
			exp_gain->r_gain, exp_gain->b_gain);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_HIST: {
		unsigned int *hist = (unsigned int *)param_data;

		vin_isp_get_hist_special(vi_comp->vipp_id, hist);

		RT_LOGI("get hist");
		break;
	}
	case COMP_INDEX_VI_CONFIG_GET_ISP_REG_CONFIG: {
		VIN_ISP_REG_GET_CFG *isp_reg_get_cfg_ptr = (VIN_ISP_REG_GET_CFG *)param_data;
		//vin_get_isp_reg(vi_comp->vipp_id, isp_reg_get_cfg_ptr->flag, isp_reg_get_cfg_ptr->path);
		break;
	}
	default: {
		error = config_dynamic_param(vi_comp, index, param_data);
		break;
	}
	}

	return error;
}

error_type vi_comp_set_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_IN void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_set_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	switch (index) {
	case COMP_INDEX_VI_CONFIG_Base: {
		vi_comp_base_config *base_config = (vi_comp_base_config *)param_data;
		memcpy(&vi_comp->base_config, base_config, sizeof(vi_comp_base_config));
		vi_comp->align_width  = RT_ALIGN(vi_comp->base_config.width, 16);
		vi_comp->align_height = RT_ALIGN(vi_comp->base_config.height, 16);
		vi_comp->max_fps      = base_config->frame_rate;

		if (vi_comp->max_fps <= 0)
			vi_comp->max_fps = 15;

#ifdef CONFIG_FRAMEDONE_TWO_BUFFER
		if ((0 == vi_comp->base_config.bonline_channel) && (0 == vi_comp->base_config.enable_high_fps_transfor)) {
			vi_comp->base_config.vin_buf_num = 2;
			RT_LOGW("channel %d update vin_buf_num %d", vi_comp->base_config.channel_id, vi_comp->base_config.vin_buf_num);
		}
#endif
		break;
	}
	case COMP_INDEX_VI_CONFIG_Normal: {
		vi_comp_normal_config *normal_config = (vi_comp_normal_config *)param_data;
		memcpy(&vi_comp->normal_config, normal_config, sizeof(vi_comp_normal_config));
		break;
	}
	case COMP_INDEX_VI_CONFIG_SET_YUV_BUF_INFO: {
		memcpy(&vi_comp->yuv_buf_info, (config_yuv_buf_info *)param_data, sizeof(config_yuv_buf_info));
		break;
	}

	case COMP_INDEX_VI_CONFIG_Dynamic_REQUEST_YUV_FRAME: {
		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when request yuv frame: %d", vi_comp->state);
			error = ERROR_TYPE_STATE_ERROR;
			break;
		}
		if (vi_comp->base_config.bonline_channel) {
			RT_LOGE("fatal error! vipp[%d] online, can't get yuvFrame!", vi_comp->base_config.channel_id);
			error = ERROR_TYPE_ERROR;
			break;
		}
//		int ret = post_msg_and_wait(vi_comp, COMP_COMMAND_PAUSE, 0);
//		if (ret != 0) {
//			RT_LOGE("fatal error! rt_vi pause fail:%d", ret);
//		}
		rt_yuv_info *p_yuv_info = (rt_yuv_info *)param_data;

		error = request_yuv_frame(vi_comp, p_yuv_info);
		RT_LOGI("request yuv frame, ret = %d, fdset = %d, fd = %d phy_addr %p\n", error, p_yuv_info->bset, p_yuv_info->fd,
			p_yuv_info->phy_addr);

//		ret = post_msg_and_wait(vi_comp, COMP_COMMAND_START, 0);
//		if (ret != 0) {
//			RT_LOGE("fatal error! rt_vi start fail:%d", ret);
//		}

		if (error < 0)
			error = ERROR_TYPE_ERROR;

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_RETURN_YUV_FRAME: {
		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when request yuv frame: %d", vi_comp->state);
			break;
		}

		error = return_yuv_frame(vi_comp, (unsigned char *)param_data);
		RT_LOGI("return yuv frame, ret = %d, phy_addr = %px\n", error, param_data);

		if (error < 0)
			error = ERROR_TYPE_ERROR;

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_IR_PARAM: {
		RTIrParam *ir_param = (RTIrParam *)param_data;
		int mode_flag       = 0;

		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when get lv: %d", vi_comp->state);
			break;
		}

		/* ir_mode of user -- 0 :color, 1: grey */
		if (ir_param->grey == 1)
			mode_flag = 2;
		else
			mode_flag = 0;

		RT_LOGI("set ir param: mode_flag = %d, ir_on = %d, ir_flash_on = %d",
			mode_flag, ir_param->ir_on, ir_param->ir_flash_on);

		/* ir_mode of isp --  2: grey, other: color */
		vin_server_reset_special(vi_comp->vipp_id, mode_flag, ir_param->ir_on, ir_param->ir_flash_on);

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_H_FLIP: {
		int bhflip	   = *((int *)param_data);
		unsigned int ctrl_id = V4L2_CID_HFLIP;
		int val		     = bhflip;

		RT_LOGI("set h flip: %d", bhflip);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, val);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_V_FLIP: {
		int bvflip	   = *((int *)param_data);
		unsigned int ctrl_id = V4L2_CID_VFLIP;
		int val		     = bvflip;

		RT_LOGI("set v flip: %d", bvflip);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, val);
		break;
	}
//	case COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG: {
//		vi_comp->catch_jpeg_flag = *((int *)param_data);
//		RT_LOGI("vi_comp->catch_jpeg_flag = %d", vi_comp->catch_jpeg_flag);
//		break;
//	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_POWER_LINE_FREQ: {
		int tmpval = *((int *)param_data);
		enum RT_POWER_LINE_FREQUENCY ePower_line = (enum RT_POWER_LINE_FREQUENCY)tmpval;
		unsigned int ctrl_id			 = V4L2_CID_POWER_LINE_FREQUENCY;

		RT_LOGI("ePower_line_freq = %d", ePower_line);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, ePower_line);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_BRIGHTNESS: {
		int brightness_level = (int)param_data;
		unsigned int ctrl_id			 = V4L2_CID_BRIGHTNESS;

		RT_LOGI("brightness_level = %d", brightness_level);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, brightness_level);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_CONTRAST: {
		int contrast_level = (int)param_data;
		unsigned int ctrl_id			 = V4L2_CID_CONTRAST;

		RT_LOGI("contrast_level = %d", contrast_level);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, contrast_level);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_SATURATION: {
		int saturation_level = (int)param_data;
		unsigned int ctrl_id			 = V4L2_CID_SATURATION;

		RT_LOGI("saturation_level = %d", saturation_level);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, saturation_level);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_HUE: {
		int hue_level = (int)param_data;
		unsigned int ctrl_id			 = V4L2_CID_HUE;

		RT_LOGI("hue_level = %d", hue_level);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, hue_level);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_SHARPNESS: {
		int sharpness_level = (int)param_data;
		unsigned int ctrl_id			 = V4L2_CID_SHARPNESS;

		RT_LOGI("sharpness_level = %d", sharpness_level);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, sharpness_level);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_METERING_MODE: {
		enum RT_AE_METERING_MODE ae_metering_mode =
				(enum RT_AE_METERING_MODE)(*((int *)param_data));
		unsigned int ctrl_id			  = V4L2_CID_EXPOSURE_METERING;

		RT_LOGI("ae_metering_mode = %d", ae_metering_mode);

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, ae_metering_mode);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_MODE: {
		int ae_mode = (int)param_data;
		unsigned int ctrl_id;
		unsigned int ctrl_value;

		ctrl_id = V4L2_CID_EXPOSURE_AUTO;
		ctrl_value = ae_mode;
		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, ctrl_value);
		ctrl_id = V4L2_CID_AUTOGAIN;
		if (!ae_mode) {
			/* auto ae */
			ctrl_value = 1;
		} else {
			/* manual ae */
			ctrl_value = 0;
		}
		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, ctrl_value);

		RT_LOGI("ae_mode = %d", ae_mode);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_EXP: {
		int exp_time = (int)param_data;
		unsigned int ctrl_id = V4L2_CID_EXPOSURE_ABSOLUTE;

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, exp_time);

		RT_LOGI("exp_time = %d", exp_time);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_GAIN: {
		int sensor_gain = (int)param_data;
		unsigned int ctrl_id = V4L2_CID_GAIN;

		vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, sensor_gain);

		RT_LOGI("sensor_gain = %d", sensor_gain);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_ORL: {
		int i;
		struct v4l2_format fmt;
		RTIspOrl *isp_orl = (RTIspOrl *)param_data;
		unsigned char orl_cnt = isp_orl->orl_cnt;
		struct v4l2_clip *clips = kmalloc(sizeof(struct v4l2_clip) * orl_cnt * 2, GFP_KERNEL);

		for (i = 0; i < isp_orl->orl_cnt; i++) {
			clips[i].c.height = isp_orl->orl_win[i].height;
			clips[i].c.width =  isp_orl->orl_win[i].width;
			clips[i].c.left =  isp_orl->orl_win[i].left;
			clips[i].c.top =  isp_orl->orl_win[i].top;

			clips[isp_orl->orl_cnt + i].c.top =  isp_orl->orl_win[i].rgb_orl;
		}
		clips[isp_orl->orl_cnt].c.width = isp_orl->orl_width;

		CLEAR(fmt);
		fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		fmt.fmt.win.clips = &clips[0];
		fmt.fmt.win.clipcount = isp_orl->orl_cnt;
		fmt.fmt.win.bitmap = NULL;
		fmt.fmt.win.field = V4L2_FIELD_NONE;
		vin_s_fmt_overlay_special(vi_comp->vipp_id, &fmt);

		vin_overlay_special(vi_comp->vipp_id,  isp_orl->on);

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG: {
		struct isp_cfg_attr_data *p_isp_attr_cfg = (struct isp_cfg_attr_data *)param_data;

		vin_set_isp_attr_cfg_special(vi_comp->vipp_id, (void *)p_isp_attr_cfg);

		RT_LOGD("set isp attr cfg");

		break;
	}
	case COMP_INDEX_VI_CONFIG_SET_RESET_HIGH_FPS: {
		if (vi_comp->state != COMP_STATE_PAUSE)
			RT_LOGE("the state is not pause when reset-high-fps");

		//reset_high_fps(vi_comp);

		break;
	}
	case COMP_INDEX_VI_CONFIG_ENABLE_HIGH_FPS: {
		vi_comp->base_config.enable_high_fps_transfor = 1;
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS: {
		int fps      = (int)param_data;

		if (fps < vi_comp->max_fps) {
			vi_comp->base_config.st_adapter_venc_fps.enable = 1;
			vi_comp->base_config.st_adapter_venc_fps.frame_cnt = -1;
			vi_comp->base_config.st_adapter_venc_fps.dst_fps = fps;
		} else if (fps > vi_comp->max_fps) {
			RT_LOGW("Notice fps[%d] > max_fps[%d]", fps, vi_comp->max_fps);
		} else {
			vi_comp->base_config.st_adapter_venc_fps.enable = 0;
		}

		RT_LOGW("set fps = %d, max_fps = %d, bEnable_reduce_fps = %d",
			fps, vi_comp->max_fps, vi_comp->base_config.st_adapter_venc_fps.enable);
		break;
	}
	case COMP_INDEX_VI_CONFIG_SET_TDM_DROP_FRAME: {
		unsigned int *drop_num = (unsigned int *)param_data;
		//error = vin_set_tdm_drop_frame_special(vi_comp->vipp_id, drop_num);
		break;
	}
	default: {
		error = config_dynamic_param(vi_comp, index, param_data);
		break;
	}
	}

	return error;
}

error_type vi_comp_get_state(
	PARAM_IN comp_handle component,
	PARAM_OUT comp_state_type *pState)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_get_state: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	*pState = vi_comp->state;
	return error;
}

/**
   valid_list --> empty_list

   @return
     ERROR_TYPE_ILLEGAL_PARAM
     ERROR_TYPE_UNEXIST
*/
error_type vi_comp_fill_this_out_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;
	video_frame_s *src_stream       = NULL;
	video_frame_node *frame_node    = NULL;
	struct vin_buffer *vin_buf      = NULL;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_fill_this_out_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	src_stream = (video_frame_s *)pBuffer->private;
	//find matching node
	int num = 0;
	video_frame_node *pEntry = NULL;
	mutex_lock(&vi_comp->out_buf_manager.mutex);
	list_for_each_entry (pEntry, &vi_comp->out_buf_manager.valid_frame_list, mList) {
		num++;
		if (pEntry->video_frame.private == src_stream->private) {
			if (pEntry->video_frame.id != src_stream->id) {
				RT_LOGE("fatal error! check video frame id:%d!=%d", pEntry->video_frame.id, src_stream->id);
			}
			frame_node = pEntry;
			break;
		}
	}
	mutex_unlock(&vi_comp->out_buf_manager.mutex);
	if (NULL == frame_node) {
		RT_LOGE("fatal error! not find matching node[%d-%p] in out_buf manager, valid frame num[%d]", src_stream->id,
			src_stream->private, num);
		return ERROR_TYPE_UNEXIST;
	}
	error = viComp_RefReduceVideoFrame(vi_comp, &frame_node->video_frame);
	if (error != ERROR_TYPE_OK) {
		RT_LOGE("fatal error! rt_vi_comp[%p] ref reduce frame id[%d] fail:%d", vi_comp, src_stream->id, error);
	}
	return error;
}

error_type vi_comp_set_callbacks(
	PARAM_IN comp_handle component,
	PARAM_IN comp_callback_type *callback,
	PARAM_IN void *callback_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	vi_comp->callback      = callback;
	vi_comp->callback_data = callback_data;
	return error;
}

/**
  output port allows to link to more tunnel_components.
*/
error_type vi_comp_tunnel_request(
	PARAM_IN comp_handle component,
	PARAM_IN comp_port_type port_type,
	PARAM_IN comp_handle tunnel_comp,
	PARAM_IN int connect_flag)
{
	error_type error		= ERROR_TYPE_OK;
	int i;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL || tunnel_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks, param error: %px, %px, %px",
			rt_component, vi_comp, tunnel_comp);
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto setup_tunnel_exit;
	}

	if (port_type == COMP_INPUT_PORT) {
		if (connect_flag == 1) {
			if (vi_comp->in_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("in port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				vi_comp->in_port_tunnel_info.valid_flag  = 1;
				vi_comp->in_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (vi_comp->in_port_tunnel_info.valid_flag == 1 && vi_comp->in_port_tunnel_info.tunnel_comp == tunnel_comp) {
				vi_comp->in_port_tunnel_info.valid_flag  = 0;
				vi_comp->in_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect in tunnel failed:  valid_flag = %d, tunnel_comp = %px, %px",
					vi_comp->in_port_tunnel_info.valid_flag,
					vi_comp->in_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	} else if (port_type == COMP_OUTPUT_PORT) {
		mutex_lock(&vi_comp->tunnelLock);
		if (connect_flag == 1) {
			for (i = 0; i < RT_MAX_VENC_COMP_NUM; i++) {
				if (0 == vi_comp->out_port_tunnel_infos[i].valid_flag) {
					break;
				}
			}
			if (i < RT_MAX_VENC_COMP_NUM) {
				vi_comp->out_port_tunnel_infos[i].valid_flag  = 1;
				vi_comp->out_port_tunnel_infos[i].tunnel_comp = tunnel_comp;
			} else {
				RT_LOGE("fatal error! no idle out tunnel");
				error = ERROR_TYPE_ERROR;
				//goto setup_tunnel_exit;
			}
		} else {
			for (i = 0; i < RT_MAX_VENC_COMP_NUM; i++) {
				if (vi_comp->out_port_tunnel_infos[i].valid_flag && vi_comp->out_port_tunnel_infos[i].tunnel_comp == tunnel_comp) {
					vi_comp->out_port_tunnel_infos[i].valid_flag = 0;
					vi_comp->out_port_tunnel_infos[i].tunnel_comp = NULL;
					break;
				}
			}
			if (RT_MAX_VENC_COMP_NUM == i) {
				RT_LOGE("fatal error! disconnect out tunnel, but no tunnel_comp match to [%p]", tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
		mutex_unlock(&vi_comp->tunnelLock);
	}

setup_tunnel_exit:

	return error;
}

error_type vi_comp_component_init(PARAM_IN comp_handle component, const rt_media_config_s *pmedia_config)
{
	int i				= 0;
	int ret				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = NULL;
	struct sched_param param = {.sched_priority = 1 };

	vi_comp = kmalloc(sizeof(struct vi_comp_ctx), GFP_KERNEL);
	if (vi_comp == NULL) {
		RT_LOGE("kmalloc for vi_comp failed");
		error = ERROR_TYPE_NOMEM;
		goto EXIT;
	}
	memset(vi_comp, 0, sizeof(struct vi_comp_ctx));

	ret = message_create(&vi_comp->msg_queue);
	if (ret < 0) {
		RT_LOGE("message create failed");
		error = ERROR_TYPE_ERROR;
		goto EXIT;
	}

	rt_component->component_private    = vi_comp;
	rt_component->init		   = vi_comp_init;
	rt_component->start		   = vi_comp_start;
	rt_component->pause		   = vi_comp_pause;
	rt_component->stop		   = vi_comp_stop;
	rt_component->destroy		   = vi_comp_destroy;
	rt_component->get_config	   = vi_comp_get_config;
	rt_component->set_config	   = vi_comp_set_config;
	rt_component->get_state		   = vi_comp_get_state;
	rt_component->fill_this_out_buffer = vi_comp_fill_this_out_buffer;
	rt_component->set_callbacks	= vi_comp_set_callbacks;
	rt_component->tunnel_request	 = vi_comp_tunnel_request;

	vi_comp->self_comp = rt_component;
	vi_comp->state     = COMP_STATE_IDLE;
	vi_comp->base_config.channel_id = pmedia_config->channelId;

	init_waitqueue_head(&vi_comp->wait_in_buf);

	/* init out buf manager*/
	mutex_init(&vi_comp->out_buf_manager.mutex);
	RT_LOGD("channel %d vin_buf_num %d", pmedia_config->channelId, pmedia_config->vin_buf_num);
	//vi_comp->out_buf_manager.empty_num = pmedia_config->vin_buf_num;
	INIT_LIST_HEAD(&vi_comp->out_buf_manager.empty_frame_list);
	INIT_LIST_HEAD(&vi_comp->out_buf_manager.valid_frame_list);
	for (i = 0; i < pmedia_config->vin_buf_num; i++) {
		video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		pNode->video_frame.id = i;
		list_add_tail(&pNode->mList, &vi_comp->out_buf_manager.empty_frame_list);
	}

	mutex_init(&vi_comp->tunnelLock);
	spin_lock_init(&vi_comp->vi_spin_lock);
	mutex_init(&vi_comp->UserFrameLock);
	init_waitqueue_head(&vi_comp->stUserFrameReadyWaitQueue);
	vi_comp->bUserFrameReadyCondition = false;
	INIT_LIST_HEAD(&vi_comp->idleUserFrameList);
	INIT_LIST_HEAD(&vi_comp->readyUserFrameList);
	INIT_LIST_HEAD(&vi_comp->usingUserFrameList);
	for (i = 0; i < pmedia_config->vin_buf_num; i++) {
		video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &vi_comp->idleUserFrameList);
	}

	vi_comp->vi_thread = kthread_create(thread_process_vi, vi_comp, "vi comp thread");
	sched_setscheduler(vi_comp->vi_thread, SCHED_FIFO, &param);
	wake_up_process(vi_comp->vi_thread);

EXIT:

	return error;
}

int thread_process_vi(void *param)
{
	struct vi_comp_ctx *vi_comp = (struct vi_comp_ctx *)param;
	message_t cmd_msg;
	unsigned long flags;

	RT_LOGD("thread_process_vi start !");
	while (1) {
		if (kthread_should_stop())
			break;

		if (get_message(&vi_comp->msg_queue, &cmd_msg) == 0) {
			/* todo */
			commond_process_vi(vi_comp, &cmd_msg);
			/* continue process message */
			continue;
		}

		if (vi_comp->state == COMP_STATE_EXECUTING) {
			/* get out buffer */
#if DEBUG_SHOW_ENCODE_TIME
			static int cnt_1 = 1;
			static int cnt_11 = 1;
			if (cnt_1 && vi_comp->base_config.channel_id == 0) {
			    cnt_1 = 0;
			    RT_LOGW("channel %d vin comp first time EXECUTING state, time: %lld",
					vi_comp->base_config.channel_id, get_cur_time());
			}
			if (cnt_11 && vi_comp->base_config.channel_id == 1) {
			    cnt_11 = 0;
			    RT_LOGW("channel %d vin comp first time EXECUTING state, time: %lld",
					vi_comp->base_config.channel_id, get_cur_time());
			}
#endif
			if (dequeue_buffer(vi_comp) != 0) {
				spin_lock_irqsave(&vi_comp->vi_spin_lock, flags);
				vi_comp->wait_in_buf_condition = 0;
				spin_unlock_irqrestore(&vi_comp->vi_spin_lock, flags);
				/* timeout is 10 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
				wait_event_timeout(vi_comp->wait_in_buf, vi_comp->wait_in_buf_condition, msecs_to_jiffies(100));
				continue;
			}
		} else if (vi_comp->state == COMP_STATE_EXIT) {
			RT_LOGW("COMP_STATE_EXIT vi_comp->state %d", vi_comp->state);
			break;
		} else {
			TMessage_WaitQueueNotEmpty(&vi_comp->msg_queue, 1000);
		}
	}

	RT_LOGD("thread_process_vi finish !");
	return 0;
}
