// SPDX-License-Identifier: GPL-2.0-only
/*
 * motor driver
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/version.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/completion.h>

#define MOTOR_VERSION		"1.0.1"
#define MOTOR_MAX_MINORS	8

#define MOTOR_IOC_CW_MODE	_IOW('k', 1, __u8)
#define MOTOR_IOC_CCW_MODE	_IOW('k', 2, __u8)

enum motor_dir {
	MOTOR_DIR_UNKNOWN = 0,
	MOTOR_DIR_CW,
	MOTOR_DIR_CCW,
};

enum motor_location {
	MOTOR_LOCATION_MIDDLE = 0,
	MOTOR_LOCATION_LEFT,
	MOTOR_LOCATION_RIGHT,
};

struct motor_workdata {
	struct list_head entry;
	enum motor_dir dir;
	int cycle;
};

struct motor_control {
	struct platform_device *pdev;
	struct device *dev;
	struct cdev cdev;

	int phase_num;
	int *phase_gpios;
	int step_num;
	u8 *cw_table;
	u8 *ccw_table;
	int phase_udelay;
	int step_mdelay;

	struct workqueue_struct *workqueue;
	struct delayed_work delaywork;
	struct list_head data_list;
	spinlock_t lock;
	struct hrtimer timer;
	struct completion timer_done;
};

struct motor_ioctl_data {
	__u8 mode;
	__u8 cycle;
};

static struct class *motor_class;
static int motor_major;
static int motor_minors[MOTOR_MAX_MINORS];
static enum motor_location motor_loc = MOTOR_LOCATION_MIDDLE;

enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
	struct motor_control *motor = container_of(timer, struct motor_control, timer);
	complete(&motor->timer_done);
	return HRTIMER_NORESTART;
}

static ssize_t motor_limiter_read_value(void)
{
	struct file *filp;
	char status;

	filp = filp_open("/sys/devices/platform/motor_limiter/motor_limiter", O_RDONLY, 0);

	if (IS_ERR(filp))
		return PTR_ERR(filp);

	char bytes_read = kernel_read(filp, &status, 1, &filp->f_pos);

	if (bytes_read < 0) {
		pr_err("Failed to read file");
		filp_close(filp, NULL);
		return 1;
	}

	filp_close(filp, NULL);

	ssize_t cur_motor_status = status - '0';

	if (cur_motor_status <= 0)
		cur_motor_status = 0;
	else
		cur_motor_status = 1;

	return cur_motor_status;
}

static void motor_set_phase(struct device *dev, int *gpios, int num, int phases, int delay)
{
	int i;
	int value;

	for (i = 0; i < num; i++) {
		value = (phases >> i) & 0x01;
		dev_dbg(dev, "set gpio %d value %d\n", gpios[i], value);
		gpio_set_value(gpios[i], value);
		udelay(delay);
	}
}

static void motor_set_stop(struct device *dev, int *gpios, int num)
{
	int i;

	for (i = 0; i < num; i++)
		gpio_set_value(gpios[i], 0);
}

static bool motor_forward_reverse_find_point(struct motor_control *motor,
											struct motor_workdata *data)
{
	int PreventStuckNum = 0;
	ktime_t kt;
	char *phase_table = NULL;

	if (motor_limiter_read_value() == 1)
		return false;

	if (data->dir == MOTOR_DIR_CW) {
		phase_table = motor->ccw_table;
		motor_loc = MOTOR_LOCATION_LEFT;
	} else {
		phase_table = motor->cw_table;
		motor_loc = MOTOR_LOCATION_RIGHT;
	}

	/* deal motor slip */
	hrtimer_init(&motor->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	motor->timer.function = timer_callback;
	init_completion(&motor->timer_done);

	for (PreventStuckNum = 0; !motor_limiter_read_value(); PreventStuckNum++) {
		for (int j = 0; j < motor->step_num; j++) {
			motor_set_phase(motor->dev, motor->phase_gpios,
							motor->phase_num, phase_table[j],
							motor->phase_udelay);
			kt = ktime_set(0, motor->step_mdelay * 1000000);
			hrtimer_start(&motor->timer, kt, HRTIMER_MODE_REL);
			wait_for_completion(&motor->timer_done);
			reinit_completion(&motor->timer_done);
		}
		if (PreventStuckNum > 50) {
			PreventStuckNum = 0;
			break;
		}
	}

	motor_set_stop(motor->dev, motor->phase_gpios, motor->phase_num);
	hrtimer_cancel(&motor->timer);
	return true;
}

