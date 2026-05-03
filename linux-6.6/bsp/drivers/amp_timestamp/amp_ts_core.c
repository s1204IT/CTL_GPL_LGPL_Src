/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>

#include <linux/amp_timestamp.h>
#include <dt-bindings/amp-timestamp/amp-timestamp-dt.h>

#include "amp_ts_core.h"

#define AMP_TIMESTAMP_VERSION "1.0.2"

#define AMP_TS_DEV_ID_PROPERTY_NAME "id"
#define COUNTER_TYPE_PROPERTY_NAME "counter_type"
#define COUNT_FREQ_PROPERTY_NAME "count_freq"
#define COUNT_FREQ_CLK_NAME "ts_clk"

#define AMP_TS_DEV_PROPERTY_NAME "amp_ts_dev"

typedef struct amp_timestamp_dev {
	uint32_t id;
	uint32_t div_us;
	struct device *dev;
	amp_counter_t counter;
	struct list_head list;
} amp_timestamp_dev_t;

static amp_counter_ops_t *g_counter_ops[] = {
	&g_arm_arch_counter_ops,
	&g_thead_riscv_arch_counter_ops,
};

static LIST_HEAD(g_amp_ts_dev_list);

static amp_timestamp_dev_t *get_amp_ts_dev(uint32_t id)
{
	amp_timestamp_dev_t *pos;

	list_for_each_entry(pos, &g_amp_ts_dev_list, list) {
		if (id == pos->id) {
			return pos;
		}
	}

	return NULL;
}

int amp_ts_get_dev(uint32_t id, amp_ts_dev_t *dev)
{
	amp_timestamp_dev_t *ts_dev = get_amp_ts_dev(id);
	if (!ts_dev)
		return -ENODEV;

	*dev = ts_dev;
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_dev);

int of_amp_ts_get_dev(struct device_node *np, amp_ts_dev_t *dev)
{
	int ret;
	amp_timestamp_dev_t *ts_dev;
	struct device_node *ts_dev_np;
	uint32_t id;

	ts_dev_np = of_parse_phandle(np, AMP_TS_DEV_PROPERTY_NAME, 0);
	if (!ts_dev_np) {
		pr_err("%s: Can't find dts property '%s'\n", np->full_name, AMP_TS_DEV_PROPERTY_NAME);
		return -EFAULT;
	}

	ret = of_property_read_u32(ts_dev_np, AMP_TS_DEV_ID_PROPERTY_NAME, &id);
	if (ret) {
		pr_err("%s: Failed to get dts property '%s', ret: %d\n", np->full_name, AMP_TS_DEV_ID_PROPERTY_NAME, ret);
		return -EFAULT;
	}

	ts_dev = get_amp_ts_dev(id);
	if (!ts_dev)
		return -ENODEV;

	*dev = ts_dev;
	return 0;
}
EXPORT_SYMBOL(of_amp_ts_get_dev);

int amp_ts_get_timestamp(amp_ts_dev_t dev, uint64_t *timestamp)
{
	amp_timestamp_dev_t *ts_dev = (amp_timestamp_dev_t *)dev;
	amp_counter_t *counter = &ts_dev->counter;
	uint64_t count_value;

	count_value = counter->ops->read_counter(counter);

	*timestamp = div_u64(count_value, ts_dev->div_us);
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_timestamp);

int amp_ts_get_count_value(amp_ts_dev_t dev, uint64_t *count_value)
{
	amp_timestamp_dev_t *ts_dev = (amp_timestamp_dev_t *)dev;
	amp_counter_t *counter = &ts_dev->counter;

	*count_value = counter->ops->read_counter(counter);
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_count_value);

int amp_ts_get_count_freq(amp_ts_dev_t dev, uint32_t *freq)
{
	amp_timestamp_dev_t *ts_dev = (amp_timestamp_dev_t *)dev;

	*freq = ts_dev->counter.count_freq;
	return 0;
}
EXPORT_SYMBOL(amp_ts_get_count_freq);

static ssize_t info_show(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	unsigned long long count_value = 0;
	int ret;

	amp_timestamp_dev_t *ts_dev = platform_get_drvdata(to_platform_device(dev));
	ret = amp_ts_get_count_value(ts_dev, &count_value);
	if (ret)
		printk(KERN_EMERG "amp_ts_get_count_value failed, ret: %d\n", ret);

	return sprintf(buf, "count: %llu\n", count_value);
}
static DEVICE_ATTR_RO(info);

static ssize_t timestamp_show(struct device *dev, struct device_attribute *attr,
							  char *buf)
{
	unsigned long long ts = 0;
	int ret;

	amp_timestamp_dev_t *ts_dev = platform_get_drvdata(to_platform_device(dev));

	ret = amp_ts_get_timestamp(ts_dev, &ts);
	if (ret)
		printk(KERN_EMERG "amp_ts_get_timestamp failed, ret: %d\n", ret);

	return sprintf(buf, "timestamp: %lluus, %llums\n", ts, div_u64(ts, 1000));
}
static DEVICE_ATTR_RO(timestamp);

