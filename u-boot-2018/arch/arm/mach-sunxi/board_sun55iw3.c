/*
 * Allwinner Sun50iw10 do poweroff in uboot with arisc
 *
 * (C) Copyright 2021  <xinouyang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <sunxi_board.h>
#include <smc.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/efuse.h>

#ifdef CONFIG_SUNXI_PMU_EXT
#include <sunxi_power/power_manage.h>
#include <fdt_support.h>
#endif

int get_group_bit_offset(enum pin_e port_group)
{
	switch (port_group) {
	case GPIO_GROUP_B:
	case GPIO_GROUP_C:
	case GPIO_GROUP_D:
	case GPIO_GROUP_E:
	case GPIO_GROUP_F:
	case GPIO_GROUP_G:
	case GPIO_GROUP_H:
	case GPIO_GROUP_I:
	case GPIO_GROUP_J:
	case GPIO_GROUP_K:
		return port_group;
		break;
	default:
		return -1;
	}
	return -1;
}

int sunxi_platform_power_off(int status)
{
	int work_mode = get_boot_work_mode();
	/* imporve later */

	if (work_mode != WORK_MODE_BOOT)
		return 0;

	/* flush cache to dram */
	flush_dcache_range(
		(ulong)ALIGN_DOWN(((ulong)working_fdt), CONFIG_SYS_CACHELINE_SIZE),
		(ulong)(ALIGN(((ulong)working_fdt + working_fdt->totalsize),
			      CONFIG_SYS_CACHELINE_SIZE)));


	/* startup cpus before shutdown */
	arm_svc_arisc_startup((ulong)working_fdt);

	if (status)
		arm_svc_poweroff_charge();
	else
		arm_svc_poweroff();

	while (1) {
		asm volatile ("wfi");
	}
	return 0;
}

#ifdef CONFIG_SUNXI_PMU_EXT
int axp1530_ext_set_dcdc3(void)
{
	char vf_name[48];
	char numStr[48];
	char vf_type_num[48];
	const char *string = "/";
	const char *vfname = "opp-";
	const char *pname = "opp-microvolt-vf";
	u32 efuse, efuse_ext, markid;
	int ndr_vol = -1, vf_type;
	uint32_t phandle, npu_freq;
	int npu_offset, ret;

	markid = readl(SUNXI_SID_BASE + 0x200) & 0xffff;
	efuse = (readl(SUNXI_SID_BASE + 0x200 + 0x48) & 0xff0) >> 4;
	efuse_ext = (readl(SUNXI_SID_BASE + 0x200 + 0x48) & 0xff000) >> 12;
	if (efuse_ext)
		efuse = efuse_ext;

	/* disabled npu's vol set */
	fdt_find_and_setprop(working_fdt, "npu", "npu-regulator",
			     "0", 1, 1);

	npu_offset = fdt_path_offset(working_fdt, "npu");
	if (npu_offset < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* get npu's vol and set in uboot*/
	ret = fdt_getprop_u32(working_fdt, npu_offset, "npu-vf", &npu_freq);
	if (ret < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	sprintf(numStr, "%d", npu_freq);
	memset(vf_name, 0, sizeof(vf_name));
	strcpy(vf_name, vfname);
	strcat(vf_name, numStr);

	ret = fdt_getprop_u32(working_fdt, npu_offset, "operating-points-v2", &phandle);
	if (ret < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	npu_offset = fdt_node_offset_by_phandle(working_fdt, phandle);
	if (npu_offset < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	ret = fdt_get_path(working_fdt, npu_offset, numStr, 32);
	if (ret < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}
	strcat(numStr, string);
	strcat(numStr, vf_name);

	switch (efuse) {
	case 0x00:
		if (markid == 0x5200) {
			vf_type = 0;
		} else {
			vf_type = 1;
		}
		break;
	case 0x01:
		vf_type = 1;
		break;
	case 0x02:
		vf_type = 2;
		break;
	case 0x12:
		vf_type = 21;
		break;
	case 0x04:
		vf_type = 3;
		break;
	case 0x14:
		vf_type = 31;
		break;
	case 0x05:
		vf_type = 4;
		break;
	case 0x06:
		vf_type = 5;
		break;
	default:
		vf_type = 1;
		break;
	}

	if (!vf_type) {
		ndr_vol = 900000;
	} else {
		sprintf(vf_type_num, "%d", vf_type);
		memset(vf_name, 0, sizeof(vf_name));
		strcpy(vf_name, pname);
		strcat(vf_name, vf_type_num);
		script_parser_fetch(numStr, vf_name, &ndr_vol, -1);
	}

	pr_debug("npu_freq = %d ndr_vol:%d\n", npu_freq, ndr_vol);
	if (ndr_vol > 0)
		pmu_ext_set_voltage("ext_dcdc3", ndr_vol / 1000, 1);

	return 0;
}

int update_pmu_ext_info_to_kernel(void)
{
	int nodeoffset, pmu_ext_type, err, i;
	uint32_t phandle = 0;

	/* get pmu_ext type */
	pmu_ext_type = pmu_ext_get_type();
	if (pmu_ext_type < 0) {
		pr_err("Could not find pmu_ext type: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* get used pmu_ext node */
	nodeoffset = fdt_path_offset(working_fdt, pmu_ext_reg[pmu_ext_type]);
	if (nodeoffset < 0) {
		pr_err("Could not find nodeoffset for used ext pmu:%s\n", pmu_ext_reg[pmu_ext_type]);
		return -1;
	}
	/* get used pmu_ext phandle */
	phandle = fdt_get_phandle(working_fdt, nodeoffset);
	if (!phandle) {
		pr_err("Could not find phandle for used ext pmu:%s\n", pmu_ext_reg[pmu_ext_type]);
		return -1;
	}
	pr_debug("get ext power phandle %d\n", phandle);

	/* delete other pmu_ext node */
	for (i = 0; i < NR_PMU_EXT_VARIANTS; i++) {
		if (i == pmu_ext_type)
			continue;

		nodeoffset = fdt_path_offset(working_fdt, pmu_ext[i]);
		if (nodeoffset < 0) {
			pr_warn("Could not find nodeoffset for unused ext pmu:%s\n", pmu_ext[i]);
			continue;
		}

		err = fdt_del_node(working_fdt, nodeoffset);
		if (err < 0) {
			pr_err("WARNING: fdt_del_node can't delete %s from node %s: %s\n",
				"compatible", "status", fdt_strerror(err));
			return -1;
		}
	}

	/* get cpu@4 node */
	nodeoffset = fdt_path_offset(working_fdt, "cpu-ext");
	if (nodeoffset < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* Change cpu-supply to ext dcdc*/
	err = fdt_setprop_u32(working_fdt, nodeoffset,
				"cpu-supply", phandle);
	if (err < 0) {
		pr_warn("WARNING: fdt_setprop can't set %s from node %s: %s\n",
			"compatible", "status", fdt_strerror(err));
		return -1;
	}

	/*special treatment for axp1530's dcdc3*/
	if (pmu_ext_type == AXP1530)
		axp1530_ext_set_dcdc3();

	return 0;
}
#endif

int sunxi_get_crypte_type(void)
{
	int version = -1;

	version = sunxi_efuse_get_soc_ver();
	pr_debug("crypte_type %d \n", version);
	/*A and B version use software*/
	if (version < 0x2) {
		return SUNXI_CRYPT_SOFTWARE;
	}

	return SUNXI_CRYPT_HW;
}
