// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Thermal sensor driver for Allwinner SOC
 * Copyright (C) 2019 frank@allwinnertech.com
 */
#include <sunxi-log.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/version.h>

#define MAX_RELY_SENSOR 10

struct platform_ths_thermal {
	int (*get_temp)(void *data, int *temp);
};

struct virtual_ths_device {
	const struct platform_ths_thermal *plat_tz;
	struct thermal_zone_device *thermal_dev[MAX_RELY_SENSOR];
	struct device *dev;
	int supply_zone_nums;
};

static int sunxi_ths_get_temp(struct thermal_zone_device *data, int *temp)
{
	struct virtual_ths_device *tmdev = (struct virtual_ths_device *)data->devdata;
	return tmdev->plat_tz->get_temp(tmdev, temp);
}

static const struct thermal_zone_device_ops ths_ops = {
	.get_temp = sunxi_ths_get_temp,
};

static int sunxi_virtual_ths_register(struct virtual_ths_device *tmdev)
{
	struct thermal_zone_device	*vir_ths;
	vir_ths = devm_thermal_of_zone_register(tmdev->dev, 0, tmdev, &ths_ops);
	if (IS_ERR(vir_ths))
		return PTR_ERR(vir_ths);

	return 0;
}

static int skin_ths_get_temp(void *data, int *temp)
{
	struct virtual_ths_device *tmdev = data;
	int temperature = 0;
	int i, ret;

	for (i = 0; i < tmdev->supply_zone_nums; i++) {
		ret = thermal_zone_get_temp(tmdev->thermal_dev[i], &temperature);
		if (ret)
			return ret;
	}

	if (temperature <= 30000)
		temperature = 30000;
	else if (temperature <= 100000)
		temperature = (30000 + (temperature - 30000) / 5);
	else
		temperature = (44000 + (temperature -100000) / 10);

	*temp = temperature;
	return 0;
}

static int ths_get_avg_temp(void *data, int *temp)
{
	struct virtual_ths_device *tmdev = data;
	int temperature, total_temp = 0;
	int i, ret;

	for (i = 0; i < tmdev->supply_zone_nums; i++) {
		ret = thermal_zone_get_temp(tmdev->thermal_dev[i], &temperature);
		if (ret)
			return ret;
		total_temp += temperature;
	}
	*temp = total_temp / tmdev->supply_zone_nums;
	return 0;
}

static int ths_get_max_temp(void *data, int *temp)
{

	struct virtual_ths_device *tmdev = data;
	int temperature, max_temp = 0;
	int i, ret;

	for (i = 0; i < tmdev->supply_zone_nums; i++) {
		ret = thermal_zone_get_temp(tmdev->thermal_dev[i], &temperature);
		if (ret)
			return ret;

		if (temperature > max_temp)
			max_temp = temperature;
	}
	*temp = max_temp;
	return 0;
}

static const struct platform_ths_thermal skin_ths = {
	.get_temp = skin_ths_get_temp,
};

static const struct platform_ths_thermal avg_temp_ths = {
	.get_temp = ths_get_avg_temp,
};

static const struct platform_ths_thermal max_temp_ths = {
	.get_temp = ths_get_max_temp,
};

static int sunxi_ths_probe(struct platform_device *pdev)
{
	struct virtual_ths_device *tmdev;
	struct device *dev = &pdev->dev;
	int ret, i;
	const __be32 *list;
	const char *rely_tz;
	struct device_node *tz_node;

	tmdev = devm_kzalloc(dev, sizeof(*tmdev), GFP_KERNEL);
	if (!tmdev)
		return -ENOMEM;
	tmdev->dev = dev;
	tmdev->plat_tz = of_device_get_match_data(&pdev->dev);
	if (!tmdev->plat_tz)
		return -EINVAL;
	platform_set_drvdata(pdev, tmdev);

	list = of_get_property(tmdev->dev->of_node, "ext-ths-rely", &tmdev->supply_zone_nums);
	tmdev->supply_zone_nums /= sizeof(*list);
	for (i = 0; i < tmdev->supply_zone_nums; i++) {
		tz_node = of_parse_phandle(tmdev->dev->of_node, "ext-ths-rely", i);
		rely_tz = of_node_full_name(tz_node);
		tmdev->thermal_dev[i] = thermal_zone_get_zone_by_name(rely_tz);
		if (IS_ERR_OR_NULL(tmdev->thermal_dev[i]))
			return -EPROBE_DEFER;
	}
	ret = sunxi_virtual_ths_register(tmdev);
	if (ret)
		sunxi_err(tmdev->dev, "failed to register ths virtual ths\n");
	return ret;
}

static int sunxi_ths_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id of_ths_match[] = {
	{ .compatible = "allwinner,sunxi-skin-ths", .data = &skin_ths },
	{ .compatible = "allwinner,sunxi-avg-temp-ths", .data = &avg_temp_ths },
	{ .compatible = "allwinner,sunxi-max-temp-ths", .data = &max_temp_ths },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, of_ths_match);

static struct platform_driver ext_ths_driver = {
	.probe = sunxi_ths_probe,
	.remove = sunxi_ths_remove,
	.driver = {
		.name = "sunxi-ext-thermal",
		.of_match_table = of_ths_match,
	},
};
module_platform_driver(ext_ths_driver);

MODULE_DESCRIPTION("Thermal sensor driver for Allwinner SOC");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("ALLWINNER");
