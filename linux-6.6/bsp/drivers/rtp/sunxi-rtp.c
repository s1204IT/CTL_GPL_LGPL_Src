// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner sunxi resistive touch panel controller driver
 *
 * Copyright (C) 2021 Allwinner Tech Co., Ltd
 * Author: Xu Minghui <xuminghuis@allwinnertech.com>
 *
 * Copyright (C) 2013 - 2014 Hans de Goede <hdegoede@redhat.com>
 *
 * The hwmon parts are based on work by Corentin LABBE which is:
 * Copyright (C) 2013 Corentin LABBE <clabbe.montjoie@gmail.com>
 */

/*
 * The sunxi-rtp controller is capable of detecting a second touch, but when a
 * second touch is present then the accuracy becomes so bad the reported touch
 * location is not useable.
 *
 * The original android driver contains some complicated heuristics using the
 * aprox. distance between the 2 touches to see if the user is making a pinch
 * open / close movement, and then reports emulated multi-touch events around
 * the last touch coordinate (as the dual-touch coordinates are worthless).
 *
 * These kinds of heuristics are just asking for trouble (and don't belong
 * in the kernel). So this driver offers straight forward, reliable single
 * touch functionality only.
 *
 */

//#define DEBUG /* Enable sunxi_debug */
#define SUNXI_MODNAME "tpadc"
#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/pinctrl/consumer.h>
#if IS_ENABLED(CONFIG_IIO)
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#endif

#define RTP_CTRL0			0x00
#define RTP_CTRL1			0x04
#define RTP_CTRL2			0x08
#define RTP_CTRL3			0x0c
#define RTP_INT_FIFOC			0x10
#define RTP_INT_FIFOS			0x14
#define RTP_TPR				0x18
#define RTP_CDAT			0x1c
#define RTP_TEMP_DATA			0x20
#define RTP_DATA			0x24

/* RTP_CTRL0 bits */
#define ADC_FIRST_DLY(x)		((x) << 24) /* 8 bits */
#define ADC_FIRST_DLY_MODE(x)		((x) << 23)
#define ADC_CLK_SEL(x)			((x) << 22)
#define ADC_CLK_DIV(x)			((x) << 20) /* 3 bits */
#define FS_DIV(x)			((x) << 16) /* 4 bits */
#define T_ACQ(x)			((x) << 0) /* 16 bits */
#define T_ACQ_DEFAULT			(0x79) /* default:488us for 24M clock in */

/* RTP_CTRL0 bits select */
#define ADC_FIRST_DLY_SELECT		(0xf)
#define ADC_FIRST_DLY_MODE_SELECT	(0)
#define ADC_CLK_SELECT			(0)
#define ADC_CLK_DIV_SELECT		(2)
#define ADC_CLK_DIV_CLK_IN		(0x3)
#define FS_DIV_SELECT			(7)	/* default:488HZ for 24M clock in */
#define T_ACQ_SELECT			(0x79) /* default:488us for 24M clock in */
#define T_ACQ_SELECT_ADC		(0x2f)

/* RTP_CTRL1 bits */
#define STYLUS_UP_DEBOUN(x)		((x) << 12) /* 8 bits */
#define STYLUS_UP_DEBOUN_EN(x)		((x) << 9)
#define STYLUS_UP_DEBOUN_SELECT		(0xf)
#define ADC_CH3_SELECT			BIT(3)
#define ADC_CH2_SELECT			BIT(2)
#define ADC_CH1_SELECT			BIT(1)
#define ADC_CH0_SELECT			BIT(0)
/* the other bits is written in compatible .data */

/* RTP_CTRL2 bits */
#define RTP_SENSITIVE_ADJUST(x)		((x) << 28) /* 4 bits */
#define RTP_MODE_SELECT(x)		((x) << 26) /* 2 bits */
#define PRE_MEA_EN(x)			((x) << 24)
#define PRE_MEA_THRE_CNT(x)		((x) << 0) /* 24 bits */
#define PRE_MEA_THRE_CNT_SELECT		(0xffffff)

/* RTP_CTRL3 bits */
#define FILTER_EN(x)			((x) << 2)
#define FILTER_TYPE(x)			((x) << 0)  /* 2 bits */

