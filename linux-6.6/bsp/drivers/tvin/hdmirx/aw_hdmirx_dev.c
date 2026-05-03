/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/string.h>
//#include <linux/stdlib.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include "aw_hdmirx_drv.h"
#include "aw_hdmirx_dev.h"
#include "sunxi-log.h"
#include "aw_hdmirx_define.h"

aw_device_rx_t aw_dev_hdmirx;

extern struct aw_hdmirx_driver *g_hdmirx_drv;
static u8 reg_region;
static unsigned long start_reg;
static unsigned long read_count;

static struct dentry *gHdmirx_debugfs_dir;

static const struct dev_pm_ops hdmirx_pm_ops = {
	.suspend = aw_hdmirx_drv_suspend,
	.resume = aw_hdmirx_drv_resume,
};

/**
 * @short of_device_id structure
 */
static const struct of_device_id dw_hdmi_rx[] = {
	{ .compatible =	"allwinner,sunxi-hdmirx" },
	{ }
};
MODULE_DEVICE_TABLE(of, dw_hdmi_rx);

/**
 * @short Platform driver structure
 */
static struct platform_driver __refdata dw_hdmirx_pdrv = {
	.remove = aw_hdmirx_drv_remove,
	.probe  = aw_hdmirx_drv_probe,
	.driver = {
		.name = "allwinner,sunxi-hdmirx",
		.owner = THIS_MODULE,
		.of_match_table = dw_hdmi_rx,
		.pm = &hdmirx_pm_ops,
	},
};

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
#if IS_ENABLED(CONFIG_PM_SLEEP)
static SIMPLE_DEV_PM_OPS(aw_hdmirx_cec_pm_ops,
		aw_hdmirx_cec_suspend, aw_hdmirx_cec_resume);
#endif

static struct platform_driver aw_hdmirx_cec_driver = {
	.probe	= aw_hdmirx_cec_probe,
	.remove	= aw_hdmirx_cec_remove,
	.driver = {
		.name = "aw-hdmirx-cec",
#if IS_ENABLED(CONFIG_PM_SLEEP)
		.pm   = &aw_hdmirx_cec_pm_ops,
#endif
	},
};

#endif

static int __parse_dump_str(const char *buf, size_t size,
				unsigned long *start, unsigned long *end)
{
	char *ptr = NULL;
	char *ptr2 = (char *)buf;
	int ret = 0, times = 0;

	/* Support single address mode, some time it haven't ',' */
next:

	/* Default dump only one register(*start =*end).
	If ptr is not NULL, we will cover the default value of end. */
	if (times == 1)
		*start = *end;

	if (!ptr2 || (ptr2 - buf) >= size)
		goto out;

	ptr = ptr2;
	ptr2 = strnchr(ptr, size - (ptr - buf), ',');
	if (ptr2) {
		*ptr2 = '\0';
		ptr2++;
	}

	ptr = strim(ptr);
	if (!strlen(ptr))
		goto next;

	ret = kstrtoul(ptr, 16, end);
	if (!ret) {
		times++;
		goto next;
	} else {
		hdmirx_wrn("String syntax errors: %s\n", ptr);
	}

out:
	return ret;
}

