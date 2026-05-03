/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_GAMMA_H__
#define __DE_GAMMA_H__

#include "de_feat.h"
#include "linux/types.h"

#if defined(SUPPORT_GAMMA)
int de_gamma_init(unsigned int sel, uintptr_t reg_base);
int de_gamma_exit(unsigned int sel);
int de_gamma_update_regs(unsigned int sel);
int de_gamma_enable(unsigned int sel, unsigned int en);
int de_gamma_set_table(u32 sel, u32 en, bool is_gamma_tbl_10bit, u32 *gamma_tbl);
#else
int __attribute__((weak)) de_gamma_init(unsigned int sel, uintptr_t reg_base) { return -1; }
int __attribute__((weak)) de_gamma_exit(unsigned int sel) { return -1; }
int __attribute__((weak)) de_gamma_update_regs(unsigned int sel) { return -1; }
int __attribute__((weak)) de_gamma_enable(unsigned int sel, unsigned int en) { return -1; }
int __attribute__((weak)) de_gamma_set_table(u32 sel, u32 en, bool is_gamma_tbl_10bit, u32 *gamma_tbl) { return -1; }
#endif /* SUPPORT_GAMMA */

#endif /* __DE_GAMMA_H__ */
