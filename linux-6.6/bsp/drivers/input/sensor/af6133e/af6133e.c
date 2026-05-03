/*
 * Copyright (C) 2012 MEMSIC.
 *
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
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/regmap.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/version.h>

#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/sched.h>
#include "af6133e.h"
#include "../../init-input.h"

#define POLL_INTERVAL_DEFAULT 20

#define AF6133E_MIN_DELAY 5
#define AF6133E_MAX_DELAY 1000

#define AF6133E_SINGLE_POWER 0

#define AF6133E_DELAY_TM_MS    10
#define AF6133E_DELAY_SET  75
#define AF6133E_DELAY_RESET 75
#define AF6133E_RETRY_COUNT    10
#define AF6133E_DEFAULT_INTERVAL_MS    20
#define AF6133E_TIMEOUT_SET_MS 15000

#define MISC_DEVICE_REGISTER 0

#define MSE_TAG                 "AF6133E"
#define MSE_FUN(f)              printk(MSE_TAG" %s\r\n", __FUNCTION__)
#define MSE_ERR(fmt, args...)   printk(KERN_ERR MSE_TAG" %s %d : \r\n"fmt, __FUNCTION__, __LINE__, ##args)
#define MSE_LOG(fmt, args...)   printk(MSE_TAG fmt, ##args)
#define MSE_VER(fmt, args...)   ((void)0)

struct AF6133E_data {
	uint8_t sensor_state;
	struct i2c_client *i2c;
	struct input_dev *idev;
	struct hrtimer work_timer;
	struct work_struct work;
	struct workqueue_struct *wq;
	bool hrtimer_running;
	int poll_interval;
	struct mutex lock;
	atomic_t enabled;
	unsigned int on_before_suspend;
};

static struct i2c_client *this_client;
static int16_t af6133e_offset[3];
static int16_t af6133e_gain[3];
static int16_t af6133e_comp[4];
static int16_t af6133e_bist_x[3];
static int16_t af6133e_bist_y[3];
static int16_t af6133e_bist_z[3];
int af6133e_odr;
static int af6133e_controldata[CONTROL_DATA_LEN];
#ifdef AF6133E_TEMP_COMP
int32_t af6133e_temp_t0;
int32_t af6133e_temp_dt;
int16_t af6133e_dt_pre;
uint16_t af6133e_t_count;
uint16_t af6133e_t_index;
int16_t af6133e_t_buf[TEMP_MF_NUM];
#endif

static struct sensor_config_info mag_config = {
	.input_type = MAGNETOMETER_TPYE,
	.np_name = "af6133e",
};

static uint32_t vtc_int_sqrt(uint32_t x)
{
	uint32_t small = 1;
	uint32_t low, high;
	
	if(x < 0) return -1;
	
	if(x < 1) {
		low = 0;
		high = 1;
	}
	else {
		low = 1;
		high = x;
	}
	
	while((high - low) > small) {
		uint32_t mid = (low + high) / 2;
		
		if((mid * mid) > x)
			high = mid;
		else
			low = mid;
	}
	
	return low;
}

static int VTC_i2c_Rx(struct i2c_client *client, char *rxData, int length)
{
	uint8_t retry;
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = client->addr,
            .flags = I2C_M_RD,
            .len = length,
            .buf = rxData,
        },
    };

    for (retry = 0; retry < 3; retry++) {
        if (i2c_transfer(client->adapter, msgs, 2) > 0)
            break;
        else
            mdelay(10);
    }

    if (retry >= 3) {
        printk(KERN_ERR "%s: retry over 3\n", __func__);
        return -EIO;
    }
    else
        return 0;
}

static int VTC_i2c_Tx(struct i2c_client *client, char *txData, int length)
{
    int retry;
    struct i2c_msg msg[] = {
        {
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
        },
    };

    for (retry = 0; retry <= 3; retry++) {
        if (i2c_transfer(client->adapter, msg, 1) > 0)
            break;
        else
            mdelay(10);
    }

    if (retry > 3) {
        printk(KERN_ERR "%s: retry over 3\n", __func__);
        return -EIO;
    }

    return 0;
}

int af6133e_i2c_master_operate(struct i2c_client *client, char *buf, int count, int i2c_flag)
{
	if(i2c_flag == I2C_FLAG_READ) {
	  	return (VTC_i2c_Rx(client, buf, count>>8));
	}
	else if(i2c_flag == I2C_FLAG_WRITE) {
	  	return (VTC_i2c_Tx(client, buf, count));
	}

	return 0;
}

static int af6133e_GetBistData(int16_t *bist_X, int16_t *bist_Y, int16_t *bist_Z)
{
	int16_t i;
	int16_t mag[3];
	uint8_t databuf[6];
	int32_t tmp_pos[3], tmp_neg[3];
	int16_t bist_loop_count = MAG_BIST_LOOP;
	int16_t bist_retry_count = BIST_RETRY_NUM;
	int16_t pos_count, neg_count;
	
	if(NULL == this_client) {
		return -1;
	}

	// open 8M
	databuf[0] = REG_CHOPPER;
	databuf[1] = 0xF1; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	/* Measure BIST X */
	for(i = 0; i < 3; i++) {
		tmp_neg[i] = 0;
		tmp_pos[i] = 0;
	}

	// XPos
	databuf[0] = REG_TEST2;
	databuf[1] = 0x04;     
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	mdelay(1);
	bist_loop_count = MAG_BIST_LOOP;
	bist_retry_count = BIST_RETRY_NUM;

	do {
		databuf[0] = REG_MEASURE;
		databuf[1] = 0x01;     
		af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

		mdelay(6);

		databuf[0] = REG_DATA;
		af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
	
		mag[0] = (databuf[1] << 8) | databuf[0];
		mag[1] = (databuf[3] << 8) | databuf[2];
		mag[2] = (databuf[5] << 8) | databuf[4];
		
		if(af6133e_controldata[2] & 0x01) {
		printk("%s XPos: %d, %d, %d\n", __func__, mag[0], mag[1], mag[2]);
		}
		
		if(mag[0] <  16383 && mag[1] <  16383 && mag[2] <  16383 &&
		mag[0] > -16384 && mag[1] > -16384 && mag[2] > -16384) {
			tmp_pos[0] += (int32_t)(mag[0]);
			tmp_pos[1] += (int32_t)(mag[1]);
			tmp_pos[2] += (int32_t)(mag[2]);
			bist_loop_count--;
		}
		else {
			bist_retry_count--;
		}
	}
	while(bist_loop_count && bist_retry_count);
  
	pos_count = bist_loop_count;

	// XNeg
	databuf[0] = REG_TEST2;
	databuf[1] = 0x0C;     
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	mdelay(1);
	bist_loop_count = MAG_BIST_LOOP;
	bist_retry_count = BIST_RETRY_NUM;

	do {
		databuf[0] = REG_MEASURE;
		databuf[1] = 0x01;     
		af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

		mdelay(6);

		databuf[0] = REG_DATA;
		af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
	
		mag[0] = (databuf[1] << 8) | databuf[0];
		mag[1] = (databuf[3] << 8) | databuf[2];
		mag[2] = (databuf[5] << 8) | databuf[4];
    
		if(af6133e_controldata[2] & 0x01) {
			printk("%s XNeg: %d, %d, %d\n", __func__, mag[0], mag[1], mag[2]);
		}
    
		if(mag[0] <  16383 && mag[1] <  16383 && mag[2] <  16383 &&
		mag[0] > -16384 && mag[1] > -16384 && mag[2] > -16384) {
			tmp_neg[0] += (int32_t)(mag[0]);
			tmp_neg[1] += (int32_t)(mag[1]);
			tmp_neg[2] += (int32_t)(mag[2]);
			bist_loop_count--;
    	}
    	else {
      		bist_retry_count--;
		}
  	}
	while(bist_loop_count && bist_retry_count);
  
	neg_count = bist_loop_count;
  
	if(pos_count != MAG_BIST_LOOP && neg_count != MAG_BIST_LOOP) {
		for(i=0;i<3;i++)
			bist_X[i] = (int16_t)(tmp_neg[i]/(MAG_BIST_LOOP-neg_count) - tmp_pos[i]/(MAG_BIST_LOOP-pos_count));
	}
	else {
    	for(i=0;i<3;i++) bist_X[i] = 0;
    
		if(af6133e_controldata[2] & 0x01) {
			printk("%s bist_X fail: pos_count:%d, neg_count:%d\n", __func__, pos_count, neg_count);
		}
		goto BIST_END;
	}

	if(af6133e_controldata[2] & 0x01) {
		printk("%s bist_X: %d, %d, %d\n", __func__, bist_X[0], bist_X[1], bist_X[2]);
	}

	/* Measure BIST Y */
	for(i = 0; i < 3; i++) {
		tmp_neg[i] = 0;
		tmp_pos[i] = 0;
	}

	// YPos
	databuf[0] = REG_TEST2;
	databuf[1] = 0x02;     
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	mdelay(1);
	bist_loop_count = MAG_BIST_LOOP;
	bist_retry_count = BIST_RETRY_NUM;
  
	do {
		databuf[0] = REG_MEASURE;
		databuf[1] = 0x01;     
		af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

		mdelay(6);

		databuf[0] = REG_DATA;
		af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
	
		mag[0] = (databuf[1] << 8) | databuf[0];
		mag[1] = (databuf[3] << 8) | databuf[2];
		mag[2] = (databuf[5] << 8) | databuf[4];
    
    	if(af6133e_controldata[2] & 0x01) {
      		printk("%s YPos: %d, %d, %d\n", __func__, mag[0], mag[1], mag[2]);
		}

		if(mag[0] <  16383 && mag[1] <  16383 && mag[2] <  16383 &&
		mag[0] > -16384 && mag[1] > -16384 && mag[2] > -16384) {
			tmp_pos[0] += (int32_t)(mag[0]);
			tmp_pos[1] += (int32_t)(mag[1]);
			tmp_pos[2] += (int32_t)(mag[2]);
			bist_loop_count--;
    	}
		else {
			bist_retry_count--;
		}
	}
	while(bist_loop_count && bist_retry_count);
  
	pos_count = bist_loop_count;

	// YNeg
	databuf[0] = REG_TEST2;
	databuf[1] = 0x0A;     
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	mdelay(1);
	bist_loop_count = MAG_BIST_LOOP;
	bist_retry_count = BIST_RETRY_NUM;
  
	do {
		databuf[0] = REG_MEASURE;
		databuf[1] = 0x01;     
		af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

		mdelay(6);

		databuf[0] = REG_DATA;
		af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
	
		mag[0] = (databuf[1] << 8) | databuf[0];
		mag[1] = (databuf[3] << 8) | databuf[2];
		mag[2] = (databuf[5] << 8) | databuf[4];
    
		if(af6133e_controldata[2] & 0x01) {
			printk("%s YNeg: %d, %d, %d\n", __func__, mag[0], mag[1], mag[2]);
		}
    
		if(mag[0] <  16383 && mag[1] <  16383 && mag[2] <  16383 &&
		mag[0] > -16384 && mag[1] > -16384 && mag[2] > -16384) {
			tmp_neg[0] += (int32_t)(mag[0]);
			tmp_neg[1] += (int32_t)(mag[1]);
			tmp_neg[2] += (int32_t)(mag[2]);
			bist_loop_count--;
		}
		else {
			bist_retry_count--;
		}
	}
	while(bist_loop_count && bist_retry_count);
  
	neg_count = bist_loop_count;
 
	if(pos_count != MAG_BIST_LOOP && neg_count != MAG_BIST_LOOP) {
		for(i=0;i<3;i++)
			bist_Y[i] = (int16_t)(tmp_neg[i]/(MAG_BIST_LOOP-neg_count) - tmp_pos[i]/(MAG_BIST_LOOP-pos_count));
	}
	else {
		for(i=0;i<3;i++) bist_Y[i] = 0;
		if(af6133e_controldata[2] & 0x01) {
			printk("%s bist_Y fail: pos_count:%d, neg_count:%d\n", __func__, pos_count, neg_count);
		}
		goto BIST_END;
	}

	if(af6133e_controldata[2] & 0x01) {
		printk("%s bist_Y: %d, %d, %d\n", __func__, bist_Y[0], bist_Y[1], bist_Y[2]);
	}
    
	/* Measure BIST Z */
	for(i = 0; i < 3; i++) {
		tmp_neg[i] = 0;
		tmp_pos[i] = 0;
	}

	// ZPos
	databuf[0] = REG_TEST2;
	databuf[1] = 0x01;     
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	mdelay(1);
	bist_loop_count = MAG_BIST_LOOP;
	bist_retry_count = BIST_RETRY_NUM;

	do {
		databuf[0] = REG_MEASURE;
		databuf[1] = 0x01;     
		af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

		mdelay(6);

		databuf[0] = REG_DATA;
		af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
	
		mag[0] = (databuf[1] << 8) | databuf[0];
		mag[1] = (databuf[3] << 8) | databuf[2];
		mag[2] = (databuf[5] << 8) | databuf[4];
		
		if(af6133e_controldata[2] & 0x01) {
			printk("%s ZPos: %d, %d, %d\n", __func__, mag[0], mag[1], mag[2]);
		}
		
		if(mag[0] <  16383 && mag[1] <  16383 && mag[2] <  16383 &&
		mag[0] > -16384 && mag[1] > -16384 && mag[2] > -16384) {
			tmp_pos[0] += (int32_t)(mag[0]);
			tmp_pos[1] += (int32_t)(mag[1]);
			tmp_pos[2] += (int32_t)(mag[2]);
			bist_loop_count--;
		}
		else {
			bist_retry_count--;
		}
	}
	while(bist_loop_count && bist_retry_count);
  
	pos_count = bist_loop_count;

	// ZNeg
	databuf[0] = REG_TEST2;
	databuf[1] = 0x09;
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	mdelay(1);
	bist_loop_count = MAG_BIST_LOOP;
	bist_retry_count = BIST_RETRY_NUM;

	do {
		databuf[0] = REG_MEASURE;
		databuf[1] = 0x01;     
		af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

		mdelay(6);

		databuf[0] = REG_DATA;
		af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
	
		mag[0] = (databuf[1] << 8) | databuf[0];
		mag[1] = (databuf[3] << 8) | databuf[2];
		mag[2] = (databuf[5] << 8) | databuf[4];

		if(af6133e_controldata[2] & 0x01) {
			printk("%s ZNeg: %d, %d, %d\n", __func__, mag[0], mag[1], mag[2]);
		}

		if(mag[0] <  16383 && mag[1] <  16383 && mag[2] <  16383 &&
		mag[0] > -16384 && mag[1] > -16384 && mag[2] > -16384) {
			tmp_neg[0] += (int32_t)(mag[0]);
			tmp_neg[1] += (int32_t)(mag[1]);
			tmp_neg[2] += (int32_t)(mag[2]);
			bist_loop_count--;
		}
		else {
			bist_retry_count--;
		}
	}
	while(bist_loop_count && bist_retry_count);

	neg_count = bist_loop_count;

	if(pos_count != MAG_BIST_LOOP && neg_count != MAG_BIST_LOOP) {
		for(i=0;i<3;i++)
		bist_Z[i] = (int16_t)(tmp_neg[i]/(MAG_BIST_LOOP-neg_count) - tmp_pos[i]/(MAG_BIST_LOOP-pos_count));
	}
	else {
		for(i=0;i<3;i++) bist_Z[i] = 0;
		
		if(af6133e_controldata[2] & 0x01)
		{
		printk("%s bist_Z fail: pos_count:%d, neg_count:%d\n", __func__, pos_count, neg_count);
		}
		goto BIST_END;
	}

	if(af6133e_controldata[2] & 0x01) {
		printk("%s bist_Z: %d, %d, %d\n", __func__, bist_Z[0], bist_Z[1], bist_Z[2]);
	}