static ssize_t hdmirx_reg_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i = 0;
	u32 value = 0;

	if ((!reg_region) || (reg_region > ((sizeof(hdmirx_regs) / sizeof(struct hdmirx_reg_t)) - 1))) {
		/* print help information */
		n += sprintf(buf + n, "echo [region_number] > /sys/class/hdmirx/hdmirx/attr/reg_dump\n");
		n += sprintf(buf + n, "cat /sys/class/hdmirx/hdmirx/attr/reg_dump\n");

		n += sprintf(buf + n, "\nregion_number:\n");
		for (i = 1; i < (sizeof(hdmirx_regs) / sizeof(struct hdmirx_reg_t)); i++) {
			n += sprintf(buf + n, "%d: %s\n", i, hdmirx_regs[i].name);
		}

		reg_region = 0;
		return n;
	}

	/* start to dump register */
	n += sprintf(buf + n, "\n%d: %s\n", reg_region, hdmirx_regs[reg_region].name);

	for (i = hdmirx_regs[reg_region].start; i < hdmirx_regs[reg_region].end; i++) {
		if ((i % 16) == 0 || i == hdmirx_regs[reg_region].start) {
			if ((i + 0x0f) < hdmirx_regs[reg_region].end) {
				n += sprintf(buf + n, "\n0x%04x-0x%04x:",
						i, (i + 0x0f) - (i % 16));
			} else {
				n += sprintf(buf + n, "\n0x%04x-0x%04x:",
						i, hdmirx_regs[reg_region].end-1);
			}
		}

		n += sprintf(buf + n, " 0x%02x", aw_hdmirx_drv_read(i));
	}
	n += sprintf(buf + n, "\n");

	reg_region = 0;
	return n;
}

ssize_t hdmirx_reg_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char *end;
	reg_region = (u8)simple_strtoull(buf, &end, 0);
	return count;
}

static ssize_t hdmirx_reg_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i;
	int j;

	if (!read_count) {
		n += sprintf(buf + n, "[Usage]\n");
		n += sprintf(buf + n, "    echo offset count > reg_read; cat reg_read\n");
		n += sprintf(buf + n, "[Params]\n");
		n += sprintf(buf + n, "    offset: register offset address\n");
		n += sprintf(buf + n, "    count : read count\n");
		n += sprintf(buf + n, "    offset + count < 1MB\n");
		return n;
	}

	n += sprintf(buf + n, "start_reg: 0x%lx count: %ld\n",
			start_reg, read_count);

	for (i = start_reg; i < start_reg + read_count; i++) {
		/* print address */
		if (((i % 16) == 0) || (i == start_reg)) {
			if ((i + 0x0f) < (start_reg + read_count)) {
				n += sprintf(buf + n, "\n0x%04x-0x%04x:",
						i, (i + 0x0f) - (i % 16));
			} else {
				n += sprintf(buf + n, "\n0x%04x-0x%04x:",
						i, (unsigned int)(start_reg + read_count - 1));
			}
		}

		/* align first line */
		if (i == start_reg) {
			for (j = 0; j < (i % 16); j++)
					n += sprintf(buf + n, " 	");
		}
		printk("i = %d\n", i);
		n += sprintf(buf + n, " 0x%02x", aw_hdmirx_drv_read(i));
	}
	n += sprintf(buf + n, "\n");

	start_reg = 0;
	read_count = 0;
	return n;
}

ssize_t hdmirx_reg_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 *separator;

	separator = strchr(buf, ',');
	if (separator != NULL) {
		if (__parse_dump_str(buf, count, &start_reg, &read_count)) {
			hdmirx_err("%s: parse buf error: %s\n", __func__, buf);
			return -EINVAL;
		}
	} else {
		separator = strchr(buf, ' ');
		if (separator != NULL) {
			start_reg = simple_strtoul(buf, NULL, 0);
			read_count = simple_strtoul(separator + 1, NULL, 0);
		} else {
			start_reg = simple_strtoul(buf, NULL, 0);
			read_count = 1;
		}
	}

	if (start_reg + read_count > 0x100000) {
		hdmirx_err("%s: start_reg + read_count must no more than 0xFFFFF: %s\n", __func__);
		start_reg = 0;
		read_count = 0;
		return -EINVAL;
	}
	return count;
}

static ssize_t hdmirx_reg_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n, "[Usage]\n");
	n += sprintf(buf + n, "    echo offset value > reg_write\n");
	n += sprintf(buf + n, "[Params]\n");
	n += sprintf(buf + n, "    offset: register offset address\n");
	n += sprintf(buf + n, "    value : write value\n");
	return n;
}

