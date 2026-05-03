/*
 *  stk342x.c - Linux kernel modules for sensortek stk301x, stk321x and stk331x
 *  proximity/ambient light sensor
 *
 *  Copyright (C) 2012~2013 Lex Hsieh / sensortek <lex_hsieh@sensortek.com.tw>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "../../init-input.h"
#include "stk3420.h"
#include "stk_ges_test.h"

#include <linux/regulator/consumer.h>
#include <asm/uaccess.h>

#include <linux/pinctrl/consumer.h>

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif

#define DRIVER_VERSION  "1.0.1"


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
#define CONST_TYPE_PREFIX const
#else
#define CONST_TYPE_PREFIX
#endif

#define STK_ALS_ENABLE
#define STK_PS_ENABLE

#define STK_POLL_ALS
#define STK_POLL_PS
#define STK_POLL_GS

/* misc define */
#define MIN_ALS_POLL_DELAY_NS	20000000

#define DEVICE_NAME     "stk342x"
#define ALS_NAME        "stk342x_ls"
#define PS_NAME         "stk342x_ps"
#define GS_NAME         "stk342x_gs"

#define KEY_GESTURE_UP                          KEY_F15
#define KEY_GESTURE_DOWN                        KEY_F16
#define KEY_GESTURE_LEFT                        KEY_F17
#define KEY_GESTURE_RIGHT                       KEY_F18

static int sProbeSuccess;

struct stk342x_data {
    struct i2c_client *client;
    struct mutex io_lock;
    struct input_dev *als_input_dev;
    struct input_dev *ps_input_dev;
    struct input_dev *gs_input_dev;

#ifdef STK_ALS_ENABLE
    bool als_enabled;
    bool re_enable_als;
    int32_t als_lux_last;
    uint32_t als_transmittance;
#endif
#ifdef STK_PS_ENABLE
	bool ps_enabled;
    bool re_enabled_ps;
	int32_t ps_distance_last;
	int32_t ps_near_state_last;
	uint16_t ps_thd_h;
	uint16_t ps_thd_l;
#endif
    bool gs_enabled;
    bool re_enabled_gs;

#ifdef STK_POLL_ALS
    struct work_struct stk_als_work;
	struct hrtimer als_timer;
	struct workqueue_struct *stk_als_wq;
	ktime_t als_poll_delay;
    bool is_als_timer_active;
#endif
	/*struct wake_lock ps_wakelock;*/
#ifdef STK_POLL_PS
	struct hrtimer ps_timer;
	struct work_struct stk_ps_work;
	struct workqueue_struct *stk_ps_wq;
    ktime_t ps_poll_delay;
    bool is_ps_timer_active;
	/*struct wake_lock ps_nosuspend_wl;*/
#endif
    /*struct wake_lock gs_wakelock;*/
#ifdef STK_POLL_GS
    struct hrtimer gs_timer;
    struct work_struct stk_gs_work;
    struct workqueue_struct *stk_gs_wq;
    bool is_gs_timer_active;
    ktime_t gs_poll_delay;
    /*struct wake_lock gs_nosuspend_wl;*/
#endif
    bool first_boot;
    atomic_t recv_reg;
    stk_ges Ges;
    int data_cnt;
};

static u32 debug_mask;
static struct sensor_config_info ls_sensor_info = {
    .input_type = LS_TYPE,
    .int_number = 0,
    .ldo = NULL,
};

static struct stk342x_data *gdata;

static int startup(void);
static int32_t stk342x_enable_gs(struct stk342x_data *pdata, uint8_t enable);

static const unsigned short normal_i2c[2] = {0x58, I2C_CLIENT_END};

static int stk_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;
	if (ls_sensor_info.twi_id == adapter->nr) {
		printk("%s: ===========addr= %x\n", __func__, client->addr);
		strlcpy(info->type, DEVICE_NAME, I2C_NAME_SIZE);
		return 0;
	} else {
		return -ENODEV;
	}
}

static int stk342x_i2c_read_data(struct i2c_client *client, unsigned char command, int length, unsigned char *values)
{
	uint8_t retry;
	int err;

	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = values,
		},
	};

	for (retry = 0; retry < 5; retry++) {
		err = i2c_transfer(client->adapter, msgs, 2);
		if (err == 2)
			break;
		else
			mdelay(5);
	}

	if (retry >= 5) {
		printk(KERN_ERR "%s: i2c read fail, err=%d\n", __func__, err);
		return -EIO;
	}
	return 0;
}

static int stk342x_i2c_write_data(struct i2c_client *client, unsigned char command, int length, unsigned char *values)
{
	int retry;
	int err;
	unsigned char data[11];
	struct i2c_msg msg;
	int index;

	if (!client)
		return -EINVAL;
	else if (length >= 10) {
		printk(KERN_ERR "%s:length %d exceeds 10\n", __func__, length);
		return -EINVAL;
    }

	data[0] = command;
	for (index = 1; index <= length; index++)
		data[index] = values[index - 1];

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = length+1;
	msg.buf = data;

	for (retry = 0; retry < 5; retry++) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			break;
		else
			mdelay(5);
	}

	if (retry >= 5) {
		printk(KERN_ERR "%s: i2c write fail, err=%d\n", __func__, err);
		return -EIO;
	}
	return 0;
}

static int stk342x_i2c_smbus_read_byte_data(struct i2c_client *client, unsigned char command)
{
	unsigned char value;
	int err;
	err = stk342x_i2c_read_data(client, command, 1, &value);
	if (err < 0)
		return err;
	return value;
}

static int stk342x_i2c_smbus_write_byte_data(struct i2c_client *client, unsigned char command, unsigned char value)
{
	int err;
	err = stk342x_i2c_write_data(client, command, 1, &value);
	return err;
}

static int32_t stk342x_init_all_reg(struct stk342x_data *pdata)
{
	int32_t ret;

    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x01, 0x03);
    if (ret < 0) {
        printk(KERN_ERR "%s: write i2c error\n", __func__);
        return ret;
    }

    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x19, 0x50);
    if (ret < 0) {
        printk(KERN_ERR "%s: write i2c error\n", __func__);
        return ret;
    }

    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x1B, 0x00);
    if (ret < 0) {
        printk(KERN_ERR "%s: write i2c error\n", __func__);
        return ret;
    }

	ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x1d, 0xf3);
	if (ret < 0) {
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
    }
    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x05, 0x6);

	if (ret < 0) {
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
    }
    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x01, 0x3);

	if (ret < 0) {
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
    }

	ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x95, 0x90);

	if (ret < 0) {
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
    }

	ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x03, 0xc0);

	if (ret < 0) {
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
    }

    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x94, 0x35);
    if (ret < 0) {
        printk(KERN_ERR "%s: write i2c error\n", __func__);
        return ret;
    }
    
    ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x02, 0x02);
    if (ret < 0) {
        printk(KERN_ERR "%s: write i2c error\n", __func__);
        return ret;
    }

    return 0;
}

static int32_t stk342x_check_pid(struct stk342x_data *pdata)
{
    unsigned char value;
    int err;

    err = stk342x_i2c_read_data(pdata->client, 0x3E, 1, &value);

    if (err < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, err);
        return err;
    }

    printk(KERN_INFO "%s: PID=0x%x\n", __func__, value);

    switch (value) {
    case 0x50:
        return 0;
    case 0x00:
        printk(KERN_ERR "PID=0x0, please make sure the chip is stk342x!\n");
        return -2;
    default:
        printk(KERN_ERR "%s: invalid PID(%#x)\n", __func__, value);
        return -1;
    }

    return 0;
}

