// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *      sunxi Watchdog Driver
 *
 *      Copyright (c) 2013 Carlo Caione
 *                    2012 Henrik Nordstrom
 *
 *      Based on xen_wdt.c
 *      (c) Copyright 2010 Novell, Inc.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/reset.h>

#define WDT_MAX_TIMEOUT         16
#define WDT_MIN_TIMEOUT         1
#define WDT_TIMEOUT_MASK        0x0F
#define WDT_TIMEOUT_V105_MASK   0x03FF

#define WDT_CTRL_RELOAD         ((1 << 0) | (0x0a57 << 1))

#define WDT_MODE_EN             (1 << 0)
#define WDT_SOFT_RST_EN         (1 << 0)
#define WDT_24M_CLK_SELECT      (1 << 8)
#define DRV_NAME		"sunxi-wdt"
#define DRV_VERSION		"1.0.6"

static bool nowayout = WATCHDOG_NOWAYOUT;
static unsigned int timeout;

/*
 * This structure stores the register offsets for different variants
 * of Allwinner's watchdog hardware.
 */
struct sunxi_wdt_reg {
	u8 wdt_software_reset; /* Offset to WDOG_SRST_REG */
	u8 wdt_ctrl;  /* Offset to WDOG_CTRL_REG */
	u8 wdt_cfg;   /* Offset to WDOG_CFG_REG */
	u8 wdt_mode;  /* Offset to WDOG_MODE_REG */
	u8 wdt_timeout_shift;  /* Bit offset of WDOG_INTV_VALUE in WDOG_MODE_REG */
	u8 wdt_reset_mask;  /* Bit mask of WDOG_CONFIG in WDOG_CFG_REG */
	u8 wdt_reset_val;   /* Value to reset whole system of WDOG_CONFIG in WDOG_CFG_REG */
	u8 wdt_min_intv_value;
	u32 wdt_max_timeout;
	u32 wdt_filed_magic; /* regs->wdt_filed_magic for write watchdog data */
	const int *wdt_timeout_map;
	int (*set_timeout)(struct watchdog_device *, unsigned int);
	int (*restart)(struct watchdog_device *, unsigned long, void *);
};

struct sunxi_wdt_dev {
	struct watchdog_device wdt_dev;
	void __iomem *wdt_base;
	const struct sunxi_wdt_reg *wdt_regs;
};

/*
 * wdt_timeout_map maps the watchdog timer interval value in seconds to
 * the value of the register WDT_MODE at bits .wdt_timeout_shift ~ +3
 *
 * [timeout seconds] = register value
 *
 */

static const int wdt_timeout_map[] = {
	[1] = 0x1,  /* 1s  */
	[2] = 0x2,  /* 2s  */
	[3] = 0x3,  /* 3s  */
	[4] = 0x4,  /* 4s  */
	[5] = 0x5,  /* 5s  */
	[6] = 0x6,  /* 6s  */
	[8] = 0x7,  /* 8s  */
	[10] = 0x8, /* 10s */
	[12] = 0x9, /* 12s */
	[14] = 0xA, /* 14s */
	[16] = 0xB, /* 16s */
};

static const int wdt_timeout_map_v104[] = {
	[1] = 0x5,  /* 1s  */
	[2] = 0x6,  /* 2s  */
	[3] = 0x7,  /* 3s  */
	[4] = 0x8,  /* 4s  */
	[5] = 0x9,  /* 5s  */
	[6] = 0xA,  /* 6s  */
	[8] = 0xB,  /* 8s  */
	[10] = 0xC, /* 10s */
	[12] = 0xD, /* 12s */
	[14] = 0xE, /* 14s */
	[16] = 0xF, /* 16s */
};
static int sunxi_wdt_restart(struct watchdog_device *wdt_dev,
			     unsigned long action, void *data)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	u32 ret = -EINVAL;

	if (regs->restart) {
		ret = regs->restart(wdt_dev, action, data);
	}

	return ret;
}

static int sunxi_wdt_restart_original(struct watchdog_device *wdt_dev,
			     unsigned long action, void *data)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	u32 val;

	/* Set system reset function */
	val = readl(wdt_base + regs->wdt_cfg);
	val &= ~(regs->wdt_reset_mask);
	val |= regs->wdt_reset_val | regs->wdt_filed_magic;
	writel(val, wdt_base + regs->wdt_cfg);

	/* Set lowest timeout and enable watchdog */
	val = readl(wdt_base + regs->wdt_mode);
	val &= ~(WDT_TIMEOUT_MASK << regs->wdt_timeout_shift);
	val |= regs->wdt_min_intv_value << regs->wdt_timeout_shift;
	val |= WDT_MODE_EN | regs->wdt_filed_magic;
	writel(val, wdt_base + regs->wdt_mode);

	/*
	 * Restart the watchdog. The default (and lowest) interval
	 * value for the watchdog is 0.5s.
	 */
	writel(WDT_CTRL_RELOAD, wdt_base + regs->wdt_ctrl);

	while (1) {
		mdelay(5);
	}
	return 0;
}

