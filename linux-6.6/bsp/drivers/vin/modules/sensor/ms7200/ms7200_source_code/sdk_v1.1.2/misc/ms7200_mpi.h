/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/******************************************************************************
* @file    ms7200_mpi.h
* @author
* @version V1.0.0
* @date    15-Nov-2014
* @brief   MacroSilicon Programming Interface.
*
* Copyright (c) 2009-2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_PROGRAMMING_INTERFACE_H__
#define __MACROSILICON_MS7200_PROGRAMMING_INTERFACE_H__

#include "ms7200_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ms7200_chip_reset mculib_chip_reset

//
#define Delay_ms mculib_delay_ms
#define Delay_us mculib_delay_us

#if MS7200_USE_I2CBUS

#define HAL_SetChipAddr ms7200_HAL_SetChipAddr
#define HAL_GetChipAddr ms7200_HAL_GetChipAddr
#define HAL_ReadByte ms7200_HAL_ReadByte
#define HAL_WriteByte ms7200_HAL_WriteByte
#define HAL_ReadWord ms7200_HAL_ReadWord
#define HAL_WriteWord ms7200_HAL_WriteWord
#define HAL_ModBits ms7200_HAL_ModBits
#define HAL_SetBits ms7200_HAL_SetBits
#define HAL_ClrBits ms7200_HAL_ClrBits
#define HAL_ToggleBits ms7200_HAL_ToggleBits
#define HAL_ReadDWord ms7200_HAL_ReadDWord
#define HAL_WriteDWord ms7200_HAL_WriteDWord
#define HAL_ReadBytes ms7200_HAL_ReadBytes
#define HAL_WriteBytes ms7200_HAL_WriteBytes
#define HAL_ReadRange ms7200_HAL_ReadRange
#define HAL_WriteRange ms7200_HAL_WriteRange
#define HAL_ModBits_Ex ms7200_HAL_ModBits_Ex
#define HAL_SetBits_Ex ms7200_HAL_SetBits_Ex
#define HAL_ClrBits_Ex ms7200_HAL_ClrBits_Ex
#define HAL_ToggleBits_Ex ms7200_HAL_ToggleBits_Ex
#define HAL_ReadDWord_Ex ms7200_HAL_ReadDWord_Ex
#define HAL_WriteDWord_Ex ms7200_HAL_WriteDWord_Ex
#define HAL_ReadRange_Ex ms7200_HAL_ReadRange_Ex
#define HAL_WriteRange_Ex ms7200_HAL_WriteRange_Ex
#endif


/***************************************************************
*  Function name: HAL_SetChipAddr
*  Description:   change I2C slave u8_address
*  Input parameters: UINT8 u8_address: chip slave u8_address
*
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_SetChipAddr(UINT8 u8_address);

/***************************************************************
*  Function name: HAL_GetChipAddr
*  Description:   change I2C slave u8_address
*  Input parameters: None
*
*  Output parameters: None
*  Returned value: UINT8 u8_address: chip slave u8_address
***************************************************************/
UINT8 HAL_GetChipAddr(VOID);

/***************************************************************
*  Function name: HAL_ReadByte
*  Description: read back 8 bits register value with 16 bits specified index
*  Input parameters: UINT16 u16_index: 16 bits register index
*  Output parameters: None
*  Returned value: UINT8 type register value
***************************************************************/
UINT8 HAL_ReadByte(UINT16 u16_index);


/***************************************************************
*  Function name: HAL_WriteByte
*  Description: write 8 bits register value to 16 bits specified index
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8  u8_value: 8 bits rgister value
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_WriteByte(UINT16 u16_index, UINT8 u8_value);


/***************************************************************
*  Function name: HAL_ReadWord
*  Description: equal to "HAL_ReadByte(u16_index) +
*                        (UINT16)(HAL_ReadByte(u16_index + 1) << 8)"
*  Input parameters: UINT16 u16_index: 16 bits register index
*  Output parameters: None
*  Returned value: UINT16 type register value
***************************************************************/
UINT16 HAL_ReadWord(UINT16 u16_index);


/***************************************************************
*  Function name:  HAL_WriteWord
*  Description: equal to "HAL_WriteByte(u16_index, (UINT8)u16_value);
				HAL_WriteByte(u16_index + 1, (UINT8)(u16_value >> 8))"
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT16 u16_value: 16 bits rgister value
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_WriteWord(UINT16 u16_index, UINT16 u16_value);


/***************************************************************
*  Function name: HAL_ModBits
*  Description: modify register value with bit mask (high active) for 16 bits index and 8 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8 u8_mask: 8 bits mask
*                                   can be set by bit mask macro or compsite of macros
*                    UINT8 u8_value: 8 bits value, value with masked bits will be ignored
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_ModBits(UINT16 u16_index, UINT8 u8_mask, UINT8 u8_value);


/***************************************************************
*  Function name: HAL_SetBits
*  Description: set bits with bit mask (high active) for 16 bits index and 8 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8 u8_mask: 8 bits mask
*                                      can be set by bit mask macro or compsite of macros
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_SetBits(UINT16 u16_index, UINT8 u8_mask);

/***************************************************************
*  Function name:  HAL_ClrBits
*  Description: clear bits with bit mask (high active) for 16 bits index and 8 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8 u8_mask: 8 bits mask
*                                      can be set by bit mask macro or compsite of macros
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_ClrBits(UINT16 u16_index, UINT8 u8_mask);

/***************************************************************
*  Function name:  HAL_ToggleBits
*  Description: toggle bits with bit mask (high active) for 16 bits index and 8 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8 u8_mask: 8 bits mask
*                                      can be set by bit mask macro or compsite of macros
*                            BOOL b_set, if true set to 1 else 0
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_ToggleBits(UINT16 u16_index, UINT8 u8_mask, BOOL b_set);

//
/***************************************************************
*  Function name:  HAL_ReadDWord
*  Description: read 32 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*  Output parameters: None
*  Returned value: 32 bits value
***************************************************************/
UINT32 HAL_ReadDWord(UINT16 u16_index);

