// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Oplus. All rights reserved.
 */

#include <linux/arm-smccc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <trace/hooks/sched.h>
#include <linux/dma-map-ops.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <trace/events/sched.h>

#define AMU_COUNTER_UPDATE_PERIOD_MS 20
#define ARM_SVC_AMU_READ_AARCH64 0xc000ff20
#define PARA_BUF_LEN		128
#define MAX_SCALE_FACTOR	5
#define LOW_UTIL_THRESHOLD	100
#define HEAVY_UTIL_THRESHOLD	600
#define MAX_TRIPS_NUM		6
#define PROC_SUNXI_AMU		"sunxi_amu"
#define AMU_CPU_MASK		0xC0  // SUN60IW2P1

struct amu_events {
	unsigned long cpu_cycles;
	unsigned long instructions;
	unsigned long mem_stall_cycles;
};

DEFINE_PER_CPU(struct amu_events *, amu_data);
DEFINE_PER_CPU(unsigned long, amu_cpu_util);
DEFINE_PER_CPU(unsigned long, amu_stall_util);
DEFINE_PER_CPU(unsigned long, amu_cpu_cycles);
DEFINE_PER_CPU(unsigned long, amu_cpu_instructions);
DEFINE_PER_CPU(unsigned long, amu_mem_stall);
DEFINE_PER_CPU(unsigned long, amu_last_update_time);

struct device *amu_dev;
bool sunxi_amu_enabled = true;
unsigned long stall_util_trips[MAX_TRIPS_NUM] = {200, 300, 400};
unsigned long util_uclamp_list[MAX_TRIPS_NUM] = {90, 85, 80};
unsigned long trips_num = 3;

inline bool is_not_amu_cpu(int cpu)
{
	return !((1 << cpu) & AMU_CPU_MASK);
}

static void sunxi_amu_update_tick_handler(void *data, struct rq *rq)
{
	struct arm_smccc_res res;
	unsigned long buf_phys;
	volatile struct amu_events *edata;
	int cpu;

	if (unlikely(!sunxi_amu_enabled))
		return;

	cpu = smp_processor_id();
	if (is_not_amu_cpu(cpu))
		return;

	edata = per_cpu(amu_data, cpu);

	if (!edata)
		return;

	buf_phys = virt_to_phys(edata);

	arm_smccc_smc(ARM_SVC_AMU_READ_AARCH64, buf_phys, buf_phys + sizeof(unsigned long),
		      buf_phys + 2 * sizeof(unsigned long), 0, 0, 0, 0, &res);

	dma_sync_single_for_cpu(amu_dev, buf_phys, sizeof(struct amu_events), DMA_FROM_DEVICE);

	per_cpu(amu_cpu_cycles, cpu) += edata->cpu_cycles;
	per_cpu(amu_cpu_instructions, cpu) += edata->instructions;
	per_cpu(amu_mem_stall, cpu) += edata->mem_stall_cycles;

}

int amu_update_counter(struct cpufreq_policy *policy, unsigned long freq, unsigned long cap)
{
	int cpu;
	unsigned int max_freq, max_cap;
	u64 now;
	unsigned long delta_ms;
	unsigned long avg_cpu_freq, avg_stall_freq;

	max_freq = freq; // todo, get max freq from 'struct policy'
	max_cap = cap;
	now = ktime_get();
	for_each_cpu(cpu, policy->cpus) {
		delta_ms = ktime_to_ms(ktime_sub(now, per_cpu(amu_last_update_time, cpu)));
		if (delta_ms < AMU_COUNTER_UPDATE_PERIOD_MS)
			return -1;
	}

	for_each_cpu(cpu, policy->cpus) {
		if (is_not_amu_cpu(cpu))
			return -1;
		delta_ms = ktime_to_ms(ktime_sub(now, per_cpu(amu_last_update_time, cpu)));
		avg_cpu_freq = per_cpu(amu_cpu_cycles, cpu) / delta_ms;// KHz
		avg_stall_freq = per_cpu(amu_mem_stall, cpu) / delta_ms; // KHz
		per_cpu(amu_cpu_util, cpu) = (avg_cpu_freq * max_cap) / max_freq ;
		per_cpu(amu_stall_util, cpu) = (avg_stall_freq * max_cap) / max_freq;

		per_cpu(amu_cpu_cycles, cpu) = 0;
		per_cpu(amu_cpu_instructions, cpu) = 0;
		per_cpu(amu_mem_stall, cpu) = 0;
		per_cpu(amu_last_update_time, cpu) = now;
	}
	return 0;
}

