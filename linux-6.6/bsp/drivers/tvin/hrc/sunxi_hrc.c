// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs HRC Driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "sunxi_hrc.h"
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mc.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

MODULE_VERSION("1.0.14");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dengguiling");
MODULE_DESCRIPTION("Allwinner HDMI Rx Capture Driver");

struct hrc_format_info {
	u8  name[32];
	u32 fourcc;
	enum hrc_fmt format;
};

static struct hrc_format_info hrc_support_fmt[] = {
	{
		.name   = "BGR888",
		.fourcc = V4L2_PIX_FMT_BGR24,
		.format = HRC_FMT_RGB,
	}, {
		.name   = "YUV444_1P",
		.fourcc = V4L2_PIX_FMT_YUV24,
		.format = HRC_FMT_YUV444_1PLANE,
	}, {
		/* special format, DE and G2D not support! */
		.name   = "YUV444_NV24",
		.fourcc = V4L2_PIX_FMT_NV24,
		.format = HRC_FMT_YUV444,
	}, {
		.name   = "YUV422_NV16",
		.fourcc = V4L2_PIX_FMT_NV16,
		.format = HRC_FMT_YUV422,
	}, {
		.name   = "YUV420_NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
		.format = HRC_FMT_YUV420,
	},
};

struct hrc_cs_info {
	u8 name[32];
	u8 ycbcr_enc;
	enum hrc_color_space cs;
};

static struct hrc_cs_info hrc_support_cs[] = {
	{
		.name = "BT601",
		.ycbcr_enc = V4L2_YCBCR_ENC_601,
		.cs = HRC_CS_BT601,
	}, {
		.name = "BT709",
		.ycbcr_enc = V4L2_YCBCR_ENC_709,
		.cs = HRC_CS_BT709,
	}, {
		.name = "BT2020",
		.ycbcr_enc = V4L2_YCBCR_ENC_BT2020,
		.cs = HRC_CS_BT2020,
	},
};

struct hrc_quan_info {
	u8 name[32];
	u8 quantization;
	enum hrc_quantization quan;
};

static struct hrc_quan_info hrc_support_quan[] = {
	{
		.name = "FULL",
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.quan = HRC_QUANTIZATION_FULL,
	}, {
		.name = "LIMIT",
		.quantization = V4L2_QUANTIZATION_LIM_RANGE,
		.quan = HRC_QUANTIZATION_LIMIT,
	},
};

struct hrc_source_id_info {
	enum hrc_source_id source_id;
	u32 port_id;
};

static struct hrc_source_id_info hrc_source_id_infos[] = {
	{ hrc_source_HDMI_1, 0 },
	{ hrc_source_HDMI_2, 1 },
	{ hrc_source_HDMI_3, 2 },
	{ hrc_source_HDMI_4, 3 },
};

struct hrc_buffer {
	struct vb2_buffer vb;
	struct list_head list;
};

struct hrc_counter {
	unsigned long long vsync;
	unsigned long long config;
	unsigned long long writeback;
	unsigned long long waitting;
	unsigned long long overflow;
	unsigned long long unusal;
	unsigned long long error;
};

struct hrc_fps {
	unsigned long jiffies;
	unsigned long ms;
};

struct hrc_drv {
	struct platform_device *pdev;
	struct v4l2_device     v4l2_dev;
	struct video_device    *video_dev;
	struct vb2_queue       vb2_q;
	struct list_head       buf_list;
	struct list_head       buf_active_list;
	struct workqueue_struct *buf_wq;
	struct work_struct     buf_work;
	wait_queue_head_t      buf_wait_queue;
	struct hrc_hw_handle   *hrc_hdl;

	/* mutex lock for video device */
	struct mutex mlock;
	/* spin lock for buffer */
	spinlock_t   slock;

	/* sysfs: /sys/class/hrc/hrc/attr/ */
	dev_t sys_devid;
	struct cdev sys_cdev;
	struct class *sys_class;
	struct device *sys_device;

	/* clk */
	struct clk *clk_hrc;
	struct clk *clk_bus_hrc;
	struct clk *clk_bus_hdmi_rx;
	struct reset_control *rst_bus_hrc;
	struct reset_control *rst_bus_hdmi_rx;

	/* dts */
	u8 fpga;
	u8 output_to_disp2;	/* disp2: only use 1 dma-buf fd */

	/* var */
	u32 version;
	int irq;
	u32 caps;

	struct v4l2_pix_format_mplane pix_fmt_mp;
	struct hrc_apply_data apply_data;

	/* status */
	u8                 capturing;
	struct hrc_counter counter;
	struct hrc_fps     fps;

	/* v4l2 debug: only test v4l2 framework */
	u8                v4l2_debug;
	struct timer_list timer;

	/* ddr debug */
	u8         ddr_debug;
	u8         ddr_debug_done;
	u8         param_flag;
	void       *dbg_in_dma_buffer;
	dma_addr_t dbg_in_dma_handle;
	u32        dbg_in_buf_size;
	void       *dbg_out_dma_buffer;
	dma_addr_t dbg_out_dma_handle;
	u32        dbg_out_buf_size;
};

struct hdmirx_video_data {
	unsigned int           id;
	bool                   is_5v;
	struct v4l2_dv_timings timings;
	struct hrc_input_param input;
};

struct hdmirx_audio_data {
	unsigned int audio_sample_rate;
};

struct hrc_receive_data {
	struct hdmirx_video_data video[HDMI_MAX_INPUT_PORT];
	struct hdmirx_audio_data audio;
};

u32 loglevel;

static RAW_NOTIFIER_HEAD(hrc_chain);
static struct v4l2_subdev v4l2_sd;
static u32 subdev_event_subscribe;
static struct hrc_receive_data receive_data;
/* mutex lock for hdmirx receive data */
static struct mutex mlock_rx_data;
static u32 user_port_id;

static int __v4l2_to_hrc_format(u32 fourcc, enum hrc_fmt *format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_support_fmt); i++) {
		if (fourcc == hrc_support_fmt[i].fourcc) {
			*format = hrc_support_fmt[i].format;
			return 0;
		}
	}
	return -EINVAL;
}

static int __hrc_to_v4l2_format(enum hrc_fmt format, u32 *fourcc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_support_fmt); i++) {
		if (format == hrc_support_fmt[i].format) {
			*fourcc = hrc_support_fmt[i].fourcc;
			return 0;
		}
	}
	return -EINVAL;
}

static int __v4l2_to_hrc_color_space(u32 ycbcr_enc, enum hrc_color_space *cs)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_support_cs); i++) {
		if (ycbcr_enc == hrc_support_cs[i].ycbcr_enc) {
			*cs = hrc_support_cs[i].cs;
			return 0;
		}
	}
	return -EINVAL;
}

static int __hrc_to_v4l2_color_space(enum hrc_color_space cs, u8 *ycbcr_enc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_support_cs); i++) {
		if (cs == hrc_support_cs[i].cs) {
			*ycbcr_enc = hrc_support_cs[i].ycbcr_enc;
			return 0;
		}
	}
	return -EINVAL;
}

static int __v4l2_to_hrc_quantization(u32 quantization, enum hrc_quantization *quan)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_support_quan); i++) {
		if (quantization == hrc_support_quan[i].quantization) {
			*quan = hrc_support_quan[i].quan;
			return 0;
		}
	}
	return -EINVAL;
}

static int __hrc_to_v4l2_quantization(enum hrc_quantization quan, u8 *quantization)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_support_quan); i++) {
		if (quan == hrc_support_quan[i].quan) {
			*quantization = hrc_support_quan[i].quantization;
			return 0;
		}
	}
	return -EINVAL;
}

static int __source_id_to_port_id(enum hrc_source_id source_id, u32 *port_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hrc_source_id_infos); i++) {
		if (source_id == hrc_source_id_infos[i].source_id) {
			*port_id = hrc_source_id_infos[i].port_id;
			return 0;
		}
	}
	return -EINVAL;
}