static int32_t stk342x_software_reset(struct stk342x_data *pdata)
{
    int32_t r;

    r = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x80, 0x00);
    if (r < 0) {
        printk(KERN_ERR "%s: software reset: read error after reset\n", __func__);
        return r;
    }
    mdelay(15);
    return 0;
}

#ifdef STK_ALS_ENABLE
inline uint32_t stk_alscode2lux(struct stk342x_data *ps_data, uint32_t alscode)
{
    alscode += ((alscode << 7) + (alscode << 3) + (alscode >> 1));
    alscode <<= 3;
    alscode /= ps_data->als_transmittance;
    return alscode;
}

uint32_t stk_lux2alscode(struct stk342x_data *ps_data, uint32_t lux)
{
    lux *= ps_data->als_transmittance;
    lux /= 1100;
    if (unlikely(lux >= (1 << 16)))
        lux = (1 << 16) - 1;

    return lux;
}

static unsigned short stk342x_get_als_reading(struct stk342x_data *als_data)
{
    unsigned char value[2];
    int err;
    err = stk342x_i2c_read_data(als_data->client, 0x13, 2, &value[0]);
    if (err < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, err);
        return err;
    }
    return (value[0] << 8) | value[1];
}

static int32_t stk342x_enable_als(struct stk342x_data *als_data, uint8_t enable)
{
    int32_t ret;
    unsigned char value = 0;
    
    if (als_data->als_enabled == enable)
        return 0;

    if (als_data->first_boot == true) {
        als_data->first_boot = false;
    }

    ret = stk342x_i2c_read_data(als_data->client, 0x00, 1, &value);
    if (ret < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, ret);
        return ret;
    }
    
    value &= (~(0x02));
    
    if (enable) {
        value |= 0x02;
        ret = stk342x_i2c_smbus_write_byte_data(als_data->client, 0x00, value);
        if (ret < 0) {
            printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
            return ret;
        }
#ifdef STK_POLL_GS
        if (!als_data->is_als_timer_active)
        {
            hrtimer_start(&als_data->als_timer, als_data->als_poll_delay, HRTIMER_MODE_REL);
            als_data->is_als_timer_active = true;
        }
#endif
        als_data->als_enabled = true;
    }
    else
    {
        value |= 0x04;
        ret = stk342x_i2c_smbus_write_byte_data(als_data->client, 0x00, value);
        if (ret < 0) {
            printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
            return ret;
        }

        if (als_data->is_als_timer_active)
        {
            hrtimer_cancel(&als_data->als_timer);
            cancel_work_sync(&als_data->stk_als_work);
			flush_workqueue(als_data->stk_als_wq);
            als_data->is_als_timer_active = false;
        }

        als_data->als_enabled = false;
    }
    return ret;
}

static ssize_t stk_als_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk342x_data *als_data =  dev_get_drvdata(dev);
    int32_t reading;

    reading = stk342x_get_als_reading(als_data);
    return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}


static ssize_t stk_als_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk342x_data *als_data =  dev_get_drvdata(dev);
    int32_t enable, ret;

    mutex_lock(&als_data->io_lock);
	enable = (als_data->als_enabled) ? 1 : 0;
    mutex_unlock(&als_data->io_lock);
    ret = stk342x_i2c_smbus_read_byte_data(als_data->client, 0x00);
    ret = (ret & 0x02) ? 1 : 0;

    if (enable != ret)
        printk(KERN_ERR "%s: driver and sensor mismatch! driver_enable=0x%x, sensor_enable=%x\n", __func__, enable, ret);

    return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_als_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk342x_data *als_data = dev_get_drvdata(dev);
    uint8_t en;
    if (sysfs_streq(buf, "1"))
        en = 1;
    else if (sysfs_streq(buf, "0"))
        en = 0;
    else {
        printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
        return -EINVAL;
    }

    printk(KERN_INFO "%s: Enable ALS : %d\n", __func__, en);
    mutex_lock(&als_data->io_lock);
    stk342x_enable_als(als_data, en);
    mutex_unlock(&als_data->io_lock);
    return size;
}

static ssize_t stk_als_lux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk342x_data *als_data = dev_get_drvdata(dev);
    int32_t als_reading;
    uint32_t als_lux;
    als_reading = stk342x_get_als_reading(als_data);
    als_lux = stk_alscode2lux(als_data, als_reading);
    return scnprintf(buf, PAGE_SIZE, "%d lux\n", als_lux);
}

static ssize_t stk_als_lux_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk342x_data *als_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 16, &value);
    if (ret < 0) {
        printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    als_data->als_lux_last = value;
    input_report_abs(als_data->als_input_dev, ABS_MISC, value);
    input_sync(als_data->als_input_dev);
    printk(KERN_INFO "%s: als input event %ld lux\n", __func__, value);

    return size;
}

static ssize_t stk_als_transmittance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk342x_data *als_data =  dev_get_drvdata(dev);
    int32_t transmittance;
    transmittance = als_data->als_transmittance;
    return scnprintf(buf, PAGE_SIZE, "%d\n", transmittance);
}


static ssize_t stk_als_transmittance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk342x_data *als_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 10, &value);
    if (ret < 0) {
        printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    als_data->als_transmittance = value;
    return size;
}

static ssize_t stk_als_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk342x_data *als_data =  dev_get_drvdata(dev);
    int64_t delay;
    mutex_lock(&als_data->io_lock);
    delay = ktime_to_ms(als_data->als_poll_delay);
    mutex_unlock(&als_data->io_lock);
    return scnprintf(buf, PAGE_SIZE, "%lld\n", delay);
}

static ssize_t stk_als_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    uint64_t value = 0;
    int ret;
    struct stk342x_data *als_data =  dev_get_drvdata(dev);
    ret = kstrtoull(buf, 10, &value);
    if (ret < 0) {
        printk(KERN_ERR "%s:kstrtoull failed, ret=0x%x\n", __func__, ret);
        return ret;
    }

    value = value * 1000 * 1000;
    if (value < MIN_ALS_POLL_DELAY_NS) {
        printk(KERN_ERR "%s: delay is too small\n", __func__);
        value = MIN_ALS_POLL_DELAY_NS;
    }
    mutex_lock(&als_data->io_lock);

    if (value != ktime_to_ns(als_data->als_poll_delay))
        als_data->als_poll_delay = ns_to_ktime(value);

    mutex_unlock(&als_data->io_lock);
    return size;
}
#endif

#ifdef STK_PS_ENABLE
static int32_t stk342x_set_ps_thd_l(struct stk342x_data *ps_data, uint16_t thd_l)
{
	unsigned char val[2];
	int ret;
	val[0] = (thd_l & 0xFF00) >> 8;
	val[1] = thd_l & 0x00FF;
	ret = stk342x_i2c_write_data(ps_data->client, 0x08, 2, val);
	if (ret < 0)
		printk(KERN_ERR "%s: fail, ret=%d\n", __func__, ret);
	return ret;
}

