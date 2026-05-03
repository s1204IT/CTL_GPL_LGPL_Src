/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define LOG_TAG "rt_media"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/mm.h>

#include <net/net_namespace.h>
#include <net/netlink.h>
#include <net/genetlink.h>

#include <asm/uaccess.h>

#include "rt_media.h"
#include <uapi/rt-media/uapi_rt_media.h>

#include "component/rt_common.h"
#include "component/rt_component.h"
#include "component/rt_venc_component.h"
#include "component/rt_vi_component.h"
#include <linux/aw_rpmsg.h>
#include <uapi/linux/sched/types.h>
//#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#include <linux/jiffies.h>

#ifndef RT_MEDIA_DEV_MAJOR
#define RT_MEDIA_DEV_MAJOR (160)
#endif
#ifndef RT_MEDIA_DEV_MINOR
#define RT_MEDIA_DEV_MINOR (0)
#endif

#define MOTION_TOTAL_REGION_NUM_DEFAULT (50)
#define REGION_D3D_TOTAL_REGION_NUM_DEFAULT (120)
#define DEBUG_RT_MEDIA_VENC_PARAM 0

#define MAX_SENSOR_NUM 3

int g_rt_media_dev_major = RT_MEDIA_DEV_MAJOR;
int g_rt_media_dev_minor = RT_MEDIA_DEV_MINOR;
/*S_IRUGO represent that g_rt_media_dev_major can be read,but canot be write*/
module_param(g_rt_media_dev_major, int, 0444);
module_param(g_rt_media_dev_minor, int, 0444);

typedef struct stream_buffer_manager {
	struct list_head empty_stream_list; //video_stream_node
	struct list_head valid_stream_list;
	struct list_head caller_hold_stream_list;
	struct mutex mutex;
	int empty_num;

	wait_queue_head_t wait_bitstream;
	unsigned int wait_bitstream_condition;
	unsigned int need_wait_up_flag;
} stream_buffer_manager;

typedef struct tdm_data_node {
	struct vin_isp_tdm_event_status tdm_data;
	struct list_head mList;
} tdm_data_node;

typedef struct tdm_data_manager {
	int b_inited;
	struct list_head empty_list;
	struct list_head valid_list;
	spinlock_t s_lock;
	int empty_num;
} tdm_data_manager;

typedef enum rt_media_state {
	RT_MEDIA_STATE_IDLE     = 0,
	RT_MEDIA_STATE_CONFIGED = 1,
	RT_MEDIA_STATE_EXCUTING = 2,
	RT_MEDIA_STATE_PAUSE    = 3,
} rt_media_state;

typedef struct media_private_info {
	int channel;
} media_private_info;

typedef struct video_recoder {
	int activate_vi_flag;
	int activate_venc_flag;
	rt_component_type *venc_comp;
	rt_component_type *vi_comp;
	stream_buffer_manager stream_buf_manager;
	int bcreate_comp_flag;
	rt_media_state state;
	rt_media_config_s config;
	config_yuv_buf_info yuv_buf_info;
	rt_pixelformat_type pixelformat;
	unsigned char *enc_yuv_phy_addr; //for mode ENCODE_OUTSIDE_YUV
	struct mutex yuv_mutex; //for mode ENCODE_OUTSIDE_YUV
	int stream_data_cb_count; //for venc main stream, don't consider jpeg.
	int max_fps; //source vin fps.
	int enable_high_fps;
	int bReset_high_fps_finish;
	int bsetup_recorder_finish;
	int bvin_is_ready;//rt_media thread setup not ready fact.
	VencMotionSearchParam motion_search_param;
	VencMotionSearchRegion *motion_region;
	int motion_region_len; //unit:bytes
	//RTVencRegionD3DParam region_d3d_param;
	//RTVencRegionD3DParam *region_d3d_region;
	int region_d3d_region_len; //unit:bytes
	VencInsertData insert_data;
	int mNLDstPid; //netlink dst port ID

	rt_component_type *vencCompCatchJpeg; //for catch jpeg.
	stream_buffer_manager streamBufManagerCatchJpeg; //for catch jpeg
} video_recoder;

typedef struct encode_config {
	int b_get_from_dts;
	char *name;
	int ch_id;//vipp_num
	int bonline_channel; //vi param, venc param
	int share_buf_num; //vi param, venc param
	RT_VENC_CODEC_TYPE codec_type;
	int res_w;
	int res_h;
	int fps;
	int bit_rate;//kb
	int gop;
	int enable_sharp;
	int vbr;
	int vbr_opt_enable;
	eVencProductMode product_mode;
	VencQPRange qp_range;
	VencVbrParam vbr_param;
	VIDEO_OUTPUT_MODE out_mode;
	RT_PIXELFORMAT_TYPE pix_fmt;//* just set it to: RT_PIXELFORMAT_UNKNOWN
	int reduce_refrec_mem;//Can reduce memory usage and optimize memory efficiency. default set enable
	int enable_tdm_raw;
	unsigned int tdm_rxbuf_cnt;
} encode_config;
struct rt_media_dev {
	struct cdev cdev; /* char device struct				   */
	struct device *dev; /* ptr to class device struct		   */
	struct device *platform_dev; /* ptr to class device struct		   */
	struct class *class; /* class for auto create device node  */

	struct semaphore sem; /* mutual exclusion semaphore		   */

	wait_queue_head_t wq; /* wait queue for poll ops			   */

	struct mutex lock_vdec;

	video_recoder recoder[VIDEO_INPUT_CHANNEL_NUM]; //1: major, 2: minor, 3: yuv

	struct task_struct *setup_thread;

	struct task_struct *reset_high_fps_thread;

	int in_high_fps_status;
	video_recoder *need_start_recoder_1;
	video_recoder *need_start_recoder_2;
	int bcsi_err;
	int bcsi_status_set[VIDEO_INPUT_CHANNEL_NUM];
	tdm_data_manager st_tdm_data_manager[MAX_SENSOR_NUM];
	encode_config enc_conf[MAX_SENSOR_NUM];
};

struct vi_part_cfg {
	int wdr;
	int width;
	int height;
	int venc_format; //1:H264 2:H265
	int flicker_mode; ////0:disable,1:50HZ,2:60HZ, default 50HZ
	int fps;
};
struct csi_status_pair {
	int bset;
	int status;
};
static struct rt_media_dev *rt_media_devp;

static int bvin_is_ready_rt_not_probe[VIDEO_INPUT_CHANNEL_NUM];//Deal with the fact that rt_media not registered
static struct csi_status_pair g_csi_status_pair[VIDEO_INPUT_CHANNEL_NUM];

static video_stream_s *ioctl_get_stream_data(video_recoder *recoder, stream_buffer_manager *stream_buf_mgr);
static int ioctl_return_stream_data(video_recoder *recoder, stream_buffer_manager *stream_buf_mgr,
	video_stream_s *video_stream);
int ioctl_start(video_recoder *recoder);

/**
  read record channel id, find matching recorder, then store source port id to NLDstPid.

  Then rt_media recorder in kernel can send message to dst AWVideoInput_cxt.
*/
static int cmd_attr_chnid(struct genl_info *info)
{
	int rc = 0;
	int nRecordChannelId = nla_get_s32(info->attrs[RT_MEDIA_CMD_ATTR_CHNID]);
	if (nRecordChannelId >= 0 && nRecordChannelId < VIDEO_INPUT_CHANNEL_NUM) {
		video_recoder *pRecorder = &rt_media_devp->recoder[nRecordChannelId];
		if (0 == pRecorder->mNLDstPid) {
			pRecorder->mNLDstPid = info->snd_portid;
		} else {
			if (pRecorder->mNLDstPid != info->snd_portid) {
				RT_LOGW("Be careful! recorder[%d] netlink dst port id change [%d->%d]", nRecordChannelId,
					pRecorder->mNLDstPid, info->snd_portid);
				pRecorder->mNLDstPid = info->snd_portid;
			}
		}
		rc = 0;
		RT_LOGD("recorder[%d] netlink dst port id=%d", nRecordChannelId, pRecorder->mNLDstPid);
	} else {
		RT_LOGE("fatal error! recorder[%d] wrong!", nRecordChannelId);
		rc = -EINVAL;
	}
	return rc;
}

static int rt_media_user_cmd(struct sk_buff *skb, struct genl_info *info)
{
	if (info->attrs[RT_MEDIA_CMD_ATTR_CHNID]) {
		return cmd_attr_chnid(info);
	} else {
		RT_LOGW("Be careful! user space should send attr chnid, we only process attr chnid now.");
		return -EINVAL;
	}
}

static int rt_media_genl_family_registered;

static const struct nla_policy rt_media_cmd_get_policy[RT_MEDIA_CMD_ATTR_MAX+1] = {
	[RT_MEDIA_CMD_ATTR_CHNID]  = { .type = NLA_S32 },
	[RT_MEDIA_CMD_ATTR_RTCBEVENT] = { .type = NLA_BINARY, .len = sizeof(RTCbEvent) }
};

static const struct genl_ops rt_media_genl_ops[] = {
	{
		.cmd		= RT_MEDIA_CMD_SEND,
		.doit		= rt_media_user_cmd,
		//.policy		= rt_media_cmd_get_policy,
	}
};

static struct genl_family rt_media_genl_family = {
	//.id		= 0,//GENL_ID_GENERATE, todo not this macro
	.name		= RT_MEDIA_GENL_NAME,
	.version	= RT_MEDIA_GENL_VERSION,
	.maxattr	= RT_MEDIA_CMD_ATTR_MAX,
	.policy		= rt_media_cmd_get_policy,
	.ops		= rt_media_genl_ops,
	.n_ops		= ARRAY_SIZE(rt_media_genl_ops),
};

static int __init rt_media_genetlink_init(void)
{
	int ret = genl_register_family(&rt_media_genl_family);
	if (ret) {
		RT_LOGE("fatal error! genl register rt_media family fail:%d", ret);
		return ret;
	}
	rt_media_genl_family_registered = 1;
	RT_LOGD("registered rt_media genl family version:%d", rt_media_genl_family.version);
	return 0;
}

static int __exit rt_media_genetlink_exit(void)
{
	int ret = 0;
	if (rt_media_genl_family_registered) {
		ret = genl_unregister_family(&rt_media_genl_family);
		if (ret) {
			RT_LOGE("fatal error! genl unregister rt_media family fail:%d", ret);
		}
		rt_media_genl_family_registered = 0;
	}
	return ret;
}

/**
  send netlink message to user process AWChannel_cxt of AWVideoInput_cxt.

  @param pid
    dst port id in user space.

  @return
    ERROR_TYPE_NOMEM
    ERROR_TYPE_ERROR
    ERROR_TYPE_OK
*/
static error_type rt_media_send_genl_RTCbEvent(int pid, RTCbEvent *pEvent)
{
	int size = nla_total_size(sizeof(RTCbEvent));
	struct sk_buff *skb = genlmsg_new(size, GFP_KERNEL);
	if (!skb) {
		RT_LOGE("fatal error! malloc fail!");
		return ERROR_TYPE_NOMEM;
	}
	void *user_hdr = genlmsg_put(skb, 0, 0, &rt_media_genl_family, 0, RT_MEDIA_CMD_CB_EVENT);
	if (user_hdr == NULL) {
		RT_LOGE("fatal error! check code!");
		nlmsg_free(skb);
		return ERROR_TYPE_NOMEM;
	}
	int rc = nla_put(skb, RT_MEDIA_CMD_ATTR_RTCBEVENT, sizeof(RTCbEvent), pEvent);
	if (rc != 0) {
		RT_LOGE("fatal error! check code[%d]!", rc);
		nlmsg_free(skb);
		return ERROR_TYPE_NOMEM;
	}
	genlmsg_end(skb, user_hdr);
	rc = genlmsg_unicast(&init_net, skb, pid);
	if (rc < 0) {
		RT_LOGE("fatal error! check code[%d]!", rc);
		nlmsg_free(skb);
		return ERROR_TYPE_NOMEM;
	}
	return ERROR_TYPE_OK;
}

error_type venc_event_handler(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_event_type eEvent,
	PARAM_IN unsigned int nData1,
	PARAM_IN unsigned int nData2,
	PARAM_IN void *pEventData)
{
	error_type ret = ERROR_TYPE_ERROR;
	video_recoder *pRecorder = (video_recoder *)pAppData;
	switch (eEvent) {
		case COMP_EVENT_CMD_COMPLETE: {
			break;
		}
		case COMP_EVENT_CMD_ERROR: {
			break;
		}
		case COMP_EVENT_VENC_DROP_FRAME: {
			//notify user space AWChannel_cxt using netlink socket
			ret = ERROR_TYPE_OK;
			if (pRecorder->mNLDstPid != 0) {
				RTCbEvent stDropFrameEvent;
				memset(&stDropFrameEvent, 0, sizeof(RTCbEvent));
				stDropFrameEvent.eEventType = RTCB_EVENT_VENC_DROP_FRAME;
				stDropFrameEvent.nData1 = 1; //drop one frame.
				ret = rt_media_send_genl_RTCbEvent(pRecorder->mNLDstPid, &stDropFrameEvent);
				if (ret != ERROR_TYPE_OK) {
					RT_LOGE("fatal error! recorder[%d] send nl msg to user space portid[%d] fail[%d]",
						pRecorder->config.channelId, pRecorder->mNLDstPid, ret);
				}
			}
			break;
		}
		default: {
			RT_LOGI("not support venc_event_handler:0x%x", eEvent);
			break;
		}
	}
	return ret;
}

error_type rt_venc_empty_in_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	video_recoder *recoder      = (video_recoder *)pAppData;
	video_frame_s *pvideo_frame = (video_frame_s *)pBuffer->private;

	LOCK_MUTEX(&recoder->yuv_mutex);
	if (recoder->enc_yuv_phy_addr != NULL)
		RT_LOGE("the enc_yuv_phy_addr[%px] is not null", recoder->enc_yuv_phy_addr);
	recoder->enc_yuv_phy_addr = (unsigned char *)pvideo_frame->phy_addr[0];
	UNLOCK_MUTEX(&recoder->yuv_mutex);
	RT_LOGI("set recoder->enc_yuv_phy_addr: %px", recoder->enc_yuv_phy_addr);
	return 0;
}

/* empty_list --> valid_list*/
error_type rt_venc_fill_out_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	video_stream_node *stream_node	= NULL;
	video_recoder *recoder		      = (video_recoder *)pAppData;
	video_stream_s *video_stream	  = (video_stream_s *)pBuffer->private;
	stream_buffer_manager *stream_buf_mgr = NULL;

	if (component == recoder->venc_comp) {
		stream_buf_mgr = &recoder->stream_buf_manager;
	} else if (component == recoder->vencCompCatchJpeg) {
		RT_LOGD("rt_media_chn[%d] comp[%p] catch jpeg fill buffer done", recoder->config.channelId, component);
		stream_buf_mgr = &recoder->streamBufManagerCatchJpeg;
	} else {
		RT_LOGE("fatal error! rt_media_chn[%d] unknown comp handle:%p", recoder->config.channelId, component);
	}

	if (component == recoder->venc_comp) {
		recoder->stream_data_cb_count++;
		if (recoder->stream_data_cb_count == 1) {
			RT_LOGW("channel %d first stream data, pts = %lld, time = %lld, width = %d", recoder->config.channelId,
				video_stream->pts, get_cur_time(), recoder->config.width);
		}
	}

	LOCK_MUTEX(&stream_buf_mgr->mutex);

	if (list_empty(&stream_buf_mgr->empty_stream_list)) {
		RT_LOGE("fatal error! channel %d rt_venc_fill_out_buffer_done error: empty list is null.[%p-%p-%p]",
			recoder->config.channelId, component, recoder->venc_comp, recoder->vencCompCatchJpeg);
		UNLOCK_MUTEX(&stream_buf_mgr->mutex);
		return ERROR_TYPE_ERROR;
	}
	stream_node = list_first_entry(&stream_buf_mgr->empty_stream_list, video_stream_node, mList);
	memcpy(&stream_node->video_stream, video_stream, sizeof(video_stream_s));

	stream_buf_mgr->empty_num--;
	list_move_tail(&stream_node->mList, &stream_buf_mgr->valid_stream_list);

	if (stream_buf_mgr->need_wait_up_flag == 1) {
		stream_buf_mgr->need_wait_up_flag = 0;
		stream_buf_mgr->wait_bitstream_condition = 1;
		wake_up(&stream_buf_mgr->wait_bitstream);
	}
	UNLOCK_MUTEX(&stream_buf_mgr->mutex);

	/* todo; */
	return 0;
}

comp_callback_type venc_callback = {
	.EventHandler	 = venc_event_handler,
	.empty_in_buffer_done = rt_venc_empty_in_buffer_done,
	.fill_out_buffer_done = rt_venc_fill_out_buffer_done,
};

static int thread_reset_high_fps(void *param)
{
	int ret		       = 0;
	int channel_id	 = 0;
	video_recoder *recoder = &rt_media_devp->recoder[channel_id];

	RT_LOGW("start");
	/* set config: reset high fps */
	ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_RESET_HIGH_FPS, NULL);
	if (ret < 0) {
		RT_LOGE("IOCTL SET_RESET_HIGH_FPS failed, ret = %d", ret);
		return -EFAULT;
	}

	ioctl_start(recoder);

	recoder->bReset_high_fps_finish = 1;

	RT_LOGW("rt_media_devp->need_start_recoder = %px, %px",
		rt_media_devp->need_start_recoder_1, rt_media_devp->need_start_recoder_2);

	if (rt_media_devp->need_start_recoder_1) {
		ioctl_start(rt_media_devp->need_start_recoder_1);
		rt_media_devp->need_start_recoder_1 = NULL;
	}

	if (rt_media_devp->need_start_recoder_2) {
		ioctl_start(rt_media_devp->need_start_recoder_2);
		rt_media_devp->need_start_recoder_2 = NULL;
	}

	rt_media_devp->in_high_fps_status = 0;
	RT_LOGW("finish");
	return 0;
}

static int wdr_get_from_partition(struct vi_part_cfg *vi_part_cfg, int sensor_num)
{
#if defined CONFIG_ISP_SERVER_MELIS
	int ret     = 0;
	void *vaddr = NULL;
	int check_sign = -1;
	SENSOR_ISP_CONFIG_S *sensor_isp_cfg;

#ifdef CONFIG_SUPPORT_THREE_CAMERA_MELIS
	if (sensor_num == 1) {
		check_sign = SENSOR_1_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR1_RESERVE_ADDR, VIN_RESERVE_SIZE);
	} else if (sensor_num == 0) {
		check_sign = SENSOR_0_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR0_RESERVE_ADDR, VIN_RESERVE_SIZE);
	} else if (sensor_num == 2) {
		check_sign = SENSOR_2_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR2_RESERVE_ADDR, VIN_RESERVE_SIZE);
	}
#else
	if (sensor_num == 1) {
		check_sign = SENSOR_1_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR1_RESERVE_ADDR, VIN_RESERVE_SIZE);
	} else {
		check_sign = SENSOR_0_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR0_RESERVE_ADDR, VIN_RESERVE_SIZE);
	}
#endif

	if (vaddr == NULL) {
		RT_LOGE("%s:map 0x%x paddr err!!!", __func__, VIN_SENSOR0_RESERVE_ADDR);
		ret = -EFAULT;
		goto ekzalloc;
	}

	sensor_isp_cfg = (SENSOR_ISP_CONFIG_S *)vaddr;

	/* check id */
	if (sensor_isp_cfg->sign != check_sign) {
		RT_LOGW("%s:sign is 0x%x but not 0x%x\n", __func__, sensor_isp_cfg->sign, check_sign);
		ret = -EINVAL;
		goto unmap;
	}

	vi_part_cfg->wdr = sensor_isp_cfg->wdr_mode > 1 ? 0 : sensor_isp_cfg->wdr_mode;
	vi_part_cfg->width = sensor_isp_cfg->width;
	vi_part_cfg->height = sensor_isp_cfg->height;
	vi_part_cfg->venc_format = sensor_isp_cfg->venc_format > 2 ? 1 : (sensor_isp_cfg->venc_format == 0 ? 1 : sensor_isp_cfg->venc_format);
	vi_part_cfg->flicker_mode = sensor_isp_cfg->flicker_mode > 2 ? 1 : sensor_isp_cfg->flicker_mode;

	vi_part_cfg->fps = sensor_isp_cfg->fps <= 0 ? 15 : sensor_isp_cfg->fps;

	RT_LOGS("wdr/width/height/vformat/flicker get from partiton is %d/%d/%d/%d %d %d %d\n",
		sensor_isp_cfg->wdr_mode, sensor_isp_cfg->width,
		sensor_isp_cfg->height, sensor_isp_cfg->venc_format,
		vi_part_cfg->flicker_mode, vi_part_cfg->fps, sensor_isp_cfg->fps);

