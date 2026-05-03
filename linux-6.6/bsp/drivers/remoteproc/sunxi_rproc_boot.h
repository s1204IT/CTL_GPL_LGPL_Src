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
#ifndef SUNXI_RPROC_BOOT_H
#define SUNXI_RPROC_BOOT_H

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/list.h>
#include <linux/remoteproc.h>

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_E907_ECC_ENABLE)
#define RV_CFG_CACHE_INIT_CTRL_REG	(0x0340) /* RV_CFG ICACHE Init Register */
#define RV_CFG_CACHE_INIT_STA_REG	(0x0350) /* RV_CFG ICACHE Init State Register */
#define RV_CFG_ICACHE_ECC_CTRL_REG	(0x0400) /* RV_CFG ICACHE Ctrl Register */
#define BIT_ICACHE_ECC_EN 			(20)
#define BIT_ICACHE_IRQ_EN			(24)
#define RV_CFG_DCACHE_ECC_CTRL_REG	(0x0408) /* RV_CFG DCACHE Ctrl Register */
#define BIT_DCACHE_ECC_EN 			(20)
#define BIT_DCACHE_IRQ_EN			(24)

#define ECC_TIMEOUT	(msecs_to_jiffies(100))
#endif

struct sunxi_rproc_priv;
struct rproc_common_boot;

struct sunxi_rproc_ops {
	int (*resource_get)(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev);
	void (*resource_put)(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev);
	int (*attach)(struct sunxi_rproc_priv *rproc_priv);
	int (*start)(struct sunxi_rproc_priv *rproc_priv);
	int (*stop)(struct sunxi_rproc_priv *rproc_priv);
	int (*reset)(struct sunxi_rproc_priv *rproc_priv);
	/* set remote processors local ram access to arm */
	int (*set_localram)(struct sunxi_rproc_priv *rproc_priv, u32 value);
	/* set remote processors start reg bit */
	int (*set_runstall)(struct sunxi_rproc_priv *rproc_priv, u32 value);
	bool (*is_booted)(struct sunxi_rproc_priv *rproc_priv);
};

struct sunxi_rproc_memory_mapping {
	u64 pa;				/* Address seen on the cpu side */
	u64 da;				/* Address seen on the remote processor side */
	u64 len;
};

struct sunxi_rproc_hifi4_cfg {
	struct clk *pll_clk;		/* pll clock */
	struct clk *mod_clk;		/* mod clock */
	struct clk *mcu_mod_clk;	/* mcu mod clock */
	struct clk *cfg_clk;		/* cfg clock */
	struct clk *ahbs_clk;		/* ahbs clock */
	struct reset_control *mod_rst;	/* mod rst control */
	struct reset_control *cfg_rst;	/* cfg rst control */
	struct reset_control *dbg_rst;	/* dbg rst control */
	struct pinctrl *jtag_pins;	/* dsp jtag pins */
	u32 mod_clk_freq;		/* DSP freq */
	void __iomem *sram_remap;
	void __iomem *hifi4_cfg;
};

struct sunxi_rproc_hifi5_cfg {
	/* TODO */
};

struct sunxi_rproc_e906_cfg {
	struct clk *pubsram_clk;	/* pubsram clock */
	struct clk *mod_clk;		/* mod clock */
	struct clk *cfg_clk;		/* cfg clock */
	struct reset_control *pubsram_rst;  /* pubsram rst control  */
	struct reset_control *mod_rst;	/* mod rst control */
	struct reset_control *cfg_rst;  /* cfg rst control */
	struct reset_control *dbg_rst;	/* dbg rst control */
	void __iomem  *e906_cfg;
};

struct sunxi_rproc_e907_cfg {
	struct rproc_common_boot *com;
	void __iomem *rv_cfg_reg_base;
#ifdef CONFIG_AW_REMOTEPROC_E907_STOP_NOTIFY
	void __iomem *rec_reg;
#endif
};

struct sunxi_rproc_arm64_cfg {
	struct clk *pll_clk;		/* pll clock */
	struct clk *mod_clk;		/* mod clock */
	struct clk *cfg_clk;		/* cfg clock */
	struct clk *ahbs_clk;		/* ahbs clock */
	struct reset_control *mod_rst;	/* mod rst control */
	struct reset_control *cfg_rst;	/* cfg rst control */
	struct reset_control *dbg_rst;	/* dbg rst control */
	struct reset_control *msg_rst;	/* msgbox rst control */
	struct pinctrl *jtag_pins;	/* arm64 jtag pins */
	u32 mod_clk_freq;		/* arm64 freq */
	void __iomem *sram_remap;
	void __iomem *arm64_cfg;
};

