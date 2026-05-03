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
//#include <linux/input-polldev.h>
#include <linux/device.h>
#include "../../init-input.h"
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
//#include <mach/system.h>
//#include <mach/hardware.h>
//#undef CONFIG_PM

/***********************************************
 *** REGISTER MAP
 ***********************************************/

#define SENSOR_CHIP_ID_ICM42607      0x05

#define ICM42607_DEV_NAME				"icm42607"
#define ICM42607_DEV_VERSION				"1.0.1"
#define ICM42607_ACC_INPUT_NAME			"icm42607_acc"
#define ICM42607_GYRO_INPUT_NAME			"icm42607_gyr"
#define ICM42607_ACC_8G_MIN				(-8 * 1024)
#define ICM42607_ACC_8G_MAX				(8 * 1024)
#define ICM42607_GYRO_DPS_MIN			(-1024)
#define ICM42607_GYRO_DPS_MAX			(1024)

#define ICM42607_MAX_DELAY				2000
#define POLL_INTERVAL_MAX				1000
#define POLL_INTERVAL					50


static const uint8_t ICM42607_ACC_INIT_REG1				= 0x02;
static const uint8_t ICM42607_ACC_INIT_VALUE1			= 0x10;
static const uint8_t ICM42607_ACC_INIT_REG2				= 0x20;
static const uint8_t ICM42607_ACC_INIT_VALUE2			= 0x00;
static const uint8_t ICM42607_ACC_INIT_REG3				= 0x21;
static const uint8_t ICM42607_ACC_INIT_VALUE3			= 0x20;

static const uint8_t ICM42607_ACC_ENABLE_REG1			= 0x21;
static const uint8_t ICM42607_ACC_ENABLE_VALUE1			= 0x20;
static const uint8_t ICM42607_ACC_ENABLE_REG2			= 0x1F;
static const uint8_t ICM42607_ACC_ENABLE_VALUE2			= 0x03;

static const uint8_t ICM42607_ACC_DISABLE_REG			= 0x1F;
static const uint8_t ICM42607_ACC_DISABLE_VALUE			= 0x00;

static const uint8_t ICM42607_ACC_RAW_DATA_X_REG_L		= 0x0c;
static const uint8_t ICM42607_ACC_RAW_DATA_X_REG_H		= 0x0b;
static const uint8_t ICM42607_ACC_RAW_DATA_Y_REG_L		= 0x0e;
static const uint8_t ICM42607_ACC_RAW_DATA_Y_REG_H		= 0x0d;
static const uint8_t ICM42607_ACC_RAW_DATA_Z_REG_L		= 0x10;
static const uint8_t ICM42607_ACC_RAW_DATA_Z_REG_H		= 0x0f;

static const uint8_t ICM42607_ACC_DATA_RATE_REG			= 0x21;
static const uint8_t ICM42607_ACC_DATA_RATE_200HZ		= 0x08;
static const uint8_t ICM42607_ACC_DATA_RATE_100HZ		= 0x09;
static const uint8_t ICM42607_ACC_DATA_RATE_50HZ		= 0x0a;
static const uint8_t ICM42607_ACC_DATA_RATE_25HZ		= 0x0b;
static const uint8_t ICM42607_ACC_DATA_RATE_12HZ		= 0x0c;

static const uint8_t ICM42607_ACC_CHECK_ID_REG			= 0x75;
static const uint8_t ICM42607P_ACC_CHECK_ID_DATA		= 0x60;
static const uint8_t ICM42670P_ACC_CHECK_ID_DATA		= 0x67;
static const uint8_t ICM42670T_ACC_CHECK_ID_DATA		= 0x64;
static const uint8_t ICM42670S_ACC_CHECK_ID_DATA		= 0x69;
static const uint8_t ICM42680_ACC_CHECK_ID_DATA			= 0x80;
static const uint8_t ICM42680P_ACC_CHECK_ID_DATA		= 0x3f;
static const uint8_t ICM42671P_ACC_CHECK_ID_DATA		= 0x52;
static const uint8_t ICM42671S_ACC_CHECK_ID_DATA		= 0x54;

static const uint8_t ICM42607_GYRO_INIT_REG1			= 0x02;
static const uint8_t ICM42607_GYRO_INIT_VALUE1			= 0x10;
static const uint8_t ICM42607_GYRO_INIT_REG2			= 0x20;
static const uint8_t ICM42607_GYRO_INIT_VALUE2			= 0x00;
static const uint8_t ICM42607_GYRO_INIT_REG3			= 0x21;
static const uint8_t ICM42607_GYRO_INIT_VALUE3			= 0x20;

static const uint8_t ICM42607_GYRO_ENABLE_REG1			= 0x20;
static const uint8_t ICM42607_GYRO_ENABLE_VALUE1		= 0x00;
static const uint8_t ICM42607_GYRO_ENABLE_REG2			= 0x1F;
static const uint8_t ICM42607_GYRO_ENABLE_VALUE2		= 0x0C;

