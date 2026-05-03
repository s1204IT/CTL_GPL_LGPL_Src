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

/*#define DEBUG */
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

#include "sunxi_rproc_boot.h"
#include "sunxi_rproc_standby.h"
#include "subdev/sunxi_rproc_standby_mgr.h"
#include <sunxi-log.h>

#ifdef dev_fmt
#undef dev_fmt
#define dev_fmt(fmt) fmt
#endif

/*
 * RISC-V CFG Peripheral Register
 */
#define RV_CFG_VER_REG			(0x0000) /* RV_CFG Version Register */
#define RV_CFG_RF1P_CFG_REG		(0x0010) /* RV_CFG Control Register0 */
#define RV_CFG_TS_TMODE_SEL_REG		(0x0040) /* RV_CFG Test Mode Select Register */
#define RV_CFG_STA_ADD_REG		(0x0204) /* RV_CFG Boot Address Register */
#define RV_CFG_WAKEUP_EN_REG		(0x0220) /* RV_CFG WakeUp Enable Register */
#define RV_CFG_WAKEUP_MASK0_REG		(0x0224) /* RV_CFG WakeUp Mask0 Register */
#define RV_CFG_WAKEUP_MASK1_REG		(0x0228) /* RV_CFG WakeUp Mask1 Register */
#define RV_CFG_WAKEUP_MASK2_REG		(0x022C) /* RV_CFG WakeUp Mask2 Register */
#define RV_CFG_WAKEUP_MASK3_REG		(0x0230) /* RV_CFG WakeUp Mask3 Register */
#define RV_CFG_WAKEUP_MASK4_REG		(0x0234) /* RV_CFG WakeUp Mask4 Register */
#define RV_CFG_WORK_MODE_REG		(0x0248) /* RV_CFG Worke Mode Register */

/*
 * RV_CFG Version Register
 */
#define SMALL_VER_MASK			(0x1f << 0)
#define LARGE_VER_MASK			(0x1f << 16)

/*
 * RV_CFG Test Mode Select Register
 */
#define BIT_TEST_MODE			(1 << 1)

/*
 * RV_CFG WakeUp Enable Register
 */
#define BIT_WAKEUP_EN			(1 << 0)

/*
 * RV_CFG Worke Mode Register
 */
#define BIT_LOCK_STA			(1 << 3)
#define BIT_DEBUG_MODE			(1 << 2)
#define BIT_LOW_POWER_MASK		(0x3)
#define BIT_DEEP_SLEEP_MODE		(0x0)
#define BIT_LIGHT_SLEEP_MODE		(0x1)

#define RPROC_NAME "e907"

extern int simulator_debug;

static int sunxi_rproc_e907_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int count);

#define RV_CORE_GATE_CLK_NAME "core-gate"
#define RV_IO_RES_NAME "rv-cfg"

extern struct rproc_common_boot common_boot;

/* E907 standby */
#ifdef CONFIG_AW_REMOTEPROC_E907_STANDBY
/*
 * message handler
 */
#define RPROC_STANDBY_E907_NAME     "e907"
#define STANDBY_MBOX_NAME           "arm-standby"
#define SUSPEND_TIMEOUT             msecs_to_jiffies(1000)
#define RESUME_TIMEOUT              msecs_to_jiffies(300)
#define RESUME_DONE_TIMEOUT         (msecs_to_jiffies(200))
#define E907_STANDBY_SUSPEND        (0xf3f30101)
#define E907_STANDBY_RESUME	        (0xf3f30201)
#define PM_FINISH_FLAG              (0x1)
#define PM_NEED_POWERDOWN_FLAG      (0x2)
#define E907_SUSPEND_FINISH_FIELD   (0x16aa0000)

struct sunxi_standby_mbox {
	struct mbox_chan *chan;
	struct mbox_client client;
};

struct sunxi_e907_standby {
	void __iomem *rec_reg;
	uint32_t running;
	struct sunxi_standby_mbox mb;
	struct completion complete_ack;
	struct completion complete_dead;
	struct delayed_work pm_work;
	struct standby_memory_store memstore[];
};
#endif /* CONFIG_AW_REMOTEPROC_E907_STANDBY */

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY)
#define E907_REBOOT_MSG             (0xf3f30301)
#define INTO_WFI_TIMEOUT            (msecs_to_jiffies(150))
#define E907_NORMAL_MODE            (0x03)
#define E907_READY_TO_STOP	    (0x16aa0001)
#endif

