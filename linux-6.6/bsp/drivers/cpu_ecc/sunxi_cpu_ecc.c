// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *  An cpu ecc  driver for Sunxi Platform of Allwinner SoC
 *
 * Copyright (C) 2024 panzhijian@allwinnertech.com
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <sunxi-sid.h>

#define ERR0STATUS_CE_MASK            (0x3 << 24)
#define ERR0STATUS_UE_MASK            (0x1 << 29)

#define ERRSELR_EL1_SEL               (0x1 << 0)

#define ERR0PFGCTLR_UC                (0x1 << 1)
#define ERR0PFGCTLR_UER               (0x1 << 3)
#define ERR0PFGCTLR_DE                (0x1 << 5)
#define ERR0PFGCTLR_CE                (0x1 << 6)
#define ERR0PFGCTLR_R                 (0x1 << 30)
#define ERR0PFGCTLR_CDNEN             (0x1 << 31)

#define CORE_NUMBER	              (core_number)

static int core_number = 4;

struct sunxi_cpu_ecc_irq_info_t {
	const char *devname;
	const char *interrupt_name;
	irq_handler_t handler;
};

static irqreturn_t sunxi_cpu_ecc_nerrirq0_handler(int irq, void *dev_id);
static irqreturn_t sunxi_cpu_ecc_nfaultirq0_handler(int irq, void *dev_id);
static irqreturn_t sunxi_cpu_ecc_nerrirq1_handler(int irq, void *dev_id);
static irqreturn_t sunxi_cpu_ecc_nfaultirq1_handler(int irq, void *dev_id);

static const struct sunxi_cpu_ecc_irq_info_t sunxi_cpu_ecc_irq_info[] = {
	{"cpu-ecc-nerrirq0", "nerrirq0", sunxi_cpu_ecc_nerrirq0_handler},
	{"cpu-ecc-nerrirq1", "nerrirq1", sunxi_cpu_ecc_nerrirq1_handler},
	{"cpu-ecc-nerrirq2", "nerrirq2", sunxi_cpu_ecc_nerrirq1_handler},
	{"cpu-ecc-nerrirq3", "nerrirq3", sunxi_cpu_ecc_nerrirq1_handler},
	{"cpu-ecc-nerrirq4", "nerrirq4", sunxi_cpu_ecc_nerrirq1_handler},
	{"cpu-ecc-nfaultirq0", "nfaultirq0", sunxi_cpu_ecc_nfaultirq0_handler},
	{"cpu-ecc-nfaultirq1", "nfaultirq1", sunxi_cpu_ecc_nfaultirq1_handler},
	{"cpu-ecc-nfaultirq2", "nfaultirq2", sunxi_cpu_ecc_nfaultirq1_handler},
	{"cpu-ecc-nfaultirq3", "nfaultirq3", sunxi_cpu_ecc_nfaultirq1_handler},
	{"cpu-ecc-nfaultirq4", "nfaultirq4", sunxi_cpu_ecc_nfaultirq1_handler},
};

#define CPU_ECC_IRQ_NUM    (sizeof(sunxi_cpu_ecc_irq_info)/sizeof(sunxi_cpu_ecc_irq_info[0]))

struct sunxi_cpu_ecc_dev {
	struct device          *dev;
	int                    irq[CPU_ECC_IRQ_NUM];
};

#ifdef CONFIG_AW_CPU_ECC_TEST
static struct workqueue_struct *cpu_ecc_workqueue;
static struct work_struct cpu_ecc_work;
static unsigned int cpu_to_bind;
static unsigned int inject_bit;