static const uint8_t ICM42607_GYRO_DISABLE_REG			= 0x1F;
static const uint8_t ICM42607_GYRO_DISABLE_VALUE		= 0x00;

static const uint8_t ICM42607_GYRO_RAW_DATA_X_REG_L		= 0x12;
static const uint8_t ICM42607_GYRO_RAW_DATA_X_REG_H		= 0x11;
static const uint8_t ICM42607_GYRO_RAW_DATA_Y_REG_L		= 0x14;
static const uint8_t ICM42607_GYRO_RAW_DATA_Y_REG_H		= 0x13;
static const uint8_t ICM42607_GYRO_RAW_DATA_Z_REG_L		= 0x16;
static const uint8_t ICM42607_GYRO_RAW_DATA_Z_REG_H		= 0x15;

static const uint8_t ICM42607_GYRO_DATA_RATE_REG		= 0x20;
static const uint8_t ICM42607_GYRO_DATA_RATE_200HZ		= 0x08;
static const uint8_t ICM42607_GYRO_DATA_RATE_100HZ		= 0x09;
static const uint8_t ICM42607_GYRO_DATA_RATE_50HZ		= 0x0a;
static const uint8_t ICM42607_GYRO_DATA_RATE_25HZ		= 0x0b;
static const uint8_t ICM42607_GYRO_DATA_RATE_12HZ		= 0x0c;

static const uint8_t ICM42607_GYRO_CHECK_ID_REG			= 0x75;
static const uint8_t ICM42607P_GYRO_CHECK_ID_DATA		= 0x60;
static const uint8_t ICM42670P_GYRO_CHECK_ID_DATA		= 0x67;
static const uint8_t ICM42670T_GYRO_CHECK_ID_DATA		= 0x64;
static const uint8_t ICM42670S_GYRO_CHECK_ID_DATA		= 0x69;
static const uint8_t ICM42680_GYRO_CHECK_ID_DATA		= 0x80;
static const uint8_t ICM42680P_GYRO_CHECK_ID_DATA		= 0x3f;
static const uint8_t ICM42671P_GYRO_CHECK_ID_DATA		= 0x52;
static const uint8_t ICM42671S_GYRO_CHECK_ID_DATA		= 0x54;

// #define ICM42607_DEBUG
#if defined(ICM42607_DEBUG)
#define ICM42607_TAG				    "qst-"
#define ICM42607_FUN(f)               	printk(ICM42607_TAG"%s\n", __FUNCTION__)
#define ICM42607_ERR(fmt, args...)    	printk(ICM42607_TAG"%s(line:%d)  " fmt, __FUNCTION__, __LINE__, ##args)
#define ICM42607_LOG(fmt, args...)    	printk(ICM42607_TAG"%s(line:%d)  " fmt, __FUNCTION__, __LINE__, ##args)
#else
#define ICM42607_FUN()
#define ICM42607_LOG(fmt, args...)
#define ICM42607_ERR(fmt, args...)
#endif

enum icm42607_type {
	ICM42607_TYPE_NONE = 0x00,
	ICM42607_TYPE_ACC  = 0x01,
	ICM42607_TYPE_GYRO = 0x02,

	ICM42607_TYPE_MAX = 0xff
};

typedef struct {
	int x;
	int y;
	int z;
} icm42607_axis;

struct icm42607_data {
	struct i2c_client		*client;

	struct input_dev 			*acc_input;
	struct input_dev 			*gyro_input;

	struct workqueue_struct 	*qmi_wq;

	struct hrtimer				acc_hr_timer;
	struct work_struct			acc_work;
	ktime_t						acc_ktime;

	struct hrtimer				gyro_hr_timer;
	struct work_struct			gyro_work;
	ktime_t						gyro_ktime;

	struct mutex 				op_mutex;

	atomic_t 					acc_enable;
	atomic_t 					acc_delay;
	atomic_t 					gyro_enable;
	atomic_t 					gyro_delay;

	atomic_t 					need_init;

	icm42607_axis				acc_out;
	icm42607_axis				gyro_out;

#if defined(CONFIG_PM)
	volatile int suspend_indator;
#endif
	struct hrtimer hr_timer;
	struct work_struct wq_hrtimer;
	ktime_t ktime;
};

/* Addresses to scan */
static const unsigned short normal_i2c[] = {0x6a, 0x6b, I2C_CLIENT_END};
static __u32 twi_id;

static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
	.np_name = ICM42607_DEV_NAME,
};

static struct icm42607_data *icm42607;

static int icm42607_reg_init(struct i2c_client *client);
static void icm42607_acc_enable(int enable);
static void icm42607_gyro_enable(int enable);

