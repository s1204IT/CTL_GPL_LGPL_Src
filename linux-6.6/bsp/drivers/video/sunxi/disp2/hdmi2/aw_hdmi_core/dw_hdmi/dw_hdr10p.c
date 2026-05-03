/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <video/sunxi_metadata.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>

#include "../../aw_hdmi_define.h"
#include "dw_video.h"
#include "dw_audio.h"
#include "dw_fc.h"
#include "dw_edid.h"
#include "dw_access.h"

#include "dw_hdr10p.h"

u32 mymin(u32 left, u32 right)
{
	if (left < right)
		return left;
	else
		return (right);
}

u32 myround(u32 in, u32 div)
{
	u32 res = 0;
	if (div == 0)
		return in;
	res = in % div;
	if (res >= (div / 2))
		return in / div + 1;
	else
		return in / div;
}

int dw_hdr10p_configure(dw_hdmi_dev_t *dev, void *config,
	dw_video_param_t *video, dw_product_param_t *prod,
	struct disp_device_dynamic_config *scfg)
{
#if defined(USE_AW_HDMI_HDR10P)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct dma_buf_map map;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map;
#endif

	if (video->mHdmi == 0) {
		hdmi_inf("DVI mode selected: packets not configured\n");
		return -1;
	}

	if (config == NULL)  {
		u32 oui = 0;
		u8 struct_3d = 0;
		u8 data[4];
		u8 *vendor_payload = prod->mVendorPayload;
		u8 payload_length = prod->mVendorPayloadLength;
		if (video->mHdmiVideoFormat == 2) {
			struct_3d = video->m3dStructure;
			VIDEO_INF("3D packets configure\n");

			/* frame packing || tab || sbs */
			if ((struct_3d == 0) || (struct_3d == 6) || (struct_3d == 8)) {
				data[0] = video->mHdmiVideoFormat << 5; /* PB4 */
				data[1] = struct_3d << 4; /* PB5 */
				data[2] = video->m3dExtData << 4;
				data[3] = 0;
				/* HDMI Licensing, LLC */
				_dw_packet_vsi_config(dev, 0x000C03, data, sizeof(data), 1);
				_dw_vsi_enable(dev, 1);
			} else {
				hdmi_err("Error:3D structure not supported %d\n",
					   struct_3d);
				return -1;
			}
		} else if ((video->mHdmiVideoFormat == 0x1) || (video->mHdmiVideoFormat == 0x0)) {
			if (prod != 0) {
				fc_spd_info_t spd_data;

				spd_data.vName = prod->mVendorName;
				spd_data.vLength = prod->mVendorNameLength;
				spd_data.pName = prod->mProductName;
				spd_data.pLength = prod->mProductNameLength;
				spd_data.code = prod->mSourceType;
				spd_data.autoSend = 1;

				oui = prod->mOUI;
				_dw_packet_spd_config(dev, &spd_data);
				_dw_packet_vsi_config(dev, oui, vendor_payload, payload_length, 1);
				_dw_vsi_enable(dev, 1);
			} else {
				VIDEO_INF("No product info provided: not configured\n");
			}
		} else {
			hdmi_inf("error: unknow video format\n");
			_dw_vsi_enable(dev, 0);
		}

		_dw_packets_metadata_config(dev);

		/* default phase 1 = true */
		dev_write_mask(dev, FC_GCP, FC_GCP_DEFAULT_PHASE_MASK,
					   ((video->mPixelPackingDefaultPhase == 1) ? 1 : 0));

		_dw_gamut_config(dev);
		return 0;
	}

