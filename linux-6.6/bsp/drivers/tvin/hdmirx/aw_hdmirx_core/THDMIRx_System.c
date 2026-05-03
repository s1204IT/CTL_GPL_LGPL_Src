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
#include "THDMIRx_System.h"

/* calculate valid bit account */
static u8 lowestBitSet(u8 x)
{
	u8 result = 0;

	/* x=0 is not properly handled by while-loop */
	if (x == 0)
		return 0;

	while ((x & 1) == 0) {
		x >>= 1;
		result++;
	}

	return result;
}

void HDMIRx_WriteRegMaskU8(uintptr_t addr, u8 mask, u8 data)
{
	u8 temp = 0;

	temp = HDMIRx_ReadRegU8(addr);
	temp &= ~(mask);
	temp |= (mask & data);
	HDMIRx_WriteRegU8(addr, temp);
}

void HDMIRx_WriteRegMaskU32(uintptr_t addr, u32 mask, u32 data)
{
	u32 temp = 0;

	temp = HDMIRx_ReadRegU32(addr);
	//printk("111temp = 0x%x\n", temp);
	temp &= ~(mask);
	temp |= (mask & data);
	//printk("222temp = 0x%x\n", temp);
	HDMIRx_WriteRegU32(addr, temp);
}
u32 HdmiRx_GetRegu32_ByRegu8(uintptr_t base, u32 dwAddr)
{
	u32 dwVal = 0;
	uintptr_t dwRegAddr = 0;

	dwRegAddr = base + dwAddr;

	dwVal = HDMIRx_ReadRegU8(dwRegAddr + 3) << 24;
	dwVal |= HDMIRx_ReadRegU8(dwRegAddr + 2) << 16;
	dwVal |= HDMIRx_ReadRegU8(dwRegAddr + 1) << 8;
	dwVal |= HDMIRx_ReadRegU8(dwRegAddr);

	return dwVal;
}

void HdmiRx_SetRegu8_Byu32(uintptr_t base, u32 dwAddr, u32 dwValue)
{
	uintptr_t dwBaseAddr = base + dwAddr;

	dwBaseAddr = dwBaseAddr - dwBaseAddr % 4;

	HDMIRx_WriteRegU8(dwBaseAddr + 3, (dwValue >> 24) & 0xFF);
	HDMIRx_WriteRegU8(dwBaseAddr + 2, (dwValue >> 16) & 0xFF);
	HDMIRx_WriteRegU8(dwBaseAddr + 1, (dwValue >> 8) & 0xFF);
	HDMIRx_WriteRegU8(dwBaseAddr, (dwValue)&0xFF);
}

// System Tick
u32 HDMIRx_GetCurrentTick(void)
{
	ktime_t now_time;
	u32 milliseconds;

	now_time = ktime_get();
	milliseconds = ktime_to_ms(now_time);

	return milliseconds;
}

//wInterval = currenttime - dwOriginalTickCount
int HDMIRx_CompareTickCount(u32 dwOriginalTickCount, u32 wInterval)
{
	ktime_t now_time;
	u32 milliseconds;

	now_time = ktime_get();
	// time interval（ms）
	milliseconds = ktime_to_ms(now_time);
	return (milliseconds - dwOriginalTickCount >= wInterval) ? 1 : 0;
}