/* RTP_INT_FIFOC irq and fifo mask / control bits */
#define TEMP_IRQ_EN(x)			((x) << 18)
#define OVERRUN_IRQ_EN(x)		((x) << 17)
#define DATA_IRQ_EN(x)			((x) << 16)
#define RTP_DATA_XY_CHANGE(x)		((x) << 13)
#define FIFO_TRIG(x)			((x) << 8)  /* 5 bits */
#define DATA_DRQ_EN(x)			((x) << 7)
#define FIFO_FLUSH(x)			((x) << 4)
#define RTP_UP_IRQ_EN(x)		((x) << 1)
#define RTP_DOWN_IRQ_EN(x)		((x) << 0)

/* RTP_INT_FIFOS irq and fifo status bits */
#define TEMP_DATA_PENDING		BIT(18)
#define FIFO_OVERRUN_PENDING		BIT(17)
#define FIFO_DATA_PENDING		BIT(16)
#define RTP_IDLE_FLG			BIT(2)
#define RTP_UP_PENDING			BIT(1)
#define RTP_DOWN_PENDING		BIT(0)

#define	RTP_VENDOR			(0x0001)
#define	RTP_PRODUCT			(0x0001)
#define	RTP_VERSION			(0x0100)

/* Registers which needs to be saved and restored before and after sleeping */
u32 sunxi_rtp_regs_offset[] = {
	RTP_INT_FIFOC,
	RTP_TPR,
};

struct sunxi_rtp_hwdata {
	u32 chopper_en_bitofs;
	u32 touch_pan_cali_en_bitofs;
	u32 tp_dual_en_bitofs;
	u32 tp_mode_en_bitofs;
	u32 tp_adc_select_bitofs;
	bool has_clock;  /* flags for the existence of clock and reset resources */
};

struct sunxi_rtp_config {
	u32 tp_sensitive_adjust;	/* tpadc sensitive parameter, from 0000(least sensitive) to 1111(most sensitive) */

	/* tpadc filter type, eg:(Median Filter Size/Averaging Filter Size)
	 * 00(4/2), 01(5/3), 10(8/4), 11(16/8)
	 */
	u32 filter_type;
};

struct sunxi_rtp {
	struct platform_device *pdev;
	struct device *dev;
	struct input_dev *input;
	struct iio_dev *indio_dev;
	void __iomem *base;
	int irq;
	struct clk *bus_clk;
	struct clk *mod_clk;
	struct reset_control *reset;
	struct pinctrl *pctrl;
	struct sunxi_rtp_hwdata *hwdata;  /* to distinguish platform own register */
	struct sunxi_rtp_config rtp_config;
	u32 regs_backup[ARRAY_SIZE(sunxi_rtp_regs_offset)];
};

static struct sunxi_rtp_hwdata sunxi_rtp_hwdata_v100 = {
	.chopper_en_bitofs        = 8,
	.touch_pan_cali_en_bitofs = 7,
	.tp_dual_en_bitofs        = 6,
	.tp_mode_en_bitofs        = 5,
	.tp_adc_select_bitofs     = 4,
	.has_clock = false,
};

static struct sunxi_rtp_hwdata sunxi_rtp_hwdata_v101 = {
	.chopper_en_bitofs        = 8,
	.touch_pan_cali_en_bitofs = 7,
	.tp_dual_en_bitofs        = 6,
	.tp_mode_en_bitofs        = 5,
	.tp_adc_select_bitofs     = 4,
	.has_clock = true,
};

static const struct of_device_id sunxi_rtp_of_match[] = {
	{ .compatible = "allwinner,sunxi-rtp-v100", .data = &sunxi_rtp_hwdata_v100},
	{ .compatible = "allwinner,sunxi-rtp-v101", .data = &sunxi_rtp_hwdata_v101},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_rtp_of_match);