static inline int parse_clk(struct device *dev, struct clk **clk, const char *clk_name)
{
	*clk = devm_clk_get(dev, clk_name);
	if (IS_ERR_OR_NULL(*clk)) {
		dev_err(dev, "get clk '%s' failed, ret: %ld\n", clk_name, PTR_ERR(*clk));
		return -ENXIO;
	}

	return 0;
}

static int sunxi_rproc_e907_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct sunxi_rproc_e907_cfg *cfg;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	u32 *map_array;
	int ret, i, count;
	const char *values_of_power_domain_names[16];
	struct rproc_common_res *res;
#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY)
	uint32_t rec_reg;
#endif

	rproc_priv->dev = dev;

	cfg = devm_kzalloc(dev, sizeof(*cfg), GFP_KERNEL);
	if (!cfg) {
		dev_err(dev, "alloc rproc cfg error\n");
		return -ENOMEM;
	}

	cfg->com = &common_boot;

	ret = rproc_parse_common_resource(pdev, cfg->com);
	if (ret)
		return ret;

	res = rproc_common_find_res(cfg->com, RV_IO_RES_NAME);
	if (!res) {
		dev_err(dev, "fail to find ' " RV_IO_RES_NAME "' res\n");
		return -ENXIO;
	}

	cfg->rv_cfg_reg_base = res->reg;
	cfg->com->base_addr = cfg->rv_cfg_reg_base;
	rproc_priv->io_base = cfg->rv_cfg_reg_base;

	count = of_property_count_strings(np, "power-domain-names");
	if (count > 0) {
		ret = of_property_read_string_array(np, "power-domain-names",
											values_of_power_domain_names, count);
		if (ret < 0) {
			dev_err(dev, "fail to get power domain names\n");
			return -ENXIO;
		}

		ret = sunxi_rproc_e907_attach_pd(dev, values_of_power_domain_names, count);
		if (ret) {
			dev_err(dev, "fail to attach pd\n");
			return -ENXIO;
		}

		pm_runtime_enable(dev);
	}

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret <= 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		ret = -ENXIO;
		goto disadle_pm;
	}
	rproc_priv->mem_maps_cnt = ret;
	rproc_priv->mem_maps = devm_kcalloc(dev, rproc_priv->mem_maps_cnt,
										sizeof(*(rproc_priv->mem_maps)),
										GFP_KERNEL);
	if (!rproc_priv->mem_maps) {
		ret = -ENOMEM;
		goto disadle_pm;
	}

	map_array = devm_kcalloc(dev, rproc_priv->mem_maps_cnt * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array) {
		ret = -ENOMEM;
		goto disadle_pm;
	}

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
									 rproc_priv->mem_maps_cnt * 3);
	if (ret) {
		dev_err(dev, "fail to read memory-mappings\n");
		ret = -ENXIO;
		goto disadle_pm;
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

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY)
	if (of_property_read_u32(np, "standby-record-reg", &rec_reg)) {
		if (of_property_read_u32(np, "stop-record-reg", &rec_reg)) {
			dev_err(dev, "failed to get record-reg for stop notify\n");
			ret = -ENXIO;
			goto disadle_pm;
		}
	}

	cfg->rec_reg = ioremap(rec_reg, 4);
	if (IS_ERR_OR_NULL(cfg->rec_reg)) {
		dev_err(dev, "fail to ioremap rec_reg for stop notify\n");
		ret = -ENXIO;
		goto disadle_pm;
	}
#endif

	rproc_priv->rproc_cfg = cfg;

	return 0;

disadle_pm:
	if (count > 0)
		pm_runtime_disable(dev);

	return ret;
}

static void sunxi_rproc_e907_resource_put(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY)
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;


	iounmap(cfg->rec_reg);
#endif

	pm_runtime_disable(dev);
}

