/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
  This file contains common structs for vdecoder and vencoder.

  @author eric_wang
  @date 20241011
*/
#ifndef _VCODEC_BASE_H_
#define _VCODEC_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum eVeLbcMode {
	LBC_MODE_DISABLE  = 0,
	LBC_MODE_1_5X     = 1,
	LBC_MODE_2_0X     = 2,
	LBC_MODE_2_5X     = 3,
	LBC_MODE_NO_LOSSY = 4,
	LBC_MODE_1_0X     = 5,
} eVeLbcMode;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _VCODEC_BASE_H_ */