static int32_t stk342x_set_ps_thd_h(struct stk342x_data *ps_data, uint16_t thd_h)
{
	unsigned char val[2];
	int ret;
	val[0] = (thd_h & 0xFF00) >> 8;
	val[1] = thd_h & 0x00FF;
	ret = stk342x_i2c_write_data(ps_data->client, 0x06, 2, val);
	if (ret < 0)
		printk(KERN_ERR "%s: fail, ret=%d\n", __func__, ret);
	return ret;
}

static unsigned short stk342x_get_ps_reading(struct stk342x_data *ps_data)
{
    unsigned char value[2];
    int err;
    err = stk342x_i2c_read_data(ps_data->client, 0x11, 2, &value[0]);
    if (err < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, err);
        return err;
    }
    return (value[0] << 8) | value[1];
}

static int32_t stk342x_enable_ps(struct stk342x_data *ps_data, uint8_t enable)
{
    int32_t ret;
    unsigned char value = 0;

    if (ps_data->ps_enabled == enable)
        return 0;

    if (ps_data->first_boot == true)
    {
        ps_data->first_boot = false;
    }

    ret = stk342x_i2c_read_data(ps_data->client, 0x00, 1, &value);
    if (ret < 0)
    {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, ret);
        return ret;
    }

    value &= (~(0x01));
    
    if (enable)
    {
		if (ps_data->ps_enabled)
        {
            value |= 0x01;
        }
        else
        {
            value |= (0x01 | 0x04);
        }

        ret = stk342x_i2c_smbus_write_byte_data(ps_data->client, 0x00, value);
        if (ret < 0)
        {
            return ret;
        }
#ifdef STK_POLL_PS
        if (!ps_data->is_ps_timer_active)
        {
            hrtimer_start(&ps_data->ps_timer, ps_data->ps_poll_delay, HRTIMER_MODE_REL);
            ps_data->is_ps_timer_active = true;
        }
#endif
        ps_data->ps_enabled = true;
    }
    else
    {
        ret = stk342x_i2c_smbus_write_byte_data(ps_data->client, 0x00, value);
        if (ret < 0)
        {
            printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
            return ret;
        }

        if (ps_data->is_ps_timer_active)
        {
            hrtimer_cancel(&ps_data->ps_timer);
            cancel_work_sync(&ps_data->stk_ps_work);
			flush_workqueue(ps_data->stk_ps_wq);
            ps_data->is_ps_timer_active = false;
        }

        ps_data->ps_enabled = false;
    }
    return ret;
}

static ssize_t stk_ps_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
    uint32_t reading;
    reading = stk342x_get_ps_reading(ps_data);
    return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}

static ssize_t stk_ps_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t enable, ret;
	struct stk342x_data *ps_data = dev_get_drvdata(dev);

    mutex_lock(&ps_data->io_lock);
	enable = (ps_data->ps_enabled) ? 1 : 0;
    mutex_unlock(&ps_data->io_lock);
    ret = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x00);
    ret = (ret & 0x01) ? 1 : 0;

	if (enable != ret)
		printk(KERN_ERR "%s: driver and sensor mismatch! driver_enable=0x%x, sensor_enable=%x\n", __func__, enable, ret);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_ps_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	uint8_t en;
	if (sysfs_streq(buf, "1"))
		en = 1;
	else if (sysfs_streq(buf, "0"))
		en = 0;
	else {
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
    printk(KERN_INFO "%s: Enable PS : %d\n", __func__, en);
    mutex_lock(&ps_data->io_lock);
    stk342x_enable_ps(ps_data, en);
    mutex_unlock(&ps_data->io_lock);
    return size;
}

static ssize_t stk_ps_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	int64_t delay;
	mutex_lock(&ps_data->io_lock);
	delay = ktime_to_ms(ps_data->ps_poll_delay);
	mutex_unlock(&ps_data->io_lock);
	return scnprintf(buf, PAGE_SIZE, "%lld\n", delay);
}

static ssize_t stk_ps_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	uint64_t value = 0;
	int ret;
	struct stk342x_data *ps_data = dev_get_drvdata(dev);

	ret = kstrtoull(buf, 10, &value);
	if (ret < 0)
    {
		printk(KERN_ERR "%s:kstrtoull failed, ret=0x%x\n", __func__, ret);
		return ret;
	}

    value = value * 1000 * 1000;
	if (value < MIN_ALS_POLL_DELAY_NS)
    {
        printk(KERN_ERR "%s: delay is too small\n", __func__);
		value = MIN_ALS_POLL_DELAY_NS;
	}
	mutex_lock(&ps_data->io_lock);
	if (value != ktime_to_ns(ps_data->ps_poll_delay))
    {
		ps_data->ps_poll_delay = ns_to_ktime(value);
    }
	mutex_unlock(&ps_data->io_lock);
	return size;
}

static ssize_t stk_ps_distance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	int32_t dist = 1, ret;
	ret = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x10);

	if (ret < 0)
		return ret;

	dist = (ret & 0x01) ? 1 : 0;
	ps_data->ps_distance_last = dist;

	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, dist);
	input_sync(ps_data->ps_input_dev);
	/*support wake lock for ps*/
	/*wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);*/
	printk(KERN_INFO "%s: ps input event %d cm\n", __func__, dist);
	return scnprintf(buf, PAGE_SIZE, "%d\n", dist);
}

static ssize_t stk_ps_distance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = kstrtoul(buf, 10, &value);
	if (ret < 0) {
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	ps_data->ps_distance_last = value;
	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, value);
	input_sync(ps_data->ps_input_dev);
	/*support wake lock for ps*/
	/*wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);*/
	printk(KERN_INFO "%s: ps input event %ld cm\n", __func__, value);
	return size;
}

static ssize_t stk_ps_code_thd_l_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t ps_thd_l1_reg, ps_thd_l2_reg;
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	ps_thd_l1_reg = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x08);
	if (ps_thd_l1_reg < 0) {
		printk(KERN_ERR "%s fail, err=0x%x", __func__, ps_thd_l1_reg);
		return -EINVAL;
	}
	ps_thd_l2_reg = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x09);
	if (ps_thd_l2_reg < 0) {
		printk(KERN_ERR "%s fail, err=0x%x", __func__, ps_thd_l2_reg);
		return -EINVAL;
	}
	ps_thd_l1_reg = ps_thd_l1_reg << 8 | ps_thd_l2_reg;
	return scnprintf(buf, PAGE_SIZE, "%d\n", ps_thd_l1_reg);
}

static ssize_t stk_ps_code_thd_l_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = kstrtoul(buf, 10, &value);
	if (ret < 0) {
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	stk342x_set_ps_thd_l(ps_data, value);
	return size;
}

static ssize_t stk_ps_code_thd_h_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t ps_thd_h1_reg, ps_thd_h2_reg;
	struct stk342x_data *ps_data =  dev_get_drvdata(dev);
	ps_thd_h1_reg = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x06);
	if (ps_thd_h1_reg < 0) {
		printk(KERN_ERR "%s fail, err=0x%x", __func__, ps_thd_h1_reg);
		return -EINVAL;
	}
	ps_thd_h2_reg = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x07);
	if (ps_thd_h2_reg < 0) {
		printk(KERN_ERR "%s fail, err=0x%x", __func__, ps_thd_h2_reg);
		return -EINVAL;
	}
	ps_thd_h1_reg = ps_thd_h1_reg << 8 | ps_thd_h2_reg;
	return scnprintf(buf, PAGE_SIZE, "%d\n", ps_thd_h1_reg);
}