static int motor_run_mstep(struct motor_control *motor, struct motor_workdata *data)
{
	int i, j;
	char *phase_table = NULL;
	ktime_t kt;

	switch (data->dir) {
	case MOTOR_DIR_CW:
		phase_table = motor->cw_table;
		break;
	case MOTOR_DIR_CCW:
		phase_table = motor->ccw_table;
		break;
	default:
		dev_err(motor->dev, "motor run step dir_%d error\n", data->dir);
		return -EINVAL;
	}

	if (!motor_forward_reverse_find_point(motor, data)) {
		if (((data->dir == MOTOR_DIR_CW) && (motor_loc != MOTOR_LOCATION_LEFT)) ||
			(((data->dir == MOTOR_DIR_CCW) && (motor_loc != MOTOR_LOCATION_RIGHT)))) {
			motor_loc = MOTOR_LOCATION_MIDDLE;
			hrtimer_init(&motor->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			motor->timer.function = timer_callback;
			init_completion(&motor->timer_done);
			for (i = 0; i < data->cycle; i++) {
				for (j = 0; j < motor->step_num; j++) {
					dev_dbg(motor->dev, "cycle_%d set motor phase 0x%x\n", i,
							phase_table[j]);
					motor_set_phase(motor->dev, motor->phase_gpios,
									motor->phase_num,
									phase_table[j],
									motor->phase_udelay);
					kt = ktime_set(0, motor->step_mdelay * 1000000);
					hrtimer_start(&motor->timer, kt, HRTIMER_MODE_REL);
					wait_for_completion(&motor->timer_done);
					reinit_completion(&motor->timer_done);
				}
		}
		motor_set_stop(motor->dev, motor->phase_gpios, motor->phase_num);
		hrtimer_cancel(&motor->timer);
		}
	}

	motor_forward_reverse_find_point(motor, data);

	return 0;
}

static void motor_mstep_work_handler(struct work_struct *work)
{
	struct motor_control *motor = container_of(work, struct motor_control, delaywork.work);
	struct motor_workdata *data;

	if (!motor)
		return;

	while (!list_empty(&motor->data_list)) {
		data = list_entry(motor->data_list.next, struct motor_workdata, entry);
		dev_dbg(motor->dev, "handler get workdata dir_%d cycle_%d\n",
				data->dir, data->cycle);

		motor_run_mstep(motor, data);

		spin_lock(&motor->lock);
		list_del(&data->entry);
		spin_unlock(&motor->lock);

		devm_kfree(motor->dev, data);
	}
}

static int motor_open(struct inode *inode, struct file *file)
{
	struct motor_control *motor = container_of(inode->i_cdev, struct motor_control, cdev);

	file->private_data = motor;

	dev_dbg(motor->dev, "file open\n");

	return 0;
}

static int motor_release(struct inode *inode, struct file *file)
{
	struct motor_control *motor = container_of(inode->i_cdev, struct motor_control, cdev);

	file->private_data = NULL;

	dev_dbg(motor->dev, "file release\n");

	return 0;
}

static long motor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct motor_control *motor = file->private_data;
	int cycle = 0;
	enum motor_dir dir = MOTOR_DIR_UNKNOWN;
	struct motor_workdata *data;
	int ret = 0;

	data = devm_kzalloc(motor->dev, sizeof(*data), GFP_KERNEL);

	switch (cmd) {
	case MOTOR_IOC_CW_MODE:
		dir = MOTOR_DIR_CW;
		get_user(cycle, (u8 __user *)arg);
		break;
	case MOTOR_IOC_CCW_MODE:
		dir = MOTOR_DIR_CCW;
		get_user(cycle, (u8 __user *)arg);
		break;
	default:
		dev_err(motor->dev, "ioctl unsupport cmd %d\n", cmd);
		ret = -EINVAL;
	}
	if (!ret) {
		INIT_LIST_HEAD(&data->entry);
		data->dir = dir;
		data->cycle = cycle;
		spin_lock(&motor->lock);
		list_add_tail(&data->entry, &motor->data_list);
		spin_unlock(&motor->lock);
		dev_dbg(motor->dev, "motor dir_%d cycle_%d\n", data->dir, data->cycle);
		queue_delayed_work(motor->workqueue, &motor->delaywork, msecs_to_jiffies(1));
	}

	return ret;
}