unsigned long all_heavy_cpu_stall_util(struct cpufreq_policy *policy)
{
	int j;
	int heavy_cpu_cnt = 0, cpu_cnt = 0;
	unsigned long cpu_util = 0, stall_util = 0;
	unsigned long min_cpu_util = 0, min_stall_util = 0;

	for_each_cpu(j, policy->cpus) {
		cpu_cnt++;

		cpu_util = per_cpu(amu_cpu_util, j);
		stall_util = per_cpu(amu_stall_util, j);
		min_cpu_util = cpu_util;
		min_stall_util = stall_util;

		if (cpu_util > HEAVY_UTIL_THRESHOLD) {
			heavy_cpu_cnt++;
			if (cpu_util < min_cpu_util) {
				min_cpu_util = cpu_util;
				min_stall_util = stall_util;
			}
		}
	}

	if (heavy_cpu_cnt == cpu_cnt)
		return min_stall_util;

	return 0;
}

unsigned long max_cpu_stall_util(struct cpufreq_policy *policy)
{
	int j;
	unsigned long cpu_util = 0, stall_util = 0;
	unsigned long max_cpu_util = 0, max_stall_util = 0;

	for_each_cpu(j, policy->cpus) {
		cpu_util = per_cpu(amu_cpu_util, j);
		stall_util = per_cpu(amu_stall_util, j);

		if (cpu_util > max_cpu_util) {
			max_cpu_util = cpu_util;
			max_stall_util = stall_util;
		}
	}

	return max_stall_util;
}

static void sunxi_map_util_freq(void *p, unsigned long util, unsigned long freq,
				unsigned long cap, unsigned long *next_freq,
				struct cpufreq_policy *policy, bool *need_freq_update)
{
	int ret;
	int cpu;
	int i;
	unsigned long min_map_util = (cap * 64) / 100; //0.8*0.8
	unsigned long scale_stall_util = 0, scale_util = 0;
	unsigned long ori_util, ori_next_freq, cur_freq;
	unsigned long max_cpu_util = 0, max_scale_util = 0;

	if (unlikely(!sunxi_amu_enabled))
		return;

	cpu = cpumask_first(policy->related_cpus);
	if (is_not_amu_cpu(cpu))
		return;

	/* util argument is smaller than 64%, skip amu freq*/
	if (util <= min_map_util)
		return;

	ret = amu_update_counter(policy, freq, cap);
	if (ret == -1)
		return;

	ori_util = util;
	cur_freq = policy->cur;

	//Case 1:
	/* Fix the map util is high but the cpu real util is low */
	for_each_cpu(i, policy->cpus)
		max_cpu_util = max(max_cpu_util, per_cpu(amu_cpu_util, i));

	if (max_cpu_util < LOW_UTIL_THRESHOLD) {
		max_scale_util = max_cpu_util * MAX_SCALE_FACTOR;
		if (util > max_scale_util)
			util = max(util / 2, max_scale_util);

		goto util_to_freq;
	}

	//Case 2:
	/* When all cpu is heavy load, use the min value of stall_util*/
	scale_stall_util = all_heavy_cpu_stall_util(policy);

	/* otherwise, use the max value of stall_util*/
	if (scale_stall_util == 0)
		scale_stall_util = max_cpu_stall_util(policy);

	/* limit map util with step-wise governor */
	for (i = 0; i < trips_num; i++) {
		if (scale_stall_util > stall_util_trips[i])
			scale_util = (util_uclamp_list[i] * cap) / 100;
	}

	if (scale_util == 0)
		return;

	util = min(scale_util, util);
	if (util == ori_util)
		return;

	/*
	 * TODO: When cpu mem stall is high, boost dram freq?
	*/

util_to_freq:
	ori_next_freq = freq * ori_util / cap;

	*next_freq = freq * util / cap;
	*need_freq_update = true;
}

int devno;
struct class *amu_class;
int sunxi_amu_create_device(void)
{
	int ret;

	ret = alloc_chrdev_region(&devno, 0, 1, "amu_dev");
	amu_class = class_create("amu_class");

	amu_dev = device_create(amu_class, NULL, devno, NULL, "amu_dev");
	return 0;
}