// static int icm42607_write_reg(unsigned char reg, unsigned char value)
// {
// 	int ret = 0;

// 	mutex_lock(&icm42607->op_mutex);
// 	ret = i2c_smbus_write_byte_data(icm42607->client, reg, value);
// 	mutex_unlock(&icm42607->op_mutex);
// 	if (ret < 0)
// 		return ret;
// 	else
// 		return 0;
// }

static int icm42607_read_reg(unsigned char reg, unsigned char *buf, unsigned char len)
{
	int ret = 0;

	mutex_lock(&icm42607->op_mutex);
	if (len == 1)
		*buf = i2c_smbus_read_byte_data(icm42607->client, reg);
	else
		ret = i2c_smbus_read_i2c_block_data(icm42607->client, reg, len, buf);

	mutex_unlock(&icm42607->op_mutex);
	if (ret < 0)
		return ret;
	else
		return 0;
}

static int icm42607_gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

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
			printk("%s: I2C connection sucess!\n", __func__);
			strlcpy(info->type, ICM42607_DEV_NAME, I2C_NAME_SIZE);
			return 0;
		}
	} else {
		printk("%s: I2C connection error!\n",  __func__);
		return -ENODEV;
	}
}

static void icm42607_set_delay(enum icm42607_type type, int delay)
{
	ICM42607_FUN();
	if (type == ICM42607_TYPE_ACC) {
		atomic_set(&icm42607->acc_delay, delay);
		icm42607->acc_ktime = ktime_set(0, (delay - 1) * NSEC_PER_MSEC);
	} else if (type == ICM42607_TYPE_GYRO) {
		atomic_set(&icm42607->gyro_delay, delay);
		icm42607->gyro_ktime = ktime_set(0, (delay - 1) * NSEC_PER_MSEC);
	}
}

static int icm42607_acc_read_raw(short raw_xyz[3])
{
	unsigned char buf_reg[6];
	int ret = 0;

	ret = icm42607_read_reg(ICM42607_ACC_RAW_DATA_X_REG_H, buf_reg, 6);

	raw_xyz[0] = (short)(buf_reg[1] | (buf_reg[0]<<8));
	raw_xyz[1] = (short)(buf_reg[3] | (buf_reg[2]<<8));
	raw_xyz[2] = (short)(buf_reg[5] | (buf_reg[4]<<8));
	//ICM42607_LOG("acc_raw [%d %d	%d]\n", raw_xyz[0],raw_xyz[1],raw_xyz[2]);

	return ret;
}

static int icm42607_read_accel_xyz(icm42607_axis *acc)
{
	int res = 0;

	short raw_xyz[3];

	res = icm42607_acc_read_raw(raw_xyz);
	acc->x = raw_xyz[0];
	acc->y = raw_xyz[1];
	acc->z = raw_xyz[2];

	return res;
}

static int icm42607_gyro_read_raw(short raw_xyz[3])
{
	unsigned char buf_reg[6];
	int ret = 0;

	ret = icm42607_read_reg(ICM42607_GYRO_RAW_DATA_X_REG_H, buf_reg, 6);

	raw_xyz[0] = (short)(buf_reg[1] | (buf_reg[0]<<8));
	raw_xyz[1] = (short)(buf_reg[3] | (buf_reg[2]<<8));
	raw_xyz[2] = (short)(buf_reg[5] | (buf_reg[4]<<8));
	//ICM42607_LOG("gyro_raw [%d %d %d]\n", raw_xyz[0],raw_xyz[1],raw_xyz[2]);
    // if (raw_xyz[0] == -32768)
	// 	return -1;

	return ret;
}

static int icm42607_read_gyro_xyz(icm42607_axis *gyro)
{
	int res = 0;
	short raw_xyz[3];

	res = icm42607_gyro_read_raw(raw_xyz);
	gyro->x = raw_xyz[0];
	gyro->y = raw_xyz[1];
	gyro->z = raw_xyz[2];

	return res;
}

static enum hrtimer_restart acc_hrtimer_callback(struct hrtimer *timer)
{
	queue_work(icm42607->qmi_wq, &icm42607->acc_work);
	hrtimer_forward_now(&icm42607->acc_hr_timer, icm42607->acc_ktime);
	return HRTIMER_RESTART;
}

