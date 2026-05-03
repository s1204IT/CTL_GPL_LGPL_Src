// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 * Copyright (C) 2011 Richard Zhao, Linaro <richard.zhao@linaro.org>
 * Copyright (C) 2011-2012 Mike Turquette, Linaro Ltd <mturquette@linaro.org>
 *
 * Adjustable divider clock implementation
 */

#include <div64.h>
#include "clk_periph.h"

/*
 * DOC: basic adjustable divider clock that cannot gate
 *
 * Traits of this clock:
 * prepare - clk_prepare only ensures that parents are prepared
 * enable - clk_enable only ensures that parents are enabled
 * rate - rate is adjustable.  clk->rate = ceiling(parent->rate / divisor)
 * parent - fixed parent.  No clk_set_parent support
 */
#define clk_div_mask(width)	((1 << (width)) - 1)
static bool _is_best_div(unsigned long rate, unsigned long now,
			 unsigned long best)
{
	return now <= rate && now > best;
}

static int clk_divider_bestdiv(struct clk_hw *hw, struct clk_hw *parent,
			       unsigned long rate,
			       unsigned long *best_parent_rate, u8 width)
{
	int i, bestdiv = 0;
	unsigned long parent_rate, best = 0, now, maxdiv;
	unsigned long parent_rate_saved = *best_parent_rate;
	struct sunxi_clk_periph *periph = to_clk_periph(hw);

	if (!rate)
		rate = 1;

	maxdiv = clk_div_mask(width) + 1;

	if (!(periph->flags & CLK_SET_RATE_PARENT)) {
		parent_rate = *best_parent_rate;
		bestdiv = DIV_ROUND_UP_ULL((u64)parent_rate, rate);
		bestdiv = bestdiv == 0 ? 1 : bestdiv;
		bestdiv = bestdiv > maxdiv ? maxdiv : bestdiv;
		return bestdiv;
	}

	/*
	 * The maximum divider we can use without overflowing
	 * unsigned long in rate * i below
	 */
	maxdiv = min(ULONG_MAX / rate, maxdiv);

	for (i = 1; i <= maxdiv; i++) {
		if (rate * i == parent_rate_saved) {
			/*
			 * It's the most ideal case if the requested rate can be
			 * divided from parent clock without needing to change
			 * parent rate, so return the divider immediately.
			 */
			*best_parent_rate = parent_rate_saved;
			return i;
		}
		parent_rate = clk_round_rate(parent->clk, rate * i);
		now = DIV_ROUND_UP_ULL((u64)parent_rate, i);
		if (_is_best_div(rate, now, best)) {
			bestdiv = i;
			best = now;
			*best_parent_rate = parent_rate;
		}
	}

	if (!bestdiv) {
		bestdiv = clk_div_mask(width) + 1;
		*best_parent_rate = clk_round_rate(parent->clk, 1);
	}

	return bestdiv;
}

static int divider_determine_rate(struct clk_hw *hw, struct clk_rate_request *req,
			   u8 width)
{
	int div;

	div = clk_divider_bestdiv(hw, req->best_parent_hw, req->rate,
				  &req->best_parent_rate, width);

	req->rate = DIV_ROUND_UP_ULL((u64)req->best_parent_rate, div);

	return 0;
}

int clk_divider_determine_rate(struct clk_hw *hw,
				      struct clk_rate_request *req)
{
	struct sunxi_clk_periph *periph = to_clk_periph(hw);
	struct sunxi_clk_periph_div *divider = &periph->divider;

	return divider_determine_rate(hw, req, divider->mwidth);
}
