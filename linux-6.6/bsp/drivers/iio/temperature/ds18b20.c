// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (C) 2022 Allwinner.
 *
 * ds18b20.c - Maxim DS18B20 Programmable Resolution 1-Wire Digital Thermometer Driver
 *
 * Datasheet : https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf
 *
 * Author: Jingyan Liang <jingyanliang@allwinnertech.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/iio/iio.h>
#include <linux/delay.h>

struct ds18b20_chip_info {
	struct platform_device *pdev;
	struct device *dev;
	struct gpio_desc *gpiod;
	spinlock_t lock;
};

static const struct iio_chan_spec ds18b20_channels[] = {
	{	/* Temperature */
		.type = IIO_TEMP,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE)
	},
};

static int ds18b20_reset(struct ds18b20_chip_info *ds18b20)
{
	int ret;

	spin_lock(&ds18b20->lock);

	gpiod_direction_output(ds18b20->gpiod, 1);
	gpiod_set_value(ds18b20->gpiod, 0);
	udelay(480);
	gpiod_set_value(ds18b20->gpiod, 1);
	gpiod_direction_input(ds18b20->gpiod);
	udelay(60);
	ret = gpiod_get_value(ds18b20->gpiod);
	udelay(240);

	spin_unlock(&ds18b20->lock);

	return ret;
}

static u8 ds18b20_read_byte(struct ds18b20_chip_info *ds18b20)
{
	int i;
	u8 byte = 0;

	spin_lock(&ds18b20->lock);

	for (i = 0; i < 8; i++) {
		byte >>= 1;
		gpiod_direction_output(ds18b20->gpiod, 1);
		udelay(2);
		gpiod_set_value(ds18b20->gpiod, 0);
		udelay(2);
		gpiod_direction_input(ds18b20->gpiod);
		if (gpiod_get_value(ds18b20->gpiod))
			byte |= BIT(7);
		udelay(60);
	}

	spin_unlock(&ds18b20->lock);

	return byte;
}

static int ds18b20_write_byte(struct ds18b20_chip_info *ds18b20, u8 byte)
{
	int i;

	spin_lock(&ds18b20->lock);

	for (i = 0; i < 8; i++) {
		gpiod_direction_output(ds18b20->gpiod, 1);
		udelay(2);
		gpiod_set_value(ds18b20->gpiod, 0);
		udelay(30);
		gpiod_set_value(ds18b20->gpiod, byte & BIT(0));
		udelay(30);
		byte >>= 1;
	}

	spin_unlock(&ds18b20->lock);

	return 0;
}

static int ds18b20_get_temp_data(struct ds18b20_chip_info *ds18b20, int *val)
{
	u8 t_l, t_h;
	u16 temp;
	int ret = 0;

	if (ds18b20_reset(ds18b20)) {
		dev_err(ds18b20->dev, "1st reset failed\n");
		ret = -EINVAL;
		goto out;
	}

	ds18b20_write_byte(ds18b20, 0xcc);
	ds18b20_write_byte(ds18b20, 0x44);

	if (ds18b20_reset(ds18b20)) {
		dev_err(ds18b20->dev, "2nd reset failed\n");
		ret = -EINVAL;
		goto out;
	}

	ds18b20_write_byte(ds18b20, 0xcc);
	ds18b20_write_byte(ds18b20, 0xbe);

	t_l = ds18b20_read_byte(ds18b20);
	t_h = ds18b20_read_byte(ds18b20);
	temp = (u16)((t_h << 8) | (t_l & 0xff));
	if (temp & BIT(11))
		*val = ((~temp + 1) & GENMASK(10, 0)) * -1;
	else
		*val = temp;

out:
	return ret;
}

static int ds18b20_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val, int *val2, long mask)
{
	struct ds18b20_chip_info *ds18b20 = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ds18b20_get_temp_data(ds18b20, val);
		if (ret)
			return ret;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		/* 12-bits corresponding to increments is 0.0625  */
		*val = 0;
		*val2 = 62500;
		return IIO_VAL_INT_PLUS_MICRO;
	default:
		return -EINVAL;
	}
}

static const struct iio_info ds18b20_info = {
	.read_raw = ds18b20_read_raw,
};

static int ds18b20_probe(struct platform_device *pdev)
{
	struct ds18b20_chip_info *ds18b20;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*ds18b20));
	if (!indio_dev)
		return -ENOMEM;

	ds18b20 = iio_priv(indio_dev);
	ds18b20->pdev = pdev;
	ds18b20->dev = &pdev->dev;
	platform_set_drvdata(pdev, ds18b20);
	spin_lock_init(&ds18b20->lock);

	indio_dev->info = &ds18b20_info;
	indio_dev->name = dev_name(ds18b20->dev);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ds18b20_channels;
	indio_dev->num_channels = ARRAY_SIZE(ds18b20_channels);

	ds18b20->gpiod = devm_gpiod_get(ds18b20->dev, "dq", GPIOD_OUT_HIGH);
	if (IS_ERR(ds18b20->gpiod)) {
		ret = PTR_ERR(ds18b20->gpiod);
		dev_err(ds18b20->dev, "get gpio failed %d\n", ret);
		return ret;
	}
	gpiod_set_consumer_name(ds18b20->gpiod, devm_kasprintf(ds18b20->dev, GFP_KERNEL, "%s dq", dev_name(ds18b20->dev)));

	return devm_iio_device_register(ds18b20->dev, indio_dev);
}

static const struct of_device_id of_ds18b20_match[] = {
	{ .compatible = "maxim,ds18b20", },
	{},
};

static struct platform_driver ds18b20_driver = {
	.probe = ds18b20_probe,
	.driver = {
		.name = "ds18b20",
		.of_match_table = of_ds18b20_match,
	},
};

module_platform_driver(ds18b20_driver)

MODULE_AUTHOR("JingyanLiang <jingyanliang@allwinnertech.com>");
MODULE_DESCRIPTION("Maxim DS18B20 Programmable Resolution 1-Wire Digital Thermometer Driver");
MODULE_LICENSE("GPL v2");
