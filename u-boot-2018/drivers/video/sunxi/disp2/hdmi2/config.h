/*
 * drivers/video/sunxi/disp2/hdmi2/config.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
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
#ifndef _CONFIG_H_
#define _CONFIG_H_
/* #define __LINUX_PLAT__ */

#if defined(__LINUX_PLAT__)
/* define use hdmi phy model */
#if (IS_ENABLED(CONFIG_ARCH_SUN8IW20))
     #ifndef CONFIG_AW_AWPHY
     #define CONFIG_AW_AWPHY        /* allwinner phy */
     #endif
#elif (IS_ENABLED(CONFIG_ARCH_SUN55IW3))
     #ifndef CONFIG_AW_INNOPHY
     #define CONFIG_AW_INNOPHY      /* innosilicon phy */
     #endif
#else
     #ifndef CONFIG_AW_SNPSPHY
     #define CONFIG_AW_SNPSPHY      /* synopsys phy */
     #endif
#endif
#else
/* define use hdmi phy model */
#if (IS_ENABLED(CONFIG_MACH_SUN8IW20))
     #ifndef CONFIG_AW_AWPHY
     #define CONFIG_AW_AWPHY        /* allwinner phy */
     #endif
#elif (IS_ENABLED(CONFIG_MACH_SUN55IW3))
     #ifndef CONFIG_AW_INNOPHY
     #define CONFIG_AW_INNOPHY      /* innosilicon phy */
     #endif
#else
     #ifndef CONFIG_AW_SNPSPHY
     #define CONFIG_AW_SNPSPHY      /* synopsys phy */
     #endif
#endif
#endif /* __LINUX_PLAT__ */

#endif /* _CONFIG_H_ */
