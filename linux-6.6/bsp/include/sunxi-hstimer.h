// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner alternative hstimer driver
 *
 * Copyright (C) 2023 allwinner Co., Ltd.
 */

#ifndef _SUNXI_HSTIMER_H
#define _SUNXI_HSTIMER_H

typedef void (*hstimer_callback)(void *param);

enum hstimer_mode {
	CONTINUOUS_MODE = 0,
	SINGLE_MODE,
	INVALID_MODE
};

/*
 * @ticks: A tick represents 5ns, which is converted
 *      by the specific time: 1us == 200ticks;
 *
 * @cb_hstimer: callback function;
 *
 * @param: Callback function parameters;
 *
 * @hsmode: hstimer mode, If it is 0, it will be timed repeatedly,
 *      if it is 1, it will only be timed once;
 *
 * @num: the hstimer number;
 */
struct hstimer_cap {
	u64			ticks;
	hstimer_callback	cb_hstimer;
	void			*param;
	enum hstimer_mode	hsmode;
	int			num;
};

struct hstimer_cap *sunxi_hstimer_init(u64 ustimes, enum hstimer_mode mode, hstimer_callback cb, void *cb_param);
void sunxi_hstimer_start(struct hstimer_cap *hstimer);
void sunxi_hstimer_exit(struct hstimer_cap *hstimer);

#endif /* _SUNXI_HSTIMER_H */