	if (prod != 0 && config != NULL) {
		u32 oui = 0x90848B;
		u8 vendor_payload[24];
		u8 payload_length = 24;
		int i = 0;
		int ret = 0;
		fc_spd_info_t spd_data;
		void *hdr_buff_addr;
		struct dma_buf *dmabuf;
		struct hdr_static_metadata *hdr_smetadata;
		struct sunxi_metadata *pMeta = (struct sunxi_metadata *) config;
		u8 *temp = kmalloc(scfg->metadata_size, GFP_KERNEL);
		dw_fc_drm_pb_t *dynamic_pb = kmalloc(sizeof(dw_fc_drm_pb_t), GFP_KERNEL);

		spd_data.vName = prod->mVendorName;
		spd_data.vLength = prod->mVendorNameLength;
		spd_data.pName = prod->mProductName;
		spd_data.pLength = prod->mProductNameLength;
		spd_data.code = prod->mSourceType;
		spd_data.autoSend = 1;

		memcpy(vendor_payload, (u8 *)&pMeta->divLut[
			   NUM_DIV - 1][MAX_LUT_SIZE - 1 - 24], 24);

		VIDEO_INF("hdr10p get maximu lumin=%d\n",
				  pMeta->hdr10_plus_smetada.
				  targeted_system_display_maximum_luminance);
		VIDEO_INF("hdr10p get maxrgb=%d\n",
				  pMeta->hdr10_plus_smetada.average_maxrgb);
		for (i = 0; i < 10; i++) {
			VIDEO_INF("%d dist val=%d\n", i,
					  pMeta->hdr10_plus_smetada.distribution_values[i]);
		}
		VIDEO_INF("hdr10p get knee_point_x=%d\n",
				  pMeta->hdr10_plus_smetada.knee_point_x);
		VIDEO_INF("hdr10p get knee_point_y=%d\n",
				  pMeta->hdr10_plus_smetada.knee_point_y);
		VIDEO_INF("hdr10p get num curve=%d\n",
				  pMeta->hdr10_plus_smetada.num_bezier_curve_anchors);
		for (i = 0; i < 9; i++) {
			VIDEO_INF("%d,bezier=%d\n", i,
					  pMeta->hdr10_plus_smetada.bezier_curve_anchors[i]);
		}

		for (i = 0; i < 24; i++) {
			VIDEO_INF("output pb%d=%x\n", 4 + i, vendor_payload[i]);
		}

		/*get the virtual addr of metadata*/
		dmabuf = dma_buf_get(scfg->metadata_fd);
		if (IS_ERR(dmabuf)) {
			hdmi_inf("dma_buf_get failed\n");
			kfree(temp);
			kfree(dynamic_pb);
			return -1;
		}

		ret = dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
		if (ret) {
			dma_buf_put(dmabuf);
			hdmi_inf("dmabuf cpu aceess failed\n");
			kfree(temp);
			kfree(dynamic_pb);
			return ret;
		}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		ret = dma_buf_vmap(dmabuf, &map);
		hdr_buff_addr = map.vaddr;
		if (ret) {
			hdr_buff_addr = NULL;
		}
#elif LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		hdr_buff_addr = dma_buf_vmap(dmabuf);
#else  /* 5.4 */
		hdr_buff_addr = dma_buf_kmap(dmabuf);
#endif
		if (!hdr_buff_addr) {
			hdmi_inf("dma_buf_kmap failed\n");
			dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
			dma_buf_put(dmabuf);
			kfree(temp);
			kfree(dynamic_pb);
			return -1;
		}

		/*obtain metadata*/
		memcpy((void *)temp, hdr_buff_addr, scfg->metadata_size);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		dma_buf_vunmap(dmabuf, &map);
#elif LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
		dma_buf_vunmap(dmabuf, hdr_buff_addr);
#else
		dma_buf_kunmap(dmabuf, 0, dmabuf);
#endif
		dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
		dma_buf_put(dmabuf);

		dynamic_pb->eotf = video->pb->eotf;
		dynamic_pb->metadata = video->pb->metadata;

		hdr_smetadata = (struct hdr_static_metadata *)temp;
		dynamic_pb->r_x =
			hdr_smetadata->disp_master.display_primaries_x[0];
		dynamic_pb->r_y =
			hdr_smetadata->disp_master.display_primaries_y[0];
		dynamic_pb->g_x =
			hdr_smetadata->disp_master.display_primaries_x[1];
		dynamic_pb->g_y =
			hdr_smetadata->disp_master.display_primaries_y[1];
		dynamic_pb->b_x =
			hdr_smetadata->disp_master.display_primaries_x[2];
		dynamic_pb->b_y =
			hdr_smetadata->disp_master.display_primaries_y[2];
		dynamic_pb->w_x =
			hdr_smetadata->disp_master.white_point_x;
		dynamic_pb->w_y =
			hdr_smetadata->disp_master.white_point_y;
		dynamic_pb->luma_max =
			hdr_smetadata->disp_master.max_display_mastering_luminance
			/ 10000;
		dynamic_pb->luma_min =
			hdr_smetadata->disp_master.min_display_mastering_luminance;
		dynamic_pb->mcll =
			hdr_smetadata->maximum_content_light_level;
		dynamic_pb->mfll =
			hdr_smetadata->maximum_frame_average_light_level;

		/*send dynamic metadata*/
		_dw_packet_spd_config(dev, &spd_data);
		_dw_packet_vsi_config(dev, oui, vendor_payload, (payload_length), 1);
		dw_fc_vsif_config(dev, 1);

		/*send static metadata*/
		dw_drm_packet_up(dev, dynamic_pb);

		kfree(temp);
		kfree(dynamic_pb);
	} else {
		VIDEO_INF("No product info provided: not configured\n");
		return -1;
	}

	/* _dw_packets_metadata_config(dev); */
	/* default phase 1 = true */
	/* dw_write_mask(FC_GCP, FC_GCP_DEFAULT_PHASE_MASK,
			((video->mPixelPackingDefaultPhase == 1) ? 1 : 0));
	_dw_gamut_config(dev); */
#endif /* USE_AW_HDMI_HDR10P */
	return 0;
}