ssize_t hdmirx_reg_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;
	u8 *separator1 = NULL;
	u8 *separator2 = NULL;
	struct aw_hdmirx_core_s *core = g_hdmirx_drv->hdmirx_core;

	separator1 = strchr(buf, ',');
	separator2 = strchr(buf, ' ');
	if (separator1 != NULL) {
		if (__parse_dump_str(buf, count, &reg_addr, &value)) {
			hdmirx_err("%s: parse buf error: %s\n", __func__, buf);
			return -EINVAL;
		}
		goto cmd_write;
	} else if (separator2 != NULL) {
		reg_addr = simple_strtoul(buf, NULL, 0);
		value = simple_strtoul(separator2 + 1, NULL, 0);
		goto cmd_write;
	} else {
		hdmirx_err("%s: error input: %s\n", __func__, buf);
		return -EINVAL;
	}

cmd_write:
	hdmirx_inf("write reg: 0x%lx value: %lx\n",
		reg_addr, value);
	aw_hdmirx_drv_write(reg_addr, value);

	return count;
}

static ssize_t hdmirx_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "Current debug=%d\n\n", gHdmirx_log_level);

	n += sprintf(buf + n, "hdmi log debug level:\n");
	n += sprintf(buf + n, "debug = 1, print video log\n");
	n += sprintf(buf + n, "debug = 2, print edid log\n");
	n += sprintf(buf + n, "debug = 3, print audio log\n");
	n += sprintf(buf + n, "debug = 4, print video+edid+audio log\n");
	n += sprintf(buf + n, "debug = 5, print cec log\n");
	n += sprintf(buf + n, "debug = 6, print hdcp log\n");
	n += sprintf(buf + n, "debug = 7, print event log\n");
	n += sprintf(buf + n, "debug = 8, print pack log\n");
	n += sprintf(buf + n, "debug = 9, print port log\n");
	n += sprintf(buf + n, "debug = 10, print all of the logs above\n");
	n += sprintf(buf + n, "debug = 11, print all of the logs above and trace log\n");

	return n;
}

static ssize_t hdmirx_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "11", 2) == 0)
		gHdmirx_log_level = 11;
	else if (strncmp(buf, "10", 2) == 0)
		gHdmirx_log_level = 10;
	else if (strncmp(buf, "9", 1) == 0)
		gHdmirx_log_level = 9;
	else if (strncmp(buf, "8", 1) == 0)
		gHdmirx_log_level = 8;
	else if (strncmp(buf, "7", 1) == 0)
		gHdmirx_log_level = 7;
	else if (strncmp(buf, "6", 1) == 0)
		gHdmirx_log_level = 6;
	else if (strncmp(buf, "5", 1) == 0)
		gHdmirx_log_level = 5;
	else if (strncmp(buf, "4", 1) == 0)
		gHdmirx_log_level = 4;
	else if (strncmp(buf, "3", 1) == 0)
		gHdmirx_log_level = 3;
	else if (strncmp(buf, "2", 1) == 0)
		gHdmirx_log_level = 2;
	else if (strncmp(buf, "1", 1) == 0)
		gHdmirx_log_level = 1;
	else if (strncmp(buf, "0", 1) == 0)
		gHdmirx_log_level = 0;
	else
		hdmirx_err("%s: invalid input: %s\n", __func__, buf);

	hdmirx_inf("set hdmirx debug level: %d\n", gHdmirx_log_level);
	return count;
}

static ssize_t hdmirx_edid_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return aw_hdmirx_drv_edid_parse(buf);
}

ssize_t hdmirx_edid_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

static ssize_t hdmirx_signal_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return aw_hdmirx_drv_dump_SignalInfo(buf);
}

ssize_t hdmirx_signal_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

static ssize_t hdmirx_status_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return aw_hdmirx_drv_dump_status(buf);
}

ssize_t hdmirx_status_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

static ssize_t hdmirx_hdcp14_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return aw_hdmirx_drv_dump_hdcp14(buf);
	return 0;
}