//sensor_isp_cfg->sign = 0xFFFFFFFF;

unmap:
	vin_unmap_kernel(vaddr);
ekzalloc:
	return ret;
#else
	return 0;
#endif
}

error_type vi_event_handler(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_event_type eEvent,
	PARAM_IN unsigned int nData1,
	PARAM_IN unsigned int nData2,
	PARAM_IN void *pEventData)
{
	/*video_recoder *recoder = (video_recoder *)pAppData;*/
	RT_LOGI("vi_event_handler, eEvent = %d", eEvent);

	if (eEvent == COMP_EVENT_CMD_RESET_ISP_HIGH_FPS) {
		struct sched_param param = {.sched_priority = 1 };
		rt_media_devp->reset_high_fps_thread = kthread_create(thread_reset_high_fps,
								      rt_media_devp, "reset-high-fps");
		sched_setscheduler(rt_media_devp->reset_high_fps_thread, SCHED_FIFO, &param);
		wake_up_process(rt_media_devp->reset_high_fps_thread);
	}

	return 0;
}

error_type rt_vi_empty_in_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	/* todo; */
	RT_LOGE("not support rt_vi_empty_in_buffer_done");
	return -1;
}

error_type rt_vi_fill_out_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	/* todo; */
	RT_LOGE("not support rt_vi_fill_out_buffer_done");
	return -1;
}

comp_callback_type vi_callback = {
	.EventHandler	 = vi_event_handler,
	.empty_in_buffer_done = rt_vi_empty_in_buffer_done,
	.fill_out_buffer_done = rt_vi_fill_out_buffer_done,
};

static int get_component_handle(video_recoder *recoder)
{
	int ret = 0;

	RT_LOGD("recoder->activate_vi_flag: %d", recoder->activate_vi_flag);
	if (recoder->activate_vi_flag == 1) {
		ret = comp_get_handle((comp_handle *)&recoder->vi_comp,
					  "video.vipp",
					  recoder,
					  &recoder->config,
					  &vi_callback);
		if (ret != ERROR_TYPE_OK) {
			RT_LOGE("get vi comp handle error: %d", ret);
			return -1;
		}
	}
	RT_LOGD("recoder->activate_venc_flag: %d", recoder->activate_venc_flag);
	if (recoder->activate_venc_flag == 1) {
		ret = comp_get_handle((comp_handle *)&recoder->venc_comp,
					  "video.encoder",
					  recoder,
					  &recoder->config,
					  &venc_callback);
		if (ret != ERROR_TYPE_OK) {
			RT_LOGE("get venc comp handle error: %d", ret);
			return -1;
		}
	}
	recoder->bcreate_comp_flag = 1;

	return ret;
}

int fps_change_id_1;
int fps_change_id_2;

static void set_fps_change_channel_id_1(int id)
{
	fps_change_id_1 = id;
}

int get_fps_change_channel_id_1(void)
{
	return fps_change_id_1;
}

void rt_media_csi_fps_change_first(int change_fps)
{
	int ret = 0;
	int channel_id = get_fps_change_channel_id_1();
	video_recoder *recoder = &rt_media_devp->recoder[channel_id];

	if (change_fps <= 0 || change_fps > recoder->max_fps) {
		RT_LOGW("IOCTL SET_FPS: invalid change_fps[%d], setup to max_fps[%d]", change_fps, recoder->max_fps);
		change_fps = recoder->max_fps;
	}

	RT_LOGI("IOCTL SET_FPS: change_fps = %d, max_fps = %d, vi_comp = %px, venc_comp = %px",
		change_fps, recoder->max_fps, recoder->vi_comp, recoder->venc_comp);

	if (!recoder->vi_comp)
		return;
	ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS, (void *)change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}

	if (!recoder->venc_comp)
		return;
	ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS, (void *)change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}
}
/* to do
static void set_fps_change_channel_id_2(int id)
{
	fps_change_id_2 = id;
}*/

int get_fps_change_channel_id_2(void)
{
	return fps_change_id_2;
}
void rt_media_csi_fps_change_second(int change_fps)
{
	int ret = 0;
	int channel_id = get_fps_change_channel_id_2();
	video_recoder *recoder	 = &rt_media_devp->recoder[channel_id];

	if (change_fps <= 0 || change_fps > recoder->max_fps) {
		RT_LOGW("IOCTL SET_FPS: invalid change_fps[%d], setup to max_fps[%d]", change_fps, recoder->max_fps);
		change_fps = recoder->max_fps;
	}

	RT_LOGI("IOCTL SET_FPS: change_fps = %d, max_fps = %d, vi_comp = %px, venc_comp = %px",
		change_fps, recoder->max_fps, recoder->vi_comp, recoder->venc_comp);

	ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS, (void *)change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}

	ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS, (void *)change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}
}

static void rt_vin_sensor_fps_change_callback(int channel_id)
{
	set_fps_change_channel_id_1(channel_id);
	vin_sensor_fps_change_callback(channel_id, rt_media_csi_fps_change_first);
}

static void rt_tdm_buffer_done_callback(struct vin_isp_tdm_event_status *status)
{
	tdm_data_manager *p_tdm_data_manager = NULL;
	tdm_data_node *p_tdm_note = NULL;
	unsigned long flags;

	//RT_LOGW("tdm%d iommu_buf %px buf_size %d buf_id %d", status->dev_id, status->iommu_buf, status->buf_size, status->buf_id);

	if (status->dev_id < MAX_SENSOR_NUM)
		p_tdm_data_manager = &rt_media_devp->st_tdm_data_manager[status->dev_id];
	else {
		RT_LOGE("dev_id not support %d", status->dev_id);
		return ;
	}

	if (0 == p_tdm_data_manager->b_inited) {
		RT_LOGW("tdm%d data manager is not inited!", status->dev_id);
		return ;
	}

	spin_lock_irqsave(&p_tdm_data_manager->s_lock, flags);

	if (list_empty(&p_tdm_data_manager->empty_list)) {
		RT_LOGW("empty list is null, tdm%d drop data!", status->dev_id);
		spin_unlock_irqrestore(&p_tdm_data_manager->s_lock, flags);
		return ;
	}
	p_tdm_note = list_first_entry(&p_tdm_data_manager->empty_list, tdm_data_node, mList);
	memcpy(&p_tdm_note->tdm_data, status, sizeof(struct vin_isp_tdm_event_status));

	p_tdm_data_manager->empty_num--;
	list_move_tail(&p_tdm_note->mList, &p_tdm_data_manager->valid_list);

	spin_unlock_irqrestore(&p_tdm_data_manager->s_lock, flags);

	return ;
}

static void rt_tdm_get_buf(struct vin_isp_tdm_event_status *status)
{
	tdm_data_node *p_tdm_note	= NULL;
	tdm_data_manager *p_tdm_data_manager = NULL;

	if (status->dev_id < MAX_SENSOR_NUM)
		p_tdm_data_manager = &rt_media_devp->st_tdm_data_manager[status->dev_id];
	else {
		RT_LOGE("dev_id not support %d", status->dev_id);
		return ;
	}

	if (0 == p_tdm_data_manager->b_inited) {
		RT_LOGW("tdm%d data manager is not inited!", status->dev_id);
		return ;
	}

	spin_lock(&p_tdm_data_manager->s_lock);

	if (list_empty(&p_tdm_data_manager->valid_list)) {
		RT_LOGD("tdm valid list is null");
		spin_unlock(&p_tdm_data_manager->s_lock);
		return ;
	}
	p_tdm_note = list_first_entry(&p_tdm_data_manager->valid_list, tdm_data_node, mList);

	list_move_tail(&p_tdm_note->mList, &p_tdm_data_manager->empty_list);
	p_tdm_data_manager->empty_num++;

	spin_unlock(&p_tdm_data_manager->s_lock);
	memcpy(status, &p_tdm_note->tdm_data, sizeof(struct vin_isp_tdm_event_status));

	//RT_LOGW("tdm%d get buf[%d] %px %d", status->dev_id, status->buf_id, status->iommu_buf, status->buf_size);

	return ;
}

static void fill_venc_config(venc_comp_base_config *pvenc_config, video_recoder *recoder)
{
	rt_media_config_s *media_config = &recoder->config;

	/* setup venc config */
	pvenc_config->codec_type = media_config->encodeType;

	pvenc_config->qp_range		    = media_config->qp_range;
	pvenc_config->profile		    = media_config->profile;
	pvenc_config->level		    = media_config->level;
	pvenc_config->src_width		    = media_config->width;
	if (media_config->en_16_align_fill_data) {
		pvenc_config->src_height = RT_ALIGN(media_config->height, 16);
	} else {
		pvenc_config->src_height = media_config->height;
	}
	pvenc_config->dst_width		    = media_config->dst_width;
	pvenc_config->dst_height	    = media_config->dst_height;
	pvenc_config->frame_rate	    = media_config->dst_fps;
	pvenc_config->product_mode      = media_config->product_mode;
	pvenc_config->bit_rate		    = media_config->bitrate;
	pvenc_config->vbv_thresh_size   = media_config->vbv_thresh_size;
	pvenc_config->vbv_buf_size      = media_config->vbv_buf_size;
	pvenc_config->pixelformat	    = recoder->pixelformat;
    pvenc_config->outputformat      = media_config->outputformat;
	pvenc_config->max_keyframe_interval = media_config->gop;
	pvenc_config->enable_overlay	= media_config->enable_overlay;
	pvenc_config->channel_id = media_config->channelId;
	pvenc_config->quality = media_config->jpg_quality;
	pvenc_config->jpg_mode = media_config->jpg_mode;
	pvenc_config->bit_rate_range.bitRateMax = media_config->bit_rate_range.bitRateMax;
	pvenc_config->bit_rate_range.bitRateMin = media_config->bit_rate_range.bitRateMin;
	pvenc_config->bit_rate_range.fRangeRatioTh = media_config->bit_rate_range.fRangeRatioTh;
	pvenc_config->bit_rate_range.nQualityTh = media_config->bit_rate_range.nQualityTh;
	pvenc_config->bit_rate_range.nQualityStep[0] = media_config->bit_rate_range.nQualityStep[0];
	pvenc_config->bit_rate_range.nQualityStep[1] = media_config->bit_rate_range.nQualityStep[1];
	pvenc_config->bit_rate_range.nSubQualityDelay = media_config->bit_rate_range.nSubQualityDelay;
	pvenc_config->bit_rate_range.nAddQualityDelay = media_config->bit_rate_range.nAddQualityDelay;
	pvenc_config->bit_rate_range.nMinQuality = media_config->bit_rate_range.nMinQuality;
	pvenc_config->bit_rate_range.nMaxQuality = media_config->bit_rate_range.nMaxQuality;
	pvenc_config->bOnlineChannel = media_config->bonline_channel;
	pvenc_config->share_buf_num = media_config->share_buf_num;
	pvenc_config->en_encpp_sharp = media_config->enable_sharp;
	pvenc_config->breduce_refrecmem = media_config->breduce_refrecmem;
	pvenc_config->mRcMode = media_config->mRcMode;

	memcpy(&pvenc_config->venc_video_signal, &media_config->venc_video_signal, sizeof(VencH264VideoSignal));

	memcpy(&pvenc_config->mb_mode_ctl, &media_config->mb_mode_ctl, sizeof(VencMBModeCtrl));
	memcpy(&pvenc_config->mMapLow, &media_config->mMapLow, sizeof(VencMbModeLambdaQpMap));
	memcpy(&pvenc_config->mMapHigh, &media_config->mMapHigh, sizeof(VencMbSplitMap));

	if (pvenc_config->mRcMode == AW_VBR)
		memcpy(&pvenc_config->vbr_param, &media_config->vbr_param, sizeof(VencVbrParam));

	if (rt_is_format422(pvenc_config->pixelformat)) {
		pvenc_config->en_encpp_sharp = 0;
	}
	pvenc_config->enable_isp2ve_linkage = media_config->enable_isp2ve_linkage;
	pvenc_config->enable_ve2isp_linkage = media_config->enable_ve2isp_linkage;
	pvenc_config->skip_sharp_param_frame_num = media_config->skip_sharp_param_frame_num;
//	if ((media_config->ve_encpp_sharp_atten_coef_per < 0) || (media_config->ve_encpp_sharp_atten_coef_per > 100)) {
//		RT_LOGE("invalid encpp_sharp_atten_coef_per %d use dafault 0!",
//			media_config->ve_encpp_sharp_atten_coef_per);
//		pvenc_config->encpp_sharp_atten_coef_per = 0;
//	} else
//		pvenc_config->encpp_sharp_atten_coef_per = media_config->ve_encpp_sharp_atten_coef_per;
}

static void flush_stream_buff(video_recoder *recoder)
{
	video_stream_s *video_stream = NULL;

	video_stream = ioctl_get_stream_data(recoder, &recoder->stream_buf_manager);

	while (video_stream) {
		ioctl_return_stream_data(recoder, &recoder->stream_buf_manager, video_stream);

		video_stream = ioctl_get_stream_data(recoder, &recoder->stream_buf_manager);
	}
}

static int rt_init_tdm_manager(tdm_data_manager *p_tdm_data_manager, const rt_media_config_s *rt_config)
{
	int i = 0;
	//init tdm buffer list manager.
	spin_lock_init(&p_tdm_data_manager->s_lock);
	if (rt_config->tdm_rxbuf_cnt)
		p_tdm_data_manager->empty_num = rt_config->tdm_rxbuf_cnt;
	else
		p_tdm_data_manager->empty_num = 5;//default 5 num

	INIT_LIST_HEAD(&p_tdm_data_manager->empty_list);
	INIT_LIST_HEAD(&p_tdm_data_manager->valid_list);
	for (i = 0; i < p_tdm_data_manager->empty_num; i++) {
		tdm_data_node *pNode = kmalloc(sizeof(tdm_data_node), GFP_KERNEL);

		if (pNode == NULL) {
			RT_LOGE("fatal error! kmalloc fail!");
			return ERROR_TYPE_NOMEM;
		}
		memset(pNode, 0, sizeof(tdm_data_node));
		list_add_tail(&pNode->mList, &p_tdm_data_manager->empty_list);
	}
	p_tdm_data_manager->b_inited = 1;
	return 0;
}

int ioctl_init(video_recoder *recoder)
{
	int i				      = 0;
	int ret				      = 0;
	stream_buffer_manager *stream_buf_mgr = &recoder->stream_buf_manager;

	ret = get_component_handle(recoder);

	/* init stream buf manager*/
	mutex_init(&stream_buf_mgr->mutex);
	stream_buf_mgr->empty_num = VENC_OUT_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&stream_buf_mgr->empty_stream_list);
	INIT_LIST_HEAD(&stream_buf_mgr->valid_stream_list);
	INIT_LIST_HEAD(&stream_buf_mgr->caller_hold_stream_list);
	for (i = 0; i < VENC_OUT_BUFFER_LIST_NODE_NUM; i++) {
		video_stream_node *pNode = kmalloc(sizeof(video_stream_node), GFP_KERNEL);

		if (pNode == NULL) {
			RT_LOGE("fatal error! kmalloc fail!");
			return ERROR_TYPE_NOMEM;
		}
		memset(pNode, 0, sizeof(video_stream_node));
		list_add_tail(&pNode->mList, &stream_buf_mgr->empty_stream_list);
	}
	init_waitqueue_head(&stream_buf_mgr->wait_bitstream);

	// init stream buf manager for catch jpeg
	stream_buf_mgr = &recoder->streamBufManagerCatchJpeg;
	mutex_init(&stream_buf_mgr->mutex);
	stream_buf_mgr->empty_num = 1;
	INIT_LIST_HEAD(&stream_buf_mgr->empty_stream_list);
	INIT_LIST_HEAD(&stream_buf_mgr->valid_stream_list);
	INIT_LIST_HEAD(&stream_buf_mgr->caller_hold_stream_list);
	for (i = 0; i < 1; i++) {
		video_stream_node *pNode = kmalloc(sizeof(video_stream_node), GFP_KERNEL);

		if (pNode == NULL) {
			RT_LOGE("fatal error! kmalloc fail!");
		}
		memset(pNode, 0, sizeof(video_stream_node));
		list_add_tail(&pNode->mList, &stream_buf_mgr->empty_stream_list);
	}
	init_waitqueue_head(&stream_buf_mgr->wait_bitstream);

	//init tdm buffer list manager.
#if defined CONFIG_RT_MEDIA_SINGEL_SENSOR || defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	if (recoder->config.enable_tdm_raw && recoder->config.channelId == CSI_SENSOR_0_VIPP_0)
		rt_init_tdm_manager(&rt_media_devp->st_tdm_data_manager[0], &recoder->config);
#endif
#if defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	if (recoder->config.enable_tdm_raw && recoder->config.channelId == CSI_SENSOR_1_VIPP_0)
		rt_init_tdm_manager(&rt_media_devp->st_tdm_data_manager[1], &recoder->config);
#endif
#if defined CONFIG_RT_MEDIA_THREE_SENSOR
	if (recoder->config.enable_tdm_raw && recoder->config.channelId == CSI_SENSOR_2_VIPP_0)
		rt_init_tdm_manager(&rt_media_devp->st_tdm_data_manager[2], &recoder->config);
#endif

	return 0;
}

int ioctl_config(video_recoder *recoder, rt_media_config_s *config)
{
	int connect_flag = 0;
	venc_comp_base_config venc_config;
	vi_comp_base_config vi_config;
	rt_media_config_s *media_config = &recoder->config;
	ve_proc_setting_info proc_ve_info;
	RT_LOGD("Ryan kernel rt-media config recoder->config.output_mode: %d", recoder->config.output_mode);

	if (bvin_is_ready_rt_not_probe[config->channelId]) {
		int i = 0;
		for (; i < VIDEO_INPUT_CHANNEL_NUM; i++) {
			if (g_csi_status_pair[i].bset) {
				rt_media_devp->bcsi_status_set[i] = g_csi_status_pair[i].status;
			}
		}
	}
    /*LOG_LEVEL_VERBOSE = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_INFO = 4,
    LOG_LEVEL_WARNING = 5,
    LOG_LEVEL_ERROR = 6,*/
	cdc_log_set_level(CONFIG_RT_MEDIA_CDC_LOG_LEVEL);

	if (config->vin_buf_num < CONFIG_YUV_BUF_NUM)
		config->vin_buf_num = CONFIG_YUV_BUF_NUM;

	memset(&venc_config, 0, sizeof(venc_comp_base_config));
	memset(&vi_config, 0, sizeof(vi_comp_base_config));
	memset(&proc_ve_info, 0, sizeof(ve_proc_setting_info));

	venc_config.channel_id = recoder->config.channelId;
	if (media_config == NULL) {
		RT_LOGE("ioctl_config: param is null");
		return -1;
	}

	if (recoder->state != RT_MEDIA_STATE_IDLE) {
		RT_LOGW("channel %d the state[%d] is not idle", config->channelId, recoder->state);
		return 0;
	}

	if (config->dst_width == 0 || config->dst_height == 0) {
		config->dst_width = config->width;
		config->dst_height = config->height;
	}

	if (config->dst_fps == 0) {
		config->dst_fps = config->fps;
	}

	memcpy(media_config, config, sizeof(rt_media_config_s));

	/* set the init-config-fps as max_fps */
	recoder->max_fps = media_config->fps;

	if (media_config->channelId < 0 || media_config->channelId > (VIDEO_INPUT_CHANNEL_NUM - 1)) {
		RT_LOGE("channel[%d] is error", media_config->channelId);
		return -1;
	}
	RT_LOGD("recoder->activate_vi_flag: %d recoder->config.output_mode: %d", recoder->activate_vi_flag, recoder->config.output_mode);
	if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
		recoder->activate_venc_flag = 1;
		recoder->activate_vi_flag   = 1;
	} else if (recoder->config.output_mode == OUTPUT_MODE_YUV) {
		recoder->activate_venc_flag = 0;
		recoder->activate_vi_flag = 1;
	} else if (recoder->config.output_mode == OUTPUT_MODE_ENCODE_FILE_YUV
		|| recoder->config.output_mode == OUTPUT_MODE_ENCODE_OUTSIDE_YUV) {
		recoder->activate_venc_flag = 1;
		recoder->activate_vi_flag   = 0;
	}

	RT_LOGD("recoder->activate_vi_flag: %d", recoder->activate_vi_flag);
	if (recoder->state == RT_MEDIA_STATE_IDLE)
		ioctl_init(recoder);

