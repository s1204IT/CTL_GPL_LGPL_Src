/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef HDR10P_H_
#define HDR10P_H_

#include <uapi/video/sunxi_display2.h>
#include "dw_dev.h"

int dw_hdr10p_configure(dw_hdmi_dev_t *dev, void *config,
	dw_video_param_t *video, dw_product_param_t *prod,
	struct disp_device_dynamic_config *scfg);

#endif	/* HDR10P_H_ */
