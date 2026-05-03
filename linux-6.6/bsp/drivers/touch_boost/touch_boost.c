// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt) "touch-boost: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/time.h>
#include <linux/sysfs.h>
#include <linux/pm_qos.h>
#include <linux/module.h>
#include <linux/version.h>

#define ATB_TRACE_DEBUG 0

struct cpu_sync {
	int		cpu;
	unsigned int	input_boost_min;
	unsigned int	input_boost_freq;
};

#define MAX_CLUSTERS 2
#define DEFAULT_DURATION_MS 100
static int touch_boost_duration_ms = DEFAULT_DURATION_MS;
static int touch_boost_trace_debug;
static int is_init;

struct aw_touch_boost {
	struct kobject kobj;
	unsigned int touch_boost_freq;
} aw_touch_boost[MAX_CLUSTERS] = {
	/* silver */
	{
		.touch_boost_freq     = 1150000,
	},
	/* gold */
	{
		.touch_boost_freq     = 0,
	}
};

struct input_handle_info {
	struct input_handle handle;
	struct list_head list;
};

struct list_head ihi_list;

struct aw_touch_boost_attr {
	struct attribute attr;
	ssize_t (*show)(struct aw_touch_boost *atb, char *buf);
	ssize_t (*store)(struct aw_touch_boost *atb, const char *buf, size_t count);
};

static noinline int tracing_mark_write(const char *buf)
{
#if ATB_TRACE_DEBUG
	trace_printk(buf);
#endif
	return 0;
}

static void cpu_freq_systrace_c(int cpu, int freq)
{
	if (touch_boost_trace_debug) {
		char buf[256];

		snprintf(buf, sizeof(buf), "C|9999|touch_boost_min_cpu%d|%d\n", cpu, freq);
		tracing_mark_write(buf);
	}
}

static ssize_t store_touch_boost_ms(struct aw_touch_boost *atb, const char *buf, size_t count)
{
	unsigned int val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	touch_boost_duration_ms = val;

	return count;
}

static ssize_t show_touch_boost_ms(struct aw_touch_boost *atb, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", touch_boost_duration_ms);
}

static ssize_t store_touch_boost_freq(struct aw_touch_boost *atb, const char *buf, size_t count)
{
	unsigned int val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	atb->touch_boost_freq = val;

	return count;
}

static ssize_t show_touch_boost_freq(struct aw_touch_boost *atb, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", atb->touch_boost_freq);
}

static ssize_t store_touch_boost_debug(struct aw_touch_boost *atb, const char *buf, size_t count)
{
	unsigned int val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	touch_boost_trace_debug = !!val;

	return count;
}

static ssize_t show_touch_boost_debug(struct aw_touch_boost *atb, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", touch_boost_trace_debug);
}