#if defined CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL && defined CONFIG_VIDEO_RT_MEDIA
	RT_LOGW("CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL = %d, configSize = %d",
		CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL, sizeof(rt_media_config_s));
#else
	RT_LOGW("CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL id not enable, configSize = %d", sizeof(rt_media_config_s));
#endif

	RT_LOGW("out_mode = %d, activate_venc = %d, vi = %d, venc_comp = %px, vi = %px, max_fps = %d",
		recoder->config.output_mode,
		recoder->activate_venc_flag, recoder->activate_vi_flag,
		recoder->venc_comp, recoder->vi_comp, recoder->max_fps);

	if (recoder->config.pixelformat == RT_PIXEL_NUM) {
		if (recoder->config.output_mode == OUTPUT_MODE_STREAM)
			recoder->pixelformat = RT_PIXEL_LBC_25X;
		else
			recoder->pixelformat = RT_PIXEL_YVU420SP; //* nv21
	}
    else {
		recoder->pixelformat = recoder->config.pixelformat;
    }

	fill_venc_config(&venc_config, recoder);

	/* setup vipp config */
	vi_config.width		 = media_config->width;
	vi_config.height	 = media_config->height;
	vi_config.frame_rate     = media_config->fps;
	vi_config.dst_fps     = media_config->dst_fps;
	vi_config.channel_id     = media_config->channelId;
	vi_config.output_mode    = media_config->output_mode;
	vi_config.pixelformat    = recoder->pixelformat;
	vi_config.enable_wdr     = media_config->enable_wdr;
	vi_config.drop_frame_num = media_config->drop_frame_num;
	vi_config.bonline_channel = media_config->bonline_channel;
	vi_config.share_buf_num = media_config->share_buf_num;
	vi_config.en_16_align_fill_data = media_config->en_16_align_fill_data;
	vi_config.vin_buf_num = media_config->vin_buf_num;
	vi_config.enable_tdm_raw = media_config->enable_tdm_raw;
	vi_config.tdm_rxbuf_cnt = media_config->tdm_rxbuf_cnt;
	vi_config.dma_merge_mode = media_config->dma_merge_mode;

	if (media_config->dma_merge_mode == DMA_STITCH_VERTICAL) {
		memcpy(&vi_config.dma_overlay_ctx, &media_config->dma_overlay_ctx, sizeof(struct dma_overlay_para));
	}
	if (media_config->dma_merge_scaler_cfg.scaler_en) {
		if (media_config->dma_merge_scaler_cfg.sensorA_scaler_cfg.width &&
			media_config->dma_merge_scaler_cfg.sensorA_scaler_cfg.height &&
			media_config->dma_merge_scaler_cfg.sensorB_scaler_cfg.width &&
			media_config->dma_merge_scaler_cfg.sensorB_scaler_cfg.height) {
			memcpy(&vi_config.dma_merge_scaler_cfg, &media_config->dma_merge_scaler_cfg, sizeof(struct dma_merge_scaler_cfg));
			RT_LOGD("SensorA scaler to %dx%d, SensorB scaler to %dx%d\n",
				media_config->dma_merge_scaler_cfg.sensorA_scaler_cfg.width, media_config->dma_merge_scaler_cfg.sensorA_scaler_cfg.height,
				media_config->dma_merge_scaler_cfg.sensorB_scaler_cfg.width, media_config->dma_merge_scaler_cfg.sensorB_scaler_cfg.height);
		} else {
			RT_LOGE("Scaler SensorA: %dx%d, SensorB: %dx%d, please check it!\n",
				media_config->dma_merge_scaler_cfg.sensorA_scaler_cfg.width, media_config->dma_merge_scaler_cfg.sensorA_scaler_cfg.height,
				media_config->dma_merge_scaler_cfg.sensorB_scaler_cfg.width, media_config->dma_merge_scaler_cfg.sensorB_scaler_cfg.height);
			media_config->dma_merge_scaler_cfg.scaler_en = 0;
		}
	}

	if (media_config->dst_fps < media_config->fps) {
		vi_config.st_adapter_venc_fps.enable = 1;
		vi_config.st_adapter_venc_fps.frame_cnt = -1;
		vi_config.st_adapter_venc_fps.dst_fps = config->dst_fps;
	} else if (config->dst_fps > config->fps) {
		RT_LOGW("Notice dst_fps[%d] > src_fps[%d]", media_config->dst_fps, media_config->fps);
	}

	memcpy(&vi_config.venc_video_signal, &media_config->venc_video_signal, sizeof(VencH264VideoSignal));

	RT_LOGI("enable_wdr = %d, %d", media_config->enable_wdr, vi_config.enable_wdr);
	if (recoder->vi_comp)
		comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Base, &vi_config);

	if (recoder->venc_comp)
		comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);

	if (recoder->vi_comp && recoder->venc_comp) {
		connect_flag = 1;
		comp_tunnel_request(recoder->venc_comp, COMP_INPUT_PORT, recoder->vi_comp, connect_flag);
		comp_tunnel_request(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);
	}
	if (recoder->vi_comp) {
		if (comp_init(recoder->vi_comp) != 0) {
			RT_LOGE("comp_init error");
			return -1;
		}
	}
	RT_LOGD("ch%d enable_tdm_raw:%d", config->channelId, media_config->enable_tdm_raw);

	if (media_config->enable_tdm_raw) {
#if ENABLE_TDM_RAW
		vin_register_tdmbuffer_done_callback(config->channelId, rt_tdm_buffer_done_callback);
#else
		RT_LOGW("fatal error, vin_register_tdmbuffer_done_callback undefined!");
#endif
	}

	if (media_config->width == 0 || media_config->height == 0) {
		vi_comp_base_config tmp_vi_config;

		RT_LOGW("err, width x height: %d %d", media_config->width, media_config->height);
		comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_GET_BASE_CONFIG, &tmp_vi_config);
		media_config->width = tmp_vi_config.width;
		media_config->height = tmp_vi_config.height;
		if (media_config->dst_width == 0 || media_config->dst_height == 0) {
			media_config->dst_width = media_config->width;
			media_config->dst_height = media_config->height;
		}
		venc_config.src_width = media_config->width;
		venc_config.src_height = media_config->height;
		venc_config.dst_width = media_config->dst_width;
		venc_config.dst_height = media_config->dst_height;
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);
		if (recoder->vi_comp)
			comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Base, &tmp_vi_config);
	}
	RT_LOGI("begin init venc comp");
	//* init venc comp
	if (recoder->venc_comp) {
		comp_init(recoder->venc_comp);

//		RT_LOGI("media_config->enable_bin_image = %d, th = %d",
//			media_config->enable_bin_image, media_config->bin_image_moving_th);
//		if (media_config->enable_bin_image == 1) {
//			rt_venc_bin_image_param bin_param;
//
//			bin_param.enable    = 1;
//			bin_param.moving_th = media_config->bin_image_moving_th;
//			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE, &bin_param);
//		}
//		if (media_config->enable_mv_info == 1)
//			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_ENABLE_MV_INFO, &media_config->enable_mv_info);
	}

	if (vi_config.st_adapter_venc_fps.enable == 0 && recoder->activate_vi_flag)
		rt_vin_sensor_fps_change_callback(media_config->channelId);

	if (recoder->config.output_mode == OUTPUT_MODE_ENCODE_FILE_YUV
		|| recoder->config.output_mode == OUTPUT_MODE_ENCODE_OUTSIDE_YUV) {
		recoder->enc_yuv_phy_addr = NULL;
		mutex_init(&recoder->yuv_mutex);
	}
	recoder->state = RT_MEDIA_STATE_CONFIGED;
	RT_LOGI("ioctl_config finish");
	return 0;
}

int ioctl_start(video_recoder *recoder)
{
	RT_LOGI("ioctl_start begin");
	if (recoder->state != RT_MEDIA_STATE_CONFIGED && recoder->state != RT_MEDIA_STATE_PAUSE) {
		RT_LOGW("channel %d ioctl_start: state[%d] is not right", recoder->config.channelId, recoder->state);
		return -1;
	}

	/* int64_t time_end = get_cur_time(); */


	/* start vi comp */
	/*int64_t time_start = get_cur_time(); */
	if (recoder->vi_comp)
		comp_start(recoder->vi_comp);
	/* start venc */
	if (recoder->venc_comp)
		comp_start(recoder->venc_comp);
	/*
	int64_t time_end_1 = get_cur_time();

	RT_LOGD("time[vi start] = %lld, time[venc start] = %lld",
			(time_end - time_start), (time_end_1 - time_end));
	*/

	RT_LOGI("ioctl_start finish");
	recoder->state = RT_MEDIA_STATE_EXCUTING;
	return 0;
}

int ioctl_pause(video_recoder *recoder)
{
	if (recoder->state != RT_MEDIA_STATE_EXCUTING) {
		RT_LOGE("ioctl_start: state[%d] is not right", recoder->state);
		return -1;
	}

	RT_LOGD("comp_pause vi start");
	/* todo : pause vi comp */
	if (recoder->vi_comp)
		comp_pause(recoder->vi_comp);
	RT_LOGD("comp_pause vi end");

	/* pause venc */
	if (recoder->venc_comp)
		comp_pause(recoder->venc_comp);

	RT_LOGD("comp_pause venc end");

	recoder->state = RT_MEDIA_STATE_PAUSE;
	return 0;
}

static void rt_destroy_tdm_list(tdm_data_manager *p_tdm_data_manager)
{
	tdm_data_node *p_tdm_node = NULL;
	int free_cout	   = 0;
	while ((!list_empty(&p_tdm_data_manager->empty_list))) {
		p_tdm_node = list_first_entry(&p_tdm_data_manager->empty_list, tdm_data_node, mList);
		if (p_tdm_node) {
			free_cout++;
			list_del(&p_tdm_node->mList);
			kfree(p_tdm_node);
		}
	}

	while ((!list_empty(&p_tdm_data_manager->valid_list))) {
		p_tdm_node = list_first_entry(&p_tdm_data_manager->valid_list, tdm_data_node, mList);
		if (p_tdm_node) {
			free_cout++;
			list_del(&p_tdm_node->mList);
			kfree(p_tdm_node);
		}
	}
	p_tdm_data_manager->b_inited = 0;

	RT_LOGW("tdm date free cnt %d", free_cout);
}

int ioctl_destroy(video_recoder *recoder)
{
	/* todo */
	video_stream_node *stream_node = NULL;
	int free_stream_cout	   = 0;

	if (recoder->state == RT_MEDIA_STATE_IDLE) {
		RT_LOGW("state is RT_MEDIA_STATE_IDLE, dont need destroy!");
		return -1;
	}
	// prevent rt_vi_comp from sending frames to rt_venc_comp
	if (recoder->vi_comp) {
		comp_pause(recoder->vi_comp);
	}
	int nValidStreamNum = 0;
	int nCallerHoldStreamNum = 0;
	/*free the comp*/
	if (recoder->venc_comp) {
		comp_stop(recoder->venc_comp);

		//rt_media_chn return stream nodes to rt_venc_comp
		mutex_lock(&recoder->stream_buf_manager.mutex);
		struct list_head *pList;
		list_for_each(pList, &recoder->stream_buf_manager.caller_hold_stream_list) { nCallerHoldStreamNum++; }
		list_for_each(pList, &recoder->stream_buf_manager.valid_stream_list) { nValidStreamNum++; }
		mutex_unlock(&recoder->stream_buf_manager.mutex);
		if (0 == nCallerHoldStreamNum) {
			if (nValidStreamNum > 0) {
				RT_LOGW("Be careful! rt_media_chn[%d] will return [%d]valid stream nodes to rt_venc_comp", recoder->config.channelId,
					nValidStreamNum);
			}
			flush_stream_buff(recoder);
		} else {
			RT_LOGE("fatal error! rt_media_chn[%d] has [%d]callerHold stream nodes, app must release them, check code!",
				recoder->config.channelId, nCallerHoldStreamNum);
		}

		comp_free_handle(recoder->venc_comp);
		recoder->venc_comp = NULL;
	}
	if (recoder->vi_comp) {
		comp_stop(recoder->vi_comp);
		comp_free_handle(recoder->vi_comp);
		recoder->vi_comp = NULL;
	}

	mutex_destroy(&recoder->stream_buf_manager.mutex);
	if (recoder->config.output_mode == OUTPUT_MODE_ENCODE_FILE_YUV
		|| recoder->config.output_mode == OUTPUT_MODE_ENCODE_OUTSIDE_YUV) {
		mutex_destroy(&recoder->yuv_mutex);
		recoder->enc_yuv_phy_addr = NULL;
	}
	while ((!list_empty(&recoder->stream_buf_manager.empty_stream_list))) {
		stream_node = list_first_entry(&recoder->stream_buf_manager.empty_stream_list, video_stream_node, mList);
		if (stream_node) {
			free_stream_cout++;
			list_del(&stream_node->mList);
			kfree(stream_node);
		}
	}

	nValidStreamNum = 0;
	while ((!list_empty(&recoder->stream_buf_manager.valid_stream_list))) {
		stream_node = list_first_entry(&recoder->stream_buf_manager.valid_stream_list, video_stream_node, mList);
		if (stream_node) {
			free_stream_cout++;
			nValidStreamNum++;
			list_del(&stream_node->mList);
			kfree(stream_node);
		}
	}
	if (nValidStreamNum > 0) {
		RT_LOGE("fatal error! rt_media_chn[%d] has [%d]valid stream nodes which have been released already. We will improve in future.",
			recoder->config.channelId, nValidStreamNum);
	}
	nCallerHoldStreamNum = 0;
	while ((!list_empty(&recoder->stream_buf_manager.caller_hold_stream_list))) {
		stream_node = list_first_entry(&recoder->stream_buf_manager.caller_hold_stream_list, video_stream_node, mList);
		if (stream_node) {
			free_stream_cout++;
			nCallerHoldStreamNum++;
			list_del(&stream_node->mList);
			kfree(stream_node);
		}
	}
	if (nCallerHoldStreamNum > 0) {
		RT_LOGE("fatal error! rt_media_chn[%d] has [%d]callerHold stream nodes which have been released already."
			"app must release them before call rt_media destory, check code!",
			recoder->config.channelId, nCallerHoldStreamNum);
	}
	RT_LOGD("free_stream_cout = %d", free_stream_cout);

	if (free_stream_cout != VENC_OUT_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of stream node is not match: %d, %d", free_stream_cout, VENC_OUT_BUFFER_LIST_NODE_NUM);

	//destroy stream buf manager for catch jpeg
	video_stream_node *pEntry, *pTemp;
	if (!list_empty(&recoder->streamBufManagerCatchJpeg.caller_hold_stream_list)) {
		RT_LOGE("fatal error! rt_media_chn[%d] why streamBufManagerCatchJpeg holder stream is not empy?", recoder->config.channelId);
		list_for_each_entry_safe (pEntry, pTemp, &recoder->streamBufManagerCatchJpeg.caller_hold_stream_list, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
		}
	}
	if (!list_empty(&recoder->streamBufManagerCatchJpeg.valid_stream_list)) {
		RT_LOGE("fatal error! rt_media_chn[%d] why streamBufManagerCatchJpeg valid stream is not empy?", recoder->config.channelId);
		list_for_each_entry_safe (pEntry, pTemp, &recoder->streamBufManagerCatchJpeg.valid_stream_list, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
		}
	}
	if (!list_empty(&recoder->streamBufManagerCatchJpeg.empty_stream_list)) {
		list_for_each_entry_safe (pEntry, pTemp, &recoder->streamBufManagerCatchJpeg.empty_stream_list, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
		}
	}
	mutex_destroy(&recoder->streamBufManagerCatchJpeg.mutex);

	if (recoder->mNLDstPid != 0) {
		recoder->mNLDstPid = 0;
	}

	if (recoder->config.enable_tdm_raw) {
#if ENABLE_TDM_RAW
		vin_register_tdmbuffer_done_callback(recoder->config.channelId, NULL);
#else
		RT_LOGW("fatal error, vin_register_tdmbuffer_done_callback undefined!");
#endif
	}

#if defined CONFIG_RT_MEDIA_SINGEL_SENSOR || defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	if (recoder->config.enable_tdm_raw && recoder->config.channelId == CSI_SENSOR_0_VIPP_0)
		rt_destroy_tdm_list(&rt_media_devp->st_tdm_data_manager[0]);
#endif
#if defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	if (recoder->config.enable_tdm_raw && recoder->config.channelId == CSI_SENSOR_1_VIPP_0)
		rt_destroy_tdm_list(&rt_media_devp->st_tdm_data_manager[1]);
#endif
#if defined CONFIG_RT_MEDIA_THREE_SENSOR
	if (recoder->config.enable_tdm_raw && recoder->config.channelId == CSI_SENSOR_2_VIPP_0)
		rt_destroy_tdm_list(&rt_media_devp->st_tdm_data_manager[2]);
#endif
	recoder->state = RT_MEDIA_STATE_IDLE;
	recoder->stream_data_cb_count = 0;
	recoder->enable_high_fps	= 0;
	recoder->bReset_high_fps_finish = 0;

	if (recoder->motion_region) {
		kfree(recoder->motion_region);
		recoder->motion_region = NULL;
		recoder->motion_region_len = 0;
	}

//	if (recoder->region_d3d_region) {
//		kfree(recoder->region_d3d_region);
//		recoder->region_d3d_region = NULL;
//		recoder->region_d3d_region_len = 0;
//	}

	if (recoder->insert_data.pBuffer) {
		kfree(recoder->insert_data.pBuffer);
		recoder->insert_data.pBuffer = NULL;
		recoder->insert_data.nDataLen = 0;
		recoder->insert_data.nBufLen = 0;
	}

	return 0;
}

static int ioctl_get_vbv_buf_info(video_recoder *recoder,
				  KERNEL_VBV_BUFFER_INFO *vbv_buf_info)
{
	error_type error = ERROR_TYPE_OK;
	KERNEL_VBV_BUFFER_INFO venc_vbv_info;

	memset(&venc_vbv_info, 0, sizeof(KERNEL_VBV_BUFFER_INFO));

	error = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO, &venc_vbv_info);
	if (error != ERROR_TYPE_OK) {
		RT_LOGI("get vbv buf info failed");
		return -1;
	}

	vbv_buf_info->share_fd = venc_vbv_info.share_fd;
	vbv_buf_info->size     = venc_vbv_info.size;
	return 0;
}

static int ioctl_get_stream_header(video_recoder *recoder, venc_comp_header_data *header_data)
{
	error_type error = ERROR_TYPE_OK;

	RT_LOGD("call comp_get_config start");
	error = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_STREAM_HEADER, header_data);
	RT_LOGD("call comp_get_config finish, error = %d", error);

	if (error != ERROR_TYPE_OK) {
		RT_LOGW("get vbv buf info failed");
		return -1;
	}

	RT_LOGD("ioctl_get_header_data: size = %d, data = %px\n",
		header_data->nLength, header_data->pBuffer);

	return 0;
}

/* valid_list --> caller_hold_list */
static video_stream_s *ioctl_get_stream_data(video_recoder *recoder, stream_buffer_manager *stream_buf_mgr)
{
	video_stream_node *stream_node	= NULL;

	LOCK_MUTEX(&stream_buf_mgr->mutex);

	if (list_empty(&stream_buf_mgr->valid_stream_list)) {
		RT_LOGD("channel %d stream buf manager[%p] valid list is null", recoder->config.channelId, stream_buf_mgr);
		UNLOCK_MUTEX(&stream_buf_mgr->mutex);
		return NULL;
	}
	stream_node = list_first_entry(&stream_buf_mgr->valid_stream_list, video_stream_node, mList);

	list_move_tail(&stream_node->mList, &stream_buf_mgr->caller_hold_stream_list);

	UNLOCK_MUTEX(&stream_buf_mgr->mutex);

	return &stream_node->video_stream;
}

