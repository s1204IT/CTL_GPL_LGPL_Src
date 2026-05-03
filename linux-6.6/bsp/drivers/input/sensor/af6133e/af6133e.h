/*
 * Copyright (C) 2010 AKM, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

/*
 * Definitions for AF6133E magnetic sensor chip.
 */
#ifndef __AF6133E_H__
#define __AF6133E_H__

#include <linux/ioctl.h>

#define AF6133E_I2C_NAME		"af6133e"

#define AF6133E_SET_OFFSET
#define AF6133E_TEMP_COMP

#define AF6133E_BUFSIZE    6

#define AF6133E_PID    0x68

/* Register Map */
#define REG_PCODE       0x00
#define REG_DATA        0x03
#define REG_MEASURE     0x0A
#define REG_RANGE       0x0B
#define REG_I2C_CHECK   0x10
#define REG_SW_RESET    0x11
#define REG_AVG         0x13
#define REG_SR_MODE     0x14
#define REG_AVG_2ND     0x15
#define REG_OSR         0x16
#define REG_LLPF        0x17
#define REG_OSC_FREQ    0x19
#define REG_TEMP        0x21
#define REG_ADC_GAIN    0x2B
#define REG_XY_WAITING  0x32
#define REG_Z_WAITING   0x33
#define REG_CHOPPER     0x35
#define REG_TEST2       0x37
#define REG_FLIPPING    0x38

/* factory test */
#define AF6133_OFFSET_MIN  -3000
#define AF6133_OFFSET_MAX   3000
#define AF6133_SENS_MIN      200
#define AF6133_SENS_MAX     1000

/* resolution */

// conversion of magnetic data (for AF6133E) to uT units
#define CONVERT_M        15	
#define CONVERT_M_DIV   100	

/* BIST parameters */
//#define MAG_BIST_LOOP	1

#define BIST_COEFF_X    79 // 0.794
#define BIST_COEFF_Y    82 // 0.817
#define BIST_COEFF_Z    79 // 0.785

/* Temp. Coeff. */
#ifdef AF6133E_TEMP_COMP
#define TEMP_RESOLUTION       908 //0.0908 * 1000
#define TEMP_SENS_COEFF_X     369 //0.00369 * 100000
#define TEMP_SENS_COEFF_Y     403 //0.00403 * 100000
#define TEMP_SENS_COEFF_Z     434 //0.00434 * 100000
#define TEMP_DELTA_THRESHOLD  2 //degree
#define TEMP_MF_NUM           5
#define TEMP_MF_IDX           (uint16_t)(TEMP_MF_NUM / 2)
#endif

/* BIST parameters */
#define BIST_RETRY_NUM   5
#define MAG_BIST_LOOP    5
#define GAIN_X_MIN      60
#define GAIN_X_MAX     600
#define GAIN_Y_MIN      60
#define GAIN_Y_MAX     600
#define GAIN_Z_MIN      60
#define GAIN_Z_MAX     600

#define BIST_COEFF_LIMIT_XY    9 //0.087 //5
#define BIST_COEFF_LIMIT_Z    70 //0.707 //45

#define BIST_GAIN_COEFF_X    (int32_t)(10000 * 666.666 / BIST_COEFF_X)
#define BIST_GAIN_COEFF_Y    (int32_t)(10000 * 666.666 / BIST_COEFF_Y)
#define BIST_GAIN_COEFF_Z    (int32_t)(10000 * 666.666 / BIST_COEFF_Z)

#define AF6133E_OFFSET_MIN    -3000
#define AF6133E_OFFSET_MAX     3000
#define AF6133E_SENS_MIN        200
#define AF6133E_SENS_MAX       1200

//abs
#define vtc_fabs(x)    ((x<0) ? (-x) : (x))

#define CONTROL_DATA_LEN 3



#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/* function switch */
#define FUNC_MASK_BIAS          0x01
#define FUNC_MASK_GAIN          0x02
#define FUNC_MASK_COMP          0x04
#define FUNC_MASK_TEMP          0x08

// #define AF6133E_IOC_SET_DELAY         _IOW(MSENSOR, 0x29, short)
// #define ECOMPASS_IOC_GET_DELAY			_IOR(MSENSOR, 0x1d, int)
// #define ECOMPASS_IOC_SET_YPR			_IOW(MSENSOR, 0x21, int[12])
// #define ECOMPASS_IOC_GET_OPEN_STATUS	_IOR(MSENSOR, 0x20, int)
// #define ECOMPASS_IOC_GET_MFLAG			_IOR(MSENSOR, 0x1e, short)
// #define ECOMPASS_IOC_GET_OFLAG			_IOR(MSENSOR, 0x1f, short)
// #define MSENSOR_IOCTL_READ_SENSORDATA	_IOR(MSENSOR, 0x03, int)
// #define ECOMPASS_IOC_GET_LAYOUT			_IOR(MSENSOR, 0X22, int)
// #define AF6133E_IOC_READ_REG		    _IOWR(MSENSOR, 0x23, unsigned char)
// #define AF6133E_IOC_WRITE_REG		    _IOW(MSENSOR,  0x24, unsigned char[2])
// #define AF6133E_IOC_READ_REGS		    _IOWR(MSENSOR, 0x25, unsigned char[10])
// #define MSENSOR_IOCTL_SENSOR_ENABLE         _IOW(MSENSOR, 0x51, int)
// #define ECOMPASS_IOC_SET_ACC_DATA               _IOW(MSENSOR, 0x52, int[3])
// #define ECOMPASS_IOC_GET_ACC_DATA               _IOR(MSENSOR, 0x53, int[3])
// #define MSENSOR_IOCTL_READ_FACTORY_SENSORDATA  _IOW(MSENSOR, 0x54, int)
// #define MSENSOR_IOCTL_MSENSOR_ENABLE             _IOW(MSENSOR, 0x55, int)
// #define MSENSOR_IOCTL_OSENSOR_ENABLE             _IOW(MSENSOR, 0x56, int)

#endif /* __AF6133E_H__ */