struct sunxi_rproc_priv {
	const char *name;
#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	const char *share_irq;
#endif
	struct rproc *rproc;
	struct sunxi_rproc_memory_mapping *mem_maps;
	struct sunxi_rproc_ops *ops;
	struct device *dev;
	struct list_head list;
	int mem_maps_cnt;
	int irq;
	u32 pc_entry;
	void __iomem *io_base;
	void *rproc_cfg;
	void *standby_priv;
	void *priv;
};

struct sunxi_rproc_priv *sunxi_rproc_priv_find(const char *name);
int sunxi_rproc_priv_ops_register(const char *name, struct sunxi_rproc_ops *rproc_ops, void *priv);
int sunxi_rproc_priv_ops_unregister(const char *name);
int sunxi_rproc_priv_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev);
void sunxi_rproc_priv_resource_put(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev);
int sunxi_rproc_priv_attach(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_start(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_stop(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_assert(struct sunxi_rproc_priv *rproc_priv);
int sunxi_rproc_priv_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value);
int sunxi_rproc_priv_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value);
bool sunxi_rproc_priv_is_booted(struct sunxi_rproc_priv *rproc_priv);

#define RPROC_COMMON_CLK_RES			(0)
#define RPROC_COMMON_RST_RES			(1)
#define RPROC_COMMON_REG_RES			(2)

struct rproc_common_res {
	const char *res_name;
	int type;
	union {
		struct reset_control *rst;
		struct clk *clk;
		void __iomem *reg;
	};
	unsigned long val;					/* only for write reg */
	unsigned long clear;				/* only for set bit */
};

struct rproc_clk_opration {
	const char *name;
	uint32_t option;
	void __iomem *reg;
	union {
		const char *parent;				/* for clk_set_parent */
		const char *prop_rate;			/* for clk_set_rate */
		unsigned long offset;			/* for write reg */
	};
	union {
		unsigned long val;				/* for clk_set_rate */
		unsigned long default_freq;		/* for clk_set_rate */
		struct clk *pclk;				/* for clk_set_parent */
		unsigned long *reg_val;			/* for write reg */
		unsigned long set;				/* for set bit */
	};

	union {
		unsigned long clear;
	};

	/* internal member */
	union {
		struct reset_control *rst;
		struct clk *clk;
		void *ptr;
	};
};

struct rproc_common_boot {
	const char *name;
	struct rproc_common_res *res;
	int nr_res;

	struct rproc_clk_opration *start_seq;
	int nr_start_seq;

	struct rproc_clk_opration *stop_seq;
	int nr_stop_seq;

	struct rproc_clk_opration *attach_seq;
	int nr_attach_seq;

	struct rproc_clk_opration *reset_seq;
	int nr_reset_seq;

	struct rproc_clk_opration *suspend_seq;
	int nr_suspend_seq;

	struct rproc_clk_opration *resume_seq;
	int nr_resume_seq;

	/* internal member */
	void * __iomem base_addr;
	struct reset_control *rst;
	int nr_rst;
	struct clk *clk;
	int nr_clk;
};

enum rproc_common_clk_op {
	RPROC_COMMON_OP_DEASSERT = 1,		/* reset_control_deassert */
	RPROC_COMMON_OP_ASSERT,			/* reset_control_assert */
	RPROC_COMMON_OP_CLK_START,
	RPROC_COMMON_OP_SETPARENT,		/* clk_set_parent */
	RPROC_COMMON_OP_SETRATE,		/* clk_set_rate */
	RPROC_COMMON_OP_DISABLE,		/* clk_disable_unprepare */
	RPROC_COMMON_OP_ENABLE,			/* clk_prepare_enable */
	RPROC_COMMON_OP_TRY_DISABLE,	/* clk_disable_unprepare */
	RPROC_COMMON_OP_TRY_ENABLE,		/* clk_prepare_enable */
	RPROC_COMMON_OP_REG_START,
	RPROC_COMMON_OP_WRITEREG,		/* write reg */
	RPROC_COMMON_OP_SETBIT,
	RPROC_COMMON_OP_USLEEP,

	RPROC_COMMON_OP_MAX
};

#define RPROC_COMMON_OP(x)					RPROC_COMMON_OP_##x

struct rproc_common_res *rproc_common_find_res(struct rproc_common_boot *com, const char *name);
int rproc_parse_common_resource(struct platform_device *pdev, struct rproc_common_boot *com);
int rproc_common_start(struct device *dev, struct rproc_common_boot *com);
int rproc_common_stop(struct device *dev, struct rproc_common_boot *com);
int rproc_common_attach(struct device *dev, struct rproc_common_boot *com);
int rproc_common_reset(struct device *dev, struct rproc_common_boot *com);
int rproc_common_suspend(struct device *dev, struct rproc_common_boot *com);
int rproc_common_resume(struct device *dev, struct rproc_common_boot *com);
int rproc_common_set_regval(struct rproc_common_boot *com, const char *name, unsigned long val);

int sunxi_rproc_send_mbox_msg(struct rproc *rproc, u32 data);

#endif