static int sunxi_rproc_e907_start(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	int ret;

	/* clear powerdown flag bit, cpus need close e907 power supply */
#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STANDBY)
	uint32_t reg_val;
	struct sunxi_e907_standby *standby_priv =
			(struct sunxi_e907_standby *)rproc_priv->standby_priv;
	reg_val = readl(standby_priv->rec_reg);
	reg_val &= ~(PM_NEED_POWERDOWN_FLAG);
	writel(reg_val, standby_priv->rec_reg);
#endif

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(dev, "%s,%d rproc does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	dev_info(dev, "boot address: 0x%08x", rproc_priv->pc_entry);

	ret = rproc_common_set_regval(cfg->com, RV_IO_RES_NAME, rproc_priv->pc_entry);
	if (ret < 0) {
		dev_err(dev, "%s,%d rproc set %s regval fialed, ret: %d\n",
				__func__, __LINE__, RV_IO_RES_NAME, ret);
	}

	return rproc_common_start(dev, cfg->com);
}

static int sunxi_rproc_e907_stop(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
#ifdef CONFIG_AW_REMOTEPROC_E907_STANDBY
	struct sunxi_e907_standby *standby_priv =
			(struct sunxi_e907_standby *)rproc_priv->standby_priv;
#endif
#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY)
	int ret;
	unsigned long timeout_jiffies;
	u32 data = E907_REBOOT_MSG;
#endif

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(rproc_priv->dev, "%s,%d rproc does not need to close clk\n",
				__func__, __LINE__);
		return 0;
	}

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY)
	/* notify remote e907 to stop*/
	ret = sunxi_rproc_send_mbox_msg(rproc_priv->rproc, data);
	if (ret < 0) {
		dev_err(rproc_priv->dev, "send mbox msg stop failed\n");
		return -EFAULT;
	}

	/* Wait for e907 to be ready to stop*/
	timeout_jiffies = jiffies + INTO_WFI_TIMEOUT;
	while (readl(cfg->rec_reg) != E907_READY_TO_STOP) {
		if (time_is_before_jiffies(timeout_jiffies)) {
			dev_err(rproc_priv->dev, "riscv wait for ready to stop timeout!");
			return -ENXIO;
		}
		msleep(1);
	}

	/* Wait e907 into wfi mode */
	timeout_jiffies = jiffies + INTO_WFI_TIMEOUT;
	while (readl(cfg->rv_cfg_reg_base + RV_CFG_WORK_MODE_REG) == E907_NORMAL_MODE) {
		if (time_is_before_jiffies(timeout_jiffies)) {
			dev_err(rproc_priv->dev, "riscv wait into wfi mode timeout!");
			return -ENXIO;
		}
		msleep(1);
	}
#endif

	rproc_common_stop(rproc_priv->dev, cfg->com);

#ifdef CONFIG_AW_REMOTEPROC_E907_STANDBY
	writel(E907_SUSPEND_FINISH_FIELD & ~(PM_FINISH_FLAG), standby_priv->rec_reg);
#endif

	pm_runtime_put_sync(rproc_priv->dev);
	return 0;
}

static int sunxi_rproc_e907_attach(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	int ret;

	/* clear powerdown flag bit, cpus need close e907 power supply */
#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_STANDBY)
	uint32_t reg_val;
	struct sunxi_e907_standby *standby_priv =
			(struct sunxi_e907_standby *)rproc_priv->standby_priv;
	reg_val = readl(standby_priv->rec_reg);
	reg_val &= ~(PM_NEED_POWERDOWN_FLAG);
	writel(reg_val, standby_priv->rec_reg);
#endif

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(dev, "%s,%d rproc does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	ret = rproc_common_attach(dev, cfg->com);
	if (ret)
		return ret;

	pm_runtime_get_sync(dev);

	return 0;
}

static int sunxi_rproc_e907_reset(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;

	return rproc_common_reset(dev, cfg->com);
}

static int sunxi_rproc_e907_enable_sram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static int sunxi_rproc_e907_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static bool sunxi_rproc_e907_is_booted(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	struct rproc_common_res *clk_res;

	clk_res = rproc_common_find_res(cfg->com, RV_CORE_GATE_CLK_NAME);

	return __clk_is_enabled(clk_res->clk);
}

static struct sunxi_rproc_ops sunxi_rproc_e907_ops = {
	.resource_get = sunxi_rproc_e907_resource_get,
	.resource_put = sunxi_rproc_e907_resource_put,
	.start = sunxi_rproc_e907_start,
	.stop = sunxi_rproc_e907_stop,
	.attach = sunxi_rproc_e907_attach,
	.reset = sunxi_rproc_e907_reset,
	.set_localram = sunxi_rproc_e907_enable_sram,
	.set_runstall = sunxi_rproc_e907_set_runstall,
	.is_booted = sunxi_rproc_e907_is_booted,
};

static int sunxi_rproc_e907_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int count)
{
	struct device_link *link;
	struct device *pd_dev;
	int i;

	/* Do nothing when in a single power domain */
	if (dev->pm_domain)
		return 0;

	for (i = 0; i < count; i++) {
		pd_dev = dev_pm_domain_attach_by_name(dev, values_of_power_domain_names[i]);
		if (IS_ERR(pd_dev))
			return PTR_ERR(pd_dev);
		/* Do nothing when power domain missing */
		else if (!pd_dev)
			return 0;
		else {
			link = device_link_add(dev, pd_dev,
								   DL_FLAG_STATELESS |
								   DL_FLAG_PM_RUNTIME |
								   DL_FLAG_RPM_ACTIVE);
			if (!link) {
				dev_err(dev, "Failed to add device_link to %s.\n",
						values_of_power_domain_names[i]);
				return -EINVAL;
			}
		}
	}

	return 0;
}