ssize_t hdmirx_hdcp14_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
static ssize_t hdmirx_cec_msg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n, "[Usage]\n");
	n += sprintf(buf + n, "    echo tx/rx msg > cec_msg\n");
	n += sprintf(buf + n, "[Example]\n");
	n += sprintf(buf + n, "    send message: echo tx 0x0f 0x84 0x00 0x00 0x00 > cec_msg\n");
	n += sprintf(buf + n, "    recv message: echo rx 0x40 0x36 > cec_msg\n");
	return n;
}

ssize_t hdmirx_cec_msg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	int len = 0;
	int i = 0;
	int tx_rx;
	unsigned long msg[16] = {0};
	unsigned char pmsg[16] = {0};
	char *token;

	if (strstr(buf, "tx") != NULL) {
		tx_rx = 0;
		printk("send msg\n");
	} else if (strstr(buf, "rx") != NULL) {
		tx_rx = 1;
		printk("rec msg\n");
	} else {
		printk("error\n");
		return -EINVAL;
	}

	while ((token = strsep((char **)&buf, " ")) != NULL && len < 16) {
		if (strncmp(token, "0x", 2) == 0 && strlen(token) > 2) {
			if (kstrtoul(token + 2, 16, &msg[len]) != 0) {
				return -EINVAL;
			}
			len++;
		}
	}

	for (i = 0; i < len; ++i) {
		pmsg[i] = (unsigned char)msg[i];
		printk("pmsg[%d] = 0x%x\n", i, pmsg[i]);
	}

	aw_hdmirx_cec_msg_debug(tx_rx, pmsg, len);

	return count;
}

static ssize_t hdmirx_cec_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += aw_hdmirx_cec_dump_info(buf + n);

	return n;
}
#endif

static DEVICE_ATTR(reg_dump, 0664,
		hdmirx_reg_dump_show, hdmirx_reg_dump_store);

static DEVICE_ATTR(reg_read, 0664,
		hdmirx_reg_read_show, hdmirx_reg_read_store);

static DEVICE_ATTR(reg_write, 0664,
		hdmirx_reg_write_show, hdmirx_reg_write_store);

static DEVICE_ATTR(debug, 0664,
		hdmirx_debug_show, hdmirx_debug_store);

static DEVICE_ATTR(edid_dump, 0664,
		hdmirx_edid_dump_show, hdmirx_edid_dump_store);

static DEVICE_ATTR(signal_dump, 0664,
		hdmirx_signal_dump_show, hdmirx_signal_dump_store);

static DEVICE_ATTR(status_dump, 0664,
		hdmirx_status_dump_show, hdmirx_status_dump_store);

static DEVICE_ATTR(hdcp14_dump, 0664,
		hdmirx_hdcp14_dump_show, hdmirx_hdcp14_dump_store);

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
static DEVICE_ATTR(cec_dump, 0444,
		hdmirx_cec_dump_show, NULL);

static DEVICE_ATTR(cec_msg, 0664,
		hdmirx_cec_msg_show, hdmirx_cec_msg_store);
#endif

static struct attribute *hdmirx_attributes[] = {
	/* reg */
	&dev_attr_reg_dump.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_debug.attr,
	&dev_attr_edid_dump.attr,
	&dev_attr_signal_dump.attr,
	&dev_attr_status_dump.attr,
	&dev_attr_hdcp14_dump.attr,
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	&dev_attr_cec_dump.attr,
	&dev_attr_cec_msg.attr,
#endif
	NULL
};

static struct attribute_group hdmirx_attribute_group = {
	.name = "attr",
	.attrs = hdmirx_attributes
};

static int _aw_hdmirx_dev_open(struct inode *inode, struct file *filp)
{
	struct file_ops *fops;

	fops = kmalloc(sizeof(struct file_ops), GFP_KERNEL | __GFP_ZERO);
	if (!fops) {
		pr_info("%s: memory allocated for hdmirx fops failed!\n", __func__);
		return -EINVAL;
	}

	filp->private_data = fops;

	return 0;
}

