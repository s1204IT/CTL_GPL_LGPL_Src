// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for maxio PHYs
 *
 * Author: zhao yang <yang_zhao@maxio-tech.com>
 *
 * Copyright (c) 2021 maxio technology, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/bitops.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/netdevice.h>

#define MAXIO_PHY_VER					"v1.7.4.2"
#define MAXIO_PAGE_SELECT				0x1f
#define MAXIO_MAE0621A_CLK_MODE_REG		0x02

static int maxio_read_paged(struct phy_device *phydev, int page, u32 regnum)
{
	int ret = 0, oldpage;

	oldpage = phy_read(phydev, MAXIO_PAGE_SELECT);
	if (oldpage >= 0) {
		phy_write(phydev, MAXIO_PAGE_SELECT, page);
		ret = phy_read(phydev, regnum);
	}
	phy_write(phydev, MAXIO_PAGE_SELECT, oldpage);

	return ret;
}

static int maxio_write_paged(struct phy_device *phydev, int page, u32 regnum, u16 val)
{
	int ret = 0, oldpage;

	oldpage = phy_read(phydev, MAXIO_PAGE_SELECT);
	if (oldpage >= 0) {
		phy_write(phydev, MAXIO_PAGE_SELECT, page);
		ret = phy_write(phydev, regnum, val);
	}
	phy_write(phydev, MAXIO_PAGE_SELECT, oldpage);

	return ret;
}

static int maxio_adcc_check(struct phy_device *phydev)
{
	int ret = 0;
	int adcvalue;
	u32 regval;
	int i;

	maxio_write_paged(phydev, 0xd96, 0x2, 0x1fff);
	maxio_write_paged(phydev, 0xd96, 0x2, 0x1000);

	for (i = 0; i < 4; i++) {
		regval = 0xf908 + i * 0x100;
		maxio_write_paged(phydev, 0xd8f, 0xb, regval);
		adcvalue = maxio_read_paged(phydev, 0xd92, 0xb);
		if (adcvalue & 0x1ff) {
			continue;
		} else {
			ret = -1;
			break;
		}
	}

	return ret;
}

static int maxio_self_check(struct phy_device *phydev, int checknum)
{
	int ret = 0;
	int i;

	for (i = 0; i < checknum; i++) {
		ret = maxio_adcc_check(phydev);
		if (ret == 0) {
			printk("MAE0621A READY\n");
			break;
		} else {
			maxio_write_paged(phydev, 0x0, 0x0, 0x1940);
			phy_write(phydev, MAXIO_PAGE_SELECT, 0x0);
			msleep(10);
			maxio_write_paged(phydev, 0x0, 0x0, 0x1140);
			maxio_write_paged(phydev, 0x0, 0x0, 0x9140);
		}
	}

	maxio_write_paged(phydev, 0xd96, 0x2, 0xfff);
	maxio_write_paged(phydev, 0x0, 0x0, 0x9140);
	phy_write(phydev, MAXIO_PAGE_SELECT, 0x0);

	return ret;
}
static int maxio_mae0621a_config_aneg(struct phy_device *phydev)
{
	return genphy_config_aneg(phydev);
}

static int maxio_mae0621a_config_init(struct phy_device *phydev)
{
	int ret = 0;
	u32 broken = 0;

	phydev_info(phydev, "MAXIO_PHY_VER: %s\n", MAXIO_PHY_VER);

	//mdc set
	ret = maxio_write_paged(phydev, 0xda0, 0x10, 0xf13);

	ret |= maxio_write_paged(phydev, 0xd90, 0x2, 0x1555);
	ret |= maxio_write_paged(phydev, 0xd90, 0x5, 0x2b15);

	//oscillator
	ret |= maxio_write_paged(phydev, 0xd92, 0x02, 0x210a);
	phydev_info(phydev, "%s clkmode(oscillator) PHY_ID: %#x\n", __func__, phydev->phy_id);

	//disable eee
	broken |= MDIO_EEE_100TX;
	broken |= MDIO_EEE_1000T;
	phydev->eee_broken_modes = broken;

	//enable auto_speed_down
	ret |= maxio_write_paged(phydev, 0xd8f, 0x0, 0x300);

	//adjust ANE
	ret |= maxio_write_paged(phydev, 0xd96, 0x13, 0x7bc);
	ret |= maxio_write_paged(phydev, 0xd8f, 0x8, 0x2500);
	ret |= maxio_write_paged(phydev, 0xd91, 0x6, 0x6880);

	phy_write(phydev, MAXIO_PAGE_SELECT, 0x0);
	ret |= maxio_self_check(phydev, 50);
	msleep(100);

	return 0;
}

static int maxio_mae0621a_resume(struct phy_device *phydev)
{
	int ret = genphy_resume(phydev);
	//soft reset
	ret |= phy_write(phydev, MII_BMCR, BMCR_RESET | phy_read(phydev, MII_BMCR));
	msleep(20);

	return ret;
}

