/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef LOG_H_
#define LOG_H_
#include "../../config.h"

#if defined(__LINUX_PLAT__)

#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm.h>
#include <asm/div64.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/compat.h>
#else
#include "../../hdmi_boot.h"

#endif

#define TRUE  true
#define FALSE false

typedef enum {
	SNPS_ERROR = 0,
	SNPS_WARN,
	SNPS_NOTICE,
	SNPS_DEBUG,
	SNPS_TRACE,
	SNPS_ASSERT
} log_t;

extern u32 hdmi_printf;

#if defined(CONFIG_HDMI20_LOG_BUFFER)
void hdmi_log(const char *fmt, ...);
int hdmi_log_init(void);
int _uboot_dump_hdmi_log(void);
#else
static inline void hdmi_log(const char *fmt, ...)
{
}
#endif

/**
 * @desc: allwinner hdmi log
 * @demo: hdmi20: xxx
*/
#ifdef hdmi_inf
#undef hdmi_inf
#endif
#define hdmi_inf(fmt, args...)                          \
	do {                                                \
		tick_printf("boot_hdmi20: [info] "fmt, ##args); \
		hdmi_log("boot_hdmi20: [info] "fmt, ##args); \
	} while (0)

#ifdef hdmi_wrn
#undef hdmi_wrn
#endif
#define hdmi_wrn(fmt, args...)                          \
	do {                                                \
		tick_printf("boot_hdmi20: [warn] "fmt, ##args); \
		hdmi_log("boot_hdmi20: [warn] "fmt, ##args); \
	} while (0)

#ifdef hdmi_err
#undef hdmi_err
#endif
#define hdmi_err(fmt, args...)                           \
	do {                                                 \
		tick_printf("boot_hdmi20: [error] "fmt, ##args); \
		hdmi_log("boot_hdmi20: [error] "fmt, ##args); \
	} while (0)

#define HDMI_INFO_MSG(fmt, args...)                        \
	do {                                                   \
		if (hdmi_printf)                                   \
			tick_printf("boot_hdmi20: [info] %s line:%d: " \
				fmt, __func__, __LINE__, ##args);          \
		hdmi_log("boot_hdmi20: [info] %s line:%d: " \
				fmt, __func__, __LINE__, ##args); \
	} while (0)

#define HDMI_ERROR_MSG(fmt, args...)                       \
	do {                                                   \
		if (hdmi_printf)                                   \
			tick_printf("boot_hdmi20 [error] %s line:%d: " \
				fmt, __func__, __LINE__, ##args);          \
		hdmi_log("boot_hdmi20 [error] %s line:%d: " \
				fmt, __func__, __LINE__, ##args);          \
	} while (0)

#define LOG_TRACE()                                            \
	do {                                                       \
		if (hdmi_printf)                                       \
			tick_printf("boot_hdmi20 [trace] %s\n", __func__); \
		hdmi_log("boot_hdmi20 [trace] %s\n", __func__); \
	} while (0)
#define LOG_TRACE1(a)                                   \
	do {                                                \
		if (hdmi_printf)                                \
			tick_printf("boot_hdmi20 [trace] %s: %d\n", \
				__func__, a);                           \
		hdmi_log("boot_hdmi20 [trace] %s: %d\n", \
				__func__, a);                           \
	} while (0)
#define LOG_TRACE2(a, b)                                   \
	do {                                                   \
		if (hdmi_printf)                                   \
			tick_printf("boot_hdmi20 [trace] %s: %d %d\n", \
				__func__, a, b);                           \
		hdmi_log("boot_hdmi20 [trace] %s: %d %d\n", \
				__func__, a, b);                           \
	} while (0)
#define LOG_TRACE3(a, b, c)                                   \
	do {                                                      \
		if (hdmi_printf)                                      \
			tick_printf("boot_hdmi20 [trace] %s: %d %d %d\n", \
				__func__, a, b, c);                           \
		hdmi_log("boot_hdmi20 [trace] %s: %d %d %d\n", \
				__func__, a, b, c);                           \
	} while (0)

#endif	/* LOG_H_ */
