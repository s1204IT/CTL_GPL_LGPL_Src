/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/libfdt.h>
#include <linux/sched/clock.h>

#include "aw_hdmirx_define.h"
#include "aw_hdmirx_log.h"

#define LOG_BUFFER_SIZE		65535

static char *buffer;
static u32 buffer_max_size;
static u32 end;
static u32 override;
static struct mutex lock_hdmirx_log;

void hdmirx_log(const char *fmt, ...)
{
	char tmp_buffer[256] = {0};
	va_list args;
	u32 len = 0;
	char *p = NULL;
	unsigned long long ts;
	unsigned long rem_nsec;

	if (!buffer || !buffer_max_size)
		return ;

	ts = local_clock();
	rem_nsec = do_div(ts, NSEC_PER_SEC);
	snprintf(tmp_buffer, sizeof(tmp_buffer),
			"[%5lu.%06lu] ",
			(unsigned long)ts,
			rem_nsec / NSEC_PER_USEC);

	va_start(args, fmt);
	vsnprintf(tmp_buffer + strlen(tmp_buffer),
			sizeof(tmp_buffer) - strlen(tmp_buffer), fmt, args);
	va_end(args);

	len = strlen(tmp_buffer);

	mutex_lock(&lock_hdmirx_log);

	if (buffer_max_size - end >= len) {
		memcpy(buffer + end, tmp_buffer, len);
		end += len;
	} else {
		if (len < buffer_max_size) {
			p = tmp_buffer;
		} else {
			/* If the size of tmp_buffer is too large,
			 * only copy the last <buffer_max_size> bytes.
			 */
			p = tmp_buffer + (len - buffer_max_size);
			len = buffer_max_size;
		}

		memcpy(buffer + end, p, buffer_max_size - end);

		memcpy(buffer, p + (buffer_max_size - end),
				len - (buffer_max_size - end));

		end = len - (buffer_max_size - end);
		if (!override)
			override = 1;
	}

	mutex_unlock(&lock_hdmirx_log);

}

char *aw_hdmirx_log_get_address(void)
{
	/* When copying buffer content to user space,
	 * buffer must be prevented from being written.
	 */
	mutex_lock(&lock_hdmirx_log);
	return buffer;
}

void aw_hdmirx_log_put_address(void)
{
	/* After copy to user is over, you can continue to write. */
	mutex_unlock(&lock_hdmirx_log);
}

unsigned int aw_hdmirx_log_get_start_index(void)
{
	/* Valid data range:
	 * 	override == 0: [0, end)
	 * 	override == 1: [end, buffer_max_size) + [0, end)
	 */
	if (override)
		return end;
	return 0;
}

unsigned int aw_hdmirx_log_get_used_size(void)
{
	if (override)
		return buffer_max_size;
	return end;
}

unsigned int aw_hdmirx_log_get_max_size(void)
{
	return buffer_max_size;
}

int aw_hdmirx_log_init(struct device_node *of_node)
{
	if (buffer) {
		kfree(buffer);
		buffer = NULL;
	}

	buffer = kzalloc(LOG_BUFFER_SIZE, GFP_KERNEL);
	if (buffer == NULL) {
		hdmirx_err("%s: kzalloc hdmi log buffer failed!\n", __func__);
		return -1;
	}
	end = 0;
	override = 0;
	buffer_max_size = LOG_BUFFER_SIZE;

	mutex_init(&lock_hdmirx_log);
	return 0;
}

void aw_hdmirx_log_exit(void)
{
	if (buffer)
		kfree(buffer);
	buffer = NULL;
	end = 0;
	override = 0;
	buffer_max_size = 0;
	mutex_destroy(&lock_hdmirx_log);
}

