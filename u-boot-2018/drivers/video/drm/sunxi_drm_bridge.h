/*
 * sunxi_drm_bridge.h
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
#ifndef _SUNXI_DRM_BRIDGE_H
#define _SUNXI_DRM_BRIDGE_H

#include <config.h>
#include <common.h>
#include <dm/device.h>
#include <errno.h>

struct display_state;
struct sunxi_drm_bridge;
struct drm_display_mode;
struct sunxi_drm_connector;

struct sunxi_drm_bridge_funcs {
	void (*enable)(struct sunxi_drm_bridge *bridge);
	void (*disable)(struct sunxi_drm_bridge *bridge);
	void (*pre_enable)(struct sunxi_drm_bridge *bridge);
	void (*post_disable)(struct sunxi_drm_bridge *bridge);
	void (*mode_set)(struct sunxi_drm_bridge *bridge,
			 const struct drm_display_mode *mode);
	bool (*detect)(struct sunxi_drm_bridge *bridge);
	void (*get_mode)(struct sunxi_drm_bridge *bridge, struct drm_display_mode *mode);
};

struct sunxi_drm_bridge {
	struct udevice *dev;
	const struct sunxi_drm_bridge_funcs *funcs;
	struct sunxi_drm_bridge *next_bridge;
	struct sunxi_drm_connector *conn;
	struct display_state *state;
};

void sunxi_drm_bridge_init(struct sunxi_drm_bridge *bridge,
			  struct sunxi_drm_connector *conn,
			  struct display_state *state);
void sunxi_drm_bridge_enable(struct sunxi_drm_bridge *bridge);
void sunxi_drm_bridge_disable(struct sunxi_drm_bridge *bridge);
void sunxi_drm_bridge_pre_enable(struct sunxi_drm_bridge *bridge);
void sunxi_drm_bridge_post_disable(struct sunxi_drm_bridge *bridge);
void sunxi_drm_bridge_mode_set(struct sunxi_drm_bridge *bridge,
			      const struct drm_display_mode *mode);
bool sunxi_drm_bridge_detect(struct sunxi_drm_bridge *bridge);



#endif /*End of file*/
