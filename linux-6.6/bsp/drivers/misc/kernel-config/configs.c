// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * copied from kernel/configs.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#ifdef CONFIG_IKCONFIG_PROC

extern unsigned char aw_config_data[];
extern unsigned int  aw_config_data_len;

static char *kernel_config_data;
static char *kernel_config_data_end;

static ssize_t
ikconfig_read_current(struct file *file, char __user *buf,
			size_t len, loff_t *offset)
{
	return simple_read_from_buffer(buf, len, offset,
			kernel_config_data, kernel_config_data_end - kernel_config_data);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0))
static const struct file_operations config_gz_proc_ops = {
	.read   = ikconfig_read_current,
	.llseek = default_llseek,
};

#else

static const struct proc_ops config_gz_proc_ops = {
	.proc_read  = ikconfig_read_current,
	.proc_lseek = default_llseek,
};
#endif

static int __init ikconfig_init(void)
{
	struct proc_dir_entry *entry;

	kernel_config_data = aw_config_data;
	kernel_config_data_end = aw_config_data + aw_config_data_len;

	/* create the current config file */
	entry = proc_create("aw-config.gz", S_IFREG | S_IRUGO, NULL,
			&config_gz_proc_ops);
	if (!entry)
		return -ENOMEM;

	proc_set_size(entry, kernel_config_data_end - kernel_config_data);

	return 0;
}

static void __exit ikconfig_cleanup(void)
{
	remove_proc_entry("aw-config.gz", NULL);
}

module_init(ikconfig_init);
module_exit(ikconfig_cleanup);

#endif /* CONFIG_IKCONFIG_PROC */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhao");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Echo the kernel .config file really used to build the kernel");