static void acc_work_func(struct work_struct *work)
{
	icm42607_axis acc = { 0 };
	int is_needinit = 0;
	/* unsigned char status0 = 0, count = 0 */;

	is_needinit = atomic_read(&icm42607->need_init);
	if (is_needinit) {
		atomic_set(&icm42607->need_init, 0);
		goto reg_init;
	}

#if 0
	while (!(status0 & 0x01) && (count++ < 20)) {
		icm42607_read_reg(ICM42607_REG_STATUSINT, &status0, 1);
	}
#endif

	icm42607_read_accel_xyz(&acc);

	icm42607->acc_out.x = acc.x;
	icm42607->acc_out.y = acc.y;
	icm42607->acc_out.z = acc.z;

	input_report_abs(icm42607->acc_input, ABS_X, acc.x);
	input_report_abs(icm42607->acc_input, ABS_Y, acc.y);
	input_report_abs(icm42607->acc_input, ABS_Z, acc.z);
	input_sync(icm42607->acc_input);
	ICM42607_LOG(":[%d %d %d ]\n",acc.x, acc.y, acc.z);
	return;

reg_init:
	icm42607_reg_init(icm42607->client);
	if (atomic_read(&icm42607->acc_enable)) {
		icm42607_acc_enable(1);
	}
	if (atomic_read(&icm42607->gyro_enable)) {
		icm42607_gyro_enable(1);
	}
}


static enum hrtimer_restart gyro_hrtimer_callback(struct hrtimer *timer)
{
	queue_work(icm42607->qmi_wq, &icm42607->gyro_work);
	hrtimer_forward_now(&icm42607->gyro_hr_timer, icm42607->gyro_ktime);
	return HRTIMER_RESTART;
}

static void gyro_work_func(struct work_struct *work)
{
	int ret = 0;
	int is_needinit = 0;
	icm42607_axis gyro = { 0 };
	/* unsigned char status0 = 0, count = 0 */;

	is_needinit = atomic_read(&icm42607->need_init);
	if (is_needinit) {
		atomic_set(&icm42607->need_init, 0);
		goto reg_init;
	}

#if 0
	while ((!(status0 & 0x01) && (count++ < 20))) {
		icm42607_read_reg(ICM42607_REG_STATUSINT, &status0, 1);
	}
#endif

	ret = icm42607_read_gyro_xyz(&gyro);
	if (ret < 0) {
		ICM42607_LOG("read gyro data error, reinit");
		goto reg_init;
	}

	icm42607->gyro_out.x = gyro.x;
	icm42607->gyro_out.y = gyro.y;
	icm42607->gyro_out.z = gyro.z;

	input_report_abs(icm42607->gyro_input, ABS_X, gyro.x);
	input_report_abs(icm42607->gyro_input, ABS_Y, gyro.y);
	input_report_abs(icm42607->gyro_input, ABS_Z, gyro.z);
	input_sync(icm42607->gyro_input);

	ICM42607_LOG(":[%d %d %d ]\n",gyro.x,gyro.y,gyro.z);
	return;

reg_init:
	icm42607_reg_init(icm42607->client);
	if (atomic_read(&icm42607->acc_enable)) {
		icm42607_acc_enable(1);
	}
	if (atomic_read(&icm42607->gyro_enable)) {
		icm42607_gyro_enable(1);
	}
}