static int _aw_hdmirx_dev_release(struct inode *inode, struct file *filp)
{
	struct file_ops *fops = (struct file_ops *)filp->private_data;

	kfree(fops);
	return 0;
}

static ssize_t _aw_hdmirx_dev_read(struct file *file, char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static ssize_t _aw_hdmirx_dev_write(struct file *file, const char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static int _aw_hdmirx_dev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static long _aw_hdmi_dev_ioctl_inner(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct file_ops *fops = (struct file_ops *)filp->private_data;

	unsigned long *p_arg = (unsigned long *)arg;
	TSourceId source_id;
	u8 edid_table[256];
	u8 port_id, type;
	u32 hdmi_num, b5vstate;

	if (!fops) {
		hdmirx_err("%s: param fops is null!!!\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case AW_IOCTL_HDMIRX_NULL:
		fops->ioctl_cmd = AW_IOCTL_HDMIRX_NULL;
		break;
	case AW_IOCTL_HDMIRX_SET_SOURCE:
		source_id = (TSourceId)p_arg[0];
		if (aw_hdmirx_drv_isvalidsource(source_id)) {
			return -EINVAL;
		}

		aw_hdmirx_drv_setsource(source_id);
		break;

	case AW_IOCTL_HDMIRX_SET_HPDTIMEINTERVAL:
		//the time ms of pull down HPD when select source
		aw_hdmirx_drv_sethpdtimeinterval(p_arg[0]);
		break;

	case AW_IOCTL_HDMIRX_SET_PORTMAP:
		source_id = (TSourceId)p_arg[0];
		if (aw_hdmirx_drv_isvalidsource(source_id)) {
			return -EINVAL;
		}

		aw_hdmirx_drv_setportmap(source_id, p_arg[1]);
		break;

	case AW_IOCTL_HDMIRX_SET_UPDATEEDID:
		if (copy_from_user((void *)edid_table, (void __user *)p_arg[0],
						256)) {
			hdmirx_err("%s: copy from user fail when hdmi compat ioctl!!!\n", __func__);
			return -EFAULT;
		}
		aw_hdmirx_drv_setupdateedid(edid_table, p_arg[1]);
		break;

	case AW_IOCTL_HDMIRX_SET_ARCENABLE:
		aw_hdmirx_drv_setarcenable(p_arg[0]);
		break;

	case AW_IOCTL_HDMIRX_SET_ARCTXPATH:
		aw_hdmirx_drv_setarctxpath((ARCTXPath_e)p_arg[0]);
		break;

	case AW_IOCTL_HDMIRX_SET_EDIDVERSION:
		aw_hdmirx_drv_setedidversion(p_arg[0]);
		break;

	case AW_IOCTL_HDMIRX_SET_PULLHOTPLUG:
		port_id = (u8)p_arg[0];
		type = (u8)p_arg[1];
		aw_hdmirx_drv_setpullhotplug(port_id, type);
		break;

	case AW_IOCTL_HDMIRX_GET_HDMINUM:
		if (g_hdmirx_drv)
			hdmi_num = g_hdmirx_drv->aw_dts.hdmi_num;
		else
			return -EINVAL;
		if (copy_to_user((void __user *)p_arg[0], (void *)&hdmi_num, sizeof(u32))) {
			hdmirx_err("%s: copy to user fail when get hdminum!\n", __func__);
			return -EINVAL;
		}
		break;

	case AW_IOCTL_HDMIRX_GET_5V:
		aw_hdmirx_drv_get5vstate(&b5vstate);
		if (copy_to_user((void __user *)p_arg[0], (void *)&b5vstate, sizeof(u32))) {
			hdmirx_err("%s: copy to user fail when get hdmi 5v!\n", __func__);
			return -EINVAL;
		}
		break;

	default:
		hdmirx_err("%s: cmd 0x%x invalid\n", __func__, cmd);
		return -EINVAL;
	}
	return 0;
}

static long _aw_hdmirx_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	//32bit
	unsigned long arg64[3] = {0};

	if (_IOC_TYPE(cmd) != HDMIRX_IOC_MAGIC) {
		hdmirx_err("%s: MAGIC is not match !\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user((void *)arg64, (void __user *)arg,
						3 * sizeof(unsigned long))) {
		hdmirx_err("%s: copy from user fail when hdmi ioctl!!!\n", __func__);
		return -EFAULT;
	}

	return _aw_hdmi_dev_ioctl_inner(filp, cmd, (unsigned long)arg64);
}

#if IS_ENABLED(CONFIG_COMPAT)
static long _aw_hdmirx_dev_compat_ioctl(struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	//32bit and 64bit
	compat_uptr_t arg32[3] = {0};
	unsigned long arg64[3] = {0};

	if (_IOC_TYPE(cmd) != HDMIRX_IOC_MAGIC) {
		hdmirx_err("%s: MAGIC is not match !\n", __func__);
		return -EFAULT;
	}

	if (!arg)
		return _aw_hdmi_dev_ioctl_inner(filp, cmd, 0);

	if (copy_from_user((void *)arg32, (void __user *)arg,
						3 * sizeof(compat_uptr_t))) {
		hdmirx_err("%s: copy from user fail when hdmi compat ioctl!!!\n", __func__);
		return -EFAULT;
	}

	arg64[0] = (unsigned long)arg32[0];
	arg64[1] = (unsigned long)arg32[1];
	arg64[2] = (unsigned long)arg32[2];
	return _aw_hdmi_dev_ioctl_inner(filp, cmd, (unsigned long)arg64);
}
#endif

static const struct file_operations aw_hdmirx_fops = {
	.owner		    = THIS_MODULE,
	.open		    = _aw_hdmirx_dev_open,
	.release	    = _aw_hdmirx_dev_release,
	.write		    = _aw_hdmirx_dev_write,
	.read		    = _aw_hdmirx_dev_read,
	.mmap		    = _aw_hdmirx_dev_mmap,
	.unlocked_ioctl	= _aw_hdmirx_dev_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl	= _aw_hdmirx_dev_compat_ioctl,
#endif
};

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_LOG_BUFFER)
static int _aw_hdmirx_seq_log_show(struct seq_file *m, void *data)
{
	char *addr = aw_hdmirx_log_get_address();
	unsigned int start = aw_hdmirx_log_get_start_index();
	unsigned int max_size = aw_hdmirx_log_get_max_size();
	unsigned int used_size = aw_hdmirx_log_get_used_size();

	if (used_size > 0) {
		if (used_size >= max_size) {
			seq_write(m, addr + start, max_size - start);
			seq_write(m, addr, start);
		} else {
			seq_write(m, addr, used_size);
		}
	}

	aw_hdmirx_log_put_address();

	return 0;
}

static int _aw_hdmirx_seq_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, _aw_hdmirx_seq_log_show, NULL);
}