static int parse_dts(amp_timestamp_dev_t *ts_dev)
{
	int ret;
	struct device *dev;
	struct device_node *dev_node;
	uint32_t id, counter_type, count_freq;
	struct clk *ts_clk;

	amp_counter_t *counter;

	dev = ts_dev->dev;
	counter = &ts_dev->counter;
	dev_node = counter->of_node;

	ret = of_property_read_u32(dev_node, COUNTER_TYPE_PROPERTY_NAME, &counter_type);
	if (ret) {
		dev_err(dev, "Failed to get dts property '%s', ret: %d\n", COUNTER_TYPE_PROPERTY_NAME, ret);
		return -EFAULT;
	}

	if ((counter_type != ARM_ARCH_COUNTER_TYPE) &&
				(counter_type != THEAD_RISCV_ARCH_COUNTER_TYPE)) {
		dev_err(dev, "Unsupported counter type(%u)!", counter_type);
		return -EINVAL;
	}

	counter->type = counter_type;

	ret = of_property_read_u32(dev_node, COUNT_FREQ_PROPERTY_NAME, &count_freq);
	if (ret) {
		ts_clk = of_clk_get_by_name(dev_node, COUNT_FREQ_CLK_NAME);
		if (IS_ERR_OR_NULL(ts_clk)) {
			dev_err(dev, "Failed to get dts property '%s', ret: %d\n", COUNT_FREQ_PROPERTY_NAME, ret);
			dev_err(dev, "Failed to get dts property '%s', ret: %d\n", COUNT_FREQ_CLK_NAME, ret);
			return -EFAULT;
		}
		count_freq = clk_get_rate(ts_clk);
	}

	if (count_freq < 1000000) {
		dev_err(dev, "count freq(%u) is little than 1MHz!", count_freq);
		return -EINVAL;
	}
	counter->count_freq = count_freq;

	ret = of_property_read_u32(dev_node, AMP_TS_DEV_ID_PROPERTY_NAME, &id);
	if (ret) {
		dev_err(dev, "Failed to get dts property '%s', ret: %d\n", AMP_TS_DEV_ID_PROPERTY_NAME, ret);
		return -EFAULT;
	}

	if (get_amp_ts_dev(id)) {
		dev_err(dev, "invalid id(%u)", id);
		return -EINVAL;
	}

	ts_dev->id = id;
	return 0;
}

static int amp_ts_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev;
	struct device_node *of_node;
	struct resource *io_res;
	void __iomem *io_va;
	amp_timestamp_dev_t *ts_dev;
	amp_counter_t *counter;

	dev = &pdev->dev;
	of_node = pdev->dev.of_node;

	io_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!io_res) {
		dev_err(dev, "get resource IORESOURCE_MEM failed\n");
		return -EFAULT;
	}

	ts_dev = devm_kzalloc(dev, sizeof(*ts_dev), GFP_KERNEL);
	if (!ts_dev) {
		dev_err(dev, "amp_timestamp_dev_t memory allocation failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, ts_dev);
	ts_dev->dev = dev;
	counter = &ts_dev->counter;

	counter->of_node = of_node;
	ret = parse_dts(ts_dev);
	if (ret) {
		dev_err(dev, "parse_dts failed, ret: %d\n", ret);
		return -EFAULT;
	}

	counter->io_res = io_res;

	io_va = devm_ioremap_resource(dev, io_res);
	if (IS_ERR_OR_NULL(io_va)) {
		dev_err(dev, "Fialed to map the IO memory of counter\n");
		return -EIO;
	}

	counter->reg_base_addr = io_va;
	counter->ops = g_counter_ops[counter->type];

	ret = counter->ops->init(counter);
	if (ret) {
		dev_err(dev, "amp counter %u init failed, ret: %d\n", ts_dev->id, ret);
		return ret;
	}

	ts_dev->div_us = counter->count_freq / 1000000;

	ret = device_create_file(dev, &dev_attr_timestamp);
	if (ret) {
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_info);
	if (ret) {
		return ret;
	}

	list_add(&ts_dev->list, &g_amp_ts_dev_list);

	dev_info(dev, "AMP timestamp device %u probe success, count_freq: %uHz\n", ts_dev->id, counter->count_freq);

	return 0;
}

static int amp_ts_remove(struct platform_device *pdev)
{
	struct device *dev;
	amp_timestamp_dev_t *ts_dev;

	dev = &pdev->dev;
	ts_dev = platform_get_drvdata(pdev);

	device_remove_file(dev, &dev_attr_timestamp);
	device_remove_file(dev, &dev_attr_info);
	list_del(&ts_dev->list);

	dev_info(dev, "AMP timestamp device %d removed!\n", ts_dev->id);
	return 0;
}

static const struct of_device_id g_amp_ts_match_table[] = {
	{ .compatible = "sunxi,amp_timestamp" },
	{},
};
MODULE_DEVICE_TABLE(of, g_amp_ts_match_table);

static struct platform_driver g_amp_ts_dirver = {
	.probe = amp_ts_probe,
	.remove = amp_ts_remove,
	.driver = {
		.name	= "amp_timestamp",
		.of_match_table = g_amp_ts_match_table,
	},
};
module_platform_driver(g_amp_ts_dirver);

MODULE_DESCRIPTION("Allwinner AMP Timestamp driver");
MODULE_AUTHOR("liusijun <liusijun@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(AMP_TIMESTAMP_VERSION);
