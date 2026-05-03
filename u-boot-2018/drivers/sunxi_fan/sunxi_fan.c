/*
 *  * Copyright 2000-2009
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#include <string.h>
#include <stdlib.h>
#include <common.h>
#include <asm/io.h>
#include <sys_config.h>
#include <fdt_support.h>
#include <pwm.h>

#define MAX_GPIO 4

struct fan_device_info {
	int dts_node;
	u32 mode;
	u32 pwm_ch;
	u32 pwm_freq;
	u32 pwm_pol;
	u32 pwm_duty;
	u32 pwm_min;
	u32 pwm_max;
	user_gpio_set_t *power_en_gpio[MAX_GPIO];
};

struct fan_device_info *fan_info_pt;

void fan_disable(void)
{
	int i;
	for (i = 0; i < MAX_GPIO; i++) {
		if (fan_info_pt->power_en_gpio[i])
			sunxi_gpio_request(fan_info_pt->power_en_gpio[i], 0);
	}
}

void fan_enable(void)
{
	int i;
	for (i = 0; i < MAX_GPIO; i++) {
		if (fan_info_pt->power_en_gpio[i])
			sunxi_gpio_request(fan_info_pt->power_en_gpio[i], 1);
	}
}

static void fan_config(void)
{
	__u64 period_ns, duty_ns, max_duty_ns, min_duty_ns;

	if (!fan_info_pt->pwm_freq) {
		pr_err("fan: pwm freq is zero\n");
		return;
	}
	if (fan_info_pt->pwm_duty > 100) {
		pr_err("fan pwm duty value is out of range:%u\n", fan_info_pt->pwm_duty);
		fan_info_pt->pwm_duty = 100;
	}
	period_ns = 1000*1000*1000 / fan_info_pt->pwm_freq;
	max_duty_ns = (fan_info_pt->pwm_max * period_ns) / 100;
	min_duty_ns = (fan_info_pt->pwm_min * period_ns) / 100;

	duty_ns = min_duty_ns + (max_duty_ns - min_duty_ns) / 100 * fan_info_pt->pwm_duty;

	if (pwm_request(fan_info_pt->pwm_ch, "sunxi_fan") < 0) {
		pr_err("pwm%d request for sunxi_fan fail!\n", fan_info_pt->pwm_ch);
	}
	pwm_set_polarity(fan_info_pt->pwm_ch, fan_info_pt->pwm_pol);
	pwm_config(fan_info_pt->pwm_ch, duty_ns, period_ns);
	pwm_enable(fan_info_pt->pwm_ch);
}

int fan_setup(void)
{
	int ret;
	int i;
	char gen_gpio_name[40] = {0};

	fan_info_pt = malloc(sizeof(struct fan_device_info));
	if (!fan_info_pt) {
		pr_err("malloc fan_device_info fail\n");
		return -1;
	}

	fan_info_pt->dts_node = fdt_path_offset(working_fdt, "/soc/fan");
	if (!fan_info_pt->dts_node) {
		pr_err("get /soc/fan dts fail\n");
		free(fan_info_pt);
		return -1;
	}

	if (fdt_getprop_u32(working_fdt, fan_info_pt->dts_node, "pwm_ch",
		(uint32_t *)&fan_info_pt->pwm_ch) < 0) {
		fan_info_pt->pwm_ch = 0;
	}

	if (fdt_getprop_u32(working_fdt, fan_info_pt->dts_node, "pwm_freq",
		(uint32_t *)&fan_info_pt->pwm_freq) < 0) {
		fan_info_pt->pwm_freq = 25000;
	}

	if (fdt_getprop_u32(working_fdt, fan_info_pt->dts_node, "pwm_pol",
		(uint32_t *)&fan_info_pt->pwm_pol) < 0) {
		fan_info_pt->pwm_pol = 0;
	}

	if (fdt_getprop_u32(working_fdt, fan_info_pt->dts_node, "pwm_duty",
		(uint32_t *)&fan_info_pt->pwm_duty) < 0) {
		fan_info_pt->pwm_duty = 50;
	}

	if (fdt_getprop_u32(working_fdt, fan_info_pt->dts_node, "pwm_min",
		(uint32_t *)&fan_info_pt->pwm_min) < 0) {
		fan_info_pt->pwm_min = 0;
	}

	if (fdt_getprop_u32(working_fdt, fan_info_pt->dts_node, "pwm_max",
		(uint32_t *)&fan_info_pt->pwm_max) < 0) {
		fan_info_pt->pwm_max = 100;
	}

	for (i = 0; i < MAX_GPIO; i++) {
		snprintf(gen_gpio_name, 40, "fan_gpio_%d", i);
		fan_info_pt->power_en_gpio[i] = malloc(sizeof(user_gpio_set_t));
		if (fan_info_pt->power_en_gpio[i]) {
			ret = fdt_get_one_gpio_by_offset(fan_info_pt->dts_node, gen_gpio_name, fan_info_pt->power_en_gpio[i]);
			if (ret < 0) {
				free(fan_info_pt->power_en_gpio[i]);
				fan_info_pt->power_en_gpio[i] = NULL;
			}
		}
	}

#if 0
	printf("pwm_ch %d\n", fan_info_pt->pwm_ch);
	printf("pwm_freq %d\n", fan_info_pt->pwm_freq);
	printf("pwm_pol %d\n", fan_info_pt->pwm_pol);
	printf("pwm_duty %d\n", fan_info_pt->pwm_duty);
	printf("pwm_min %d\n", fan_info_pt->pwm_min);
	printf("pwm_max %d\n", fan_info_pt->pwm_max);
#endif
	return 0;
}

int fan_init(void)
{
	if (!fan_setup()) {
		fan_config();
		fan_enable();
	}
	return 0;
}
