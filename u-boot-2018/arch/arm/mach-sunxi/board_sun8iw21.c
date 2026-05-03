// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023-2026
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#ifdef CONFIG_SUNXI_SET_EFUSE_POWER
#include <sunxi_power/power_manage.h>
#include <sunxi_power/pmu_axp2101.h>
#include <linux/delay.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#endif

int get_group_bit_offset(enum pin_e port_group)
{
	switch (port_group) {
	case GPIO_GROUP_A:
	case GPIO_GROUP_C:
	case GPIO_GROUP_D:
	case GPIO_GROUP_E:
	case GPIO_GROUP_F:
	case GPIO_GROUP_G:
	case GPIO_GROUP_I:
		return port_group;
		break;
	default:
		return -1;
	}
    return -1;
}

/* set efuse voltage before burn efuse */
#ifdef CONFIG_SUNXI_SET_EFUSE_POWER
extern int pmu_axp2101_set_reg_value(unsigned char reg_addr, unsigned char reg_value);
extern int pmu_axp2101_get_reg_value(unsigned char reg_addr);
void set_efuse_voltage(int status)
{
	uint val;
	int nodeoffset, len;
	int vol;
	const char *power_supply;
	const char *vol_value = "voltage";
	const char *power_name = "power_supply";

	nodeoffset = fdt_path_offset(working_fdt, "/soc/sid");
	if (nodeoffset < 0) {
		 printf ("libfdt fdt_path_offset() returned %s\n",
				 fdt_strerror(nodeoffset));
		 return ;
	}
	vol = fdt_getprop_u32_default_node(working_fdt, nodeoffset, 0, vol_value, -1);
	power_supply = fdt_getprop(working_fdt, nodeoffset, power_name, &len);

	if (status) {
		pmu_set_voltage((char *)power_supply, vol, -1);
		val = pmu_axp2101_get_reg_value(0x90);
		val |= (0x20);
		pmu_axp2101_set_reg_value(0x90, val);
		mdelay(20);
	} else {
		mdelay(20);
		val = pmu_get_reg_value(0x90);
		val &= ~(1 << 5);
		pmu_set_reg_value(0x90, val);
	}
}
#endif

