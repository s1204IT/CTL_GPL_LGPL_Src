/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <sunxi_board.h>
#include <physical_key.h>
#include <sunxi_power/axp.h>

#define READ_POWER_KEY_TIMEOUT		3000

static int press_key_check(void)
{
	int ret, power_key_cnt = 0;
	int start_time;

	ret = sunxi_phykey_probe();
	if (ret) {
		pr_err("phy key probe is error\n");
		return ret;
	}

	ret = sunxi_phykey_init();
	if (ret) {
		pr_err("phy key init is error\n");
		return ret;
	}

	ret = -ENOSYS;
	start_time = get_timer_masked();

	while (sunxi_phykey_read() > 0) {
		if (pmu_get_key_irq() > 0)
			power_key_cnt++;
		if (power_key_cnt >= 3)
			return 0;
		if ((get_timer_masked() - start_time) > READ_POWER_KEY_TIMEOUT) {
			break;
		}
	}

	return ret;
}

int sunxi_check_update_by_phykey(void)
{
	int ret;
	ret = press_key_check();

	if (!ret)
		sunxi_board_run_fel();

	return 0;
}
