/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI ETA6973  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __ETA6973_H__
#define __ETA6973_H__

#define ETA6973_CHIP_ID             (0x78)
#define ETA6974_CHIP_ID             (0x38)

#define ETA6973_DEVICE_ADDR			(0x3A3)
#ifdef CONFIG_ETA6973_SUNXI_I2C_SLAVE
#define ETA6973_RUNTIME_ADDR		CONFIG_ETA6973_SUNXI_I2C_SLAVE
#else
#define ETA6973_RUNTIME_ADDR        (0x6b)
#endif

/* List of registers for eta6973 */
#define ETA6973_REG_00		(0x00)
#define ETA6973_REG_01		(0x01)
#define ETA6973_REG_02		(0x02)
#define ETA6973_REG_03		(0x03)
#define ETA6973_REG_04		(0x04)
#define ETA6973_REG_05		(0x05)
#define ETA6973_REG_06		(0x06)
#define ETA6973_REG_07		(0x07)
#define ETA6973_REG_08		(0x08)
#define ETA6973_REG_09		(0x09)
#define ETA6973_REG_0A		(0x0A)
#define ETA6973_REG_0B		(0x0B)

int bmu_eta6973_probe(void);
int bmu_eta6973_get_vbus_status(void);
bool get_eta6973(void);
#endif /* __ETA6973_REGS_H__ */


