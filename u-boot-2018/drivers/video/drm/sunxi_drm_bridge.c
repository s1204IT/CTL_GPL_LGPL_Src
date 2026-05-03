/*
 * drivers/video/drm/sunxi_drm_bridge/sunxi_drm_bridge.c
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

#include "sunxi_drm_bridge.h"

void sunxi_drm_bridge_init(struct sunxi_drm_bridge *bridge,
			  struct sunxi_drm_connector *conn,
			  struct display_state *state)
{
	if (!bridge)
		return;

	bridge->conn = conn;
	bridge->state = state;

	if (bridge->next_bridge)
		sunxi_drm_bridge_init(bridge->next_bridge, conn, state);
}

void sunxi_drm_bridge_pre_enable(struct sunxi_drm_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->funcs && bridge->funcs->pre_enable)
		bridge->funcs->pre_enable(bridge);

	if (bridge->next_bridge)
		sunxi_drm_bridge_pre_enable(bridge->next_bridge);
}

void sunxi_drm_bridge_post_disable(struct sunxi_drm_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->next_bridge)
		sunxi_drm_bridge_post_disable(bridge->next_bridge);

	if (bridge->funcs && bridge->funcs->post_disable)
		bridge->funcs->post_disable(bridge);
}

void sunxi_drm_bridge_enable(struct sunxi_drm_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->funcs && bridge->funcs->enable)
		bridge->funcs->enable(bridge);

	if (bridge->next_bridge)
		sunxi_drm_bridge_enable(bridge->next_bridge);
}

void sunxi_drm_bridge_disable(struct sunxi_drm_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->next_bridge)
		sunxi_drm_bridge_disable(bridge->next_bridge);

	if (bridge->funcs && bridge->funcs->disable)
		bridge->funcs->disable(bridge);
}

void sunxi_drm_bridge_mode_set(struct sunxi_drm_bridge *bridge,
			      const struct drm_display_mode *mode)
{
	if (!bridge || !mode)
		return;

	if (bridge->funcs && bridge->funcs->mode_set)
		bridge->funcs->mode_set(bridge, mode);

	if (bridge->next_bridge)
		sunxi_drm_bridge_mode_set(bridge->next_bridge, mode);
}

bool sunxi_drm_bridge_detect(struct sunxi_drm_bridge *bridge)
{
	if (bridge->funcs && bridge->funcs->detect)
		if (!bridge->funcs->detect(bridge))
			return false;

	if (bridge->next_bridge)
		return sunxi_drm_bridge_detect(bridge->next_bridge);

	return true;
}
