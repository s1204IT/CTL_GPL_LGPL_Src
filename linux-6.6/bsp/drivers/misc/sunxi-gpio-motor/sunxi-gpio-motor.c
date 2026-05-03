// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 - 2024 Allwinner Technology Co.,Ltd. All rights reserved.
 */
/*
 * Copyright (c) 2024 zhangyuanjing@allwinnertech.com
 */

/* #define DEBUG */
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "sunxi-gpio-motor.h"

#define DRV_NAME    "sunxi-gpio-motor"
#define DRV_VERSION "1.1.0"

#define PIN_LOW	 0
#define PIN_HIGH 1

#define DEFAULT_TIME_ON_PERIOD 1000  // (us)

static struct hrtimer motor_hrtimer;
static sunxi_motor_dev_t *motor_dev;
static sunxi_motor_device_status_t device_status_k2u;
static sunxi_motor_cmd_t cmd_u2k;
static sunxi_motor_para_t para_u2k;

static u32 motor_run_time_ab;
static u32 motor_run_time_cd;
static u32 time_to_running_ab;
static u32 time_to_running_cd;
static u32 time_per_step_ab;
static u32 time_per_step_cd;

static u32 dir_ab;
static u32 dir_cd;

static const sunxi_motor_pin_step_t motor_pin_step[] = {
	{ PIN_LOW, PIN_LOW, PIN_LOW, PIN_LOW },
	{ PIN_HIGH, PIN_LOW, PIN_LOW, PIN_LOW },
	{ PIN_HIGH, PIN_HIGH, PIN_LOW, PIN_LOW },
	{ PIN_LOW, PIN_HIGH, PIN_LOW, PIN_LOW },
	{ PIN_LOW, PIN_HIGH, PIN_HIGH, PIN_LOW },
	{ PIN_LOW, PIN_LOW, PIN_HIGH, PIN_LOW },
	{ PIN_LOW, PIN_LOW, PIN_HIGH, PIN_HIGH },
	{ PIN_LOW, PIN_LOW, PIN_LOW, PIN_HIGH },
	{ PIN_HIGH, PIN_LOW, PIN_LOW, PIN_HIGH },
};

static void sunxi_motor_set_ab_pin_value(u8 pin_map_index)
{
	gpio_set_value(motor_dev->ab_pin_black,
		       motor_pin_step[pin_map_index].pin_black);
	gpio_set_value(motor_dev->ab_pin_yellow,
		       motor_pin_step[pin_map_index].pin_yellow);
	gpio_set_value(motor_dev->ab_pin_brown,
		       motor_pin_step[pin_map_index].pin_brown);
	gpio_set_value(motor_dev->ab_pin_blue,
		       motor_pin_step[pin_map_index].pin_blue);
}

static void sunxi_motor_set_cd_pin_value(u8 pin_map_index)
{
	gpio_set_value(motor_dev->cd_pin_black,
		       motor_pin_step[pin_map_index].pin_black);
	gpio_set_value(motor_dev->cd_pin_yellow,
		       motor_pin_step[pin_map_index].pin_yellow);
	gpio_set_value(motor_dev->cd_pin_brown,
		       motor_pin_step[pin_map_index].pin_brown);
	gpio_set_value(motor_dev->cd_pin_blue,
		       motor_pin_step[pin_map_index].pin_blue);
}

static void sunxi_motor_enable(void)
{
	sunxi_motor_set_ab_pin_value(DIR_NONE);
	sunxi_motor_set_cd_pin_value(DIR_NONE);
	motor_run_time_ab = 0;
	motor_run_time_cd = 0;
	MOTOR_DEBUG("sunxi gpio motor enable success, now enable timer.\n");
	hrtimer_start(&motor_hrtimer, ktime_set(0, 0), HRTIMER_MODE_REL);
}

static void sunxi_motor_disable(void)
{
	sunxi_motor_set_ab_pin_value(DIR_NONE);
	sunxi_motor_set_cd_pin_value(DIR_NONE);
	device_status_k2u.status = 0;
	MOTOR_DEBUG("sunxi gpio motor set to disable.\n");
}