static int sunxi_hrc_enable_resource(struct hrc_drv *hrc_drv)
{
	if (hrc_drv->rst_bus_hrc && reset_control_status(hrc_drv->rst_bus_hrc)) {
		if (reset_control_deassert(hrc_drv->rst_bus_hrc)) {
			hrc_err("rst_bus_hrc deassert failed!\n");
			return -EINVAL;
		}
	}

	/* If rst_bus_hdmi_rx is not enabled, HRC cannot work. */
	if (hrc_drv->rst_bus_hdmi_rx && reset_control_status(hrc_drv->rst_bus_hdmi_rx)) {
		if (reset_control_deassert(hrc_drv->rst_bus_hdmi_rx)) {
			hrc_err("rst_bus_hdmi_rx deassert failed!\n");
			return -EINVAL;
		}
	}

	if (hrc_drv->clk_bus_hdmi_rx && !__clk_is_enabled(hrc_drv->clk_bus_hdmi_rx)) {
		if (clk_prepare_enable(hrc_drv->clk_bus_hdmi_rx)) {
			hrc_err("clk_bus_hdmi_rx enable failed!\n");
			return -EINVAL;
		}
	}

	if (hrc_drv->clk_bus_hrc && !__clk_is_enabled(hrc_drv->clk_bus_hrc)) {
		if (clk_prepare_enable(hrc_drv->clk_bus_hrc)) {
			hrc_err("clk_bus_hrc enable failed!\n");
			return -EINVAL;
		}
	}

	if (hrc_drv->clk_hrc && !__clk_is_enabled(hrc_drv->clk_hrc)) {
		if (clk_prepare_enable(hrc_drv->clk_hrc)) {
			hrc_err("clk_hrc enable failed!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_hrc_disable_resource(struct hrc_drv *hrc_drv)
{
	if (hrc_drv->clk_hrc && __clk_is_enabled(hrc_drv->clk_hrc))
		clk_disable_unprepare(hrc_drv->clk_hrc);

	if (hrc_drv->clk_bus_hrc && __clk_is_enabled(hrc_drv->clk_bus_hrc))
		clk_disable_unprepare(hrc_drv->clk_bus_hrc);

	if (hrc_drv->rst_bus_hrc && !reset_control_status(hrc_drv->rst_bus_hrc)) {
		if (reset_control_assert(hrc_drv->rst_bus_hrc)) {
			hrc_err("rst_bus_hrc assert failed!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_hrc_set_next_buf_addr(struct hrc_drv *hrc_drv)
{
	int i;
	int ret = -1;
	struct hrc_buffer *buf = NULL;
	dma_addr_t buf_addr;
	unsigned long flags = 0;
	struct hrc_output_param *output = &hrc_drv->apply_data.output;
	struct v4l2_pix_format_mplane *pix_mp = &hrc_drv->pix_fmt_mp;

	if (IS_ERR_OR_NULL(hrc_drv))
		return -EINVAL;

	hrc_dbg("\n");

	spin_lock_irqsave(&hrc_drv->slock, flags);

	if (list_empty(&hrc_drv->buf_list)) {
		hrc_drv->counter.waitting++;
		spin_unlock_irqrestore(&hrc_drv->slock, flags);
		/* waitting for buffer queue */
		schedule_work(&hrc_drv->buf_work);
		return 1;
	}

	buf = list_entry(hrc_drv->buf_list.next, struct hrc_buffer, list);
	list_del(&buf->list);
	list_add_tail(&buf->list, &hrc_drv->buf_active_list);

	spin_unlock_irqrestore(&hrc_drv->slock, flags);

	memset(output->out_addr, 0, sizeof(output->out_addr));
	if (hrc_drv->output_to_disp2) {
		buf_addr = vb2_dma_contig_plane_dma_addr(&buf->vb, 0);
		output->out_addr[0][0] = (u32)(buf_addr >> 0);
		output->out_addr[0][1] = (u32)(buf_addr >> 32);
		output->out_addr[1][0] = (u32)((buf_addr + pix_mp->width * pix_mp->height) >> 0);
		output->out_addr[1][1] = (u32)((buf_addr + pix_mp->width * pix_mp->height) >> 32);
	} else {
		for (i = 0; i < hrc_drv->pix_fmt_mp.num_planes; i++) {
			buf_addr = vb2_dma_contig_plane_dma_addr(&buf->vb, i);
			output->out_addr[i][0] = (buf_addr >> 0) & 0xFFFFFFFF;
			output->out_addr[i][1] = (buf_addr >> 32) & 0xFFFFFFFF;
		}
	}

	hrc_drv->apply_data.dirty |= HRC_APPLY_ADDR_DIRTY;
	ret = sunxi_hrc_hardware_apply(hrc_drv->hrc_hdl, &hrc_drv->apply_data);
	if (ret) {
		hrc_err("hrc apply address error\n");
		return ret;
	}

	return 0;
}

static int sunxi_hrc_config_params(struct hrc_drv *hrc_drv)
{
	int ret = -EINVAL;
	struct hrc_input_param *input = &hrc_drv->apply_data.input;
	struct hrc_output_param *output = &hrc_drv->apply_data.output;
	struct v4l2_pix_format_mplane *pix_mp = &hrc_drv->pix_fmt_mp;

	/* if use tcon colorbar, no input parameter form hdmi rx.
	 * if use hdmi rx, we should copy reveive data from hdmi rx.
	 **/
	if (!(hrc_drv->param_flag & DEBUG_INPUT_PARAM)) {
		mutex_lock(&mlock_rx_data);
		memcpy(input, &receive_data.video[user_port_id].input, sizeof(*input));
		mutex_unlock(&mlock_rx_data);
	}

	if (hrc_drv->param_flag & DEBUG_OUTPUT_PARAM)
		goto CONFIG_TO_HARDWARE;

	memset(output, 0, sizeof(*output));

	output->width  = hrc_drv->pix_fmt_mp.width;
	output->height = hrc_drv->pix_fmt_mp.height;

	ret = __v4l2_to_hrc_format(pix_mp->pixelformat, &output->format);
	if (ret) {
		hrc_err("v4l2 format(%c%c%c%c) unsupported!\n",
			(pix_mp->pixelformat >> 0) & 0xFF, (pix_mp->pixelformat >> 8) & 0xFF,
			(pix_mp->pixelformat >> 16) & 0xFF, (pix_mp->pixelformat >> 24) & 0xFF);
		return ret;
	}

	ret = __v4l2_to_hrc_color_space(pix_mp->ycbcr_enc, &output->cs);
	if (ret) {
		hrc_err("v4l2 ycbcr_enc(%d) unsupported!\n", pix_mp->ycbcr_enc);
		return ret;
	}

	ret = __v4l2_to_hrc_quantization(pix_mp->quantization, &output->quantization);
	if (ret) {
		hrc_err("v4l2 quantization(%d) unsupported!\n", pix_mp->quantization);
		return ret;
	}

CONFIG_TO_HARDWARE:
	hrc_drv->apply_data.dirty |= HRC_APPLY_PARAM_DIRTY;
	ret = sunxi_hrc_hardware_apply(hrc_drv->hrc_hdl, &hrc_drv->apply_data);
	if (ret) {
		hrc_err("hrc apply param error!\n");
		return ret;
	}

	if (hrc_drv->fpga)
		hrc_reg_dump(hrc_drv->hrc_hdl, NULL, 0);

	return 0;
}

/* --- TEST FUNCTION START --- */

static void sunxi_hrc_fill_buff(struct hrc_drv *hrc_drv, char *vbuf)
{
	static int repeat;
	static int color_start;
	static unsigned int color[3] = {0xFF, 0xFF << 8, 0xFF << 16};
	int i, j;
	/* int w = hrc_drv->pix_fmt.width; */
	int h = hrc_drv->pix_fmt_mp.height;
	int bpp = hrc_drv->pix_fmt_mp.plane_fmt[0].bytesperline;
	/* int depth = hrc_drv->pix_fmt.bytesperline / hrc_drv->pix_fmt.width; */
	int color_idx = color_start;
#define COLOR_HEIGHT 32
#define REPEAT_FRAME 15

	/* !!! FIXME: only support RGB888 !!! */

	for (i = 0 ; i < h; i++) {
		if (!(i % COLOR_HEIGHT) && i != 0) {
			color_idx++;
			color_idx %= ARRAY_SIZE(color);
		}

		for (j = 0; j < bpp; j += 3) {
			vbuf[(bpp * i) + j + 0] = (color[color_idx] >> 0) & 0xFF;
			vbuf[(bpp * i) + j + 1] = (color[color_idx] >> 8) & 0xFF;
			vbuf[(bpp * i) + j + 2] = (color[color_idx] >> 16) & 0xFF;
		}
	}

	/* slow down */
	repeat++;
	repeat %= REPEAT_FRAME;
	if (!repeat) {
		color_start++;
		color_start %= ARRAY_SIZE(color);
	}
}

static void sunxi_hrc_timer_handler(struct timer_list *t)
{
	struct hrc_drv *hrc_drv = container_of(t, struct hrc_drv, timer);
	struct hrc_buffer *buf = NULL;
	char *vbuf;

	spin_lock(&hrc_drv->slock);
	if (!list_empty(&hrc_drv->buf_list)) {
		buf = list_entry(hrc_drv->buf_list.next, struct hrc_buffer, list);
		if (buf->vb.state != VB2_BUF_STATE_ACTIVE) {
			hrc_err("buffer no active!!!\n");
			return;
		}
		list_del(&buf->list);
	} else {
		spin_unlock(&hrc_drv->slock);
		goto out;
	}
	spin_unlock(&hrc_drv->slock);

	vbuf = vb2_plane_vaddr(&buf->vb, 0);

	memset(vbuf, 0xff, hrc_drv->pix_fmt_mp.plane_fmt[0].sizeimage);
	sunxi_hrc_fill_buff(hrc_drv, vbuf);

	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

out:
	mod_timer(&hrc_drv->timer, jiffies + HZ / 30);
}

/* --- TEST FUNCTION END --- */

static void sunxi_hrc_wait_buf_queue(struct work_struct *work)
{
	struct hrc_drv *hrc_drv = container_of(work, struct hrc_drv, buf_work);
	int ret;

	ret = wait_event_timeout(hrc_drv->buf_wait_queue,
				 !hrc_drv->capturing ||
				 !list_empty(&hrc_drv->buf_list),
				 msecs_to_jiffies(500));

	if (!hrc_drv->capturing)
		return;

	if (!list_empty(&hrc_drv->buf_list)) {
		ret = sunxi_hrc_set_next_buf_addr(hrc_drv);
		if (ret) {
			hrc_err("set addr error!\n");
			return;
		}

		sunxi_hrc_hardware_commit(hrc_drv->hrc_hdl);
	} else {
		hrc_wrn("!!!buffer queue too slow!!!\n");
		schedule_work(&hrc_drv->buf_work);
	}
}

static int sunxi_hrc_handle_unusual_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_timeout_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_overflow_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_vsync_irq(struct hrc_drv *hrc_drv)
{
	/* TODO */
	return 0;
}

static int sunxi_hrc_handle_config_irq(struct hrc_drv *hrc_drv)
{
	int ret;

	/* ddr_debug: only one output buffer for test. */
	if (hrc_drv->ddr_debug)
		return 0;

	ret = sunxi_hrc_set_next_buf_addr(hrc_drv);
	if (ret < 0) {
		hrc_err("set addr error\n");
		return ret;
	} else if (ret == 1) {
		/* wait for buffer queue */
		return 0;
	}

	sunxi_hrc_hardware_commit(hrc_drv->hrc_hdl);
	return 0;
}

static int sunxi_hrc_handle_writeback_irq(struct hrc_drv *hrc_drv)
{
	struct hrc_buffer *buf;
	unsigned long flags = 0;

	if (hrc_drv->ddr_debug) {
		hrc_drv->ddr_debug_done = 1;
		return 0;
	}

	spin_lock_irqsave(&hrc_drv->slock, flags);

	/* Writeback but no buffer? something wrong! */
	if (list_empty(&hrc_drv->buf_active_list)) {
		hrc_wrn("buf_active_list empty, but wb done!!!\n");
		spin_unlock_irqrestore(&hrc_drv->slock, flags);
		return -EINVAL;
	}

	buf = list_entry(hrc_drv->buf_active_list.next, struct hrc_buffer, list);
	if (buf->vb.state == VB2_BUF_STATE_ACTIVE) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

		/* calculate writeback fps */
		hrc_drv->fps.ms = jiffies_to_msecs(jiffies - hrc_drv->fps.jiffies);
		hrc_drv->fps.jiffies = jiffies;
	} else {
		hrc_err("buf not active!\n");
	}

	spin_unlock_irqrestore(&hrc_drv->slock, flags);
	return 0;
}

static irqreturn_t sunxi_hrc_irq_handler(int irq, void *data)
{
	struct hrc_drv *hrc_drv = (struct hrc_drv *)data;
	int ret = 0;
	u32 state = 0;

	ret = sunxi_hrc_hardware_get_irq_state(hrc_drv->hrc_hdl, &state);
	if (ret || !state)
		return IRQ_NONE;

	if (state & HRC_IRQ_UNUSUAL) {
		hrc_drv->counter.unusal++;

		if (sunxi_hrc_handle_unusual_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_TIMEOUT) {
		if (sunxi_hrc_handle_timeout_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_OVERFLOW) {
		hrc_drv->counter.overflow++;

		if (sunxi_hrc_handle_overflow_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_FRAME_VSYNC) {
		hrc_drv->counter.vsync++;

		if (sunxi_hrc_handle_vsync_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_CFG_FINISH) {
		hrc_drv->counter.config++;

		if (sunxi_hrc_handle_config_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	if (state & HRC_IRQ_WB_FINISH) {
		hrc_drv->counter.writeback++;

		if (sunxi_hrc_handle_writeback_irq(hrc_drv))
			hrc_drv->counter.error++;
	}

	sunxi_hrc_hardware_clr_irq_state(hrc_drv->hrc_hdl, state);
	return IRQ_HANDLED;
}

static int sunxi_hrc_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	strscpy(cap->driver, HRC_DRV_NAME, sizeof(cap->driver));
	strscpy(cap->card, HRC_DRV_NAME, sizeof(cap->card));

	cap->capabilities = hrc_drv->caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int sunxi_hrc_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(hrc_support_fmt))
		return -EINVAL;

	strscpy(f->description, hrc_support_fmt[f->index].name, sizeof(f->description));

	f->pixelformat = hrc_support_fmt[f->index].fourcc;

	return 0;
}

static int sunxi_hrc_g_fmt_vid_cap_mplane(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct hrc_drv *hrc_drv = video_drvdata(file);

	memcpy(pix_mp, &hrc_drv->pix_fmt_mp, sizeof(*pix_mp));

	return 0;
}

static int sunxi_hrc_try_fmt_vid_cap_mplane(struct file *file, void *priv, struct v4l2_format *f)
{
	int ret;
	struct hrc_size_range range;
	struct hrc_drv *hrc_drv = video_drvdata(file);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	enum hrc_fmt input_format;
	enum hrc_fmt output_format;
	enum hrc_color_space input_cs;
	enum hrc_color_space output_cs;
	enum hrc_quantization input_quan;
	enum hrc_quantization output_quan;

	mutex_lock(&mlock_rx_data);
	input_format = receive_data.video[user_port_id].input.format;
	input_cs = receive_data.video[user_port_id].input.cs;
	input_quan = receive_data.video[user_port_id].input.quantization;
	mutex_unlock(&mlock_rx_data);

	/* width and height */
	sunxi_hrc_hardware_get_size_range(hrc_drv->hrc_hdl, &range);
	v4l_bound_align_image(&pix_mp->width, range.min_width,
			      range.max_width, range.align_width,
			      &pix_mp->height, range.min_height,
			      range.max_height, range.align_height, 0);

	/* format */
	if (hrc_drv->param_flag & DEBUG_INPUT_PARAM)
		input_format = hrc_drv->apply_data.input.format;

	ret = __v4l2_to_hrc_format(pix_mp->pixelformat, &output_format);
	if (ret)
		/* if not support, use the same as input. */
		output_format = input_format;

	ret = sunxi_hrc_hardware_get_format(hrc_drv->hrc_hdl, input_format, &output_format);
	if (ret) {
		hrc_err("Can't get output format!\n");
		return -EINVAL;
	}

	ret = __hrc_to_v4l2_format(output_format, &pix_mp->pixelformat);
	if (ret) {
		hrc_err("Unknown hrc format(%d)!\n", output_format);
		return ret;
	}

	/* bytesperline */
	switch (pix_mp->pixelformat) {
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_YUV24:
		pix_mp->num_planes = 1;
		pix_mp->plane_fmt[0].bytesperline = pix_mp->width * 3;
		pix_mp->plane_fmt[0].sizeimage = pix_mp->width * 3 * pix_mp->height;
		break;
	case V4L2_PIX_FMT_NV24:
		if (hrc_drv->output_to_disp2) {
			pix_mp->num_planes = 1;
			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height * 3;
		} else {
			pix_mp->num_planes = 2;
			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height;
			pix_mp->plane_fmt[1].bytesperline = pix_mp->width * 2;
			pix_mp->plane_fmt[1].sizeimage = pix_mp->width * 2 * pix_mp->height;
		}
		break;
	case V4L2_PIX_FMT_NV16:
		if (hrc_drv->output_to_disp2) {
			pix_mp->num_planes = 1;
			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height * 2;
		} else {
			pix_mp->num_planes = 2;
			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height;
			pix_mp->plane_fmt[1].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[1].sizeimage = pix_mp->width * pix_mp->height;
		}
		break;
	case V4L2_PIX_FMT_NV12:
		if (hrc_drv->output_to_disp2) {
			pix_mp->num_planes = 1;
			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height * 3 / 2;
		} else {
			pix_mp->num_planes = 2;
			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height;
			pix_mp->plane_fmt[1].bytesperline = pix_mp->width / 2;
			pix_mp->plane_fmt[1].sizeimage = pix_mp->width / 2 * pix_mp->height;
		}
		break;
	default:
		hrc_err("Unsupport pixelformat!!!");
	}

	/* ycbcr_enc */
	if (hrc_drv->param_flag & DEBUG_INPUT_PARAM)
		input_cs = hrc_drv->apply_data.input.cs;

	ret = __v4l2_to_hrc_color_space(pix_mp->ycbcr_enc, &output_cs);
	if (ret)
		/* if not support, use the same as input. */
		output_cs = input_cs;

	ret = sunxi_hrc_hardware_get_color_space(hrc_drv->hrc_hdl, input_cs, &output_cs);
	if (ret) {
		hrc_err("Can't get output color space!\n");
		return -EINVAL;
	}

	ret = __hrc_to_v4l2_color_space(output_cs, &pix_mp->ycbcr_enc);
	if (ret) {
		hrc_err("Unknown hrc color space(%d)!\n", output_cs);
		return ret;
	}

	/* quantization */
	if (hrc_drv->param_flag & DEBUG_INPUT_PARAM)
		input_quan = hrc_drv->apply_data.input.quantization;

	ret = __v4l2_to_hrc_quantization(pix_mp->quantization, &output_quan);
	if (ret)
		/* if not support, use the same as input. */
		output_quan = input_quan;

	ret = sunxi_hrc_hardware_get_quantization(hrc_drv->hrc_hdl, input_quan, &output_quan);
	if (ret) {
		hrc_err("Can't get output quantization!\n");
		return -EINVAL;
	}

	ret = __hrc_to_v4l2_quantization(output_quan, &pix_mp->quantization);
	if (ret) {
		hrc_err("Unknown hrc quantization(%d)!\n", output_quan);
		return ret;
	}

	return 0;
}

static int sunxi_hrc_s_fmt_vid_cap_mplane(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret;

	hrc_dbg("[s] wxh: %dx%d fmt: %c%c%c%c ycbcr: %d quan: %d ",
		pix_mp->width, pix_mp->height,
		(pix_mp->pixelformat >> 0) & 0xFF, (pix_mp->pixelformat >> 8) & 0xFF,
		(pix_mp->pixelformat >> 16) & 0xFF, (pix_mp->pixelformat >> 24) & 0xFF,
		pix_mp->ycbcr_enc, pix_mp->quantization);
	hrc_dbg("nplanes: %d bpp: (%d %d %d) size: (%d %d %d)\n",
		pix_mp->num_planes,
		pix_mp->plane_fmt[0].bytesperline, pix_mp->plane_fmt[1].bytesperline,
		pix_mp->plane_fmt[2].bytesperline, pix_mp->plane_fmt[0].sizeimage,
		pix_mp->plane_fmt[1].sizeimage, pix_mp->plane_fmt[2].sizeimage);

	ret = sunxi_hrc_try_fmt_vid_cap_mplane(file, priv, f);
	if (ret < 0)
		return ret;

	memcpy(&hrc_drv->pix_fmt_mp, pix_mp, sizeof(hrc_drv->pix_fmt_mp));

	hrc_dbg("[e] wxh: %dx%d fmt: %c%c%c%c ycbcr: %d quan: %d ",
		pix_mp->width, pix_mp->height,
		(pix_mp->pixelformat >> 0) & 0xFF, (pix_mp->pixelformat >> 8) & 0xFF,
		(pix_mp->pixelformat >> 16) & 0xFF, (pix_mp->pixelformat >> 24) & 0xFF,
		pix_mp->ycbcr_enc, pix_mp->quantization);
	hrc_dbg("nplanes: %d bpp: (%d %d %d) size: (%d %d %d)\n",
		pix_mp->num_planes,
		pix_mp->plane_fmt[0].bytesperline, pix_mp->plane_fmt[1].bytesperline,
		pix_mp->plane_fmt[2].bytesperline, pix_mp->plane_fmt[0].sizeimage,
		pix_mp->plane_fmt[1].sizeimage, pix_mp->plane_fmt[2].sizeimage);
	return 0;
}

static int sunxi_hrc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	if (hrc_drv->output_to_disp2 && p->memory != V4L2_MEMORY_DMABUF)
		return -EINVAL;

	return vb2_reqbufs(&hrc_drv->vb2_q, p);
}

static int sunxi_hrc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_querybuf(&hrc_drv->vb2_q, p);
}

static int sunxi_hrc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_qbuf(&hrc_drv->vb2_q, hrc_drv->v4l2_dev.mdev, p);
}

static int sunxi_hrc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_dqbuf(&hrc_drv->vb2_q, p, file->f_flags & O_NONBLOCK);
}

static int sunxi_hrc_enum_input(struct file *file, void *priv, struct v4l2_input *inp)
{
	if (inp->index >= HDMI_MAX_INPUT_PORT)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	snprintf(inp->name, sizeof(inp->name), "hrc-%d", inp->index);
	return 0;
}

static int sunxi_hrc_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = user_port_id;
	return 0;
}

static int sunxi_hrc_s_input(struct file *file, void *priv, unsigned int i)
{
	hrc_dbg("set input: %d\n", i);

	if (i >= HDMI_MAX_INPUT_PORT)
		return -EINVAL;

	user_port_id = i;
	return 0;
}

static int sunxi_hrc_g_parm(struct file *file, void *priv, struct v4l2_streamparm *parms)
{
	hrc_dbg("\n");
	return 0;
}

static int sunxi_hrc_s_parm(struct file *file, void *priv, struct v4l2_streamparm *parms)
{
	hrc_dbg("\n");
	return 0;
}

static int sunxi_hrc_streamon(struct file *file, void *priv, enum v4l2_buf_type type)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret = 0;

	hrc_dbg("\n");

	if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	if (hrc_drv->capturing)
		return -EBUSY;

	hrc_drv->counter.vsync = 0;
	hrc_drv->counter.config = 0;
	hrc_drv->counter.writeback = 0;
	hrc_drv->counter.waitting = 0;
	hrc_drv->counter.overflow = 0;
	hrc_drv->counter.unusal = 0;
	hrc_drv->counter.error = 0;
	hrc_drv->fps.jiffies = 0;
	hrc_drv->fps.ms = 0;

	ret = vb2_streamon(&hrc_drv->vb2_q, type);
	if (ret) {
		hrc_err("vb2_streamon error\n");
		goto ERR_STREAMON;
	}

	if (hrc_drv->v4l2_debug) {
		timer_setup(&hrc_drv->timer, sunxi_hrc_timer_handler, 0);
		hrc_drv->timer.expires = jiffies + HZ / 2;
		add_timer(&hrc_drv->timer);
	} else {
		ret = sunxi_hrc_hardware_enable(hrc_drv->hrc_hdl);
		if (ret) {
			hrc_err("hrc enable error\n");
			goto ERR_HRC_ENABLE;
		}

		ret = sunxi_hrc_config_params(hrc_drv);
		if (ret) {
			hrc_err("sunxi_hrc_config_params error\n");
			goto ERR_HRC_CONFIG;
		}

		ret = sunxi_hrc_set_next_buf_addr(hrc_drv);
		if (ret < 0) {
			hrc_err("sunxi_hrc_set_next_buf_addr error\n");
			goto ERR_HRC_SET_ADDR;
		} else if (ret == 0) {
			sunxi_hrc_hardware_commit(hrc_drv->hrc_hdl);
		}
	}

	hrc_drv->capturing = 1;
	return 0;

ERR_HRC_SET_ADDR:
ERR_HRC_CONFIG:
	sunxi_hrc_hardware_disable(hrc_drv->hrc_hdl);
ERR_HRC_ENABLE:
ERR_STREAMON:
	return ret;
}

static int sunxi_hrc_streamoff(struct file *file, void *priv, enum v4l2_buf_type type)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret = 0;

	hrc_dbg("\n");

	if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE || !hrc_drv->capturing)
		return -EINVAL;

	if (hrc_drv->v4l2_debug)
		del_timer(&hrc_drv->timer);
	else
		sunxi_hrc_hardware_disable(hrc_drv->hrc_hdl);

	hrc_drv->capturing = 0;
	cancel_work_sync(&hrc_drv->buf_work);
	wake_up(&hrc_drv->buf_wait_queue);

	ret = vb2_streamoff(&hrc_drv->vb2_q, type);
	if (ret) {
		hrc_err("vb2_streamoff error!\n");
		return ret;
	}

	return 0;
}

static int sunxi_hrc_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *fsize)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	struct hrc_size_range range;

	if (fsize->index > 0)
		return -EINVAL;

	sunxi_hrc_hardware_get_size_range(hrc_drv->hrc_hdl, &range);

	fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
	fsize->stepwise.min_width  = range.min_width;
	fsize->stepwise.min_height = range.min_height;
	fsize->stepwise.max_width  = range.max_width;
	fsize->stepwise.max_height = range.max_height;

	return 0;
}

static const struct v4l2_ioctl_ops v4l2_hrc_ioctl_ops = {
	.vidioc_querycap               = sunxi_hrc_querycap,

	.vidioc_enum_fmt_vid_cap       = sunxi_hrc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap_mplane   = sunxi_hrc_g_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane = sunxi_hrc_try_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane   = sunxi_hrc_s_fmt_vid_cap_mplane,

	.vidioc_reqbufs                = sunxi_hrc_reqbufs,
	.vidioc_querybuf               = sunxi_hrc_querybuf,
	.vidioc_qbuf                   = sunxi_hrc_qbuf,
	.vidioc_dqbuf                  = sunxi_hrc_dqbuf,

	.vidioc_enum_input             = sunxi_hrc_enum_input,
	.vidioc_g_input                = sunxi_hrc_g_input,
	.vidioc_s_input                = sunxi_hrc_s_input,
	.vidioc_g_parm                 = sunxi_hrc_g_parm,
	.vidioc_s_parm                 = sunxi_hrc_s_parm,

	.vidioc_streamon               = sunxi_hrc_streamon,
	.vidioc_streamoff              = sunxi_hrc_streamoff,

	.vidioc_enum_framesizes        = sunxi_hrc_enum_framesizes,
};

static int sunxi_hrc_v4l2_open(struct file *file)
{
	hrc_dbg("\n");

	v4l2_fh_open(file);

	return 0;
}

static int sunxi_hrc_v4l2_close(struct file *file)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	hrc_dbg("\n");

	if (hrc_drv->capturing) {
		if (hrc_drv->v4l2_debug)
			del_timer(&hrc_drv->timer);
		else
			sunxi_hrc_hardware_disable(hrc_drv->hrc_hdl);

		hrc_drv->capturing = 0;
		cancel_work_sync(&hrc_drv->buf_work);
		wake_up(&hrc_drv->buf_wait_queue);
	}

	vb2_queue_release(&hrc_drv->vb2_q);
	v4l2_fh_release(file);

	return 0;
}