static const struct file_operations aw_hdmirx_log_fops = {
	.open = _aw_hdmirx_seq_log_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int __init aw_hdmirx_module_init(void)
{
	int ret = 0;

	/* Create and add a character device */
	alloc_chrdev_region(&aw_dev_hdmirx.hdmirx_devid, 0, 1, "hdmirx");/* corely for device number */
	aw_dev_hdmirx.hdmirx_cdev = cdev_alloc();
	if (!aw_dev_hdmirx.hdmirx_cdev) {
		hdmirx_err("%s: device register cdev_alloc failed!!!\n", __func__);
		return -1;
	}

	cdev_init(aw_dev_hdmirx.hdmirx_cdev, &aw_hdmirx_fops);
	aw_dev_hdmirx.hdmirx_cdev->owner = THIS_MODULE;

	/* Create a path: /proc/device/hdmirx */
	if (cdev_add(aw_dev_hdmirx.hdmirx_cdev, aw_dev_hdmirx.hdmirx_devid, 1)) {
		hdmirx_err("%s: device register cdev_add failed!!!\n", __func__);
		return -1;
	}

	/* Create a path: sys/class/hdmirx */
	aw_dev_hdmirx.hdmirx_class = class_create("hdmirx");
	if (IS_ERR(aw_dev_hdmirx.hdmirx_class)) {
		hdmirx_err("%s: device register class_create failed!!!\n", __func__);
		return -1;
	}

	/* Create a path "sys/class/hdmi/hdmirx" */
	aw_dev_hdmirx.hdmirx_device = device_create(aw_dev_hdmirx.hdmirx_class, NULL, aw_dev_hdmirx.hdmirx_devid, NULL, "hdmirx");
	if (!aw_dev_hdmirx.hdmirx_device) {
		hdmirx_err("%s: device register device_create failed!!!\n", __func__);
		return -1;
	}

	/* Create a path: sys/class/hdmirx/hdmirx/attr */
	ret = sysfs_create_group(&aw_dev_hdmirx.hdmirx_device->kobj, &hdmirx_attribute_group);
	if (ret) {
		hdmirx_err("%s: device register sysfs_create_group failed!!!\n", __func__);
		return -1;
	}

	ret = platform_driver_register(&dw_hdmirx_pdrv);
	if (ret != 0) {
		hdmirx_err("%s: device register platform_driver_register failed!!!\n", __func__);
		return -1;
	}

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
	ret = platform_driver_register(&aw_hdmirx_cec_driver);
	if (ret != 0) {
		hdmirx_err("%s: cec platform_driver_register failed!!!\n", __func__);
		return -1;
	}
#endif

	gHdmirx_debugfs_dir = debugfs_create_dir("hdmirx", NULL);
	if (!gHdmirx_debugfs_dir) {
		hdmirx_wrn("debugfs create dir failed!\n");
	} else {
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_LOG_BUFFER)
		debugfs_create_file("hdmi_log", 0440, gHdmirx_debugfs_dir, NULL, &aw_hdmirx_log_fops);
#endif
	}

	hdmirx_inf("hdmirx module init end.\n");
	return ret;
}

