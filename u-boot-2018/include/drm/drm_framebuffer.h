/*
 * drm_framebuffer.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _DRM_FRAMEBUFFER_H
#define _DRM_FRAMEBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <drm/drm_fourcc.h>
#include <linux/list.h>

struct sunxi_drm_device;
struct drm_mode_fb_cmd2;

struct drm_framebuffer {
	int fb_id;
	/**
	 * @dev: DRM device this framebuffer belongs to
	 */
	struct sunxi_drm_device *drm;
	/**
	 * @head: Place on the &drm_mode_config.fb_list, access protected by
	 * &drm_mode_config.fb_lock.
	 */
	struct list_head head;
	/**
	 * @format: framebuffer format information
	 */
	const struct drm_format_info *format;

	//start address of framebuffer
	unsigned long dma_addr;
	//size of memory in Byte
	unsigned int buf_size;
	/**
	 * @pitches: Line stride per buffer. For userspace created object this
	 * is copied from drm_mode_fb_cmd2.
	 */
	unsigned int pitches[DRM_FORMAT_MAX_PLANES];
	/**
	 * @offsets: Offset from buffer start to the actual pixel data in bytes,
	 * per buffer. For userspace created object this is copied from
	 * drm_mode_fb_cmd2.
	 *
	 * Note that this is a linear offset and does not take into account
	 * tiling or buffer layout per @modifier. It is meant to be used when
	 * the actual pixel data for this framebuffer plane starts at an offset,
	 * e.g. when multiple planes are allocated within the same backing
	 * storage buffer object. For tiled layouts this generally means its
	 * @offsets must at least be tile-size aligned, but hardware often has
	 * stricter requirements.
	 *
	 * This should not be used to specifiy x/y pixel offsets into the buffer
	 * data (even for linear buffers). Specifying an x/y pixel offset is
	 * instead done through the source rectangle in &struct drm_plane_state.
	 */
	unsigned int offsets[DRM_FORMAT_MAX_PLANES];
	/**
	 * @modifier: Data layout modifier. This is used to describe
	 * tiling, or also special layouts (like compression) of auxiliary
	 * buffers. For userspace created object this is copied from
	 * drm_mode_fb_cmd2.
	 */
	uint64_t modifier;
	/**
	 * @width: Logical width of the visible area of the framebuffer, in
	 * pixels.
	 */
	unsigned int width;
	/**
	 * @height: Logical height of the visible area of the framebuffer, in
	 * pixels.
	 */
	unsigned int height;
	/**
	 * @flags: Framebuffer flags like DRM_MODE_FB_INTERLACED or
	 * DRM_MODE_FB_MODIFIERS.
	 */
	int flags;
	/**
	 * @hot_x: X coordinate of the cursor hotspot. Used by the legacy cursor
	 * IOCTL when the driver supports cursor through a DRM_PLANE_TYPE_CURSOR
	 * universal plane.
	 */
	int hot_x;
	/**
	 * @hot_y: Y coordinate of the cursor hotspot. Used by the legacy cursor
	 * IOCTL when the driver supports cursor through a DRM_PLANE_TYPE_CURSOR
	 * universal plane.
	 */
	int hot_y;
};

struct drm_framebuffer *drm_fb_lock(void);
int drm_framebuffer_alloc(struct sunxi_drm_device *drm, struct drm_mode_fb_cmd2 *cmd);
int drm_framebuffer_free(struct sunxi_drm_device *drm, struct drm_framebuffer *fb);
struct drm_framebuffer *drm_framebuffer_lookup(struct sunxi_drm_device *drm, int fb_id);
int drm_framebuffer_plane_width(int width,
				const struct drm_framebuffer *fb, int plane);
int drm_framebuffer_plane_height(int height,
				 const struct drm_framebuffer *fb, int plane);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