static void sunxi_cpu_ecc_enject_err_work(struct work_struct *work)
{
	u64 data0 = 0;
	u32 data1 = 0;

	/*
	* cpu_to_bind: 0~3->cpu0~cpu3, 4->dsu
	* ERRSELR_EL1: S3_0_C5_C3_1
	* bit0: 0-Select record 0 containing errors
	* from level-1 and level-2 RAMs located in the Cortex-A55 core.
	* 1-Select record 1 containing errors from level-3 RAMs located in the DSU.
	*/
	if (cpu_to_bind >= 0 && cpu_to_bind < CORE_NUMBER) {
		pr_debug("cpu ecc inject %u bit(s) to cpu%u\n",
			inject_bit, cpu_to_bind);
		asm("MRS %0, S3_0_C5_C3_1" : "=r" (data0));
		data0 &= (~(u64)ERRSELR_EL1_SEL);
		asm volatile("MSR S3_0_C5_C3_1, %0" : : "r" (data0));
	} else if (cpu_to_bind == CORE_NUMBER) {
		pr_debug("cpu ecc inject %u bit(s) to dsu\n", inject_bit);
		asm("MRS %0, S3_0_C5_C3_1" : "=r" (data0));
		data0 |= ERRSELR_EL1_SEL;
		asm volatile("MSR S3_0_C5_C3_1, %0" : : "r" (data0));
	} else {
		pr_warn("cpu to bind err: %u\n", cpu_to_bind);
		return;
	}

	/* ERR0PFGCDNR(a55) - ERXPFGCDN_EL1(armv8): S3_0_C15_C2_2 */
	data1 = 0x500;
	asm volatile("MSR S3_0_C15_C2_2, %0" : : "r" (data1));
	/* ERR0PFGCTLR(a55) - ERXPFGCTL_EL1(armv8): S3_0_C15_C2_1 */
	asm("MRS %0, S3_0_C15_C2_1" : "=r" (data1));

	if (inject_bit <= 1) {
		/* inject 1 bit err */
		data1 |= ERR0PFGCTLR_CDNEN | ERR0PFGCTLR_R | ERR0PFGCTLR_CE;
	} else {
		/* inject multi bits err */
		data1 |= (0x1 << 31) | (0x1 << 30) | (0x1 << 5) | (0x1 << 3) | (0x1 << 1);
		data1 |= ERR0PFGCTLR_CDNEN | ERR0PFGCTLR_R | ERR0PFGCTLR_DE
			| ERR0PFGCTLR_UER | ERR0PFGCTLR_UC;
	}

	asm volatile("MSR S3_0_C15_C2_1, %0" : : "r" (data1));

	pr_debug("cpu%d finish inject\n", cpu_to_bind);
}

static ssize_t cpu_ecc_inject_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	unsigned int cpu_to_bind_tmp;

	ret = sscanf(buf, "%u %u", &cpu_to_bind, &inject_bit);
	cpu_to_bind_tmp = cpu_to_bind;
	if (cpu_to_bind_tmp > CORE_NUMBER) {
		pr_warn("cpu_to_bind_tmp err: %u\n", cpu_to_bind_tmp);
		return -cpu_to_bind_tmp;
	} else if (cpu_to_bind_tmp == CORE_NUMBER) {
		cpu_to_bind_tmp = 0;
	}

	if (!queue_work_on(cpu_to_bind_tmp, cpu_ecc_workqueue, &cpu_ecc_work)) {
		pr_err("failed to queue work\n");
		return -EFAULT;
	}

	return count;
}
static CLASS_ATTR_WO(cpu_ecc_inject);

static struct attribute *cpu_ecc_class_attrs[] = {
	&class_attr_cpu_ecc_inject.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cpu_ecc_class);

static struct class cpu_ecc_class = {
	.name		= "cpu_ecc",
	.class_groups	= cpu_ecc_class_groups,
};
#endif /* #ifdef CONFIG_AW_CPU_ECC_TEST */