BIST_END:

	// close 8M
	databuf[0] = REG_CHOPPER;  
	databuf[1] = 0xC1; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	// Recovery Golden Mode
	databuf[0] = REG_TEST2;
	databuf[1] = 0x00;
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	databuf[0] = REG_MEASURE;
	databuf[1] = 0x01;
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	return 0;
}

static int af6133e_SetBistData(void)
{  
	int16_t i;
	int16_t bistX[3], bistY[3], bistZ[3];
	int16_t gain[3];
	int16_t comp_coeff[4];
 
	if(NULL == this_client) {
		return -1;
	}   
  
	/* get BIST data */
	if(af6133e_GetBistData(bistX, bistY, bistZ)) {
		return -1;
	}

	/* update BIST data */
	for(i = 0; i < 3; i++) {
		af6133e_bist_x[i] = bistX[i];
		af6133e_bist_y[i] = bistY[i];
		af6133e_bist_z[i] = bistZ[i];
	}

	if(af6133e_controldata[2] & 0x01) {
		printk("%s bist_X: %d, %d, %d\n", __func__, af6133e_bist_x[0], af6133e_bist_x[1], af6133e_bist_x[2]);
		printk("%s bist_Y: %d, %d, %d\n", __func__, af6133e_bist_y[0], af6133e_bist_y[1], af6133e_bist_y[2]);
		printk("%s bist_Z: %d, %d, %d\n", __func__, af6133e_bist_z[0], af6133e_bist_z[1], af6133e_bist_z[2]);
	}

  if(af6133e_bist_x[0] > 0 && af6133e_bist_y[1] > 0 && af6133e_bist_z[2] > 0) {
		uint32_t SSensz = ((af6133e_bist_x[2]*BIST_COEFF_X) * (af6133e_bist_x[2]*BIST_COEFF_X) + 
						(af6133e_bist_y[2]*BIST_COEFF_Y) * (af6133e_bist_y[2]*BIST_COEFF_Y)) /10000 +
						((af6133e_bist_z[2]*BIST_COEFF_Z)/100) * ((af6133e_bist_z[2]*BIST_COEFF_Z)/100);
		int16_t SSensz_i16;

		if(af6133e_controldata[2] & 0x01) {
			printk("%s SSensz: %d\n", __func__, SSensz);
		}

		SSensz = vtc_int_sqrt(SSensz);

		if(af6133e_controldata[2] & 0x01) {
		printk("%s SSensz = sqrt(SSensz): %d\n", __func__, SSensz);
		}

		SSensz_i16 = (int16_t)SSensz;

		if(af6133e_controldata[2] & 0x01) {
			printk("%s SSensz_i16: %d\n", __func__, SSensz_i16);
		}
    
		//calculate sens. compensation coeff.
		gain[0] = BIST_GAIN_COEFF_X / af6133e_bist_x[0];
		gain[1] = BIST_GAIN_COEFF_Y / af6133e_bist_y[1];
		gain[2] = BIST_GAIN_COEFF_Z / af6133e_bist_z[2];

		comp_coeff[0] = -(int16_t)((af6133e_bist_y[0]*BIST_COEFF_Y*100) / (af6133e_bist_x[0]*BIST_COEFF_X));
		comp_coeff[1] = -(int16_t)((af6133e_bist_x[1]*BIST_COEFF_X*100) / (af6133e_bist_y[1]*BIST_COEFF_Y));
		comp_coeff[2] = -(int16_t)((af6133e_bist_x[2]*BIST_COEFF_X) / SSensz_i16);
		comp_coeff[3] = -(int16_t)((af6133e_bist_y[2]*BIST_COEFF_Y) / SSensz_i16);

		if(af6133e_controldata[2] & 0x01) {
			printk("%s gain: %d, %d, %d\n", __func__, gain[0], gain[1], gain[2]);
			printk("%s comp_coeff: %d, %d, %d, %d\n", __func__, comp_coeff[0], comp_coeff[1], comp_coeff[2], comp_coeff[3]);
		}
	}
  
	if( gain[0] < GAIN_X_MIN || gain[0] > GAIN_X_MAX ||
		gain[1] < GAIN_Y_MIN || gain[1] > GAIN_Y_MAX ||
		gain[2] < GAIN_Z_MIN || gain[2] > GAIN_Z_MAX ||
		(comp_coeff[0] + comp_coeff[1]) >  BIST_COEFF_LIMIT_XY ||
		(comp_coeff[0] + comp_coeff[1]) < -BIST_COEFF_LIMIT_XY ||  
		vtc_fabs(comp_coeff[2]) > BIST_COEFF_LIMIT_Z ||
		vtc_fabs(comp_coeff[3]) > BIST_COEFF_LIMIT_Z) {
			MSE_ERR("af6133e BISC parameters over range\n");
	}
	else {
		af6133e_gain[0] = gain[0];
		af6133e_gain[1] = gain[1];
		af6133e_gain[2] = gain[2];

		af6133e_comp[0] = comp_coeff[0];
		af6133e_comp[1] = comp_coeff[1];
		af6133e_comp[2] = comp_coeff[2];
		af6133e_comp[3] = comp_coeff[3];
	}

	if(af6133e_controldata[2] & 0x01) {
		printk("%s af6133e_gain: %d, %d, %d\n", __func__, af6133e_gain[0], af6133e_gain[1], af6133e_gain[2]);
		printk("%s af6133e_comp: %d, %d, %d, %d\n", __func__, af6133e_comp[0], af6133e_comp[1], af6133e_comp[2], af6133e_comp[3]);
	}
  
	return 0;
}