static ssize_t cpu_util_uclamp_write(struct file *file, const char __user *buff,
				     size_t len, loff_t *ppos)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	char temp[32];
	unsigned long val;
	int ret = 0;
	int i = 0, j = 0, k = 0;

	if (len > PARA_BUF_LEN - 1) {
		pr_err("len %ld is too long\n", len);
		return -EINVAL;
	}

	if (copy_from_user(&kbuf, buff, len))
		return -EFAULT;

	for (i = 0; i < len; i++) {
		if (kbuf[i] >= '0' && kbuf[i] <= '9') {
			temp[j++] = kbuf[i];
		} else if (kbuf[i] == ' ' || kbuf[i] == '\n') {
			if (j != 0) {
				temp[j] = '\0';
				ret = kstrtoul(temp, 0, &val);
				if (ret)
					return -EINVAL;
			}
			if (k % 2 == 0)
				stall_util_trips[k / 2] = val;
			else
				util_uclamp_list[k / 2] = val;
			k++;
			if (k >= MAX_TRIPS_NUM * 2)
				break;
			j = 0;
		}
	}

	if (k % 2 != 0) {
		pr_err("input parameters err\n");
		sunxi_amu_enabled = false;
		return -EINVAL;
	}

	trips_num = k / 2;

	return len;
}

static ssize_t cpu_util_uclamp_read(struct file *file, char __user *buffer,
				    size_t count, loff_t *off)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	int len = 0;
	int i;

	for (i = 0; i < trips_num; i++) {
		len += snprintf(kbuf + len, PARA_BUF_LEN, "%lu  ", stall_util_trips[i]);
		len += snprintf(kbuf + len, PARA_BUF_LEN, "%lu  ", util_uclamp_list[i]);
	}
	len += snprintf(kbuf + len, PARA_BUF_LEN, "\n");
	if (len == PARA_BUF_LEN)
		kbuf[len - 1] = '\0';

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static const struct proc_ops proc_cpu_util_uclamp_ops = {
	.proc_write          = cpu_util_uclamp_write,
	.proc_read		= cpu_util_uclamp_read,
};

static ssize_t enable_para_write(struct file *file, const char __user *buff,
				 size_t len, loff_t *ppos)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	char *str;
	long val;
	int ret = 0;

	if (len > PARA_BUF_LEN - 1) {
		pr_err("len %ld is too long\n", len);
		return -EINVAL;
	}

	if (copy_from_user(&kbuf, buff, len))
		return -EFAULT;
	kbuf[len] = '\0';

	str = strstrip(kbuf);
	if (!str) {
		pr_err("buff %s is invalid\n", kbuf);
		return -EINVAL;
	}

	ret = kstrtoul(str, 0, &val);
	if (ret)
		return -EINVAL;

	sunxi_amu_enabled = !!val;

	return len;
}

static ssize_t enable_para_read(struct file *file, char __user *buffer,
				size_t count, loff_t *off)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	int len;

	len = snprintf(kbuf, PARA_BUF_LEN, "%d\n", sunxi_amu_enabled);
	if (len == PARA_BUF_LEN)
		kbuf[len - 1] = '\0';

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static const struct proc_ops proc_enable_para_ops = {
	.proc_write          = enable_para_write,
	.proc_read		= enable_para_read,
};

static struct proc_dir_entry *para_entry, *para_entry1;
static int __init sunxi_amu_create_proc(void)
{
	struct proc_dir_entry *root_dir_entry = proc_mkdir(PROC_SUNXI_AMU, NULL);

	para_entry = proc_create("enable", 0600, root_dir_entry, &proc_enable_para_ops);

	if (para_entry)
		pr_info("Register sunxi amu enable entry.\n");

	para_entry1 = proc_create("cpu_util_uclamp", 0600, root_dir_entry,
				  &proc_cpu_util_uclamp_ops);

	if (para_entry1)
		pr_info("Register sunxi swappiness passed.\n");

	return 0;
}

void sunxi_amu_register_vh(void)
{
	register_trace_android_vh_scheduler_tick(sunxi_amu_update_tick_handler, NULL);
	register_trace_android_vh_map_util_freq(sunxi_map_util_freq, NULL);
}

void sunxi_amu_unregister_vh(void)
{
	unregister_trace_android_vh_scheduler_tick(sunxi_amu_update_tick_handler, NULL);
	unregister_trace_android_vh_map_util_freq(sunxi_map_util_freq, NULL);
}

static int __init amu_init(void)
{
	int i;

	for (i = 0; i < 8; i++)
		per_cpu(amu_data, i) = kmalloc(sizeof(struct amu_events) * 3, GFP_KERNEL);

	sunxi_amu_create_device();
	if (IS_ENABLED(CONFIG_ARM64_AMU_EXTN))
		sunxi_amu_register_vh();
	sunxi_amu_create_proc();

	return 0;
}

static void __exit amu_exit(void)
{
	remove_proc_subtree(PROC_SUNXI_AMU, NULL);

	sunxi_amu_unregister_vh();

	device_destroy(amu_class, devno);
	class_destroy(amu_class);
	unregister_chrdev_region(devno, 1);
}

module_init(amu_init);
module_exit(amu_exit);

MODULE_DESCRIPTION("aw amu");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("heryli<henryli@allwinnertech.com>");