static int sunxi_rtp_clk_get(struct sunxi_rtp *chip)
{
	struct device *dev = chip->dev;

	chip->reset = devm_reset_control_get(chip->dev, NULL);
	if (IS_ERR(chip->reset)) {
		sunxi_err(dev, "get tpadc reset failed\n");
		return PTR_ERR(chip->reset);
	}

	chip->mod_clk = devm_clk_get(chip->dev, "mod");
	if (IS_ERR(chip->mod_clk)) {
		sunxi_err(dev, "get tpadc mode clock failed\n");
		return PTR_ERR(chip->mod_clk);
	}

	chip->bus_clk = devm_clk_get(chip->dev, "bus");
	if (IS_ERR(chip->bus_clk)) {
		sunxi_err(dev, "get tpadc bus clock failed\n");
		return PTR_ERR(chip->bus_clk);
	}

	return 0;
}

static int sunxi_rtp_clk_enable(struct sunxi_rtp *chip)
{
	int err;
	struct device *dev = chip->dev;

	err = reset_control_reset(chip->reset);
	if (err) {
		sunxi_err(dev, "reset_control_reset() failed\n");
		goto err0;
	}

	err = clk_prepare_enable(chip->mod_clk);
	if (err) {
		sunxi_err(dev, "Cannot enable rtp->mod_clk\n");
		goto err1;
	}

	err = clk_prepare_enable(chip->bus_clk);
	if (err) {
		sunxi_err(dev, "Cannot enable rtp->bus_clk\n");
		goto err2;
	}

	return 0;

err2:
	clk_disable_unprepare(chip->mod_clk);
err1:
	reset_control_assert(chip->reset);
err0:
	return err;
}

static void sunxi_rtp_clk_disable(struct sunxi_rtp *chip)
{
	clk_disable_unprepare(chip->bus_clk);
	clk_disable_unprepare(chip->mod_clk);
	reset_control_assert(chip->reset);
}

static void sunxi_rtp_irq_enable(struct sunxi_rtp *chip)
{
	u32 reg_val;

	/* Active input IRQs */
	reg_val = readl(chip->base + RTP_INT_FIFOC);
	reg_val |= TEMP_IRQ_EN(1) | DATA_IRQ_EN(1) | FIFO_TRIG(1) | FIFO_FLUSH(1) | RTP_UP_IRQ_EN(1) |
		OVERRUN_IRQ_EN(1) | DATA_DRQ_EN(1) | RTP_DOWN_IRQ_EN(1);
	writel(reg_val, chip->base + RTP_INT_FIFOC);
}

static void sunxi_rtp_irq_disable(struct sunxi_rtp *chip)
{
	writel(0, chip->base + RTP_INT_FIFOC);
}

static irqreturn_t sunxi_rtp_irq_handler(int irq, void *dev_id)
{
	struct sunxi_rtp *chip = dev_id;
	struct device *dev = chip->dev;
	u32 irq_sta;
#if IS_ENABLED(CONFIG_RTP_USED_AS_NORMAL_ADC)
	u32 i, data;
#else
	u32 x, y;
	static bool ignore_first_packet = true;
#endif

	/* Read irq flags */
	irq_sta = readl(chip->base + RTP_INT_FIFOS);

	sunxi_rtp_irq_disable(chip);

#if IS_ENABLED(CONFIG_RTP_USED_AS_NORMAL_ADC)
	if (irq_sta & FIFO_DATA_PENDING) {
		sunxi_debug(dev, "sunxi-rtp report adc fifo data\n");
		for (i = 0; i < 4; i++) {
			data = readl(chip->base + RTP_DATA);
			input_event(chip->input, EV_MSC, MSC_SCAN, data);
		}
		input_sync(chip->input);
	}
#else
	if (irq_sta & FIFO_DATA_PENDING) {
		sunxi_debug(dev, "sunxi-rtp fifo data pending\n");

		/* The 1st location reported after an up event is unreliable */
		if (!ignore_first_packet) {
			sunxi_debug(dev, "sunxi-rtp report fifo data\n");
			x = readl(chip->base + RTP_DATA);
			y = readl(chip->base + RTP_DATA);
			input_report_abs(chip->input, ABS_X, x);
			input_report_abs(chip->input, ABS_Y, y);
			/*
			 * The hardware has a separate down status bit, but
			 * that gets set before we get the first location,
			 * resulting in reporting a click on the old location.
			 */
			input_report_key(chip->input, BTN_TOUCH, 1);  /* 1: report touch down event, 0: report touch up event */
			input_sync(chip->input);
		} else {
			sunxi_debug(dev, "sunxi-rtp ignore first fifo data\n");
			ignore_first_packet = false;
		}
	}

	if (irq_sta & RTP_UP_PENDING) {
		sunxi_debug(dev, "sunxi-rtp up pending\n");
		ignore_first_packet = true;
		input_report_key(chip->input, BTN_TOUCH, 0);
		input_sync(chip->input);
	}
#endif
	/* Clear irq flags */
	writel(irq_sta, chip->base + RTP_INT_FIFOS);

	sunxi_rtp_irq_enable(chip);
	return IRQ_HANDLED;
}