/***************************************************************
*  Function name:  HAL_WriteDWord
*  Description: write 32 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT32 u32_value: 32 bits value
*  Output parameters: None
*  Returned value: 32 bits value
***************************************************************/
VOID HAL_WriteDWord(UINT16 u16_index, UINT32 u32_value);

//I2C read/write with length
/***************************************************************
*  Function name:  HAL_ReadBytes
*  Description: burst mode read
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT16 u16_length: 16 bits length
*                    UINT8 *p_u8_value: data buffer to read
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_ReadBytes(UINT16 u16_index, UINT16 u16_length, UINT8 *p_u8_value);

/***************************************************************
*  Function name:  HAL_WriteBytes
*  Description: burst mode write
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT16 u16_length: 16 bits length
*                    UINT8 *p_u8_value: data buffer to write
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_WriteBytes(UINT16 u16_index, UINT16 u16_length, UINT8 *p_u8_value);

#if MS7200_EXT_APIS
/***************************************************************
*  Function name: UINT32 HAL_ReadRange(UINT16 u16_index, UINT8 u8_bitpos, UINT8 u8_length)
*  Description:   read back 32 bits register value by specified index with start bit and length
*                 in case of 16 bits index and 8 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8 u8_bitpos: start bit of fisrt index register, should be less than 8
*                    UINT8 u8_length: register length , should less than 33
*  Output parameters: None
*  Returned value: UINT32 type register value
***************************************************************/
UINT32 HAL_ReadRange(UINT16 u16_index, UINT8 u8_bitpos, UINT8 u8_length);


/***************************************************************
*  Function name: VOID HAL_WriteRange(UINT16 u16_index, UINT8 u8_bitpos, UINT8 u8_length, UINT32 u32_value)
*  Description:   write 32 bits register value to specified index with start bit and length
*                 in case of 16 bits index and 8 bits value
*  Input parameters: UINT16 u16_index: 16 bits register index
*                    UINT8 u8_bitpos: start bit of fisrt index register, should be less than 8
*                    UINT8 u8_length: register length , should less than 33
*                    UINT32 u32_value: 32 bits register value
*  Output parameters: None
*  Returned value: None
***************************************************************/
VOID HAL_WriteRange(UINT16 u16_index, UINT8 u8_bitpos, UINT8 u8_length, UINT32 u32_value);
#endif


//extend APIs is for HDMI RX register access only
VOID HAL_ModBits_Ex(UINT16 u16_index, UINT32 u32_mask, UINT32 u32_value);
VOID HAL_SetBits_Ex(UINT16 u16_index, UINT32 u32_mask);
VOID HAL_ClrBits_Ex(UINT16 u16_index, UINT32 u32_mask);
VOID HAL_ToggleBits_Ex(UINT16 u16_index, UINT32 u32_mask, BOOL b_set);
UINT32 HAL_ReadDWord_Ex(UINT16 u16_index);
VOID HAL_WriteDWord_Ex(UINT16 u16_index, UINT32 u32_value);
UINT32 HAL_ReadRange_Ex(UINT16 u16_index, UINT8 u8_bitpos, UINT8 u8_length);
VOID HAL_WriteRange_Ex(UINT16 u16_index, UINT8 u8_bitpos, UINT8 u8_length, UINT32 u32_value);


//
#if MS7200_DEBUG_LOG

#ifndef _PLATFORM_WINDOWS_
#define mculib_log printf
#endif

#define MS7200_PRINTF       mculib_log
#define MS7200_LOG(X)       mculib_uart_log((UINT8 *)X)
#define MS7200_LOG1(X, Y)   mculib_uart_log1((UINT8 *)X, Y)
#define MS7200_LOG2(X, Y)   mculib_uart_log2((UINT8 *)X, Y)

#else
#define MS7200_PRINTF
#define MS7200_LOG(X)
#define MS7200_LOG1(X, Y)
#define MS7200_LOG2(X, Y)
#endif

#ifdef __cplusplus
}
#endif

#endif  // __MACROSILICON_MS7200_PROGRAMMING_INTERFACE_H__