static ssize_t stk_ps_code_thd_h_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk342x_data *ps_data = dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = kstrtoul(buf, 10, &value);
	if (ret < 0) {
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	stk342x_set_ps_thd_h(ps_data, value);
	return size;
}
#endif

static unsigned short stk342x_get_gs_reading(struct stk342x_data *pdata)
{
    unsigned char value[2];
    int err;
    err = stk342x_i2c_read_data(pdata->client, 0x20, 2, &value[0]);
    if (err < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, err);
        return err;
    }
    return (value[0] << 8) | value[1];
}

static int32_t stk342x_enable_gs(struct stk342x_data *pdata, uint8_t enable)
{
    int32_t ret;
    unsigned char value = 0;
    
    if (pdata->gs_enabled == enable)
        return 0;

    if (pdata->first_boot == true) {
        pdata->first_boot = false;
    }

    ret = stk342x_i2c_read_data(pdata->client, 0x00, 1, &value);
    if (ret < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, ret);
        return ret;
    }

    value &= (~(0x01));

    if (enable) {
		if (pdata->gs_enabled)
        {
            value |= 0x01;
        }
        else
        {
            value |= (0x01 | 0x04);
        }

        ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x00, value);
        if (ret < 0) {
            printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
            return ret;
        }
#ifdef STK_POLL_GS
        if (!pdata->is_gs_timer_active)
        {
            hrtimer_start(&pdata->gs_timer, pdata->gs_poll_delay, HRTIMER_MODE_REL);
            pdata->is_gs_timer_active = true;
        }
#endif
        pdata->gs_enabled = true;
    }
    else
    {
        ret = stk342x_i2c_smbus_write_byte_data(pdata->client, 0x00, 0x00);
        if (ret < 0) {
            printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
            return ret;
        }

        if (pdata->is_gs_timer_active)
        {
            hrtimer_cancel(&pdata->gs_timer);
            cancel_work_sync(&pdata->stk_gs_work);
			flush_workqueue(pdata->stk_gs_wq);
            pdata->is_gs_timer_active = false;
        }

        pdata->gs_enabled = false;
    }
    return ret;
}

static ssize_t stk_gs_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk342x_data *pdata =  dev_get_drvdata(dev);
    uint32_t reading;
    reading = stk342x_get_gs_reading(pdata);
    return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}

static ssize_t stk_gs_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t enable, ret;
    struct stk342x_data *pdata =  dev_get_drvdata(dev);

    mutex_lock(&pdata->io_lock);
    enable = (pdata->gs_enabled) ? 1 : 0;
    mutex_unlock(&pdata->io_lock);
    ret = stk342x_i2c_smbus_read_byte_data(pdata->client, 0x00);

    return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_gs_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk342x_data *pdata =  dev_get_drvdata(dev);
    uint8_t en;

    if (sysfs_streq(buf, "1"))
    {
        en = 1;
    }
    else if (sysfs_streq(buf, "0"))
    {
        en = 0;
    }
    else 
    {
        printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
        return -EINVAL;
    }
    
    printk(KERN_INFO "%s: Enable GS : %d\n", __func__, en);
    mutex_lock(&pdata->io_lock);
    stk342x_enable_gs(pdata, en);
    mutex_unlock(&pdata->io_lock);
    return size;
}

static ssize_t stk_gs_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk342x_data *pdata =  dev_get_drvdata(dev);
    int64_t delay;
    mutex_lock(&pdata->io_lock);
    delay = ktime_to_ms(pdata->gs_poll_delay);
    mutex_unlock(&pdata->io_lock);
    return scnprintf(buf, PAGE_SIZE, "%lld\n", delay);
}

static ssize_t stk_gs_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    uint64_t value = 0;
    int ret;
    struct stk342x_data *pdata =  dev_get_drvdata(dev);
    ret = kstrtoull(buf, 10, &value);
    if (ret < 0) {
        printk(KERN_ERR "%s:kstrtoull failed, ret=0x%x\n", __func__, ret);
        return ret;
    }

    value = value * 1000 * 1000;
    if (value < MIN_ALS_POLL_DELAY_NS) {
        printk(KERN_ERR "%s: delay is too small\n", __func__);
        value = MIN_ALS_POLL_DELAY_NS;
    }
    mutex_lock(&pdata->io_lock);
    if (value != ktime_to_ns(pdata->gs_poll_delay))
        pdata->gs_poll_delay = ns_to_ktime(value);
    mutex_unlock(&pdata->io_lock);
    return size;
}

static ssize_t stk_all_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint8_t gs_reg;
    uint8_t cnt;
    uint8_t i = 0;
    int len = 0;
    struct stk342x_data *pdata =  dev_get_drvdata(dev);
    uint8_t stk342x_reg_map[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x10, 0x11, 0x12, 0x13, 0x14, 0x17, 0x18, 0x19,
        0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x26, 0x27, 0x3E, 0x3F, 0x80, 0x94,
    };

    cnt = sizeof(stk342x_reg_map) / sizeof(stk342x_reg_map[0]);

    for (i = 0; i < cnt; i++)
    {
        gs_reg = stk342x_i2c_smbus_read_byte_data(pdata->client, stk342x_reg_map[i]);

        if (gs_reg < 0) {
            printk(KERN_ERR "%s fail, ret=%d", __func__, gs_reg);
            return -EINVAL;
        } else {
            printk(KERN_ERR "reg[0x%2X]=0x%2X\n", cnt, gs_reg);
            len += scnprintf(buf+len, PAGE_SIZE-len, "[%2X]%2X,", cnt, gs_reg);
        }
    }

    return len;
    /*return scnprintf(buf, PAGE_SIZE, "[0]%2X [1]%2X [2]%2X [3]%2X [4]%2X [5]%2X [6/7 HTHD]%2X,%2X [8/9 LTHD]%2X, %2X [A]%2X [B]%2X [C]%2X [D]%2X [E/F Aoff]%2X,%2X,[10]%2X [11/12 GS]%2X,%2X [13]%2X [14]%2X [15/16 Foff]%2X,%2X [17]%2X [18]%2X [3E]%2X [3F]%2X\n",
    gs_reg[0], gs_reg[1], gs_reg[2], gs_reg[3], gs_reg[4], gs_reg[5], gs_reg[6], gs_reg[7], gs_reg[8],
    gs_reg[9], gs_reg[10], gs_reg[11], gs_reg[12], gs_reg[13], gs_reg[14], gs_reg[15], gs_reg[16], gs_reg[17],
    gs_reg[18], gs_reg[19], gs_reg[20], gs_reg[21], gs_reg[22], gs_reg[23], gs_reg[24], gs_reg[25], gs_reg[26]);
    */
}

static ssize_t stk_recv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk342x_data *pdata =  dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&pdata->recv_reg));
}

static ssize_t stk_recv_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long value = 0;
	int ret;
	int32_t recv_data;
	struct stk342x_data *pdata =  dev_get_drvdata(dev);
	ret = kstrtoul(buf, 16, &value);
	if (ret < 0) {
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	recv_data = stk342x_i2c_smbus_read_byte_data(pdata->client, value);
	printk("%s: reg 0x%x=0x%x\n", __func__, (int)value, recv_data);
	atomic_set(&pdata->recv_reg, recv_data);
	return size;
}

