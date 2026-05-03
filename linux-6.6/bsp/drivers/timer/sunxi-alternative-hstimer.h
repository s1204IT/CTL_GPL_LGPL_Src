// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner alternative hstimer driver
 *
 * Copyright (C) 2023 allwinner Co., Ltd.
 */

#ifndef _SUNXI_ALTERNATIVE_HSTIMER_H
#define _SUNXI_ALTERNATIVE_HSTIMER_H

#define HSTIMER_IRQ_EN_REG		0x00
#define HSTIMER_IRQ_EN(x)		BIT(x)
#define HSTIMER_IRQ_STA_REG		0x04
#define HSTIMER_IRQ_STA(x)		BIT(x)
#define HSTIMER_VER_REG			0x10

#define HSTIMER_CTRL_REG(val)		(0x20 * (val) + 0x20)
#define HSTIMER_MODE_SELECT_BIT		(7)
#define HSTIMER_RELOAD			BIT(1)
#define HSTIMER_START			BIT(0)  /* 0: start, 1: stop */

#define HSTIMER_INTV_LO_REG(val)	(0x20 * (val) + 0x20 + 0x04)
#define HSTIMER_INTV_HI_REG(val)	(0x20 * (val) + 0x20 + 0x08)
#define HSTIMER_CURNT_LO_REG(val)	(0x20 * (val) + 0x20 + 0x0c)
#define HSTIMER_CURNT_HI_REG(val)	(0x20 * (val) + 0x20 + 0x10)

#define HSTIMER_MIN_TICKS		0x1
#define HSTIMER_MAX_TICKS		0xFFFFFFFFFFFFFF
#define HSTIMER_LOW_TICKS_MASK		0xFFFFFFFF
#define HSTIMER_MAX_TICKS_MASK		0xFFFFFF

/*
 * A tick represents 5ns, which is converted by the specific time: 1us == 200ticks;
 */
#define USTIMES_TO_TICKS(t)		((t) * 200)

struct sunxi_hstimer {
	struct platform_device	*pdev;
	struct device		*dev;
	void __iomem		*base;
	struct clk		*bus_clk;
	struct reset_control	*reset;
	struct hstimer_cap	hstimer0;
	struct hstimer_cap	hstimer1;
};

#endif /* _SUNXI_ALTERNATIVE_HSTIMER_H */
