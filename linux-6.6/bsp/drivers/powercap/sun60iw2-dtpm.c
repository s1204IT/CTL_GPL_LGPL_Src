// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2021 Linaro Limited
 *
 * Author: Daniel Lezcano <daniel.lezcano@linaro.org>
 *
 * DTPM hierarchy description
 */
#include <linux/dtpm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static struct dtpm_node sunxi_hierarchy[] __initdata = {
	[0] = { .name = "sunxi",
		.type = DTPM_NODE_VIRTUAL },
	[1] = { .name = "package",
		.type = DTPM_NODE_VIRTUAL,
		.parent = &sunxi_hierarchy[0] },
	[2] = { .name = "/cpus/cpu@0",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[3] = { .name = "/cpus/cpu@100",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[4] = { .name = "/cpus/cpu@200",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[5] = { .name = "/cpus/cpu@300",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[6] = { .name = "/cpus/cpu@400",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[7] = { .name = "/cpus/cpu@500",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[8] = { .name = "/cpus/cpu@600",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[9] = { .name = "/cpus/cpu@700",
		.type = DTPM_NODE_DT,
		.parent = &sunxi_hierarchy[1] },
	[10] = { /* sentinel */ }
};

static struct of_device_id aw_dtpm_match_table[] __initdata = {
		{.compatible = "arm,sun60iw2p1", .data = sunxi_hierarchy },
		{},
};

static int __init aw_dtpm_init(void)
{
	return dtpm_create_hierarchy(aw_dtpm_match_table);
}
module_init(aw_dtpm_init);

static void __exit aw_dtpm_exit(void)
{
	return dtpm_destroy_hierarchy();
}
module_exit(aw_dtpm_exit);

MODULE_SOFTDEP("pre: panfrost cpufreq-dt");
MODULE_DESCRIPTION("Allwinner DTPM driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dtpm");
MODULE_AUTHOR("Daniel Lezcano <daniel.lezcano@kernel.org");
MODULE_VERSION("1.0.0");
