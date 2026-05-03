/*
 * drm_print.h
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
#ifndef _DRM_PRINT_H
#define _DRM_PRINT_H
#include <linux/printk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct drm_printer - drm output "stream"
 *
 * Do not use struct members directly.  Use drm_printer_seq_file(),
 * drm_printer_info(), etc to initialize.  And drm_printf() for output.
 */
struct drm_printer {
	void *arg;
	const char *prefix;
};

#define drm_printf(p, fmt, args...)	printf(fmt, ##args)

/**
 * drm_printf_indent - Print to a &drm_printer stream with indentation
 * @printer: DRM printer
 * @indent: Tab indentation level (max 5)
 * @fmt: Format string
 */
#define drm_printf_indent(printer, indent, fmt, ...) \
	drm_printf((printer), "%.*s" fmt, (indent), "\t\t\t\t\tX", ##__VA_ARGS__)

#define DRM_WARN(fmt, args...) pr_err(fmt, ##args)
#define DRM_ERROR(fmt, args...) pr_err(fmt, ##args)
#define DRM_INFO(fmt, args...) pr_info(fmt, ##args)
#define DRM_NOTE(fmt, args...) pr_info(fmt, ##args)
#define DRM_DEBUG(fmt, args...) pr_debug(fmt, ##args)
#define DRM_DEBUG_KMS(fmt, args...) pr_debug(fmt, ##args)
#define DRM_DEBUG_DRIVER(fmt, args...) pr_debug(fmt, ##args)


#ifdef __cplusplus
}
#endif

#endif /*End of file*/