static const struct file_operations motor_file_ops = {
	.open = motor_open,
	.release = motor_release,
	.unlocked_ioctl = motor_ioctl,
};

static ssize_t motor_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", (int)motor_loc);
}

static ssize_t motor_ctrl_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct motor_control *motor = (struct motor_control *)platform_get_drvdata(pdev);

	int err = 0;
	char *ptr = NULL;
	char *p[2];
	int cycle = 1;
	enum motor_dir dir = MOTOR_DIR_UNKNOWN;
	struct motor_workdata *data;

	ptr = (char *)buf;
	if (!strchr(ptr, ',')) {
		err = kstrtouint(ptr, 10, &dir);
		if (err) {
			dev_err(motor->dev, "failed to parse str %s\n", ptr);
			dir = MOTOR_DIR_UNKNOWN;
		}
	} else {
		p[0] = strsep(&ptr, ",");
		p[1] = strsep(&ptr, ",");

		err = kstrtouint(p[0], 10, &dir);
		if (err) {
			dev_err(motor->dev, "failed to parse str %s\n", ptr);
			dir = MOTOR_DIR_UNKNOWN;
		}
		err = kstrtouint(p[1], 10, &cycle);
		if (err) {
			dev_err(motor->dev, "failed to parse str %s\n", ptr);
			cycle = 0;
		}
	}

	data = devm_kzalloc(motor->dev, sizeof(*data), GFP_KERNEL);
	INIT_LIST_HEAD(&data->entry);
	data->dir = dir & 0xff;
	data->cycle = cycle;
	spin_lock(&motor->lock);
	list_add_tail(&data->entry, &motor->data_list);
	spin_unlock(&motor->lock);
	dev_dbg(motor->dev, "motor dir_%d cycle_%d\n", data->dir, data->cycle);

	queue_delayed_work(motor->workqueue, &motor->delaywork, msecs_to_jiffies(1));

	return count;
}

static struct device_attribute motor_ctrl_attr = {
	.attr = {
		.name = "motor_ctrl",
		.mode = 0664,
	},
	.show = motor_ctrl_show,
	.store = motor_ctrl_store,
};

