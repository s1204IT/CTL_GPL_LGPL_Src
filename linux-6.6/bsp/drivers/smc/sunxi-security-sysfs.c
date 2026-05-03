// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/err.h>
#include <sunxi-log.h>
#include <sunxi-smc.h>
#include <uapi/security/sunxi-drm-heap.h>

static struct sunxi_drm_info sunxi_drm_info;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static ssize_t drm_region_show(const struct class *class, const struct class_attribute *attr, char *buf)
#else
static ssize_t drm_region_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	int err = 0;

	err = optee_probe_drm_configure(&sunxi_drm_info.drm_base, &sunxi_drm_info.drm_size, &sunxi_drm_info.tee_base);
	if (err) {
		pr_err("optee_probe_drm_configure() failed\n");
		return err;
	}

	return sprintf(buf, "drm_base=0x%lx\ndrm_size=0x%lx\ntee_base=0x%lx\n",
			sunxi_drm_info.drm_base,
			sunxi_drm_info.drm_size,
			sunxi_drm_info.tee_base);
}

static struct class_attribute security_attrs[] = {
	__ATTR(drm_region,	S_IRUGO,	drm_region_show,	NULL),
};

static struct class *security_class;
#define CLASS_NAME		"sunxi_security"

static int sunxi_security_sysfs_init(void)
{
	int err;
	int i;

	/* sys/class/sunxi-security */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	security_class = class_create(CLASS_NAME);
#else
	security_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(security_class)) {
		sunxi_err(NULL, "class_create('%s') failed\n", CLASS_NAME);
		return PTR_ERR(security_class);
	}

	/* sys/class/sunxi-security/... */
	for (i = 0; i < ARRAY_SIZE(security_attrs); i++) {
		err = class_create_file(security_class, &security_attrs[i]);
		if (err) {
			sunxi_err(NULL, "class_create_file('%s') failed\n", security_attrs[i].attr.name);
			goto abort;
		}
	}

	return 0;

abort:
	while (i--)
		class_remove_file(security_class, &security_attrs[i]);
	class_destroy(security_class);
	return err;
}

static void sunxi_security_sysfs_exit(void)
{
	int i = ARRAY_SIZE(security_attrs);
	while (i--)
		class_remove_file(security_class, &security_attrs[i]);
	class_destroy(security_class);
}

module_init(sunxi_security_sysfs_init);
module_exit(sunxi_security_sysfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_VERSION("V0.0.1");
MODULE_DESCRIPTION("sunxi security sysfs driver");