static int af6133e_Chipset_Init(void)
{
	/* AF6133E Golden Mode Setting (Temporary) */
	uint8_t databuf[2];
	
	databuf[0] = REG_PCODE;
	af6133e_i2c_master_operate(this_client, databuf, 0x101, I2C_FLAG_READ); 
	
	if(AF6133E_PID != databuf[0]) {
		MSE_ERR("af6133e PCODE is incorrect: %d\n", databuf[0]);
		return -3;
	} 
	else { 
		printk("%s chip id:%#x\n",__func__,databuf[0]);
	}
  
	databuf[0] = REG_AVG;
	databuf[1] = 0x47; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	databuf[0] = REG_SR_MODE;
	databuf[1] = 0x0D; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	databuf[0] = REG_OSR;
	databuf[1] = 0x3D; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	
	databuf[0] = REG_OSC_FREQ;
	databuf[1] = 0x3F; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);
	
	databuf[0] = REG_XY_WAITING;
	databuf[1] = 0x07; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	databuf[0] = REG_AVG_2ND;
	databuf[1] = 0x50; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	mdelay(5);
	return 0;
}

static int af6133e_mag_temp_comp(int16_t *mag_data)
{
	int32_t temp_buf[3];

	if(af6133e_controldata[2] & 0x01) {
		printk("%s af6133e_odr:%d\n", __func__, af6133e_odr);
	}

	if(af6133e_odr > 0 && af6133e_t_count++ > af6133e_odr) {
		//int16_t temp_dt;
		int16_t i, j;
		int16_t buf[TEMP_MF_NUM];
		int16_t delta;
		uint8_t databuf[2];

		af6133e_t_count = 0;

		databuf[0] = REG_TEMP;
		af6133e_i2c_master_operate(this_client, databuf, 0x201, I2C_FLAG_READ);

		af6133e_t_buf[af6133e_t_index] = (databuf[1] << 8) | databuf[0];
		af6133e_t_buf[af6133e_t_index] = (int16_t)((float)af6133e_t_buf[af6133e_t_index] * TEMP_RESOLUTION) / 1000;

		if(af6133e_controldata[2] & 0x01) {
			printk("%s t_buf[%d]: %d\n", __func__, af6133e_t_index, af6133e_t_buf[af6133e_t_index]);
		}

		for(i=0;i<TEMP_MF_NUM;i++)
		buf[i] = af6133e_t_buf[i];

		for(i=0;i<(TEMP_MF_NUM-1);i++)
			for(j=0;j<(TEMP_MF_NUM-1);j++)
				if(buf[j] > buf[j+1]) {
					int16_t temp = buf[j+1];
					buf[j+1] = buf[j];
					buf[j] = temp;
				}

		af6133e_temp_dt = buf[TEMP_MF_IDX] - af6133e_temp_t0;
		delta = af6133e_temp_dt - af6133e_dt_pre;

		if(af6133e_controldata[2] & 0x01) {
			printk("%s af6133e_temp_dt:%d = buf[%d]:%d - af6133e_temp_t0:%d \n", __func__, af6133e_temp_dt, TEMP_MF_IDX, buf[TEMP_MF_IDX], af6133e_temp_t0);
			printk("%s delta:%d = af6133e_temp_dt:%d - af6133e_dt_pre:%d \n", __func__, delta, af6133e_temp_dt, af6133e_dt_pre);
		}

		if(vtc_fabs(delta) > TEMP_DELTA_THRESHOLD)
			af6133e_dt_pre = af6133e_temp_dt;
		else
			af6133e_temp_dt = af6133e_dt_pre;
		
		if(++af6133e_t_index >= TEMP_MF_NUM)
			af6133e_t_index = 0;

  	}

	temp_buf[0] = (int32_t)mag_data[0];
	temp_buf[1] = (int32_t)mag_data[1];
	temp_buf[2] = (int32_t)mag_data[2];

	if(af6133e_controldata[2] & 0x01) {
		printk("%s af6133e_temp_dt: %d; temp_buf: %d, %d, %d;\n", __func__, af6133e_temp_dt, temp_buf[0], temp_buf[1], temp_buf[2]);
	}

	temp_buf[0] = temp_buf[0] * af6133e_temp_dt * TEMP_SENS_COEFF_X;
	temp_buf[1] = temp_buf[1] * af6133e_temp_dt * TEMP_SENS_COEFF_Y;
	temp_buf[2] = temp_buf[2] * af6133e_temp_dt * TEMP_SENS_COEFF_Z;

	if(af6133e_controldata[2] & 0x01) {
		printk("%s temp_buf: %d, %d, %d\n", __func__, temp_buf[0], temp_buf[1], temp_buf[2]);
	}

	mag_data[0] += (int16_t)(temp_buf[0] / 100000);
	mag_data[1] += (int16_t)(temp_buf[1] / 100000);
	mag_data[2] += (int16_t)(temp_buf[2] / 100000);

	return 0;
}