static void sunxi_motor_speed_config(void)
{
	if (IS_DIR_AB(cmd_u2k.dir)) {
		time_per_step_ab = cmd_u2k.ab_time_per_step;
		time_to_running_ab = 8 * cmd_u2k.ab_time_per_step * cmd_u2k.ab_step;
	} else {
		time_per_step_ab = 0;
		time_to_running_ab = 0;
	}

	if (IS_DIR_CD(cmd_u2k.dir)) {
		time_per_step_cd = cmd_u2k.cd_time_per_step;
		time_to_running_cd = 8 * cmd_u2k.cd_time_per_step * cmd_u2k.cd_step;
	} else {
		time_per_step_cd = 0;
		time_to_running_cd = 0;
	}
}

static void sunxi_motor_wake_up(void)
{
	motor_dev->n_que_waiting = 1;
	wake_up(&motor_dev->i_wait_que);
}

static enum hrtimer_restart sunxi_motor_timer_fn(struct hrtimer *hrtimer)
{
	static int ab_step = 1;
	static int cd_step = 1;
	ktime_t period;
	static int ab_period_cnt;
	static int cd_period_cnt;

	if ((motor_run_time_ab >= time_to_running_ab) && (motor_run_time_cd >= time_to_running_cd)) {
		sunxi_motor_disable();
		if (cmd_u2k.block == 1) {
			sunxi_motor_wake_up();
		}
		device_status_k2u.status = 0;
		return HRTIMER_NORESTART;
	} else {
		mutex_lock(&motor_dev->lock);
		device_status_k2u.status = 1;

		if (!IS_DIR_AB(cmd_u2k.dir) && !IS_DIR_CD(cmd_u2k.dir)) {
			pr_err("sunxi gpio motor: given dir para %d error\n",
			       cmd_u2k.dir);
			mutex_unlock(&motor_dev->lock);
			return HRTIMER_NORESTART;
		}

		if ((motor_run_time_ab < time_to_running_ab) && IS_DIR_AB(cmd_u2k.dir)) {
			ab_period_cnt++;
			if (ab_period_cnt >= time_per_step_ab) {
				MOTOR_DEBUG("dir[%d] step[%d]\r\n", GET_DIR_AB(cmd_u2k.dir), ab_step);
				ab_period_cnt = 0;
				motor_run_time_ab += time_per_step_ab;
				sunxi_motor_set_ab_pin_value(ab_step);
				if (GET_DIR_AB(cmd_u2k.dir) == DIR_UP) {
					device_status_k2u.y -= 1;
					if (--ab_step < 1) {
						ab_step = 8;
					}
				} else if (GET_DIR_AB(cmd_u2k.dir) == DIR_DOWN) {
					device_status_k2u.y += 1;
					if (++ab_step > 8) {
						ab_step = 1;
					}
				}
			}
		}

		if ((motor_run_time_cd < time_to_running_cd) && IS_DIR_CD(cmd_u2k.dir)) {
			cd_period_cnt++;
			if (cd_period_cnt >= time_per_step_cd) {
				MOTOR_DEBUG("dir[%d] step[%d]\r\n", GET_DIR_CD(cmd_u2k.dir), cd_step);
				cd_period_cnt = 0;
				motor_run_time_cd += time_per_step_cd;
				sunxi_motor_set_cd_pin_value(cd_step);
				if (DIR_RIGHT == GET_DIR_CD(cmd_u2k.dir)) {
					device_status_k2u.x += 1;
					if (--cd_step < 1) {
						cd_step = 8;
					}
				} else if (DIR_LEFT == GET_DIR_CD(cmd_u2k.dir)) {
					device_status_k2u.x -= 1;
					if (++cd_step > 8) {
						cd_step = 1;
					}
				}
			}
		}


		period = ktime_set(0, motor_dev->time_on_period * 1000);
		hrtimer_forward_now(&motor_hrtimer, period);
		mutex_unlock(&motor_dev->lock);
		return HRTIMER_RESTART;
	}
}

void sunxi_motor_drv_stop(u8 dir_stop)
{
	if ((dir_stop & DIR_AB_STOP) == DIR_AB_STOP) {
		MOTOR_DEBUG("dir ab stop\r\n");
		time_to_running_ab = 0;
	}

	if ((dir_stop & DIR_CD_STOP) == DIR_CD_STOP) {
		MOTOR_DEBUG("dir cd stop\r\n");
		time_to_running_cd = 0;
	}

	if ((motor_run_time_ab >= time_to_running_ab) && (motor_run_time_cd >= time_to_running_cd)) {
		MOTOR_DEBUG("dir ab & cd stop\r\n");
		device_status_k2u.status = 0;
		hrtimer_cancel(&motor_hrtimer);
		sunxi_motor_disable();
	}
}

