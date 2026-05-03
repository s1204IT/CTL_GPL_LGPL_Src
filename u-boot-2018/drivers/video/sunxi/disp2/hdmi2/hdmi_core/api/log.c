/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "log.h"
#include <sys_config.h>
#include <fdt_support.h>
#include <common.h>
#include <environment.h>

#define LOG_BUFFER_SIZE         (CONFIG_HDMI20_LOG_BUFFER_SIZE)

static char *buffer;
static u32 end;
static u32 override;

void hdmi_log(const char *fmt, ...)
{
	char tmp_buffer[128] = {0};
	va_list args;
	u32 len = 0;
	char *p = NULL;

	if (!buffer || !LOG_BUFFER_SIZE)
		return ;

	va_start(args, fmt);
	vsnprintf(tmp_buffer, sizeof(tmp_buffer), fmt, args);
	va_end(args);

	len = strlen(tmp_buffer);

	if (LOG_BUFFER_SIZE - end >= len) {
		memcpy(buffer + end, tmp_buffer, len);
		end += len;
	} else {
		if (len < LOG_BUFFER_SIZE) {
			p = tmp_buffer;
		} else {
			/* If the size of tmp_buffer is too large,
			 * only copy the last <LOG_BUFFER_SIZE> bytes.
			 */
			p = tmp_buffer + (len - LOG_BUFFER_SIZE);
			len = LOG_BUFFER_SIZE;
		}

		memcpy(buffer + end, p, LOG_BUFFER_SIZE - end);

		memcpy(buffer, p + (LOG_BUFFER_SIZE - end),
				len - (LOG_BUFFER_SIZE - end));

		end = len - (LOG_BUFFER_SIZE - end);
		if (!override)
			override = 1;
	}
}

int hdmi_log_reserve_mem_to_kernel(void)
{
	int ret = 0;
	int fdt_node = -1;
	u32 d[8] = {0};

	if (!buffer)
		return -1;

	/* notice: make sure we use the only one nodt "hdmi". */
	fdt_node = fdt_path_offset(working_fdt, "/soc/hdmi");
	assert(fdt_node >= 0);

	d[0] = 0;
	d[1] = cpu_to_fdt32((uintptr_t)buffer);
	d[2] = 0;
	d[3] = cpu_to_fdt32(LOG_BUFFER_SIZE);

	d[4] = 0;
	d[5] = cpu_to_fdt32(end);
	d[6] = 0;
	d[7] = cpu_to_fdt32(override);

	ret = fdt_setprop(working_fdt, fdt_node, "hdmi_log_mem", d, 32);
	if (ret < 0) {
		tick_printf("%s: fdt_setprop hdmi_log_mem failed!\n", __func__);
		return ret;
	}

	ret = fdt_add_mem_rsv(working_fdt, (uint64_t)((uintptr_t)buffer), (uint64_t)LOG_BUFFER_SIZE);
	if (ret < 0) {
		tick_printf("%s: fdt_add_mem_rsv failed!\n", __func__);
		return ret;
	}
	return 0;
}

int hdmi_log_init(void)
{
	buffer = (void *)memalign(PAGE_SIZE, LOG_BUFFER_SIZE);
	if (!buffer) {
		tick_printf("%s: memalign failed!\n", __func__);
		return -1;
	}
	memset(buffer, 0, LOG_BUFFER_SIZE);
	return 0;
}

int _uboot_dump_hdmi_log(void)
{
	int i;

	if (!buffer)
		return -1;

	if (override) {
		for (i = end; i < LOG_BUFFER_SIZE; i++)
			printf("%c", *(buffer+i));
		for (i = 0; i < end; i++)
			printf("%c", *(buffer+i));
	} else {
		for (i = 0; i < end; i++)
			printf("%c", *(buffer+i));
	}

	return 0;
}