#ifdef CONFIG_AW_REMOTEPROC_E907_STANDBY
static void pm_work_func(struct work_struct *work)
{
	struct delayed_work *pm_work = to_delayed_work(work);
	struct sunxi_e907_standby *e907_standby;
	uint32_t reg_val;

	e907_standby = container_of(pm_work, struct sunxi_e907_standby, pm_work);

	reg_val = readl(e907_standby->rec_reg);
       if (reg_val == (E907_SUSPEND_FINISH_FIELD | PM_FINISH_FLAG)) {
		complete(&e907_standby->complete_dead);
       } else
		schedule_delayed_work(&e907_standby->pm_work, msecs_to_jiffies(50));
}

static void sunxi_rproc_e907_standby_rxcallback(struct mbox_client *cl, void *data)
{
	uint32_t rec_data = *(uint32_t *)data;
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_rproc_standby *rproc_standby;
	struct sunxi_e907_standby *e907_standby;

	rproc_standby = sunxi_rproc_get_standby_handler(rproc->priv);
	if (!rproc_standby) {
		dev_err(cl->dev, "rx get standby resource failed\n");
		return;
	}
	e907_standby = rproc_standby->priv;

	switch (rec_data) {
	case E907_STANDBY_SUSPEND:
	case E907_STANDBY_RESUME:
		complete(&e907_standby->complete_ack);
		break;
	default:
		dev_warn(cl->dev, "rx unkown data(0x%08x)\n", rec_data);
		break;
	}
}

static int sunxi_rproc_e907_standby_request_mbox(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;
	struct device *dev = rproc_standby->dev;

	e907_standby->mb.client.rx_callback = sunxi_rproc_e907_standby_rxcallback;
	e907_standby->mb.client.tx_done = NULL;
	e907_standby->mb.client.tx_block = false;
	e907_standby->mb.client.dev = dev;

	e907_standby->mb.chan = mbox_request_channel_byname(&e907_standby->mb.client,
							STANDBY_MBOX_NAME);
	if (IS_ERR(e907_standby->mb.chan)) {
		if (PTR_ERR(e907_standby->mb.chan) == -EPROBE_DEFER)
		dev_err(dev, "standby get %s mbox failed\n", STANDBY_MBOX_NAME);
		e907_standby->mb.chan = NULL;
		return -EBUSY;
	}

	return 0;
}

static void sunxi_rproc_e907_standby_free_mbox(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;

	mbox_free_channel(e907_standby->mb.chan);
	e907_standby->mb.chan = NULL;
}

