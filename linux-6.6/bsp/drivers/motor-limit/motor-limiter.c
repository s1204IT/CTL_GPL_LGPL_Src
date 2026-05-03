// SPDX-License-Identifier: GPL-2.0-only
/*
 * motor-limiter driver
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

#define MOTOR_LIMITER_VERSION		"1.0.1"
#define MOTOR_LIMITER_MAX_MINORS	8

struct motor_limiter {
	struct platform_device *pdev;
	struct device *dev;
	struct cdev cdev;
	struct fasync_struct *async;
	atomic_t fcnt;

	int gpio;
	int irq;
	int status;

	spinlock_t lock;
};

static struct class *motor_limiter_class;
static int motor_limiter_major;
static int motor_limiter_minors[MOTOR_LIMITER_MAX_MINORS];

static int motor_limiter_open(struct inode *inode, struct file *file)
{
	struct motor_limiter *limiter = container_of(inode->i_cdev, struct motor_limiter, cdev);

	file->private_data = limiter;
	atomic_inc(&limiter->fcnt);
	enable_irq(limiter->irq);

	dev_dbg(limiter->dev, "file open cnt %d\n", atomic_read(&limiter->fcnt));

	return 0;
}

static int motor_limiter_release(struct inode *inode, struct file *file)
{
	struct motor_limiter *limiter = container_of(inode->i_cdev, struct motor_limiter, cdev);

	file->private_data = NULL;
	if (limiter->async)
		limiter->async->fa_file = NULL;

	if (atomic_dec_and_test(&limiter->fcnt))
		disable_irq(limiter->irq);

	dev_dbg(limiter->dev, "file release cnt %d\n", atomic_read(&limiter->fcnt));

	return 0;
}

ssize_t motor_limiter_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct motor_limiter *limiter = file->private_data;
	unsigned long flags = 0;

	spin_lock_irqsave(&limiter->lock, flags);
	put_user(limiter->status, (unsigned int __user *)buf);
	spin_unlock_irqrestore(&limiter->lock, flags);

	return sizeof(limiter->status);
}

int motor_limiter_fasync(int fd, struct file *filp, int on)
{
	struct motor_limiter *limiter = filp->private_data;

	return fasync_helper(fd, filp, on, &limiter->async);
}

static const struct file_operations motor_limiter_file_ops = {
	.open = motor_limiter_open,
	.release = motor_limiter_release,
	.read = motor_limiter_read,
	.fasync = motor_limiter_fasync,
};

static ssize_t motor_limiter_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct motor_limiter *limiter = (struct motor_limiter *)platform_get_drvdata(pdev);

	return sprintf(buf, "%d\n", gpio_get_value(limiter->gpio));
}

static struct device_attribute motor_limiter_attr = {
	.attr = {
		.name = "motor_limiter",
		.mode = 0444,
	},
	.show = motor_limiter_show,
	.store = NULL,
};

static irqreturn_t motor_limiter_handler(int irq, void *dev_id)
{
	struct motor_limiter *limiter = (struct motor_limiter *)dev_id;
	unsigned long flags = 0;

	spin_lock_irqsave(&limiter->lock, flags);
	limiter->status = gpio_get_value(limiter->gpio);
	spin_unlock_irqrestore(&limiter->lock, flags);

	if (limiter->async && limiter->async->fa_file)
		kill_fasync(&limiter->async, SIGIO, POLL_IN);

	return IRQ_HANDLED;
}

static int motor_limiter_fdt_parse(struct motor_limiter *limiter, struct device_node *np)
{
	limiter->gpio = of_get_named_gpio(np, "limiter-gpio", 0);
	if (limiter->gpio < 0) {
		dev_err(limiter->dev, "failed to get property limiter-gpio\n");
		return -EINVAL;
	}
	limiter->irq = gpio_to_irq(limiter->gpio);
	return 0;
}

static unsigned int motor_limiter_get_minors(struct motor_limiter *limiter)
{
	unsigned int i;

	spin_lock(&limiter->lock);

	for (i = 0; i < MOTOR_LIMITER_MAX_MINORS; i++) {
		if (motor_limiter_minors[i] == 0) {
			motor_limiter_minors[i] = 1;
			break;
		}
	}

	spin_unlock(&limiter->lock);

	return i;
}

static void motor_limiter_put_minors(struct motor_limiter *limiter, unsigned int minor)
{
	spin_lock(&limiter->lock);

	motor_limiter_minors[minor] = 0;

	spin_unlock(&limiter->lock);
}

static int motor_limiter_probe(struct platform_device *pdev)
{
	struct motor_limiter *limiter = NULL;
	int minor;
	int err;

	limiter = devm_kzalloc(&pdev->dev, sizeof(*limiter), GFP_KERNEL);
	if (IS_ERR_OR_NULL(limiter))
		return -ENOMEM;  /* Do not print prompts after kzalloc errors */

	limiter->pdev = pdev;
	limiter->dev = &pdev->dev;

	err = motor_limiter_fdt_parse(limiter, pdev->dev.of_node);
	if (err < 0) {
		dev_err(limiter->dev, "failed to get fdt resource\n");
		return err;
	}

	platform_set_drvdata(pdev, limiter);

	spin_lock_init(&limiter->lock);
	atomic_set(&limiter->fcnt, 0);

	err = devm_request_irq(limiter->dev, limiter->irq, motor_limiter_handler,
							IRQ_TYPE_EDGE_BOTH, dev_name(limiter->dev),
							limiter);
	if (err) {
		dev_err(limiter->dev, "Cannot request limiter IRQ\n");
		return err;
	}
	disable_irq(limiter->irq);

	cdev_init(&limiter->cdev, &motor_limiter_file_ops);
	limiter->cdev.owner = THIS_MODULE;

	minor = motor_limiter_get_minors(limiter);

	err = cdev_add(&limiter->cdev, MKDEV(motor_limiter_major, minor), 1);
	if (err) {
		dev_err(limiter->dev, "chardev registration failed\n");
		goto err0;
	}

	if (IS_ERR(device_create(motor_limiter_class, limiter->dev,
							MKDEV(motor_limiter_major, minor), limiter,
							dev_name(limiter->dev)))) {
		dev_err(limiter->dev, "can't create device\n");
		goto err1;
	}

	err = device_create_file(limiter->dev, &motor_limiter_attr);
	if (err < 0) {
		dev_err(limiter->dev, "failed to create device file motor_limiter\n");
		goto err2;
	}

	dev_info(limiter->dev, "probe success\n");
	return 0;