static ssize_t sunxi_hrc_v4l2_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_read(&hrc_drv->vb2_q, data, count, ppos, file->f_flags & O_NONBLOCK);
}

static unsigned int sunxi_hrc_v4l2_poll(struct file *file, struct poll_table_struct *wait)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);

	return vb2_poll(&hrc_drv->vb2_q, file, wait);
}

static int sunxi_hrc_v4l2_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct hrc_drv *hrc_drv = video_drvdata(file);
	int ret;

	/* hrc_dbg("mmap called, vma=0x%08lx\n", (unsigned long)vma); */
	ret = vb2_mmap(&hrc_drv->vb2_q, vma);
	/* hrc_dbg("vma start=0x%08lx, size=%ld, ret=%d\n", (unsigned long)vma->vm_start, */
	/*                 (unsigned long)vma->vm_end - (unsigned long)vma->vm_start, ret); */
	return ret;
}

static const struct v4l2_file_operations v4l2_hrc_fops = {
	.owner		= THIS_MODULE,
	.open           = sunxi_hrc_v4l2_open,
	.release        = sunxi_hrc_v4l2_close,
	.read           = sunxi_hrc_v4l2_read,
	.poll		= sunxi_hrc_v4l2_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = sunxi_hrc_v4l2_mmap,
};

