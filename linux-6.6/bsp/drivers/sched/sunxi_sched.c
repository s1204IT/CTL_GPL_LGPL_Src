// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner trace stack
 *
 * Copyright (C) 2022 Allwinner.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/seq_file.h>
#include <linux/energy_model.h>
#include <trace/events/sched.h>
#include <trace/events/task.h>
#include <trace/hooks/sched.h>
#include <linux/sched/cputime.h>
#include <linux/version.h>
#include "../../../kernel/sched/sched.h"

#define BIG_TASK_ROTATION_FREQ_MIN 1200000
struct task_rotate_work {
	struct work_struct w;
	struct task_struct *src_task;
	struct task_struct *dst_task;
	int src_cpu;
	int dst_cpu;
};

static int sunxi_sched_enable;
static DEFINE_RAW_SPINLOCK(migration_lock);
DEFINE_PER_CPU(struct task_rotate_work, task_rotate_works);

int idle_cpu(int cpu)
{
	struct rq *rq = cpu_rq(cpu);

	if (rq->curr != rq->idle)
		return 0;

	if (rq->nr_running)
		return 0;

#if IS_ENABLED(CONFIG_SMP)
	if (rq->ttwu_pending)
		return 0;
#endif

	return 1;
}

static void task_rotate_work_func(struct work_struct *work)
{
	struct task_rotate_work *wr = container_of(work,
				struct task_rotate_work, w);

	int ret = -1;
	struct rq *src_rq, *dst_rq;

	ret = migrate_swap(wr->src_task, wr->dst_task,
			   task_cpu(wr->dst_task), task_cpu(wr->src_task));

	put_task_struct(wr->src_task);
	put_task_struct(wr->dst_task);

	src_rq = cpu_rq(wr->src_cpu);
	dst_rq = cpu_rq(wr->dst_cpu);

	local_irq_disable();
	double_rq_lock(src_rq, dst_rq);
	src_rq->active_balance = 0;
	dst_rq->active_balance = 0;
	double_rq_unlock(src_rq, dst_rq);
	local_irq_enable();
}

void task_rotate_work_init(void)
{
	int i;

	for_each_possible_cpu(i) {
		struct task_rotate_work *wr = &per_cpu(task_rotate_works, i);

		INIT_WORK(&wr->w, task_rotate_work_func);
	}
}

int select_idle_cpu_from_domains(struct task_struct *p,
				 struct perf_domain **prefer_pds, int len)
{
	int i = 0;
	struct perf_domain *pd;
	int cpu, best_cpu = -1;

	for (; i < len; i++) {
		pd = prefer_pds[i];
		for_each_cpu_and(cpu, perf_domain_span(pd), cpu_active_mask) {
			if (!cpumask_test_cpu(cpu, p->cpus_ptr))
				continue;
			if (idle_cpu(cpu)) {
				best_cpu = cpu;
				break;
			}
		}
		if (best_cpu != -1)
			break;
	}

	return best_cpu;
}

int select_bigger_idle_cpu(struct task_struct *p)
{
	struct root_domain *rd = cpu_rq(smp_processor_id())->rd;
	struct perf_domain *pd, *prefer_pds[NR_CPUS];
	int cpu = task_cpu(p), bigger_idle_cpu = -1;
	int i = 0;
	long max_capacity = capacity_orig_of(cpu);
	long capacity;

	rcu_read_lock();
	pd = rcu_dereference(rd->pd);

	for (; pd; pd = pd->next) {
		capacity = capacity_orig_of(cpumask_first(perf_domain_span(pd)));
		if (capacity > max_capacity &&
		    cpumask_intersects(p->cpus_ptr, perf_domain_span(pd))) {
			prefer_pds[i++] = pd;
		}
	}

	if (i != 0)
		bigger_idle_cpu = select_idle_cpu_from_domains(p, prefer_pds, i);

	rcu_read_unlock();
	return bigger_idle_cpu;
}

#define HEAVY_TASK_NUM 4
#define TASK_ROTATION_THRESHOLD_NS 6000000

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static inline unsigned long task_util(struct task_struct *p)
{
	return READ_ONCE(p->se.avg.util_avg);
}

static inline unsigned long _task_util_est(struct task_struct *p)
{
	return READ_ONCE(p->se.avg.util_est) & ~UTIL_AVG_UNCHANGED;
}
#else
static inline unsigned long task_util(struct task_struct *p)
{
	return READ_ONCE(p->se.avg.util_avg);
}

static inline unsigned long _task_util_est(struct task_struct *p)
{
	struct util_est ue = READ_ONCE(p->se.avg.util_est);

	return max(ue.ewma, (ue.enqueued & ~UTIL_AVG_UNCHANGED));
}
#endif
static inline bool task_fits_capacity(struct task_struct *p, int cap)
{
	unsigned long util, util_est;

	util = task_util(p);
	util_est = _task_util_est(p);

	util = max(util, util_est);

	return util * 1280 <  cap * 1024;
}