err2:
	device_destroy(motor_limiter_class, MKDEV(motor_limiter_major, minor));
err1:
	cdev_del(&limiter->cdev);
err0:
	motor_limiter_put_minors(limiter, minor);
	return err;
}

static int motor_limiter_remove(struct platform_device *pdev)
{
	struct motor_limiter *limiter = (struct motor_limiter *)platform_get_drvdata(pdev);
	unsigned int minor = MINOR(limiter->cdev.dev);

	device_remove_file(limiter->dev, &motor_limiter_attr);

	device_destroy(motor_limiter_class, MKDEV(motor_limiter_major, minor));

	cdev_del(&limiter->cdev);

	motor_limiter_put_minors(limiter, minor);

	return 0;
}

static const struct of_device_id of_motor_limiter_match[] = {
	{ .compatible = "motor-limiter", },
	{},
};

static struct platform_driver motor_limiter_driver = {
	.probe = motor_limiter_probe,
	.remove = motor_limiter_remove,
	.driver = {
		.name = "motor-limiter",
		.of_match_table = of_motor_limiter_match,
	},
};

static int __init motor_limiter_init(void)
{
	int ret;
	dev_t dev;

	motor_limiter_class = class_create("motor-limiter");
	if (IS_ERR(motor_limiter_class)) {
		ret = PTR_ERR(motor_limiter_class);
		pr_err("motor-limiter: can't register motor-limiter class\n");
		goto err;
	}

	ret = alloc_chrdev_region(&dev, 0, MOTOR_LIMITER_MAX_MINORS, "motor-limiter");
	if (ret) {
		pr_err("motor-limiter: can't register character device\n");
		goto err_class;
	}
	motor_limiter_major = MAJOR(dev);

	ret = platform_driver_register(&motor_limiter_driver);
	if (ret) {
		pr_err("motor-limiter: can't register platfrom driver\n");
		goto err_unchr;
	}

	pr_info("motor limiter linux driver init ok (Version %s)\n", MOTOR_LIMITER_VERSION);

	return 0;

err_unchr:
	unregister_chrdev_region(dev, MOTOR_LIMITER_MAX_MINORS);
err_class:
	class_destroy(motor_limiter_class);
err:
	return ret;
}

static void __exit motor_limiter_exit(void)
{
	platform_driver_unregister(&motor_limiter_driver);

	unregister_chrdev_region(MKDEV(motor_limiter_major, 0), MOTOR_LIMITER_MAX_MINORS);

	class_destroy(motor_limiter_class);
}

module_init(motor_limiter_init)
module_exit(motor_limiter_exit)

MODULE_AUTHOR("AWA2252");
MODULE_DESCRIPTION("Motor Limiter Driver");
MODULE_VERSION(MOTOR_LIMITER_VERSION);
MODULE_LICENSE("GPL");
