// SPDX-License-Identifier: GPL-2.0
/*
 * Hstimer driver for Allwinner Soc
 *
 * Copyright (C) 2023 Allwinner Co., Ltd.
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/clockchips.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/sched_clock.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <sunxi-hstimer.h>
#include "sunxi-alternative-hstimer.h"

/*
 * @ {hstimer0,hstimer1}_used: 0: not used, 1: is being used;
 */
static atomic_t hstimer0_used = ATOMIC_INIT(0);
static atomic_t hstimer1_used = ATOMIC_INIT(0);

static struct sunxi_hstimer *g_sunxi_hstimer;

/*
 * sunxi_hstimer_init - Init hstimer, if mode is 0, hstimer0 will
 * be timed repeatedly, if it is 1, it will only be timed once;
 *
 * @ustimes: Must be in us unit;
 * @mode: 0 or 1, pick one of two;
 * @cb: your callback function;
 * @cb_param: Parameters for your callback function;
 *
 * Returns pointer to the hstimer on success or an error pointer.
 */
struct hstimer_cap *sunxi_hstimer_init(u64 ustimes, enum hstimer_mode mode, hstimer_callback cb, void *cb_param)
{
	struct hstimer_cap *hstimer;
	u32 value;

	if (atomic_read(&hstimer0_used) && atomic_read(&hstimer1_used)) {
		dev_err(g_sunxi_hstimer->dev, "All hstimers are using, init failed!\n");
		return NULL;
	}

	if (!atomic_read(&hstimer0_used)) {
		atomic_inc(&hstimer0_used);
		hstimer = &g_sunxi_hstimer->hstimer0;
		hstimer->num = 0;
		goto find;
	}

	if (!atomic_read(&hstimer1_used)) {
		atomic_inc(&hstimer1_used);
		hstimer = &g_sunxi_hstimer->hstimer1;
		hstimer->num = 1;
	}

find:
	hstimer->ticks = USTIMES_TO_TICKS(ustimes);

	if ((hstimer->ticks > HSTIMER_MAX_TICKS) || (hstimer->ticks < HSTIMER_MIN_TICKS)) {
		dev_err(g_sunxi_hstimer->dev, "Unsupported time: too small or too big!\n");
		return NULL;
	}

	if (mode >= INVALID_MODE) {
		dev_err(g_sunxi_hstimer->dev, "Unsupported hstimer mode!\n");
		return NULL;
	}

	hstimer->cb_hstimer = cb;
	hstimer->param  = cb_param;
	hstimer->hsmode = mode;

	value = readl(g_sunxi_hstimer->base + HSTIMER_IRQ_EN_REG);
	value |= HSTIMER_IRQ_EN(hstimer->num);
	writel(value, g_sunxi_hstimer->base + HSTIMER_IRQ_EN_REG);

	return hstimer;
}
EXPORT_SYMBOL(sunxi_hstimer_init);

void sunxi_hstimer_start(struct hstimer_cap *hstimer)
{
	u32 lo_ticks, hi_ticks;
	u32 value;
	void __iomem	*base = g_sunxi_hstimer->base;

	if (!hstimer || (hstimer->hsmode == INVALID_MODE))
		return;

	lo_ticks = lower_32_bits(hstimer->ticks);
	hi_ticks = upper_32_bits(hstimer->ticks);

	writel(lo_ticks, base + HSTIMER_INTV_LO_REG(hstimer->num));
	writel(hi_ticks, base + HSTIMER_INTV_HI_REG(hstimer->num));

	value = readl(base + HSTIMER_CTRL_REG(hstimer->num));
	value |= (hstimer->hsmode << HSTIMER_MODE_SELECT_BIT);
	value |= HSTIMER_RELOAD;
	value |= HSTIMER_START;

	writel(value, base + HSTIMER_CTRL_REG(hstimer->num));
}
EXPORT_SYMBOL(sunxi_hstimer_start);

void sunxi_hstimer_exit(struct hstimer_cap *hstimer)
{
	u32 value;
	void __iomem	*base = g_sunxi_hstimer->base;

	if (!hstimer)
		return;

	value = readl(base + HSTIMER_CTRL_REG(hstimer->num));
	value &= ~HSTIMER_START;
	writel(value, base + HSTIMER_CTRL_REG(hstimer->num));

	value = readl(base + HSTIMER_IRQ_EN_REG);
	value |= ~HSTIMER_IRQ_EN(hstimer->num);
	writel(value, base + HSTIMER_IRQ_EN_REG);

	hstimer->cb_hstimer = NULL;
	hstimer->param  = NULL;
	hstimer->hsmode = INVALID_MODE;

	if (hstimer->num == 0)
		atomic_set(&hstimer0_used, 0);
	else if (hstimer->num == 1)
		atomic_set(&hstimer1_used, 0);
}
EXPORT_SYMBOL(sunxi_hstimer_exit);

