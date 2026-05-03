/*
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <console.h>
#include <environment.h>
#include <fdtdec.h>
#include <fs.h>
#include <i2c.h>
#include <initcall.h>
#include <malloc.h>
#include <mapmem.h>
#include <os.h>
#include <post.h>
#include <relocate.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <linux/errno.h>
#include <spare_head.h>
#include <sunxi_board.h>
#include <private_uboot.h>
#include <boot_param.h>
#include <sunxi_gpadc.h>
#include <fdt_support.h>
#include <sunxi_sys_config_gpio.h>

#define PIN_NUM_MAX    4
#define ADC_MASK       0x0f
#define ADCS_MASK      0xf0
#define FDT_PATH       "/sunxi_dram_para_select"
#define PIO_ONE_PIN_DATA(n, i)		(((*(volatile unsigned int *)(SUNXI_PIO_BASE + ((n) - 1) * PIOC_o_OFFSET + PIOC_REG_o_DATA)) & (1 << i)) >> i)

struct dram_para_pin_message {
	int pin_num;
	user_gpio_set_t pin_set[PIN_NUM_MAX];
};

struct dram_para_pin_message pin_message;

int global_dram_para_select = -EAGAIN;
int sunxi_get_dram_para_select(void)
{
	int i, node, ret = 0, io_en = 0;
	u32 adc_val, select_mode;
	const char *status;
	int enable_flag = 0;
	u32 adc_channel, select_dram_para = 0;
	u32 vol_range[] = {163, 382, 608, 811, 1050, 1315, 1569, 1800}; /*mV*/

	node = fdt_node_offset_by_compatible(working_fdt, 0, "allwinner,sunxi-dram-para-select");
	if (node < 0) {
		pr_err("unable to find allwinner,sunxi-dram-para-select node in device tree.\n");
		return node;
	}

	status = fdt_getprop(working_fdt, node, "status", NULL);

    if (status) {
		if (!strcmp(status, "okay"))
			enable_flag = 1;
    }

	if (enable_flag) {
		ret =  fdt_getprop_u32(working_fdt, node, "gpadc_channel", &adc_channel);
		ret |= fdt_getprop_u32(working_fdt, node, "select_mode", &select_mode);
		if (ret < 0) {
			pr_err("sunxi-dram-para-select get fdt para is error\n");
			return ret;
		}

		pin_message.pin_num = fdt_get_all_pin(node, "pinctrl-0", pin_message.pin_set);
		if (pin_message.pin_num > PIN_NUM_MAX) {
			pr_err("sunxi-dram-para-select get pin num larger than max\n");
			return -EINVAL;
		} else if (pin_message.pin_num < 0) {
			pr_err("sunxi-dram-para-select get pin num error\n");
			return pin_message.pin_num;
		}

		for (i = 0; i < pin_message.pin_num; i++) {
			pr_debug("name is %s, port is %d, port num is %d, mux is %d pll is %d\n",\
					 pin_message.pin_set[i].gpio_name,\
					 pin_message.pin_set[i].port,\
					 pin_message.pin_set[i].port_num,\
					 pin_message.pin_set[i].mul_sel,
					 pin_message.pin_set[i].pull);
		}

		fdt_set_all_pin(FDT_PATH, "pinctrl-0");

		if (select_mode == 1) {
			for (i = 0; i < pin_message.pin_num; i++) {
				if (pin_message.pin_set[i].port == 0)
					continue;
				select_dram_para |= (PIO_ONE_PIN_DATA((pin_message.pin_set[i].port), (pin_message.pin_set[i].port_num)) << i);
			}
		} else if (select_mode != 0) {

			ret = sunxi_gpadc_init();
			if (ret)
				pr_err("sunxi gpadc init is error\n");

			adc_val = sunxi_get_gpadc_vol(adc_channel & ADC_MASK);
			for (select_dram_para = 0; select_dram_para < (sizeof(vol_range)/sizeof(vol_range[0]) - 1); select_dram_para++) {
				if ((adc_val < (vol_range[select_dram_para + 1] + vol_range[select_dram_para]) / 2)) {
					break;
				}
			}

			/* read aother gpio */
			if (select_mode == 3) {
				for (i = 0; i < pin_message.pin_num; i++) {
					if (pin_message.pin_set[i].port == 0)
						continue;
					io_en = PIO_ONE_PIN_DATA((pin_message.pin_set[i].port), (pin_message.pin_set[i].port_num));
					break;
				}
			}

			if (select_mode == 4) {
				int adc_vol = sunxi_get_gpadc_vol((adc_channel & ADCS_MASK) >> 4);
				if (adc_vol <= 1818 && adc_vol >= 1782) {
					io_en = 1;
				}
			}
		}

		select_dram_para += (io_en * 8);
		global_dram_para_select = select_dram_para;
	}
	return 0;

}