static int sunxi_rproc_e907_standby_init(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev)
{
	struct sunxi_e907_standby *e907_standby;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct reserved_mem *rmem;
	uint32_t rec_reg;
	int ret;
	int index = 0;

	if (!rproc_standby || !rproc_standby->rproc_priv)
		return -EINVAL;

	if (of_property_read_u32(np, "standby-record-reg", &rec_reg)) {
		dev_err(dev, "failed to get standby-record-reg property\n");
		return -ENXIO;
	}

	if (of_get_property(np, "standby-ctrl-en", NULL)) {
		if (of_property_read_u32(np, "standby-ctrl-en", &rproc_standby->ctrl_en)) {
			dev_err(dev, "failed to get standby-ctrl-en property\n");
			return -ENXIO;
		}
	} else
		rproc_standby->ctrl_en = 0;

	if (!rproc_standby->ctrl_en) {
		rproc_standby->dev = dev;
		rproc_standby->priv = NULL;
		dev_warn(dev, "standby ctrl_en is 0\n");
		return 0;
	}

	rproc_standby->dev = dev;
	ret = sunxi_rproc_standby_init_prepare(rproc_standby);
	if (ret) {
		dev_err(dev, "standby prepare failed\n");
		return -EINVAL;
	}

	e907_standby = kzalloc(struct_size(e907_standby, memstore, rproc_standby->num_memstore), GFP_KERNEL);
	if (!e907_standby) {
		dev_err(dev, "alloc e907_standby failed\n");
		return -ENOMEM;
	}

	ret = of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	if (ret) {
		dev_err(dev, "fail to pemory-region iterator init for standby, return %d\n", ret);
		return -ENODEV;
	}

	while (of_phandle_iterator_next(&it) == 0) {
		if (of_get_property(it.node, "sleep-data-loss", NULL)) {
			rmem = of_reserved_mem_lookup(it.node);
			if (!rmem) {
				dev_err(dev, "unable to acquire memory-region for standby init\n");
				ret = -EINVAL;
				goto init_free_priv;
			}

			if (index < rproc_standby->num_memstore) {
				e907_standby->memstore[index].start = rmem->base;
				e907_standby->memstore[index].size = rmem->size;
			}
			index++;
		}
	}

	if (index != rproc_standby->num_memstore) {
		dev_err(dev, "standby num_memstore(%d) invalid, index: %d\n", rproc_standby->num_memstore, index);
		ret = -EINVAL;
		goto init_free_priv;
	}

	e907_standby->rec_reg = ioremap(rec_reg, 4);
	if (IS_ERR_OR_NULL(e907_standby->rec_reg)) {
		dev_err(dev, "fail to ioremap e907_standby rec_reg\n");
		ret = -ENXIO;
		goto init_free_priv;
	}

	if (e907_standby->running != 1)
		e907_standby->running = 0;
	rproc_standby->priv = e907_standby;

	ret = sunxi_rproc_e907_standby_request_mbox(rproc_standby);
	if (ret) {
		dev_err(dev, "request e907 standby mbox failed, return %d\n", ret);
		goto init_rec_unmap;
	}
	rproc_standby->rproc_priv->standby_priv = e907_standby;


	init_completion(&e907_standby->complete_ack);
	init_completion(&e907_standby->complete_dead);

	INIT_DELAYED_WORK(&e907_standby->pm_work, pm_work_func);

	dev_info(dev, "e907 standby init end\n");

	return 0;

init_rec_unmap:
	iounmap(e907_standby->rec_reg);

init_free_priv:
	rproc_standby->priv = NULL;
	kfree(e907_standby);
	rproc_standby->ctrl_en = 0;

	return ret;

}

static void sunxi_rproc_e907_standby_exit(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev)
{
	struct sunxi_e907_standby *e907_standby;

	if (!rproc_standby || !rproc_standby->priv)
		return;

	e907_standby = rproc_standby->priv;

	if (rproc_standby->ctrl_en) {
		cancel_delayed_work_sync(&e907_standby->pm_work);

		reinit_completion(&e907_standby->complete_dead);
		reinit_completion(&e907_standby->complete_ack);

		sunxi_rproc_e907_standby_free_mbox(rproc_standby);

		iounmap(e907_standby->rec_reg);
	}

	rproc_standby->priv = NULL;
	kfree(e907_standby);
}

static int sunxi_rproc_e907_standby_start(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;

	e907_standby->running = 1;

	return 0;
}

static int sunxi_rproc_e907_standby_stop(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;

	e907_standby->running = 0;

	return 0;
}