// static int AF6133E_i2c_rxdata(struct i2c_client *i2c, unsigned char *rxData, int length)
// {
// 	struct i2c_msg msgs[] = {
// 		{
// 			.addr = i2c->addr,
// 			.flags = 0,
// 			.len = 1,
// 			.buf = rxData,
// 		},
// 		{
// 			.addr = i2c->addr,
// 			.flags = I2C_M_RD,
// 			.len = length,
// 			.buf = rxData,
// 		}, };
// 	unsigned char addr = rxData[0];

// 	if (i2c_transfer(i2c->adapter, msgs, 2) < 0) {
// 		dev_err(&i2c->dev, "%s: transfer failed.", __func__);
// 		return -EIO;
// 	}

// 	dev_vdbg(&i2c->dev, "RxData: len=%02x, addr=%02x, data=%02x",
// 			length, addr, rxData[0]);
// 	return 0;
// }

static int AF6133E_i2c_txdata(struct i2c_client *i2c, unsigned char *txData, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		}, 
	};

	if (i2c_transfer(i2c->adapter, msg, 1) < 0) {
		dev_err(&i2c->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}

	dev_vdbg(&i2c->dev, "TxData: len=%02x, addr=%02x data=%02x",
			length, txData[0], txData[1]);
	return 0;
}