/* caller_hold_list -->  empty_list*/
static int ioctl_return_stream_data(video_recoder *recoder, stream_buffer_manager *stream_buf_mgr,
	video_stream_s *video_stream)
{
	video_stream_node *stream_node	= NULL;
	comp_buffer_header_type buffer_header;
	video_stream_s mvideo_stream;

	memset(&mvideo_stream, 0, sizeof(video_stream_s));
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));

	LOCK_MUTEX(&stream_buf_mgr->mutex);

	if (list_empty(&stream_buf_mgr->caller_hold_stream_list)) {
		RT_LOGE("fatal error! ioctl return_stream_data: error, caller_hold_list is null");
		UNLOCK_MUTEX(&stream_buf_mgr->mutex);
		return -1;
	}

	stream_node = list_first_entry(&stream_buf_mgr->caller_hold_stream_list, video_stream_node, mList);
	//memmove(&stream_node->video_stream, video_stream, sizeof(video_stream_s));
	if (stream_node->video_stream.id != video_stream->id) {
		RT_LOGE("fatal error! ioctl return_stream_data: id not match: %d!=%d. return now!", stream_node->video_stream.id, video_stream->id);
		UNLOCK_MUTEX(&stream_buf_mgr->mutex);
		return -1;
	}
	memcpy(&mvideo_stream, &stream_node->video_stream, sizeof(video_stream_s));

	list_move_tail(&stream_node->mList, &stream_buf_mgr->empty_stream_list);
	stream_buf_mgr->empty_num++;

	UNLOCK_MUTEX(&stream_buf_mgr->mutex);

	buffer_header.private = &mvideo_stream;

	rt_component_type *pVencComp = NULL;
	if (stream_buf_mgr == &recoder->stream_buf_manager) {
		pVencComp = recoder->venc_comp;
	} else if (stream_buf_mgr == &recoder->streamBufManagerCatchJpeg) {
		pVencComp = recoder->vencCompCatchJpeg;
	} else {
		RT_LOGE("fatal error! rt_media_chn[%d] unknown stream buf manager[%p]", recoder->config.channelId, stream_buf_mgr);
	}
	comp_fill_this_out_buffer(pVencComp, &buffer_header);

	return 0;
}

static int copy_jpeg_data(catch_jpeg_buf_info *user_buf_info, video_stream_s *video_stream)
{
	int result = 0;
	catch_jpeg_buf_info src_buf_info;

	memset(&src_buf_info, 0, sizeof(catch_jpeg_buf_info));

	if (copy_from_user(&src_buf_info, (void __user *)user_buf_info, sizeof(struct catch_jpeg_buf_info))) {
		RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
		return -EFAULT;
	}

	if (video_stream->size0 + video_stream->size1 + video_stream->size2 > src_buf_info.buf_size) {
		RT_LOGE("buf_size overflow: %d > %d", video_stream->size0 + video_stream->size1 + video_stream->size2,
			src_buf_info.buf_size);
		result = -1;
		goto copy_jpeg_data_exit;
	}

	if (copy_to_user((void *)src_buf_info.buf, video_stream->data0, video_stream->size0)) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto copy_jpeg_data_exit;
	}
	if (video_stream->size1 > 0) {
		if (copy_to_user((void *)src_buf_info.buf + video_stream->size0, video_stream->data1, video_stream->size1)) {
			RT_LOGE(" copy_to_user fail\n");
			result = -1;
			goto copy_jpeg_data_exit;
		}
	}
	if (video_stream->size2 > 0) {
		if (copy_to_user((void *)src_buf_info.buf + video_stream->size0 + video_stream->size1, video_stream->data2,
			video_stream->size2)) {
			RT_LOGE(" copy_to_user fail\n");
			result = -1;
			goto copy_jpeg_data_exit;
		}
	}

	unsigned int jpegLen = video_stream->size0 + video_stream->size1 + video_stream->size2;
	if (copy_to_user((void *)user_buf_info, &jpegLen, sizeof(unsigned int))) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto copy_jpeg_data_exit;
	}

copy_jpeg_data_exit:

	return result;
}

static int ioctl_output_yuv_catch_jpeg_start(video_recoder *recoder, catch_jpeg_config *jpeg_config)
{
	int ret		    = 0;
	int connect_flag    = 0;
	int catch_jpeg_flag = 0;
	venc_comp_base_config venc_config;
	rt_media_config_s *media_config = &recoder->config;

	memset(&venc_config, 0, sizeof(venc_comp_base_config));
	ret = comp_get_handle((comp_handle *)&recoder->vencCompCatchJpeg,
			"video.encoder",
			recoder,
			&recoder->config,
			&venc_callback);
	if (ret != ERROR_TYPE_OK) {
		RT_LOGE("get venc comp handle error: %d", ret);
		return -1;
	}

	venc_config.quality		  = jpeg_config->qp;
	venc_config.codec_type		  = RT_VENC_CODEC_JPEG;
	venc_config.src_width		  = media_config->width;
	venc_config.src_height		  = media_config->height;
	venc_config.dst_width		  = jpeg_config->width;
	venc_config.dst_height		  = jpeg_config->height;
	venc_config.frame_rate		  = media_config->dst_fps;
	venc_config.bit_rate		  = media_config->bitrate;
	venc_config.pixelformat		  = recoder->pixelformat;
	venc_config.max_keyframe_interval = media_config->gop;
	venc_config.rotate_angle = jpeg_config->rotate_angle;
	venc_config.channel_id = jpeg_config->channel_id;
	memcpy(&venc_config.venc_video_signal, &media_config->venc_video_signal, sizeof(VencH264VideoSignal));
	venc_config.mRcMode = AW_CBR;

	venc_config.en_encpp_sharp = media_config->enable_sharp;
	if (rt_is_format422(venc_config.pixelformat)) {
		venc_config.en_encpp_sharp = 0;
	}
	if (0 < jpeg_config->overlay_info.osd_num) {
		venc_config.enable_overlay = 1;
	}
	venc_config.nRecvFrameNum = 1; //only take one jpeg, so receive one frame is enough.
	RT_LOGI("src_w&h = %d, %d; dst_w&h = %d, %d, pixelformat = %d", venc_config.src_width, venc_config.src_height,
		venc_config.dst_width, venc_config.dst_height, venc_config.pixelformat);

	comp_set_config(recoder->vencCompCatchJpeg, COMP_INDEX_VENC_CONFIG_Base, &venc_config);

	connect_flag = 1;

	comp_tunnel_request(recoder->vencCompCatchJpeg, COMP_INPUT_PORT, recoder->vi_comp, connect_flag);

	comp_tunnel_request(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->vencCompCatchJpeg, connect_flag);

	comp_init(recoder->vencCompCatchJpeg);

	comp_start(recoder->vencCompCatchJpeg);

	if (0 < jpeg_config->overlay_info.osd_num) {
		RT_LOGD("overlay_info.osd_num: %d\n", jpeg_config->overlay_info.osd_num);
		ret = comp_set_config(recoder->vencCompCatchJpeg, COMP_INDEX_VENC_CONFIG_SET_OSD, (void *)&jpeg_config->overlay_info);
	}

//	catch_jpeg_flag = 1;
//	comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG, (void *)&catch_jpeg_flag);

	return ret;
}

static int ioctl_output_yuv_catch_jpeg_stop(video_recoder *recoder)
{
	int connect_flag    = 0;
	int catch_jpeg_flag = 0;

	//comp_pause(recoder->vi_comp);

	/* disconnect the tunnel */
	comp_tunnel_request(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->vencCompCatchJpeg, connect_flag);

	//comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG, (void *)&catch_jpeg_flag);

	//comp_start(recoder->vi_comp);

	/* destroy venc comp*/
	comp_pause(recoder->vencCompCatchJpeg);

	comp_stop(recoder->vencCompCatchJpeg);

	comp_free_handle(recoder->vencCompCatchJpeg);

	recoder->vencCompCatchJpeg = NULL;

	//deinit stream buf manager
	video_stream_node *pEntry, *pTmp;
	stream_buffer_manager *stream_buf_mgr = &recoder->streamBufManagerCatchJpeg;
	if (!list_empty(&stream_buf_mgr->caller_hold_stream_list)) {
		RT_LOGE("fatal error! why caller hold stream not empty?");
		list_for_each_entry_safe (pEntry, pTmp, &stream_buf_mgr->caller_hold_stream_list, mList) {
			list_move_tail(&pEntry->mList, &stream_buf_mgr->empty_stream_list);
			stream_buf_mgr->empty_num++;
		}
	}
	if (!list_empty(&stream_buf_mgr->valid_stream_list)) {
		RT_LOGE("fatal error! why valid stream not empty?");
		list_for_each_entry_safe (pEntry, pTmp, &stream_buf_mgr->valid_stream_list, mList) {
			list_move_tail(&pEntry->mList, &stream_buf_mgr->empty_stream_list);
			stream_buf_mgr->empty_num++;
		}
	}
	if (!list_empty(&stream_buf_mgr->empty_stream_list)) {
		struct list_head *pList;
		int cnt = 0; list_for_each(pList, &stream_buf_mgr->empty_stream_list) { cnt++; }
		if (cnt != stream_buf_mgr->empty_num) {
			RT_LOGE("fatal error! stream buf manager of catch jpeg empty num wrong:%d!=%d", cnt, stream_buf_mgr->empty_num);
		}
	}

	RT_LOGI("rt_media_chn[%d] vencCompCatchJpeg = %p", recoder->config.channelId, recoder->vencCompCatchJpeg);
	return 0;
}

static int ioctl_output_yuv_catch_jpeg_getData(video_recoder *recoder, catch_jpeg_buf_info *user_buf_info)
{
	int result = 0;
	video_stream_s *video_stream = NULL;
	int loop_count		     = 0;
	int loop_max_count	   = 2; /* mean 2 s*/

catch_jpeg_get_stream_data:

	video_stream = ioctl_get_stream_data(recoder, &recoder->streamBufManagerCatchJpeg);
	if (video_stream == NULL) {
		if (loop_count >= loop_max_count) {
			RT_LOGE("channel %d get stream data timeout", recoder->config.channelId);
			return -EFAULT;
		}

		RT_LOGD("channel %d catch_jpeg_getData: video stream is null", recoder->config.channelId);

		LOCK_MUTEX(&recoder->streamBufManagerCatchJpeg.mutex);
		recoder->streamBufManagerCatchJpeg.wait_bitstream_condition = 0;
		recoder->streamBufManagerCatchJpeg.need_wait_up_flag	= 1;
		UNLOCK_MUTEX(&recoder->streamBufManagerCatchJpeg.mutex);

		wait_event_timeout(recoder->streamBufManagerCatchJpeg.wait_bitstream,
			recoder->streamBufManagerCatchJpeg.wait_bitstream_condition, msecs_to_jiffies(1000));

		loop_count++;
		goto catch_jpeg_get_stream_data;
	}

	RT_LOGI("video_stream->offset0 = %d, size0 = %d, data0 = %p", video_stream->offset0, video_stream->size0,
		video_stream->data0);

	result = copy_jpeg_data(user_buf_info, video_stream);

	ioctl_return_stream_data(recoder, &recoder->streamBufManagerCatchJpeg, video_stream);

	return result;
}

static int ioctl_reset_encoder_type(video_recoder *recoder, RT_VENC_CODEC_TYPE encoder_type)
{
	venc_comp_base_config venc_config;
	int connect_flag	   = 0;
	int ret			   = 0;
	comp_state_type comp_state = 0;

	memset(&venc_config, 0, sizeof(venc_comp_base_config));

	if (recoder->config.encodeType == encoder_type) {
		RT_LOGE("encoder type is the same, no need to reset: %d, %d",
			recoder->config.encodeType, encoder_type);
		return -1;
	}

	/* pause venc and vi comp */
	comp_get_state(recoder->venc_comp, &comp_state);
	RT_LOGI("state of venc_comp: %d", comp_state);
	if (comp_state != COMP_STATE_PAUSE)
		comp_pause(recoder->venc_comp);

	comp_get_state(recoder->vi_comp, &comp_state);
	RT_LOGI("state of vi_comp: %d", comp_state);
	if (comp_state != COMP_STATE_PAUSE)
		comp_pause(recoder->vi_comp);

	flush_stream_buff(recoder);

	/* flush buffer of venc comp */
	comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_FLUSH_IN_BUFFER, NULL);

	comp_stop(recoder->venc_comp);

	/* disconnect the tunnel */
	connect_flag = 0;
	comp_tunnel_request(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);

	/* destroy venc comp*/
	comp_free_handle(recoder->venc_comp);

	recoder->venc_comp = NULL;

	/* create and init venc comp */
	ret = comp_get_handle((comp_handle *)&recoder->venc_comp,
			"video.encoder",
			recoder,
			&recoder->config,
			&venc_callback);
	if (ret != ERROR_TYPE_OK) {
		RT_LOGE("get venc comp handle error: %d", ret);
		return -1;
	}

	recoder->config.encodeType = encoder_type;

	fill_venc_config(&venc_config, recoder);

	comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);

	connect_flag = 1;

	comp_tunnel_request(recoder->venc_comp, COMP_INPUT_PORT, recoder->vi_comp, connect_flag);

	comp_tunnel_request(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);

	comp_init(recoder->venc_comp);

	/* restart venc and vi comp*/
	//comp_start(recoder->venc_comp);

	//comp_start(recoder->vi_comp);

	return 0;
}
static int fops_mmap(struct file *filp, struct vm_area_struct *vma)
{
	//* todo

	return 0;
}

static int fops_open(struct inode *inode, struct file *filp)
{
	media_private_info *media_info;

	media_info = kmalloc(sizeof(media_private_info), GFP_KERNEL);
	if (!media_info)
		return -ENOMEM;

	memset(media_info, 0, sizeof(media_private_info));
	media_info->channel = -1;

	filp->private_data = media_info;

	nonseekable_open(inode, filp);
	return 0;
}

static int fops_release(struct inode *inode, struct file *filp)
{
	/* todo */
	media_private_info *media_info = filp->private_data;

	kfree(media_info);

	return 0;
}

static long fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* todo */
	int result = 0;
	int ret			       = 0;
	int i = 0;
	media_private_info *media_info = filp->private_data;
	video_recoder *recoder	 = &rt_media_devp->recoder[media_info->channel];

	switch (cmd) {
	case IOCTL_CONFIG: {
		rt_media_config_s config;

		memset(&config, 0, sizeof(rt_media_config_s));

		if (copy_from_user(&config, (void __user *)arg, sizeof(struct rt_media_config_s))) {
			RT_LOGE("IOCTL CONFIG copy_from_user fail\n");
			return -EFAULT;
		}

		if (rt_media_devp->bcsi_err
		&& (config.output_mode == OUTPUT_MODE_STREAM
			|| config.output_mode == OUTPUT_MODE_YUV)) {
			RT_LOGE("csi is err\n");
			return -1;
		}

		if (config.channelId < 0 || config.channelId > (VIDEO_INPUT_CHANNEL_NUM - 1)) {
			RT_LOGE("IOCTL CONFIG: channel[%d] is error", config.channelId);
			return -1;
		}

		RT_LOGD("IOCTL CONFIG: media_info->channel = %d, config->channelId = %d, config.dma_merge_mode = %d",
			media_info->channel, config.channelId, config.dma_merge_mode);

		if (media_info->channel == -1)
			media_info->channel = config.channelId;

		if (media_info->channel != config.channelId) {
			RT_LOGE("IOCTL CONFIG: channel id not match = %d, %d",
				media_info->channel, config.channelId);
			return -1;
		}

		recoder = &rt_media_devp->recoder[media_info->channel];
		if (ioctl_config(recoder, &config) != 0) {
			RT_LOGE("config err");
			return -1;
		}
		RT_LOGD("IOCTL CONFIG end");
		break;
	}
	case IOCTL_START: {
		RT_LOGD("IOCTL START start %d", recoder->config.width);
		RT_LOGS("rt_media_devp->in_high_fps_status %d", rt_media_devp->in_high_fps_status);
		if (recoder->enable_high_fps == 1 && recoder->bReset_high_fps_finish == 0) {
			RT_LOGW("bReset_high_fps_finish is not 1, not start, state = %d", recoder->state);
			return -EFAULT;
		}

		if (recoder->config.channelId == 1 && rt_media_devp->in_high_fps_status == 1) {
			rt_media_devp->need_start_recoder_1 = recoder;
			RT_LOGW("the high fps is not finish , not start the second recoder channel");
			return 0;
		}

		if (recoder->config.channelId == 2 && rt_media_devp->in_high_fps_status == 1) {
			rt_media_devp->need_start_recoder_2 = recoder;
			RT_LOGW("the high fps is not finish , not start the third recoder channel");
			return 0;
		}

	#if defined CONFIG_VIDEO_RT_MEDIA
		if (recoder->config.output_mode == OUTPUT_MODE_STREAM  || recoder->config.output_mode == OUTPUT_MODE_YUV) {
			if (rt_media_devp->bcsi_status_set[media_info->channel]) {
				ioctl_start(recoder);
			} else {
				RT_LOGW("csi not ready.");
				return -1;
			}
		} else {
			ioctl_start(recoder);
		}
	#else
		ioctl_start(recoder);
	#endif

		RT_LOGD("IOCTL START end");
		break;
	}
	case IOCTL_PAUSE: {
		RT_LOGD("IOCTL PAUSE start");
		ioctl_pause(recoder);
		RT_LOGD("IOCTL PAUSE end");
		break;
	}
	case IOCTL_DESTROY: {
		RT_LOGD("IOCTL_DESTROY start");
		ioctl_destroy(recoder);
		RT_LOGD("IOCTL_DESTROY end");
		break;
	}
	case IOCTL_GET_VBV_BUFFER_INFO: {
		KERNEL_VBV_BUFFER_INFO vbv_buf_info;

		memset(&vbv_buf_info, 0, sizeof(vbv_buf_info));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (ioctl_get_vbv_buf_info(recoder, &vbv_buf_info) != 0) {
			RT_LOGI("ioctl_get_vbv_buf_info failed");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &vbv_buf_info, sizeof(KERNEL_VBV_BUFFER_INFO))) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_STREAM_HEADER_SIZE: {
		venc_comp_header_data header_data;

		memset(&header_data, 0, sizeof(venc_comp_header_data));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_STREAM_HEADER_SIZE: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (ioctl_get_stream_header(recoder, &header_data) != 0) {
			RT_LOGW("ioctl_get_stream_header failed");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &header_data.nLength, sizeof(int))) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO copy_to_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("end IOCTL_GET_STREAM_HEADER_SIZE: header_data.nLength %d\n", header_data.nLength);
		break;
	}
	case IOCTL_GET_STREAM_HEADER_DATA: {
		venc_comp_header_data header_data;
		memset(&header_data, 0, sizeof(venc_comp_header_data));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_STREAM_HEADER_DATA: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (ioctl_get_stream_header(recoder, &header_data) != 0) {
			RT_LOGW("ioctl_get_stream_header failed");
			return -EFAULT;
		}
		if (copy_to_user((void *)arg, header_data.pBuffer, header_data.nLength)) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO copy_to_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("IOCTL_GET_STREAM_HEADER_DATA: %x %x %x %x %x %x",
			header_data.pBuffer[0],
			header_data.pBuffer[1],
			header_data.pBuffer[2],
			header_data.pBuffer[3],
			header_data.pBuffer[4],
			header_data.pBuffer[5]);
		break;
	}
	case IOCTL_GET_STREAM_DATA: {
		int rc;
		video_stream_s *video_stream = NULL;

		if (recoder->venc_comp == NULL) {
			RT_LOGE("channel %d IOCTL GET_STREAM_DATA: recoder->venc_comp is null", recoder->config.channelId);
			return -EFAULT;
		}
	_get_stream:
		video_stream = ioctl_get_stream_data(recoder, &recoder->stream_buf_manager);
		if (video_stream == NULL) {
			RT_LOGD("channel %d IOCTL GET_STREAM_DATA: video stream is null", recoder->config.channelId);

			LOCK_MUTEX(&recoder->stream_buf_manager.mutex);
			recoder->stream_buf_manager.wait_bitstream_condition = 0;
			recoder->stream_buf_manager.need_wait_up_flag = 1;
			UNLOCK_MUTEX(&recoder->stream_buf_manager.mutex);
			rc = wait_event_timeout(recoder->stream_buf_manager.wait_bitstream, recoder->stream_buf_manager.wait_bitstream_condition,
				HZ/5);
			LOCK_MUTEX(&recoder->stream_buf_manager.mutex);
			if (0 == rc) {
				if (0 == recoder->stream_buf_manager.need_wait_up_flag) {
					RT_LOGE("fatal error! rt_media_chn[%d] has no stream, but flag-condition[%d-%d]!", recoder->config.channelId,
						recoder->stream_buf_manager.need_wait_up_flag, recoder->stream_buf_manager.wait_bitstream_condition);
				}
				UNLOCK_MUTEX(&recoder->stream_buf_manager.mutex);
				return -EFAULT;
			} else {
				if (recoder->stream_buf_manager.need_wait_up_flag) {
					RT_LOGE("fatal error! rt_media_chn[%d] stream ready[%d], but flag-condition[%d-%d]!", recoder->config.channelId, rc,
						recoder->stream_buf_manager.need_wait_up_flag, recoder->stream_buf_manager.wait_bitstream_condition);
				}
				UNLOCK_MUTEX(&recoder->stream_buf_manager.mutex);
				goto _get_stream;
			}
		}

		RT_LOGD("channel %d video_stream->offset0 = %d, size0 = %d, dataNum=%d", recoder->config.channelId,
			video_stream->offset0, video_stream->size0, video_stream->nDataNum);

		RT_LOGI("get stream, pts = %lld", video_stream->pts);
		if (copy_to_user((void *)arg, video_stream, sizeof(video_stream_s))) {
			RT_LOGE("channel %d IOCTL GET_STREAM_DATA copy_to_user fail", recoder->config.channelId);
			return -EFAULT;
		}

		break;
	}
	case IOCTL_RETURN_STREAM_DATA: {
		video_stream_s cur_stream;

		memset(&cur_stream, 0, sizeof(video_stream_s));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL RETURN_STREAM_DATA: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (copy_from_user(&cur_stream, (void __user *)arg, sizeof(struct video_stream_s))) {
			RT_LOGE("IOCTL RETURN_STREAM_DATA copy_from_user fail\n");
			return -EFAULT;
		}

		ioctl_return_stream_data(recoder, &recoder->stream_buf_manager, &cur_stream);

		break;
	}
	case IOCTL_CATCH_JPEG_START: {
		catch_jpeg_config *pJpegConfig = (catch_jpeg_config *)kmalloc(sizeof(catch_jpeg_config), GFP_KERNEL);
		if (NULL == pJpegConfig) {
			RT_LOGE("fatal error! malloc fail");
			return -ENOMEM;
		}
		memset(pJpegConfig, 0, sizeof(catch_jpeg_config));

		if (copy_from_user(pJpegConfig, (void __user *)arg, sizeof(struct catch_jpeg_config))) {
			RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
			kfree(pJpegConfig);
			return -EFAULT;
		}

//		if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
//			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START, pJpegConfig);
//			if (ret != 0) {
//				RT_LOGE("COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START failed");
//				vfree(pJpegConfig);
//				return -EFAULT;
//			}
//		} else {
			ioctl_output_yuv_catch_jpeg_start(recoder, pJpegConfig);
//		}
		kfree(pJpegConfig);
		break;
	}
	case IOCTL_CATCH_JPEG_STOP: {
//		if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
//			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CATCH_JPEG_STOP, 0);
//			if (ret != 0) {
//				RT_LOGE("COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START failed");
//				return -EFAULT;
//			}
//		} else
			ioctl_output_yuv_catch_jpeg_stop(recoder);

		break;
	}
	case IOCTL_CATCH_JPEG_GET_DATA: {
//		if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
//			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA, (void *)arg);
//			if (ret != 0) {
//				RT_LOGE("COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA failed");
//				return -EFAULT;
//			}
//		} else
			return ioctl_output_yuv_catch_jpeg_getData(recoder, (catch_jpeg_buf_info *)arg);

		break;
	}
	case IOCTL_SET_OSD: {
		if (recoder->config.output_mode != OUTPUT_MODE_YUV) {
			VideoInputOSD *pInputOsd = (VideoInputOSD *)kmalloc(sizeof(VideoInputOSD), GFP_KERNEL);
			//memset(pInputOsd, 0, sizeof(VideoInputOSD));
			if (copy_from_user(pInputOsd, (void __user *)arg, sizeof(VideoInputOSD))) {
				RT_LOGE("set osd copy_from_user fail\n");
				kfree(pInputOsd);
				return -EFAULT;
			}

			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_OSD, (void *)pInputOsd);
			if (ret != 0) {
				RT_LOGE("IOCTL_SET_OSD failed");
				kfree(pInputOsd);
				return -EFAULT;
			}
			kfree(pInputOsd);
		} else {
			RT_LOGE("IOCTL_SET_OSD is not support OUTPUT_MODE_YUV, please use IOCTL_CATCH_JPEG_START");
			return -EFAULT;
		}
		break;
	}
