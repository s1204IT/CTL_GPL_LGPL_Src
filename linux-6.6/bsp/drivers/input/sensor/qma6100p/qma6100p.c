/* drivers/hwmon/qma6100p.c
 *
 *allwinner platform

 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/version.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include "../../init-input.h"
//#include <mach/system.h>
//#include <mach/hardware.h>
//#undef CONFIG_PM

// if use i2c detect, set 1
#define USE_DETECT   0
static int gUseDetect;


#if defined(CONFIG_PM)
#include <linux/pm.h>
#endif

/*
 * Defines
 */
#define assert(expr) \
	do {\
		if (!(expr)) {\
			printk("Assertion failed! %s,%d,%s,%s\n",\
				__FILE__, __LINE__, __func__, #expr);\
		}\
	} while (0)

#define QMA6100P_DRV_NAME		"qma6100p"
#define SENSOR_NAME 		QMA6100P_DRV_NAME
#define QMA6100P_DEVICE_NAME	"qma6100p"

#define ABSMIN_4G          -16384     //±4g，14bit
#define ABSMAX_4G           16383
#define POLL_INTERVAL_MAX	1000
#define POLL_INTERVAL		50
#define INPUT_FUZZ	2
#define INPUT_FLAT	2
#define TIANJU_FILTER

#define QMA6100P_WHO_AM_I            0x90

/***********************************************
 *** REGISTER MAP FOR QMA6100P
 ***********************************************/
#define QMA6100P_REG_CHIP_ID         0x00
#define QMA6100P_REG_XOUT_LSB        0x01
#define QMA6100P_REG_YOUT_LSB        0x03
#define QMA6100P_REG_ZOUT_LSB        0x05
#define QMA6100P_REG_INT_STATUS0     0x09
#define QMA6100P_REG_INT_STATUS2     0x0B
#define QMA6100P_REG_FIFO_STATUS     0x0E
#define QMA6100P_REG_RANGE_FILTER    0x0F
#define QMA6100P_REG_BAND_WIDTH      0x10
#define QMA6100P_REG_PM              0x11
#define QMA6100P_REG_INT_EN1         0x17
#define QMA6100P_REG_INT_EN2         0x18
#define QMA6100P_REG_INT1_MAP1       0x1A
#define QMA6100P_REG_INT2_MAP1       0x1C
#define QMA6100P_REG_INT_CFG0        0x20
#define QMA6100P_REG_INT_CFG1        0x21
#define QMA6100P_REG_MD_CFG0         0x2C
#define QMA6100P_REG_MD_CFG2         0x2E
#define QMA6100P_REG_MD_CFG3         0x2F
#define QMA6100P_REG_MD_RST_CFG      0x30
#define QMA6100P_REG_FIFO_WM         0x31
#define QMA6100P_REG_NVM             0x33
#define QMA6100P_REG_SW_RESET        0x36
#define QMA6100P_REG_FIFO_CFG0       0x3E
#define QMA6100P_REG_FIFO_DATA       0x3F
#define QMA6100P_REG_TST0_ANA        0x4A
#define QMA6100P_REG_AFE_ANA         0x56
#define QMA6100P_REG_TST1_ANA        0x5F
/***********************************************
 *** register config
 ***********************************************/
#define QMA6100P_MCLK_51_2K          0x04

#define QMA6100P_STANDBY_MODE        0x00
#define QMA6100P_WAKE_MODE           0x80

#define QMA6100P_DEFAULT_LPF         0x60
#define QMA6100P_DEFAULT_RANGE       0x04

#define QMA6100P_INT_FWM_EN          0x40
#define QMA6100P_INT_FULL_INT        0x02
#define QMA6100P_INT_AMD_EN          0x07
#define QMA6100P_DIS_PU_SENB         0x80
#define QMA6100P_INT_LVL             0x05

#define QMA6100P_DIS_I2C             0x20
#define QMA6100P_LATCH_INT           0x01

#define QMA6100P_INT1_FWM_EN         0x40
#define QMA6100P_INT1_AMD_EN         0x01

#define QMA6100P_FIFO_MODE_BYPASS    0x07
#define QMA6100P_FIFO_MODE_FIFO      0x47

#define QMA6100P_FIFO_WM_DEFAULT     0x20

/***********************************************
 *** CONFIG ODR
 ***********************************************/
#define QMA6100P_ODR_12HZ_REG_VALUE  0x07  //15Hz 
#define QMA6100P_ODR_25HZ_REG_VALUE  0x06  //25Hz 
#define QMA6100P_ODR_50HZ_REG_VALUE  0x05  //50Hz 
#define QMA6100P_ODR_100HZ_REG_VALUE 0x00  //100Hz
#define QMA6100P_ODR_200HZ_REG_VALUE 0x01  //200Hz
#define QMA6100P_ODR_400HZ_REG_VALUE 0x02  //500Hz



#define Z_OFF_DEFAULT  (8) 		/* (110) */
#define XY_THR_N	  (-15) 	/* (-240) */
#define XY_THR_P	  (15) 		/* (240) */
#define Z_THR_MIN_N   (-50) 	/* (-780) */
#define Z_THR_MIN_P   (50) 		/* (780) */
#define Z_THR_MAX_N   (-78) 	/* (-1200) */
#define Z_THR_MAX_P   (78) 		/* (1200) */
#define SUM_DOT	   (50)
#define THRESHOLD_VAL (8) 		/* (40) */

//static struct device  			*hwmon_dev;
static struct i2c_client		*qma6100p_i2c_client;
static struct input_dev  *qma6100p_idev;

struct qma6100p_data_s {
	    struct i2c_client		*client;
	    struct input_dev		*inputDev;
	    struct mutex			interval_mutex;

		struct delayed_work		dwork;//allen
		int	time_of_cali;
		atomic_t enable;

#if defined(CONFIG_PM)
	volatile int suspend_indator;
#endif
	struct hrtimer hr_timer;
	struct work_struct wq_hrtimer;
	ktime_t ktime;

} qma6100p_data;

/* Addresses to scan */
static const unsigned short normal_i2c[] = {0x12, I2C_CLIENT_END};
static __u32 twi_id;
static unsigned int   delayMs = POLL_INTERVAL;

static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
	.np_name = QMA6100P_DEVICE_NAME,
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask;
#define dprintk(level_mask, fmt, arg...) \
	do {\
		if (unlikely(debug_mask & (level_mask))) \
			printk(KERN_DEBUG fmt, ## arg); \
	} while (0)

module_param_named(debug_mask, debug_mask, int, S_IRUGO);

static int startup(void);
static int qma6100p_reg_init(struct i2c_client *client);

/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:
 *	                = 0; success;
 *	                < 0; err
 */

static int qma6100p_gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	//__s32 ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	if (twi_id == adapter->nr) {
		printk("%s: addr= %x\n", __func__, client->addr);
/*
 		ret = gsensor_i2c_test(client);
		if (!ret) {
			pr_info("%s:I2C connection might be something wrong or maybe the other gsensor equipment! \n",__func__);
			return -ENODEV;
		} else
*/
		{
			dprintk(DEBUG_INIT, "%s: I2C connection sucess!\n", __func__);
			strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
			return 0;
		}

	} else {
		dprintk(DEBUG_INIT, "%s: I2C connection error!\n",  __func__);
		return -ENODEV;
	}
}