static int AF6133E_read_xyz(struct AF6133E_data *dev, int *vec)
{
	uint8_t databuf[6];
	int16_t output[3];
	int16_t i;

	/* read sensor data */
	databuf[0] = REG_DATA;
	af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);
  
	output[0] = (databuf[1] << 8) | databuf[0];
	output[1] = (databuf[3] << 8) | databuf[2];
	output[2] = (databuf[5] << 8) | databuf[4];

	if(af6133e_controldata[2] & 0x01) {
		printk("%s magdata (raw): %d, %d, %d,\n", __func__, output[0], output[1], output[2]);
	}

	/* reduce sensor offset */  
	if(af6133e_controldata[1] & FUNC_MASK_BIAS) {
		for(i=0;i<3;i++)
			output[i] = output[i] - af6133e_offset[i];
	}

	if(af6133e_controldata[2] & 0x01) {
		printk("%s magdata (remove offset): %d, %d, %d,\n", __func__, output[0], output[1], output[2]);
	}
  
#ifdef AF6133E_TEMP_COMP
	if(af6133e_controldata[1] & FUNC_MASK_TEMP) {
		af6133e_mag_temp_comp(output);
		if(af6133e_controldata[2] & 0x01) {
			printk("%s magdata (temperature compensation): %d, %d, %d,\n", __func__, output[0], output[1], output[2]);
		}
	}
#endif

	/* multiple sensor gain */  
  	if(af6133e_controldata[1] & FUNC_MASK_GAIN) {
		for(i=0;i<3;i++)
		  	output[i] = (output[i] * af6133e_gain[i]) / 100;
		  
		if(af6133e_controldata[2] & 0x01) {
      		printk("%s gain: %d, %d, %d, (1/100)\n", __func__, af6133e_gain[0], af6133e_gain[1], af6133e_gain[2]);
      		printk("%s magdata (comp gain): %d, %d, %d,\n", __func__, output[0], output[1], output[2]);
    	}
  	}

  	/* sens. comp. */
  	if(af6133e_controldata[1] & FUNC_MASK_COMP) {
		vec[0] = output[0] + (output[1] * af6133e_comp[0]) / 100;
		vec[1] = output[1] + (output[0] * af6133e_comp[1]) / 100;
		vec[2] = output[2] + (output[0] * af6133e_comp[2]) / 100
						+ (output[1] * af6133e_comp[3]) / 100;

		if(af6133e_controldata[2] & 0x01) {
			printk("%s orthogonal: %d, %d, %d, %d, (1/100)\n", __func__, af6133e_comp[0], af6133e_comp[1], af6133e_comp[2], af6133e_comp[3]);
			printk("%s magdata (comp orthogonal): %d, %d, %d,\n", __func__, vec[0], vec[1], vec[2]);
		}
  	}
	else {
		vec[0] = output[0];
		vec[1] = output[1];
		vec[2] = output[2];
	}
  
	vec[0] = -vec[0];
	vec[1] = -vec[1];
	vec[2] = -vec[2];
	if(af6133e_controldata[2] & 0x01) {
		printk("%s magdata (output): %d, %d, %d,\n", __func__, vec[0], vec[1], vec[2]);
	}

	/* next measurement */    
	databuf[0] = REG_MEASURE;
	databuf[1] = 0x01;
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	return 0;
}

static void AF6133E_report_values(struct AF6133E_data *dev, int *xyz)
{
	input_report_abs(dev->idev, ABS_RX, xyz[0]);
	input_report_abs(dev->idev, ABS_RY, xyz[1]);
	input_report_abs(dev->idev, ABS_RZ, xyz[2]);
	input_sync(dev->idev);
}

static void AF6133E_input_work_func(struct AF6133E_data *dev)
{
	int xyz[3] = { 0 };
	int err;

	err = AF6133E_read_xyz(dev, xyz);

	if (err < 0) {
		printk("AF6133E_read_xyz failed\n");
	} else {
		AF6133E_report_values(dev, xyz);
	}
}

static enum hrtimer_restart AF6133E_work(struct hrtimer *timer)
{
	struct AF6133E_data *mag;
	ktime_t poll_delay;
	mag = container_of((struct hrtimer *)timer, struct AF6133E_data, work_timer);

	queue_work(mag->wq, &mag->work);
	if (mag->poll_interval > 0) {
		poll_delay = ktime_set(0, mag->poll_interval * NSEC_PER_MSEC);
	} else {
		poll_delay = ktime_set(0, POLL_INTERVAL_DEFAULT * NSEC_PER_MSEC);
	}
	mag->hrtimer_running = true;
	hrtimer_forward_now(&mag->work_timer, poll_delay);

	return HRTIMER_RESTART;
}

static void report_event(struct work_struct *work)
{
	struct AF6133E_data *mag = container_of(work, struct AF6133E_data, work);

	if (atomic_read(&mag->enabled) <= 0) {
		return;
	}
	AF6133E_input_work_func(mag);

	return;
}

static struct input_dev *AF6133E_init_input(struct AF6133E_data *dev)
{
	int status;
	//struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	struct input_dev *input = NULL;