static int sunxi_rtp_open(struct input_dev *dev)
{
	struct sunxi_rtp *chip = input_get_drvdata(dev);

	/* Active input IRQs */
	sunxi_rtp_irq_enable(chip);

	return 0;
}

static void sunxi_rtp_close(struct input_dev *dev)
{
	struct sunxi_rtp *chip = input_get_drvdata(dev);

	/* Deactive input IRQs */
	sunxi_rtp_irq_disable(chip);
}

static int sunxi_rtp_resource_get(struct sunxi_rtp *chip)
{
	const struct of_device_id *of_id;
	int err;

	of_id = of_match_device(sunxi_rtp_of_match, chip->dev);
	if (!of_id) {
		sunxi_err(chip->dev, "of_match_device() failed\n");
		return -EINVAL;
	}
	chip->hwdata = (struct sunxi_rtp_hwdata *)(of_id->data);

	chip->base = devm_platform_ioremap_resource(chip->pdev, 0);
	if (IS_ERR(chip->base)) {
		sunxi_err(chip->dev, "Fail to map IO resource\n");
		return PTR_ERR(chip->base);
	}

	/* If there are clock resources, has_clock is true, apply for clock resources;
	 * otherwise, has_clock is false, you do not need to apply for clock resources.
	 */
	if (chip->hwdata->has_clock) {
		err = sunxi_rtp_clk_get(chip);
		if (err) {
			sunxi_err(chip->dev, "sunxi rtp get clock failed\n");
			return err;
		}
	}

	chip->irq = platform_get_irq(chip->pdev, 0);
	err = devm_request_irq(chip->dev, chip->irq, sunxi_rtp_irq_handler, 0, "sunxi-rtp", chip);
	if (err) {
		sunxi_err(chip->dev, "tpadc request irq failed\n");
		return err;
	}

	chip->pctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR(chip->pctrl)) {
		sunxi_debug(chip->dev, "pinctrl_get failed, don't support pinmux\n");
	}

	return 0;
}

static void sunxi_rtp_resource_put(struct sunxi_rtp *chip)
{
	/* TODO:
	 * If there is an operation that needs to be released later, you can add it directly.
	 */
}

static int sunxi_rtp_inputdev_register(struct sunxi_rtp *chip)
{
	int err;

	chip->input = devm_input_allocate_device(chip->dev);
	if (!chip->input) {
		sunxi_err(chip->dev, "devm_input_allocate_device() failed\n");
		return -ENOMEM;
	}

	chip->input->name = "sunxi-rtp";
	chip->input->phys = "sunxi_rtp/input0";
	chip->input->open = sunxi_rtp_open;
	chip->input->close = sunxi_rtp_close;
	chip->input->id.bustype = BUS_HOST;
	chip->input->id.vendor = RTP_VENDOR;
	chip->input->id.product = RTP_PRODUCT;
	chip->input->id.version = RTP_VERSION;
#if IS_ENABLED(CONFIG_RTP_USED_AS_NORMAL_ADC)
	chip->input->evbit[0] = BIT_MASK(EV_MSC);
	__set_bit(EV_MSC, chip->input->evbit);
	__set_bit(MSC_SCAN, chip->input->mscbit);
#else
	chip->input->evbit[0] =  BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	__set_bit(BTN_TOUCH, chip->input->keybit);
	input_set_abs_params(chip->input, ABS_X, 0, 4095, 0, 0);
	input_set_abs_params(chip->input, ABS_Y, 0, 4095, 0, 0);
#endif
	input_set_drvdata(chip->input, chip);

	err = input_register_device(chip->input);
	if (err) {
		sunxi_err(chip->dev, "sunxi rtp register input device failed\n");
		return err;
	}

	return 0;
}

