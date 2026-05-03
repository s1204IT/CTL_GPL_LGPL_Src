/*
 *
 * $Id: stk3x1x.h
 *
 * Copyright (C) 2012~2013 Lex Hsieh     <lex_hsieh@sensortek.com.tw>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */
#ifndef __STK342X_H__
#define __STK342X_H__

#define STK_GPIO_PULL_DEFAULT   ((u32)-1)
#define STK_GPIO_DRVLVL_DEFAULT ((u32)-1)
#define STK_GPIO_DATA_DEFAULT   ((u32)-1)

#include <linux/pinctrl/pinconf-generic.h>
#include "stk_ges_test.h"
/*
 * PIN_CONFIG_PARAM_MAX - max available value of 'enum pin_config_param'.
 * see include/linux/pinctrl/pinconf-generic.h
 */
//#define PIN_CONFIG_PARAM_MAX    PIN_CONFIG_PERSIST_STATE
/*enum stk_pin_config_param {
	STK_PINCFG_TYPE_FUNC = PIN_CONFIG_PARAM_MAX + 1,
	STK_PINCFG_TYPE_DAT,
	STK_PINCFG_TYPE_PUD,
	STK_PINCFG_TYPE_DRV,
};*/

#endif /*__STK3X1X_H__*/