static int sunxi_motor_do_action(void)
{
	if (IS_DIR_AB(cmd_u2k.dir)) {
		if (cmd_u2k.ab_step == 0 || cmd_u2k.ab_time_per_step == 0) {
			pr_err("sunxi_gpio_motor: ab given step (%d) or time"
			       " per step (%d) is invalid.\n",
			       cmd_u2k.ab_step, cmd_u2k.ab_time_per_step);
			return 0;
		}
	}

	if (IS_DIR_CD(cmd_u2k.dir)) {
		if (cmd_u2k.cd_step == 0 || cmd_u2k.cd_time_per_step == 0) {
			pr_err("sunxi_gpio_motor: cd given step (%d) or time"
			       " per step (%d) is invalid.\n",
			       cmd_u2k.cd_step, cmd_u2k.cd_time_per_step);
			return 0;
		}
	}

	if ((GET_DIR_AB(cmd_u2k.dir) < DIR_NONE) || (GET_DIR_AB(cmd_u2k.dir) > DIR_MAX)
			|| (GET_DIR_CD(cmd_u2k.dir) < DIR_NONE) || (GET_DIR_CD(cmd_u2k.dir) > DIR_MAX)) {
		pr_err("sunxi_gpio_motor: given dir (%d) not supported.\n",
		       cmd_u2k.dir);
		return 0;
	}

	if (!IS_DIR_AB(cmd_u2k.dir) && !IS_DIR_CD(cmd_u2k.dir)) {
		sunxi_motor_drv_stop(DIR_AB_STOP | DIR_CD_STOP);
		mdelay(5);
		return 0;
	}

	hrtimer_cancel(&motor_hrtimer);

	device_status_k2u.status = 0;
	device_status_k2u.dir	 = cmd_u2k.dir;
	motor_dev->n_que_waiting = 0;

	/* run motor */
	sunxi_motor_speed_config();
	sunxi_motor_enable();
	if (cmd_u2k.block == 1) {
		wait_event_interruptible(motor_dev->i_wait_que,
					 motor_dev->n_que_waiting);
	}
	return 0;
}

static int sunxi_motor_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sunxi_motor_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t sunxi_motor_drv_read(struct file *file, char __user *buf,
				    size_t size, loff_t *offset)
{
	int ret = 0;
	ret	= copy_to_user(buf, &device_status_k2u,
			       sizeof(sunxi_motor_device_status_t));
	if (ret) {
		pr_err("sunxi_gpio_motor: copy_to_user data err\n");
		return -EFAULT;
	}
	return sizeof(sunxi_motor_device_status_t);
}

static ssize_t sunxi_motor_drv_write(struct file *file, const char __user *buf,
				     size_t size, loff_t *offset)
{
	int err;
	err = copy_from_user(&cmd_u2k, buf, sizeof(sunxi_motor_cmd_t));
	if (err < 0) {
		pr_err("sunxi_gpio_motor: copy_from_user data err:%d !!!\n\r",
		       err);
		return -1;
	}
	return sunxi_motor_do_action();
}

static int sunxi_motor_uapate_para(void)
{
	if (UPDATE_DRV == para_u2k.update_obj & UPDATE_DRV) {
		cmd_u2k.dir = para_u2k.cmd_para.dir;
	}

	if (UPDATE_AB_STEP == para_u2k.update_obj & UPDATE_AB_STEP) {
		cmd_u2k.ab_step = para_u2k.cmd_para.ab_step;
	}

	if (UPDATE_AB_STEPTIME == para_u2k.update_obj & UPDATE_AB_STEPTIME) {
		cmd_u2k.ab_time_per_step = para_u2k.cmd_para.ab_time_per_step;
	}

	if (UPDATE_CD_STEP == para_u2k.update_obj & UPDATE_CD_STEP) {
		cmd_u2k.cd_step = para_u2k.cmd_para.cd_step;
	}

	if (UPDATE_CD_STEPTIME == para_u2k.update_obj & UPDATE_CD_STEPTIME) {
		cmd_u2k.cd_time_per_step = para_u2k.cmd_para.cd_time_per_step;
	}

	para_u2k.update_obj = 0;
	return 0;
}