	printk("%s\n", __FUNCTION__);
	hrtimer_init(&dev->work_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dev->work_timer.function = AF6133E_work;
	dev->hrtimer_running = false;

	INIT_WORK(&dev->work, report_event);
	dev->wq = create_singlethread_workqueue("magsensor_event");
	if (IS_ERR(dev->wq)) {
		dev_err(&dev->i2c->dev, "error create work queue\n");
		return NULL;
	}
	flush_workqueue(dev->wq);

	input = input_allocate_device();
	if (!input) {
		dev_err(&dev->i2c->dev, "error alloc input device\n");
		goto error_alloc_device;
	}

	input->name = "af6133e";
	input->id.bustype = BUS_I2C;

	__set_bit(EV_ABS, input->evbit);
	input_set_events_per_packet(input, 100);
	input_set_abs_params(input, ABS_RX, -32768, 32767, 0, 0);
	input_set_abs_params(input, ABS_RY, -32768, 32767, 0, 0);
	input_set_abs_params(input, ABS_RZ, -32768, 32767, 0, 0);

	/* Report the dummy value */
	input_set_abs_params(input, ABS_MISC, INT_MIN, INT_MAX, 0, 0);

	input_set_capability(input, EV_REL, REL_X);
	input_set_capability(input, EV_REL, REL_Y);
	input_set_capability(input, EV_REL, REL_Z);

	status = input_register_device(input);
	if (status) {
		dev_err(&dev->i2c->dev, "error registering input device\n");
		goto error_register;
	}

	return input;

error_register:
	input_free_device(input);
error_alloc_device:
	destroy_workqueue(dev->wq);
	return NULL;
}

static int AF6133E_check_device(struct AF6133E_data *dev)
{
	int rc;
	unsigned char rd_buffer[2];
	rd_buffer[0] = REG_PCODE;
	// rc = AF6133E_i2c_rxdata(dev->i2c, rd_buffer, 1);
	af6133e_i2c_master_operate(dev->i2c, rd_buffer, 1, I2C_FLAG_READ);
	rc = VTC_i2c_Rx(dev->i2c, rd_buffer, 1);
	if (rc) {
		dev_err(&dev->i2c->dev, "read reg id failed.(%d)\n", rc);
		return rc;
	}
	printk("AF6133E read ID is 0x%x\n", rd_buffer[0]);

	if (rd_buffer[0] != AF6133E_PID)
		return -ENODEV;

	rd_buffer[0] = REG_AVG;
	rd_buffer[1] = 0x47;
	AF6133E_i2c_txdata(dev->i2c, rd_buffer, 2);

	rd_buffer[0] = REG_SR_MODE;
	rd_buffer[1] = 0x0D;
	AF6133E_i2c_txdata(dev->i2c, rd_buffer, 2);

	rd_buffer[0] = REG_OSR;
	rd_buffer[1] = 0x3D;
	AF6133E_i2c_txdata(dev->i2c, rd_buffer, 2);

	rd_buffer[0] = REG_OSC_FREQ;
	rd_buffer[1] = 0x3F;
	AF6133E_i2c_txdata(dev->i2c, rd_buffer, 2);

	rd_buffer[0] = REG_XY_WAITING;
	rd_buffer[1] = 0x07;
	AF6133E_i2c_txdata(dev->i2c, rd_buffer, 2);

	rd_buffer[0] = REG_AVG_2ND;
	rd_buffer[1] = 0x50;
	AF6133E_i2c_txdata(dev->i2c, rd_buffer, 2);

	mdelay(5);
	return 0;
}

void af6133e_Para_Init(void)
{
	int16_t i;

	for(i=0;i<3;i++) {
		af6133e_offset[i] = 0;
		af6133e_gain[i] = 100;
	}

	for(i=0;i<4;i++) {
		af6133e_comp[i] = 0;
	}

#ifdef AF6133E_TEMP_COMP
	af6133e_temp_t0 = 0;
	af6133e_temp_dt = 0;
	af6133e_dt_pre = 0;
	af6133e_t_count = 0;
	af6133e_t_index = 0;
#endif
}

static int af6133e_GetSensorOffset(int16_t *offset)
{
#ifdef AF6133E_SET_OFFSET
	uint8_t databuf[6];
#endif
	if(NULL == this_client) {
		return -1;
	}   

#ifdef AF6133E_SET_OFFSET
	databuf[0] = REG_AVG_2ND;
	databuf[1] = 0x10; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	databuf[0] = REG_MEASURE;
	databuf[1] = 0x01; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	mdelay(8);

	databuf[0] = REG_DATA;
	af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);

	offset[0] = (databuf[1] << 8) | databuf[0];
	offset[1] = (databuf[3] << 8) | databuf[2];
	offset[2] = (databuf[5] << 8) | databuf[4];

	if(af6133e_controldata[2] & 0x01) {
		printk("%s offset data (REG_AVG_2ND = 0x10): %d, %d, %d,\n", __func__, offset[0], offset[1], offset[2]);
	}

	databuf[0] = REG_AVG_2ND;
	databuf[1] = 0x50; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	databuf[0] = REG_MEASURE;
	databuf[1] = 0x01; 
	af6133e_i2c_master_operate(this_client, databuf, 0x02, I2C_FLAG_WRITE);

	mdelay(8);

	databuf[0] = REG_DATA;
	af6133e_i2c_master_operate(this_client, databuf, 0x601, I2C_FLAG_READ);

	offset[0] += (databuf[1] << 8) | databuf[0];
	offset[1] += (databuf[3] << 8) | databuf[2];
	offset[2] += (databuf[5] << 8) | databuf[4];

	if(af6133e_controldata[2] & 0x01) {
		printk("%s offset data (REG_AVG_2ND = 0x50): %d, %d, %d,\n", __func__, offset[0], offset[1], offset[2]);
	}

	offset[0] = 0;
	offset[1] = 0;
	offset[2] = offset[2] / 2;

	if(af6133e_controldata[2] & 0x01) {
		printk("%s offset data (final): %d, %d, %d,\n", __func__, offset[0], offset[1], offset[2]);
	}
#else
	offset[0] = 0;
	offset[1] = 0;
	offset[2] = 0;
#endif

	return 0;
}

static int af6133e_SetSensorOffset(void)
{
#ifdef AF6133E_TEMP_COMP
	uint8_t databuf[6];
#endif

	if(NULL == this_client) {
		return -1;
	}

	if(af6133e_GetSensorOffset(af6133e_offset)) {
		return -2;
	}

#ifdef AF6133E_TEMP_COMP  
	databuf[0] = REG_TEMP;
	af6133e_i2c_master_operate(this_client, databuf, 0x201, I2C_FLAG_READ);

	af6133e_temp_t0 = (int32_t)((int16_t)((databuf[1] << 8) | databuf[0]));
	af6133e_temp_t0 = (af6133e_temp_t0 * TEMP_RESOLUTION) / 1000;
#endif

	return 0;
}

static int AF6133E_set_poll_delay(struct AF6133E_data *dev, unsigned int delay_msec)
{
	ktime_t poll_delay;
	int sampling_rate;

	if (atomic_read(&dev->enabled)) {
		if (dev->poll_interval > 0) {
			poll_delay = ktime_set(0, dev->poll_interval * NSEC_PER_MSEC);
			sampling_rate = (1000/(dev->poll_interval + 1)) + 10;
		} else {
			poll_delay = ktime_set(0, POLL_INTERVAL_DEFAULT * NSEC_PER_MSEC);
			sampling_rate = (1000/POLL_INTERVAL_DEFAULT) + 10;
		}

		// AF6133E_Continuous_Mode_With_Auto_SR(dev, AF6133E_CMD_BW01, sampling_rate);
		hrtimer_start(&dev->work_timer, poll_delay, HRTIMER_MODE_REL);
	}

	return 0;
}