static ssize_t qma6100p_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = qma6100p_i2c_client;
	struct qma6100p_data_s *qma6100p = NULL;

	qma6100p = i2c_get_clientdata(client);
	return sprintf(buf, "%d\n", delayMs);
}

static ssize_t qma6100p_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	//int error;
	struct i2c_client *client = qma6100p_i2c_client;
	struct qma6100p_data_s *qma6100p  = NULL;

	qma6100p    = i2c_get_clientdata(client);
	dprintk(DEBUG_INIT, "delay store %d\n", __LINE__);

	data = simple_strtoul(buf, NULL, 10);

	if (data > POLL_INTERVAL_MAX)
		data = POLL_INTERVAL_MAX;
	if (0 >= data)
		data = 10;

	mutex_lock(&qma6100p->interval_mutex);
	delayMs = data;
	mutex_unlock(&qma6100p->interval_mutex);
	printk(KERN_INFO "%s: qma6100p delay %d\n", __func__, delayMs);

	if (atomic_read(&qma6100p_data.enable)) {
		if (delayMs <= 10) {
			// early 0.5ms
			qma6100p_data.ktime = ktime_set(0, delayMs* NSEC_PER_MSEC - NSEC_PER_MSEC / 2);
		} else {
			qma6100p_data.ktime = ktime_set(0, delayMs* NSEC_PER_MSEC);
		}
		hrtimer_start(&qma6100p_data.hr_timer, qma6100p_data.ktime, HRTIMER_MODE_REL);
	}

	return count;
}