static int icm42607_input_init(struct icm42607_data *icm42607)
{
	struct input_dev *acc_dev, *gyro_dev;
	int err = 0;

	ICM42607_LOG("-----acc input init\n");
	acc_dev = input_allocate_device();
	if (!acc_dev) {
		ICM42607_ERR("acc input can't allocate device!\n");
		return -ENOMEM;
	}

	acc_dev->name = ICM42607_ACC_INPUT_NAME;
	acc_dev->id.bustype = BUS_I2C;
	input_set_capability(acc_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(acc_dev, ABS_X, ICM42607_ACC_8G_MIN, ICM42607_ACC_8G_MAX, 0, 0);
	input_set_abs_params(acc_dev, ABS_Y, ICM42607_ACC_8G_MIN, ICM42607_ACC_8G_MAX, 0, 0);
	input_set_abs_params(acc_dev, ABS_Z, ICM42607_ACC_8G_MIN, ICM42607_ACC_8G_MAX, 0, 0);
	input_set_drvdata(acc_dev, icm42607);

	err = input_register_device(acc_dev);
	if (err < 0) {
		ICM42607_ERR("acc input can't register device!\n");
		input_free_device(acc_dev);
		return err;
	}
	icm42607->acc_input = acc_dev;

	ICM42607_LOG("-------gyro input init\n");
	gyro_dev = input_allocate_device();
	if (!gyro_dev) {
		input_unregister_device(icm42607->acc_input);
		ICM42607_ERR("gyro input can't allocate device!\n");
		return -ENOMEM;
	}
	gyro_dev->name = ICM42607_GYRO_INPUT_NAME;
	gyro_dev->id.bustype = BUS_I2C;
	input_set_capability(gyro_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(gyro_dev, ABS_X, ICM42607_GYRO_DPS_MIN, ICM42607_GYRO_DPS_MAX, 0, 0);
	input_set_abs_params(gyro_dev, ABS_Y, ICM42607_GYRO_DPS_MIN, ICM42607_GYRO_DPS_MAX, 0, 0);
	input_set_abs_params(gyro_dev, ABS_Z, ICM42607_GYRO_DPS_MIN, ICM42607_GYRO_DPS_MAX, 0, 0);
	input_set_drvdata(gyro_dev, (void *)icm42607);

	err = input_register_device(gyro_dev);
	if (err < 0) {
		ICM42607_ERR("gyro input can't register device!\n");
		input_unregister_device(icm42607->acc_input);
		input_free_device(gyro_dev);
		return err;
	}
	icm42607->gyro_input = gyro_dev;

	return 0;
}

static void icm42607_input_deinit(struct icm42607_data *icm42607)
{
	ICM42607_LOG("called\n");
	if (icm42607->acc_input) {
		input_unregister_device(icm42607->acc_input);
		icm42607->acc_input = NULL;
	}

	if (icm42607->gyro_input) {
		input_unregister_device(icm42607->gyro_input);
		icm42607->gyro_input = NULL;
	}
}

static void icm42607_acc_enable(int enable)
{
	int res = 0;
	unsigned char reg_value = 0;
	struct i2c_client *client = icm42607->client;

	if (enable) {
		reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_INIT_REG2);
		reg_value |= (ICM42607_ACC_INIT_VALUE2 & 0xE0);
		res = i2c_smbus_write_byte_data(client, ICM42607_ACC_INIT_REG2, reg_value);

		reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_INIT_REG3);
		reg_value |= (ICM42607_ACC_INIT_VALUE3 & 0xE0);
		res = i2c_smbus_write_byte_data(client, ICM42607_ACC_INIT_REG3, reg_value);

		reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_ENABLE_REG1);
		reg_value |= (ICM42607_ACC_ENABLE_VALUE1 & 0xE0);
		res = i2c_smbus_write_byte_data(client, ICM42607_ACC_ENABLE_REG1, reg_value);

		reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_ENABLE_REG2);
		reg_value |= (ICM42607_ACC_ENABLE_VALUE2 & 0x03);
		res = i2c_smbus_write_byte_data(client, ICM42607_ACC_ENABLE_REG2, reg_value);
	}
	else {
		reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_DISABLE_REG);
		reg_value |= (ICM42607_ACC_DISABLE_VALUE & 0x03);
		res = i2c_smbus_write_byte_data(client, ICM42607_ACC_DISABLE_REG, reg_value);
	}
}

static void icm42607_gyro_enable(int enable)
{
	int res = 0;
	unsigned char reg_value = 0;
	struct i2c_client *client = icm42607->client;

	if (enable) {
		reg_value = i2c_smbus_read_byte_data(client, ICM42607_GYRO_INIT_REG2);
		reg_value |= (ICM42607_GYRO_INIT_VALUE2 & 0xE0);
		res = i2c_smbus_write_byte_data(client, ICM42607_GYRO_INIT_REG2, reg_value);

		reg_value = i2c_smbus_read_byte_data(client, ICM42607_GYRO_INIT_REG3);
		reg_value |= (ICM42607_GYRO_INIT_VALUE3 & 0xE0);
		res = i2c_smbus_write_byte_data(client, ICM42607_GYRO_INIT_REG3, reg_value);

		reg_value = i2c_smbus_read_byte_data(client, ICM42607_GYRO_ENABLE_REG1);
		reg_value |= (ICM42607_GYRO_ENABLE_VALUE1 & 0xE0);
		res = i2c_smbus_write_byte_data(client, ICM42607_GYRO_ENABLE_REG1, reg_value);

		reg_value = i2c_smbus_read_byte_data(client, ICM42607_GYRO_ENABLE_REG2);
		reg_value |= (ICM42607_GYRO_ENABLE_VALUE2 & 0x0C);
		res = i2c_smbus_write_byte_data(client, ICM42607_GYRO_ENABLE_REG2, reg_value);
	}
	else {
		reg_value = i2c_smbus_read_byte_data(client, ICM42607_GYRO_DISABLE_REG);
		reg_value |= (ICM42607_GYRO_DISABLE_VALUE & 0x0C);
		res = i2c_smbus_write_byte_data(client, ICM42607_GYRO_DISABLE_REG, reg_value);
	}
}

static int icm42607_set_enable(enum icm42607_type type, int enable)
{
	ICM42607_LOG("type:%d--enable :%d\n", type, enable);

	if (type == ICM42607_TYPE_ACC) {
		atomic_set(&icm42607->acc_enable, enable);
		icm42607_acc_enable(enable);
		if (enable) {
			icm42607->acc_ktime = ktime_set(0, (atomic_read(&icm42607->acc_delay) - 1) * NSEC_PER_MSEC);
			hrtimer_start(&icm42607->acc_hr_timer, icm42607->acc_ktime, HRTIMER_MODE_REL);
		} else {
			hrtimer_cancel(&icm42607->acc_hr_timer);
		}
	} else if (type == ICM42607_TYPE_GYRO) {
		atomic_set(&icm42607->gyro_enable, enable);
		icm42607_gyro_enable(enable);
		if (enable) {
			icm42607->gyro_ktime = ktime_set(0, (atomic_read(&icm42607->gyro_delay) - 1) * NSEC_PER_MSEC);
			hrtimer_start(&icm42607->gyro_hr_timer, icm42607->gyro_ktime, HRTIMER_MODE_REL);
		} else {
			hrtimer_cancel(&icm42607->gyro_hr_timer);
		}
	}

	return 0;
}