static int AF6133E_set_enable(struct AF6133E_data *dev, unsigned int enable)
{
	unsigned char buffer[2];
	ktime_t poll_delay;
	int sampling_rate;
	int err = 0;

	if (!dev) {
		printk("dev pointer is NULL\n");
		return -1;
	}

	if (enable) {
	    if (atomic_read(&dev->enabled)) {
			return 0;
	    }

		dev->hrtimer_running = true;
		if (dev->poll_interval > 0) {
			poll_delay = ktime_set(0, dev->poll_interval * NSEC_PER_MSEC);
			sampling_rate = (1000/(dev->poll_interval + 1)) + 10;
		} else {
			poll_delay = ktime_set(0, POLL_INTERVAL_DEFAULT * NSEC_PER_MSEC);
			sampling_rate = (1000/POLL_INTERVAL_DEFAULT) + 10;
		}

		err = af6133e_Chipset_Init();
		if(err) {
			MSE_ERR("af6133e register initial fail\n");
		}
		/*
		err = af6133e_SetSensorOffset();
		if(err)
		{
		MSE_ERR("af6133e sensor offset initial fail\n");
		}
		*/
		err = af6133e_SetBistData();
		if(err) {
			MSE_ERR("af6133e sensor gain & comp. initial fail\n");
		}

		hrtimer_start(&dev->work_timer, poll_delay, HRTIMER_MODE_REL);
		atomic_set(&dev->enabled, 1);
	} else {
		if (atomic_read(&dev->enabled)) {
			buffer[0] = REG_MEASURE;
			buffer[1] = 0x00;
			if (AF6133E_i2c_txdata(dev->i2c, buffer, 2)) {
				dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", REG_MEASURE, __LINE__);
			}
			if (dev->hrtimer_running) {
				dev->hrtimer_running = false;
				hrtimer_cancel(&dev->work_timer);
				cancel_work_sync(&dev->work);
				flush_workqueue(dev->wq);
			}
			atomic_set(&dev->enabled, 0);
		}
	}
	dev->sensor_state = 1;

	return 0;
}

static ssize_t AF6133E_chip_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", "af6133e chip");
}

static ssize_t AF6133E_layout_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t AF6133E_layout_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	  return 0;
}

static ssize_t AF6133E_value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	int vec[3];
	struct AF6133E_data *dev_priv = dev_get_drvdata(dev);

	ret = AF6133E_read_xyz(dev_priv, vec);
	if (ret) {
		dev_warn(&dev_priv->i2c->dev, "read xyz failed\n");
	}

	return sprintf(buf, "%d %d %d\n", vec[0], vec[1], vec[2]);
}

static ssize_t AF6133E_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct AF6133E_data *clientdata = i2c_get_clientdata(this_client);
	unsigned long data;

	data = simple_strtoul(buf, NULL, 10);

	if (data > AF6133E_MAX_DELAY) {
		data = AF6133E_MAX_DELAY;
	}
	if (data < AF6133E_MIN_DELAY) {
		data = AF6133E_MIN_DELAY;
	}

	clientdata->poll_interval = data - 1;
	AF6133E_set_poll_delay(clientdata, clientdata->poll_interval);

	return count;
}

static ssize_t AF6133E_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct AF6133E_data *clientdata = i2c_get_clientdata(this_client);
	return sprintf(buf, "%d\n", clientdata->poll_interval + 1);
}

static ssize_t AF6133E_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct AF6133E_data *clientdata = i2c_get_clientdata(this_client);
	unsigned long data;

	data = simple_strtoul(buf, NULL, 10);

	if (data == 1) {
		AF6133E_set_enable(clientdata, 1);
	} else if (data == 0) {
		AF6133E_set_enable(clientdata, 0);
	}
	return count;
}

static ssize_t AF6133E_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct AF6133E_data *clientdata = i2c_get_clientdata(this_client);
	int enable = atomic_read(&clientdata->enabled);

	return sprintf(buf, "%d\n", enable);
}

static DEVICE_ATTR(chipinfo, S_IRUSR|S_IRGRP, AF6133E_chip_info, NULL);
static DEVICE_ATTR(layout, S_IRUSR|S_IRGRP|S_IWUSR, AF6133E_layout_show, AF6133E_layout_store);
static DEVICE_ATTR(value, S_IRUSR|S_IRGRP, AF6133E_value_show, NULL);
static DEVICE_ATTR(delay, 0644, AF6133E_delay_show, AF6133E_delay_store);
static DEVICE_ATTR(enable, 0644, AF6133E_enable_show, AF6133E_enable_store);

static struct attribute *AF6133E_attributes[] = {
	&dev_attr_chipinfo.attr,
	&dev_attr_layout.attr,
	&dev_attr_value.attr,
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	NULL,
};

static const struct attribute_group AF6133E_attr_group = {
		.attrs = AF6133E_attributes,
};

#if MISC_DEVICE_REGISTER
static int AF6133E_open(struct inode *inode, struct file *file)
{
	int ret = -1;
	ret = nonseekable_open(inode, file);

	return ret;
}

static int AF6133E_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long AF6133E_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	int delay;				/* for GET_DELAY */
	int set_delay;
	struct AF6133E_data *clientdata = i2c_get_clientdata(this_client);
	uint32_t enable;

		switch (cmd) {
		case AF6133E_IOC_SET_DELAY:
			if (copy_from_user(&set_delay, argp, sizeof(set_delay))) {
				printk("dev add set delay failed\n");
				return -EFAULT;
			}

			if (set_delay < AF6133E_MIN_DELAY) {
				set_delay = AF6133E_MIN_DELAY;
			}

			printk("%s %s %d xiexie\n", __func__, "AF6133E_IOC_SET_DELAY", set_delay);
			clientdata->poll_interval = set_delay;
			AF6133E_set_poll_delay(clientdata, clientdata->poll_interval);
			break;

		case ECOMPASS_IOC_GET_DELAY:
			delay = clientdata->poll_interval;
			if (copy_to_user(argp, &delay, sizeof(delay))) {
				printk(KERN_ERR "copy_to_user failed.");
				return -EFAULT;
			}
			break;

		case MSENSOR_IOCTL_MSENSOR_ENABLE:
			if (copy_from_user(&enable, argp, sizeof(enable))) {
				return -EFAULT;
			}

			printk("MSENSOR_ENABLE  %s enable = %d xiexie \n", __FUNCTION__, enable);
			AF6133E_set_enable(clientdata, enable);
			break;

		case MSENSOR_IOCTL_OSENSOR_ENABLE:
			if (copy_from_user(&enable, argp, sizeof(enable))) {
				return -EFAULT;
			}

			printk("OSENSOR_ENABLE  %s enable = %d xiexie \n", __FUNCTION__, enable);
			AF6133E_set_enable(clientdata, enable);
			break;

		default:
			printk(KERN_ERR "%s not supported = 0x%04x", __FUNCTION__, cmd);
			break;
	}

	return 0;
}

