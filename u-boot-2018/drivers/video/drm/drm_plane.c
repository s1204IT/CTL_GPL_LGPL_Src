/*
 * drm_plane/drm_plane.c
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

#include <drm/drm_plane.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_connector.h>
#include "sunxi_drm_connector.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_drv.h"



static int __drm_universal_plane_init(struct sunxi_drm_device *drm,
				      struct drm_plane *plane,
				      uint32_t possible_crtcs,
				      const struct drm_plane_funcs *funcs,
				      const uint32_t *formats,
				      unsigned int format_count,
				      const uint64_t *format_modifiers,
				      enum drm_plane_type type,
				      const char *name)
{
	static const uint64_t default_modifiers[] = {
		DRM_FORMAT_MOD_LINEAR,
	};
	unsigned int format_modifier_count = 0;


	if (drm->num_total_plane > 64) {
		pr_err("Too many plane :%d\n", drm->num_total_plane);
		return -EINVAL;
	}

	/*
	 * First driver to need more than 64 formats needs to fix this. Each
	 * format is encoded as a bit and the current code only supports a u64.
	 */
	if (format_count > 64) {
		pr_err("Too many format count:%d\n", format_count);
		return -EINVAL;
	}


	mutex_init(&plane->mutex);

	plane->drm = drm;
	plane->funcs = funcs;
	plane->format_types = kmalloc_array(format_count, sizeof(uint32_t),
					    GFP_KERNEL);
	if (!plane->format_types) {
		pr_err("out of memory when allocating plane\n");
		return -ENOMEM;
	}

	if (format_modifiers) {
		const uint64_t *temp_modifiers = format_modifiers;

		while (*temp_modifiers++ != DRM_FORMAT_MOD_INVALID)
			format_modifier_count++;
	} else {
		format_modifiers = default_modifiers;
		format_modifier_count = ARRAY_SIZE(default_modifiers);
	}


	plane->modifier_count = format_modifier_count;
	plane->modifiers = kmalloc_array(format_modifier_count,
					 sizeof(format_modifiers[0]),
					 GFP_KERNEL);

	if (format_modifier_count && !plane->modifiers) {
		pr_err("out of memory when allocating plane\n");
		kfree(plane->format_types);
		return -ENOMEM;
	}
	plane->name = kmalloc(strlen(name) + 1, __GFP_ZERO);

	if (!plane->name) {
		kfree(plane->format_types);
		kfree(plane->modifiers);
		return -ENOMEM;
	} else {
		sprintf(plane->name, "%s", name);
	}

	memcpy(plane->format_types, formats, format_count * sizeof(uint32_t));
	plane->format_count = format_count;
	memcpy(plane->modifiers, format_modifiers,
	       format_modifier_count * sizeof(format_modifiers[0]));
	plane->possible_crtcs = possible_crtcs;
	plane->type = type;

	list_add_tail(&plane->head, &drm->plane_list);
	plane->index = drm->num_total_plane++;

	return 0;
}

/**
 * drm_universal_plane_init - Initialize a new universal plane object
 * @dev: DRM device
 * @plane: plane object to init
 * @possible_crtcs: bitmask of possible CRTCs
 * @funcs: callbacks for the new plane
 * @formats: array of supported formats (DRM_FORMAT\_\*)
 * @format_count: number of elements in @formats
 * @format_modifiers: array of struct drm_format modifiers terminated by
 *                    DRM_FORMAT_MOD_INVALID
 * @type: type of plane (overlay, primary, cursor)
 * @name: printf style format string for the plane name, or NULL for default name
 *
 * Initializes a plane object of type @type. The &drm_plane_funcs.destroy hook
 * should call drm_plane_cleanup() and kfree() the plane structure. The plane
 * structure should not be allocated with devm_kzalloc().
 *
 * Note: consider using drmm_universal_plane_alloc() instead of
 * drm_universal_plane_init() to let the DRM managed resource infrastructure
 * take care of cleanup and deallocation.
 *
 * Drivers that only support the DRM_FORMAT_MOD_LINEAR modifier support may set
 * @format_modifiers to NULL. The plane will advertise the linear modifier.
 *
 * Returns:
 * Zero on success, error code on failure.
 */
int drm_universal_plane_init(struct sunxi_drm_device *drm, struct drm_plane *plane,
			     uint32_t possible_crtcs,
			     const struct drm_plane_funcs *funcs,
			     const uint32_t *formats, unsigned int format_count,
			     const uint64_t *format_modifiers,
			     enum drm_plane_type type,
			     const char *name)
{
	int ret;


	ret = __drm_universal_plane_init(drm, plane, possible_crtcs, funcs,
					 formats, format_count, format_modifiers,
					 type, name);

	return ret;
}


/**
 * drm_plane_cleanup - Clean up the core plane usage
 * @plane: plane to cleanup
 *
 * This function cleans up @plane and removes it from the DRM mode setting
 * core. Note that the function does *not* free the plane structure itself,
 * this is the responsibility of the caller.
 */
void drm_plane_cleanup(struct drm_plane *plane)
{
	kfree(plane->format_types);
	kfree(plane->modifiers);

	/* Note that the plane_list is considered to be static; should we
	 * remove the drm_plane at runtime we would have to decrement all
	 * the indices on the drm_plane after us in the plane_list.
	 */

	list_del(&plane->head);
	plane->drm->num_total_plane--;

	if (plane->state && plane->funcs->atomic_destroy_state)
		plane->funcs->atomic_destroy_state(plane, plane->state);

	kfree(plane->name);

	memset(plane, 0, sizeof(*plane));
}