static int sunxi_wdt_restart_v105(struct watchdog_device *wdt_dev,
			     unsigned long action, void *data)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	u32 val;

	/* Set watchdog disable */
	val = readl(wdt_base + regs->wdt_mode);
	val &= ~WDT_MODE_EN;
	val |= regs->wdt_filed_magic;
	writel(val, wdt_base + regs->wdt_mode);

	/*
	 * Restart the watchdog.
	 */
	val = readl(wdt_base + regs->wdt_software_reset);
	val |= (WDT_SOFT_RST_EN | regs->wdt_filed_magic);
	writel(val, wdt_base + regs->wdt_software_reset);

	while (1) {
		mdelay(5);
	}

	return 0;
}

static int sunxi_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;

	writel(WDT_CTRL_RELOAD, wdt_base + regs->wdt_ctrl);

	return 0;
}

static int sunxi_wdt_set_timeout(struct watchdog_device *wdt_dev,
		unsigned int timeout)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	u32 ret = -EINVAL;

	if (regs->set_timeout) {
		ret = regs->set_timeout(wdt_dev, timeout);
	}

	return ret;
}

static int sunxi_wdt_set_timeout_original(struct watchdog_device *wdt_dev,
		unsigned int timeout)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	u32 reg;

	if (regs->wdt_timeout_map[timeout] == 0)
		timeout++;

	sunxi_wdt->wdt_dev.timeout = timeout;

	reg = readl(wdt_base + regs->wdt_mode);
	reg &= ~(WDT_TIMEOUT_MASK << regs->wdt_timeout_shift);
	reg |= (regs->wdt_timeout_map[timeout] << regs->wdt_timeout_shift) |
		regs->wdt_filed_magic;
	writel(reg, wdt_base + regs->wdt_mode);

	sunxi_wdt_ping(wdt_dev);

	return 0;
}

static int sunxi_wdt_set_timeout_v105(struct watchdog_device *wdt_dev,
		unsigned int timeout)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	u32 reg;

	if (timeout == 0)
		timeout++;

	sunxi_wdt->wdt_dev.timeout = timeout;

	reg = readl(wdt_base + regs->wdt_mode);
	reg &= ~(WDT_TIMEOUT_V105_MASK << regs->wdt_timeout_shift);
	reg |= (timeout * 2 << regs->wdt_timeout_shift) |
		regs->wdt_filed_magic;
	writel(reg, wdt_base + regs->wdt_mode);

	sunxi_wdt_ping(wdt_dev);

	return 0;
}


static int sunxi_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;

	writel((0 | regs->wdt_filed_magic), wdt_base + regs->wdt_mode);

	return 0;
}

static int sunxi_wdt_start(struct watchdog_device *wdt_dev)
{
	u32 reg;
	struct sunxi_wdt_dev *sunxi_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunxi_wdt->wdt_base;
	const struct sunxi_wdt_reg *regs = sunxi_wdt->wdt_regs;
	int ret;

	ret = sunxi_wdt_set_timeout(&sunxi_wdt->wdt_dev,
			sunxi_wdt->wdt_dev.timeout);
	if (ret < 0)
		return ret;

	/* Set system reset function */
	reg = readl(wdt_base + regs->wdt_cfg);
	reg &= ~(regs->wdt_reset_mask);
	reg |= regs->wdt_reset_val | regs->wdt_filed_magic;
	writel(reg, wdt_base + regs->wdt_cfg);

	/* Enable watchdog */
	reg = readl(wdt_base + regs->wdt_mode);
	reg &= ~(WDT_TIMEOUT_MASK << regs->wdt_timeout_shift);
	reg |= regs->wdt_min_intv_value << regs->wdt_timeout_shift;
	reg |= WDT_MODE_EN | regs->wdt_filed_magic;
	writel(reg, wdt_base + regs->wdt_mode);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_wdt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sunxi_wdt_dev *sunxi_wdt = platform_get_drvdata(pdev);
	if (watchdog_active(&sunxi_wdt->wdt_dev))
		sunxi_wdt_stop(&sunxi_wdt->wdt_dev);

	return 0;
}

static int sunxi_wdt_resume(struct platform_device *pdev)
{
	struct sunxi_wdt_dev *sunxi_wdt = platform_get_drvdata(pdev);
	if (watchdog_active(&sunxi_wdt->wdt_dev)) {
		sunxi_wdt_start(&sunxi_wdt->wdt_dev);
	}

	return 0;
}
#endif

