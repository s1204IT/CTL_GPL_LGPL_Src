/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
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
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <asm/io.h>
#include <linux/remoteproc.h>
#include <linux/mailbox_client.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/of_reserved_mem.h>
#include <linux/arm-smccc.h>

#include "sunxi_rproc_boot.h"
#include "sunxi_rproc_standby.h"

/*
 * Register define
 */
#define HIFI4_ALT_RESET_VEC_REG		(0x0000) /* HIFI4 Reset Control Register */
#define HIFI4_CTRL_REG0			(0x0004) /* HIFI4 Control Register0 */
#define HIFI4_PRID_REG			(0x000c) /* HIFI4 PRID Register */
#define HIFI4_STAT_REG			(0x0010) /* HIFI4 STAT Register */
#define HIFI4_BIST_CTRL_REG		(0x0014) /* HIFI4 BIST CTRL Register */
#define HIFI4_JTRST_REG			(0x001c) /* HIFI4 JTAG CONFIG RESET Register */
#define HIFI4_VER_REG			(0x0020) /* HIFI4 Version Register */

/*
 * HIFI4 Control Register0
 */
#define BIT_RUN_STALL			(0)
#define BIT_START_VEC_SEL		(1)
#define BIT_HIFI4_CLKEN			(2)

#define HIFI4_BOOT_SRAM_REMAP_REG	(0x8)
#define BIT_SRAM_REMAP_ENABLE		(0)

static int sunxi_rproc_arm64_reset(struct sunxi_rproc_priv *rproc_priv);
static int sunxi_rproc_arm64_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value);
static int sunxi_rproc_arm64_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value);

static int devm_sunxi_rproc_arm64_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct sunxi_rproc_arm64_cfg *arm64_cfg;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	u32 *map_array;
	int ret, i;

	rproc_priv->dev = dev;

	arm64_cfg = devm_kzalloc(dev, sizeof(*arm64_cfg), GFP_KERNEL);
	if (!arm64_cfg) {
		dev_err(dev, "alloc arm cfg error\n");
		return -ENOMEM;
	}

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret <= 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		return -ENXIO;
	}
	rproc_priv->mem_maps_cnt = ret;
	rproc_priv->mem_maps = devm_kcalloc(dev, rproc_priv->mem_maps_cnt,
				       sizeof(struct sunxi_rproc_memory_mapping),
				       GFP_KERNEL);
	if (!rproc_priv->mem_maps)
		return -ENOMEM;

	map_array = devm_kcalloc(dev, rproc_priv->mem_maps_cnt * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
					 rproc_priv->mem_maps_cnt * 3);
	if (ret) {
		dev_err(dev, "fail to read memory-mappings\n");
		return -ENXIO;
	}

	for (i = 0; i < rproc_priv->mem_maps_cnt; i++) {
		rproc_priv->mem_maps[i].da = map_array[i * 3];
		rproc_priv->mem_maps[i].len = map_array[i * 3 + 1];
		rproc_priv->mem_maps[i].pa = map_array[i * 3 + 2];
		dev_dbg(dev, "memory-mappings[%d]: da: 0x%llx, len: 0x%llx, pa: 0x%llx\n",
			i, rproc_priv->mem_maps[i].da, rproc_priv->mem_maps[i].len,
			rproc_priv->mem_maps[i].pa);
	}

	devm_kfree(dev, map_array);

	rproc_priv->rproc_cfg = arm64_cfg;

	return 0;
}

static int sunxi_rproc_arm64_attach(struct sunxi_rproc_priv *rproc_priv)
{
	return 0;
}

#define PSCI_FN_BASE                    (0xc4000000U)
#define PSCI_FN(n)                      (PSCI_FN_BASE + (n))
#define PSCI_CPU_OFF                    PSCI_FN(2)
#define PSCI_CPU_ON                     PSCI_FN(3)

static int sunxi_rproc_arm64_start(struct sunxi_rproc_priv *rproc_priv)
{
	//struct sunxi_rproc_arm64_cfg *arm64_cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	struct arm_smccc_res res;
	int ret;
	//u32 reg_val;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	/* reset arm */
	ret = sunxi_rproc_arm64_reset(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc reset err\n");
		return ret;
	}

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	//cpu3
	arm_smccc_smc(PSCI_CPU_ON, 0x300, rproc_priv->pc_entry, 0, 0, 0, 0, 0, &res);

	dev_err(dev, "%s,%d, PSCI_CPU_ON %x, start addr 0x%x\n", __func__, __LINE__, PSCI_CPU_ON, rproc_priv->pc_entry);
	return 0;
}

static int sunxi_rproc_arm64_stop(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);
	ret = sunxi_rproc_arm64_reset(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "rproc reset err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_arm64_reset(struct sunxi_rproc_priv *rproc_priv)
{
	return 0;
}

static int sunxi_rproc_arm64_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static int sunxi_rproc_arm64_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	struct sunxi_rproc_arm64_cfg *arm64_cfg = rproc_priv->rproc_cfg;
	u32 reg_val;

	reg_val = readl(arm64_cfg->arm64_cfg + HIFI4_CTRL_REG0);
	reg_val &= ~(1 << BIT_RUN_STALL);
	reg_val |= (value << BIT_RUN_STALL);
	writel(reg_val, (arm64_cfg->arm64_cfg + HIFI4_CTRL_REG0));

	return 0;
}

static bool sunxi_rproc_arm64_is_booted(struct sunxi_rproc_priv *rproc_priv)
{
	return 0;
}

static struct sunxi_rproc_ops sunxi_rproc_arm64_ops = {
	.resource_get = devm_sunxi_rproc_arm64_resource_get,
	.attach = sunxi_rproc_arm64_attach,
	.start = sunxi_rproc_arm64_start,
	.stop = sunxi_rproc_arm64_stop,
	.reset = sunxi_rproc_arm64_reset,
	.set_localram = sunxi_rproc_arm64_set_localram,
	.set_runstall = sunxi_rproc_arm64_set_runstall,
	.is_booted = sunxi_rproc_arm64_is_booted,
};

/* arm64_boot_init must run before sunxi_rproc probe */
static int __init sunxi_rproc_arm64_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register("arm64", &sunxi_rproc_arm64_ops, NULL);
	if (ret) {
		pr_err("arm register ops failed\n");
		return ret;
	}

	return 0;
}
subsys_initcall(sunxi_rproc_arm64_boot_init);

static void __exit sunxi_rproc_arm64_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister("arm64");
	if (ret)
		pr_err("arm unregister ops failed\n");
}
module_exit(sunxi_rproc_arm64_boot_exit)

MODULE_DESCRIPTION("Allwinner sunxi rproc arm64 boot driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