static long sunxi_motor_drv_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	switch (cmd) {
	case MOTOR_DRV_SET_CMD:
		if (copy_from_user(&cmd_u2k, (void *)arg,
				   sizeof(sunxi_motor_cmd_t))) {
			return -EFAULT;
		} else {
			return sunxi_motor_do_action();
		}
	case MOTOR_DRV_GET_CMD:
		if (copy_to_user((void __user *)arg, &cmd_u2k,
				 sizeof(sunxi_motor_cmd_t))) {
			return -EFAULT;
		}
		break;
	case MOTOR_DRV_SET_STOP:
		sunxi_motor_drv_stop(DIR_AB_STOP | DIR_CD_STOP);
		break;
	case MOTOR_DRV_SET_STOP_AB:
		sunxi_motor_drv_stop(DIR_AB_STOP);
		break;
	case MOTOR_DRV_SET_STOP_CD:
		sunxi_motor_drv_stop(DIR_CD_STOP);
		break;
	case MOTOR_DRV_SET_START:
		return sunxi_motor_do_action();
	case MOTOR_DRV_SET_PARA:
		if (copy_from_user(&para_u2k, (void *)arg,
				   sizeof(sunxi_motor_para_t))) {
			return -EFAULT;
		} else {
			return sunxi_motor_uapate_para();
		}
	case MOTOR_DRV_SET_BLOCK:
		if (copy_from_user(&cmd_u2k.block, (int __user *)arg,
				   sizeof(int)))
			return -EFAULT;
		break;
	case MOTOR_DRV_GET_STATUS:
		if (copy_to_user((void __user *)arg, &device_status_k2u,
				 sizeof(sunxi_motor_device_status_t))) {
			return -EFAULT;
		}
		break;
	case MOTOR_DRV_SET_STATUS:
		if (copy_from_user(&device_status_k2u, (void __user *)arg,
				 sizeof(sunxi_motor_device_status_t))) {
			return -EFAULT;
		}
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations sunxi_motor_fops = {
	.owner		= THIS_MODULE,
	.open		= sunxi_motor_open,
	.read		= sunxi_motor_drv_read,
	.write		= sunxi_motor_drv_write,
	.unlocked_ioctl = sunxi_motor_drv_ioctl,
	.release	= sunxi_motor_close,
};

static struct miscdevice sunxi_gpio_motor_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi_gpio_motor",
	.fops  = &sunxi_motor_fops,
};