static void sunxi_rtp_inputdev_unregister(struct sunxi_rtp *chip)
{
	input_unregister_device(chip->input);
}

#if IS_ENABLED(CONFIG_RTP_USED_AS_NORMAL_ADC)
static void sunxi_rtp_normal_mode_reg_setup(struct sunxi_rtp *chip)
{
	writel(ADC_FIRST_DLY(ADC_FIRST_DLY_SELECT) | ADC_CLK_DIV(ADC_CLK_DIV_CLK_IN) | T_ACQ(T_ACQ_SELECT_ADC),
			chip->base + RTP_CTRL0);

	/* by default, the channal0 ~ channal3 are opened as ordinary adcs */
	writel(BIT(chip->hwdata->tp_mode_en_bitofs) | BIT(chip->hwdata->tp_adc_select_bitofs) | ADC_CH0_SELECT | ADC_CH1_SELECT |
		ADC_CH2_SELECT | ADC_CH3_SELECT, chip->base + RTP_CTRL1);
}
#else
static void sunxi_rtp_calibrate(struct sunxi_rtp *chip)
{
	u32 reg_val;

	reg_val = readl(chip->base + RTP_CTRL0);
	writel(reg_val | T_ACQ_DEFAULT, chip->base + RTP_CTRL0);

	reg_val = readl(chip->base + RTP_CTRL1);
	writel(reg_val | BIT(chip->hwdata->touch_pan_cali_en_bitofs), chip->base + RTP_CTRL1);
}

static void sunxi_rtp_reg_setup(struct sunxi_rtp *chip)
{
	u32 reg_val;
	struct device_node *np = chip->dev->of_node;
	struct sunxi_rtp_config *rtp_config = &chip->rtp_config;
	rtp_config->tp_sensitive_adjust = 15;
	rtp_config->filter_type = 1;

	sunxi_rtp_calibrate(chip);

	writel(ADC_FIRST_DLY(ADC_FIRST_DLY_SELECT) | ADC_FIRST_DLY_MODE(ADC_FIRST_DLY_MODE_SELECT) |
			ADC_CLK_DIV(ADC_CLK_DIV_SELECT) | FS_DIV(FS_DIV_SELECT) | T_ACQ(T_ACQ_SELECT),
			chip->base + RTP_CTRL0);

	/*
	 * tp_sensitive_adjust is an optional property
	 * tp_mode = 0 : only x and y coordinates, as we don't use dual touch
	 */
	of_property_read_u32(np, "allwinner,tp-sensitive-adjust",
			     &rtp_config->tp_sensitive_adjust);
	writel(RTP_SENSITIVE_ADJUST(rtp_config->tp_sensitive_adjust) | RTP_MODE_SELECT(0),
	       chip->base + RTP_CTRL2);

	reg_val = readl(chip->base + RTP_CTRL2);
	writel(reg_val | PRE_MEA_THRE_CNT(PRE_MEA_THRE_CNT_SELECT), chip->base + RTP_CTRL2);

	/*
	 * Enable median and averaging filter, optional property for
	 * filter type.
	 */
	of_property_read_u32(np, "allwinner,filter-type", &rtp_config->filter_type);
	writel(FILTER_EN(1) | FILTER_TYPE(rtp_config->filter_type), chip->base + RTP_CTRL3);

	/*
	 * Set stylus up debounce to aprox 10 ms, enable debounce, and
	 * finally enable tp mode.
	 */
	reg_val = STYLUS_UP_DEBOUN(STYLUS_UP_DEBOUN_SELECT) | STYLUS_UP_DEBOUN_EN(1)
		| BIT(chip->hwdata->chopper_en_bitofs) | BIT(chip->hwdata->tp_mode_en_bitofs);

	writel(reg_val, chip->base + RTP_CTRL1);
}
#endif

static void sunxi_rtp_reg_destroy(struct sunxi_rtp *chip)
{
	/* TODO:
	 * If there is an operation that needs to be released later, you can add it directly.
	 */
}

