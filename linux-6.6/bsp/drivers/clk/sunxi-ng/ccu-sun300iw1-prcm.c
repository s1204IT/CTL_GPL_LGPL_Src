// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/iopoll.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/module.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/of_platform.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun300iw1-prcm.h"

#define SUNXI_CCU_VERSION               "0.0.2"
#define GPRCM_LPCLK_CTRL_REG	0x0
#define GPRCM_RCCAL_32K_SEL		BIT(24)
#define	GPRCM_XO32K_CLK_SEL		BIT(20)
#define GPRCM_XO32K_CLTRIM		GENMASK(9, 4)
#define GPRCM_XO32K_EN			BIT(2)
#define GPRCM_XO32K_CLTRIM_DEFAULT_VAL  0x180

#define GPRCM_LPCLK_STATUS_REG	0x4
#define GPRCM_XO32K_RDY			BIT(0)

#define GPRCM_RCOSC_CALIB_REG0	0xc
#define	GPRCM_RCO_CALIB			BIT(29)
#define	GPRCM_RSTN				BIT(28)
#define GPRCM_WKUP_EN			BIT(16)
#define GPRCM_WKUP_TIME			(7680 & 0x1FFF)

#define GPRCM_RCOSC_CALIB_REG2	0x14
#define GPRCM_CNT_TARGET		8192

#define GPRCM_RCOSC_CALIB_REG3	0x18

#define GPRCM_REST_CONTROL		0x1c
#define GPRCM_RCCAL_RSTN		BIT(0)

#define SYSRTC_CTRL_RTC_FREQ_SEL		BIT(9)
#define SYSRTC_CTRL_HOSC_SEL			GENMASK(7, 5)
#define SYSRTC_CTRL_HOSC_24M			BIT(6)
#define SYSRTC_CTRL_HOSC_40M			BIT(5)
#define SYSRTC_CTRL_32K_SEL				GENMASK(4, 3)
#define SYSRTC_CTRL_32K_SEL_RCCAL		BIT(3)
#define SYSRTC_CTRL_LFCLK_SEL			BIT(2)

#define XO32K_READY_TIMES		4
#define XO32K_FRE_32000			32000
#define XO32K_FRE_32768			32768
#define HOSC_FRE_24M			24000000
#define HOSC_FRE_40M			40000000
#define SYS32K_EXTERN			0
#define SYS32K_INTERNAL			1

static DEFINE_SPINLOCK(prcm_ccu_lock);

/* rst_def_start */
static struct ccu_reset_map sun300iw1_prcm_ccu_resets[] = {
	[RST_RTC_WDG]		= { 0x001C, BIT(3) },
};
/* rst_def_end */

static void __iomem *map_sysrtc_base(struct device *dev)
{
	struct device_node *np = dev->of_node;
	const __be32 *gprcm_reg;
	u32 count;
	u32 base, size;

	gprcm_reg = of_get_property(np, "sysrtc_reg", &count);
	if ((!gprcm_reg) || (count != sizeof(u32) * 4))
		return NULL;

	base = be32_to_cpu(gprcm_reg[1]);
	size = be32_to_cpu(gprcm_reg[3]);

	return ioremap(base, size);
}

static int enable_rccal_32k(struct device *dev)
{
	u32 val;
	u32 div;
	u32 hosc_fre;
	int ret_val = 0;
	struct clk *hosc_clock;
	struct ccu_reset *reset;

	reset = dev_get_drvdata(dev);

	hosc_clock = devm_clk_get(dev, "hosc");
	if (!IS_ERR(hosc_clock)) {
		hosc_fre = clk_get_rate(hosc_clock);

		if (hosc_fre) {
			writel(GPRCM_CNT_TARGET, reset->base + GPRCM_RCOSC_CALIB_REG2);

			/* 125:  32000 / 256 = 125 (32000 represents a clock rate of 32K)
			 * 32:   8192  / 256 = 125 (8192 represents the fixed count value of RCOSC clock)
			 * 64 bit division operation is not supported, so the following steps should be taken
			 */
			div = hosc_fre / 125 * 32;
			writel(div, reset->base + GPRCM_RCOSC_CALIB_REG3);

			val = readl(reset->base + GPRCM_RCOSC_CALIB_REG0);
			writel(GPRCM_WKUP_TIME | val, reset->base + GPRCM_RCOSC_CALIB_REG0);

			val = readl(reset->base + GPRCM_RCOSC_CALIB_REG0);
			writel(GPRCM_WKUP_EN | val, reset->base + GPRCM_RCOSC_CALIB_REG0);

			val = readl(reset->base + GPRCM_RCOSC_CALIB_REG0);
			writel(GPRCM_RCO_CALIB | val, reset->base + GPRCM_RCOSC_CALIB_REG0);

			val = readl(reset->base + GPRCM_RCOSC_CALIB_REG0);
			writel(GPRCM_RSTN | val, reset->base + GPRCM_RCOSC_CALIB_REG0);

			val = readl(reset->base + GPRCM_REST_CONTROL);
			writel(GPRCM_RCCAL_RSTN | val, reset->base + GPRCM_REST_CONTROL);
		} else {
			sunxi_err(NULL, "hosc clk fre is 0\n");
			ret_val = -EINVAL;
		}
	} else {
		sunxi_err(NULL, "get hosc clk fre err\n");
		ret_val = -EINVAL;
	}

	return ret_val;

}