static void enable_sensor(int enable)
{
	int error;
	u8 databuf[2] = {0};

	databuf[0] = i2c_smbus_read_byte_data(qma6100p_i2c_client, QMA6100P_REG_PM);
	if (!enable) {
		databuf[0] = (databuf[0] & 0x70) | 0x87; //accel standby
		hrtimer_cancel(&qma6100p_data.hr_timer);
	} else {
		databuf[0] = (databuf[0] & 0x70) | 0x84; //accel enable
		printk(KERN_INFO "%s: qma6100p delay %d\n", __func__, delayMs);
		if (delayMs <= 10) {
			// early 0.5ms
			qma6100p_data.ktime = ktime_set(0, delayMs * NSEC_PER_MSEC - NSEC_PER_MSEC / 2);
		} else {
			qma6100p_data.ktime = ktime_set(0, delayMs * NSEC_PER_MSEC);
		}
		hrtimer_start(&qma6100p_data.hr_timer, qma6100p_data.ktime, HRTIMER_MODE_REL);
	}
	error = i2c_smbus_write_byte_data(qma6100p_i2c_client, QMA6100P_REG_PM, databuf[0]);
	assert(error == 0);

}

static ssize_t qma6100p_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = qma6100p_i2c_client;
	struct qma6100p_data_s *qma6100p = NULL;

	qma6100p = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&qma6100p_data.enable));

}

static ssize_t qma6100p_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;

	struct i2c_client *client = qma6100p_i2c_client;
	struct qma6100p_data_s *qma6100p = NULL;

	qma6100p = i2c_get_clientdata(client);

	printk(KERN_INFO "%s: buf=%s\n", __func__, buf);
	data = simple_strtoul(buf, NULL, 10);

	atomic_set(&qma6100p_data.enable, data);
	enable_sensor(data);
	return count;
}



static ssize_t QMA6100P_register_store(struct device *dev,
	    struct device_attribute *attr,
	    const char *buf, size_t count)
{
	unsigned int address, value;
	int result = 0;
	sscanf(buf, "0x%x=0x%x", &address, &value);

	result = i2c_smbus_write_byte_data(qma6100p_i2c_client, address,value);

	if (result)
		printk("%s:fail to write sensor_register\n", __func__);

	return count;
}

static ssize_t QMA6100P_register_show(struct device *dev,
	    struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	//u8 reg[0x5b];
	char i;
	int buffer[3] = {0};
	i = 0x0f;
	*buffer = i;
	buffer[0] = i2c_smbus_read_byte_data(qma6100p_i2c_client, i);
	count += sprintf(buf, "0x%x: 0x%x\n", i, buffer[0]);
	for (i = 0x00;i < 0x5a; i++) {
		*buffer = i;
		buffer[0] = i2c_smbus_read_byte_data(qma6100p_i2c_client, i);
		count += sprintf(&buf[count],"0x%x: 0x%x\n", i, buffer[0]);
	}
	return count;
}

static DEVICE_ATTR(enable, 0644,
		qma6100p_enable_show, qma6100p_enable_store);

static DEVICE_ATTR(delay, 0644,
		qma6100p_delay_show, qma6100p_delay_store);

static DEVICE_ATTR(reg, 0644,
		QMA6100P_register_show, QMA6100P_register_store);

static struct attribute *qma6100p_attributes[] = {
//	&dev_attr_reg13_reset.attr,
	&dev_attr_reg.attr,
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
//	&dev_attr_calibration_run.attr,
	NULL
};

static struct attribute_group qma6100p_attribute_group = {
	.attrs = qma6100p_attributes
};

static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
	schedule_work(&qma6100p_data.wq_hrtimer);
	hrtimer_forward_now(&qma6100p_data.hr_timer, qma6100p_data.ktime);
	return HRTIMER_RESTART;
}

static void wq_func_hrtimer(struct work_struct *work)
{
	u8 xyz[6] = {0 };
	s16 x = 0, y = 0, z = 0;

	i2c_smbus_read_i2c_block_data(qma6100p_i2c_client, QMA6100P_REG_XOUT_LSB, 6, xyz);

	//printk("acc.x[0] %x/%d, acc.x[1] %x/%d\n", xyz[0], xyz[0], xyz[1], xyz[1]);
	//printk("acc.y[2] %x/%d, acc.y[3] %x/%d\n", xyz[2], xyz[2], xyz[3], xyz[3]);
	//printk("acc.x[4] %x/%d, acc.x[5] %x/%d\n", xyz[4], xyz[4], xyz[5], xyz[5]);
	x = (signed short)((signed short)(xyz[1] << 8) | (xyz[0] >> 2));
	y = (signed short)((signed short)(xyz[3] << 8) | (xyz[2] >> 2));
	z = (signed short)((signed short)(xyz[5] << 8) | (xyz[4] >> 2));
	//printk("acc.x %d, acc.y %d, acc.z %d\n", x, y, z);
	input_report_abs(qma6100p_data.inputDev, ABS_X, x);
	input_report_abs(qma6100p_data.inputDev, ABS_Y, y);
	input_report_abs(qma6100p_data.inputDev, ABS_Z, z);

	input_sync(qma6100p_data.inputDev);
}