static ssize_t show_sensordata_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	icm42607_read_accel_xyz(&icm42607->acc_out);
	icm42607_read_gyro_xyz(&icm42607->gyro_out);
	return sprintf(buf, "%d %d %d	%d %d %d\n", icm42607->acc_out.x, icm42607->acc_out.y, icm42607->acc_out.z,
					icm42607->gyro_out.x, icm42607->gyro_out.y, icm42607->gyro_out.z);
}

static ssize_t show_dumpallreg_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	int res;
	int i = 0;
	char strbuf[600];
	char tempstrbuf[24];
	unsigned char databuf[2] = {0};
	int length = 0;

	ICM42607_FUN();
	/* Check status register for data availability */
	for (i = 0; i <= 50; i++) {
		databuf[0] = i;
		res = icm42607_read_reg(databuf[0], &databuf[1], 1);
		if (res < 0)
			ICM42607_LOG("qma8658 dump registers 0x%02x failed !\n", i);

		length = scnprintf(tempstrbuf, sizeof(tempstrbuf), "0x%2x=0x%2x\n", databuf[0], databuf[1]);
		snprintf(strbuf + length * i, sizeof(strbuf) - (length * i), "%s", tempstrbuf);
	}

	return scnprintf(buf, sizeof(strbuf), "%s\n", strbuf);
}

static ssize_t icm42607_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ICM42607_FUN();

	return sprintf(buf, "acc:%d gyro:%d\n", atomic_read(&icm42607->acc_enable), atomic_read(&icm42607->gyro_enable));
}

