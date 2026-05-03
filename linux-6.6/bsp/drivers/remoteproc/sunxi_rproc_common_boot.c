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
#include <linux/err.h>

#include "sunxi_rproc_boot.h"

struct rproc_common_res *rproc_common_find_res(struct rproc_common_boot *com, const char *name)
{
	int i;

	for (i = 0; i < com->nr_res; i++) {
		if (!strcmp(com->res[i].res_name, name))
			return &com->res[i];
	}

	return NULL;
}
EXPORT_SYMBOL(rproc_common_find_res);

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_ECC_ENABLE)
static int enable_cache_ecc(struct device *dev, struct rproc_clk_opration *op)
{
	unsigned int value;
	unsigned long timeout_jiffies;

	if (strcmp(op->name, "core-gate")) {
		return 0;
	}

	if (0x03 == (readl(op->reg + RV_CFG_CACHE_INIT_STA_REG) & 0x3)) {
		dev_info(dev, "cache ecc had already init!!\n");
	} else {
		value = readl(op->reg + RV_CFG_CACHE_INIT_CTRL_REG);
		value |= 0x3;
		writel(value, (op->reg + RV_CFG_CACHE_INIT_CTRL_REG));

		dev_info(dev, "waiting for e907 cache init done\n");
		/* Wait read complete */
		timeout_jiffies = jiffies + ECC_TIMEOUT;
		while (0x03 != readl(op->reg + RV_CFG_CACHE_INIT_STA_REG)) {
			if (time_is_before_jiffies(timeout_jiffies)) {
				dev_err(dev, "riscv enable cache ecc funtion timeout!");
				break;
			}

			msleep(1);
		}

		value = readl(op->reg + RV_CFG_ICACHE_ECC_CTRL_REG);
		value |= (1 << BIT_ICACHE_ECC_EN);
		value |= (1 << BIT_ICACHE_IRQ_EN);
		writel(value, (op->reg + RV_CFG_ICACHE_ECC_CTRL_REG));

		value = readl(op->reg + RV_CFG_DCACHE_ECC_CTRL_REG);
		value |= (1 << BIT_DCACHE_ECC_EN);
		value |= (1 << BIT_DCACHE_IRQ_EN);
		writel(value, (op->reg + RV_CFG_DCACHE_ECC_CTRL_REG));
	}

	return 0;
}
#endif