#define aw_touch_boost_attr_ro(_name)		\
static struct aw_touch_boost_attr _name =	\
__ATTR(_name, 0444, show_##_name, NULL)

#define aw_touch_boost_attr_rw(_name)			\
static struct aw_touch_boost_attr _name =		\
__ATTR(_name, 0644, show_##_name, store_##_name)

aw_touch_boost_attr_rw(touch_boost_freq);
aw_touch_boost_attr_rw(touch_boost_ms);
aw_touch_boost_attr_rw(touch_boost_debug);

static struct attribute *touch_boost_default_attrs[] = {
	&touch_boost_freq.attr,
	&touch_boost_ms.attr,
	&touch_boost_debug.attr,
	NULL
};
ATTRIBUTE_GROUPS(touch_boost_default);

#define to_aw_touch_boost(k) \
		container_of(k, struct aw_touch_boost, kobj)
#define to_attr(a) container_of(a, struct aw_touch_boost_attr, attr)

static ssize_t show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct aw_touch_boost *atb = to_aw_touch_boost(kobj);
	struct aw_touch_boost_attr *cattr = to_attr(attr);
	ssize_t ret = -EIO;

	if (cattr->show)
		ret = cattr->show(atb, buf);

	return ret;
}

static ssize_t store(struct kobject *kobj, struct attribute *attr,
		     const char *buf, size_t count)
{
	struct aw_touch_boost *atb = to_aw_touch_boost(kobj);
	struct aw_touch_boost_attr *cattr = to_attr(attr);
	ssize_t ret = -EIO;

	if (cattr->store)
		ret = cattr->store(atb, buf, count);

	return ret;
}

static const struct sysfs_ops sysfs_ops = {
	.show	= show,
	.store	= store,
};

static struct kobj_type ktype_aw_touch_boost = {
	.sysfs_ops	= &sysfs_ops,
//	.default_attrs	= default_attrs,
	.default_groups = touch_boost_default_groups,
};

static DEFINE_PER_CPU(struct cpu_sync, sync_info);
static struct workqueue_struct *input_boost_wq;

static struct work_struct input_boost_work;
static struct delayed_work input_boost_rem;
static u64 last_input_time;
#define MIN_INPUT_INTERVAL (150 * USEC_PER_MSEC)

static DEFINE_PER_CPU(struct freq_qos_request, qos_req);

static void boost_adjust_notify(struct cpufreq_policy *policy)
{
	unsigned int cpu = cpumask_first(policy->related_cpus);
	struct cpu_sync *s = &per_cpu(sync_info, cpu);
	unsigned int ib_min = s->input_boost_min;
	struct freq_qos_request *req = &per_cpu(qos_req, cpu);
	int ret;
	int orig_min = policy->min;

	cpu_freq_systrace_c(cpu, ib_min);

	ret = freq_qos_update_request(req, ib_min);
	if (ret < 0)
		pr_err("Failed to update freq constraint in boost_adjust: %d\n", ib_min);

	pr_debug("CPU%u policy min before boost: %u kHz, boost min: %u kHz, after boost: %u kHz\n",
		 cpu, orig_min, ib_min, policy->min);
}

static void update_policy_online(void)
{
	unsigned int i;
	struct cpufreq_policy *policy;
	struct cpumask online_cpus;

	/* Re-evaluate policy to trigger adjust notifier for online CPUs */
	cpus_read_lock();
	online_cpus = *cpu_online_mask;
	for_each_cpu(i, &online_cpus) {
		policy = cpufreq_cpu_get(i);
		if (!policy) {
			pr_err("%s: cpufreq policy not found for cpu%d\n", __func__, i);
			return;
		}

		cpumask_andnot(&online_cpus, &online_cpus, policy->related_cpus);
		boost_adjust_notify(policy);
	}
	cpus_read_unlock();
}

static void do_input_boost_rem(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *i_sync_info;

	/* Reset the input_boost_min for all CPUs in the system */
	pr_debug("Resetting input boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		i_sync_info = &per_cpu(sync_info, i);
		i_sync_info->input_boost_min = 0;
	}

	/* Update policies for all online CPUs */
	update_policy_online();
}

static void do_input_boost(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *i_sync_info;
	unsigned int first_cpu;
	int cluster_id;

	cancel_delayed_work_sync(&input_boost_rem);

	/* Set the input_boost_min for all CPUs in the system */
	pr_debug("Setting input boost min for all CPUs\n");
	for (i = 0; i < 8; i++) {
		struct cpufreq_policy *policy;
		struct aw_touch_boost *atb;

		policy = cpufreq_cpu_get(i);
		if (!policy) {
			pr_info("cpu %d, policy is null\n", i);
			continue;
		}
		first_cpu = cpumask_first(policy->related_cpus);
		cluster_id = topology_physical_package_id(first_cpu);
		cpufreq_cpu_put(policy);
		if (cluster_id >= MAX_CLUSTERS)
			continue;
		atb = &aw_touch_boost[cluster_id];

		i_sync_info = &per_cpu(sync_info, i);
		i_sync_info->input_boost_min = atb->touch_boost_freq;
	}

	/* Update policies for all online CPUs */
	update_policy_online();

	queue_delayed_work(input_boost_wq, &input_boost_rem,
			   msecs_to_jiffies(touch_boost_duration_ms));
}

static void inputboost_input_event(struct input_handle *handle,
				   unsigned int type, unsigned int code, int value)
{
	u64 now;
	int cpu;
	int enabled = 0;
	unsigned int first_cpu;
	int cluster_id;

	if (type != EV_ABS && type != EV_KEY)
		return;

	now = ktime_to_us(ktime_get());
	if (now - last_input_time < MIN_INPUT_INTERVAL)
		return;

	for_each_possible_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct aw_touch_boost *atb;

		policy = cpufreq_cpu_get(cpu);
		if (!policy) {
			pr_info("cpu %d, policy is null\n", cpu);
			continue;
		}
		first_cpu = cpumask_first(policy->related_cpus);
		cluster_id = topology_physical_package_id(first_cpu);
		cpufreq_cpu_put(policy);
		if (cluster_id >= MAX_CLUSTERS)
			continue;
		atb = &aw_touch_boost[cluster_id];

		if (atb->touch_boost_freq > 0) {
			enabled = 1;
			break;
		}
	}
	if (!enabled)
		return;

	if (work_pending(&input_boost_work))
		return;

	queue_work(input_boost_wq, &input_boost_work);
	last_input_time = ktime_to_us(ktime_get());
}