static struct file_operations AF6133E_fops = {
	.owner = THIS_MODULE,
	.open = AF6133E_open,
	.release = AF6133E_release,
	.unlocked_ioctl = AF6133E_unlocked_ioctl,
};

static struct miscdevice AF6133E_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "AF6133E",
    .fops = &AF6133E_fops,
};
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int AF6133E_probe(struct i2c_client *client)
#else
static int AF6133E_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	int res = 0;
	struct AF6133E_data *dev;
#if MISC_DEVICE_REGISTER
	int err = 0;
#endif
	printk("dev probing AF6133E\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("AF6133E i2c functionality check failed.\n");
		res = -ENODEV;
		goto out;
	}

	dev = devm_kzalloc(&client->dev, sizeof(struct AF6133E_data),
			GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "memory allocation failed.\n");
		res = -ENOMEM;
		goto out;
	}

	dev->poll_interval = AF6133E_DEFAULT_INTERVAL_MS;
	atomic_set(&dev->enabled, 0);
	dev->on_before_suspend = 0;

	this_client = client;
	dev->i2c = client;
	dev_set_drvdata(&client->dev, dev);
	i2c_set_clientdata(dev->i2c, dev);
	this_client = client;

	mutex_init(&dev->lock);

	af6133e_Para_Init();

	res = AF6133E_check_device(dev);
	if (res) {
		dev_err(&client->dev, "Check device failed\n");
		res = -ENODEV;
		goto out;
	}

	res = af6133e_SetSensorOffset();
	if(res) {
		MSE_ERR("af6133e sensor offset initial fail\n");
		res = -ENODEV;
		goto out;
	}

	/*
	res = af6133e_SetBistData();
	if(res) {
		MSE_ERR("af6133e sensor gain & comp. initial fail\n");
		goto out;
	}
	*/

	dev->idev = AF6133E_init_input(dev);
	if (!dev->idev) {
		dev_err(&client->dev, "init input device failed\n");
		res = -ENODEV;
		goto out;
	}
	res = sysfs_create_group(&dev->idev->dev.kobj, &AF6133E_attr_group);
	if (res) {
		dev_err(&client->dev, "Unable to creat sysfs group\n");
		goto exit_misc_device_register_failed;
	}

	kobject_uevent(&dev->idev->dev.kobj, KOBJ_CHANGE);
#if MISC_DEVICE_REGISTER
	err = misc_register(&AF6133E_device);
	if (err) {
		printk(KERN_ERR "AF6133E misc device register failed\n");
	}
	/* create sysfs group */
	res = sysfs_create_group(&client->dev.kobj, &AF6133E_attr_group);
	if (res) {
		res = -EROFS;
		dev_err(&client->dev, "Unable to creat sysfs group\n");
	}
#endif
	printk("AF6133E successfully probed\n");

	return 0;

exit_misc_device_register_failed:
	input_unregister_device(dev->idev);
	input_free_device(dev->idev);
out:
	return res;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void AF6133E_remove(struct i2c_client *client)
#else
static int AF6133E_remove(struct i2c_client *client)
#endif
{
	struct AF6133E_data *dev_priv = dev_get_drvdata(&client->dev);

	printk("dev remove AF6133E\n");
	AF6133E_set_enable(dev_priv, false);

	if (dev_priv->idev) {
		destroy_workqueue(dev_priv->wq);
		sysfs_remove_group(&dev_priv->idev->dev.kobj, &AF6133E_attr_group);
		input_unregister_device(dev_priv->idev);
	}

#if MISC_DEVICE_REGISTER
	sysfs_remove_group(&client->dev.kobj, &AF6133E_attr_group);
	misc_deregister(&AF6133E_device);
#endif
	input_sensor_free(&(mag_config.input_type));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    return;
#else
	return 0;
#endif
}

static int AF6133E_suspend(struct device *dev)
{
	int res = 0;
	struct AF6133E_data *dev_priv = dev_get_drvdata(dev);
	dev_dbg(dev, "suspended\n");
	input_set_power_enable(&(mag_config.input_type), 0);
	dev_priv->on_before_suspend = atomic_read(&dev_priv->enabled);
	if (dev_priv->on_before_suspend) {
		return AF6133E_set_enable(dev_priv, false);
	}

	return res;
}

static int AF6133E_resume(struct device *dev)
{
	int res = 0;
	struct AF6133E_data *dev_priv = dev_get_drvdata(dev);

	dev_dbg(dev, "resumed\n");

	input_set_power_enable(&(mag_config.input_type), 1);
	if (dev_priv->on_before_suspend) {
		return AF6133E_set_enable(dev_priv, true);
	}

	return res;
}

static const struct i2c_device_id AF6133E_id[] = {
	{ AF6133E_I2C_NAME, 0 },
	{ }
};

static struct of_device_id AF6133E_match_table[] = {
	{ .compatible = "allwinner,af6133e", },
	{ },
};

static const struct dev_pm_ops AF6133E_pm_ops = {
	.suspend = AF6133E_suspend,
	.resume = AF6133E_resume,
};

static struct i2c_driver AF6133E_driver = {
	.probe 		= AF6133E_probe,
	.remove 	= AF6133E_remove,
	.id_table	= AF6133E_id,
	.driver 	= {
		.owner	= THIS_MODULE,
		.name	= AF6133E_I2C_NAME,
		.of_match_table = AF6133E_match_table,
		.pm = &AF6133E_pm_ops,
	},
};

static int startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__, __LINE__);

	if (input_sensor_startup(&(mag_config.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else {
		ret = input_sensor_init(&(mag_config.input_type));
	}

	if (0 != ret) {
	    printk("%s:gsensor.init_platform_resource err. \n", __func__);
	}

	input_set_power_enable(&(mag_config.input_type), 1);
	return 0;
}

static int __init AF6133E_init(void)
{
	if (startup() != 0)
		return -1;

	return i2c_add_driver(&AF6133E_driver);
}

static void __exit AF6133E_exit(void)
{
	i2c_del_driver(&AF6133E_driver);
}

MODULE_DESCRIPTION("MEMSIC AF6133E Magnetic Sensor Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(AF6133E_init);
module_exit(AF6133E_exit);