//	case IOCTL_GET_BIN_IMAGE_DATA: {
//		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA, (void *)arg);
//		if (ret <= 0) {
//			RT_LOGE("IOCTL_GET_BIN_IMAGE_DATA failed");
//			return -EFAULT;
//		}
//		RT_LOGI("IOCTL_GET_BIN_IMAGE_DATA, ret = %d", ret);
//		return ret;
//	}
//	case IOCTL_GET_MV_INFO_DATA: {
//		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA, (void *)arg);
//		if (ret <= 0) {
//			RT_LOGE("IOCTL_GET_MV_INFO_DATA failed");
//			return -EFAULT;
//		}
//		RT_LOGI("IOCTL_GET_MV_INFO_DATA, ret = %d", ret);
//		return ret;
//	}
	case IOCTL_CONFIG_YUV_BUF_INFO: {
		if (copy_from_user(&recoder->yuv_buf_info, (void __user *)arg, sizeof(struct config_yuv_buf_info))) {
			RT_LOGE("copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_YUV_BUF_INFO, (void *)&recoder->yuv_buf_info);
		if (ret < 0) {
			RT_LOGE("failed, ret = %d", ret);
			return -EFAULT;
		}

		break;
	}
	case IOCTL_REQUEST_YUV_FRAME: {
		rt_yuv_info s_yuv_info;

		memset(&s_yuv_info, 0, sizeof(rt_yuv_info));
		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_REQUEST_YUV_FRAME, (void *)&s_yuv_info);
		if (ret < 0) {
			RT_LOGE("IOCTL_REQUEST_YUV_FRAME failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &s_yuv_info, sizeof(s_yuv_info))) {
			RT_LOGE("IOCTL_REQUEST_YUV_FRAME copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_RETURN_YUV_FRAME: {
		unsigned char *phy_addr = NULL;

		if (copy_from_user(&phy_addr, (void __user *)arg, sizeof(unsigned char *))) {
			RT_LOGE("IOCTL_RETURN_YUV_FRAME copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_RETURN_YUV_FRAME, (void *)phy_addr);
		if (ret < 0) {
			RT_LOGE("IOCTL_RETURN_YUV_FRAME failed, ret = %d", ret);
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_ISP_LV: {
		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_LV, NULL);
		if (ret < 0) {
			RT_LOGE("IOCTL GET_ISP_LV failed, ret = %d", ret);
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_GET_ISP_HIST: {
		unsigned int *hist = (unsigned int *)kmalloc(sizeof(unsigned int)*256, GFP_KERNEL);

		memset(hist, 0, 256 * sizeof(unsigned int));

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_HIST, (void *)&hist[0]);
		if (ret < 0) {
			RT_LOGE("IOCTL GET_ISP_HIST failed, ret = %d", ret);
			kfree(hist);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &hist[0], 256 * sizeof(unsigned int))) {
			RT_LOGE("IOCTL GET_ISP_HIST copy_to_user fail\n");
			kfree(hist);
			return -EFAULT;
		}
		kfree(hist);
		return ret;
	}
	case IOCTL_SET_SENSOR_EXP: {
		int exp_time = (int)arg;

		RT_LOGI("exp_time = %d", exp_time);

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_EXP, (void *)exp_time);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_SENSOR_EXP failed, ret = %d", ret);
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_SET_SENSOR_GAIN: {
		int sensor_gain = (int)arg;

		RT_LOGI("sensor_gain = %d", sensor_gain);

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_GAIN, (void *)sensor_gain);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_ISP_GAIN failed, ret = %d", ret);
			return -EFAULT;
		}

		return ret;
	}
	case IOCTL_GET_ISP_EXP_GAIN: {
		struct sensor_exp_gain exp_gain;

		memset(&exp_gain, 0, sizeof(struct sensor_exp_gain));

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_EXP_GAIN, (void *)&exp_gain);
		if (ret < 0) {
			RT_LOGE("IOCTL GET_ISP_EXP_GAIN failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &exp_gain, sizeof(struct sensor_exp_gain))) {
			RT_LOGE("IOCTL GET_ISP_EXP_GAIN copy_to_user fail\n");
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_SET_ISP_ATTR_CFG: {
//		if (sizeof(RTIspCfgAttrData) != sizeof(struct isp_cfg_attr_data)) {
//			RT_LOGE("fatal error! rt_media and isp cfg attr data size is not same, check code!");
//		}
		RTIspCtrlAttr *p_ctrlattr = kmalloc(sizeof(RTIspCtrlAttr), GFP_KERNEL);
		memset(p_ctrlattr, 0, sizeof(RTIspCtrlAttr));

		if (copy_from_user(p_ctrlattr, (void __user *)arg, sizeof(RTIspCtrlAttr))) {
			RT_LOGE("IOCTL SET_ISP_ATTR_CFG copy_from_user fail\n");
			kfree(p_ctrlattr);
			return -EFAULT;
		}

		RT_LOGD("IOCTL SET_ISP_ATTR_CFG:ctrl id:%d\n", p_ctrlattr->isp_attr_cfg.cfg_id);

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG,
			(void *)&p_ctrlattr->isp_attr_cfg);
		if (ret < 0) {
			RT_LOGE("COMP INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG failed, ret = %d", ret);
			kfree(p_ctrlattr);
			return -EFAULT;
		}
		kfree(p_ctrlattr);
		break;
	}
	case IOCTL_GET_ISP_ATTR_CFG: {
//		if (sizeof(RTIspCfgAttrData) != sizeof(struct isp_cfg_attr_data)) {
//			RT_LOGE("fatal error! rt_media and isp cfg attr data size is not same, check code!");
//		}
		RTIspCtrlAttr *p_ctrlattr = kmalloc(sizeof(RTIspCtrlAttr), GFP_KERNEL);
		memset(p_ctrlattr, 0, sizeof(RTIspCtrlAttr));

		if (copy_from_user(p_ctrlattr, (void __user *)arg, sizeof(RTIspCtrlAttr))) {
			RT_LOGE("IOCTL_GET_ISP_ATTR_CFG copy_from_user fail\n");
			kfree(p_ctrlattr);
			return -EFAULT;
		}

		RT_LOGD("IOCTL_GET_ISP_ATTR_CFG:ctrl id:%d\n", p_ctrlattr->isp_attr_cfg.cfg_id);

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_ISP_ARRT_CFG,
			(void *)&p_ctrlattr->isp_attr_cfg);
		if (ret < 0) {
			RT_LOGE("COMP_INDEX_VI_CONFIG_Dynamic_GET_ISP_ARRT_CFG failed, ret = %d", ret);
			kfree(p_ctrlattr);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, p_ctrlattr, sizeof(RTIspCtrlAttr))) {
			RT_LOGE("IOCTL_GET_ISP_ATTR_CFG copy_to_user fail\n");
			kfree(p_ctrlattr);
			return -EFAULT;
		}
		kfree(p_ctrlattr);
		break;
	}
	case IOCTL_SET_ISP_ORL: {
		RTIspOrl isp_orl;

		memset(&isp_orl, 0, sizeof(RTIspOrl));

		if (copy_from_user(&isp_orl, (void __user *)arg, sizeof(RTIspOrl))) {
			RT_LOGE("IOCTL SET_ISP_ORL copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_ORL, (void *)&isp_orl);
		if (ret < 0) {
			RT_LOGE("COMP INDEX_VI_CONFIG_Dynamic_SET_ORL failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_ISP_AE_METERING_MODE: {
		int ae_metering_mode = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_METERING_MODE, &ae_metering_mode);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_AE_METERING_MODE failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_ISP_AE_MODE: {
		int ae_mode = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_MODE, (void *)ae_mode);

		if (ret < 0) {
			RT_LOGE("COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_MODE failed, ret = %d", ret);
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_SET_IR_PARAM: {
		RTIrParam mIrParam;
		int is_night_case = 0;

		memset(&mIrParam, 0, sizeof(RTIrParam));

		if (copy_from_user(&mIrParam, (void __user *)arg, sizeof(RTIrParam))) {
			RT_LOGE("IOCTL_SET_IR_COLOR2GREY copy_from_user fail\n");
			return -EFAULT;
		}

		if (mIrParam.grey == 1)
			is_night_case = 1;

		RT_LOGW("is_night_case = %d, venc_comp = %px", is_night_case, recoder->venc_comp);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE, (void *)&is_night_case);
			if (ret < 0)
				RT_LOGW("VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE failed, ret = %d", ret);
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_IR_PARAM, (void *)&mIrParam);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_IR_COLOR2GREY failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_HORIZONTAL_FLIP: {
		int bhflip = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_H_FLIP, &bhflip);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_HORIZONTAL_FLIP failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VERTICAL_FLIP: {
		int bvflip = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_V_FLIP, &bvflip);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_VERTICAL_FLIP failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_FORCE_KEYFRAME: {
		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_ForceKeyFrame, NULL);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_FORCE_KEYFRAME failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_RESET_ENCODER_TYPE: {
		RT_VENC_CODEC_TYPE encoder_type = (RT_VENC_CODEC_TYPE)arg;

		ret = ioctl_reset_encoder_type(recoder, encoder_type);
		if (ret != 0) {
			RT_LOGE("reset encoder type, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_REQUEST_ENC_YUV_FRAME: {
		unsigned char *phy_addr = NULL;

		LOCK_MUTEX(&recoder->yuv_mutex);
		if (recoder->enc_yuv_phy_addr == NULL) {
			RT_LOGI(" enc_yuv_phy_addr is null");
			UNLOCK_MUTEX(&recoder->yuv_mutex);
			return -1;
		}
		phy_addr		  = recoder->enc_yuv_phy_addr;
		recoder->enc_yuv_phy_addr = NULL;
		UNLOCK_MUTEX(&recoder->yuv_mutex);
		if (copy_to_user((void *)arg, &phy_addr, sizeof(unsigned char *))) {
			RT_LOGE("IOCTL REQUEST_ENC_YUV_FRAME copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SUBMIT_ENC_YUV_FRAME: {
		unsigned char *phy_addr = NULL;
		comp_buffer_header_type mbuffer_header;
		video_frame_s mvideo_frame;
		int align_width  = RT_ALIGN(recoder->config.width, 16);
		int align_height = RT_ALIGN(recoder->config.height, 16);
		int y_size       = align_width * align_height;

		memset(&mbuffer_header, 0, sizeof(comp_buffer_header_type));
		memset(&mvideo_frame, 0, sizeof(video_frame_s));

		if (copy_from_user(&phy_addr, (void __user *)arg, sizeof(unsigned char *))) {
			RT_LOGE("IOCTL_RETURN_YUV_FRAME copy_from_user fail\n");
			return -EFAULT;
		}
		mvideo_frame.phy_addr[0] = (unsigned int)phy_addr;
		mvideo_frame.phy_addr[1] = (unsigned int)phy_addr + y_size;
		mbuffer_header.private   = &mvideo_frame;
		comp_empty_this_in_buffer(recoder->venc_comp, &mbuffer_header);

		break;
	}
	case IOCTL_SET_POWER_LINE_FREQ: {
		int power_line_freq = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_POWER_LINE_FREQ, &power_line_freq);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_POWER_LINE_FREQ failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_BRIGHTNESS: {
		int brightness_level = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_BRIGHTNESS, (void *)brightness_level);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_BRIGHTNESS failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_CONTRAST: {
		int contrast_level = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_CONTRAST, (void *)contrast_level);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_CONTRAST failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_SATURATION: {
		int saturation_level = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_SATURATION, (void *)saturation_level);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_SATURATION failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_HUE: {
		int hue_level = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_HUE, (void *)hue_level);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_HUE failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_SHARPNESS: {
		int sharpness_level = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_SHARPNESS, (void *)sharpness_level);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_SHARPNESS failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_SENSOR_NAME: {
		char sensor_name[30];
		memset(sensor_name, 0, sizeof(sensor_name));

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_SENSOR_NAME, (void *)&sensor_name[0]);
		if (ret < 0) {
			RT_LOGE("COMP INDEX_VI_CONFIG_Dynamic_GET_SENSOR_NAME failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &sensor_name, sizeof(sensor_name))) {
			RT_LOGE("IOCTL GET_SENSOR_NAME copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_SENSOR_RESOLUTION: {
		struct sensor_resolution sensor_resolution;

		if (copy_from_user(&sensor_resolution, (void __user *)arg, sizeof(struct sensor_resolution))) {
			RT_LOGE("IOCTL GET_SENSOR_RESOLUTION copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_SENSOR_RESOLUTION, (void *)&sensor_resolution);
		if (ret < 0) {
			RT_LOGE("COMP INDEX_VI_CONFIG_Dynamic_GET_SENSOR_RESOLUTION failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &sensor_resolution, sizeof(struct sensor_resolution))) {
			RT_LOGE("IOCTL GET_SENSOR_RESOLUTION copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_TUNNING_CTL_DATA: {
//		if (sizeof(RTTunningCtlData) != sizeof(struct tunning_ctl_data)) {
//			RT_LOGE("fatal error! rt_media and isp tunning_ctl_data is not same, check code!");
//		}
		struct tunning_ctl_data TunningCtlData;

		if (copy_from_user(&TunningCtlData, (void __user *)arg, sizeof(struct tunning_ctl_data))) {
			RT_LOGE("IOCTL GET_TUNNING_CTL_DATA copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_TUNNING_CTL_DATA, (void *)&TunningCtlData);
		if (ret < 0) {
			RT_LOGE("COMP INDEX_VI_CONFIG_Dynamic_GET_TUNNING_CTL_DATA failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &TunningCtlData, sizeof(struct tunning_ctl_data))) {
			RT_LOGE("IOCTL GET_TUNNING_CTL_DATA copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_FIX_QP: {
//		if (sizeof(RTVencFixQP) != sizeof(VencFixQP)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencFixQP is not same, check code!");
//		}
		VencFixQP fix_qp;

		if (copy_from_user(&fix_qp, (void __user *)arg, sizeof(VencFixQP))) {
			RT_LOGE("IOCTL SET_FIX_QP copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_FIX_QP, (void *)&fix_qp);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_FIX_QP failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_H264_VIDEO_TIMING: {
//		if (sizeof(RTVencH264VideoTiming) != sizeof(VencH264VideoTiming)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencH264VideoTiming is not same, check code!");
//		}
		VencH264VideoTiming st_h264_video_timing;

		if (copy_from_user(&st_h264_video_timing, (void __user *)arg, sizeof(VencH264VideoTiming))) {
			RT_LOGE("IOCTL SET_H264_VIDEO_TIMING copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_H264_TIMING, (void *)&st_h264_video_timing);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_H264_VIDEO_TIMING failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_H265_VIDEO_TIMING: {
//		if (sizeof(RTVencH265TimingS) != sizeof(VencH265TimingS)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencH265TimingS is not same, check code!");
//		}
		VencH265TimingS st_h265_video_timing;

		if (copy_from_user(&st_h265_video_timing, (void __user *)arg, sizeof(VencH265TimingS))) {
			RT_LOGE("IOCTL SET_H265_VIDEO_TIMING copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_H265_TIMING, (void *)&st_h265_video_timing);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_H265_VIDEO_TIMING failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_TDM_BUF: {
		struct vin_isp_tdm_event_status tdm_status;

		if (copy_from_user(&tdm_status, (void __user *)arg, sizeof(struct vin_isp_tdm_event_status))) {
			RT_LOGE("IOCTL GET_TDM_BUF copy_from_user fail\n");
			return -EFAULT;
		}

		rt_tdm_get_buf(&tdm_status);

		if (copy_to_user((void *)arg, &tdm_status, sizeof(struct vin_isp_tdm_event_status))) {
			RT_LOGE("IOCTL GET_TDM_BUF copy_to_user fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_RETURN_TDM_BUF: {
		struct vin_isp_tdm_event_status tdm_status;

		if (copy_from_user(&tdm_status, (void __user *)arg, sizeof(struct vin_isp_tdm_event_status))) {
			RT_LOGE("IOCTL RETURN_TDM_BUF copy_from_user fail\n");
			return -EFAULT;
		}
#if ENABLE_TDM_RAW
		vin_return_tdmbuffer_special(recoder->config.channelId, &tdm_status);
#else
		RT_LOGW("fatal error, vin_return_tdmbuffer_special undefined!");
#endif
		break;
	}
	case IOCTL_GET_TDM_DATA: {
		struct vin_isp_tdm_data st_tdm_data;

		if (copy_from_user(&st_tdm_data, (void __user *)arg, sizeof(struct vin_isp_tdm_data))) {
			RT_LOGE("IOCTL GET_TDM_DATA copy_from_user fail");
			return -EFAULT;
		}

#if ENABLE_TDM_RAW
		vin_get_tdm_data_special(recoder->config.channelId, &st_tdm_data);
#else
		RT_LOGW("fatal error, vin_get_tdm_data_special undefined!");
#endif

		if (copy_to_user((void *)arg, &st_tdm_data, sizeof(struct vin_isp_tdm_data))) {
			RT_LOGE("IOCTL GET_TDM_DATA copy_to_user fail");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_QP_RANGE: {
		VencQPRange qp_range;

		if (copy_from_user(&qp_range, (void __user *)arg, sizeof(VencQPRange))) {
			RT_LOGE("IOCTL SET_QP_RANGE copy_from_user fail");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_QP_RANGE, (void *)&qp_range);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_QP_RANGE failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_MB_RC_MOVE_STATUS: {
		int nMoveStatus = (int)arg;

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_MB_MOVE_STATUS, &nMoveStatus);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_MB_RC_MOVE_STATUS failed, ret = %d, nMoveStatus = %d", ret, nMoveStatus);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_JPEG_QUALITY: {
		int nQuality = (int)arg;

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_JPG_QUALITY, (void *)nQuality);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_JPEG_QUALITY failed, ret = %d, quality = %d", ret, nQuality);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_BITRATE: {
		int bitrate = (int)arg;

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_BITRATE, (void *)bitrate);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_BITRATE failed, ret = %d, bitrate = %d", ret, bitrate);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_FPS: {
		int fps = ((int)arg);

		if (fps <= 0 || fps > recoder->max_fps) {
			RT_LOGW("IOCTL SET_FPS: invalid fps[%d], setup to max_fps[%d]", fps, recoder->max_fps);
			fps = recoder->max_fps;
		}

		RT_LOGI("IOCTL SET_FPS: fps = %d, max_fps = %d, vi_comp = %px, venc_comp = %px",
			fps, recoder->max_fps, recoder->vi_comp, recoder->venc_comp);

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS, (void *)fps);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_FPS failed, ret = %d, fps = %d", ret, fps);
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS, (void *)fps);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_FPS failed, ret = %d, fps = %d", ret, fps);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VBR_PARAM: {
//		if (sizeof(RTVencVbrParam) != sizeof(VencVbrParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencVbrParam is not same, check code!");
//		}
		VencVbrParam rt_vbr_param;

		memset(&rt_vbr_param, 0, sizeof(VencVbrParam));

		if (copy_from_user(&rt_vbr_param, (void __user *)arg, sizeof(VencVbrParam))) {
			RT_LOGE("IOCTL SET_VBR_PARAM copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_VBR_PARAM, (void *)&rt_vbr_param);
		if (ret != 0) {
			RT_LOGE("VENC SET_VBR_PARAM fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_SUM_MB_INFO: {
//		if (sizeof(RTVencMBSumInfo) != sizeof(VencMBSumInfo)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMBSumInfo is not same, check code!");
//		}
		VencMBSumInfo rt_mb_sum;

		memset(&rt_mb_sum, 0, sizeof(VencMBSumInfo));

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_GET_SUM_MB_INFO, (void *)&rt_mb_sum);
		if (ret != 0) {
			RT_LOGE("VENC GET_SUM_MB_INFO fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &rt_mb_sum, sizeof(VencMBSumInfo))) {
			RT_LOGE("IOCTL GET_SUM_MB_INFO copy_to_user fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_ENC_MOTION_SEARCH_PARAM: {
//		if (sizeof(RTVencMotionSearchParam) != sizeof(VencMotionSearchParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMotionSearchParam is not same, check code!");
//		}
		VencMotionSearchParam *pmotion_search_param = &recoder->motion_search_param;
		int total_region_num;

		if (copy_from_user(pmotion_search_param, (void __user *)arg, sizeof(VencMotionSearchParam))) {
			RT_LOGE("IOCTL SET_ENC_MOTION_SEARCH_PARAM copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("motion_search_param hor_region_num %d, ver_region_num %d", recoder->motion_search_param.hor_region_num,
			recoder->motion_search_param.ver_region_num);

		if (0 == recoder->motion_search_param.en_motion_search) {
			if (recoder->motion_region) {
				kfree(recoder->motion_region);
				recoder->motion_region = NULL;
				recoder->motion_region_len = 0;
			}
		} else {
			if (0 == recoder->motion_search_param.dis_default_para) {
				total_region_num = MOTION_TOTAL_REGION_NUM_DEFAULT;
			} else {
				if ((0 == recoder->motion_search_param.hor_region_num) || (0 == recoder->motion_search_param.ver_region_num)) {
					RT_LOGE("IOCTL SET_ENC_MOTION_SEARCH_PARAM invalid params, %d %d\n", recoder->motion_search_param.hor_region_num,
						recoder->motion_search_param.ver_region_num);
					return -EFAULT;
				} else {
					total_region_num = recoder->motion_search_param.hor_region_num * recoder->motion_search_param.ver_region_num;
				}
			}

			recoder->motion_region_len = total_region_num * sizeof(VencMotionSearchRegion);
			RT_LOGI("total_region_num %d, motion_region_len %d", total_region_num, recoder->motion_region_len);

			// free motion_region when disable motion_search or ioctl_destroy
			recoder->motion_region = kmalloc(recoder->motion_region_len, GFP_KERNEL);
			if (recoder->motion_region == NULL) {
				RT_LOGE("IOCTL SET_ENC_MOTION_SEARCH_PARAM: kmalloc fail\n");
				return -EFAULT;
			}
			memset(recoder->motion_region, 0, recoder->motion_region_len);
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM, (void *)pmotion_search_param);
		if (ret != 0) {
			RT_LOGE("VENC COMP INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM fail\n");
			if (recoder->motion_region) {
				kfree(recoder->motion_region);
				recoder->motion_region = NULL;
				recoder->motion_region_len = 0;
			}
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_ENC_MOTION_SEARCH_RESULT: {
//		if (sizeof(RTVencMotionSearchResult) != sizeof(VencMotionSearchResult)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMotionSearchResult is not same, check code!");
//		}
//		if (sizeof(RTVencMotionSearchRegion) != sizeof(VencMotionSearchRegion)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMotionSearchRegion is not same, check code!");
//		}
		VencMotionSearchResult motion_result;
		VencMotionSearchResult motion_result_from_user;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->motion_region == NULL || 0 == recoder->motion_region_len) {
			RT_LOGE("IOCTL GET_ENC_MOTION_SEARCH_RESULT: invalid motion_region\n");
			return -EFAULT;
		}

		memset(&motion_result_from_user, 0, sizeof(VencMotionSearchResult));
		if (copy_from_user(&motion_result_from_user, (void __user *)arg, sizeof(VencMotionSearchResult))) {
			RT_LOGE("motion_result_from_user copy_from_user fail\n");
			return -EFAULT;
		}

		memset(&motion_result, 0, sizeof(VencMotionSearchResult));
		motion_result.region = recoder->motion_region;

		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT, (void *)&motion_result);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)motion_result_from_user.region, motion_result.region, recoder->motion_region_len)) {
			RT_LOGE("region copy_to_user fail\n");
			return -EFAULT;
		}
		motion_result.region = motion_result_from_user.region;
		if (copy_to_user((void *)arg, &motion_result, sizeof(VencMotionSearchResult))) {
			RT_LOGE("motion_result copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VENC_SUPER_FRAME_PARAM: {
//		if (sizeof(RTVencSuperFrameConfig) != sizeof(VencSuperFrameConfig)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencSuperFrameConfig is not same, check code!");
//		}
		VencSuperFrameConfig super_frame_config;

		memset(&super_frame_config, 0, sizeof(VencSuperFrameConfig));

		if (copy_from_user(&super_frame_config, (void __user *)arg, sizeof(VencSuperFrameConfig))) {
			RT_LOGE("IOCTL SET_VENC_SUPER_FRAME_PARAM copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_SUPER_FRAME_PARAM,
			(void *)&super_frame_config);
		if (ret != 0) {
			RT_LOGE("VENC SET_SUPER_FRAME_PARAM fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_SHARP: {
		int bSharp = (int)arg;
		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_SHARP, (void *)&bSharp);
		if (ret != 0) {
			RT_LOGE("VENC COMP INDEX_VENC_CONFIG_SET_SHARP fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_CSI_STATUS: {
		//not buildin mode, such ko mode, this is invalid
		int i = 0;
		for (; i < VIDEO_INPUT_CHANNEL_NUM; i++) {
		#if defined CONFIG_VIDEO_RT_MEDIA//build in mode
			if (g_csi_status_pair[i].bset) {
				rt_media_devp->bcsi_status_set[i] = g_csi_status_pair[i].status;
			}
		#else//not for real value, only compatibility
			rt_media_devp->bcsi_status_set[i] = 1;
		#endif
		}
		if (copy_to_user((void *)arg, rt_media_devp->bcsi_status_set, sizeof(int) * VIDEO_INPUT_CHANNEL_NUM)) {
			RT_LOGE("IOCTL GET_CSI_STATUS copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_ROI: {
//		if (sizeof(RTVencROIConfig) != sizeof(VencROIConfig)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencROIConfig is not same, check code!");
//		}
		VencROIConfig sRoiConfig[MAX_ROI_AREA];

		memset(sRoiConfig, 0, sizeof(VencROIConfig) * MAX_ROI_AREA);

		if (copy_from_user(sRoiConfig, (void __user *)arg, sizeof(VencROIConfig) * MAX_ROI_AREA)) {
			RT_LOGE("IOCTL SET_ROI copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp) {
			for (i = 0; i < MAX_ROI_AREA; i++) {
				comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_ROI, &sRoiConfig[i]);
			}
		}
		break;
	}
	case IOCTL_SET_GDC: {
//		if (sizeof(RTsGdcParam) != sizeof(sGdcParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc sGdcParam is not same, check code!");
//		}
		sGdcParam sGdcConfig;

		memset(&sGdcConfig, 0, sizeof(sGdcParam));

		if (copy_from_user(&sGdcConfig, (void __user *)arg, sizeof(sGdcParam))) {
			RT_LOGE("IOCTL SET_GDC copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("eMountMode %d peak_weights_strength %d peak_m %d %d", sGdcConfig.eMountMode, sGdcConfig.peak_weights_strength,
			sGdcConfig.peak_m, sGdcConfig.th_strong_edge);
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_GDC, &sGdcConfig);
		break;
	}
	case IOCTL_SET_ROTATE: {
		unsigned int rotate_angle = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_ROTATE, &rotate_angle);
		break;
	}
	case IOCTL_SET_REC_REF_LBC_MODE: {
		eVeLbcMode rec_lbc_mode = (eVeLbcMode)arg;
		RT_LOGD("rec_lbc_mode %d\n", rec_lbc_mode);

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_REC_REF_LBC_MODE, &rec_lbc_mode);
		break;
	}
	case IOCTL_SET_WEAK_TEXT_TH: {
		RTVenWeakTextTh WeakTextTh;

		memset(&WeakTextTh, 0, sizeof(RTVenWeakTextTh));

		if (copy_from_user(&WeakTextTh, (void __user *)arg, sizeof(RTVenWeakTextTh))) {
			RT_LOGE("IOCTL SET_WEAK_TEXT_TH copy_from_user fail\n");
			return -EFAULT;
		}

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		RT_LOGD("en_weak_text_th:%d, weak_text_th:%d%%", WeakTextTh.en_weak_text_th, (int)WeakTextTh.weak_text_th);

		if (recoder->venc_comp && WeakTextTh.en_weak_text_th) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_WEAK_TEXT_TH, (void *)&WeakTextTh.weak_text_th);
			if (ret != 0) {
				RT_LOGE("COMP INDEX_VENC_CONFIG_SET_WEAK_TEXT_TH fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	/*
	case IOCTL_SET_REGION_D3D_PARAM: {
		RTVencRegionD3DParam *pregion_d3d_param = &recoder->region_d3d_param;
		int total_region_num;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (copy_from_user(pregion_d3d_param, (void __user *)arg, sizeof(RTVencRegionD3DParam))) {
			RT_LOGE("IOCTL SET_REGION_D3D_PARAM copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("RegionD3D result_num:%d, Region:%dx%d, Expand:%dx%d, Coef:%d,{%d,%d,%d},{%d,%d,%d,%d}, Th:{%d%%,%d%%,%d%%}\n",
			pregion_d3d_param->result_num,
			pregion_d3d_param->hor_region_num,
			pregion_d3d_param->ver_region_num,
			pregion_d3d_param->hor_expand_num,
			pregion_d3d_param->ver_expand_num,
			pregion_d3d_param->chroma_offset,
			pregion_d3d_param->static_coef[0],
			pregion_d3d_param->static_coef[1],
			pregion_d3d_param->static_coef[2],
			pregion_d3d_param->motion_coef[0],
			pregion_d3d_param->motion_coef[1],
			pregion_d3d_param->motion_coef[2],
			pregion_d3d_param->motion_coef[3],
			(int)pregion_d3d_param->zero_mv_rate_th[0],
			(int)pregion_d3d_param->zero_mv_rate_th[1],
			(int)pregion_d3d_param->zero_mv_rate_th[2]);

		if (recoder->region_d3d_param.en_region_d3d == 0) {
			if (recoder->region_d3d_region) {
				kfree(recoder->region_d3d_region);
				recoder->region_d3d_region = NULL;
				recoder->region_d3d_region_len = 0;
			}
		} else {
			if (recoder->region_d3d_param.dis_default_para == 0) {
				total_region_num = REGION_D3D_TOTAL_REGION_NUM_DEFAULT;
			} else {
				if ((recoder->region_d3d_param.hor_region_num == 0) || (recoder->region_d3d_param.ver_region_num == 0)) {
					RT_LOGE("IOCTL SET_REGION_D3D_PARAM invalid params, %d %d\n", recoder->region_d3d_param.hor_region_num,
						recoder->region_d3d_param.ver_region_num);
					return -EFAULT;
				} else {
					total_region_num = recoder->region_d3d_param.hor_region_num * recoder->region_d3d_param.ver_region_num;
				}
			}

			recoder->region_d3d_region_len = total_region_num * sizeof(RTVencRegionD3DRegion);
			RT_LOGI("total_region_num %d, region_d3d_region_len %d", total_region_num, recoder->region_d3d_region_len);

			// free region_d3d_region when disable region_d3d or ioctl_destroy
			recoder->region_d3d_region = kmalloc(recoder->region_d3d_region_len, GFP_KERNEL);
			if (recoder->region_d3d_region == NULL) {
				RT_LOGE("IOCTL SET_REGION_D3D_PARAM: kmalloc fail\n");
				return -EFAULT;
			}
			memset(recoder->region_d3d_region, 0, recoder->region_d3d_region_len);
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM, (void *)pregion_d3d_param);
		if (ret != 0) {
			RT_LOGE("VENC_COMP INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM fail\n");
			if (recoder->region_d3d_region) {
				kfree(recoder->region_d3d_region);
				recoder->region_d3d_region = NULL;
				recoder->region_d3d_region_len = 0;
			}
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_REGION_D3D_RESULT: {
		RTVencRegionD3DResult region_d3d_result;
		RTVencRegionD3DResult region_d3d_result_from_user;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->region_d3d_region == NULL || 0 == recoder->region_d3d_region_len) {
			RT_LOGE("IOCTL GET_REGION_D3D_RESULT: invalid region_d3d_region\n");
			return -EFAULT;
		}

		memset(&region_d3d_result_from_user, 0, sizeof(RTVencRegionD3DResult));
		if (copy_from_user(&region_d3d_result_from_user, (void __user *)arg, sizeof(RTVencRegionD3DResult))) {
			RT_LOGE("region_d3d_result_from_user copy_from_user fail\n");
			return -EFAULT;
		}

		// memset(&region_d3d_result, 0, sizeof(RTVencRegionD3DResult));
		// region_d3d_result.region = recoder->region_d3d_region;

		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT, (void *)&region_d3d_result);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)region_d3d_result_from_user.region, region_d3d_result.region, recoder->region_d3d_region_len)) {
			RT_LOGE("region copy_to_user fail\n");
			return -EFAULT;
		}
		region_d3d_result.region = region_d3d_result_from_user.region;
		if (copy_to_user((void *)arg, &region_d3d_result, sizeof(RTVencRegionD3DResult))) {
			RT_LOGE("region_d3d_result copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	*/
	case IOCTL_SET_CHROMA_QP_OFFSET: {
		int nChromaQPOffset = (int)arg;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}
		RT_LOGD("nChromaQPOffset %d\n", nChromaQPOffset);

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_CHROMA_QP_OFFSET, &nChromaQPOffset);
		break;
	}
	case IOCTL_SET_ENC_AND_DEC_CASE: {
		int bEncAndDecCase = (int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_ENC_AND_DEC_CASE, &bEncAndDecCase);
		break;
	}
	case IOCTL_SET_P_SKIP: {
		unsigned int bpskip = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_P_SKIP, &bpskip);
		break;
	}
	case IOCTL_SET_NULL_FRAME: {
		unsigned int b_en = (unsigned int)arg;
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_NULL_FRAME, &b_en);
		break;
	}
	case IOCTL_SET_H264_CONSTRAINT_FLAG: {
//		if (sizeof(RTVencH264ConstraintFlag) != sizeof(VencH264ConstraintFlag)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencH264ConstraintFlag is not same, check code!");
//		}
		VencH264ConstraintFlag ConstraintFlag;

		if (recoder->config.encodeType != 0) {
			RT_LOGW("only support h.264, return\n");
			return -1;
		}

		memset(&ConstraintFlag, 0, sizeof(VencH264ConstraintFlag));

		if (copy_from_user(&ConstraintFlag, (void __user *)arg, sizeof(VencH264ConstraintFlag))) {
			RT_LOGE("IOCTL SET_H264_CONSTRAINT_FLAG copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("constraint:%d %d %d  %d %d %d", ConstraintFlag.constraint_0, ConstraintFlag.constraint_1,
			ConstraintFlag.constraint_2, ConstraintFlag.constraint_3, ConstraintFlag.constraint_4, ConstraintFlag.constraint_5);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG, (void *)&ConstraintFlag);
			if (ret != 0) {
				RT_LOGE("COMP INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_VE2ISP_D2D_LIMIT: {
//		if (sizeof(RTVencVe2IspD2DLimit) != sizeof(VencVe2IspD2DLimit)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencVe2IspD2DLimit is not same, check code!");
//		}
		VencVe2IspD2DLimit D2DLimit;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		memset(&D2DLimit, 0, sizeof(VencVe2IspD2DLimit));

		if (copy_from_user(&D2DLimit, (void __user *)arg, sizeof(VencVe2IspD2DLimit))) {
			RT_LOGE("IOCTL SET_VE2ISP_D2D_LIMIT copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("en_d2d_limit:%d, d2d_level:%d %d %d %d %d %d", D2DLimit.en_d2d_limit,
			D2DLimit.d2d_level[0], D2DLimit.d2d_level[1], D2DLimit.d2d_level[2],
			D2DLimit.d2d_level[3], D2DLimit.d2d_level[4], D2DLimit.d2d_level[5]);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT, (void *)&D2DLimit);
			if (ret != 0) {
				RT_LOGE("COMP INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_EN_SMALL_SEARCH_RANGE: {
		int nEnSmallSearchRange = (int)arg;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		RT_LOGD("nEnSmallSearchRange:%d\n", nEnSmallSearchRange);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_EN_SMALL_SEARCH_RANGE, (void *)&nEnSmallSearchRange);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_EN_SMALL_SEARCH_RANGE fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_FORCE_CONF_WIN: {
//		if (sizeof(RTVencForceConfWin) != sizeof(VencForceConfWin)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencForceConfWin is not same, check code!");
//		}
		VencForceConfWin ConfWin;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		memset(&ConfWin, 0, sizeof(VencForceConfWin));

		if (copy_from_user(&ConfWin, (void __user *)arg, sizeof(VencForceConfWin))) {
			RT_LOGE("IOCTL SET_FORCE_CONF_WIN copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("en_force_conf:%d, %d %d, %d %d\n", ConfWin.en_force_conf,
			ConfWin.top_offset, ConfWin.bottom_offset, ConfWin.left_offset, ConfWin.right_offset);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_FORCE_CONF_WIN, (void *)&ConfWin);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_FORCE_CONF_WIN fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_ROT_VE2ISP: {
//		if (sizeof(RTVencRotVe2Isp) != sizeof(VencRotVe2Isp)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencRotVe2Isp is not same, check code!");
//		}
		VencRotVe2Isp RotVe2Isp;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		memset(&RotVe2Isp, 0, sizeof(VencRotVe2Isp));

		if (copy_from_user(&RotVe2Isp, (void __user *)arg, sizeof(VencRotVe2Isp))) {
			RT_LOGE("IOCTL SET_ROT_VE2ISP copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("d2d %d lv_th[%d,%d] lv_level[%d,%d], d3d %d lv_th[%d,%d] lv_level[%d,%d]\n", RotVe2Isp.dis_default_d2d,
			RotVe2Isp.d2d_min_lv_th, RotVe2Isp.d2d_max_lv_th, RotVe2Isp.d2d_min_lv_level, RotVe2Isp.d2d_max_lv_level,
			RotVe2Isp.dis_default_d3d, RotVe2Isp.d3d_min_lv_th, RotVe2Isp.d3d_max_lv_th, RotVe2Isp.d3d_min_lv_level,
			RotVe2Isp.d3d_max_lv_level);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_ROT_VE2ISP, (void *)&RotVe2Isp);
			if (ret != 0) {
				RT_LOGE("COMP INDEX_VENC_CONFIG_SET_ROT_VE2ISP fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_INSERT_DATA: {
//		if (sizeof(RTVencInsertData) != sizeof(VencInsertData)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencInsertData is not same, check code!");
//		}
		VencInsertData InsertData;
		unsigned int insert_data_buf_len = 0;

		if (recoder->config.encodeType != 1) {
			RT_LOGW("only support jpg, type %d return\n", recoder->config.encodeType);
			return -1;
		}

		memset(&InsertData, 0, sizeof(VencInsertData));

		if (copy_from_user(&InsertData, (void __user *)arg, sizeof(VencInsertData))) {
			RT_LOGE("IOCTL SET_INSERT_DATA copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("set InsertData status:%d, buf:%px, data_len:%d, framerate:%d\n", InsertData.eStatus,
			InsertData.pBuffer, InsertData.nDataLen, InsertData.nFrameRate);

		if (NULL == InsertData.pBuffer || 0 == InsertData.nDataLen) {
			RT_LOGE("invalid input params! %px,%d\n", InsertData.pBuffer, InsertData.nDataLen);
			return -EFAULT;
		}

		if (0 < InsertData.nBufLen) {
			insert_data_buf_len = InsertData.nBufLen;
		} else {
			insert_data_buf_len = RT_MAX_INSERT_FRAME_LEN;
		}

		if (insert_data_buf_len < InsertData.nDataLen) {
			RT_LOGE("invalid input params! data_len %d > %d\n", InsertData.nDataLen, insert_data_buf_len);
			return -EFAULT;
		}

		if (NULL == recoder->insert_data.pBuffer) {
			recoder->insert_data.pBuffer = kmalloc(insert_data_buf_len, GFP_KERNEL);
			if (NULL == recoder->insert_data.pBuffer) {
				RT_LOGE("kmalloc failed! size=%d\n", InsertData.nDataLen);
				return -EFAULT;
			}
			memset(recoder->insert_data.pBuffer, 0, insert_data_buf_len);
		}
		if (copy_from_user(recoder->insert_data.pBuffer, (void __user *)InsertData.pBuffer, InsertData.nDataLen)) {
			RT_LOGE("IOCTL SET_INSERT_DATA copy_from_user fail\n");
			if (recoder->insert_data.pBuffer) {
				kfree(recoder->insert_data.pBuffer);
				recoder->insert_data.pBuffer = NULL;
				recoder->insert_data.nDataLen = 0;
				recoder->insert_data.nBufLen = 0;
			}
			return -EFAULT;
		}
		recoder->insert_data.nBufLen = insert_data_buf_len;
		recoder->insert_data.nDataLen = InsertData.nDataLen;
		recoder->insert_data.eStatus = InsertData.eStatus;
		recoder->insert_data.nFrameRate = InsertData.nFrameRate;

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_INSERT_DATA, (void *)&recoder->insert_data);
			if (ret != 0) {
				RT_LOGE("COMP INDEX_VENC_CONFIG_SET_INSERT_DATA fail\n");
				if (recoder->insert_data.pBuffer) {
					kfree(recoder->insert_data.pBuffer);
					recoder->insert_data.pBuffer = NULL;
					recoder->insert_data.nDataLen = 0;
					recoder->insert_data.nBufLen = 0;
				}
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_GET_INSERT_DATA_BUF_STATUS: {
//		if (sizeof(RT_VENC_BUF_STATUS) != sizeof(VENC_BUF_STATUS)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VENC_BUF_STATUS is not same, check code!");
//		}
		VENC_BUF_STATUS eStatus;

		if (recoder->config.encodeType != 1) {
			RT_LOGW("only support jpg, type %d return\n", recoder->config.encodeType);
			return -1;
		}

		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_INSERT_DATA_BUF_STATUS, (void *)&eStatus);
		if (ret != 0) {
			RT_LOGE("VENC COMP INDEX_VENC_CONFIG_GET_INSERT_DATA_BUF_STATUS fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &eStatus, sizeof(VENC_BUF_STATUS))) {
			RT_LOGE("IOCTL GET_INSERT_DATA_BUF_STATUS copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_GRAY: {
		unsigned int enable_gray = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_GRAY, &enable_gray);
		break;
	}
	case IOCTL_SET_WB_YUV: {
//		if (sizeof(RTsWbYuvParam) != sizeof(sWbYuvParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc sWbYuvParam is not same, check code!");
//		}
		sWbYuvParam sWbyuvConfig;

		memset(&sWbyuvConfig, 0, sizeof(sWbYuvParam));

		if (copy_from_user(&sWbyuvConfig, (void __user *)arg, sizeof(sWbYuvParam))) {
			RT_LOGE("IOCTL SET_WB_YUV copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_WBYUV, &sWbyuvConfig);
		RT_LOGD("bEnableWbYuv %d bEnableCrop %d scalerRatio %d", sWbyuvConfig.bEnableWbYuv, sWbyuvConfig.bEnableCrop,
			sWbyuvConfig.scalerRatio);
		break;
	}
	case IOCTL_GET_WB_YUV: {
		RTVencThumbInfo s_wbyuv_info;

		memset(&s_wbyuv_info, 0, sizeof(RTVencThumbInfo));

		if (copy_from_user(&s_wbyuv_info, (void __user *)arg, sizeof(RTVencThumbInfo))) {
			RT_LOGE("IOCTL GET_WB_YUV copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("nThumbSize %d %px", s_wbyuv_info.nThumbSize, s_wbyuv_info.pThumbBuf);
		if (recoder->venc_comp) {
			if (comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_WBYUV, &s_wbyuv_info))
				return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_2DNR: {
//		if (sizeof(RTs2DfilterParam) != sizeof(s2DfilterParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc s2DfilterParam is not same, check code!");
//		}
		s2DfilterParam s_2dnr_param;

		memset(&s_2dnr_param, 0, sizeof(s2DfilterParam));

		if (copy_from_user(&s_2dnr_param, (void __user *)arg, sizeof(s2DfilterParam))) {
			RT_LOGE("IOCTL SET_2DNR copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_2DNR, (void *)&s_2dnr_param);
		break;
	}
	case IOCTL_SET_3DNR: {
		int b_en = (int)arg;
		RT_LOGW("b_en %d", b_en);
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_3DNR, &b_en);
		break;
	}
	case IOCTL_SET_CYCLIC_INTRA_REFRESH: {
//		if (sizeof(RTVencCyclicIntraRefresh) != sizeof(VencCyclicIntraRefresh)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencCyclicIntraRefresh is not same, check code!");
//		}
		VencCyclicIntraRefresh s_intra_refresh;

		memset(&s_intra_refresh, 0, sizeof(VencCyclicIntraRefresh));

		if (copy_from_user(&s_intra_refresh, (void __user *)arg, sizeof(VencCyclicIntraRefresh))) {
			RT_LOGE("IOCTL SET_CYCLIC_INTRA_REFRESH copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_CYCLE_INTRA_REFRESH, &s_intra_refresh);
		break;
	}
	case IOCTL_SET_P_FRAME_INTRA: {
		unsigned int enable_p_intra = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_P_FRAME_INTRA, &enable_p_intra);
		break;
	}
	case IOCTL_RESET_IN_OUT_BUFFER: {
		comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_FLUSH_IN_BUFFER, NULL);
		flush_stream_buff(recoder);
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_RESET_VE_INOUT_BUFFER, NULL);
		break;
	}
	case IOCTL_GET_SENSOR_RESERVE_ADDR: {
		SENSOR_ISP_CONFIG_S *p_sensor_isp_config;
		p_sensor_isp_config = vmalloc(sizeof(SENSOR_ISP_CONFIG_S));
		memset(p_sensor_isp_config, 0, sizeof(SENSOR_ISP_CONFIG_S));
		if (recoder->vi_comp)
			comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_GET_SENSOR_RESERVE_ADDR, p_sensor_isp_config);
		if (copy_to_user((void *)arg, p_sensor_isp_config, sizeof(SENSOR_ISP_CONFIG_S))) {
			RT_LOGE("IOCTL GET_SENSOR_RESERVE_ADDR copy_to_user fail\n");
			vfree(p_sensor_isp_config);
			return -EFAULT;
		}
		vfree(p_sensor_isp_config);
		break;
	}
	case IOCTL_DROP_FRAME: {
		int nDropNum = (int)arg;
		if (recoder->venc_comp) {
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_DROP_FRAME, &nDropNum);
		}
		break;
	}
	case IOCTL_SET_CAMERA_MOVE_STATUS: {
//		if (sizeof(RT_VENC_CAMERA_MOVE_STATUS) != sizeof(eCameraStatus)) {
//			RT_LOGE("fatal error! rt_media and libcedarc eCameraStatus is not same, check code!");
//		}
		eCameraStatus camera_move_status = (eCameraStatus)arg;
		if (recoder->venc_comp) {
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CAMERA_MOVE_STATUS, &camera_move_status);
		}
		break;
	}
	case IOCTL_SET_CROP: {
//		if (sizeof(RTCropInfo) != sizeof(VencCropCfg)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencCropCfg is not same, check code!");
//		}
		VencCropCfg s_crop_info;

		memset(&s_crop_info, 0, sizeof(VencCropCfg));

		if (copy_from_user(&s_crop_info, (void __user *)arg, sizeof(VencCropCfg))) {
			RT_LOGE("IOCTL SET_CROP copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_CROP, &s_crop_info);
		break;
	}
	case IOCTL_SET_REGION_DETECT_LINK: {
//		if (sizeof(sRegionLinkParam) != sizeof(RTsRegionLinkParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc sRegionLinkParam size is not same, check code!");
//		}
		sRegionLinkParam s_region_link;

		memset(&s_region_link, 0, sizeof(sRegionLinkParam));

		if (copy_from_user(&s_region_link, (void __user *)arg, sizeof(sRegionLinkParam))) {
			RT_LOGE("IOCTL SET_REGION_DETECT_LINK copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_REGION_DETECT_LINK, &s_region_link);
		break;
	}
	case IOCTL_GET_REGION_DETECT_LINK: {
//		if (sizeof(sRegionLinkParam) != sizeof(RTsRegionLinkParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc sRegionLinkParam size is not same, check code!");
//		}
		sRegionLinkParam s_region_link;

		memset(&s_region_link, 0, sizeof(sRegionLinkParam));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("recoder->venc_comp is null\n");
			return -EFAULT;
		}

		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_REGION_DETECT_LINK, &s_region_link);
		if (ret != ERROR_TYPE_OK) {
			RT_LOGE("get vbv buf info failed");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &s_region_link, sizeof(sRegionLinkParam))) {
			RT_LOGE("copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VENC_TARTGET_BITS_CLIP_PARAM: {
//		if (sizeof(RT_VENC_TARGET_BITS_CLIP_PARAM) != sizeof(VencTargetBitsClipParam)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencTargetBitsClipParam size is not same, check code!");
//		}
		VencTargetBitsClipParam rt_venc_target_bits_clip_param;
		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}
		memset(&rt_venc_target_bits_clip_param, 0, sizeof(VencTargetBitsClipParam));
		if (copy_from_user(&rt_venc_target_bits_clip_param, (void __user *)arg, sizeof(VencTargetBitsClipParam))) {
			RT_LOGE("venc_target_bits_clip_param copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_TARGET_BITS_CLIP_PARAM, (void *)&rt_venc_target_bits_clip_param);
				if (ret != 0) {
					RT_LOGE("VENC COMP INDEX_VENC_CONFIG_TARGET_BITS_CLIP_PARAM fail\n");
					return -EFAULT;
				}
		}
		break;
	}
	case IOCTL_SET_MB_MAP_PARAM: {
//		if (sizeof(RTVencMBModeCtrl) != sizeof(VencMBModeCtrl)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMBModeCtrl size is not same, check code!");
//		}
//		if (sizeof(RTVencMbModeLambdaQpMap) != sizeof(VencMbModeLambdaQpMap)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMbModeLambdaQpMap size is not same, check code!");
//		}
//		if (sizeof(RTVencMbSplitMap) != sizeof(VencMbSplitMap)) {
//			RT_LOGE("fatal error! rt_media and libcedarc VencMbSplitMap size is not same, check code!");
//		}
		RTVencMbMapParam mb_map_param;
		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}
		memset(&mb_map_param, 0, sizeof(RTVencMbMapParam));
		if (copy_from_user(&mb_map_param, (void __user *)arg, sizeof(RTVencMbMapParam))) {
			RT_LOGE("copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_MB_MAP_PARAM, (void *)&mb_map_param);
			if (ret != 0) {
				RT_LOGE("VENC COMP INDEX_VENC_CONFIG_MB_MAP_PARAM fail\n");
				return -EFAULT;
			}
		}
		break;
	}
	case IOCTL_GET_ISP_REG: {
		//Copy VIN_ISP_REG_GET_CFG struct param from user space
		VIN_ISP_REG_GET_CFG isp_reg_get_cfg;
		memset(&isp_reg_get_cfg, 0, sizeof(isp_reg_get_cfg));
		if (copy_from_user(&isp_reg_get_cfg, (void __user *)arg, sizeof(isp_reg_get_cfg))) {
			RT_LOGE("IOCTL GET_ISP_REG copy_from_user fail\n");
			return -EFAULT;
		}

		if (isp_reg_get_cfg.flag == 0) {
			//Call VI component interface to get isp reg
			ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_GET_ISP_REG_CONFIG, (void *)&isp_reg_get_cfg);
		} else if (isp_reg_get_cfg.flag == 1) {
			//Copy isp reg save file path from user space
			//char save_file_path[isp_reg_get_cfg.len + 1];
			char *save_file_path = (char *)vmalloc(isp_reg_get_cfg.len + 1);
			memset(save_file_path, 0, isp_reg_get_cfg.len + 1);
			if (copy_from_user(save_file_path, (void __user *)isp_reg_get_cfg.path, isp_reg_get_cfg.len)) {
				RT_LOGE("IOCTL GET_ISP_REG isp reg save file path copy_from_user fail\n");
				vfree(save_file_path);
				return -EFAULT;
			}

			//Call VI component interface to get isp reg
			isp_reg_get_cfg.path = save_file_path;
			ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_GET_ISP_REG_CONFIG, (void *)&isp_reg_get_cfg);
			vfree(save_file_path);
		}

		//ioctl error
		if (ret < 0) {
			RT_LOGE("IOCTL GET_ISP_REG failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_TDM_DROP_FRAME: {
		unsigned int drop_num;
		if (copy_from_user(&drop_num, (void __user *)arg, sizeof(drop_num))) {
			RT_LOGE("IOCTL SET_TDM_DROP_FRAME copy_from_user fail");
			return -EFAULT;
		}
		RT_LOGI("IOCTL SET_TDM_DROP_FRAME drop_num = %d", drop_num);
		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_TDM_DROP_FRAME, (void *)&drop_num);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_TDM_DROP_FRAME failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_INPUT_BIT_WIDTH_START: {
		RT_VIN_SET_BIT_WIDTH bit_width;
		if (copy_from_user(&bit_width, (void __user *)arg, sizeof(bit_width))) {
			RT_LOGE("IOCTL SET_INPUT_BIT_WIDTH_START copy_from_user fail");
			return -EFAULT;
		}
		RT_LOGI("IOCTL SET_INPUT_BIT_WIDTH_START bit_width = %d", bit_width);
		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_INPUT_BIT_WIDTH_START, (void *)&bit_width);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_INPUT_BIT_WIDTH_START failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_INPUT_BIT_WIDTH_STOP: {
		RT_VIN_SET_BIT_WIDTH bit_width;
		if (copy_from_user(&bit_width, (void __user *)arg, sizeof(bit_width))) {
			RT_LOGE("IOCTL SET_INPUT_BIT_WIDTH_STOP copy_from_user fail");
			return -EFAULT;
		}
		RT_LOGI("IOCTL SET_INPUT_BIT_WIDTH_STOP bit_width = %d", bit_width);
		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_INPUT_BIT_WIDTH_STOP, (void *)&bit_width);
		if (ret < 0) {
			RT_LOGE("IOCTL SET_INPUT_BIT_WIDTH_STOP failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VENC_SEI_ATTR: {
		RT_VENC_SEI_ATTR stSEIAttr;
		memset(&stSEIAttr, 0, sizeof(RT_VENC_SEI_ATTR));
		if (copy_from_user(&stSEIAttr, (void __user *)arg, sizeof(RT_VENC_SEI_ATTR))) {
			RT_LOGE("IOCTL SET_VENC_SEI_ATTR copy_from_user fail");
			return -EFAULT;
		}
		error_type eError;
		if (recoder->venc_comp) {
			eError = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SEI, &stSEIAttr);
			if (eError != ERROR_TYPE_OK) {
				RT_LOGE("fatal error! rtChn[%d] venc comp sei fail:%d", recoder->config.channelId, eError);
				return -EFAULT;
			}
		} else {
			RT_LOGE("fatal error! venc comp NULL?");
			return -ENODEV;
		}
		break;
	}
	case IOCTL_SET_VBR_OPT_PARAM: {
		VencVbrOptParam stVbrOptParam;
		memset(&stVbrOptParam, 0, sizeof(VencVbrOptParam));
		if (copy_from_user(&stVbrOptParam, (void __user *)arg, sizeof(VencVbrOptParam))) {
			RT_LOGE("IOCTL SET_VBR_OPT_PARAM copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_VBR_OPT_PARAM, &stVbrOptParam);
		break;
	}
	case IOCTL_GET_VBR_OPT_PARAM: {
		VencVbrOptParam stVbrOptParam;
		memset(&stVbrOptParam, 0, sizeof(VencVbrOptParam));
		if (copy_from_user(&stVbrOptParam, (void __user *)arg, sizeof(VencVbrOptParam))) {
			RT_LOGE("IOCTL GET_VBR_OPT_PARAM copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp) {
			if (comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_VBR_OPT_PARAM, &stVbrOptParam))
				return -EFAULT;
		}
		if (copy_to_user((void *)arg, &stVbrOptParam, sizeof(VencVbrOptParam))) {
			RT_LOGE("IOCTL GET_VBR_OPT_PARAM copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	default: {
		RT_LOGE("fatal error! not support cmd: 0x%x", cmd);
		result = -EPERM;
		break;
	}
	}

	return result;
}

static const struct file_operations rt_media_fops = {
	.owner		= THIS_MODULE,
	.mmap		= fops_mmap,
	.open		= fops_open,
	.release	= fops_release,
	.llseek		= no_llseek,
	.unlocked_ioctl = fops_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fops_ioctl,
#endif
};

#if defined(CONFIG_OF)
static struct of_device_id rt_media_match[] = {
	{
		.compatible = "allwinner,rt-media",
	},
	{}
};
MODULE_DEVICE_TABLE(of, rt_media_match);
#endif

#ifdef CONFIG_PM
static int rt_media_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* todo */
	RT_LOGD("[cedar] standby suspend\n");

	return 0;
}

static int rt_media_resume(struct platform_device *pdev)
{
	/* todo */
	RT_LOGD("[cedar] standby resume\n");
	return 0;
}
#endif

static int rt_is_sensor_0(int vipp_num)
{
	if (vipp_num == CSI_SENSOR_0_VIPP_0
		|| vipp_num == CSI_SENSOR_0_VIPP_1
		|| vipp_num == CSI_SENSOR_0_VIPP_2
		|| vipp_num == CSI_SENSOR_0_VIPP_3
	)
		return 1;

	return 0;
}

static int rt_is_sensor_1(int vipp_num)
{
	if (vipp_num == CSI_SENSOR_1_VIPP_0
		|| vipp_num == CSI_SENSOR_1_VIPP_1
		|| vipp_num == CSI_SENSOR_1_VIPP_2
		|| vipp_num == CSI_SENSOR_1_VIPP_3
	)
		return 1;

	return 0;
}

static int rt_is_sensor_2(int vipp_num)
{
	if (vipp_num == CSI_SENSOR_2_VIPP_0
		|| vipp_num == CSI_SENSOR_2_VIPP_1
		|| vipp_num == CSI_SENSOR_2_VIPP_2
		|| vipp_num == CSI_SENSOR_2_VIPP_3
	)
		return 1;

	return 0;
}

static int setup_recoder_config(int channel_id)
{
	int ret			= 0;
	video_recoder *recoder  = &rt_media_devp->recoder[channel_id];
	rt_media_config_s config;
	struct vi_part_cfg vi_part_cfg;
	encode_config *venc_cfg = &rt_media_devp->enc_conf[rt_which_sensor(channel_id)];

	memset(&config, 0, sizeof(rt_media_config_s));
	config.channelId	 = channel_id;
	config.bonline_channel = venc_cfg->bonline_channel;
	config.share_buf_num = venc_cfg->share_buf_num;
	config.encodeType	= venc_cfg->codec_type;
	config.width  = venc_cfg->res_w;
	config.height = venc_cfg->res_h;
	config.fps		 = venc_cfg->fps;
	config.bitrate		 = venc_cfg->bit_rate*1024;
	config.gop		 = venc_cfg->gop;
	config.product_mode = venc_cfg->product_mode;
	config.qp_range.nQpInit = venc_cfg->qp_range.nQpInit;
	config.qp_range.nMinqp = venc_cfg->qp_range.nMinqp;
	config.qp_range.nMaxqp = venc_cfg->qp_range.nMaxqp;
	config.qp_range.nMinPqp = venc_cfg->qp_range.nMinPqp;
	config.qp_range.nMaxPqp = venc_cfg->qp_range.nMaxPqp;
	config.qp_range.bEnMbQpLimit = venc_cfg->qp_range.bEnMbQpLimit;
	config.vbr_param.nMovingTh = venc_cfg->vbr_param.nMovingTh;
	config.vbr_param.nQuality = venc_cfg->vbr_param.nQuality;
	config.vbr_param.nIFrmBitsCoef = venc_cfg->vbr_param.nIFrmBitsCoef;
	config.vbr_param.nPFrmBitsCoef = venc_cfg->vbr_param.nPFrmBitsCoef;
	ret = wdr_get_from_partition(&vi_part_cfg, rt_which_sensor(channel_id));
	if (!ret) {
		RT_LOGW("get info fram part: w&h = %d, %d, venc_format = %d, wdr = %d, flicker = %d",
			vi_part_cfg.width, vi_part_cfg.height, vi_part_cfg.venc_format,
			vi_part_cfg.wdr, vi_part_cfg.flicker_mode);

		if (vi_part_cfg.width && vi_part_cfg.height) {
			config.width  = vi_part_cfg.width;
			config.height = vi_part_cfg.height;
		}

		if (vi_part_cfg.venc_format == 2)
			config.encodeType = 2;
		else if (vi_part_cfg.venc_format == 1)
			config.encodeType = 0;

		//if (vi_part_cfg.flicker_mode == 1)
		//	config.fps = 16;
		config.fps = vi_part_cfg.fps;
		config.enable_wdr = vi_part_cfg.wdr;
	}
	config.profile	= VENC_H264ProfileMain;
	config.level	  = VENC_H264Level51;
	config.drop_frame_num = 0;
	config.output_mode    = venc_cfg->out_mode;
	config.pixelformat = venc_cfg->pix_fmt;

	config.venc_video_signal.video_format = DEFAULT;
	config.venc_video_signal.full_range_flag = 1;
	config.venc_video_signal.src_colour_primaries = VENC_BT709;
	config.venc_video_signal.dst_colour_primaries = VENC_BT709;
	config.breduce_refrecmem = venc_cfg->reduce_refrec_mem;
	config.enable_sharp = venc_cfg->enable_sharp;
	config.tdm_rxbuf_cnt = venc_cfg->tdm_rxbuf_cnt;
	config.enable_tdm_raw = venc_cfg->enable_tdm_raw;

	ret = ioctl_config(recoder, &config);
	if (ret != 0) {
		RT_LOGE("config err ret %d", ret);
		ioctl_destroy(recoder);
		return -1;
	}

	return ret;
}
static int setup_recoder(void)
{
#if defined CONFIG_RT_MEDIA_SINGEL_SENSOR || defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	int single_channel_id	= CSI_SENSOR_0_VIPP_0;
	video_recoder *recoder_single  = &rt_media_devp->recoder[single_channel_id];
#endif
#if defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	int dual_channel_id		= CSI_SENSOR_1_VIPP_0;
	video_recoder *recoder_dual  = &rt_media_devp->recoder[dual_channel_id];
#endif
#if defined CONFIG_RT_MEDIA_THREE_SENSOR
	int three_channel_id		= CSI_SENSOR_2_VIPP_0;
	video_recoder *recoder_three  = &rt_media_devp->recoder[three_channel_id];
#endif

	RT_LOGW("* start setup_recoder");

#if defined CONFIG_RT_MEDIA_SINGEL_SENSOR || defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	setup_recoder_config(single_channel_id);
	if (recoder_single->bvin_is_ready || bvin_is_ready_rt_not_probe[single_channel_id]) {
		rt_media_devp->bcsi_status_set[single_channel_id] = 1;
		ioctl_start(recoder_single);
	} else {
		recoder_single->bsetup_recorder_finish = 1;
	}
#endif

#if defined CONFIG_RT_MEDIA_DUAL_SENSOR || defined CONFIG_RT_MEDIA_THREE_SENSOR
	setup_recoder_config(dual_channel_id);
	if (recoder_dual->bvin_is_ready || bvin_is_ready_rt_not_probe[dual_channel_id]) {
		rt_media_devp->bcsi_status_set[dual_channel_id] = 1;
		ioctl_start(recoder_dual);
	} else {
		recoder_dual->bsetup_recorder_finish = 1;
	}
#endif

#if defined CONFIG_RT_MEDIA_THREE_SENSOR
	setup_recoder_config(three_channel_id);
	if (recoder_three->bvin_is_ready || bvin_is_ready_rt_not_probe[three_channel_id]) {
		rt_media_devp->bcsi_status_set[three_channel_id] = 1;
		ioctl_start(recoder_three);
	} else {
		recoder_three->bsetup_recorder_finish = 1;
	}
#endif
	RT_LOGW("finish");
	return 0;
}

int rt_vin_is_ready(int channel_id)
{
	int ret = 0;
	RT_LOGD("rt_vin_is_ready vinc%d", channel_id);
	if (rt_media_devp) {
		video_recoder *recoder = &rt_media_devp->recoder[channel_id];
		rt_media_devp->bcsi_status_set[channel_id] = 1;
		if (recoder->bsetup_recorder_finish) {
			ioctl_start(recoder);
		} else {
			RT_LOGW("%d rt_media thread not ready", channel_id);
			recoder->bvin_is_ready = 1;
		}
	} else {
		RT_LOGW("%d rt_media not probe yet", channel_id);
		bvin_is_ready_rt_not_probe[channel_id] = 1;
		g_csi_status_pair[channel_id].bset = 1;
		g_csi_status_pair[channel_id].status = 1;
	}

	return ret;
}
EXPORT_SYMBOL(rt_vin_is_ready);

int rt_vin_buf_info_get(int channel_id, int *buf_num, int *buf_size)
{
	int ret = 0;
	RT_LOGD("rt_vin_buf_info_get vinc%d", channel_id);
	if (rt_media_devp) {
		video_recoder *recoder = &rt_media_devp->recoder[channel_id];
		if (recoder->vi_comp) {
			vi_bkbuf_info bkbuf_info;
			memset(&bkbuf_info, 0, sizeof(vi_bkbuf_info));
			comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_BKBUF_INFO, (void *)&bkbuf_info);
			*buf_num = bkbuf_info.buf_num;
			*buf_size = bkbuf_info.buf_size;
		} else {
			RT_LOGD("%d rt vi_comp is NULL", channel_id);
			*buf_num = 0;
			*buf_size = 0;
		}
	} else {
		RT_LOGD("%d rt_media not probe yet", channel_id);
		*buf_num = 0;
		*buf_size = 0;
	}

	return ret;
}
EXPORT_SYMBOL(rt_vin_buf_info_get);

static int thread_setup(void *param)
{
	setup_recoder();
	RT_LOGW("finish");
	return 0;
}

static int rt_get_venc_config(struct device_node *node, encode_config *config)
{
	int i = 0;
	struct device_node *child;

	for_each_available_child_of_node(node, child) {
		config->name = (char *)of_get_property(child, "name", NULL);
		// if (config->name)
		// 	RT_LOGW("%d Found child node: %s\n", i, config->name);
		config->b_get_from_dts = 1;
		of_property_read_u32(child, "ch_id", &config->ch_id);
		of_property_read_u32(child, "enable_online", &config->bonline_channel);
		of_property_read_u32(child, "online_share_buf_num", &config->share_buf_num);
		of_property_read_u32(child, "codec_type", &config->codec_type);
		of_property_read_u32(child, "res_w", &config->res_w);
		of_property_read_u32(child, "res_h", &config->res_h);
		of_property_read_u32(child, "fps", &config->fps);
		of_property_read_u32(child, "bit_rate", &config->bit_rate);
		of_property_read_u32(child, "gop", &config->gop);
		of_property_read_u32(child, "enable_sharp", &config->enable_sharp);
		of_property_read_u32(child, "product_mode", &config->product_mode);
		of_property_read_u32(child, "vbr", &config->vbr);
		of_property_read_u32(child, "init_qp", &config->qp_range.nQpInit);
		of_property_read_u32(child, "i_min_qp", &config->qp_range.nMinqp);
		of_property_read_u32(child, "i_max_qp", &config->qp_range.nMaxqp);
		of_property_read_u32(child, "p_min_qp", &config->qp_range.nMinPqp);
		of_property_read_u32(child, "p_max_qp", &config->qp_range.nMaxPqp);
		of_property_read_u32(child, "enable_mb_qp_limit", &config->qp_range.bEnMbQpLimit);
		of_property_read_u32(child, "moving_th", &config->vbr_param.nMovingTh);
		of_property_read_u32(child, "quality", &config->vbr_param.nQuality);
		of_property_read_u32(child, "i_frm_bits_coef", &config->vbr_param.nIFrmBitsCoef);
		of_property_read_u32(child, "p_frm_bits_coef", &config->vbr_param.nPFrmBitsCoef);
		of_property_read_u32(child, "out_mode", &config->out_mode);
		of_property_read_u32(child, "pix_fmt", &config->pix_fmt);
		of_property_read_u32(child, "reduce_refrec_mem", &config->reduce_refrec_mem);
		of_property_read_u32(child, "enable_tdm_raw", &config->enable_tdm_raw);
		of_property_read_u32(child, "tdm_rxbuf_cnt", &config->tdm_rxbuf_cnt);
		i++;
	}
	return 0;
}

#if defined CONFIG_VIN_INIT_MELIS
static int rt_media_csi_err(void *dev, void *data, int len)
{
	RT_LOGE("melis rpmsg notify csi is err");
	rt_media_devp->bcsi_err = 1;
	return 0;
}
#endif

static int rt_media_probe(struct platform_device *pdev)
{
	int i   = 0;
	int ret = 0;
	int devno;
	struct sched_param param = {.sched_priority = 1 };

#if defined(CONFIG_OF)
	struct device_node *node;
#endif
	dev_t dev;

	dev = 0;

	RT_LOGW("rt_media install start!!!\n");

#if defined(CONFIG_OF)
	node = pdev->dev.of_node;
#endif
	/*register or alloc the device number.*/
	if (g_rt_media_dev_major) {
		dev = MKDEV(g_rt_media_dev_major, g_rt_media_dev_minor);
		ret = register_chrdev_region(dev, 1, "rt-media");
	} else {
		ret		     = alloc_chrdev_region(&dev, g_rt_media_dev_minor, 1, "rt-media");
		g_rt_media_dev_major = MAJOR(dev);
		g_rt_media_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		RT_LOGW("rt_media_dev: can't get major %d\n", g_rt_media_dev_major);
		return ret;
	}

	rt_media_devp = kmalloc(sizeof(struct rt_media_dev), GFP_KERNEL);
	if (rt_media_devp == NULL) {
		RT_LOGW("malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	memset(rt_media_devp, 0, sizeof(struct rt_media_dev));
	rt_media_devp->platform_dev = &pdev->dev;

	for (i = 0; i < VIDEO_INPUT_CHANNEL_NUM; i++) {
		rt_media_devp->recoder[i].state = RT_MEDIA_STATE_IDLE;
#if !defined CONFIG_VIN_INIT_MELIS
//if e907 not enable, default vin is ready.
		rt_media_devp->bcsi_status_set[i] = 1;
		rt_media_devp->recoder[i].bvin_is_ready = 1;
#endif
	}

#if defined CONFIG_VIN_INIT_MELIS
	rpmsg_notify_add("e907_rproc@0", "rt-media", rt_media_csi_err, rt_media_devp);
#endif

	struct device_node *child;
	int ii = 0;
	for_each_available_child_of_node(node, child) {
		const char *name = of_get_property(child, "name", NULL);
		// if (name)
		// 	RT_LOGW("%d Found child node: %s\n", ii, name);
		if (MAX_SENSOR_NUM > ii)
			rt_get_venc_config(child, &rt_media_devp->enc_conf[ii]);
		ii++;
	}
#if DEBUG_RT_MEDIA_VENC_PARAM
	for (ii = 0; ii < MAX_SENSOR_NUM; ii++) {
		if (rt_media_devp->enc_conf[ii].b_get_from_dts) {
			encode_config *venc_conf = &rt_media_devp->enc_conf[ii];
			if (venc_conf->name)
				RT_LOGW("%s: ", venc_conf->name);
			if (venc_conf->res_w == 0 || venc_conf->res_h == 0)
				RT_LOGW("res_w res_h is 0, will use default param");
			else {
				RT_LOGW("ch_id %d online %d share_buf_num %d codec_type %d res_wh %dx%d fps %d bitrate %d gop %d enable_sharp %d",\
					venc_conf->ch_id, venc_conf->bonline_channel, venc_conf->share_buf_num, venc_conf->codec_type, venc_conf->res_w, venc_conf->res_h, \
					venc_conf->fps, venc_conf->bit_rate, venc_conf->gop, venc_conf->enable_sharp);
				RT_LOGW("ch_id %d out_mode %d pixfmt %d reduce_refrec_mem %d enable_tdm_raw %d tdm_rxbuf_cnt %d",\
					venc_conf->ch_id, venc_conf->out_mode, venc_conf->pix_fmt, venc_conf->reduce_refrec_mem, \
					venc_conf->enable_tdm_raw, venc_conf->tdm_rxbuf_cnt);
				RT_LOGW("ch_id %d vbr %d vbr_opt_enable %d product_mode %d init_qp %d i_min_qp %d i_max_qp %d p_min_qp %d p_max_qp %d",\
					venc_conf->ch_id, venc_conf->vbr, venc_conf->vbr_opt_enable, venc_conf->product_mode, \
					venc_conf->qp_range.i_init_qp, venc_conf->qp_range.i_min_qp, \
					venc_conf->qp_range.i_max_qp, venc_conf->qp_range.p_min_qp, venc_conf->qp_range.p_max_qp);
				RT_LOGW("ch_id %d moving_th %d quality %d i_frm_bits_coef %d p_frm_bits_coef %d\n",\
					venc_conf->ch_id, venc_conf->vbr_param.nMovingTh, \
					venc_conf->vbr_param.nQuality, venc_conf->vbr_param.nIFrmBitsCoef, venc_conf->vbr_param.nPFrmBitsCoef);
			}
		}
	}
#endif
	/* Create char device */
	devno = MKDEV(g_rt_media_dev_major, g_rt_media_dev_minor);
	cdev_init(&rt_media_devp->cdev, &rt_media_fops);
	rt_media_devp->cdev.owner = THIS_MODULE;
	/* rt_media_devp->cdev.ops = &rt_media_fops; */
	ret = cdev_add(&rt_media_devp->cdev, devno, 1);
	if (ret)
		RT_LOGW("Err:%d add cedardev", ret);
	rt_media_devp->class = class_create(THIS_MODULE, "rt-media");
	rt_media_devp->dev   = device_create(rt_media_devp->class, NULL, devno, NULL, "rt-media");

	RT_LOGW("rt_media install end!!!\n");
	RT_LOGD("rt_media_devp %d vi%px ven%px", rt_media_devp->recoder[0].activate_vi_flag, rt_media_devp->recoder[0].vi_comp, rt_media_devp->recoder[0].venc_comp);
#if defined CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL && defined CONFIG_VIDEO_RT_MEDIA
	if (!rt_media_devp->bcsi_err) {
		rt_media_devp->setup_thread = kthread_create(thread_setup, rt_media_devp, "rt_media thread");
		sched_setscheduler(rt_media_devp->setup_thread, SCHED_FIFO, &param);
		wake_up_process(rt_media_devp->setup_thread);
	}
#endif

	return 0;
}
static int rt_media_remove(struct platform_device *pdev)
{
	dev_t dev;

	dev = MKDEV(g_rt_media_dev_major, g_rt_media_dev_minor);

	/* Destroy char device */
	if (rt_media_devp) {
		if (rt_media_devp->setup_thread)
			kthread_stop(rt_media_devp->setup_thread);

		if (rt_media_devp->reset_high_fps_thread)
			kthread_stop(rt_media_devp->reset_high_fps_thread);

		cdev_del(&rt_media_devp->cdev);
		device_destroy(rt_media_devp->class, dev);
		class_destroy(rt_media_devp->class);
	}

	unregister_chrdev_region(dev, 1);

	kfree(rt_media_devp);
	return 0;
}

static struct platform_driver rt_media_driver = {
	.probe  = rt_media_probe,
	.remove = rt_media_remove,
#if defined(CONFIG_PM)
	.suspend = rt_media_suspend,
	.resume  = rt_media_resume,
#endif
	.driver = {
		.name  = "rt-media",
		.owner = THIS_MODULE,

#if defined(CONFIG_OF)
		.of_match_table = rt_media_match,
#endif
	},
};

static int __init rt_media_module_init(void)
{
	rt_media_genetlink_init();
	/*need not to register device here,because the device is registered by device tree */
	/*platform_device_register(&sunxi_device_cedar);*/
	RT_LOGD("rt_media version 0.1\n");
	return platform_driver_register(&rt_media_driver);
}

static void __exit rt_media_module_exit(void)
{
	rt_media_genetlink_exit();
	platform_driver_unregister(&rt_media_driver);
}

#ifdef CONFIG_AW_IPC_FASTBOOT
arch_initcall(rt_media_module_init);
#else
fs_initcall(rt_media_module_init);
#endif
//late_initcall(rt_media_module_init);

module_exit(rt_media_module_exit);

MODULE_AUTHOR("Soft-wxw");
MODULE_DESCRIPTION("User rt-media interface");
MODULE_LICENSE("GPL");
MODULE_VERSION("rt-v1.0");
MODULE_ALIAS("platform:rt-media");

//* todo