static int sunxi_rtp_select_pin_state(struct sunxi_rtp *chip, char *name)
{
	int err = 0;
	struct pinctrl_state *pctrl_state;

	pctrl_state = pinctrl_lookup_state(chip->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		sunxi_err(chip->dev, "pinctrl_lookup_state failed! return %p\n", pctrl_state);
		return err;
	}

	err = pinctrl_select_state(chip->pctrl, pctrl_state);
	if (err) {
		sunxi_err(chip->dev, "pinctrl select state(%s) failed! return %d\n", name, err);
		return err;
	}


	return err;
}

static int sunxi_rtp_hw_init(struct sunxi_rtp *chip)
{
	int err;

	if (chip->hwdata->has_clock) {
		err = sunxi_rtp_clk_enable(chip);
		if (err) {
			sunxi_err(chip->dev, "enable rtp clock failed\n");
			return err;
		}
	}
#if IS_ENABLED(CONFIG_RTP_USED_AS_NORMAL_ADC)
	sunxi_rtp_normal_mode_reg_setup(chip);
#else
	sunxi_rtp_reg_setup(chip);
#endif

	if (chip->pctrl) {
		err = sunxi_rtp_select_pin_state(chip, PINCTRL_STATE_DEFAULT);
		if (err) {
			sunxi_err(chip->dev, "request rtp gpio failed!\n");
			goto err0;
		}
	}

	return 0;

err0:
	sunxi_rtp_select_pin_state(chip, PINCTRL_STATE_SLEEP);
	return err;
}

static void sunxi_rtp_hw_exit(struct sunxi_rtp *chip)
{
	if (chip->pctrl)
		sunxi_rtp_select_pin_state(chip, PINCTRL_STATE_SLEEP);

	sunxi_rtp_reg_destroy(chip);

	if (chip->hwdata->has_clock)
		sunxi_rtp_clk_disable(chip);
}

#if IS_ENABLED(CONFIG_IIO)
static int sunxi_rtp_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val, int *val2, long mask)
{
	int i = 0, ret = 0;
	int id_vol[4];
	struct sunxi_rtp *chip = iio_priv(indio_dev);

	mutex_lock(&indio_dev->mlock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		for (i = 0; i < 4; i++)
			id_vol[i] = readl(chip->base + RTP_DATA) & 0xfff;
		*val = id_vol[chan->channel];
		ret = IIO_VAL_INT;
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&indio_dev->mlock);

	return ret;
}

#define ADC_CHANNEL(_index, _id) {	\
	.type = IIO_VOLTAGE,			\
	.indexed = 1,					\
	.channel = _index,				\
	.address = _index,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.datasheet_name = _id,							\
}

static const struct iio_chan_spec rtp_adc_iio_channels[] = {
	ADC_CHANNEL(0, "rtp_chan0"),
	ADC_CHANNEL(1, "rtp_chan1"),
	ADC_CHANNEL(2, "rtp_chan2"),
	ADC_CHANNEL(3, "rtp_chan3"),
};

static const struct iio_info sunxi_rtp_iio_info = {
	.read_raw = &sunxi_rtp_read_raw,
};

static void sunxi_rtp_remove_iio(void *data)
{
	struct iio_dev *indio_dev = data;

	if (!indio_dev)
		pr_err("indio_dev is null\n");
	else
		iio_device_unregister(indio_dev);

}

static struct sunxi_rtp *sunxi_rtp_iio_init(struct platform_device *pdev)
{
	int ret;
	struct sunxi_rtp *chip;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*chip));
	if (!indio_dev)
		return NULL;

	chip = iio_priv(indio_dev);
	chip->indio_dev = indio_dev;
	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	chip->indio_dev->dev.parent = chip->dev;
	chip->indio_dev->name = chip->pdev->name;
	chip->indio_dev->channels = rtp_adc_iio_channels;
	chip->indio_dev->num_channels = ARRAY_SIZE(rtp_adc_iio_channels);
	chip->indio_dev->info = &sunxi_rtp_iio_info;
	chip->indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_device_register(chip->indio_dev);
	if (ret < 0) {
		dev_err(chip->dev, "unable to register iio device\n");
		return NULL;
	}
	/*
	 * Destroy IIO:the kernel will call the callback to execute sunxi_rtp_remove_iio.
	 */
	ret = devm_add_action(chip->dev, sunxi_rtp_remove_iio, chip->indio_dev);

	if (ret) {
		dev_err(chip->dev, "unable to add iio cleanup action\n");
		goto err_iio_unregister;
	}

	return chip;