static int sunxi_hstimer_resource_get(struct sunxi_hstimer *chip)
{
	chip->bus_clk = devm_clk_get(chip->dev, "bus");
	if (IS_ERR(chip->bus_clk)) {
		dev_err(chip->dev, "Request bus clock failed\n");
		return -EINVAL;
	}

	chip->reset = devm_reset_control_get(chip->dev, NULL);
	if (IS_ERR_OR_NULL(chip->reset)) {
		dev_err(chip->dev, "Request reset failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_hstimer_clk_init(struct sunxi_hstimer *chip)
{
	int ret;

	ret = reset_control_deassert(chip->reset);
	if (ret)
		return ret;

	ret = clk_prepare_enable(chip->bus_clk);
	if (ret)
		return ret;

	return 0;
}

static irqreturn_t sunxi_hstimer0_irq_handler(int irq, void *dev_id)
{
	struct sunxi_hstimer *chip = dev_id;
	struct hstimer_cap *hstimer = &chip->hstimer0;
	int ret = IRQ_NONE;
	u32 status;

	status = readl(chip->base + HSTIMER_IRQ_STA_REG);

	if (status & HSTIMER_IRQ_STA(0)) {
		status = HSTIMER_IRQ_STA(0);
		writel(status, chip->base + HSTIMER_IRQ_STA_REG);

		if (hstimer->cb_hstimer)
			hstimer->cb_hstimer(hstimer->param);

		ret = IRQ_HANDLED;
	}

	return ret;
}

static irqreturn_t sunxi_hstimer1_irq_handler(int irq, void *dev_id)
{
	struct sunxi_hstimer *chip = dev_id;
	struct hstimer_cap *hstimer = &chip->hstimer1;
	int ret = IRQ_NONE;
	u32 status;

	status = readl(chip->base + HSTIMER_IRQ_STA_REG);

	if (status & HSTIMER_IRQ_STA(1)) {
		status = HSTIMER_IRQ_STA(1);
		writel(status, chip->base + HSTIMER_IRQ_STA_REG);

		if (hstimer->cb_hstimer)
			hstimer->cb_hstimer(hstimer->param);

		ret = IRQ_HANDLED;
	}

	return ret;
}

static int sunxi_hstimer_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct sunxi_hstimer *chip;
	struct resource *res;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (IS_ERR_OR_NULL(chip))
		return -ENOMEM;

	chip->dev = &pdev->dev;
	chip->pdev = pdev;
	g_sunxi_hstimer = chip;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chip->base))
		return PTR_ERR(chip->base);

	ret = sunxi_hstimer_resource_get(chip);
	if (ret) {
		dev_err(&pdev->dev, "sunxi hstimer of resource get failed\n");
		return ret;
	}

	ret = sunxi_hstimer_clk_init(chip);
	if (ret) {
		dev_err(&pdev->dev, "sunxi hstimer of clk init failed\n");
		return ret;
	}

	irq = platform_get_irq_byname(pdev, "hstimer0");
	if (irq < 0)
		return -EINVAL;

	ret = devm_request_irq(&pdev->dev, irq, sunxi_hstimer0_irq_handler,
			       0, "hstimer0", chip);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request sunxi hstimer0 IRQ\n");
		return ret;
	}

	irq = platform_get_irq_byname(pdev, "hstimer1");
	if (irq < 0)
		return -EINVAL;

	ret = devm_request_irq(&pdev->dev, irq, sunxi_hstimer1_irq_handler,
			       0, "hstimer1", chip);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request sunxi hstimer1 IRQ\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id sunxi_hstimer_ids[] = {
	{ .compatible = "allwinner,sunxi-alternative-hstimer" },
	{}
};

static struct platform_driver sunxi_hstimer_driver = {
	.probe  = sunxi_hstimer_probe,
	.driver = {
		.name  = "sunxi-hstimer",
		.of_match_table = sunxi_hstimer_ids,
	},
};

static int __init sunxi_hstimer_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_hstimer_driver);
	if (ret)
		pr_err("Register sunxi-hstimer failed!\n");

	return ret;
}
module_init(sunxi_hstimer_driver_init);

static void __exit sunxi_hstimer_driver_exit(void)
{
	return platform_driver_unregister(&sunxi_hstimer_driver);
}
module_exit(sunxi_hstimer_driver_exit);

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harry@allwinnertech.com");