static int inputboost_input_connect(struct input_handler *handler,
				    struct input_dev *dev, const struct input_device_id *id)
{
	int error;
	struct input_handle *handle;
	struct input_handle_info *handle_info;

	handle_info = kzalloc(sizeof(*handle_info), GFP_KERNEL);
	handle = &handle_info->handle;
	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";
	INIT_LIST_HEAD(&handle_info->list);
	list_add_tail(&handle_info->list, &ihi_list);

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void inputboost_input_disconnect(struct input_handle *handle)
{
	struct input_handle_info *handle_info = container_of(handle,
							     struct input_handle_info, handle);

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
	list_del(&handle_info->list);
}

static const struct input_device_id inputboost_ids[] = {
	/* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
			BIT_MASK(ABS_MT_POSITION_Y)
		},
	},
	/* touchpad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			BIT_MASK(ABS_X) | BIT_MASK(ABS_Y)
		},
	},
	/* Keypad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ },
};

static struct input_handler inputboost_input_handler = {
	.event		= inputboost_input_event,
	.connect	= inputboost_input_connect,
	.disconnect	= inputboost_input_disconnect,
	.name		= "touch-boost",
	.id_table	= inputboost_ids,
};

struct kobject *input_boost_kobj;

static int cluster_init(int first_cpu, struct device *dev)
{
	int cluster_id;
	struct aw_touch_boost *atb;
	struct cpufreq_policy *policy;

	policy = cpufreq_cpu_get(first_cpu);
	if (!policy)
		return -EINVAL;

	cluster_id = topology_cluster_id(first_cpu);
	if (cluster_id >= MAX_CLUSTERS) {
		pr_err("Unsupported number of clusters(%d). Only %u supported\n",
		       cluster_id, MAX_CLUSTERS);
		cpufreq_cpu_put(policy);
		return -EINVAL;
	}
	pr_info("cluster idx = %d, cpumask = 0x%x\n",
		cluster_id, (int)cpumask_bits(policy->related_cpus)[0]);

	atb = &aw_touch_boost[cluster_id];

	cpufreq_cpu_put(policy);
	memset(&atb->kobj, 0, sizeof(struct kobject));
	kobject_init(&atb->kobj, &ktype_aw_touch_boost);
	return kobject_add(&atb->kobj, &dev->kobj, "touch_boost");
};

static int __init touch_boost_init(void)
{
	int cpu, ret;
	struct cpu_sync *s;
	struct cpufreq_policy *policy, *last_policy;
	struct freq_qos_request *req;
	unsigned int first_cpu;
	struct device *cpu_dev;

	if (is_init)
		return 0;

	is_init = 1;

	input_boost_wq = alloc_workqueue("inputboost_wq", WQ_HIGHPRI, 0);
	if (!input_boost_wq)
		return -EFAULT;

	INIT_WORK(&input_boost_work, do_input_boost);
	INIT_DELAYED_WORK(&input_boost_rem, do_input_boost_rem);
	INIT_LIST_HEAD(&ihi_list);

	for_each_possible_cpu(cpu) {
		s = &per_cpu(sync_info, cpu);
		s->cpu = cpu;
		req = &per_cpu(qos_req, cpu);
		policy = cpufreq_cpu_get(cpu);
		if (!policy) {
			pr_err("%s: cpufreq policy not found for cpu%d\n", __func__, cpu);
			return -ESRCH;
		}
		if (policy == last_policy)
			continue;

		first_cpu = cpumask_first(policy->related_cpus);
		cpu_dev = get_cpu_device(first_cpu);
		if (!cpu_dev)
			return -ENODEV;

		cluster_init(first_cpu, cpu_dev);

		last_policy = policy;
		ret = freq_qos_add_request(&policy->constraints, req, FREQ_QOS_MIN, policy->min);
		if (ret < 0) {
			pr_err("%s: Failed to add freq constraint (%d)\n", __func__, ret);
			return ret;
		}
	}

	ret = input_register_handler(&inputboost_input_handler);
	return 0;
}

static void __exit touch_boost_exit(void)
{
	struct aw_touch_boost *atb;
	struct input_handle_info *handle_info;
	int i;

	for (i = 0; i < MAX_CLUSTERS; i++) {
		atb = &aw_touch_boost[i];
		kobject_put(&atb->kobj);
	}

	list_for_each_entry(handle_info, &ihi_list, list) {
		input_close_device(&handle_info->handle);
		input_unregister_handle(&handle_info->handle);
		kfree(handle_info);
	}
}

module_init(touch_boost_init);
module_exit(touch_boost_exit);

MODULE_DESCRIPTION("aw touch boost module");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("heryli<henryli@allwinnertech.com>");
