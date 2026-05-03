// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2023 - 2024 Allwinner Technology Co.,Ltd. All rights reserved.
 */
/*
 * Copyright (c) 2024 zhangyuanjing@allwinnertech.com
 */

#ifndef __SUNXI_GPIO_MOTOR_H__
#define __SUNXI_GPIO_MOTOR_H__

#include <linux/wait.h>

#ifdef DEBUG
#define MOTOR_DEBUG(fmt, arg...)                                               \
	printk("%s(%d) - " fmt, __func__, __LINE__, ##arg)
#else
#define MOTOR_DEBUG(fmt, arg...)
#endif

/* dir mode define */
#define DIR_NONE	 0
#define DIR_UP		 0x01
#define DIR_DOWN	 0x02
#define DIR_AB_SHIFT 0x00
#define DIR_RIGHT	 0x01
#define DIR_LEFT	 0x02
#define DIR_CD_SHIFT 0x02
#define DIR_MAX		 3

#define DIR_AB_STOP 0x10
#define DIR_CD_STOP 0x01

#define UPDATE_DRV          (0x1 << 0)
#define UPDATE_AB_STEP		(0x1 << 1)
#define UPDATE_AB_STEPTIME  (0x1 << 2)
#define UPDATE_CD_STEP      (0x1 << 3)
#define UPDATE_CD_STEPTIME  (0x1 << 4)

#define GET_DIR_AB(x) ((x >> DIR_AB_SHIFT) & 0x03)
#define GET_DIR_CD(x) ((x >> DIR_CD_SHIFT) & 0x03)
#define IS_DIR_AB(x)  ((GET_DIR_AB(x) == DIR_UP) || (GET_DIR_AB(x) == DIR_DOWN))
#define IS_DIR_CD(x)  ((GET_DIR_CD(x) == DIR_RIGHT) || (GET_DIR_CD(x) == DIR_LEFT))

typedef struct sunxi_motor_device_status {
	int x;	    /* X-coordinate step of the motor device */
	int y;	    /* Y-coordinate step of the motor device */
	int dir;    /* Direction of movement of the motor device */
	int status; /* Motor device status */
} sunxi_motor_device_status_t;

typedef struct sunxi_motor_cmd {
	int dir;			  /* Direction of rotation for the motor command */
	int ab_step;		  /* Number of steps to move the motor in up and down dir */
	int ab_time_per_step; /* Time duration for each microstep in up and down dir */
	int cd_step;		  /* Number of steps to move the motor in left and right dir */
	int cd_time_per_step; /* Time duration for each microstep in left and right dir */
	int block;			  /* block flag for motor commands */
} sunxi_motor_cmd_t;

typedef struct sunxi_motor_para {
	sunxi_motor_cmd_t cmd_para;
	int update_obj;
} sunxi_motor_para_t;

/* ioctl define */
#define MOTOR_DRV_MAGICNUM     'm'
#define MOTOR_DRV_SET_STOP     _IO(MOTOR_DRV_MAGICNUM, 0)
#define MOTOR_DRV_SET_STOP_AB  _IO(MOTOR_DRV_MAGICNUM, 1)
#define MOTOR_DRV_SET_STOP_CD  _IO(MOTOR_DRV_MAGICNUM, 2)
#define MOTOR_DRV_SET_START    _IO(MOTOR_DRV_MAGICNUM, 3)
#define MOTOR_DRV_SET_BLOCK    _IOW(MOTOR_DRV_MAGICNUM, 4, int)
#define MOTOR_DRV_SET_PARA     _IOW(MOTOR_DRV_MAGICNUM, 5, sunxi_motor_para_t)
#define MOTOR_DRV_SET_CMD      _IOW(MOTOR_DRV_MAGICNUM, 6, sunxi_motor_cmd_t)
#define MOTOR_DRV_GET_CMD      _IOR(MOTOR_DRV_MAGICNUM, 7, sunxi_motor_cmd_t)
#define MOTOR_DRV_GET_STATUS                                                   \
	_IOR(MOTOR_DRV_MAGICNUM, 8, sunxi_motor_device_status_t)
#define MOTOR_DRV_SET_STATUS                                                   \
	_IOR(MOTOR_DRV_MAGICNUM, 9, sunxi_motor_device_status_t)

typedef struct sunxi_motor_pin_step {
	u8 pin_black;
	u8 pin_yellow;
	u8 pin_brown;
	u8 pin_blue;
} sunxi_motor_pin_step_t;

typedef struct sunxi_motor_dev {
	int major;
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;

	/* pinctrl */
	int ab_pin_black;
	int ab_pin_yellow;
	int ab_pin_brown;
	int ab_pin_blue;

	int cd_pin_black;
	int cd_pin_yellow;
	int cd_pin_brown;
	int cd_pin_blue;

	/* time on one period */
	u32 time_on_period;

	/* private data */
	struct mutex lock;
	spinlock_t spin_lock;
	volatile int n_que_waiting;
	wait_queue_head_t i_wait_que;
} sunxi_motor_dev_t;

#endif /* __SUNXI_GPIO_MOTOR_H__ */