static const struct watchdog_info sunxi_wdt_info = {
	.identity	= DRV_NAME,
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_KEEPALIVEPING |
			  WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops sunxi_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= sunxi_wdt_start,
	.stop		= sunxi_wdt_stop,
	.ping		= sunxi_wdt_ping,
	.set_timeout	= sunxi_wdt_set_timeout,
	.restart	= sunxi_wdt_restart,
};

static const struct sunxi_wdt_reg sun4i_wdt_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0x00,
	.wdt_cfg = 0x04,
	.wdt_mode = 0x04,
	.wdt_timeout_shift = 3,
	.wdt_reset_mask = 0x02,
	.wdt_reset_val = 0x02,
	.wdt_min_intv_value = 0x0,
	.wdt_max_timeout = 16,
	.wdt_filed_magic = 0x16AA0000,
	.wdt_timeout_map = wdt_timeout_map,
	.set_timeout  = sunxi_wdt_set_timeout_original,
	.restart	  = sunxi_wdt_restart_original,
};

static const struct sunxi_wdt_reg sun6i_wdt_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0x10,
	.wdt_cfg = 0x14,
	.wdt_mode = 0x18,
	.wdt_timeout_shift = 4,
	.wdt_reset_mask = 0x03,
	.wdt_reset_val = 0x01,
	.wdt_min_intv_value = 0x0,
	.wdt_max_timeout = 16,
	.wdt_filed_magic = 0x16AA0000,
	.wdt_timeout_map = wdt_timeout_map,
	.set_timeout  = sunxi_wdt_set_timeout_original,
	.restart	  = sunxi_wdt_restart_original,
};

static const struct sunxi_wdt_reg wdt_v102_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0x00,
	.wdt_cfg = 0x04,
	.wdt_mode = 0x04,
	.wdt_timeout_shift = 3,
	.wdt_reset_mask = 0x02,
	.wdt_reset_val = 0x02,
	.wdt_min_intv_value = 0x0,
	.wdt_max_timeout = 16,
	.wdt_filed_magic = 0,
	.wdt_timeout_map = wdt_timeout_map,
	.set_timeout  = sunxi_wdt_set_timeout_original,
	.restart	  = sunxi_wdt_restart_original,
};

static const struct sunxi_wdt_reg sun50i_wdt_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0xb0,
	.wdt_cfg = 0xb4,
	.wdt_mode = 0xb8,
	.wdt_timeout_shift = 4,
	.wdt_reset_mask = 0x03,
	.wdt_reset_val = 0x01,
	.wdt_min_intv_value = 0x0,
	.wdt_max_timeout = 16,
	.wdt_filed_magic = 0x16AA0000,
	.wdt_timeout_map = wdt_timeout_map,
	.set_timeout  = sunxi_wdt_set_timeout_original,
	.restart	  = sunxi_wdt_restart_original,
};

static const struct sunxi_wdt_reg wdt_v103_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0x0c,
	.wdt_cfg = 0x10,
	.wdt_mode = 0x14,
	.wdt_timeout_shift = 4,
	.wdt_reset_mask = 0x03,
	.wdt_reset_val = 0x01,
	.wdt_min_intv_value = 0x0,
	.wdt_max_timeout = 16,
	.wdt_filed_magic = 0x16AA0000,
	.wdt_timeout_map = wdt_timeout_map,
	.set_timeout  = sunxi_wdt_set_timeout_original,
	.restart	  = sunxi_wdt_restart_original,
};

static const struct sunxi_wdt_reg wdt_v104_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0x10,
	.wdt_cfg = 0x14,
	.wdt_mode = 0x18,
	.wdt_timeout_shift = 4,
	.wdt_reset_mask = 0x03,
	.wdt_reset_val = 0x01,
	.wdt_min_intv_value = 0x0,
	.wdt_max_timeout = 16,
	.wdt_filed_magic = 0x16AA0000,
	.wdt_timeout_map = wdt_timeout_map_v104,
	.set_timeout  = sunxi_wdt_set_timeout_original,
	.restart	  = sunxi_wdt_restart_original,
};

static const struct sunxi_wdt_reg wdt_v105_reg = {
	.wdt_software_reset = 0x08,
	.wdt_ctrl = 0x10,
	.wdt_cfg = 0x14,
	.wdt_mode = 0x18,
	.wdt_timeout_shift = 4,
	.wdt_reset_mask = 0x03,
	.wdt_reset_val = 0x01,
	.wdt_min_intv_value = 0x1,
	.wdt_max_timeout = 300,
	.wdt_filed_magic = 0x16AA0000,
	.wdt_timeout_map = wdt_timeout_map_v104,
	.set_timeout  = sunxi_wdt_set_timeout_v105,
	.restart	  = sunxi_wdt_restart_v105,
};