err_iio_unregister:
	iio_device_unregister(chip->indio_dev);

	return NULL;
}
#endif

static int sunxi_rtp_probe(struct platform_device *pdev)
{
	struct sunxi_rtp *chip;
	int err;

#if IS_ENABLED(CONFIG_IIO)
	chip = sunxi_rtp_iio_init(pdev);
	if (!chip) {
		sunxi_err(chip->dev, "sunxi rtp iio_init failed\n");
		goto err0;
	}
#else
	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err0;
	}

	chip->pdev = pdev;
	chip->dev = &pdev->dev;
#endif

	err = sunxi_rtp_resource_get(chip);
	if (err) {
		sunxi_err(chip->dev, "sunxi rtp get resource failed\n");
		goto err0;
	}

	err = sunxi_rtp_inputdev_register(chip);
	if (err) {
		sunxi_err(chip->dev, "sunxi rtp inputdev register failed\n");
		goto err1;
	}

	err = sunxi_rtp_hw_init(chip);
	if (err) {
		sunxi_err(chip->dev, "sunxi rtp hw_init failed\n");
		goto err2;
	}

	platform_set_drvdata(pdev, chip);

	sunxi_debug(chip->dev, "sunxi rtp probe success\n");
	return 0;

err2:
	sunxi_rtp_inputdev_unregister(chip);
err1:
	sunxi_rtp_resource_put(chip);
err0:
	return err;
}

static int sunxi_rtp_remove(struct platform_device *pdev)
{
	struct sunxi_rtp *chip = platform_get_drvdata(pdev);

	sunxi_rtp_hw_exit(chip);
	sunxi_rtp_inputdev_unregister(chip);
	sunxi_rtp_resource_put(chip);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static inline void sunxi_rtp_save_regs(struct sunxi_rtp *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_rtp_regs_offset); i++)
		chip->regs_backup[i] = readl(chip->base + sunxi_rtp_regs_offset[i]);
}

static inline void sunxi_rtp_restore_regs(struct sunxi_rtp *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_rtp_regs_offset); i++)
		writel(chip->regs_backup[i], chip->base + sunxi_rtp_regs_offset[i]);
}

static int sunxi_rtp_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtp *chip = platform_get_drvdata(pdev);

	disable_irq(chip->irq);
	sunxi_rtp_save_regs(chip);
	if (chip->hwdata->has_clock)
		sunxi_rtp_clk_disable(chip);

	return 0;
}

static int sunxi_rtp_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtp *chip = platform_get_drvdata(pdev);
	int err;

	if (chip->hwdata->has_clock) {
		err = sunxi_rtp_clk_enable(chip);
		if (err) {
			sunxi_err(chip->dev, "resume: enable rtp clock failed\n");
			return err;
		}
	}
	sunxi_rtp_restore_regs(chip);
	enable_irq(chip->irq);

	return 0;
}

static const struct dev_pm_ops sunxi_rtp_dev_pm_ops = {
	.suspend_noirq = sunxi_rtp_suspend_noirq,
	.resume_noirq = sunxi_rtp_resume_noirq,
};
#define SUNXI_RTP_DEV_PM_OPS (&sunxi_rtp_dev_pm_ops)
#else
#define SUNXI_RTP_DEV_PM_OPS NULL
#endif

static struct platform_driver sunxi_rtp_driver = {
	.driver = {
		.name	= "sunxi-rtp",
		.of_match_table = of_match_ptr(sunxi_rtp_of_match),
		.pm = SUNXI_RTP_DEV_PM_OPS,
	},
	.probe	= sunxi_rtp_probe,
	.remove	= sunxi_rtp_remove,
};

module_platform_driver(sunxi_rtp_driver);

MODULE_DESCRIPTION("Allwinner sunxi resistive touch panel controller driver");
MODULE_AUTHOR("Xu Minghui<Xuminghuis@allwinnertech.com>");
MODULE_AUTHOR("Chen Mingxi<Chenmingxi@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("2.1.1");
