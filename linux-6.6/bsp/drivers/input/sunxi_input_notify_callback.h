/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef SUNXI_INPUT_NOTIFY_CALLBACK_H
#define SUNXI_INPUT_NOTIFY_CALLBACK_H

#if !IS_ENABLED(CONFIG_AW_DRM) || IS_ENABLED(CONFIG_FB)
#include <linux/fb.h>
#else
#include <video/sunxi_drm_notify.h>

#ifdef fb_event
#undef fb_event
#endif

#ifdef fb_register_client
#undef fb_register_client
#endif

#ifdef fb_unregister_client
#undef fb_unregister_client
#endif

#ifdef FB_EVENT_BLANK
#undef FB_EVENT_BLANK
#endif

#define fb_event sunxi_notify_event
#define fb_register_client sunxi_disp_register_client
#define fb_unregister_client sunxi_disp_unregister_client
#define FB_EVENT_BLANK SUNXI_EVENT_BLANK

#endif // !IS_ENABLED(CONFIG_FB)

#endif //SUNXI_INPUT_NOTIFY_CALLBACK_H