static void sun300iw1_change_sysrct(struct device *dev, void __iomem *sysrtc_base, int mode)
{
	u32 extern_clk_fre = XO32K_FRE_32000, val;
	struct device_node *node = dev->of_node;
	u32 hosc_fre;
	struct clk *hosc_clock;
	hosc_clock = devm_clk_get(dev, "hosc");

	if (!IS_ERR(hosc_clock)) {
		/* change rtc clk src is xo32k */
		val = readl(sysrtc_base);

		hosc_fre = clk_get_rate(hosc_clock);

		val &= ~SYSRTC_CTRL_HOSC_SEL;

		if (HOSC_FRE_24M == hosc_fre) {
			val |= SYSRTC_CTRL_HOSC_24M;
		} else if (HOSC_FRE_40M == hosc_fre) {
			val |= SYSRTC_CTRL_HOSC_40M;
		} else {
			sunxi_err(dev, "prcm hosc fre get error\n");
		}

		if (mode == SYS32K_EXTERN) {
			if (of_property_read_u32(node, "extern_fre", &extern_clk_fre) != 0) {
				sunxi_err(dev, "Failed to read 'extern_fre' property\n");
				return;
			}

			if (XO32K_FRE_32768 == extern_clk_fre) {
				val |= SYSRTC_CTRL_RTC_FREQ_SEL;
			} else if (XO32K_FRE_32000 == extern_clk_fre) {
				val &= SYSRTC_CTRL_RTC_FREQ_SEL;
			} else {
				sunxi_err(dev, "prcm extern_clk_fre get error\n");
			}

			val &= ~SYSRTC_CTRL_32K_SEL;
			val |= SYSRTC_CTRL_LFCLK_SEL;
		} else if (mode == SYS32K_INTERNAL) {
			val &= ~SYSRTC_CTRL_RTC_FREQ_SEL;
			val &= ~SYSRTC_CTRL_32K_SEL;
			val |= SYSRTC_CTRL_32K_SEL_RCCAL;
		} else {
			sunxi_err(dev, "prcm sysrtc clk mode error\n");
		}
		writel(val, sysrtc_base);
	}

}

static int sun300iw1_extern_32k_enable(struct device *dev)
{
	u32 val, old_val, i, times = 0;
	struct ccu_reset *reset;
	reset = dev_get_drvdata(dev);

	/* enable extern 32k */
	val = readl(reset->base + GPRCM_LPCLK_CTRL_REG);
	old_val = val;
	val &= (~GPRCM_XO32K_CLTRIM);
	val |= GPRCM_XO32K_EN;
	writel(val, reset->base + GPRCM_LPCLK_CTRL_REG);

	for (i = 0; i < 130; i++) {
		msleep(8);
		val = readl(reset->base + GPRCM_LPCLK_STATUS_REG);
		if (val & GPRCM_XO32K_RDY) {
			times++;
			if (XO32K_READY_TIMES == times) {
				return 0;
			}
		} else {
			times = 0;
		}
	}

	old_val &= ~GPRCM_XO32K_EN;
	writel(old_val, reset->base + GPRCM_LPCLK_CTRL_REG);
	sunxi_err(dev, "enable extern 32k error\n");
	return -1;
}

static void sun300iw1_change_rccal(struct device *dev, void __iomem *sysrtc_base)
{
	int ret_val;
	u32 val;
	struct ccu_reset *reset;
	reset = dev_get_drvdata(dev);

	/* change sys32k src is rccal */
	val = readl(reset->base + GPRCM_LPCLK_CTRL_REG);
	val |= GPRCM_RCCAL_32K_SEL;
	writel(val, reset->base + GPRCM_LPCLK_CTRL_REG);

	/* enable rccal */
	ret_val = enable_rccal_32k(dev);
	if (ret_val) {
		sunxi_err(dev, "enable rccal32k error\n");
	}

	sun300iw1_change_sysrct(dev, sysrtc_base, SYS32K_INTERNAL);

	return;
}

