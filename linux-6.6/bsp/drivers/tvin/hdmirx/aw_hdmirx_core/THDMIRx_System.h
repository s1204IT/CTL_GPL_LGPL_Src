/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _THDMIRX_SYSTEM_H_
#define _THDMIRX_SYSTEM_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "include/TFDDefs.h"

#define TFC_Support (1)
#define Crypto_Support (0)  // Need OTP_Crytpto and crypto
#define HDMIRX_PR_PATCH (0)
#define HDMRIX_422_PATCH (1)
#define HDMIRX_PORT_NEEDREMAP (1)  // application decide port map
#define HDMIRX_USE_APLL (0)  // use APLL for audio detect


#define _MATCH_HFREQ_ 0x01
#define _MATCH_VFREQ_ 0x02
#define _MATCH_HPOL_ 0x04
#define _MATCH_VPOL_ 0x08
#define _MATCH_HWIDTH_ 0x10
#define _MATCH_VWIDTH_ 0x20
#define _MATCH_HFREQENCY_ 0x40
#define _MATCH_VFREQENCY_ 0x80
#define _MAX_RGBMATCH_LEVEL_ 16

#define _BIT0_ 0x01
#define _BIT1_ 0x02
#define _BIT2_ 0x04
#define _BIT3_ 0x08
#define _BIT4_ 0x10
#define _BIT5_ 0x20
#define _BIT6_ 0x40
#define _BIT7_ 0x80

#define _BIT8_ 0x0100
#define _BIT9_ 0x0200
#define _BIT10_ 0x0400
#define _BIT11_ 0x0800

#define _BIT12_ 0x1000
#define _BIT13_ 0x2000
#define _BIT14_ 0x4000
#define _BIT15_ 0x8000

#define _BIT16_ 0x00010000
#define _BIT17_ 0x00020000
#define _BIT18_ 0x00040000
#define _BIT19_ 0x00080000

#define _BIT20_ 0x00100000
#define _BIT21_ 0x00200000
#define _BIT22_ 0x00400000
#define _BIT23_ 0x00800000

#define _BIT24_ 0x01000000
#define _BIT25_ 0x02000000
#define _BIT26_ 0x04000000
#define _BIT27_ 0x08000000

#define _BIT28_ 0x10000000
#define _BIT29_ 0x20000000
#define _BIT30_ 0x40000000
#define _BIT31_ 0x80000000

#define _MATCH_HFREQ_ 0x01
#define _MATCH_VFREQ_ 0x02
#define _MATCH_HPOL_ 0x04
#define _MATCH_VPOL_ 0x08
#define _MATCH_HWIDTH_ 0x10
#define _MATCH_VWIDTH_ 0x20
#define _MATCH_HFREQENCY_ 0x40
#define _MATCH_VFREQENCY_ 0x80
#define _MAX_RGBMATCH_LEVEL_ 16

/*
typedef struct {
	U8 PortID;
	struct timer_list hpdtimer;
}timer_port_list;
*/
typedef enum {
	_3D_Struct_FramePacking = 0,
	_3D_Struct_FieldAlternative,
	_3D_Struct_LineAlternative,
	_3D_Struct_SideBySide_Full,
	_3D_Struct_L_Depth,
	_3D_Struct_L_Depth_Graphics_GraphicsDepth,
	_3D_Struct_TopBottom,
	_3D_Struct_Reserved,
	_3D_Struct_SideBySide_Half = 0x08,
	_3D_Struct_2DTo3D = 0xFF
} e_3D_Struct;  // HDMI1.4

typedef enum {
	_3D_SideBySide_H_OLOR = 0,
	_3D_SideBySide_H_OLER,
	_3D_SideBySide_H_ELOR,
	_3D_SideBySide_H_ELER,
	_3D_SideBySide_Q_OLOR,
	_3D_SideBySide_Q_OLER,
	_3D_SideBySide_Q_ELOR,
	_3D_SideBySide_Q_ELER,
} e_3D_Ext_Data;