static int sunxi_hrc_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				 unsigned int *nplanes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	int i;
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vq);

	hrc_dbg("wxh(%dx%d) sizeimage(%d %d %d)\n",
		hrc_drv->pix_fmt_mp.width, hrc_drv->pix_fmt_mp.height,
		hrc_drv->pix_fmt_mp.plane_fmt[0].sizeimage,
		hrc_drv->pix_fmt_mp.plane_fmt[1].sizeimage,
		hrc_drv->pix_fmt_mp.plane_fmt[2].sizeimage);

	*nplanes = hrc_drv->pix_fmt_mp.num_planes;

	for (i = 0; i < *nplanes; i++)
		sizes[i] = hrc_drv->pix_fmt_mp.plane_fmt[i].sizeimage;

	if (*nbuffers < 2)
		*nbuffers = 2;
	else if (*nbuffers > 10)
		*nbuffers = 10;

	hrc_dbg("nbuffers: %d nplanes: %d\n", *nbuffers, *nplanes);

	return 0;
}

static int sunxi_hrc_buf_prepare(struct vb2_buffer *vb)
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size;
	int i;

	for (i = 0; i < hrc_drv->pix_fmt_mp.num_planes; i++) {
		size = hrc_drv->pix_fmt_mp.plane_fmt[i].sizeimage;

		if (vb2_plane_size(vb, i) < size) {
			hrc_err("data will not fit into plane (%lu < %lu)\n",
				vb2_plane_size(vb, i), size);
			return -EINVAL;
		}

		vb2_set_plane_payload(vb, i, size);

		if (vb->memory == V4L2_MEMORY_MMAP)
			vb->planes[i].m.offset = vb2_dma_contig_plane_dma_addr(vb, i);
	}

	return 0;
}

static void sunxi_hrc_buf_queue(struct vb2_buffer *vb)
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vb->vb2_queue);
	struct hrc_buffer *buf = container_of(vb, struct hrc_buffer, vb);
	unsigned long flags = 0;

	hrc_dbg("\n");

	spin_lock_irqsave(&hrc_drv->slock, flags);
	list_add_tail(&buf->list, &hrc_drv->buf_list);
	spin_unlock_irqrestore(&hrc_drv->slock, flags);

	wake_up(&hrc_drv->buf_wait_queue);
}

static int sunxi_hrc_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	/* struct hrc_drv *hrc_drv = vb2_get_drv_priv(vq); */

	hrc_dbg("\n");

	return 0;
}

static void sunxi_hrc_stop_streaming(struct vb2_queue *vq)
{
	struct hrc_drv *hrc_drv = vb2_get_drv_priv(vq);
	unsigned long flags = 0;

	hrc_dbg("\n");

	spin_lock_irqsave(&hrc_drv->slock, flags);
	while (!list_empty(&hrc_drv->buf_list)) {
		struct hrc_buffer *buf;

		buf = list_entry(hrc_drv->buf_list.next, struct hrc_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	while (!list_empty(&hrc_drv->buf_active_list)) {
		struct hrc_buffer *buf;

		buf = list_entry(hrc_drv->buf_active_list.next, struct hrc_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&hrc_drv->slock, flags);
}

static const struct vb2_ops vb2_hrc_ops = {
	.queue_setup     = sunxi_hrc_queue_setup,
	.buf_prepare     = sunxi_hrc_buf_prepare,
	.buf_queue       = sunxi_hrc_buf_queue,
	.start_streaming = sunxi_hrc_start_streaming,
	.stop_streaming  = sunxi_hrc_stop_streaming,
};

static int sunxi_hrc_subdev_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
					    struct v4l2_event_subscription *sub)
{
	int ret;

	hrc_dbg("\n");

	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		ret = v4l2_src_change_event_subdev_subscribe(sd, fh, sub);
		if (!ret)
			subdev_event_subscribe |= BIT(V4L2_EVENT_SOURCE_CHANGE);
		else
			subdev_event_subscribe &= ~BIT(V4L2_EVENT_SOURCE_CHANGE);
		return ret;
	default:
		return -EINVAL;
	}
}

static int sunxi_hrc_subdev_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
					      struct v4l2_event_subscription *sub)
{
	hrc_dbg("\n");

	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		subdev_event_subscribe &= ~BIT(V4L2_EVENT_SOURCE_CHANGE);
		break;
	default:
		return -EINVAL;
	}
	return v4l2_event_subdev_unsubscribe(sd, fh, sub);
}

static int sunxi_hrc_subdev_get_dv_timings(struct v4l2_subdev *sd,
					   struct v4l2_dv_timings *timings)
{
	hrc_dbg("\n");

	mutex_lock(&mlock_rx_data);
	memcpy(&timings->bt, &receive_data.video[user_port_id].timings, sizeof(*timings));
	mutex_unlock(&mlock_rx_data);
	return 0;
}