static const struct of_device_id sunxi_wdt_dt_ids[] = {
	{ .compatible = "allwinner,sun4i-a10-wdt", .data = &sun4i_wdt_reg },
	{ .compatible = "allwinner,sun6i-a31-wdt", .data = &sun6i_wdt_reg },
	{ .compatible = "allwinner,wdt-v102", .data = &wdt_v102_reg }, /* this compatible is used as sun8iw11 */
	{ .compatible = "allwinner,sun50i-wdt", .data = &sun50i_wdt_reg },
	{ .compatible = "allwinner,wdt-v103", .data = &wdt_v103_reg },
	{ .compatible = "allwinner,wdt-v104", .data = &wdt_v104_reg },
	{ .compatible = "allwinner,wdt-v105", .data = &wdt_v105_reg },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_wdt_dt_ids);

static int sunxi_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_wdt_dev *sunxi_wdt;
	struct device_node *np = pdev->dev.of_node;
	int err, val;
	struct reset_control *wdt_reset;

	sunxi_wdt = devm_kzalloc(dev, sizeof(*sunxi_wdt), GFP_KERNEL);
	if (!sunxi_wdt)
		return -ENOMEM;

	platform_set_drvdata(pdev, sunxi_wdt);
	sunxi_wdt->wdt_regs = of_device_get_match_data(dev);
	if (!sunxi_wdt->wdt_regs)
		return -ENODEV;

	sunxi_wdt->wdt_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sunxi_wdt->wdt_base))
		return PTR_ERR(sunxi_wdt->wdt_base);

	sunxi_wdt->wdt_dev.info = &sunxi_wdt_info;
	sunxi_wdt->wdt_dev.ops = &sunxi_wdt_ops;
	sunxi_wdt->wdt_dev.timeout = sunxi_wdt->wdt_regs->wdt_max_timeout;
	sunxi_wdt->wdt_dev.max_timeout = sunxi_wdt->wdt_regs->wdt_max_timeout;
	sunxi_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
	sunxi_wdt->wdt_dev.parent = dev;

	 /* SYS_24M_CLK clock source select */
	if (of_property_read_bool(np, "sys-24m-clk-select")) {
		val = readl(sunxi_wdt->wdt_base + sunxi_wdt->wdt_regs->wdt_cfg);
		val |= WDT_24M_CLK_SELECT | sunxi_wdt->wdt_regs->wdt_filed_magic;
		writel(val, sunxi_wdt->wdt_base + sunxi_wdt->wdt_regs->wdt_cfg);
	}

	watchdog_init_timeout(&sunxi_wdt->wdt_dev, timeout, dev);
	watchdog_set_nowayout(&sunxi_wdt->wdt_dev, nowayout);
	watchdog_set_restart_priority(&sunxi_wdt->wdt_dev, 128);

	watchdog_set_drvdata(&sunxi_wdt->wdt_dev, sunxi_wdt);

	sunxi_wdt_stop(&sunxi_wdt->wdt_dev);

	watchdog_stop_on_reboot(&sunxi_wdt->wdt_dev);
	err = devm_watchdog_register_device(dev, &sunxi_wdt->wdt_dev);
	if (unlikely(err))
		return err;

	wdt_reset = devm_reset_control_get_optional(&pdev->dev, NULL);
	if (wdt_reset) {
		reset_control_deassert(wdt_reset);
	}

	dev_info(dev, "Watchdog enabled (timeout=%d sec, nowayout=%d), driver version: %s\n",
		 sunxi_wdt->wdt_dev.timeout, nowayout, DRV_VERSION);

	return 0;
}

static void sunxi_wdt_shutdown(struct platform_device *pdev)
{
	struct sunxi_wdt_dev *sunxi_wdt = platform_get_drvdata(pdev);

	sunxi_wdt_stop(&sunxi_wdt->wdt_dev);
}

static struct platform_driver sunxi_wdt_driver = {
	.probe		= sunxi_wdt_probe,
	.shutdown       = sunxi_wdt_shutdown,
	.driver		= {
		.name		= DRV_NAME,
		.of_match_table	= sunxi_wdt_dt_ids,
	},
#ifdef CONFIG_PM
	.suspend        = sunxi_wdt_suspend,
	.resume         = sunxi_wdt_resume,
#endif
};

module_platform_driver(sunxi_wdt_driver);

module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout, "Watchdog heartbeat in seconds");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
		"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Carlo Caione <carlo.caione@gmail.com>");
MODULE_AUTHOR("Henrik Nordstrom <henrik@henriknordstrom.net>");
MODULE_DESCRIPTION("sunxi WatchDog Timer Driver");
MODULE_VERSION(DRV_VERSION);
