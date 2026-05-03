/*
 * drm_util.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _DRM_UTIL_H
#define _DRM_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * for_each_if - helper for handling conditionals in various for_each macros
 * @condition: The condition to check
 *
 * Typical use::
 *
 *	#define for_each_foo_bar(x, y) \'
 *		list_for_each_entry(x, y->list, head) \'
 *			for_each_if(x->something == SOMETHING)
 *
 * The for_each_if() macro makes the use of for_each_foo_bar() less error
 * prone.
 */
#define for_each_if(condition) if (!(condition)) {} else



#ifdef __cplusplus
}
#endif

#endif /*End of file*/