static ssize_t stk_send_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t stk_send_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int addr, cmd;
	int32_t ret, i;
	char *token[10];
	struct stk342x_data *pdata =  dev_get_drvdata(dev);
	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");
	ret = kstrtoul(token[0], 16, (unsigned long *)&(addr));
	if (ret < 0) {
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	ret = kstrtoul(token[1], 16, (unsigned long *)&(cmd));
	if (ret < 0) {
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	printk(KERN_INFO "%s: write reg 0x%x=0x%x\n", __func__, addr, cmd);

	ret = stk342x_i2c_smbus_write_byte_data(pdata->client, (unsigned char)addr, (unsigned char)cmd);
	if (0 != ret) {
		printk(KERN_ERR "%s: stk342x_i2c_smbus_write_byte_data fail\n", __func__);
		return ret;
	}
	return size;
}

#ifdef STK_ALS_ENABLE
static struct device_attribute als_enable_attribute = __ATTR(enable, 0660, stk_als_enable_show, stk_als_enable_store);
static struct device_attribute als_lux_attribute = __ATTR(lux, 0664, stk_als_lux_show, stk_als_lux_store);
static struct device_attribute als_code_attribute = __ATTR(code, 0444, stk_als_code_show, NULL);
static struct device_attribute als_transmittance_attribute = __ATTR(transmittance, 0664, stk_als_transmittance_show, stk_als_transmittance_store);
static struct device_attribute als_poll_delay_attribute = __ATTR(ls_poll_delay, 0660, stk_als_delay_show, stk_als_delay_store);

static struct attribute *stk_als_attrs[] = {
    &als_enable_attribute.attr,
    &als_lux_attribute.attr,
    &als_code_attribute.attr,
    &als_transmittance_attribute.attr,
    &als_poll_delay_attribute.attr,
    NULL
};

/*
static struct attribute_group stk_als_attribute_group = {
	.name = "driver",
	.attrs = stk_als_attrs,
};
*/
#endif

static struct device_attribute ps_enable_attribute = __ATTR(enable, 0660, stk_ps_enable_show, stk_ps_enable_store);
static struct device_attribute ps_delay_attribute = __ATTR(ps_poll_delay, 0660, stk_ps_delay_show, stk_ps_delay_store);
static struct device_attribute ps_distance_attribute = __ATTR(distance, 0664, stk_ps_distance_show, stk_ps_distance_store);
static struct device_attribute ps_code_attribute = __ATTR(code, 0444, stk_ps_code_show, NULL);
static struct device_attribute ps_code_thd_l_attribute = __ATTR(codethdl, 0664, stk_ps_code_thd_l_show, stk_ps_code_thd_l_store);
static struct device_attribute ps_code_thd_h_attribute = __ATTR(codethdh, 0664, stk_ps_code_thd_h_show, stk_ps_code_thd_h_store);
#ifdef STK_TUNE0
static struct device_attribute ps_cali_attribute = __ATTR(cali, 0444, stk_ps_cali_show, NULL);
#endif

static struct attribute *stk_ps_attrs[] = {
    &ps_enable_attribute.attr,
    &ps_delay_attribute.attr,
    &ps_distance_attribute.attr,
    &ps_code_attribute.attr,
	&ps_code_thd_l_attribute.attr,
	&ps_code_thd_h_attribute.attr,
#ifdef STK_TUNE0
	&ps_cali_attribute.attr,
#endif
    NULL
};

/*
static struct attribute_group stk_ps_attribute_group = {
    .name = "driver",
    .attrs = stk_ps_attrs,
};*/

static struct device_attribute gs_enable_attribute = __ATTR(enable, 0660, stk_gs_enable_show, stk_gs_enable_store);
static struct device_attribute gs_delay_attribute = __ATTR(gs_poll_delay, 0660, stk_gs_delay_show, stk_gs_delay_store);
static struct device_attribute gs_code_attribute = __ATTR(code, 0444, stk_gs_code_show, NULL);
static struct device_attribute recv_attribute = __ATTR(recv, 0664, stk_recv_show, stk_recv_store);
static struct device_attribute send_attribute = __ATTR(send, 0664, stk_send_show, stk_send_store);
static struct device_attribute all_reg_attribute = __ATTR(allreg, 0444, stk_all_reg_show, NULL);

static struct attribute *stk_gs_attrs[] = {
    &gs_enable_attribute.attr,
    &gs_delay_attribute.attr,
    &gs_code_attribute.attr,
    &recv_attribute.attr,
    &send_attribute.attr,
    &all_reg_attribute.attr,
    NULL
};

/*
static struct attribute_group stk_gs_attribute_group = {
    .name = "driver",
    .attrs = stk_gs_attrs,
};*/

static ssize_t gesture_enable_show(CONST_TYPE_PREFIX struct class *cls,
	       CONST_TYPE_PREFIX struct class_attribute *attr, char *buf)
{

    int32_t enable, ret;

    mutex_lock(&gdata->io_lock);
    enable = (gdata->gs_enabled) ? 1 : 0;
    mutex_unlock(&gdata->io_lock);
    ret = stk342x_i2c_smbus_read_byte_data(gdata->client, 0x00);

    return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t gesture_enable_store(CONST_TYPE_PREFIX struct class *cls,
	       CONST_TYPE_PREFIX struct class_attribute *attr,
	       const char *buf, size_t count)
{
    uint8_t en;

    if (sysfs_streq(buf, "1")) {
		en = 1;
    } else if (sysfs_streq(buf, "0")) {
		en = 0;
    } else {
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
    }

    printk(KERN_INFO "%s: Enable GS : %d\n", __func__, en);
    mutex_lock(&gdata->io_lock);
    stk342x_enable_gs(gdata, en);
    mutex_unlock(&gdata->io_lock);

	return count;
}

static CLASS_ATTR_RW(gesture_enable);

static struct attribute *gesture_control_class_attrs[] = {
	&class_attr_gesture_enable.attr,
	NULL
};

ATTRIBUTE_GROUPS(gesture_control_class);

static struct class gesture_control_class = {
	.name = "stk342x_gestrue",
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(6, 3, 0))
	.owner = THIS_MODULE,
#endif
	.class_groups = gesture_control_class_groups,
};

#ifdef STK_POLL_ALS
static enum hrtimer_restart stk_als_timer_func(struct hrtimer *timer)
{
	struct stk342x_data *als_data = container_of(timer, struct stk342x_data, als_timer);
	queue_work(als_data->stk_als_wq, &als_data->stk_als_work);
	hrtimer_forward_now(&als_data->als_timer, als_data->als_poll_delay);
	return HRTIMER_RESTART;
}

static void stk_als_poll_work_func(struct work_struct *work)
{
	struct stk342x_data *als_data = container_of(work, struct stk342x_data, stk_als_work);
	int32_t reading, reading_lux, flag_reg;

	flag_reg = stk342x_i2c_smbus_read_byte_data(als_data->client, 0x10);
	if (flag_reg < 0)
		return;

	if (!(flag_reg & 0x80))
		return;

	reading = stk342x_get_als_reading(als_data);
	if (reading < 0)
		return;

	reading_lux = stk_alscode2lux(als_data, reading);

    //printk(KERN_INFO "als = %d, lux = %d\n", reading, reading_lux);

    als_data->als_lux_last = reading_lux;

    input_report_abs(als_data->als_input_dev, ABS_MISC, reading_lux);
    input_sync(als_data->als_input_dev);

	return;
}
#endif /* #ifdef STK_POLL_ALS */


#ifdef STK_POLL_PS
static enum hrtimer_restart stk_ps_timer_func(struct hrtimer *timer)
{
	struct stk342x_data *ps_data = container_of(timer, struct stk342x_data, ps_timer);
	queue_work(ps_data->stk_ps_wq, &ps_data->stk_ps_work);
	hrtimer_forward_now(&ps_data->ps_timer, ps_data->ps_poll_delay);
	return HRTIMER_RESTART;
}

static void stk_ps_poll_work_func(struct work_struct *work)
{
	struct stk342x_data *ps_data = container_of(work, struct stk342x_data, stk_ps_work);
	unsigned short reading;
	int32_t near_far_state;
	uint8_t org_flag_reg;

#ifdef STK_TUNE0
	if (!(ps_data->psi_set) || !(ps_data->ps_enabled))
		return;
#endif
	org_flag_reg = stk342x_i2c_smbus_read_byte_data(ps_data->client, 0x10);
	if (org_flag_reg < 0)
		return;

	if (!(org_flag_reg & 0x40))
		return;

	near_far_state = (org_flag_reg & 0x01) ? 1 : 0;
	reading = stk342x_get_ps_reading(ps_data);

	ps_data->ps_distance_last = near_far_state;

	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
	input_sync(ps_data->ps_input_dev);
	/*support wake lock for ps*/
	/*wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);*/

	return;
}
#endif

#ifdef STK_POLL_GS
static enum hrtimer_restart stk_gs_timer_func(struct hrtimer *timer)
{
    struct stk342x_data *pdata = container_of(timer, struct stk342x_data, gs_timer);
    queue_work(pdata->stk_gs_wq, &pdata->stk_gs_work);
    hrtimer_forward_now(&pdata->gs_timer, pdata->gs_poll_delay);
    return HRTIMER_RESTART;
}

static void stk_gs_poll_work_func(struct work_struct *work)
{
    struct stk342x_data *pdata = container_of(work, struct stk342x_data, stk_gs_work);
    int err;
    //int act = 0;
    int i = 0;
    unsigned char fifo_cnt = 0;
    uint16_t gse = 0;
    uint16_t gsw = 0;
    uint16_t gsn = 0;
    uint16_t gss = 0;
    uint8_t ps_data[8];
    int ges_code = KEY_RESERVED;

    err = stk342x_i2c_read_data(pdata->client, 0x1E, 1, &fifo_cnt);

    if (err < 0) {
        printk(KERN_ERR "%s: fail, ret=%d\n", __func__, err);
        return;
    }
    if (fifo_cnt & 0x1F)
    {
        for (i = 0; i < (fifo_cnt & 0x1F); i++)
        {
            stk342x_i2c_read_data(pdata->client, 0x20, 8, ps_data);

            gse = (ps_data[0] << 8 | ps_data[1]);
            gsw = (ps_data[2] << 8 | ps_data[3]);
            gsn = (ps_data[4] << 8 | ps_data[5]);
            gss = (ps_data[6] << 8 | ps_data[7]);

            pdata->Ges.ori_u[pdata->data_cnt] = gse;
            pdata->Ges.ori_d[pdata->data_cnt] = gsw;
            pdata->Ges.ori_l[pdata->data_cnt] = gsn;
            pdata->Ges.ori_r[pdata->data_cnt] = gss;
            pdata->data_cnt ++;
            
            if (pdata->data_cnt >= 32)
            {
                err = ges_test(&pdata->Ges);

                pdata->data_cnt = 0;
				ges_code = KEY_RESERVED;
                switch (pdata->Ges.gesture_motion_ ) {
                    case DIR_UP:
						ges_code = KEY_GESTURE_UP;
                        printk(KERN_ERR "UP\n");
                        break;
                    case DIR_DOWN:
						ges_code = KEY_GESTURE_DOWN;
                        printk(KERN_ERR "DOWN\n");
                        break;
                    case DIR_LEFT:
						ges_code = KEY_GESTURE_LEFT;
                        printk(KERN_ERR "LEFT\n");
                        break;
                    case DIR_RIGHT:
						ges_code = KEY_GESTURE_RIGHT;
                        printk(KERN_ERR "RIGHT\n");
                        break;
                    default:
                        // printf("NONE\n");
                        break;
                }
                memset(&pdata->Ges, 0, sizeof(stk_ges));
                pdata->Ges.gesture_motion_ = DIR_NONE;
            }
        }
    }

	input_report_key(pdata->gs_input_dev, ges_code, 1);
    input_sync(pdata->gs_input_dev);
	input_report_key(pdata->gs_input_dev, ges_code, 0);
    input_sync(pdata->gs_input_dev);
    /*support wake lock for gs*/
    /*wake_lock_timeout(&pdata->gs_wakelock, 3*HZ);*/

    return;
}
#endif

static int32_t stk342x_init_all_setting(struct i2c_client *client)
{
    int32_t ret;
    struct stk342x_data *pdata = i2c_get_clientdata(client);

    ret = stk342x_software_reset(pdata);
    if (ret < 0)
        return ret;

    ret = stk342x_check_pid(pdata);
    if (ret < 0)
        return ret;

    ret = stk342x_init_all_reg(pdata);
    if (ret < 0)
        return ret;

    atomic_set(&pdata->recv_reg, 0);

    stk342x_set_ps_thd_h(pdata, pdata->ps_thd_h);
    stk342x_set_ps_thd_l(pdata, pdata->ps_thd_l);
    return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int stk342x_suspend(struct device *dev)
{
	struct i2c_client *client =  to_i2c_client(dev);
	struct stk342x_data *pdata =  i2c_get_clientdata(client);
	int err;
#ifndef STK_POLL_GS
	/*struct i2c_client *client = to_i2c_client(dev);*/
#endif
#if 0
	if (NORMAL_STANDBY == standby_type) {

		/* process for super standby */
	} else if (SUPER_STANDBY == standby_type) {

		if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
			printk("lradc-key: talking standby, enable wakeup source lradc!!\n");
			enable_wakeup_src(CPUS_GPIO_SRC, 0);
		} else {
				}
	}

	return 0;
#endif

	printk(KERN_INFO "%s\n", __func__);
	mutex_lock(&pdata->io_lock);

#ifdef STK_ALS_ENABLE
	if (pdata->als_enabled)
    {
        stk342x_enable_als(pdata, 0);
        pdata->re_enable_als = true;
		pr_err("aw==== in the re enable als \n");
    }
#endif

#ifdef STK_POLL_PS
	if (pdata->ps_enabled)
    {
        stk342x_enable_ps(pdata, 0);
        pdata->re_enabled_ps = true;
        pr_err("aw==== in the re enable ps \n");
    }
#else
    pr_err("aw==== suspend \n");

    if (SUPER_STANDBY == standby_type) {
        err = enable_wakeup_src(CPUS_GPIO_SRC, 0);
        if (err)
            printk(KERN_WARNING "%s: set_irq_wake(%d) failed, err=(%d)\n", __func__, pdata->irq, err);
    } else {
        printk(KERN_ERR "%s: not support wakeup source", __func__);
    }
#endif

	if (pdata->gs_enabled)
    {
        stk342x_enable_gs(pdata, 0);
        pdata->re_enabled_gs = true;
        pr_err("aw==== in the re enable gs \n");
    }

	mutex_unlock(&pdata->io_lock);

	if (ls_sensor_info.sensor_power_ldo != NULL) {
		err = regulator_disable(ls_sensor_info.sensor_power_ldo);
		if (err)
			printk("stk power down failed\n");
	}

	return 0;
}

static int stk342x_resume(struct device *dev)
{
	struct i2c_client *client =  to_i2c_client(dev);
	int err;
#ifndef STK_POLL_GS
#endif
	struct stk342x_data *pdata;

	if (ls_sensor_info.sensor_power_ldo != NULL) {
		err = regulator_enable(ls_sensor_info.sensor_power_ldo);
		if (err)
			printk("stk power on failed\n");
	}

	stk342x_init_all_setting(client);
	pdata =  i2c_get_clientdata(client);

#ifndef STK_POLL_GS
	/*struct i2c_client *client = to_i2c_client(dev);*/
#endif
	printk(KERN_INFO "%s\n", __func__);

	mutex_lock(&pdata->io_lock);
#ifdef STK_ALS_ENABLE
	if (pdata->re_enable_als)
    {
		stk342x_enable_als(pdata, 1);
		pdata->re_enable_als = false;
	}
#endif
#ifdef STK_PS_ENABLE
	if (pdata->ps_enabled)
    {
#ifdef STK_POLL_PS

	/*wake_unlock(&pdata->ps_nosuspend_wl);*/
#else
		if (SUPER_STANDBY == standby_type) {
			err = disable_wakeup_src(CPUS_GPIO_SRC, 0);
			if (err)
				printk(KERN_WARNING "%s: disable_irq_wake(%d) failed, err=(%d)\n", __func__, pdata->irq, err);
		}
#endif
	}
    else if (pdata->re_enabled_ps)
    {
		stk342x_enable_ps(pdata, 1);
		pdata->re_enabled_ps = false;
	}
#endif

    if (pdata->re_enabled_gs)
    {
		stk342x_enable_gs(pdata, 1);
		pdata->re_enabled_gs = false;
	}

	mutex_unlock(&pdata->io_lock);
	return 0;
}

#endif

static int stk342x_sysfs_create_files(struct kobject *kobj, struct attribute **attrs)
{
    int err;
    while (*attrs != NULL)
    {
        err = sysfs_create_file(kobj, *attrs);
        if (err)
            return err;
        attrs++;
    }
    return 0;
}

static int stk342x_sysfs_remove_files(struct kobject *kobj, struct attribute **attrs)
{
    while (*attrs != NULL)
    {
        sysfs_remove_file(kobj, *attrs);
        attrs++;
    }
    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int stk342x_probe(struct i2c_client *client)
#else
static int stk342x_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
    int err = -ENODEV;
    struct stk342x_data *pdata;

    if (ls_sensor_info.dev == NULL)
    {
        ls_sensor_info.dev = &client->dev;
    }
    printk(KERN_INFO "%s: driver version = %s\n", __func__, DRIVER_VERSION);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        printk(KERN_ERR "%s: No Support for I2C_FUNC_I2C\n", __func__);
        return -ENODEV;
    }

    pdata = kzalloc(sizeof(struct stk342x_data), GFP_KERNEL);
    if (!pdata)
    {
        printk(KERN_ERR "%s: failed to allocate stk342x_data\n", __func__);
        return -ENOMEM;
    }

    pdata->client = client;
    i2c_set_clientdata(client, pdata);
    mutex_init(&pdata->io_lock);

#ifdef STK_ALS_ENABLE
	pdata->als_enabled = false;
    pdata->als_transmittance = 500;
	pdata->re_enable_als = false;
#endif
#ifdef STK_PS_ENABLE
    pdata->ps_thd_h = 1700;
    pdata->ps_thd_l = 1500;
	pdata->ps_enabled = false;
	pdata->re_enabled_ps = false;
#endif
	pdata->first_boot = true;

    pdata->als_input_dev = input_allocate_device();
    if (pdata->als_input_dev == NULL)
    {
        printk(KERN_ERR "%s: could not allocate als device\n", __func__);
        err = -ENOMEM;
        goto err_als_input_allocate;
    }
    pdata->ps_input_dev = input_allocate_device();
    if (pdata->ps_input_dev == NULL)
    {
        printk(KERN_ERR "%s: could not allocate ps device\n", __func__);
        err = -ENOMEM;
        goto err_ps_input_allocate;
    }
    pdata->gs_input_dev = input_allocate_device();
    if (pdata->gs_input_dev == NULL)
    {
        printk(KERN_ERR "%s: could not allocate gs device\n", __func__);
        err = -ENOMEM;
        goto err_gs_input_allocate;
    }
    pdata->als_input_dev->name = ALS_NAME;
    pdata->ps_input_dev->name = PS_NAME;
    pdata->gs_input_dev->name = GS_NAME;
    set_bit(EV_ABS, pdata->als_input_dev->evbit);
    set_bit(EV_ABS, pdata->ps_input_dev->evbit);
//    set_bit(EV_ABS, pdata->gs_input_dev->evbit);

    input_set_abs_params(pdata->als_input_dev, ABS_MISC, 0, stk_alscode2lux(pdata, (1 << 16) - 1), 0, 0);
#ifdef NEED_REPORT_MUTIL_LEVEL_DATA
    input_set_abs_params(pdata->ps_input_dev, ABS_DISTANCE, 0, 5, 0, 0);
#else
    input_set_abs_params(pdata->ps_input_dev, ABS_DISTANCE, 0, 1, 0, 0);
#endif

	input_set_capability(pdata->gs_input_dev, EV_KEY, KEY_GESTURE_UP);
	input_set_capability(pdata->gs_input_dev, EV_KEY, KEY_GESTURE_DOWN);
	input_set_capability(pdata->gs_input_dev, EV_KEY, KEY_GESTURE_LEFT);
	input_set_capability(pdata->gs_input_dev, EV_KEY, KEY_GESTURE_RIGHT);

    err = input_register_device(pdata->als_input_dev);
    if (err < 0)
    {
        printk(KERN_ERR "%s: can not register als input device\n", __func__);
        input_free_device(pdata->als_input_dev);
        goto err_als_input_register;
    }
    err = input_register_device(pdata->ps_input_dev);
    if (err < 0)
    {
        printk(KERN_ERR "%s: can not register ps input device\n", __func__);
        input_free_device(pdata->ps_input_dev);
        goto err_ps_input_register;
    }
    err = input_register_device(pdata->gs_input_dev);
    if (err < 0) {
        printk(KERN_ERR "%s: can not register gs input device\n", __func__);
        input_free_device(pdata->gs_input_dev);
        goto err_gs_input_register;
    }

    err = stk342x_sysfs_create_files(&pdata->als_input_dev->dev.kobj, stk_als_attrs);
    if (err < 0)
    {
        printk(KERN_ERR "%s:could not create sysfs group for als\n", __func__);
        goto err_als_sysfs_create_group;
    }
    kobject_uevent(&pdata->als_input_dev->dev.kobj, KOBJ_CHANGE);
    err = stk342x_sysfs_create_files(&pdata->ps_input_dev->dev.kobj, stk_ps_attrs);
    if (err < 0)
    {
        printk(KERN_ERR "%s:could not create sysfs group for ps\n", __func__);
        goto err_ps_sysfs_create_group;
    }
    kobject_uevent(&pdata->ps_input_dev->dev.kobj, KOBJ_CHANGE);
    err = stk342x_sysfs_create_files(&pdata->gs_input_dev->dev.kobj, stk_gs_attrs);
    if (err < 0)
    {
        printk(KERN_ERR "%s:could not create sysfs group for gs\n", __func__);
        goto err_gs_sysfs_create_group;
    }
    kobject_uevent(&pdata->gs_input_dev->dev.kobj, KOBJ_CHANGE);
    err = class_register(&gesture_control_class);
    if (err) {
		printk(KERN_ERR "%s:could not create sysfs gestrue control for gs\n", __func__);
    }

    input_set_drvdata(pdata->als_input_dev, pdata);
    input_set_drvdata(pdata->ps_input_dev, pdata);
    input_set_drvdata(pdata->gs_input_dev, pdata);

#ifdef STK_POLL_ALS
    pdata->stk_als_wq = create_singlethread_workqueue("stk_als_wq");
    INIT_WORK(&pdata->stk_als_work, stk_als_poll_work_func);
    hrtimer_init(&pdata->als_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    pdata->als_poll_delay = ns_to_ktime(20 * NSEC_PER_MSEC);
    pdata->als_timer.function = stk_als_timer_func;
    pdata->is_als_timer_active = false;
#endif

#ifdef STK_POLL_PS
    pdata->stk_ps_wq = create_singlethread_workqueue("stk_ps_wq");
    INIT_WORK(&pdata->stk_ps_work, stk_ps_poll_work_func);
    hrtimer_init(&pdata->ps_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    pdata->ps_poll_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
    pdata->ps_timer.function = stk_ps_timer_func;
    pdata->is_ps_timer_active = false;
#endif

#ifdef STK_POLL_GS
    pdata->stk_gs_wq = create_singlethread_workqueue("stk_gs_wq");
    INIT_WORK(&pdata->stk_gs_work, stk_gs_poll_work_func);
    hrtimer_init(&pdata->gs_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    pdata->gs_poll_delay = ns_to_ktime(30 * NSEC_PER_MSEC);
    pdata->gs_timer.function = stk_gs_timer_func;
    pdata->is_gs_timer_active = false;
#endif

    device_init_wakeup(&client->dev, true);
    err = stk342x_init_all_setting(client);
    if (err < 0)
    {
        goto err_init_all_setting;
    }

    gdata = pdata;

    sProbeSuccess = 1;
    printk(KERN_INFO "%s: probe successfully\n", __func__);

    return 0;

err_init_all_setting:
	input_sensor_free(&(ls_sensor_info.input_type));
	device_init_wakeup(&client->dev, false);
#ifdef STK_POLL_GS
	hrtimer_try_to_cancel(&pdata->gs_timer);
	destroy_workqueue(pdata->stk_gs_wq);
#endif

	stk342x_sysfs_remove_files(&pdata->gs_input_dev->dev.kobj, stk_gs_attrs);
err_gs_sysfs_create_group:
	stk342x_sysfs_remove_files(&pdata->ps_input_dev->dev.kobj, stk_ps_attrs);
err_ps_sysfs_create_group:
	stk342x_sysfs_remove_files(&pdata->als_input_dev->dev.kobj, stk_als_attrs);
err_als_sysfs_create_group:

	input_unregister_device(pdata->gs_input_dev);
err_gs_input_register:
	input_unregister_device(pdata->ps_input_dev);
err_ps_input_register:
	input_unregister_device(pdata->als_input_dev);
err_als_input_register:

err_als_input_allocate:
err_ps_input_allocate:
err_gs_input_allocate:

    mutex_destroy(&pdata->io_lock);
    kfree(pdata);
    return err;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void stk342x_remove(struct i2c_client *client)
#else
static int stk342x_remove(struct i2c_client *client)
#endif
{
    struct stk342x_data *pdata = i2c_get_clientdata(client);

    device_init_wakeup(&client->dev, false);
#if (!defined(STK_POLL_GS))
    if (0 != ls_sensor_info.int_number)
        input_free_int(&(ls_sensor_info.input_type), pdata);
#endif

#ifdef STK_POLL_GS
    hrtimer_try_to_cancel(&pdata->gs_timer);
    destroy_workqueue(pdata->stk_gs_wq);
    class_unregister(&gesture_control_class);
    stk342x_sysfs_remove_files(&pdata->gs_input_dev->dev.kobj, stk_gs_attrs);
    input_unregister_device(pdata->gs_input_dev);
#endif

	/*support wake lock for gs*
#ifdef STK_POLL_GS
	wake_lock_destroy(&pdata->gs_nosuspend_wl);
#endif
	wake_lock_destroy(&pdata->gs_wakelock);	*/
	mutex_destroy(&pdata->io_lock);
	kfree(pdata);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    return;
#else
    return 0;
#endif
}

static const struct i2c_device_id stk_gs_id[] = {
    { "stk342x", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, stk_gs_id);

static const struct of_device_id stk342x_of_match[] = {
    {.compatible = "allwinner,stk342x"},
    {},
};

#if IS_ENABLED(CONFIG_PM)
static UNIVERSAL_DEV_PM_OPS(stk_pm_ops, stk342x_suspend,
		stk342x_resume, NULL);
#endif

static struct i2c_driver stk_gs_driver = {
    .class   = I2C_CLASS_HWMON,
    .driver = {
        .of_match_table = stk342x_of_match,
        .name  = DEVICE_NAME,
        .owner = THIS_MODULE,
        #if IS_ENABLED(CONFIG_PM)
        .pm = &stk_pm_ops,
        #endif
    },
    .probe    = stk342x_probe,
    .remove = stk342x_remove,
    .id_table = stk_gs_id,
    .address_list	= normal_i2c,
};

static int startup(void)
{
	int ret = 0;

	if (input_sensor_startup(&(ls_sensor_info.input_type))) {
		printk("%s: ls_fetch_sysconfig_para err.\n", __func__);
		return -1;
	} else {
		ret = input_sensor_init(&(ls_sensor_info.input_type));
		if (0 != ret) {
			printk("%s:ls_init_platform_resource err. \n", __func__);
		}
	}
	if (ls_sensor_info.sensor_used == 0) {
		printk("*** ls_used set to 0 !\n");
		printk("*** if use light_sensor,please put the sys_config.fex ls_used set to 1. \n");
		return -1;
	}
	return 0;
}

static int __init stk342x_init(void)
{
	if (startup() != 0)
		return -1;
	if (!ls_sensor_info.isI2CClient)
		stk_gs_driver.detect = stk_detect;

	i2c_add_driver(&stk_gs_driver);

	return sProbeSuccess ? 0 : -ENODEV;
}

static void __exit stk342x_exit(void)
{
		printk("%s  exit !!\n", __func__);
		i2c_del_driver(&stk_gs_driver);
		input_sensor_free(&(ls_sensor_info.input_type));
}

late_initcall(stk342x_init);
module_exit(stk342x_exit);
module_param_named(debug_mask, debug_mask, int, 0644);
MODULE_AUTHOR("allwinner");
MODULE_DESCRIPTION("Sensortek stk342x Proximity Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