static int motor_control_fdt_parse(struct motor_control *motor, struct device_node *np)
{
	int err;
	int i;
	char motor_gpio_name[32] = {0};
	char str[256];
	char *ptr = NULL;

	err = of_property_read_u32(np, "motor-phase-num", &motor->phase_num);
	if (err) {
		dev_err(motor->dev, "failed to get gpio num\n");
		return -EINVAL;
	}

	err = of_property_read_u32(np, "motor-step-num", &motor->step_num);
	if (err) {
		dev_err(motor->dev, "failed to get phase num\n");
		return -EINVAL;
	}

	err = of_property_read_u32(np, "motor-phase-udelay", &motor->phase_udelay);
	if (err) {
		dev_err(motor->dev, "failed to get phase udelay time\n");
		return -EINVAL;
	}

	err = of_property_read_u32(np, "motor-step-mdelay", &motor->step_mdelay);
	if (err) {
		dev_err(motor->dev, "failed to get step mdelay time\n");
		return -EINVAL;
	}

	motor->phase_gpios = devm_kzalloc(motor->dev, sizeof(int) * motor->phase_num, GFP_KERNEL);
	if (IS_ERR_OR_NULL(motor->phase_gpios)) {
		dev_err(motor->dev, "failed to alloc motor phase_gpios mem\n");
		return -ENOMEM;
	}

	motor->cw_table = devm_kzalloc(motor->dev, motor->step_num, GFP_KERNEL);
	if (IS_ERR_OR_NULL(motor->cw_table)) {
		dev_err(motor->dev, "failed to alloc motor cw_table mem\n");
		return -ENOMEM;
	}

	motor->ccw_table = devm_kzalloc(motor->dev, motor->step_num, GFP_KERNEL);
	if (IS_ERR_OR_NULL(motor->ccw_table)) {
		dev_err(motor->dev, "failed to alloc motor ccw_table mem\n");
		return -ENOMEM;
	}

	for (i = 0; i < motor->phase_num; i++) {
		sprintf(motor_gpio_name, "motor-gpio-phase%d", i);
		motor->phase_gpios[i] = of_get_named_gpio(np, motor_gpio_name, 0);
		if (motor->phase_gpios[i] < 0) {
			dev_err(motor->dev, "failed to get property %s\n", motor_gpio_name);
			return -EINVAL;
		}
		err = gpio_request(motor->phase_gpios[i], motor_gpio_name);
		if (err < 0) {
			dev_err(motor->dev, "failed to request gpio_%d %s\n",
					motor->phase_gpios[i], motor_gpio_name);
			return err;
		}
		gpio_direction_output(motor->phase_gpios[i], 0);
		dev_dbg(motor->dev, "gpio_%d %s output 0",
				motor->phase_gpios[i], motor_gpio_name);
	}

	err = of_property_read_u8_array(np, "motor-cw-table", motor->cw_table, motor->step_num);
	if (err < 0)
		dev_err(motor->dev, "failed to get motor-cw-table size %d\n", motor->step_num);

	err = of_property_read_u8_array(np, "motor-ccw-table", motor->ccw_table, motor->step_num);
	if (err < 0)
		dev_err(motor->dev, "failed to get motor-ccw-table index %d\n", motor->step_num);

	dev_info(motor->dev, "motor-phase-num %d\n", motor->phase_num);
	dev_info(motor->dev, "motor-step-num %d\n", motor->step_num);

	memset(str, 0, sizeof(str));
	ptr = str;
	ptr += sprintf(ptr, "motor-cw-table < ");
	for (i = 0; i < motor->step_num; i++)
		ptr += sprintf(ptr, "0x%02x ", motor->cw_table[i]);
	sprintf(ptr, ">");
	dev_info(motor->dev, "%s\n", str);

	memset(str, 0, sizeof(str));
	ptr = str;
	ptr += sprintf(ptr, "motor-ccw-table < ");
	for (i = 0; i < motor->step_num; i++)
		ptr += sprintf(ptr, "0x%02x ", motor->ccw_table[i]);
	sprintf(ptr, ">");
	dev_info(motor->dev, "%s\n", str);

	return 0;
}

static unsigned int motor_control_get_minors(struct motor_control *motor)
{
	unsigned int i;

	spin_lock(&motor->lock);

	for (i = 0; i < MOTOR_MAX_MINORS; i++) {
		if (motor_minors[i] == 0) {
			motor_minors[i] = 1;
			break;
		}
	}

	spin_unlock(&motor->lock);

	return i;
}

static void motor_control_put_minors(struct motor_control *motor, unsigned int minor)
{
	spin_lock(&motor->lock);

	motor_minors[minor] = 0;

	spin_unlock(&motor->lock);
}