static int sunxi_motor_probe(struct platform_device *pdev)
{
	int ret		       = 0;
	struct device_node *np = pdev->dev.of_node;

	MOTOR_DEBUG("sunxi_gpio_motor: probe start.\n");

	/* request gpio for moto device */
	motor_dev->ab_pin_black =
		of_get_named_gpio_flags(np, "ab-pin-black", 0, NULL);
	if (!gpio_is_valid(motor_dev->ab_pin_black)) {
		dev_err(&pdev->dev, "could not get ab-pin-black gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->ab_pin_black,
				    GPIOF_DIR_OUT, "ab-pin-black");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request ab-pin-black gpio\n");
		return -ENOMEM;
	}

	motor_dev->ab_pin_yellow =
		of_get_named_gpio_flags(np, "ab-pin-yellow", 0, NULL);
	if (!gpio_is_valid(motor_dev->ab_pin_yellow)) {
		dev_err(&pdev->dev, "could not get ab-pin-yellow gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->ab_pin_yellow,
				    GPIOF_DIR_OUT, "ab-pin-yellow");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request ab-pin-yellow gpio\n");
		return -ENOMEM;
	}

	motor_dev->ab_pin_brown =
		of_get_named_gpio_flags(np, "ab-pin-brown", 0, NULL);
	if (!gpio_is_valid(motor_dev->ab_pin_brown)) {
		dev_err(&pdev->dev, "could not get ab-pin-yellow gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->ab_pin_brown,
				    GPIOF_DIR_OUT, "ab-pin-brown");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request ab-pin-brown gpio\n");
		return -ENOMEM;
	}

	motor_dev->ab_pin_blue =
		of_get_named_gpio_flags(np, "ab-pin-blue", 0, NULL);
	if (!gpio_is_valid(motor_dev->ab_pin_blue)) {
		dev_err(&pdev->dev, "could not get ab-pin-blue gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->ab_pin_blue,
				    GPIOF_DIR_OUT, "ab-pin-blue");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request ab-pin-blue gpio\n");
		return -ENOMEM;
	}

	motor_dev->cd_pin_black =
		of_get_named_gpio_flags(np, "cd-pin-black", 0, NULL);
	if (!gpio_is_valid(motor_dev->cd_pin_black)) {
		dev_err(&pdev->dev, "could not get cd-pin-black gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->cd_pin_black,
				    GPIOF_DIR_OUT, "cd-pin-black");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request cd-pin-black gpio\n");
		return -ENOMEM;
	}

	motor_dev->cd_pin_yellow =
		of_get_named_gpio_flags(np, "cd-pin-yellow", 0, NULL);
	if (!gpio_is_valid(motor_dev->cd_pin_yellow)) {
		dev_err(&pdev->dev, "could not get cd-pin-yellow gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->cd_pin_yellow,
				    GPIOF_DIR_OUT, "cd-pin-yellow");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request cd-pin-yellow gpio\n");
		return -ENOMEM;
	}

	motor_dev->cd_pin_brown =
		of_get_named_gpio_flags(np, "cd-pin-brown", 0, NULL);
	if (!gpio_is_valid(motor_dev->cd_pin_brown)) {
		dev_err(&pdev->dev, "could not get cd-pin-yellow gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->cd_pin_brown,
				    GPIOF_DIR_OUT, "cd-pin-brown");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request cd-pin-brown gpio\n");
		return -ENOMEM;
	}

	motor_dev->cd_pin_blue =
		of_get_named_gpio_flags(np, "cd-pin-blue", 0, NULL);
	if (!gpio_is_valid(motor_dev->cd_pin_blue)) {
		dev_err(&pdev->dev, "could not get cd-pin-blue gpio\n");
		return -ENOMEM;
	}

	ret = devm_gpio_request_one(&pdev->dev, motor_dev->cd_pin_blue,
				    GPIOF_DIR_OUT, "cd-pin-blue");
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request cd-pin-blue gpio\n");
		return -ENOMEM;
	}

	motor_dev->time_on_period = DEFAULT_TIME_ON_PERIOD;

	/* register misc device */
	ret = misc_register(&sunxi_gpio_motor_miscdev);
	if (0 != ret) {
		pr_err("sunxi_gpio_motor: register motor device failed!\n");
		return -ENOMEM;
	}
	mutex_init(&motor_dev->lock);

	/* init gpio default status and disable */
	sunxi_motor_disable();

	/* init hrtimer */
	hrtimer_init(&motor_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	motor_hrtimer.function = sunxi_motor_timer_fn;

	device_status_k2u.x   = 0;
	device_status_k2u.y   = 0;
	device_status_k2u.dir = DIR_NONE;

	MOTOR_DEBUG("sunxi_gpio_motor: probe done.\n");

	return 0;
}

static int sunxi_motor_remove(struct platform_device *pdev)
{
	/* deinit timer and deregister device */
	hrtimer_cancel(&motor_hrtimer);
	misc_deregister(&sunxi_gpio_motor_miscdev);
	return 0;
}

static const struct of_device_id sunxi_gpio_motor[] = {
	{ .compatible = "allwinner,gpio-motor" },
	{ /* Sentinel */ },
};

static struct platform_driver motor_driver = {
	.probe = sunxi_motor_probe,
	.remove = sunxi_motor_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = sunxi_gpio_motor,
	},
};

static int __init sunxi_gpio_motor_init(void)
{
	int ret = 0;

	/* init data */
	motor_run_time_ab	 = 0;
	motor_run_time_cd	 = 0;
	time_to_running_ab	 = 0;
	time_to_running_cd	 = 0;
	time_per_step_ab = 0;
	time_per_step_cd = 0;

	if (motor_dev)
		kfree(motor_dev);

	/* init sunxi gpio motor */
	motor_dev = kzalloc(sizeof(sunxi_motor_dev_t), GFP_KERNEL);
	memset(motor_dev, 0, sizeof(sunxi_motor_dev_t));
	if (!motor_dev)
		return -ENOMEM;

	ret			 = platform_driver_register(&motor_driver);
	motor_dev->n_que_waiting = 0;

	init_waitqueue_head(&motor_dev->i_wait_que);
	mutex_init(&motor_dev->lock);

	pr_info("sunxi_gpio_motor: init success.\n");
	return ret;
}

static void __exit sunxi_gpio_motor_exit(void)
{
	platform_driver_unregister(&motor_driver);
	kfree(motor_dev);
	return;
}

module_init(sunxi_gpio_motor_init);
module_exit(sunxi_gpio_motor_exit);

MODULE_AUTHOR("zhangyuanjing<zhangyuanjing@allwinnertech.com>");
MODULE_DESCRIPTION("allwinner gpio motor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