static int exec_clk_opration(struct device *dev, struct rproc_clk_opration *op)
{
	int ret;

	if (op->option < RPROC_COMMON_OP_DEASSERT || op->option >= RPROC_COMMON_OP_MAX) {
		dev_err(dev, "'%s' invalid option: %d\n", op->name, op->option);
		return -ENXIO;
	}

	switch (op->option) {
	case RPROC_COMMON_OP_DEASSERT:  /* reset_control_deassert */
		dev_dbg(dev, "%s deassert [%px]\n", op->name, op->rst);
		ret = reset_control_deassert(op->rst);
		if (ret) {
			dev_err(dev, "%s deassert err, ret: %d\n", op->name, ret);
			return ret;
		}
		break;
	case RPROC_COMMON_OP_ASSERT:  /* reset_control_assert */
		dev_dbg(dev, "%s assert [%px]\n", op->name, op->rst);
		ret = reset_control_assert(op->rst);
		if (ret) {
			dev_err(dev, "%s deassert err, ret: %d\n", op->name, ret);
			return ret;
		}
		break;
	case RPROC_COMMON_OP_SETPARENT:  /* clk_set_parent */
		WARN_ON(!op->pclk);
		dev_dbg(dev, "%s set parent %s [%px,%px]\n", op->name, op->parent, op->clk, op->pclk);
		ret = clk_set_parent(op->clk, op->pclk);
		if (ret) {
			dev_err(dev, "%s set parent %s failed, ret: %d\n", op->name, op->parent, ret);
			return ret;
		}
		break;
	case RPROC_COMMON_OP_SETRATE: /* clk_set_rate */
		dev_dbg(dev, "%s set rate %ld [%px]\n", op->name, op->val, op->clk);
		ret = clk_set_rate(op->clk, op->val);
		if (ret) {
			dev_warn(dev, "set %s rate to %ld failed, ret: %d\n", op->name, op->val, ret);
			return ret;
		}
		break;
	case RPROC_COMMON_OP_DISABLE: /* clk_disable_unprepare */
		dev_dbg(dev, "%s disable_unprepare [%px]\n", op->name, op->clk);
		clk_disable_unprepare(op->clk);
		break;
	case RPROC_COMMON_OP_ENABLE: /* clk_prepare_enable */
		dev_dbg(dev, "%s prepare_enable [%px]\n", op->name, op->clk);
		ret = clk_prepare_enable(op->clk);
		if (ret) {
			dev_err(dev, "clk_prepare_enable failed, name: '%s', ret: %d\n",
					op->name, ret);
			return ret;
		}

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_ECC_ENABLE)
		enable_cache_ecc(dev, op);
#endif

		break;
	case RPROC_COMMON_OP_TRY_DISABLE: /* clk_disable_unprepare */
		if (__clk_is_enabled(op->clk)) {
			dev_dbg(dev, "%s disable_unprepare [%px]\n", op->name, op->clk);
			clk_disable_unprepare(op->clk);
		}
		break;
	case RPROC_COMMON_OP_TRY_ENABLE: /* clk_prepare_enable */
		if (__clk_is_enabled(op->clk))
			break;
		dev_dbg(dev, "%s prepare_enable [%px]\n", op->name, op->clk);
		ret = clk_prepare_enable(op->clk);
		if (ret) {
			dev_err(dev, "clk_prepare_enable failed, name: '%s', ret: %d\n",
					op->name, ret);
			return ret;
		}
		break;
	case RPROC_COMMON_OP_WRITEREG: /* write reg */
		dev_dbg(dev, "writel %lx to %px\n", *op->reg_val, (op->reg + op->offset));
		writel(*op->reg_val, (op->reg + op->offset));
		break;
	case RPROC_COMMON_OP_SETBIT: { /* set bit */
			unsigned long reg_val = readl(op->reg + op->offset);
			dev_dbg(dev, "clear %lx for %lx\n", op->clear, reg_val);
			reg_val &= ~(op->clear);
			dev_dbg(dev, "set %lx for %lx\n", op->set, reg_val);
			reg_val |= op->set;
			dev_dbg(dev, "writel %lx to %px\n", reg_val, (op->reg + op->offset));
			writel(reg_val, (op->reg + op->offset));
		}
		break;
	case RPROC_COMMON_OP_USLEEP: /* usleep */
		dev_dbg(dev, "udelay %ld\n", op->val);
		udelay(op->val);
		break;
	default:
		dev_err(dev, "'%s' invalid option: %d\n", op->name, op->option);
		return -ENXIO;
	}

	return 0;
}

static int parse_common_res(struct device *dev, struct rproc_common_boot *com, struct rproc_clk_opration *op)
{
	int ret;
	uint32_t new_rate;
	struct rproc_common_res *res;
	struct device_node *np = dev->of_node;

	if (op->option < RPROC_COMMON_OP_DEASSERT || op->option >= RPROC_COMMON_OP_MAX) {
		dev_err(dev, "'%s' invalid option: %d\n", op->name, op->option);
		return -ENXIO;
	}

	if (op->option == RPROC_COMMON_OP_USLEEP)
		return 0;

	res = rproc_common_find_res(com, op->name);
	if (!res) {
		dev_err(dev, "%s: resource '%s' not exist\n", __func__, op->name);
		return -ENXIO;
	}

	op->ptr = res->reg;

	if (res->type == RPROC_COMMON_RST_RES && op->option >= RPROC_COMMON_OP_CLK_START) {
		dev_err(dev, "%s is rst source, can't exec %d option\n", op->name, op->option);
		return -ENXIO;
	} else if (res->type == RPROC_COMMON_CLK_RES) {
		if (op->option <= RPROC_COMMON_OP_CLK_START ||
				op->option >= RPROC_COMMON_OP_REG_START) {
			dev_err(dev, "%s is clk source, can't exec %d option\n", op->name, op->option);
			return -ENXIO;
		}
	} else if (res->type == RPROC_COMMON_REG_RES) {
		if (!(op->option > RPROC_COMMON_OP_REG_START)) {
			dev_err(dev, "%s is reg source, can't exec %d option\n", op->name, op->option);
			return -ENXIO;
		}
	}

	switch (op->option) {
	case RPROC_COMMON_OP_SETPARENT:  /* clk_set_parent */
		op->pclk = devm_clk_get(dev, op->parent);
		if (IS_ERR_OR_NULL(op->pclk)) {
			dev_err(dev, "get clk '%s' failed, ret: %ld\n", op->parent, PTR_ERR(op->pclk));
			return -ENXIO;
		}
		dev_dbg(dev, "get clk '%s' success\n", op->parent);
		break;
	case RPROC_COMMON_OP_SETRATE: /* clk_set_rate */
		ret = of_property_read_u32(np, op->prop_rate, &new_rate);
		if (ret) {
			dev_err(dev, "parse dts property '%s' failed, ret: %d", op->prop_rate, ret);
			return -ENXIO;
		}
		op->val = new_rate;
		dev_dbg(dev, "get clk %s rate from dts('%ld') success\n", op->name, op->val);
		break;
	case RPROC_COMMON_OP_WRITEREG: /* write reg */
		op->reg_val = &res->val;
		break;
	}

	return 0;
}