static const struct v4l2_subdev_core_ops subdev_hrc_core_ops = {
	.subscribe_event = sunxi_hrc_subdev_subscribe_event,
	.unsubscribe_event = sunxi_hrc_subdev_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops subdev_hrc_video_ops = {
	.g_dv_timings = sunxi_hrc_subdev_get_dv_timings,
};

static const struct v4l2_subdev_ops subdev_hrc_ops = {
	.core = &subdev_hrc_core_ops,
	.video = &subdev_hrc_video_ops,
};

static ssize_t loglevel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "loglevel: %d\n", loglevel);
	return n;
}

static ssize_t loglevel_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long long value;

	if (!kstrtoull(buf, 10, &value))
		loglevel = value > 8 ? 8 : (u32)value;
	return count;
}

static DEVICE_ATTR_RW(loglevel);

static ssize_t reg_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);

	return hrc_reg_dump(hrc_drv->hrc_hdl, buf, 0) % PAGE_SIZE;
}

static DEVICE_ATTR_RO(reg_dump);

static u32 reg_addr;
static u32 reg_val;
static int __get_user_input_value(char **ptr, const char *sep, unsigned long long *value)
{
	int ret;
	char *token;

	if (IS_ERR_OR_NULL(ptr))
		return -EINVAL;

	do {
		token = strsep(ptr, sep);
		if (IS_ERR_OR_NULL(token))
			return -EINVAL;
	} while (token[0] == '\0');

	ret = kstrtoull(token, 0, value);
	if (ret)
		return ret;

	return 0;
}

static ssize_t reg_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);

	/* n += sprintf(buf + n, "Read register value:\n"); */
	/* n += sprintf(buf + n, "    echo <start> > reg_read; cat reg_read\n"); */
	/* n += sprintf(buf + n, "    echo <start>,<end> > reg_read; cat reg_read\n"); */
	/* n += sprintf(buf + n, "    echo <start> <end> > reg_read; cat reg_read\n"); */

	if (!reg_val)
		reg_val = reg_addr + 4;

	if (reg_addr == reg_val)
		reg_val += 4;

	/* 4-byte alignment */
	reg_val = ((reg_val) + 3) & ~(3);

	hrc_inf("read addr: 0x%x - 0x%x\n", reg_addr, reg_val);

	for (i = reg_addr; i < reg_val; i += 4) {
		if (!(i % 16) || i == reg_addr)
			n += sprintf(buf + n, "\n0x%08x:", i);

		n += sprintf(buf + n, " 0x%08x", hrc_read(hrc_drv->hrc_hdl, i));
	}
	n += sprintf(buf + n, "\n");

	reg_addr = 0;
	reg_val = 0;

	return n;
}

static ssize_t reg_read_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	char *ptr = (char *)buf;
	unsigned long long value;

	ret = __get_user_input_value(&ptr, ", ", &value);
	if (ret)
		goto ERR_GET_USER_INPUT_VALUE;

	reg_addr = (u32)value;

	/* option */
	ret = __get_user_input_value(&ptr, ", ", &value);
	if (!ret)
		reg_val = (u32)value;

	if (reg_addr % 4) {
		hrc_err("reg addr must align 4 bytes! reg_addr: 0x%x\n", reg_addr);
		reg_addr = 0;
		reg_val = 0;
		return -EINVAL;
	}

	return count;

ERR_GET_USER_INPUT_VALUE:
	hrc_err("get value failed, buf: %s\n", buf);
	return ret;
}

static DEVICE_ATTR_RW(reg_read);

static ssize_t reg_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "Write the value to register:\n");
	n += sprintf(buf + n, "    echo <addr>,<val> > reg_write\n");
	n += sprintf(buf + n, "    echo <addr> <val> > reg_write\n");

	return n;
}

static ssize_t reg_write_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int ret;
	char *ptr = (char *)buf;
	unsigned long long value;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);

	ret = __get_user_input_value(&ptr, ", ", &value);
	if (ret)
		goto ERR_GET_USER_INPUT_VALUE;

	reg_addr = (u32)value;

	ret = __get_user_input_value(&ptr, ", ", &value);
	if (ret)
		goto ERR_GET_USER_INPUT_VALUE;

	reg_val = (u32)value;

	if (reg_addr % 4) {
		hrc_err("reg addr must align 4 bytes! reg_addr: 0x%x\n", reg_addr);
		reg_addr = 0;
		reg_val = 0;
		return -EINVAL;
	}

	hrc_inf("reg_addr: 0x%x reg_val: 0x%x\n", reg_addr, reg_val);

	hrc_write(hrc_drv->hrc_hdl, reg_addr, reg_val);

	reg_addr = 0;
	reg_val = 0;

	return count;

ERR_GET_USER_INPUT_VALUE:
	hrc_err("get value failed, buf: %s\n", buf);
	return ret;
}

static DEVICE_ATTR_RW(reg_write);

static ssize_t dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);

	n += sprintf(buf + n, "\n[TOP]\n");
	n += sprintf(buf + n, "  - fps      : %ld\n", 1000 / hrc_drv->fps.ms);
	n += sprintf(buf + n, "  - vsync    : %lld\n", hrc_drv->counter.vsync);
	n += sprintf(buf + n, "  - config   : %lld\n", hrc_drv->counter.config);
	n += sprintf(buf + n, "  - writeback: %lld\n", hrc_drv->counter.writeback);
	n += sprintf(buf + n, "  - waitting : %lld\n", hrc_drv->counter.waitting);
	n += sprintf(buf + n, "  - overflow : %lld\n", hrc_drv->counter.overflow);
	n += sprintf(buf + n, "  - unusal   : %lld\n", hrc_drv->counter.unusal);
	n += sprintf(buf + n, "  - error    : %lld\n", hrc_drv->counter.error);

	return sunxi_hrc_hardware_dump(hrc_drv->hrc_hdl, buf, n);
}

static DEVICE_ATTR_RO(dump);

/* v4l2 framework debug
 *
 * if v4l2_debug = 1:
 *     timer(simulation) -> hrc(driver) -> v4l2(framework) -> application
 * else:
 *     hrc(hardware) -> hrc(driver) -> v4l2(framework) -> application
 **/
static ssize_t v4l2_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	ssize_t n = 0;

	n += sprintf(buf + n, "v4l2_debug: %d\n", hrc_drv->v4l2_debug);
	return n;
}

static ssize_t v4l2_debug_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	unsigned long long value;

	if (!kstrtoull(buf, 10, &value))
		hrc_drv->v4l2_debug = !!(u8)(value);
	return count;
}

static DEVICE_ATTR_RW(v4l2_debug);

static ssize_t params_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	struct hrc_input_param input = hrc_drv->apply_data.input;
	struct hrc_output_param output = hrc_drv->apply_data.output;

	n += sprintf(buf + n, "\n[INPUT]\n");
	n += sprintf(buf + n, "  - data_src: %d\n", input.data_src);
	n += sprintf(buf + n, "  - field_mode: %d\n", input.field_mode);
	n += sprintf(buf + n, "  - field_inverse: %d\n", input.field_inverse);
	n += sprintf(buf + n, "  - field_order: %d\n", input.field_order);
	n += sprintf(buf + n, "  - width: %d\n", input.width);
	n += sprintf(buf + n, "  - height: %d\n", input.height);
	n += sprintf(buf + n, "  - format: %d\n", input.format);
	n += sprintf(buf + n, "  - depth: %d\n", input.depth);
	n += sprintf(buf + n, "  - cs: %d\n", input.cs);
	n += sprintf(buf + n, "  - quantization: %d\n", input.quantization);
	n += sprintf(buf + n, "  - setup_by_user: %s\n",
		     hrc_drv->param_flag & DEBUG_INPUT_PARAM ? "true" : "false");
	n += sprintf(buf + n, "  - dbg_buf_size: %d\n", hrc_drv->dbg_in_buf_size);

	n += sprintf(buf + n, "\n[OUTPUT]\n");
	n += sprintf(buf + n, "  - width: %d\n", output.width);
	n += sprintf(buf + n, "  - height: %d\n", output.height);
	n += sprintf(buf + n, "  - format: %d\n", output.format);
	n += sprintf(buf + n, "  - cs: %d\n", output.cs);
	n += sprintf(buf + n, "  - quantization: %d\n", output.quantization);
	n += sprintf(buf + n, "  - setup_by_user: %s\n",
		     hrc_drv->param_flag & DEBUG_OUTPUT_PARAM ? "true" : "false");
	n += sprintf(buf + n, "  - dbg_out_buf_size: %d\n", hrc_drv->dbg_out_buf_size);

	mutex_lock(&mlock_rx_data);
	n += sprintf(buf + n, "\n[HDMIRX]\n");
	n += sprintf(buf + n, "  - [VIDEO]\n");
	n += sprintf(buf + n, "    - is_5v:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].is_5v);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - data_src:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.data_src);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - field_mode:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.field_mode);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - field_inverse:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.field_inverse);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - field_order:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.field_order);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - width:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.width);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - height:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.height);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - format:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.format);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - depth:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.depth);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - cs:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.cs);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "    - quantization:");
	for (i = 0; i < HDMI_MAX_INPUT_PORT; i++)
		n += sprintf(buf + n, " %d", receive_data.video[i].input.quantization);
	n += sprintf(buf + n, "\n");

	n += sprintf(buf + n, "  - [AUDIO]\n");
	n += sprintf(buf + n, "    - audio_sample_rate: %d\n",
		     receive_data.audio.audio_sample_rate);
	mutex_unlock(&mlock_rx_data);

	return n;
}

static int __ddr_debug_malloc_input_ddr_memory(struct hrc_drv *hrc_drv)
{
	struct hrc_input_param *input = &hrc_drv->apply_data.input;

	if (hrc_drv->dbg_in_dma_buffer)
		goto MALLOC_DONE;

	/* 4 bytes align, Y and C Channel */
	hrc_drv->dbg_in_buf_size = input->width * input->height * 4 * 2;

	hrc_drv->dbg_in_dma_buffer = dma_alloc_coherent(&hrc_drv->pdev->dev,
							PAGE_ALIGN(hrc_drv->dbg_in_buf_size),
							&hrc_drv->dbg_in_dma_handle,
							GFP_KERNEL);
	if (IS_ERR_OR_NULL(hrc_drv->dbg_in_dma_buffer))
		return -ENOMEM;

MALLOC_DONE:
	memset(hrc_drv->dbg_in_dma_buffer, 0, hrc_drv->dbg_in_buf_size);

	input->ddr_addr[0] = hrc_drv->dbg_in_dma_handle;
	input->ddr_addr[1] = hrc_drv->dbg_in_dma_handle + hrc_drv->dbg_in_buf_size / 2;
	return 0;
}