static ssize_t icm42607_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int type, enable = 0;

	ICM42607_FUN();

	if (2 == sscanf(buf, "%d %d", &type, &enable)) {
		if ((type == ICM42607_TYPE_ACC) || (type == ICM42607_TYPE_GYRO)) {
			if (enable) {
				icm42607_set_enable(type, 1);
			} else {
				icm42607_set_enable(type, 0);
			}
		}
	} else {
		ICM42607_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t icm42607_acc_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enable = 0;

	ICM42607_FUN();

	if (1 == sscanf(buf, "%d", &enable)) {
		if (enable) {
			icm42607_set_enable(ICM42607_TYPE_ACC, 1);
		} else {
			icm42607_set_enable(ICM42607_TYPE_ACC, 0);
		}
	} else {
		ICM42607_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t icm42607_gyro_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enable = 0;

	ICM42607_FUN();

	if (1 == sscanf(buf, "%d", &enable)) {
		if (enable) {
			icm42607_set_enable(ICM42607_TYPE_GYRO, 1);
		} else {
			icm42607_set_enable(ICM42607_TYPE_GYRO, 0);
		}
	} else {
		ICM42607_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t icm42607_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ICM42607_FUN();
	return sprintf(buf, "acc:%d gyro:%d\n", atomic_read(&icm42607->acc_delay), atomic_read(&icm42607->gyro_delay));
}

static ssize_t icm42607_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int type, delay;

	ICM42607_FUN();
	if (2 == sscanf(buf, "%d %d", &type, &delay)) {
		if (delay > ICM42607_MAX_DELAY) {
			delay = ICM42607_MAX_DELAY;
		} else if (delay <= 1) {
			delay = 1;
		}

		if ((type == ICM42607_TYPE_ACC) || (type == ICM42607_TYPE_GYRO)) {
			icm42607_set_delay(type, delay);
		}
	} else {
		ICM42607_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t icm42607_acc_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int delay;

	ICM42607_FUN();
	if (1 == sscanf(buf, "%d", &delay)) {
		if (delay > ICM42607_MAX_DELAY) {
			delay = ICM42607_MAX_DELAY;
		} else if (delay <= 1) {
			delay = 1;
		}

		icm42607_set_delay(ICM42607_TYPE_ACC, delay);
	} else {
		ICM42607_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t icm42607_gyro_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int delay;

	ICM42607_FUN();
	if (1 == sscanf(buf, "%d", &delay)) {
		if (delay > ICM42607_MAX_DELAY) {
			delay = ICM42607_MAX_DELAY;
		} else if (delay <= 1) {
			delay = 1;
		}

		icm42607_set_delay(ICM42607_TYPE_GYRO, delay);
	} else {
		ICM42607_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static DEVICE_ATTR(sensordata,	0644, show_sensordata_value, NULL);
static DEVICE_ATTR(dumpallreg,	0644, show_dumpallreg_value, NULL);
static DEVICE_ATTR(enable,		0644, icm42607_enable_show, icm42607_enable_store);
static DEVICE_ATTR(acc_enable,	0644, NULL, icm42607_acc_enable_store);
static DEVICE_ATTR(gyro_enable,	0644, NULL, icm42607_gyro_enable_store);
static DEVICE_ATTR(delay,		0644, icm42607_delay_show, icm42607_delay_store);
static DEVICE_ATTR(acc_delay,	0644, NULL, icm42607_acc_delay_store);
static DEVICE_ATTR(gyro_delay,	0644, NULL, icm42607_gyro_delay_store);

static struct attribute *icm42607_attributes[] = {
	&dev_attr_sensordata.attr,
	&dev_attr_dumpallreg.attr,
	&dev_attr_enable.attr,
	&dev_attr_acc_enable.attr,
	&dev_attr_gyro_enable.attr,
	&dev_attr_delay.attr,
	&dev_attr_acc_delay.attr,
	&dev_attr_gyro_delay.attr,
	NULL
};

static struct attribute_group icm42607_attribute_group = {
	.attrs = icm42607_attributes
};

static int icm42607_reg_init(struct i2c_client *client)
{
	int res = 0;
	unsigned char reg_value = 0;

    reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_CHECK_ID_REG);
	if (reg_value != ICM42607P_ACC_CHECK_ID_DATA) {
		printk("%s:check id err,reg_value:%d\n", __func__, reg_value);
		return -1;
	} else {
		printk("%s:check id success -->icm42607\n", __func__);
	}

    //icm42607 reset
	reg_value = i2c_smbus_read_byte_data(client, ICM42607_ACC_INIT_REG1);
	reg_value |= (ICM42607_ACC_INIT_VALUE1 & 0x10);
	res = i2c_smbus_write_byte_data(client, ICM42607_ACC_INIT_REG1, reg_value);
	msleep(100);

	return res;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int icm42607_i2c_probe(struct i2c_client *client)
#else
static int icm42607_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	int err = 0;
	struct icm42607_data *data;

	ICM42607_LOG("start\n");

	if (gsensor_info.dev == NULL)
		gsensor_info.dev = &client->dev;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C|I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA)) {
		ICM42607_ERR("%s: check_functionality failed.", __func__);
		err = -ENODEV;
		goto exit;
	}

	data = kzalloc(sizeof(struct icm42607_data), GFP_KERNEL);
	if (!data) {
		ICM42607_ERR("%s: can't allocate memory for icm42607_Data!\n", __func__);
		err = -ENOMEM;
		goto exit;
	}
	icm42607 = data;
	data->client = client;
	mutex_init(&data->op_mutex);
	i2c_set_clientdata(client, icm42607);

	err = icm42607_reg_init(client);
	if (err < 0) {
		ICM42607_ERR("%s: g_icm42607 read id fail!\n", __func__);
		goto exit1;
	}

	atomic_set(&icm42607->acc_delay, (unsigned int) POLL_INTERVAL_MAX);
	atomic_set(&icm42607->gyro_delay, (unsigned int) POLL_INTERVAL_MAX);

	icm42607->qmi_wq = create_singlethread_workqueue("icm42607_wq");
	if (!icm42607->qmi_wq) {
		ICM42607_ERR("%s: g_icm42607 create work queue fail!\n", __func__);
		goto exit1;
	}
	flush_workqueue(icm42607->qmi_wq);

	hrtimer_init(&icm42607->acc_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	icm42607->acc_hr_timer.function = acc_hrtimer_callback;
	INIT_WORK(&icm42607->acc_work, acc_work_func);

	hrtimer_init(&icm42607->gyro_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	icm42607->gyro_hr_timer.function = gyro_hrtimer_callback;
	INIT_WORK(&icm42607->gyro_work, gyro_work_func);

	err = icm42607_input_init(icm42607);
	if (err < 0) {
		ICM42607_ERR("input init fail!\n");
		goto exit1;
	}

	err = sysfs_create_group(&icm42607->acc_input->dev.kobj, &icm42607_attribute_group);
	err += sysfs_create_group(&icm42607->gyro_input->dev.kobj, &icm42607_attribute_group);
	if (err < 0) {
		ICM42607_ERR("%s: create group fail!\n", __func__);
		goto exit2;
	}
	kobject_uevent(&icm42607->acc_input->dev.kobj, KOBJ_CHANGE);
	kobject_uevent(&icm42607->gyro_input->dev.kobj, KOBJ_CHANGE);

	printk("icm42607 successfully probed\n");

	return 0;

exit2:
	icm42607_input_deinit(icm42607);
exit1:
	if (icm42607) {
		kfree(icm42607);
		icm42607 = NULL;
	}
exit:
	return err;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
#else
static int icm42607_remove(struct i2c_client *client)
{
	ICM42607_FUN();
	if (icm42607) {
		sysfs_remove_group(&icm42607->acc_input->dev.kobj, &icm42607_attribute_group);
		sysfs_remove_group(&icm42607->gyro_input->dev.kobj, &icm42607_attribute_group);

		icm42607_set_enable(ICM42607_TYPE_ACC, 0);
		icm42607_set_enable(ICM42607_TYPE_GYRO, 0);

		cancel_work_sync(&icm42607->acc_work);
		cancel_work_sync(&icm42607->gyro_work);

		destroy_workqueue(icm42607->qmi_wq);

		icm42607_input_deinit(icm42607);
		kfree(icm42607);
		icm42607 = NULL;
	}
	return 0;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void icm42607_i2c_remove(struct i2c_client *client)
#else
static int icm42607_i2c_remove(struct i2c_client *client)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    return;
#else
	return icm42607_remove(client);
#endif
}


static void icm42607_shutdown(struct i2c_client *client)
{
	ICM42607_FUN();

	if (icm42607) {
		icm42607_set_enable(ICM42607_TYPE_ACC, 0);
		icm42607_set_enable(ICM42607_TYPE_GYRO, 0);
	}
}

static int icm42607_suspend(struct device *dev)
{
	ICM42607_FUN();

	if (atomic_read(&icm42607->acc_enable)) {
		hrtimer_cancel(&icm42607->acc_hr_timer);
	}

	if (atomic_read(&icm42607->gyro_enable)) {
		hrtimer_cancel(&icm42607->gyro_hr_timer);
	}

	input_set_power_enable(&(gsensor_info.input_type), 0);

	return 0;
}

static int icm42607_resume(struct device *dev)
{
	ICM42607_FUN();
	input_set_power_enable(&(gsensor_info.input_type), 1);
	atomic_set(&icm42607->need_init, 1);

	if (atomic_read(&icm42607->acc_enable)) {
		hrtimer_start(&icm42607->acc_hr_timer, icm42607->acc_ktime, HRTIMER_MODE_REL);
		//schedule_delayed_work(&icm42607->acc_work, msecs_to_jiffies(atomic_read(&icm42607->acc_delay) - 1));
	}

	if (atomic_read(&icm42607->gyro_enable)) {
		hrtimer_start(&icm42607->gyro_hr_timer, icm42607->gyro_ktime, HRTIMER_MODE_REL);
		//schedule_delayed_work(&icm42607->gyro_work, msecs_to_jiffies(atomic_read(&icm42607->gyro_delay) - 1));
	}

	return 0;
}


static const struct i2c_device_id icm42607_id[] = {
	{ICM42607_DEV_NAME, 0},
	{ }
};

MODULE_DEVICE_TABLE(i2c, icm42607_id);

#ifdef CONFIG_OF
static const struct of_device_id icm42607_of_match[] = {
       { .compatible = "allwinner,icm42607", },
       { }
};
#endif
MODULE_DEVICE_TABLE(of, icm42607_of_match);

#if IS_ENABLED(CONFIG_PM)
static UNIVERSAL_DEV_PM_OPS(icm42607_pm_ops, icm42607_suspend,
		icm42607_resume, NULL);
#endif

static struct i2c_driver icm42607_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = ICM42607_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = icm42607_of_match,
#endif
#if IS_ENABLED(CONFIG_PM)
		.pm = &icm42607_pm_ops,
#endif
	},
	.probe    = icm42607_i2c_probe,
	.remove   = icm42607_i2c_remove,
	.id_table = icm42607_id,
	.shutdown	= icm42607_shutdown,
	.address_list = normal_i2c,
};

static int startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__, __LINE__);

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

static int __init icm42607_i2c_init(void)
{
	int ret = -1;

	ICM42607_LOG("%s A+G driver: init\n", ICM42607_DEV_NAME);
	if (startup() != 0)
		return -1;

	if (!gsensor_info.isI2CClient)
		icm42607_driver.detect = icm42607_gsensor_detect;

	ret = i2c_add_driver(&icm42607_driver);
	if (ret < 0) {
		printk("add icm42607 i2c driver failed\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit icm42607_i2c_exit(void)
{
	ICM42607_LOG("%s A+G driver exit\n", ICM42607_DEV_NAME);

	i2c_del_driver(&icm42607_driver);
	input_sensor_free(&(gsensor_info.input_type));
}

module_init(icm42607_i2c_init);
module_exit(icm42607_i2c_exit);

MODULE_DESCRIPTION("icm42607 IMU driver");
MODULE_AUTHOR("qing_lv@qstcorp.com");
MODULE_LICENSE("GPL");
MODULE_VERSION(ICM42607_DEV_VERSION);