int rproc_parse_common_resource(struct platform_device *pdev, struct rproc_common_boot *com)
{
	struct device *dev = &pdev->dev;
	int i, ret = 0;

	for (i = 0; i < com->nr_res; i++) {
		if (com->res[i].type == RPROC_COMMON_CLK_RES) {
			struct clk *clk;
			clk = devm_clk_get(dev, com->res[i].res_name);
			if (IS_ERR_OR_NULL(clk)) {
				dev_err(dev, "get clk '%s' failed, ret: %ld\n", com->res[i].res_name, PTR_ERR(clk));
				return -ENXIO;
			}
			dev_dbg(dev, "get clk '%s' success [%px]\n", com->res[i].res_name, clk);
			com->res[i].clk = clk;
		} else if (com->res[i].type == RPROC_COMMON_RST_RES) {
			struct reset_control *rst;
			rst = devm_reset_control_get(dev, com->res[i].res_name);
			if (IS_ERR_OR_NULL(rst)) {
				dev_err(dev, "get rst '%s' failed, ret: %ld\n", com->res[i].res_name, PTR_ERR(rst));
				return -ENXIO;
			}
			dev_dbg(dev, "get rst '%s' success [%px]\n", com->res[i].res_name, rst);
			com->res[i].rst = rst;
		} else if (com->res[i].type == RPROC_COMMON_REG_RES) {
			struct resource *res;
			res = platform_get_resource_byname(pdev, IORESOURCE_MEM, com->res[i].res_name);
			if (IS_ERR_OR_NULL(res)) {
				dev_err(dev, "get io resource '%s' failed, ret: %ld\n",
						com->res[i].res_name, PTR_ERR(res));
				return -ENXIO;
			}

			com->res[i].reg = devm_ioremap_resource(dev, res);
			if (IS_ERR_OR_NULL(com->res[i].reg)) {
				dev_err(dev, "ioremap resource '%s' failed, ret: %ld\n",
						res->name, PTR_ERR(com->res[i].reg));
				return -ENXIO;
			}
			dev_info(dev, "%s base: 0x%08lx, va: 0x%px\n", com->res[i].res_name, (unsigned long)res->start,
							com->res[i].reg);
		} else {
			dev_err(dev, "Unknow type '%d'\n", com->res[i].type);
			return -ENXIO;
		}
	}

	if (com->nr_start_seq) {
		for (i = 0; i < com->nr_start_seq; i++) {
			ret = parse_common_res(dev, com, &com->start_seq[i]);
			if (ret < 0) {
				dev_err(dev, "%s: parse_common_res '%s' failed\n",
						__func__, com->start_seq[i].name);
				return -ENXIO;
			}
		}
	}

	if (com->nr_stop_seq) {
		for (i = 0; i < com->nr_stop_seq; i++) {
			ret = parse_common_res(dev, com, &com->stop_seq[i]);
			if (ret < 0) {
				dev_err(dev, "%s: parse_common_res '%s' failed\n",
						__func__, com->stop_seq[i].name);
				return -ENXIO;
			}
		}
	}

	if (com->nr_attach_seq) {
		for (i = 0; i < com->nr_attach_seq; i++) {
			ret = parse_common_res(dev, com, &com->attach_seq[i]);
			if (ret < 0) {
				dev_err(dev, "%s: parse_common_res '%s' failed\n",
						__func__, com->attach_seq[i].name);
				return -ENXIO;
			}
		}
	}

	if (com->nr_reset_seq) {
		for (i = 0; i < com->nr_reset_seq; i++) {
			ret = parse_common_res(dev, com, &com->reset_seq[i]);
			if (ret < 0) {
				dev_err(dev, "%s: parse_common_res '%s' failed\n",
						__func__, com->reset_seq[i].name);
				return -ENXIO;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(rproc_parse_common_resource);

int rproc_common_start(struct device *dev, struct rproc_common_boot *com)
{
	int i, ret = 0;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!com->nr_start_seq) {
		dev_info(dev, "%s,%d rproc start_seq is null, skip\n",
				__func__, __LINE__);
		return 0;
	}

	for (i = 0; i < com->nr_start_seq; i++) {
		com->start_seq[i].reg = com->base_addr;
		ret = exec_clk_opration(dev, &com->start_seq[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(rproc_common_start);

int rproc_common_stop(struct device *dev, struct rproc_common_boot *com)
{
	int i, ret = 0;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!com->nr_stop_seq) {
		dev_info(dev, "%s,%d rproc start_seq is null, skip\n",
				__func__, __LINE__);
		return 0;
	}

	for (i = 0; i < com->nr_stop_seq; i++) {
		ret = exec_clk_opration(dev, &com->stop_seq[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(rproc_common_stop);

int rproc_common_attach(struct device *dev, struct rproc_common_boot *com)
{
	int i, ret = 0;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!com->nr_attach_seq) {
		dev_info(dev, "%s,%d rproc start_seq is null, skip\n",
				__func__, __LINE__);
		return 0;
	}

	for (i = 0; i < com->nr_attach_seq; i++) {
		com->attach_seq[i].reg = com->base_addr;
		ret = exec_clk_opration(dev, &com->attach_seq[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(rproc_common_attach);

int rproc_common_reset(struct device *dev, struct rproc_common_boot *com)
{
	int i, ret = 0;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!com->nr_reset_seq) {
		dev_info(dev, "%s,%d rproc start_seq is null, skip\n",
				__func__, __LINE__);
		return 0;
	}

	for (i = 0; i < com->nr_reset_seq; i++) {
		ret = exec_clk_opration(dev, &com->reset_seq[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(rproc_common_reset);

int rproc_common_suspend(struct device *dev, struct rproc_common_boot *com)
{
	int i, ret = 0;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!com->nr_suspend_seq) {
		dev_info(dev, "%s,%d rproc suspend_seq is null, skip\n",
				__func__, __LINE__);
		return 0;
	}

	for (i = 0; i < com->nr_suspend_seq; i++) {
		ret = exec_clk_opration(dev, &com->suspend_seq[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(rproc_common_suspend);

int rproc_common_resume(struct device *dev, struct rproc_common_boot *com)
{
	int i, ret = 0;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!com->nr_resume_seq) {
		dev_info(dev, "%s,%d rproc suspend_seq is null, skip\n",
				__func__, __LINE__);
		return 0;
	}

	for (i = 0; i < com->nr_resume_seq; i++) {
		com->resume_seq[i].reg = com->base_addr;
		ret = exec_clk_opration(dev, &com->resume_seq[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(rproc_common_resume);

int rproc_common_set_regval(struct rproc_common_boot *com, const char *name, unsigned long val)
{
	struct rproc_common_res *res;

	res = rproc_common_find_res(com, name);
	if (!res)
		return -ENODEV;
	if (res->type != RPROC_COMMON_REG_RES)
		return -EINVAL;
	res->val = val;
	return 0;
}
EXPORT_SYMBOL(rproc_common_set_regval);