typedef struct {
	u8 b3DOn;
	e_3D_Struct _3D_Struct;
	e_3D_Ext_Data _3D_ExtData;
} THDMI3DInfo;

/********define from display**********/


// bit handle
#define HDMIRx_ReadBit(reg, bit_num) ((reg >> bit_num) & 0x1)
#define HDMIRx_SetBit(reg, bit_num) (reg |= (0x1 << bit_num))
#define HDMIRx_ClrBit(reg, bit_num) (reg &= (~(0x1 << bit_num)))
#define SizeOfArray(array) (sizeof(array) / sizeof(array[0]))

// register handle
#define HDMIRx_ReadRegU8(dwAddr) readb((volatile void __iomem *)(dwAddr))
#define HDMIRx_WriteRegU8(dwAddr, ucVal) writeb(ucVal, (volatile void __iomem *)(dwAddr))
#define HDMIRx_ReadRegU32(dwAddr) readl((volatile void __iomem *)(dwAddr))
#define HDMIRx_WriteRegU32(dwAddr, ucVal) writel(ucVal, (volatile void __iomem *)(dwAddr))


#define IS_DIFF(_p1, _p2) ((_p1) != (_p2))

#define HDMILOG_1_WITH_CONDITION(_c, _s, _p1)  \
		do {	\
			if ((_c)) \
				hdmirx_err(_s, _p1); \
		} while (0);
#define HDMILOG_2_WITH_CONDITION(_c, _s, _p1, _p2)  \
		do {	\
			if ((_c)) \
				hdmirx_err(_s, _p1, _p2); \
		} while (0);

#define CHECK_POINTER(_p, ...)                                  \
	do {                                                        \
		if (!(_p)) {                                            \
			hdmirx_err("%s(), ERROR: %s is NULL!\n", #_p, __func__); \
		}                                                       \
	} while (0)

#define CHECK_RANGE(_p, _min, _max, ...)                                    \
	do {                                                                    \
		if ((_p) < (_min) || (_p) > (_max)) {                               \
			hdmirx_err("%s(), ERROR: %s is out of range!\n", __FUNCTION__, #_p); \
		}                                                                   \
	} while (0)

#define CHECK_LESS_THAN(_p, _max, ...)                                      \
	do {                                                                    \
		if ((_p) >= (_max)) {                                               \
			hdmirx_err("%s(), ERROR: %s is out of range!\n", __FUNCTION__, #_p); \
		}                                                                   \
	} while (0)

#define LOOP_DELAY(_cnt)                               \
	do {                                               \
		U32 delay_i;                                   \
		for (delay_i = (_cnt); delay_i > 0; delay_i--) \
			;                                          \
		;                                              \
	} while (0);

#define CHECK_TIMEOUT(_now, _last, _t, ...)           \
	do {                                              \
		if ((_now) - (_last) > (_t)) {                \
			hdmirx_err("%s(), time out!\n", __FUNCTION__); \
		}                                             \
	} while (0)

#define LOOP_DELAY(_cnt)                               \
	do {                                               \
		U32 delay_i;                                   \
		for (delay_i = (_cnt); delay_i > 0; delay_i--) \
			;                                          \
		;                                              \
	} while (0);

#define CHECK_TIMEOUT(_now, _last, _t, ...)           \
	do {                                              \
		if ((_now) - (_last) > (_t)) {                \
			hdmirx_err("%s(), time out!\n", __FUNCTION__); \
		}                                             \
	} while (0)

void HDMIRx_WriteRegMaskU8(uintptr_t addr, u8 mask, u8 data);
void HDMIRx_WriteRegMaskU32(uintptr_t addr, u32 mask, u32 data);
u32 HdmiRx_GetRegu32_ByRegu8(uintptr_t base, u32 dwAddr);
void HdmiRx_SetRegu8_Byu32(uintptr_t base, u32 dwAddr, u32 dwValue);
u32 HDMIRx_GetCurrentTick(void);
int HDMIRx_CompareTickCount(u32 dwOriginalTickCount, u32 wInterval);

#endif /* _THDMIRX_SYSTEM_H_ */