static int maxio_mae0621a_suspend(struct phy_device *phydev)
{
	int ret = genphy_suspend(phydev);
	//back to 0 page
	ret |= phy_write(phydev, MAXIO_PAGE_SELECT, 0);

	return ret;
}

static int maxio_mae0621a_status(struct phy_device *phydev)
{
	return genphy_read_status(phydev);
}

static int maxio_mae0621a_probe(struct phy_device *phydev)
{
	//oscillator
	maxio_write_paged(phydev, 0xd92, 0x02, 0x210a);
	phydev_info(phydev, "%s clkmode(oscillator) PHY_ID: %#x\n", __func__, phydev->phy_id);
	phy_write(phydev, MAXIO_PAGE_SELECT, 0x0);
	mdelay(100);
	return 0;
}

static int maxio_mae0621aq3ci_config_init(struct phy_device *phydev)
{
	int ret = 0;
	u32 broken = 0;

	phydev_info(phydev, "MAXIO_PHY_VER: %s\n", MAXIO_PHY_VER);

	//disable eee
	broken |= MDIO_EEE_100TX;
	broken |= MDIO_EEE_1000T;
	phydev->eee_broken_modes = broken;
	//new patch v0517
	//MDC set
	ret = maxio_write_paged(phydev, 0xdab, 0x17, 0xf13);

	//auto speed down enable
	ret |= maxio_write_paged(phydev, 0xd8f, 0x10, 0x300);

	//bandgap set
	ret |= maxio_write_paged(phydev, 0xd96, 0x15, 0xc08a);

	//ZX
	ret |= maxio_write_paged(phydev, 0xda4, 0x12, 0x7bc);
	ret |= maxio_write_paged(phydev, 0xd8f, 0x16, 0x2500);

	ret |= maxio_write_paged(phydev, 0xd90, 0x16, 0x1555);
	ret |= maxio_write_paged(phydev, 0xd92, 0x11, 0x2b15);

	ret |= maxio_write_paged(phydev, 0xd96, 0x16, 0x4010);
	ret |= maxio_write_paged(phydev, 0xda5, 0x10, 0x4a12);
	ret |= maxio_write_paged(phydev, 0xda5, 0x11, 0x4a12);
	ret |= maxio_write_paged(phydev, 0xda5, 0x12, 0x4a12);
	ret |= maxio_write_paged(phydev, 0xda8, 0x11, 0x175);

	//clkout 125MHZ
	ret |= maxio_write_paged(phydev, 0xa43, 0x19, 0x823);

	//soft reset
	ret |= maxio_write_paged(phydev, 0x0, 0x0, 0x9140);

	//back to 0 page
	ret |= phy_write(phydev, MAXIO_PAGE_SELECT, 0);

	return 0;
}

static int maxio_mae0621aq3ci_resume(struct phy_device *phydev)
{
	int ret = 0;

	ret = genphy_resume(phydev);
	ret |= maxio_write_paged(phydev, 0xdaa, 0x17, 0x1001);
	//led set
	ret |= maxio_write_paged(phydev, 0xdab, 0x15, 0x0);
	ret |= phy_write(phydev, MAXIO_PAGE_SELECT, 0);

	return ret;
}

static int maxio_mae0621aq3ci_suspend(struct phy_device *phydev)
{
	int ret = 0;

	ret = maxio_write_paged(phydev, 0xdaa, 0x17, 0x1011);
	//led set
	ret |= maxio_write_paged(phydev, 0xdab, 0x15, 0x5550);
	ret |= phy_write(phydev, MAXIO_PAGE_SELECT, 0);
	ret |= genphy_suspend(phydev);
	ret |= phy_write(phydev, MAXIO_PAGE_SELECT, 0);

	return ret;
}

static struct phy_driver maxio_nc_drvs[] = {
	{
		.phy_id			= 0x4411,
		.phy_id_mask	= 0xffff,
		.name		= "MAE0621A Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.probe			= maxio_mae0621a_probe,
		.config_init	= maxio_mae0621a_config_init,
		.config_aneg	= maxio_mae0621a_config_aneg,
		.read_status	= maxio_mae0621a_status,
		.suspend	= maxio_mae0621a_suspend,
		.resume		= maxio_mae0621a_resume,
	},
	{
		.phy_id			= 0x7b744412,
		.phy_id_mask	= 0x7fffffff,
		.name		= "MAE0621A-Q3C(I) Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.config_init	= maxio_mae0621aq3ci_config_init,
		.suspend	= maxio_mae0621aq3ci_suspend,
		.resume		= maxio_mae0621aq3ci_resume,
	},
};
module_phy_driver(maxio_nc_drvs);

static struct mdio_device_id __maybe_unused maxio_nc_tbl[] = {
	{ 0x4411, 0xffff },
	{ 0x7b744412, 0x7fffffff },
	{ }
};
MODULE_DEVICE_TABLE(mdio, maxio_nc_tbl);

MODULE_DESCRIPTION("Maxio PHY driver");
MODULE_AUTHOR("Zhao Yang");
MODULE_LICENSE("GPL");