static irqreturn_t sunxi_cpu_ecc_nerrirq0_handler(int irq, void *dev_id)
{
	int cpu = smp_processor_id();
	u32 val = 0;

	/*
	* clear pending
	* ERR0STATUS: S3_0_C5_C4_2
	*/
	asm("MRS %0, S3_0_C5_C4_2" : "=r" (val));
	val |= ERR0STATUS_UE_MASK;
	asm volatile("MSR S3_0_C5_C4_2, %0" : : "r" (val));

#ifdef CONFIG_AW_CPU_ECC_TEST
	/*
	* disable cpu ecc inject
	* ERXPFGCTL_EL1: S3_0_C15_C2_1
	*/
	val = 0x0;
	asm volatile("MSR S3_0_C15_C2_1, %0" : : "r" (val));
#endif
	pr_debug("cpu(dsu) ecc nerrirq0 isr on cpu%d, irq = %d\n", cpu, irq);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_cpu_ecc_nfaultirq0_handler(int irq, void *dev_id)
{
	int cpu = smp_processor_id();
	u32 val = 0;

	/*
	* clear pending
	* ERR0STATUS: S3_0_C5_C4_2
	*/
	asm("MRS %0, S3_0_C5_C4_2" : "=r" (val));
	val |= ERR0STATUS_CE_MASK;
	asm volatile("MSR S3_0_C5_C4_2, %0" : : "r" (val));

#ifdef CONFIG_AW_CPU_ECC_TEST
	/*
	* disable cpu ecc inject
	* ERXPFGCTL_EL1: S3_0_C15_C2_1
	*/
	val = 0x0;
	asm volatile("MSR S3_0_C15_C2_1, %0" : : "r" (val));
#endif
	pr_debug("dsu ecc nfaultirq0 isr on cpu%d, irq = %d\n", cpu, irq);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_cpu_ecc_nerrirq1_handler(int irq, void *dev_id)
{
	int cpu = smp_processor_id();
	u32 val = 0;

	/*
	* clear pending
	* ERR0STATUS: S3_0_C5_C4_2
	*/
	asm("MRS %0, S3_0_C5_C4_2" : "=r" (val));
	val |= ERR0STATUS_UE_MASK;
	asm volatile("MSR S3_0_C5_C4_2, %0" : : "r" (val));

#ifdef CONFIG_AW_CPU_ECC_TEST
	/*
	* disable cpu ecc inject
	* ERXPFGCTL_EL1: S3_0_C15_C2_1
	*/
	val = 0x0;
	asm volatile("MSR S3_0_C15_C2_1, %0" : : "r" (val));
#endif
	pr_debug("cpu(core0) ecc nerrirq1 isr on cpu%d, irq = %d\n", cpu, irq);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_cpu_ecc_nfaultirq1_handler(int irq, void *dev_id)
{
	int cpu = smp_processor_id();
	u32 val = 0;

	/*
	* clear pending
	* ERR0STATUS: S3_0_C5_C4_2
	*/
	asm("MRS %0, S3_0_C5_C4_2" : "=r" (val));
	val |= ERR0STATUS_CE_MASK;
	asm volatile("MSR S3_0_C5_C4_2, %0" : : "r" (val));

#ifdef CONFIG_AW_CPU_ECC_TEST
	/*
	* disable cpu ecc inject
	* ERXPFGCTL_EL1: S3_0_C15_C2_1
	*/
	val = 0x0;
	asm volatile("MSR S3_0_C15_C2_1, %0" : : "r" (val));
#endif
	pr_debug("cpu ecc nfaultirq1 isr on cpu%d, irq = %d\n", cpu, irq);

	return IRQ_HANDLED;
}

static int sunxi_cpu_ecc_init(struct platform_device *pdev)
{
	struct sunxi_cpu_ecc_dev *cpu_ecc_dev = platform_get_drvdata(pdev);
	int i = 0;
	int ret = 0;

	for (i = 0; i < CPU_ECC_IRQ_NUM; i++) {
		cpu_ecc_dev->irq[i] = platform_get_irq_byname(pdev, sunxi_cpu_ecc_irq_info[i].interrupt_name);
		if (cpu_ecc_dev->irq[i] < 0) {
			ret = cpu_ecc_dev->irq[i];
			return ret;
		}

		/* dsu irq need not to set affinity*/
		if (i % (CORE_NUMBER + 1) != 0) {
			ret = irq_set_affinity(cpu_ecc_dev->irq[i], cpumask_of(i % (CORE_NUMBER + 1) - 1));
			if (ret) {
				dev_err(&pdev->dev, "Failed to set interrupt affinity\n");
				return ret;
			}
		}

		ret = devm_request_irq(&pdev->dev, cpu_ecc_dev->irq[i],
			sunxi_cpu_ecc_irq_info[i].handler,
			IRQF_SHARED, sunxi_cpu_ecc_irq_info[i].devname, cpu_ecc_dev);
		if (ret) {
			dev_err(&pdev->dev, "cpu ecc could not request irq_info%d!\n", i);
			return ret;
		}
	}

	dev_dbg(&pdev->dev, "cpu ecc init successed\n");

	return ret;
}

static void sunxi_cpu_ecc_exit(struct platform_device *pdev)
{
	struct sunxi_cpu_ecc_dev *cpu_ecc_dev = platform_get_drvdata(pdev);
	int i = 0;

	for (i = 0; i < CPU_ECC_IRQ_NUM; i++) {
		if (cpu_ecc_dev->irq[i] >= 0) {
			devm_free_irq(&pdev->dev, cpu_ecc_dev->irq[i], cpu_ecc_dev);
		}
	}

	dev_dbg(&pdev->dev, "cpu ecc exit successed\n");
}

static bool sunxi_is_ecc_enable(void)
{
	return sunxi_sid_get_ecc_status() ? true : false;
}

static int sunxi_cpu_ecc_probe(struct platform_device *pdev)
{
	struct sunxi_cpu_ecc_dev *cpu_ecc_dev = NULL;
	struct device *dev = &pdev->dev;
	int err = 0;

	if (!sunxi_is_ecc_enable())
		return -ENODEV;

	cpu_ecc_dev = devm_kzalloc(dev, sizeof(*cpu_ecc_dev), GFP_KERNEL);
	if (!cpu_ecc_dev)
		return -ENOMEM;

	core_number = num_possible_cpus();
	platform_set_drvdata(pdev, cpu_ecc_dev);
	err = sunxi_cpu_ecc_init(pdev);
	if (err) {
		dev_err(dev, "cpu ecc inti failed, err = %d\n", err);
		return err;
	}

#ifdef CONFIG_AW_CPU_ECC_TEST
	cpu_ecc_workqueue = create_workqueue("cpu_ecc_workqueue");
	if (!cpu_ecc_workqueue) {
		dev_err(dev, "failed to create cpu ecc workqueue\n");
		return -ENOMEM;
	}

	INIT_WORK(&cpu_ecc_work, sunxi_cpu_ecc_enject_err_work);
	err = class_register(&cpu_ecc_class);
	if (err) {
		dev_err(dev, "failed to cpu_ecc class register, err = %d\n", err);
		return err;
	}
#endif

	return 0;
}

static int sunxi_cpu_ecc_remove(struct platform_device *pdev)
{
#ifdef CONFIG_AW_CPU_ECC_TEST
	class_unregister(&cpu_ecc_class);
	flush_workqueue(cpu_ecc_workqueue);
	destroy_workqueue(cpu_ecc_workqueue);
#endif

	sunxi_cpu_ecc_exit(pdev);

	return 0;
}

static const struct of_device_id sunxi_cpu_ecc_of_match[] = {
	{ .compatible = "allwinner,sun55iw6-cpu-ecc", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_cpu_ecc_of_match);

static struct platform_driver sunxi_cpu_ecc_driver = {
	.probe = sunxi_cpu_ecc_probe,
	.remove	= sunxi_cpu_ecc_remove,
	.driver = {
		.name = "sunxi-cpu-ecc",
		.of_match_table = sunxi_cpu_ecc_of_match,
	},
};
module_platform_driver(sunxi_cpu_ecc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner cpu ecc driver");
MODULE_ALIAS("platform: sunxi_cpu_ecc");
MODULE_AUTHOR("panzhijian <panzhijian@allwinnertech.com>");
MODULE_VERSION("1.0.0");