static int qma6100p_reg_init(struct i2c_client *client)
{
	int res = 0;
	unsigned char reg_value = 0 ;

    reg_value = i2c_smbus_read_byte_data(client, QMA6100P_REG_CHIP_ID);
	if (reg_value != QMA6100P_WHO_AM_I) {
		printk("%s:check id err,reg_value:%d\n", __func__, reg_value);
		return -1;
	}else{
		printk("%s:check id success -->qma6100p\n", __func__);
	}
    //6100p reset
	res = i2c_smbus_write_byte_data(client, QMA6100P_REG_SW_RESET, 0xb6);
	msleep(20);
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_SW_RESET,0x00);
	msleep(30);
	//config mclc,mclk=51.2k,wakeup mode
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_PM, 0x84);
	//ana setting
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_TST0_ANA, 0x20);
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_AFE_ANA,  0x01);
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_TST1_ANA, 0x80);
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_TST1_ANA, 0x00);
	//set range ±8g
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_RANGE_FILTER, 0x04);
	//set odr 400hz
	//reg_value =QMA6100P_DEFAULT_LPF | QMA6100P_ODR_400HZ_REG_VALUE;
	res += i2c_smbus_write_byte_data(client, QMA6100P_REG_BAND_WIDTH, 0x00);
	if (res) {
		dev_err(&client->dev, "qma6100p init err=: %d!\n", res);
		return res;
	}   

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int qma6100p_probe(struct i2c_client *client)
#else
static int qma6100p_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
{
	int result;
	struct i2c_adapter *adapter;
	struct qma6100p_data_s* data = &qma6100p_data;

	printk( "qma6100p probe********************************SSS\n");

	if (gsensor_info.dev == NULL)
		gsensor_info.dev = &client->dev;
#if !USE_DETECT
	if (!gUseDetect && startup() != 0)
		return 0;
#endif 

	printk( "qma6100p probe\n");
	qma6100p_i2c_client = client;
	adapter = to_i2c_adapter(client->dev.parent);
	result = i2c_check_functionality(adapter,
					 I2C_FUNC_SMBUS_BYTE |
					 I2C_FUNC_SMBUS_BYTE_DATA);
	assert(result);

	result = qma6100p_reg_init(client);
	if (result) {
		input_sensor_free(&(gsensor_info.input_type));
		return -1;
	}
	//hwmon_dev = hwmon_device_register(&client->dev);
	//assert(!(IS_ERR(hwmon_dev)));

	/*input poll device register */
	qma6100p_idev = input_allocate_device();
	if (!qma6100p_idev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		result = -ENOMEM;
		return result;
	}
 
	qma6100p_idev->name = QMA6100P_DRV_NAME;
	qma6100p_idev->id.bustype = BUS_I2C;

	mutex_init(&data->interval_mutex);
	input_set_capability(qma6100p_idev, EV_ABS, ABS_MISC);
	input_set_abs_params(qma6100p_idev, ABS_X, ABSMIN_4G, ABSMAX_4G, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(qma6100p_idev, ABS_Y, ABSMIN_4G, ABSMAX_4G, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(qma6100p_idev, ABS_Z, ABSMIN_4G, ABSMAX_4G, INPUT_FUZZ, INPUT_FLAT);
	input_set_drvdata(qma6100p_idev, &qma6100p_data);

	result = input_register_device(qma6100p_idev);
	if (result) {
		dev_err(&client->dev, "register device failed!\n");
		return result;
	}
	result = sysfs_create_group(&qma6100p_idev->dev.kobj, &qma6100p_attribute_group);

	if (result) {
		dev_err(&client->dev, "create sys failed\n");
	}

	kobject_uevent(&qma6100p_idev->dev.kobj, KOBJ_CHANGE);

	data->client  = client;
	data->inputDev = qma6100p_idev;
	i2c_set_clientdata(client, data);

	hrtimer_init(&qma6100p_data.hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	qma6100p_data.hr_timer.function = my_hrtimer_callback;
	INIT_WORK(&qma6100p_data.wq_hrtimer, wq_func_hrtimer);
	
	printk( "qma6100p probe finish\n");
	return result;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void qma6100p_remove(struct i2c_client *client)
#else
static int qma6100p_remove(struct i2c_client *client)
#endif //#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
{
	
	//hwmon_device_unregister(hwmon_dev);

	sysfs_remove_group(&qma6100p_idev->dev.kobj, &qma6100p_attribute_group);
	input_unregister_device(qma6100p_idev);
	input_free_device(qma6100p_idev);
	i2c_set_clientdata(qma6100p_i2c_client, NULL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	return;
#else
	return 0;
#endif //#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
}

#if IS_ENABLED(CONFIG_PM)
static int qma6100p_resume(struct device *dev)
{
	int enable = atomic_read(&qma6100p_data.enable);
	int result;
	printk( "qma6100p_resume\n");
	input_set_power_enable(&(gsensor_info.input_type), 1);
	msleep(10);
	result = qma6100p_reg_init(qma6100p_i2c_client);
	if(result) {
		printk( "%s qma6100p_reg_init failed!\n", __func__);
		return -1;
	}
		
	enable_sensor(enable);
	return 0;
}

static int qma6100p_suspend(struct device *dev)
{
	printk( "qma6100p_suspend\n");
	enable_sensor(0);
	input_set_power_enable(&(gsensor_info.input_type), 0);
	return 0;
}

static UNIVERSAL_DEV_PM_OPS(qma6100p_pm_ops, qma6100p_suspend,
		qma6100p_resume, NULL);
#endif

#if !USE_DETECT
static const struct of_device_id qma6100p_of_match[] = {
	{.compatible = "allwinner,qma6100p"},
	{},
};
#endif
static const struct i2c_device_id qma6100p_id[] = {
	{ QMA6100P_DRV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, qma6100p_id);

static struct i2c_driver qma6100p_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
#if !USE_DETECT
		.of_match_table = qma6100p_of_match,
#endif
		.name = QMA6100P_DRV_NAME,
		.owner = THIS_MODULE,

#if IS_ENABLED(CONFIG_PM)
		.pm = &qma6100p_pm_ops,
#endif
	},
	.probe = qma6100p_probe,
	.remove	= qma6100p_remove,
	.id_table = qma6100p_id,
#if USE_DETECT
	.detect = qma6100p_gsensor_detect,
#endif
	.address_list = normal_i2c,
};

static int startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__,__LINE__);
#if USE_DETECT
	printk("mir3da use sepical device node name = %s\n", QMA6100P_DEVICE_NAME);
	gsensor_info.node = of_find_node_by_name(NULL, QMA6100P_DEVICE_NAME);
#endif
	if (input_sensor_startup(&(gsensor_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else
		ret = input_sensor_init(&(gsensor_info.input_type));

	if (0 != ret) {
	    printk("%s:gsensor.init_platform_resource err. \n", __func__);
	}

	twi_id = gsensor_info.twi_id;
	input_set_power_enable(&(gsensor_info.input_type), 1);
	return 0;
}

static int __init qma6100p_init(void)
{
	int ret = -1;
#if !USE_DETECT
	struct device_node *np = NULL;
	char *p = NULL;
#endif

#if USE_DETECT
	if (startup() != 0)
		return -1;
#else
	np = of_find_node_by_name(NULL, QMA6100P_DEVICE_NAME);
	if (np && np->parent) {
		p = (char *) np->parent->name;
		// if not set USE_DETECT but do not config ctp in twi, also use i2c detect
		printk("<===Lion %s:parent name =%s ===>\n",__func__,p);
		if (strncmp(p, "twi", 3) != 0) {
			gUseDetect = 1;
			if (startup() != 0)
				return -1;
			qma6100p_driver.detect = qma6100p_gsensor_detect;
		}
	}
#endif
	ret = i2c_add_driver(&qma6100p_driver);
	if (ret < 0) {
		printk( "add qma6100p i2c driver failed\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit qma6100p_exit(void)
{
	printk("remove qma6100p i2c driver.\n");
	i2c_del_driver(&qma6100p_driver);
	input_sensor_free(&(gsensor_info.input_type));
	input_set_power_enable(&(gsensor_info.input_type), 0);
}

MODULE_AUTHOR("Qing <qing_lv@qstcorp.com>");
MODULE_DESCRIPTION("qma6100p driver");
MODULE_LICENSE("GPL");

module_init(qma6100p_init);
module_exit(qma6100p_exit);