/**
 * drm_plane_from_index - find the registered plane at an index
 * @dev: DRM device
 * @idx: index of registered plane to find for
 *
 * Given a plane index, return the registered plane from DRM device's
 * list of planes with matching index. This is the inverse of drm_plane_index().
 */
struct drm_plane *drm_plane_from_index(struct sunxi_drm_device *dev, int idx)
{
	struct drm_plane *plane;

	drm_for_each_plane(plane, dev)
		if (idx == plane->index)
			return plane;

	return NULL;
}


int drm_plane_check_pixel_format(struct drm_plane *plane,
				 u32 format, u64 modifier)
{
	unsigned int i;

	for (i = 0; i < plane->format_count; i++) {
		if (format == plane->format_types[i])
			break;
	}
	if (i == plane->format_count)
		return -EINVAL;

	if (plane->funcs->format_mod_supported) {
		if (!plane->funcs->format_mod_supported(plane, format, modifier))
			return -EINVAL;
	} else {
		if (!plane->modifier_count)
			return 0;

		for (i = 0; i < plane->modifier_count; i++) {
			if (modifier == plane->modifiers[i])
				break;
		}
		if (i == plane->modifier_count)
			return -EINVAL;
	}

	return 0;
}

/**
 * drm_any_plane_has_format - Check whether any plane supports this format and modifier combination
 * @dev: DRM device
 * @format: pixel format (DRM_FORMAT_*)
 * @modifier: data layout modifier
 *
 * Returns:
 * Whether at least one plane supports the specified format and modifier combination.
 */
bool drm_any_plane_has_format(struct sunxi_drm_device *dev, u32 format, u64 modifier)
{
	struct drm_plane *plane;

	drm_for_each_plane(plane, dev) {
		if (drm_plane_check_pixel_format(plane, format, modifier) == 0)
			return true;
	}

	return false;
}


int drm_framebuffer_check_src_coords(uint32_t src_x, uint32_t src_y,
				     uint32_t src_w, uint32_t src_h,
				     const struct drm_framebuffer *fb)
{
	unsigned int fb_width, fb_height;

	fb_width = fb->width << 16;
	fb_height = fb->height << 16;

	/* Make sure source coordinates are inside the fb. */
	if (src_w > fb_width ||
	    src_x > fb_width - src_w ||
	    src_h > fb_height ||
	    src_y > fb_height - src_h) {
		DRM_ERROR("Invalid source coordinates "
			    "%u.%06ux%u.%06u+%u.%06u+%u.%06u (fb %ux%u)\n",
			    src_w >> 16, ((src_w & 0xffff) * 15625) >> 10,
			    src_h >> 16, ((src_h & 0xffff) * 15625) >> 10,
			    src_x >> 16, ((src_x & 0xffff) * 15625) >> 10,
			    src_y >> 16, ((src_y & 0xffff) * 15625) >> 10,
			    fb->width, fb->height);
		return -ENOSPC;
	}

	return 0;
}

int drm_get_primary_plane_id(struct sunxi_drm_device *drm, int crtc_id)
{
	struct sunxi_drm_crtc *crtc;
	int i = 0;

	crtc = sunxi_drm_crtc_find(drm, crtc_id);
	if (!crtc) {
		DRM_ERROR("Invalid crtc id:%d\n", crtc_id);
		return -2;
	}

	for (i = 0; i < crtc->plane_cnt; ++i) {
		if (crtc->plane[i].plane.type == DRM_PLANE_TYPE_PRIMARY)
			return crtc->plane[i].plane.index;
	}

	return -1;
}

int drm_mode_setplane(struct sunxi_drm_device *drm, struct drm_mode_set_plane *plane_req)
{
	struct drm_plane *plane = NULL;
	struct drm_framebuffer *fb = NULL;
	struct sunxi_drm_crtc *crtc;
	int ret = -1;

	crtc = sunxi_drm_crtc_find(drm, plane_req->crtc_id);
	if (!crtc) {
		DRM_ERROR("Invalid crtc id:%d\n", plane_req->crtc_id);
		return -2;
	}

	plane = drm_plane_from_index(drm, plane_req->plane_id);

	if (!plane) {
		DRM_ERROR("Can not find plane %d\n", plane_req->plane_id);
		return -1;
	}
	mutex_lock(&plane->mutex);

	fb = drm_framebuffer_lookup(drm, plane_req->fb_id);

	if (!fb) {
		/* No fb means shut it down */
		ret = plane->funcs->disable_plane(plane);
		if (!ret) {
			plane->fb = NULL;
		}
		goto OUT;
	}

	ret = drm_plane_check_pixel_format(plane, fb->format->format,
					   fb->modifier);
	if (ret) {
		DRM_ERROR("Invalid pixel format %p4cc, modifier 0x%llx\n",
			  &fb->format->format, fb->modifier);
		goto OUT;
	}

	ret = drm_framebuffer_check_src_coords(
		plane_req->src_x, plane_req->src_y, plane_req->src_w,
		plane_req->src_h, fb);

	if (!ret) {
		ret = plane->funcs->update_plane(plane, crtc, fb,
						 plane_req->crtc_x, plane_req->crtc_y,
						 plane_req->crtc_w, plane_req->crtc_h,
						 plane_req->src_x, plane_req->src_y,
						 plane_req->src_w, plane_req->src_h);
		if (!ret) {
			plane->fb = fb;
		}
	}


OUT:
	mutex_unlock(&plane->mutex);
	return ret;
}



//End of File