static int __ddr_debug_malloc_output_ddr_memory(struct hrc_drv *hrc_drv)
{
	u32 dbg_out_buf_size;
	u32 offset;
	struct hrc_output_param *output = &hrc_drv->apply_data.output;

	switch (output->format) {
	case HRC_FMT_RGB:
	case HRC_FMT_YUV444_1PLANE:
		dbg_out_buf_size = output->width * output->height * 3;
		break;
	case HRC_FMT_YUV444:
		dbg_out_buf_size = output->width * output->height * 3;
		break;
	case HRC_FMT_YUV422:
		dbg_out_buf_size = output->width * output->height * 2;
		break;
	case HRC_FMT_YUV420:
		dbg_out_buf_size = output->width * output->height * 3 / 2;
		break;
	default:
		return -EINVAL;
	}

	if (hrc_drv->dbg_out_dma_buffer) {
		if (dbg_out_buf_size == hrc_drv->dbg_out_buf_size)
			goto MALLOC_DONE;

		/* remalloc */
		dma_free_coherent(&hrc_drv->pdev->dev, PAGE_ALIGN(hrc_drv->dbg_out_buf_size),
				  hrc_drv->dbg_out_dma_buffer, hrc_drv->dbg_out_dma_handle);
		hrc_drv->dbg_out_dma_buffer = NULL;
	}

	hrc_drv->dbg_out_buf_size = dbg_out_buf_size;
	hrc_drv->dbg_out_dma_buffer = dma_alloc_coherent(&hrc_drv->pdev->dev,
							 PAGE_ALIGN(hrc_drv->dbg_out_buf_size),
							 &hrc_drv->dbg_out_dma_handle,
							 GFP_KERNEL);
	if (IS_ERR_OR_NULL(hrc_drv->dbg_out_dma_buffer))
		return -ENOMEM;

MALLOC_DONE:
	memset(hrc_drv->dbg_out_dma_buffer, 0, hrc_drv->dbg_out_buf_size);

	offset = output->width * output->height;
	output->out_addr[0][0] = hrc_drv->dbg_out_dma_handle & 0xFFFFFFFF;
	output->out_addr[0][1] = (hrc_drv->dbg_out_dma_handle >> 32) & 0xFF;
	output->out_addr[1][0] = (hrc_drv->dbg_out_dma_handle + offset) & 0xFFFFFFFF;
	output->out_addr[1][1] = ((hrc_drv->dbg_out_dma_handle + offset) >> 32) & 0xFF;
	return 0;
}

static int __ddr_debug_free_all_memory(struct hrc_drv *hrc_drv)
{
	if (hrc_drv->dbg_in_dma_buffer) {
		dma_free_coherent(&hrc_drv->pdev->dev, PAGE_ALIGN(hrc_drv->dbg_in_buf_size),
				  hrc_drv->dbg_in_dma_buffer, hrc_drv->dbg_in_dma_handle);
		hrc_drv->dbg_in_dma_buffer = NULL;
		hrc_drv->dbg_in_buf_size = 0;
		hrc_drv->dbg_in_dma_handle = 0;
	}

	if (hrc_drv->dbg_out_dma_buffer) {
		dma_free_coherent(&hrc_drv->pdev->dev, PAGE_ALIGN(hrc_drv->dbg_out_buf_size),
				  hrc_drv->dbg_out_dma_buffer, hrc_drv->dbg_out_dma_handle);
		hrc_drv->dbg_out_dma_buffer = NULL;
		hrc_drv->dbg_out_buf_size = 0;
		hrc_drv->dbg_out_dma_handle = 0;
	}
	return 0;
}

static ssize_t params_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	int i;
	int ret;
#define IN_PARAM_NUM	10
#define OUT_PARAM_NUM	5
	u32 inputs[IN_PARAM_NUM] = {0};
	u32 outputs[OUT_PARAM_NUM] = {0};
	char *ptr;
	unsigned long long value;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);
	struct hrc_input_param *input = &hrc_drv->apply_data.input;
	struct hrc_output_param *output = &hrc_drv->apply_data.output;

	if (!strncmp(buf, "input=", 6)) {
		ptr = (char *)buf + 6;

		for (i = 0; i < IN_PARAM_NUM; i++) {
			ret = __get_user_input_value(&ptr, ", ", &value);
			if (ret)
				goto ERR_GET_USER_INPUT_VALUE;

			inputs[i] = (u32)value;
		}

		memset(input, 0, sizeof(*input));

		input->data_src      = inputs[0];
		input->field_mode    = inputs[1];
		input->field_inverse = inputs[2];
		input->field_order   = inputs[3];
		input->width         = inputs[4];
		input->height        = inputs[5];
		input->format        = inputs[6];
		input->depth         = inputs[7];
		input->cs            = inputs[8];
		input->quantization  = inputs[9];

		if (input->data_src == HRC_DATA_SRC_DDR) {
			ret = __ddr_debug_malloc_input_ddr_memory(hrc_drv);
			if (ret) {
				hrc_err("malloc input memory failed!\n");
				return ret;
			}
		}

		hrc_drv->param_flag |= DEBUG_INPUT_PARAM;
	} else if (!strncmp(buf, "output=", 7)) {
		ptr = (char *)buf + 7;

		for (i = 0; i < OUT_PARAM_NUM; i++) {
			ret = __get_user_input_value(&ptr, ", ", &value);
			if (ret)
				goto ERR_GET_USER_INPUT_VALUE;

			outputs[i] = (u32)value;
		}

		memset(output, 0, sizeof(*output));
		output->width         = outputs[0];
		output->height        = outputs[1];
		output->format        = outputs[2];
		output->cs            = outputs[3];
		output->quantization  = outputs[4];

		if (input->data_src == HRC_DATA_SRC_DDR) {
			ret = __ddr_debug_malloc_output_ddr_memory(hrc_drv);
			if (ret) {
				hrc_err("malloc input memory failed!\n");
				return ret;
			}
		}

		hrc_drv->param_flag |= DEBUG_OUTPUT_PARAM;
	} else {
		hrc_inf("clean parameters.\n");

		if (hrc_drv->param_flag & DEBUG_INPUT_PARAM)
			memset(input, 0, sizeof(*input));

		if (hrc_drv->param_flag & DEBUG_OUTPUT_PARAM)
			memset(output, 0, sizeof(*output));

		hrc_drv->param_flag = 0;
	}

	return count;

ERR_GET_USER_INPUT_VALUE:
	hrc_err("get value failed, buf: %s\n", buf);
	return ret;
}

static DEVICE_ATTR_RW(params);

/*  ddr debug:
 *  we can write the image data into DDR and then let HRC test itself.
 **/
static ssize_t ddr_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);

	n += sprintf(buf + n, "ddr_debug: %d ddr_debug_done: %d\n",
		     hrc_drv->ddr_debug, hrc_drv->ddr_debug_done);
	return n;
}

static ssize_t ddr_debug_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int ret;
	struct hrc_drv *hrc_drv = dev_get_drvdata(dev);

	if (buf[0] == '1') {
		if (!(hrc_drv->param_flag & DEBUG_INPUT_PARAM) ||
		    !(hrc_drv->param_flag & DEBUG_OUTPUT_PARAM)) {
			hrc_err("Please setup input and output parameter first!\n");
			return -EINVAL;
		}

		hrc_drv->ddr_debug = 1;
		hrc_drv->ddr_debug_done = 0;

		ret = sunxi_hrc_hardware_enable(hrc_drv->hrc_hdl);
		if (ret) {
			hrc_err("hrc enable error\n");
			return ret;
		}

		ret = sunxi_hrc_config_params(hrc_drv);
		if (ret) {
			hrc_err("sunxi_hrc_config_params error\n");
			return ret;
		}

		hrc_drv->apply_data.dirty |= HRC_APPLY_ADDR_DIRTY;
		ret = sunxi_hrc_hardware_apply(hrc_drv->hrc_hdl, &hrc_drv->apply_data);
		if (ret) {
			hrc_err("hrc apply address error\n");
			return ret;
		}

		sunxi_hrc_hardware_commit(hrc_drv->hrc_hdl);

		hrc_inf("start ddr debug...\n");
	} else {
		hrc_inf("close ddr debug, free memory!\n");

		sunxi_hrc_hardware_disable(hrc_drv->hrc_hdl);
		__ddr_debug_free_all_memory(hrc_drv);

		hrc_drv->ddr_debug = 0;
		hrc_drv->ddr_debug_done = 0;
		return count;
	}

	return count;
}

static DEVICE_ATTR_RW(ddr_debug);

static struct attribute *hrc_attributes[] = {
	&dev_attr_loglevel.attr,
	&dev_attr_reg_dump.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_dump.attr,
	&dev_attr_v4l2_debug.attr,
	&dev_attr_params.attr,
	&dev_attr_ddr_debug.attr,
	NULL,
};

static struct attribute_group hrc_attribute_group = {
	.name  = "attr",
	.attrs = hrc_attributes,
};

static int sunxi_hrc_driver_open(struct inode *inode, struct file *file)
{
	struct hrc_drv *hrc_drv = container_of(inode->i_cdev, struct hrc_drv, sys_cdev);

	file->private_data = hrc_drv;

	if (!(hrc_drv->param_flag & DEBUG_INPUT_PARAM) ||
	    !(hrc_drv->param_flag & DEBUG_OUTPUT_PARAM)) {
		hrc_err("Please setup input and output parameter first!\n");
		return -ENOMEM;
	}

	return 0;
}

static int sunxi_hrc_driver_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t sunxi_hrc_driver_write(struct file *file, const char __user *user, size_t count,
				      loff_t *ppos)
{
	struct hrc_drv *hrc_drv = file->private_data;
	ssize_t bytes = 0;

	if (*ppos >= hrc_drv->dbg_in_buf_size)
		return -ENOSPC;

	if (count > hrc_drv->dbg_in_buf_size - *ppos)
		count = hrc_drv->dbg_in_buf_size - *ppos;

	if (copy_from_user(hrc_drv->dbg_in_dma_buffer + *ppos, user, count))
		return -EFAULT;

	*ppos += count;
	bytes = count;

	return bytes;
}

static ssize_t sunxi_hrc_driver_read(struct file *file, char __user *user, size_t count,
				     loff_t *ppos)
{
	struct hrc_drv *hrc_drv = file->private_data;
	ssize_t bytes = 0;

	if (hrc_drv->ddr_debug_done) {
		/* return output ddr data */

		if (*ppos >= hrc_drv->dbg_out_buf_size)
			return 0;

		if (count > hrc_drv->dbg_out_buf_size - *ppos)
			count = hrc_drv->dbg_out_buf_size - *ppos;

		if (copy_to_user(user, hrc_drv->dbg_out_dma_buffer + *ppos, count))
			return -EFAULT;
	} else {
		/* return input ddr data */

		if (*ppos >= hrc_drv->dbg_in_buf_size)
			return 0;

		if (count > hrc_drv->dbg_in_buf_size - *ppos)
			count = hrc_drv->dbg_in_buf_size - *ppos;

		if (copy_to_user(user, hrc_drv->dbg_in_dma_buffer + *ppos, count))
			return -EFAULT;
	}

	*ppos += count;
	bytes = count;

	return bytes;
}