static int sun300iw1_sys32k_change(void *args)
{
	u32 val;
	int ret_val;
	struct device *dev = args;
	struct ccu_reset *reset;
	const char *extern_32k_status;
	struct device_node *node = dev->of_node;
	void __iomem *sysrtc_base = map_sysrtc_base(dev);
	reset = dev_get_drvdata(dev);

	/* determine if the 32K configuration is complete? */
	val = readl(reset->base + GPRCM_LPCLK_CTRL_REG);
	if (!(val & (GPRCM_RCCAL_32K_SEL | GPRCM_XO32K_CLK_SEL))) {

		/* start configure 32k */
	    if (of_property_read_string(node, "extern_32k", &extern_32k_status) != 0) {
			sunxi_err(dev, "Failed to read 'extern_32k' property\n");
			return -EINVAL;
		}

		/* determine whether to use external 32k */
		if (strcmp(extern_32k_status, "disabled")) {
			/* enable extern 32k */
			ret_val = sun300iw1_extern_32k_enable(dev);
			if (!ret_val) {
				/* change sys32k src is rccal */
				val = readl(reset->base + GPRCM_LPCLK_CTRL_REG);
				val |= GPRCM_RCCAL_32K_SEL;
				writel(val, reset->base + GPRCM_LPCLK_CTRL_REG);

				/* change lpclk src is xo32k */
				val = readl(reset->base + GPRCM_LPCLK_CTRL_REG);
				val |= GPRCM_XO32K_CLK_SEL;
				writel(val, reset->base + GPRCM_LPCLK_CTRL_REG);

				udelay(40);

				/* change sys32k src is xo32k */
				val = readl(reset->base + GPRCM_LPCLK_CTRL_REG);
				val &= (~GPRCM_RCCAL_32K_SEL);
				writel(val, reset->base + GPRCM_LPCLK_CTRL_REG);

				sun300iw1_change_sysrct(dev, sysrtc_base, SYS32K_EXTERN);
			} else {
				sunxi_warn(dev, "enable extern 32k error, change rccal\n");
				sun300iw1_change_rccal(dev, sysrtc_base);
			}
		} else {
			sun300iw1_change_rccal(dev, sysrtc_base);
		}
	}

	return 0;
}

static int sun300iw1_prcm_ccu_really_probe(struct device *dev)
{
	struct ccu_reset *reset;
	void __iomem *reg;
	int ret;
	static struct task_struct *sys32k_task;

	struct device_node *node = dev->of_node;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	reset = kzalloc(sizeof(*reset), GFP_KERNEL);
	if (!reset) {
		sunxi_err(NULL, "sunxi prcm ccu probe error\n");
		return -1;
	}

	reset->rcdev.of_node = node;
	reset->rcdev.ops = &ccu_reset_ops;
	reset->rcdev.owner = THIS_MODULE;
	reset->rcdev.nr_resets = ARRAY_SIZE(sun300iw1_prcm_ccu_resets);
	reset->base = reg;
	reset->lock = &prcm_ccu_lock;
	reset->reset_map = sun300iw1_prcm_ccu_resets;

	dev_set_drvdata(dev, reset);

	ret = reset_controller_register(&reset->rcdev);
	if (ret) {
		sunxi_err(NULL, "sunxi prcm ccu probe error\n");
		kfree(reset);
	}

	sys32k_task = kthread_run(sun300iw1_sys32k_change, dev, "change_sys32k");
	if (IS_ERR(sys32k_task)) {
		sunxi_err(NULL, "sun300iw1_prcm_ccu change 32k thread creat failed\n");
	}

	sunxi_info(NULL, "sunxi prcm ccu driver version: %s\n", SUNXI_CCU_VERSION);

	return 0;
}

static int sun300iw1_prcm_ccu_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	ret = sun300iw1_prcm_ccu_really_probe(dev);
	if (ret)
		sunxi_err(NULL, "sun300iw1_prcm_ccu probe failed\n");

	return ret;
}

static const struct of_device_id sun300iw1_prcm_ccu_ids[] = {
	{ .compatible = "allwinner,sun300iw1-prcm-ccu" },
	{ }
};

static struct platform_driver sun300iw1_prcm_ccu_driver = {
	.probe	= sun300iw1_prcm_ccu_probe,
	.driver	= {
		.name	= "sun300iw1-prcm-ccu",
		.of_match_table	= sun300iw1_prcm_ccu_ids,
	},
};

static int __init sun300iw1_prcm_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun300iw1_prcm_ccu_driver);
	if (err)
		sunxi_err(NULL, "register ccu sun300iw1_prcm failed\n");

	return err;
}

core_initcall(sun300iw1_prcm_ccu_init);

static void __exit sun300iw1_prcm_ccu_exit(void)
{
	platform_driver_unregister(&sun300iw1_prcm_ccu_driver);
}
module_exit(sun300iw1_prcm_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun300iw1_prcm clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(SUNXI_CCU_VERSION);
