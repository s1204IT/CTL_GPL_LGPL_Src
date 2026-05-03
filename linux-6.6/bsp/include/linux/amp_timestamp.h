/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright(c) 2023 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __AMP_TIMESTAMP_H__
#define __AMP_TIMESTAMP_H__

#include <linux/types.h>

typedef void *amp_ts_dev_t;

int amp_ts_get_dev(uint32_t id, amp_ts_dev_t *dev);
int of_amp_ts_get_dev(struct device_node *np, amp_ts_dev_t *dev);

int amp_ts_get_timestamp(amp_ts_dev_t dev, uint64_t *timestamp);
int amp_ts_get_count_value(amp_ts_dev_t dev, uint64_t *count_value);
int amp_ts_get_count_freq(amp_ts_dev_t dev, uint32_t *freq);

#endif /* __AMP_TIMESTAMP_H__ */