static const struct file_operations hrc_driver_fops = {
	.owner   = THIS_MODULE,
	.open    = sunxi_hrc_driver_open,
	.release = sunxi_hrc_driver_release,
	.write   = sunxi_hrc_driver_write,
	.read    = sunxi_hrc_driver_read,
};

static int sunxi_hrc_drv_resource_request(struct platform_device *pdev)
{
	int ret = 0;
	struct hrc_drv *hrc_drv = dev_get_drvdata(&pdev->dev);
	struct hrc_hw_create_info info;

	/* init lock */
	mutex_init(&hrc_drv->mlock);
	mutex_init(&mlock_rx_data);
	spin_lock_init(&hrc_drv->slock);
	INIT_LIST_HEAD(&hrc_drv->buf_list);
	INIT_LIST_HEAD(&hrc_drv->buf_active_list);

	/* get dts params */
	if (device_property_read_bool(&pdev->dev, "fpga")) {
		hrc_drv->fpga = 1;
		hrc_wrn("hrc use fpga mode!!!\n");
	} else {
		hrc_drv->fpga = 0;
	}

	if (device_property_read_bool(&pdev->dev, "output_to_disp2")) {
		hrc_drv->output_to_disp2 = 1;
		hrc_wrn("hrc output to disp2 framework!!!\n");
	} else {
		hrc_drv->output_to_disp2 = 0;
	}

	/* init dts clk */
	hrc_drv->clk_hrc = devm_clk_get(&pdev->dev, "clk_hrc");
	if (IS_ERR(hrc_drv->clk_hrc)) {
		hrc_err("get clk_hrc error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->clk_bus_hrc = devm_clk_get(&pdev->dev, "clk_bus_hrc");
	if (IS_ERR(hrc_drv->clk_bus_hrc)) {
		hrc_err("get clk_bus_hrc error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->clk_bus_hdmi_rx = devm_clk_get(&pdev->dev, "clk_bus_hdmi_rx");
	if (IS_ERR(hrc_drv->clk_bus_hdmi_rx)) {
		hrc_err("get clk_bus_hdmi_rx error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->rst_bus_hrc = devm_reset_control_get(&pdev->dev, "rst_bus_hrc");
	if (IS_ERR(hrc_drv->rst_bus_hrc)) {
		hrc_err("get rst_bus_hrc error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	hrc_drv->rst_bus_hdmi_rx = devm_reset_control_get(&pdev->dev, "rst_bus_hdmi_rx");
	if (IS_ERR(hrc_drv->rst_bus_hdmi_rx)) {
		hrc_err("get rst_bus_hdmi_rx error!\n");
		ret = -1;
		goto ERR_CLK_INIT;
	}

	sunxi_hrc_enable_resource(hrc_drv);

	/* TODO: init dts power */

	/* init irq */
	hrc_drv->irq = platform_get_irq(pdev, 0);
	if (hrc_drv->irq < 0) {
		hrc_err("platform_get_irq error!\n");
		ret = hrc_drv->irq;
		goto ERR_IRQ_INIT;
	}

	ret = devm_request_threaded_irq(&pdev->dev, hrc_drv->irq,
					sunxi_hrc_irq_handler, NULL,
					IRQF_SHARED, "hrc-irq", hrc_drv);
	if (ret < 0) {
		hrc_err("devm_request_threaded_irq error!\n");
		goto ERR_IRQ_INIT;
	}

	/* get hardware handle */
	info.pdev = pdev;
	info.version = hrc_drv->version;
	hrc_drv->hrc_hdl = sunxi_hrc_handle_create(&info);
	if (IS_ERR_OR_NULL(hrc_drv->hrc_hdl)) {
		hrc_err("sunxi_hrc_handle_create error!\n");
		ret = -EINVAL;
		goto ERR_DEVICE_INIT;
	}

	return 0;

ERR_IRQ_INIT:
ERR_DEVICE_INIT:
ERR_CLK_INIT:
	return ret;
}

static void sunxi_hrc_drv_resource_release(struct platform_device *pdev)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(&pdev->dev);

	sunxi_hrc_disable_resource(hrc_drv);
	sunxi_hrc_handle_destory(hrc_drv->hrc_hdl);
}

static const struct of_device_id hrc_of_match[] = {
	{ .compatible = "allwinner,sunxi-hrc-v110" },
	{ }
};
MODULE_DEVICE_TABLE(of, hrc_of_match);

static int sunxi_hrc_probe(struct platform_device *pdev)
{
	int ret;
	struct hrc_drv *hrc_drv;
	struct vb2_queue *vb2_q;
	struct video_device *video_dev;
	const struct of_device_id *match;

	hrc_dbg("\n");

	hrc_drv = devm_kzalloc(&pdev->dev, sizeof(*hrc_drv), GFP_KERNEL);
	if (!hrc_drv)
		return -ENOMEM;
	dev_set_drvdata(&pdev->dev, hrc_drv);
	hrc_drv->pdev = pdev;

	match = of_match_device(hrc_of_match, &pdev->dev);
	if (!match) {
		hrc_err("No matching device found!\n");
		ret = -EINVAL;
		goto ERR_DRVIER_INIT;
	}

	if (!strcmp(match->compatible, "allwinner,sunxi-hrc-v110")) {
		hrc_drv->version = 0x110;
	} else {
		hrc_err("No matching device found!\n");
		ret = -EINVAL;
		goto ERR_DRVIER_INIT;
	}
	hrc_inf("hrc version: %x\n", hrc_drv->version);

	/* driver resource */
	ret = sunxi_hrc_drv_resource_request(pdev);
	if (ret) {
		hrc_err("sunxi_hrc_drv_resource_request error!\n");
		goto ERR_DRVIER_INIT;
	}

	/* v4l2 device */
	snprintf(hrc_drv->v4l2_dev.name, sizeof(hrc_drv->v4l2_dev.name), "%s_v4l2", HRC_DRV_NAME);

	ret = v4l2_device_register(&pdev->dev, &hrc_drv->v4l2_dev);
	if (ret < 0) {
		hrc_err("v4l2_device_register error: %d\n", ret);
		goto ERR_V4L2_DEVICE_REGISTER;
	}

	/* vb2 */
	vb2_q                  = &hrc_drv->vb2_q;
	vb2_q->type            = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	vb2_q->io_modes        = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	vb2_q->drv_priv        = hrc_drv;
	vb2_q->buf_struct_size = sizeof(struct hrc_buffer);
	vb2_q->ops             = &vb2_hrc_ops;
	vb2_q->mem_ops         = &vb2_dma_contig_memops;
	vb2_q->lock            = &hrc_drv->mlock;
	vb2_q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	vb2_q->dev             = hrc_drv->v4l2_dev.dev;
	ret = vb2_queue_init(vb2_q);
	if (ret) {
		hrc_err("vb2_queue_init error: %d\n", ret);
		goto ERR_VB2_QUEUE_INIT_ERR;
	}

	/* video device */
	video_dev = video_device_alloc();
	if (!video_dev) {
		hrc_err("video_device_alloc error!\n");
		goto ERR_VIDEO_DEVICE_ALLOC;
	}
	hrc_drv->video_dev = video_dev;
	snprintf(video_dev->name, sizeof(video_dev->name), "%s_vid", HRC_DRV_NAME);

	hrc_drv->caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;

	video_dev->fops        = &v4l2_hrc_fops;
	video_dev->ioctl_ops   = &v4l2_hrc_ioctl_ops;
	video_dev->device_caps = hrc_drv->caps;
	video_dev->release     = video_device_release_empty;
	video_dev->v4l2_dev    = &hrc_drv->v4l2_dev;
	video_dev->queue       = &hrc_drv->vb2_q;
	video_dev->lock        = &hrc_drv->mlock;
	video_dev->flags       = V4L2_FL_USES_V4L2_FH;

	video_set_drvdata(video_dev, hrc_drv);

	ret = video_register_device(video_dev, VFL_TYPE_VIDEO, -1);
	if (ret < 0) {
		hrc_err("video_register_device error: %d\n", ret);
		goto ERR_VIDEO_REGISTER_DEVICE;
	}

	/* subdev */
	v4l2_subdev_init(&v4l2_sd, &subdev_hrc_ops);
	snprintf(v4l2_sd.name, sizeof(v4l2_sd.name), "%s", HRC_DRV_NAME);
	v4l2_set_subdevdata(&v4l2_sd, hrc_drv);
	v4l2_sd.grp_id = (0x1 << 8);
	v4l2_sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;

	ret = v4l2_device_register_subdev(&hrc_drv->v4l2_dev, &v4l2_sd);
	if (ret) {
		hrc_err("v4l2_device_register_subdev error: %d\n", ret);
		goto ERR_REGISTER_SUBDEV;
	}

	ret = v4l2_device_register_subdev_nodes(&hrc_drv->v4l2_dev);
	if (ret) {
		hrc_err("v4l2_device_register_subdev_nodes error: %d\n", ret);
		goto ERR_REGISTER_SUBDEV_NODE;
	}

	/* workqueue */
	hrc_drv->buf_wq = create_workqueue("hrc_buf_workqueue");
	if (!hrc_drv->buf_wq) {
		hrc_err("create_workqueue error!\n");
		goto ERR_WORKQUEUE_CREATE;
	}
	INIT_WORK(&hrc_drv->buf_work, sunxi_hrc_wait_buf_queue);
	init_waitqueue_head(&hrc_drv->buf_wait_queue);

	/* sysfs */
	alloc_chrdev_region(&hrc_drv->sys_devid, 0, 1, "hrc");

	cdev_init(&hrc_drv->sys_cdev, &hrc_driver_fops);
	if (cdev_add(&hrc_drv->sys_cdev, hrc_drv->sys_devid, 1)) {
		hrc_err("cdev_add failed!\n");
		ret = -1;
		goto ERR_CDEV_ADD;
	}

	hrc_drv->sys_class = class_create("hrc");
	if (IS_ERR(hrc_drv->sys_class)) {
		hrc_err("class_create failed!\n");
		ret = -1;
		goto ERR_CLASS_CREATE;
	}

	hrc_drv->sys_device = device_create(hrc_drv->sys_class, &pdev->dev,
					    hrc_drv->sys_devid, hrc_drv, "hrc");
	if (!hrc_drv->sys_device) {
		hrc_err("device_create failed!\n");
		ret = -1;
		goto ERR_DEVICE_CREATE;
	}

	ret = sysfs_create_group(&hrc_drv->sys_device->kobj, &hrc_attribute_group);
	if (ret) {
		hrc_err("sysfs_create_group error: %d\n", ret);
		goto ERR_REGISTER_SUBDEV_NODE;
	}

	return 0;

ERR_REGISTER_SUBDEV_NODE:
	device_destroy(hrc_drv->sys_class, hrc_drv->sys_devid);
ERR_DEVICE_CREATE:
	class_destroy(hrc_drv->sys_class);
ERR_CLASS_CREATE:
	cdev_del(&hrc_drv->sys_cdev);
ERR_CDEV_ADD:
	cancel_work_sync(&hrc_drv->buf_work);
	destroy_workqueue(hrc_drv->buf_wq);
ERR_WORKQUEUE_CREATE:
	v4l2_set_subdevdata(&v4l2_sd, NULL);
	v4l2_device_unregister_subdev(&v4l2_sd);
ERR_REGISTER_SUBDEV:
	video_unregister_device(video_dev);
ERR_VIDEO_REGISTER_DEVICE:
	video_device_release(video_dev);
ERR_VIDEO_DEVICE_ALLOC:
	vb2_queue_release(vb2_q);
ERR_VB2_QUEUE_INIT_ERR:
	v4l2_device_unregister(&hrc_drv->v4l2_dev);
ERR_V4L2_DEVICE_REGISTER:
	sunxi_hrc_drv_resource_release(pdev);
ERR_DRVIER_INIT:
	devm_kfree(&pdev->dev, hrc_drv);
	return ret;
}

static int sunxi_hrc_remove(struct platform_device *pdev)
{
	struct hrc_drv *hrc_drv = dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&hrc_drv->sys_device->kobj, &hrc_attribute_group);
	device_destroy(hrc_drv->sys_class, hrc_drv->sys_devid);
	class_destroy(hrc_drv->sys_class);
	cdev_del(&hrc_drv->sys_cdev);

	cancel_work_sync(&hrc_drv->buf_work);
	destroy_workqueue(hrc_drv->buf_wq);

	v4l2_set_subdevdata(&v4l2_sd, NULL);
	v4l2_device_unregister_subdev(&v4l2_sd);
	video_unregister_device(hrc_drv->video_dev);
	video_device_release(hrc_drv->video_dev);
	vb2_queue_release(&hrc_drv->vb2_q);
	v4l2_device_unregister(&hrc_drv->v4l2_dev);

	sunxi_hrc_drv_resource_release(pdev);

	devm_kfree(&pdev->dev, hrc_drv);
	return 0;
}

static struct platform_driver hrc_pdrv = {
	.probe  = sunxi_hrc_probe,
	.remove = sunxi_hrc_remove,
	.driver = {
		.name = "allwinner,sunxi-hrc",
		.owner = THIS_MODULE,
		.of_match_table = hrc_of_match,
	},
};

/* define our own notifier_call_chain */
int sunxi_hrc_call_notifiers(unsigned long val, void *data)
{
	return raw_notifier_call_chain(&hrc_chain, val, data);
}
EXPORT_SYMBOL_GPL(sunxi_hrc_call_notifiers);

/* define our own notifier_chain_register func */
static int sunxi_hrc_register_notifier(struct notifier_block *nb)
{
	int err = raw_notifier_chain_register(&hrc_chain, nb);

	if (err)
		return err;
	return 0;
}

static int sunxi_hrc_queue_hpd_event(int id)
{
	struct v4l2_event ev;
	u32 change;

	if (!(subdev_event_subscribe & BIT(V4L2_EVENT_SOURCE_CHANGE)))
		return 0;

	mutex_lock(&mlock_rx_data);
	if (receive_data.video[id].is_5v)
		change = V4L2_EVENT_SRC_CH_HPD_IN;
	else
		change = V4L2_EVENT_SRC_CH_HPD_OUT;
	mutex_unlock(&mlock_rx_data);

	memset(&ev, 0, sizeof(ev));
	ev.id                   = id;
	ev.type                 = V4L2_EVENT_SOURCE_CHANGE;
	ev.u.src_change.changes = change;
	v4l2_subdev_notify_event(&v4l2_sd, &ev);
	return 0;
}

static int sunxi_hrc_queue_timing_event(int id)
{
	struct v4l2_event ev;

	if (!(subdev_event_subscribe & BIT(V4L2_EVENT_SOURCE_CHANGE)))
		return 0;

	memset(&ev, 0, sizeof(ev));
	ev.id                   = id;
	ev.type                 = V4L2_EVENT_SOURCE_CHANGE;
	ev.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION;
	v4l2_subdev_notify_event(&v4l2_sd, &ev);
	return 0;
}

static int sunxi_hrc_queue_asr_event(void)
{
	struct v4l2_event ev;
	u32 change;

	if (!(subdev_event_subscribe & BIT(V4L2_EVENT_SOURCE_CHANGE)))
		return 0;

	mutex_lock(&mlock_rx_data);
	switch (receive_data.audio.audio_sample_rate) {
	case 22000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_22K;
		break;
	case 24000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_24K;
		break;
	case 32000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_32K;
		break;
	case 44100:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_44K1;
		break;
	case 48000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_48K;
		break;
	case 88000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_88K;
		break;
	case 96000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_96K;
		break;
	case 176400:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_176K4;
		break;
	case 192000:
		change = V4L2_EVENT_SRC_CH_AUD_SAMPLING_RATE_192K;
		break;
	default:
		hrc_err("Unsupported audio sample rate: %d!!!\n",
			receive_data.audio.audio_sample_rate);
		mutex_unlock(&mlock_rx_data);
		return -EINVAL;
	}
	mutex_unlock(&mlock_rx_data);

	memset(&ev, 0, sizeof(ev));
	ev.id                   = 0;
	ev.type                 = V4L2_EVENT_SOURCE_CHANGE;
	ev.u.src_change.changes = change;
	v4l2_subdev_notify_event(&v4l2_sd, &ev);
	return 0;
}

int sunxi_hrc_init_event(struct notifier_block *nb, unsigned long event, void *data)
{
	int ret;
	struct hrc_from_hdmirx_signal *signal_info;
	struct hrc_from_hdmirx_5V *hpd_info;
	u32 *fs;
	int port_id;

	if (IS_ERR_OR_NULL(nb) || IS_ERR_OR_NULL(data))
		return -EINVAL;

	switch (event) {
	case SIGNAL_CHANGE:
		signal_info = (struct hrc_from_hdmirx_signal *)data;

		ret = __source_id_to_port_id(signal_info->source_id, &port_id);
		if (ret) {
			hrc_err("Unknow source id: %d\n", signal_info->source_id);
			return ret;
		}

		if (port_id > HDMI_MAX_INPUT_PORT) {
			hrc_err("port_id: %d MAX_PORT: %d\n", port_id, HDMI_MAX_INPUT_PORT);
			return -EINVAL;
		}

		hrc_inf("get new signal from hdmirx id %d:\n", port_id);
		hrc_inf("width: %d, height: %d, format: 0x%x, depth: 0x%x\n",
			signal_info->timings.width, signal_info->timings.height,
			signal_info->format, signal_info->depth);
		hrc_inf("color_space: 0x%x, quantization: 0x%x interlaced: 0x%x\n",
			signal_info->csc, signal_info->quantization,
			signal_info->timings.interlaced);

		mutex_lock(&mlock_rx_data);
		memset(&receive_data.video[port_id].input, 0,
		       sizeof(receive_data.video[port_id].input));
		receive_data.video[port_id].input.data_src = HRC_DATA_SRC_HDMI_RX;
		if (signal_info->timings.interlaced) {
			receive_data.video[port_id].input.field_mode = HRC_FIELD_MODE_FIELD;
			receive_data.video[port_id].input.field_inverse = 1;
			receive_data.video[port_id].input.field_order = HRC_FIELD_ORDER_BOTTOM;
			/* HRC will write back the two fields and output one frame */
			signal_info->timings.height *= 2;
		} else {
			receive_data.video[port_id].input.field_mode = HRC_FIELD_MODE_FRAME;
			receive_data.video[port_id].input.field_inverse = 0;
			receive_data.video[port_id].input.field_order = HRC_FIELD_ORDER_BOTTOM;
		}
		receive_data.video[port_id].input.width = signal_info->timings.width;
		receive_data.video[port_id].input.height = signal_info->timings.height;
		receive_data.video[port_id].input.format = signal_info->format;
		receive_data.video[port_id].input.depth = signal_info->depth;
		receive_data.video[port_id].input.cs = signal_info->csc;
		receive_data.video[port_id].input.quantization = signal_info->quantization;

		memcpy(&receive_data.video[port_id].timings, &signal_info->timings,
		       sizeof(receive_data.video[port_id].timings));
		mutex_unlock(&mlock_rx_data);

		sunxi_hrc_queue_timing_event(port_id);
		break;

	case HPD_CHANGE:
		hpd_info = (struct hrc_from_hdmirx_5V *)data;
		hrc_inf("source plug stats is %d id = %d\n", hpd_info->is5V, hpd_info->PortID);

		port_id = hpd_info->PortID - 1;
		if (port_id > HDMI_MAX_INPUT_PORT) {
			hrc_err("port_id: %d MAX_PORT: %d\n", port_id, HDMI_MAX_INPUT_PORT);
			return -EINVAL;
		}

		mutex_lock(&mlock_rx_data);
		receive_data.video[port_id].is_5v = hpd_info->is5V;
		receive_data.video[port_id].id    = hpd_info->PortID;
		mutex_unlock(&mlock_rx_data);

		sunxi_hrc_queue_hpd_event(port_id);
		break;

	case AUDIO_FSRATE_CHANGE:
		fs = (u32 *)data;
		hrc_inf("audio sample rate is %d\n", *fs);

		mutex_lock(&mlock_rx_data);
		receive_data.audio.audio_sample_rate = *fs;
		mutex_unlock(&mlock_rx_data);

		sunxi_hrc_queue_asr_event();
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

/* define a notifier_block */
static struct notifier_block hrc_init_notifier = {
	.notifier_call = sunxi_hrc_init_event,
};

static int __init sunxi_hrc_module_init(void)
{
	int ret;

	sunxi_hrc_register_notifier(&hrc_init_notifier);

	ret = platform_driver_register(&hrc_pdrv);
	if (ret) {
		hrc_err("platform_driver_register error!\n");
		return ret;
	}

	return 0;
}

static void __exit sunxi_hrc_module_exit(void)
{
	platform_driver_unregister(&hrc_pdrv);
}

late_initcall(sunxi_hrc_module_init);
module_exit(sunxi_hrc_module_exit);
