/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/******************************************************************************
* @file mpi.c
* @author
* @version V1.0.0
* @date 11-Nov-2014
* @brief   MacroSilicon Programming Interface.
*
* Copyright (c) 2009-2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include "ms7200_comm.h"


VOID mculib_chip_reset(VOID)
{
#if VIN_FALSE
	ST_GPIO_Init(RESET_PORT, RESET_PIN, GPIO_MODE_OUT_OD_HIZ_FAST);
	ST_GPIO_Init(LED_PORT, GPIO_PIN_ALL, GPIO_MODE_OUT_PP_HIGH_SLOW);
	// 20140512
	// chip reset,send low level pulse > 100us
	ST_GPIO_WriteLow(RESET_PORT, RESET_PIN);
	mculib_delay_ms(1);
	ST_GPIO_WriteHigh(RESET_PORT, RESET_PIN);
	//delay for chip stable
	mculib_delay_ms(1);
#endif
}

BOOL mculib_chip_read_interrupt_pin(VOID)
{
#if VIN_FALSE
	return ST_GPIO_ReadInputPin(INT_PORT, INT_PIN);
#else
	//if no use interrupt pin, must return TRUE
	return TRUE;
#endif
}

VOID mculib_delay_ms(UINT8 u8_ms)
{
#if VIN_FALSE
	do {
		mculib_delay_us(250);
		mculib_delay_us(250);
		mculib_delay_us(250);
		mculib_delay_us(240);
		u8_ms--;
	} while (u8_ms);
#endif
}

VOID mculib_delay_us(UINT8 u8_us)
{
#if VIN_FALSE
	do {
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
#ifdef RELEASE_MODE //compile in release mode
		nop();
		nop(); //1.03us
#endif
		u8_us--;
	} while (u8_us);
#endif
}

//beflow APIs is for 16bits I2C slaver address for access ms933x register, must be implemented
UINT8 mculib_i2c_read_16bidx8bval(UINT8 u8_address, UINT16 u16_index)
{
#if VIN_FALSE
	UINT8 u8_value = 0;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index)))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index >> 8)))
		goto STOP;

	_i2c_start();

	if (!_i2c_write_byte(u8_address | 0x01))
		goto STOP;

	u8_value = _i2c_read_byte(TRUE);

STOP:
	_i2c_stop();
	return u8_value;
#endif

	return 0;
}

BOOL mculib_i2c_write_16bidx8bval(UINT8 u8_address, UINT16 u16_index, UINT8 u8_value)
{
#if VIN_FALSE
	BOOL result = FALSE;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index)))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index >> 8)))
		goto STOP;

	if (!_i2c_write_byte(u8_value))
		goto STOP;

	result = TRUE;

STOP:
	_i2c_stop();
	return result;
#endif

	return TRUE;
}

VOID mculib_i2c_burstread_16bidx8bval(UINT8 u8_address, UINT16 u16_index, UINT16 u16_length, UINT8 *pu8_value)
{
#if VIN_FALSE
	UINT16 i;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index)))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index >> 8)))
		goto STOP;

	_i2c_start();

	if (!_i2c_write_byte(u8_address | 0x01))
		goto STOP;

	for (i = 0; i < (u16_length-1); i++)
		*pu8_value++ = _i2c_read_byte(FALSE);

	*pu8_value = _i2c_read_byte(TRUE);

STOP:
	_i2c_stop();
#else
	UINT16 i;

	for (i = 0; i < u16_length; i++)
		*(pu8_value + i) = mculib_i2c_read_16bidx8bval(u8_address, u16_index + i);
#endif

}

VOID mculib_i2c_burstwrite_16bidx8bval(UINT8 u8_address, UINT16 u16_index, UINT16 u16_length, UINT8 *pu8_value)
{
#if VIN_FALSE
	UINT16 i;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index)))
		goto STOP;

	if (!_i2c_write_byte((UINT8)(u16_index >> 8)))
		goto STOP;

	for (i = 0; i < u16_length - 1; i++)
		if (!_i2c_write_byte(*pu8_value++))
			goto STOP;

	if (!_i2c_write_byte(*pu8_value))
		goto STOP;

STOP:
	_i2c_stop();
#else
	UINT16 i;

	for (i = 0; i < u16_length; i++)
		mculib_i2c_write_16bidx8bval(u8_address, u16_index + i, *(pu8_value + i));
#endif

}

//beflow APIs is for 8bits I2C slaver address, for HDMI TX DDC
//if user need use hdmi tx ddc, must be implemented
//20K is for DDC. 100K is for chip register access
VOID mculib_i2c_set_speed(UINT8 u8_i2c_speed)
{
#if VIN_FALSE
	switch (u8_i2c_speed) {
	case 0: //I2C_SPEED_20K
		g_u8_i2c_delay = 15; //measure is 20.7KHz
		break;
	case 1: //I2C_SPEED_100K
		g_u8_i2c_delay = 2; //measure is 108.7KHz
		break;
	case 2: //I2C_SPEED_400K
		g_u8_i2c_delay = 1; //measure is 150.6KHz
		break;
	case 3: //I2C_SPEED_750K
		g_u8_i2c_delay = 1; //measure is 150.6KHz
		break;
	}
#endif
}

UINT8 mculib_i2c_read_8bidx8bval(UINT8 u8_address, UINT8 u8_index)
{
#if VIN_FALSE
	UINT8 u8_value = 0;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte(u8_index))
		goto STOP;

	_i2c_start();

	if (!_i2c_write_byte(u8_address | 0x01))
		goto STOP;

	u8_value = _i2c_read_byte(TRUE);

STOP:
	_i2c_stop();
	return u8_value;
#endif

	return 0;
}

BOOL mculib_i2c_write_8bidx8bval(UINT8 u8_address, UINT8 u8_index, UINT8 u8_value)
{
#if VIN_FALSE
	BOOL result = FALSE;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte(u8_index))
		goto STOP;

	if (!_i2c_write_byte(u8_value))
		goto STOP;

	result = TRUE;

STOP:
	_i2c_stop();
	return result;
#endif

	return TRUE;
}

VOID mculib_i2c_burstread_8bidx8bval(UINT8 u8_address, UINT8 u8_index, UINT8 u8_length, UINT8 *pu8_value)
{
#if VIN_FALSE
	UINT8 i;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte(u8_index))
		goto STOP;

	_i2c_start();

	if (!_i2c_write_byte(u8_address | 0x01))
		goto STOP;

	for (i = 0; i < (u8_length-1); i++)
		*pu8_value++ = _i2c_read_byte(FALSE);

	*pu8_value = _i2c_read_byte(TRUE);

STOP:
	_i2c_stop();
#else
	UINT8 i;

	for (i = 0; i < u8_length; i++)
		*(pu8_value + i) = mculib_i2c_read_8bidx8bval(u8_address, u8_index + i);
#endif
}

//8-bit index for HDMI EDID block 2-3 read
//for HDMI CTS test only, not necessary for user
BOOL mculib_i2c_write_blank(UINT8 u8_address, UINT8 u8_index)
{
#if VIN_FALSE
	BOOL result = FALSE;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte(u8_index))
		goto STOP;

	result = TRUE;

STOP:
	///_i2c_stop();
	return result;
#endif

	return TRUE;
}

VOID mculib_i2c_burstread_8bidx8bval_ext(UINT8 u8_address, UINT8 u8_index, UINT8 u8_length)
{
#if VIN_FALSE
	UINT8 i;
	UINT8 read_falg;

	_i2c_start();

	if (!_i2c_write_byte(u8_address))
		goto STOP;

	if (!_i2c_write_byte(u8_index))
		goto STOP;

	_i2c_start();

	if (!_i2c_write_byte(u8_address | 0x01))
		goto STOP;

	for (i = 0; i < (u8_length-1); i++) {
		read_falg = _i2c_read_byte(FALSE);
		// if(i==0)
		// mculib_uart_log1("read_falg=",read_falg);
	}

	_i2c_read_byte(TRUE);

STOP:
	_i2c_stop();
#endif
}

//below APIs is for for sdk internal debug, not necessary for user
VOID mculib_uart_log(UINT8 *u8_string)
{
#if VIN_FALSE
	//mculib_uart_print_string(u8_string);
	//mculib_uart_print_string("\r\n");
	printf((const char *)u8_string);
	printf("\r\n");
#endif
}

VOID mculib_uart_log1(UINT8 *u8_string, UINT16 u16_hex)
{
#if VIN_FALSE
	//mculib_uart_print_string(u8_string);
	//mculib_uart_print_string("0x");
	//mculib_uart_print_hex(u16_hex);
	//mculib_uart_print_string("\r\n");
	printf((const char *)u8_string);
	printf("0x%x\r\n", u16_hex);
#endif
}

VOID mculib_uart_log2(UINT8 *u8_string, UINT16 u16_dec)
{
#if VIN_FALSE
	//mculib_uart_print_string(u8_string);
	//mculib_uart_print_dec(u16_dec);
	//mculib_uart_print_string("\r\n");
	printf((const char *)u8_string);
	printf("%d\r\n", u16_dec);
#endif
}