void task_check_for_rotation(struct rq *src_rq)
{
	u64 wc, wait, max_wait = 0, run, max_run = 0;
	int deserved_cpu = nr_cpu_ids, dst_cpu = nr_cpu_ids;
	int i, src_cpu = cpu_of(src_rq);
	struct rq *dst_rq;
	int heavy_task = 0;
	int force = 0;
	struct task_rotate_work *wr;

	for_each_possible_cpu(i) {
		struct rq *rq = cpu_rq(i);
		struct task_struct *curr_task = rq->curr;

		if (curr_task && !task_fits_capacity(curr_task, cpu_rq(i)->cpu_capacity))
			heavy_task += 1;

		if (heavy_task >= HEAVY_TASK_NUM)
			break;
	}

	if (heavy_task < HEAVY_TASK_NUM) {
		//trace_printk("RN1:no enough heavy tasks\n");
		return;
	}

	wc = ktime_get_raw_ns();
	for_each_possible_cpu(i) {
		struct rq *rq = cpu_rq(i);

		if (capacity_orig_of(i) >= 1024)
			continue;

		if (!rq->misfit_task_load || (rq->curr->policy != SCHED_NORMAL))
			continue;

		wait = wc - rq->curr->android_vendor_data1[3];

		if (wait > max_wait) {
			max_wait = wait;
			deserved_cpu = i;
		}
	}
	if (deserved_cpu != src_cpu) {
		//trace_printk("RN2:deserved_cpu isn't here\n");
		return;
	}

	for_each_possible_cpu(i) {
		struct rq *rq = cpu_rq(i);

		if (capacity_orig_of(i) <= capacity_orig_of(src_cpu))
			continue;

		if (rq->curr->policy != SCHED_NORMAL)
			continue;

		if (rq->nr_running > 1)
			continue;

		run = wc - rq->curr->android_vendor_data1[3];

		if (run < TASK_ROTATION_THRESHOLD_NS)
			continue;

		if (run > max_run) {
			max_run = run;
			dst_cpu = i;
		}
	}

	if (dst_cpu == nr_cpu_ids) {
		//trace_printk("RN3:no dst_cpu\n");
		return;
	}

	dst_rq = cpu_rq(dst_cpu);

	double_rq_lock(src_rq, dst_rq);
	if (dst_rq->curr->policy == SCHED_NORMAL) {
		if (!cpumask_test_cpu(dst_cpu, src_rq->curr->cpus_ptr) ||
		    !cpumask_test_cpu(src_cpu, dst_rq->curr->cpus_ptr)) {
			double_rq_unlock(src_rq, dst_rq);
			//trace_printk("RN4:no cpumask\n");
			return;
		}

		if (!src_rq->active_balance && !dst_rq->active_balance) {
			src_rq->active_balance = 1;
			dst_rq->active_balance = 1;

			get_task_struct(src_rq->curr);
			get_task_struct(dst_rq->curr);

			wr = &per_cpu(task_rotate_works, src_cpu);

			wr->src_task = src_rq->curr;
			wr->dst_task = dst_rq->curr;

			wr->src_cpu = src_rq->cpu;
			wr->dst_cpu = dst_rq->cpu;
			force = 1;
		}
	}
	double_rq_unlock(src_rq, dst_rq);

	if (force) {
		queue_work_on(src_cpu, system_highpri_wq, &wr->w);
		//trace_printk("queue work:  src_cpu:%d/%d dst_cpu:%d/%d\n",
		//	       src_cpu, wr->src_task->pid, wr->dst_cpu, wr->dst_task->pid);
	}
}

void check_for_migration(struct task_struct *p)
{
	int cpu = task_cpu(p);
	struct rq *rq = cpu_rq(cpu);

	//henryli: reduce check times
	if (capacity_orig_of(cpu) >= 1024)
		return;

	if (rq->misfit_task_load) {
		struct cpufreq_policy *policy;
		int opp_curr = 0;

		policy = cpufreq_cpu_get(cpu);
		if (policy) {
			opp_curr = policy->cur;
			cpufreq_cpu_put(policy);
		}
		if (opp_curr <= BIG_TASK_ROTATION_FREQ_MIN)
			return;

		raw_spin_lock(&migration_lock);

		task_check_for_rotation(rq);

		raw_spin_unlock(&migration_lock);
	}
}

void hook_scheduler_tick(void *data, struct rq *rq)
{
	if (rq->curr->policy == SCHED_NORMAL && likely(sunxi_sched_enable))
		check_for_migration(rq->curr);
}

void rotat_after_enqueue_task(void __always_unused *data, struct rq *rq,
			      struct task_struct *p, int flags)
{
	p->android_vendor_data1[3] = ktime_get_raw_ns();
}

void rotat_task_stats(void __always_unused *data, struct task_struct *p)
{
	p->android_vendor_data1[3] = ktime_get_raw_ns();
}

void rotat_task_newtask(void __always_unused *data,
			struct task_struct *p, unsigned long clone_flags)
{
	p->android_vendor_data1[3] = 0;
}

static int __init sunxi_sched_init(void)
{
	int ret;

	sunxi_sched_enable = 1;
	task_rotate_work_init();
	ret = register_trace_android_vh_scheduler_tick(hook_scheduler_tick, NULL);
	if (ret)
		pr_info("scheduler: register scheduler_tick hooks failed, returned %d\n", ret);
	ret = register_trace_android_rvh_after_enqueue_task(rotat_after_enqueue_task, NULL);
	if (ret)
		pr_info("scheduler: register after_enqueue_task hooks failed, returned %d\n", ret);
	ret = register_trace_android_rvh_new_task_stats(rotat_task_stats, NULL);
	if (ret)
		pr_info("scheduler: register new_task_stats hooks failed, returned %d\n", ret);

	ret = register_trace_task_newtask(rotat_task_newtask, NULL);
	if (ret)
		pr_info("scheduler: register task_newtask hooks failed, returned %d\n", ret);

	return ret;
}

static void __exit sunxi_sched_exit(void)
{
	sunxi_sched_enable = 0;
	unregister_trace_android_vh_scheduler_tick(hook_scheduler_tick, NULL);
	unregister_trace_task_newtask(rotat_task_newtask, NULL);
}

module_init(sunxi_sched_init);
module_exit(sunxi_sched_exit);

MODULE_DESCRIPTION("sunxi sched");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("henryli<henryli@allwinnertech.com>");