static int motor_control_probe(struct platform_device *pdev)
{
	struct motor_control *motor = NULL;
	int minor;
	int err;

	motor = devm_kzalloc(&pdev->dev, sizeof(*motor), GFP_KERNEL);
	if (IS_ERR_OR_NULL(motor))
		return -ENOMEM;  /* Do not print prompts after kzalloc errors */

	motor->pdev = pdev;
	motor->dev = &pdev->dev;

	err = motor_control_fdt_parse(motor, pdev->dev.of_node);
	if (err < 0) {
		dev_err(motor->dev, "failed to get fdt resource\n");
		return err;
	}

	spin_lock_init(&motor->lock);

	INIT_LIST_HEAD(&motor->data_list);

	motor->workqueue = create_singlethread_workqueue(dev_name(motor->dev));
	INIT_DELAYED_WORK(&motor->delaywork, motor_mstep_work_handler);

	platform_set_drvdata(pdev, motor);

	cdev_init(&motor->cdev, &motor_file_ops);
	motor->cdev.owner = THIS_MODULE;

	minor = motor_control_get_minors(motor);

	err = cdev_add(&motor->cdev, MKDEV(motor_major, minor), 1);
	if (err) {
		dev_err(motor->dev, "chardev registration failed\n");
		goto err0;
	}

	if (IS_ERR(device_create(motor_class, motor->dev, MKDEV(motor_major, minor),
		motor, dev_name(motor->dev)))) {
		dev_err(motor->dev, "can't create device\n");
		goto err1;
	}

	err = device_create_file(motor->dev, &motor_ctrl_attr);
	if (err < 0) {
		dev_err(motor->dev, "failed to create device file motor_ctrl\n");
		goto err2;
	}

	dev_info(motor->dev, "probe success\n");
	return 0;

err2:
	device_destroy(motor_class, MKDEV(motor_major, minor));
err1:
	cdev_del(&motor->cdev);
err0:
	motor_control_put_minors(motor, minor);
	return err;
}

static int motor_control_remove(struct platform_device *pdev)
{
	struct motor_control *motor = (struct motor_control *)platform_get_drvdata(pdev);
	unsigned int minor = MINOR(motor->cdev.dev);
	int i;

	device_remove_file(motor->dev, &motor_ctrl_attr);

	device_destroy(motor_class, MKDEV(motor_major, minor));

	cdev_del(&motor->cdev);

	motor_control_put_minors(motor, minor);

	cancel_delayed_work(&motor->delaywork);
	destroy_workqueue(motor->workqueue);

	for (i = 0; i < motor->phase_num; i++)
		gpio_free(motor->phase_gpios[i]);

	return 0;
}

static const struct of_device_id of_motor_control_match[] = {
	{ .compatible = "motor-control", },
	{},
};

static struct platform_driver motor_control_driver = {
	.probe = motor_control_probe,
	.remove = motor_control_remove,
	.driver = {
		.name = "motor-control",
		.of_match_table = of_motor_control_match,
	},
};

static int __init motor_init(void)
{
	int ret;
	dev_t dev;

	motor_class = class_create("motor");
	if (IS_ERR(motor_class)) {
		ret = PTR_ERR(motor_class);
		pr_err("motor: can't register motor class\n");
		goto err;
	}

	ret = alloc_chrdev_region(&dev, 0, MOTOR_MAX_MINORS, "motor");
	if (ret) {
		pr_err("motor: can't register character device\n");
		goto err_class;
	}
	motor_major = MAJOR(dev);

	ret = platform_driver_register(&motor_control_driver);
	if (ret) {
		pr_err("motor: can't register platfrom driver\n");
		goto err_unchr;
	}

	pr_info("motor linux driver init ok (Version %s)\n", MOTOR_VERSION);

	return 0;

err_unchr:
	unregister_chrdev_region(dev, MOTOR_MAX_MINORS);
err_class:
	class_destroy(motor_class);
err:
	return ret;
}

static void __exit motor_exit(void)
{
	platform_driver_unregister(&motor_control_driver);

	unregister_chrdev_region(MKDEV(motor_major, 0), MOTOR_MAX_MINORS);

	class_destroy(motor_class);
}

module_init(motor_init)
module_exit(motor_exit)

MODULE_AUTHOR("AWA2252");
MODULE_DESCRIPTION("Motor Control Driver");
MODULE_VERSION(MOTOR_VERSION);
MODULE_LICENSE("GPL");