static int sunxi_rproc_e907_standby_prepare(struct sunxi_rproc_standby *rproc_standby)
{
	int i;
	int ret;
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;
	struct device *dev = rproc_standby->dev;
	struct standby_memory_store *store;

	if (!e907_standby->running)
		return 0;

	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e907_standby->memstore[i];
		store->iomap = ioremap(store->start, store->size);
		if (!store->iomap) {
			dev_err(dev, "store ioremap failed, index: %d\n", i);
			ret = -ENOMEM;
			goto data_store_free;
		}

		store->buf = vmalloc(store->size);
		if (!store->buf) {
			dev_err(dev, "store vmalloc failed, index: %d\n", i);
			ret = -ENOMEM;
			goto data_store_free;
		}
	}

	return 0;

data_store_free:
	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e907_standby->memstore[i];
		if (store->iomap) {
			iounmap(store->iomap);
			store->iomap = NULL;
		}

		if (store->buf) {
			vfree(store->buf);
			store->buf = NULL;
		}
	}

	return ret;
}

static void sunxi_rproc_e907_standby_complete(struct sunxi_rproc_standby *rproc_standby)
{
	int i;
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;
	struct standby_memory_store *store;

	if (!e907_standby->running)
		return;

	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e907_standby->memstore[i];
		if (store->iomap) {
			iounmap(store->iomap);
			store->iomap = NULL;
		}

		if (store->buf) {
			vfree(store->buf);
			store->buf = NULL;
		}
	}
}

static int sunxi_rproc_e907_standby_suspend(struct sunxi_rproc_standby *rproc_standby)
{
	int ret;
	int i;
	uint32_t data = E907_STANDBY_SUSPEND;
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;
	struct sunxi_rproc_e907_cfg *e907_cfg = rproc_standby->rproc_priv->rproc_cfg;
	struct standby_memory_store *store;
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
	struct sunxi_rproc_standby_dev *stdby_dev = rproc_standby->stdby_dev;

	if (RPROC_STANDBY_STAY_ALIVE == stdby_dev->stby_mode) {
		/* tell cpus not to close e907 power */
		writel(E907_SUSPEND_FINISH_FIELD | PM_NEED_POWERDOWN_FLAG, e907_standby->rec_reg);
		dev_err(rproc_standby->dev, "do not need suspend in this time\n");
		return 0;
	}
#endif

	if (!e907_standby->running || !sunxi_rproc_e907_is_booted(rproc_standby->rproc_priv))
		return 0;

	ret = mbox_send_message(e907_standby->mb.chan, &data);
	if (ret < 0) {
		dev_err(rproc_standby->dev, "suspend send mbox msg failed\n");
		return ret;
	}

	ret = wait_for_completion_timeout(&e907_standby->complete_ack, SUSPEND_TIMEOUT);
	if (!ret) {
		dev_err(rproc_standby->dev, "timeout return suspend ack\n");
		return -EBUSY;
	}

	/* check whether e907 completes the suspend process */
	schedule_delayed_work(&e907_standby->pm_work, msecs_to_jiffies(50));
	ret = wait_for_completion_timeout(&e907_standby->complete_dead, SUSPEND_TIMEOUT);
	cancel_delayed_work_sync(&e907_standby->pm_work);
	if (!ret) {
		dev_err(rproc_standby->dev, "timeout return suspend dead\n");
		writel(E907_SUSPEND_FINISH_FIELD & ~(PM_FINISH_FLAG), e907_standby->rec_reg);
		return -EBUSY;
	}
	msleep(1);

	/* FIXME: restore if failed
	 * The heartbeat packet is expected to restart the system.
	 */
	rproc_common_suspend(rproc_standby->dev, e907_cfg->com);

	/* store data */
	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e907_standby->memstore[i];
		memcpy_fromio(store->buf, store->iomap, store->size);
	}

	return 0;
}