static void __exit aw_hdmirx_module_exit(void)
{
	debugfs_remove(gHdmirx_debugfs_dir);
	gHdmirx_debugfs_dir = NULL;

	if (g_hdmirx_drv) {
		//aw_hdmirx_drv_remove(g_hdmirx_drv->pdev);
		platform_driver_unregister(&dw_hdmirx_pdrv);
		aw_hdmirx_core_exit(g_hdmirx_drv->hdmirx_core);
		hdmirx_inf("000");
		kfree(g_hdmirx_drv);
	}
	hdmirx_inf("111");
#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_CEC)
		platform_driver_unregister(&aw_hdmirx_cec_driver);
#endif
	hdmirx_inf("222");

	//platform_driver_unregister(&dw_hdmirx_pdrv);

	sysfs_remove_group(&aw_dev_hdmirx.hdmirx_device->kobj, &hdmirx_attribute_group);

	device_destroy(aw_dev_hdmirx.hdmirx_class, aw_dev_hdmirx.hdmirx_devid);

	class_destroy(aw_dev_hdmirx.hdmirx_class);

	cdev_del(aw_dev_hdmirx.hdmirx_cdev);

#if IS_ENABLED(CONFIG_AW_VIDEO_SUNXI_HDMIRX_LOG_BUFFER)
	aw_hdmirx_log_exit();
#endif

	hdmirx_inf("hdmirx module exit end.\n");
}

late_initcall(aw_hdmirx_module_init);
module_exit(aw_hdmirx_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("huangwenhao");
MODULE_DESCRIPTION("aw hdmi rx module driver");
MODULE_VERSION("1.0");