static int sunxi_rproc_e907_standby_resume(struct sunxi_rproc_standby *rproc_standby)
{
	int ret;
	int i;
	unsigned long timeout_jiffies;
	uint32_t reg_val;
	uint32_t data = E907_STANDBY_RESUME;
	struct sunxi_e907_standby *e907_standby = rproc_standby->priv;
	struct sunxi_rproc_e907_cfg *e907_cfg = rproc_standby->rproc_priv->rproc_cfg;
	struct standby_memory_store *store;
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
	struct sunxi_rproc_standby_dev *stdby_dev = rproc_standby->stdby_dev;

	/* clear powerdown flag bit, cpus need close e907 power supply */
	reg_val = readl(e907_standby->rec_reg);
	reg_val &= ~(PM_NEED_POWERDOWN_FLAG);
	writel(reg_val, e907_standby->rec_reg);

	if (RPROC_STANDBY_STAY_ALIVE == stdby_dev->stby_mode) {
		dev_err(rproc_standby->dev, "do not need suspend in this time\n");
		return 0;
	}
#endif

	if (!e907_standby->running)
		return 0;

	/* restore data */
	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e907_standby->memstore[i];
		memcpy_toio(store->iomap, store->buf, store->size);
	}

	dev_info(rproc_standby->dev, "boot address: 0x%08x", rproc_standby->rproc_priv->pc_entry);
	ret = rproc_common_set_regval(e907_cfg->com, RV_IO_RES_NAME, rproc_standby->rproc_priv->pc_entry);
	if (ret < 0) {
		dev_err(rproc_standby->dev, "%s,%d rproc set %s regval fialed, ret: %d\n",
				__func__, __LINE__, RV_IO_RES_NAME, ret);
		return ret;
	}

	if (!sunxi_rproc_e907_is_booted(rproc_standby->rproc_priv)) {
		rproc_common_resume(rproc_standby->dev, e907_cfg->com);
	}

	timeout_jiffies = jiffies + RESUME_DONE_TIMEOUT;
	reg_val = readl(e907_standby->rec_reg);
	if ((reg_val & 0xffff0000) == E907_SUSPEND_FINISH_FIELD) {
		while (readl(e907_standby->rec_reg) & 0x01) {
			if (time_is_before_jiffies(timeout_jiffies)) {
				dev_err(rproc_standby->dev, "riscv wait resume timeout");
				break;
			}
		}
	}

	ret = mbox_send_message(e907_standby->mb.chan, &data);
	if (ret < 0) {
		dev_err(rproc_standby->dev, "resume send mbox msg failed\n");
		return ret;
	}

	ret = wait_for_completion_timeout(&e907_standby->complete_ack, SUSPEND_TIMEOUT);
	if (!ret) {
		dev_err(rproc_standby->dev, "timeout return resume ack\n");
		return -EBUSY;
	}

	return 0;
}

static struct sunxi_rproc_standby_ops rproc_e907_standby_ops = {
	.init = sunxi_rproc_e907_standby_init,
	.exit = sunxi_rproc_e907_standby_exit,
	.attach = sunxi_rproc_e907_standby_start,
	.detach = sunxi_rproc_e907_standby_stop,
	.start = sunxi_rproc_e907_standby_start,
	.stop = sunxi_rproc_e907_standby_stop,
	.prepare = sunxi_rproc_e907_standby_prepare,
	.suspend = sunxi_rproc_e907_standby_suspend,
	.resume = sunxi_rproc_e907_standby_resume,
	.complete = sunxi_rproc_e907_standby_complete,
};
#endif /* CONFIG_AW_REMOTEPROC_E907_STANDBY */

/* xxx_boot_init must run before sunxi_rproc_init */
static int __init sunxi_rproc_e907_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register(RPROC_NAME, &sunxi_rproc_e907_ops, NULL);
	if (ret) {
		sunxi_err(NULL, "rproc("RPROC_NAME") register sunxi rproc ops failed, ret: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_AW_REMOTEPROC_E907_STANDBY
	ret = sunxi_rproc_standby_ops_register(RPROC_STANDBY_E907_NAME, &rproc_e907_standby_ops, NULL);
	if (ret) {
		pr_err("e907 registers standby ops failed\n");
		return ret;
	}
#endif

	return 0;
}
#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
core_initcall(sunxi_rproc_e907_boot_init);
#else
subsys_initcall(sunxi_rproc_e907_boot_init);
#endif

static void __exit sunxi_rproc_e907_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister(RPROC_NAME);
	if (ret)
		sunxi_err(NULL, "rproc("RPROC_NAME") unregister sunxi rproc ops failed, ret: %d\n", ret);
}
module_exit(sunxi_rproc_e907_boot_exit)

MODULE_DESCRIPTION("Allwinner sunxi rproc e907 boot driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");
