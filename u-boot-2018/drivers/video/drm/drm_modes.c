// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright © 1997-2003 by The XFree86 Project, Inc.
 * Copyright © 2007 Dave Airlie
 * Copyright © 2007-2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 * Copyright 2005-2006 Luc Verhaegen
 * Copyright (c) 2001, Andy Ritger  aritger@nvidia.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#include <div64.h>
#include <common.h>
#include <drm/drm_modes.h>
#include <linux/compat.h>
#include <malloc.h>
#include <linux/math64.h>

/**
 * drm_mode_create - create a new display mode
 *
 * Create a new, cleared drm_display_mode.
 *
 * Returns:
 * Pointer to new mode on success, NULL on error.
 */
struct drm_display_mode *drm_mode_create(void)
{
	struct drm_display_mode *nmode;

	nmode = malloc(sizeof(struct drm_display_mode));
	memset(nmode, 0, sizeof(struct drm_display_mode));
	if (!nmode)
		return NULL;

	return nmode;
}

/**
 * drm_mode_copy - copy the mode
 * @dst: mode to overwrite
 * @src: mode to copy
 *
 * Copy an existing mode into another mode, preserving the object id and
 * list head of the destination mode.
 */
void drm_mode_copy(struct drm_display_mode *dst, const struct drm_display_mode *src)
{
	*dst = *src;
}

/**
 * drm_mode_duplicate - allocate and duplicate an existing mode
 * @mode: mode to duplicate
 *
 * Just allocate a new mode, copy the existing mode into it, and return
 * a pointer to it.  Used to create new instances of established modes.
 *
 * Returns:
 * Pointer to duplicated mode on success, NULL on error.
 */
struct drm_display_mode *drm_mode_duplicate(const struct drm_display_mode *mode)
{
	struct drm_display_mode *nmode;

	nmode = drm_mode_create();
	if (!nmode)
		return NULL;

	drm_mode_copy(nmode, mode);

	return nmode;
}

/**
 * drm_mode_destroy - remove a mode
 * @mode: mode to remove
 */
void drm_mode_destroy(struct drm_display_mode *mode)
{
	if (!mode)
		return;

	kfree(mode);
}

static bool drm_mode_match_timings(const struct drm_display_mode *mode1,
				   const struct drm_display_mode *mode2)
{
	return mode1->hdisplay == mode2->hdisplay &&
	       mode1->hsync_start == mode2->hsync_start &&
	       mode1->hsync_end == mode2->hsync_end &&
	       mode1->htotal == mode2->htotal &&
	       mode1->hskew == mode2->hskew &&
	       mode1->vdisplay == mode2->vdisplay &&
	       mode1->vsync_start == mode2->vsync_start &&
	       mode1->vsync_end == mode2->vsync_end &&
	       mode1->vtotal == mode2->vtotal &&
	       mode1->vscan == mode2->vscan;
}

static bool drm_mode_match_clock(const struct drm_display_mode *mode1,
				 const struct drm_display_mode *mode2)
{
	/*
	 * do clock check convert to PICOS
	 * so fb modes get matched the same
	 */
	if (mode1->clock && mode2->clock)
		return KHZ2PICOS(mode1->clock) == KHZ2PICOS(mode2->clock);
	else
		return mode1->clock == mode2->clock;
}

static bool drm_mode_match_flags(const struct drm_display_mode *mode1,
				 const struct drm_display_mode *mode2)
{
	return (mode1->flags & ~DRM_MODE_FLAG_3D_MASK) ==
	       (mode2->flags & ~DRM_MODE_FLAG_3D_MASK);
}

static bool drm_mode_match_3d_flags(const struct drm_display_mode *mode1,
				    const struct drm_display_mode *mode2)
{
	return (mode1->flags & DRM_MODE_FLAG_3D_MASK) ==
	       (mode2->flags & DRM_MODE_FLAG_3D_MASK);
}

static bool drm_mode_match_aspect_ratio(const struct drm_display_mode *mode1,
					const struct drm_display_mode *mode2)
{
	return mode1->picture_aspect_ratio == mode2->picture_aspect_ratio;
}

/**
 * drm_mode_match - test modes for (partial) equality
 * @mode1: first mode
 * @mode2: second mode
 * @match_flags: which parts need to match (DRM_MODE_MATCH_*)
 *
 * Check to see if @mode1 and @mode2 are equivalent.
 *
 * Returns:
 * True if the modes are (partially) equal, false otherwise.
 */
bool drm_mode_match(const struct drm_display_mode *mode1,
		    const struct drm_display_mode *mode2,
		    unsigned int match_flags)
{
	if (!mode1 && !mode2)
		return true;

	if (!mode1 || !mode2)
		return false;

	if (match_flags & DRM_MODE_MATCH_TIMINGS &&
	    !drm_mode_match_timings(mode1, mode2))
		return false;

	if (match_flags & DRM_MODE_MATCH_CLOCK &&
	    !drm_mode_match_clock(mode1, mode2))
		return false;

	if (match_flags & DRM_MODE_MATCH_FLAGS &&
	    !drm_mode_match_flags(mode1, mode2))
		return false;

	if (match_flags & DRM_MODE_MATCH_3D_FLAGS &&
	    !drm_mode_match_3d_flags(mode1, mode2))
		return false;

	if (match_flags & DRM_MODE_MATCH_ASPECT_RATIO &&
	    !drm_mode_match_aspect_ratio(mode1, mode2))
		return false;

	return true;
}

/**
 * drm_mode_equal - test modes for equality
 * @mode1: first mode
 * @mode2: second mode
 *
 * Check to see if @mode1 and @mode2 are equivalent.
 *
 * Returns:
 * True if the modes are equal, false otherwise.
 */
bool drm_mode_equal(const struct drm_display_mode *mode1,
		    const struct drm_display_mode *mode2)
{
	return drm_mode_match(mode1, mode2,
			      DRM_MODE_MATCH_TIMINGS |
			      DRM_MODE_MATCH_CLOCK |
			      DRM_MODE_MATCH_FLAGS |
			      DRM_MODE_MATCH_3D_FLAGS |
			      DRM_MODE_MATCH_ASPECT_RATIO);
}

/**
 * drm_display_mode_from_videomode - fill in @dmode using @vm,
 * @vm: videomode structure to use as source
 * @dmode: drm_display_mode structure to use as destination
 *
 * Fills out @dmode using the display mode specified in @vm.
 */
void drm_display_mode_from_videomode(const struct videomode *vm,
				     struct drm_display_mode *dmode)
{
	dmode->hdisplay = vm->hactive;
	dmode->hsync_start = dmode->hdisplay + vm->hfront_porch;
	dmode->hsync_end = dmode->hsync_start + vm->hsync_len;
	dmode->htotal = dmode->hsync_end + vm->hback_porch;

	dmode->vdisplay = vm->vactive;
	dmode->vsync_start = dmode->vdisplay + vm->vfront_porch;
	dmode->vsync_end = dmode->vsync_start + vm->vsync_len;
	dmode->vtotal = dmode->vsync_end + vm->vback_porch;

	dmode->clock = vm->pixelclock / 1000;

	dmode->flags = 0;
	if (vm->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		dmode->flags |= DRM_MODE_FLAG_PHSYNC;
	else if (vm->flags & DISPLAY_FLAGS_HSYNC_LOW)
		dmode->flags |= DRM_MODE_FLAG_NHSYNC;
	if (vm->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		dmode->flags |= DRM_MODE_FLAG_PVSYNC;
	else if (vm->flags & DISPLAY_FLAGS_VSYNC_LOW)
		dmode->flags |= DRM_MODE_FLAG_NVSYNC;
	if (vm->flags & DISPLAY_FLAGS_INTERLACED)
		dmode->flags |= DRM_MODE_FLAG_INTERLACE;
	if (vm->flags & DISPLAY_FLAGS_DOUBLESCAN)
		dmode->flags |= DRM_MODE_FLAG_DBLSCAN;
	if (vm->flags & DISPLAY_FLAGS_DOUBLECLK)
		dmode->flags |= DRM_MODE_FLAG_DBLCLK;
}

/**
 * drm_display_mode_to_videomode - fill in @vm using @dmode,
 * @dmode: drm_display_mode structure to use as source
 * @vm: videomode structure to use as destination
 *
 * Fills out @vm using the display mode specified in @dmode.
 */
void drm_display_mode_to_videomode(const struct drm_display_mode *dmode,
				   struct videomode *vm)
{
	vm->hactive = dmode->hdisplay;
	vm->hfront_porch = dmode->hsync_start - dmode->hdisplay;
	vm->hsync_len = dmode->hsync_end - dmode->hsync_start;
	vm->hback_porch = dmode->htotal - dmode->hsync_end;

	vm->vactive = dmode->vdisplay;
	vm->vfront_porch = dmode->vsync_start - dmode->vdisplay;
	vm->vsync_len = dmode->vsync_end - dmode->vsync_start;
	vm->vback_porch = dmode->vtotal - dmode->vsync_end;

	vm->pixelclock = dmode->clock * 1000;

	vm->flags = 0;
	if (dmode->flags & DRM_MODE_FLAG_PHSYNC)
		vm->flags |= DISPLAY_FLAGS_HSYNC_HIGH;
	else if (dmode->flags & DRM_MODE_FLAG_NHSYNC)
		vm->flags |= DISPLAY_FLAGS_HSYNC_LOW;
	if (dmode->flags & DRM_MODE_FLAG_PVSYNC)
		vm->flags |= DISPLAY_FLAGS_VSYNC_HIGH;
	else if (dmode->flags & DRM_MODE_FLAG_NVSYNC)
		vm->flags |= DISPLAY_FLAGS_VSYNC_LOW;
	if (dmode->flags & DRM_MODE_FLAG_INTERLACE)
		vm->flags |= DISPLAY_FLAGS_INTERLACED;
	if (dmode->flags & DRM_MODE_FLAG_DBLSCAN)
		vm->flags |= DISPLAY_FLAGS_DOUBLESCAN;
	if (dmode->flags & DRM_MODE_FLAG_DBLCLK)
		vm->flags |= DISPLAY_FLAGS_DOUBLECLK;
}


/**
 * drm_mode_vrefresh - get the vrefresh of a mode
 * @mode: mode
 *
 * Returns:
 * @modes's vrefresh rate in Hz, rounded to the nearest integer. Calculates the
 * value first if it is not yet set.
 */
int drm_mode_vrefresh(const struct drm_display_mode *mode)
{
	unsigned int num, den;

	if (mode->htotal == 0 || mode->vtotal == 0)
		return 0;

	num = mode->clock;
	den = mode->htotal * mode->vtotal;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		num *= 2;
	if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		den *= 2;
	if (mode->vscan > 1)
		den *= mode->vscan;

	return DIV_ROUND_CLOSEST(mul_u32_u32(num, 1000), den);
}


/**
 * drm_mode_set_name - set the name on a mode
 * @mode: name will be set in this mode
 *
 * Set the name of @mode to a standard format which is <hdisplay>x<vdisplay>
 * with an optional 'i' suffix for interlaced modes.
 */
void drm_mode_set_name(struct drm_display_mode *mode)
{
	bool interlaced = !!(mode->flags & DRM_MODE_FLAG_INTERLACE);

	snprintf(mode->name, DRM_DISPLAY_MODE_LEN, "%dx%d%s",
		 mode->hdisplay, mode->vdisplay,
		 interlaced ? "i" : "");
}


/**
 * drm_gtf_mode_complex - create the modeline based on the full GTF algorithm
 * @hdisplay: hdisplay size
 * @vdisplay: vdisplay size
 * @vrefresh: vrefresh rate.
 * @interlaced: whether to compute an interlaced mode
 * @margins: desired margin (borders) size
 * @GTF_M: extended GTF formula parameters
 * @GTF_2C: extended GTF formula parameters
 * @GTF_K: extended GTF formula parameters
 * @GTF_2J: extended GTF formula parameters
 *
 * GTF feature blocks specify C and J in multiples of 0.5, so we pass them
 * in here multiplied by two.  For a C of 40, pass in 80.
 *
 * Returns:
 * The modeline based on the full GTF algorithm stored in a drm_display_mode object.
 * The display mode object is allocated with drm_mode_create(). Returns NULL
 * when no mode could be allocated.
 */
struct drm_display_mode *
drm_gtf_mode_complex(int hdisplay, int vdisplay,
		     int vrefresh, bool interlaced, int margins,
		     int GTF_M, int GTF_2C, int GTF_K, int GTF_2J)
{	/* 1) top/bottom margin size (% of height) - default: 1.8, */
#define	GTF_MARGIN_PERCENTAGE		18
	/* 2) character cell horizontal granularity (pixels) - default 8 */
#define	GTF_CELL_GRAN			8
	/* 3) Minimum vertical porch (lines) - default 3 */
#define	GTF_MIN_V_PORCH			1
	/* width of vsync in lines */
#define V_SYNC_RQD			3
	/* width of hsync as % of total line */
#define H_SYNC_PERCENT			8
	/* min time of vsync + back porch (microsec) */
#define MIN_VSYNC_PLUS_BP		550
	/* C' and M' are part of the Blanking Duty Cycle computation */
#define GTF_C_PRIME	((((GTF_2C - GTF_2J) * GTF_K / 256) + GTF_2J) / 2)
#define GTF_M_PRIME	(GTF_K * GTF_M / 256)
	struct drm_display_mode *drm_mode;
	unsigned int hdisplay_rnd, vdisplay_rnd, vfieldrate_rqd;
	int top_margin, bottom_margin;
	int interlace;
	unsigned int hfreq_est;
	int vsync_plus_bp, __maybe_unused vback_porch;
	unsigned int vtotal_lines, __maybe_unused vfieldrate_est;
	unsigned int __maybe_unused hperiod;
	unsigned int vfield_rate, __maybe_unused vframe_rate;
	int left_margin, right_margin;
	unsigned int total_active_pixels, ideal_duty_cycle;
	unsigned int hblank, total_pixels, pixel_freq;
	int hsync, hfront_porch, vodd_front_porch_lines;
	unsigned int tmp1, tmp2;

	if (!hdisplay || !vdisplay)
		return NULL;

	drm_mode = drm_mode_create();
	if (!drm_mode)
		return NULL;

	/* 1. In order to give correct results, the number of horizontal
	 * pixels requested is first processed to ensure that it is divisible
	 * by the character size, by rounding it to the nearest character
	 * cell boundary:
	 */
	hdisplay_rnd = (hdisplay + GTF_CELL_GRAN / 2) / GTF_CELL_GRAN;
	hdisplay_rnd = hdisplay_rnd * GTF_CELL_GRAN;

	/* 2. If interlace is requested, the number of vertical lines assumed
	 * by the calculation must be halved, as the computation calculates
	 * the number of vertical lines per field.
	 */
	if (interlaced)
		vdisplay_rnd = vdisplay / 2;
	else
		vdisplay_rnd = vdisplay;

	/* 3. Find the frame rate required: */
	if (interlaced)
		vfieldrate_rqd = vrefresh * 2;
	else
		vfieldrate_rqd = vrefresh;

	/* 4. Find number of lines in Top margin: */
	top_margin = 0;
	if (margins)
		top_margin = (vdisplay_rnd * GTF_MARGIN_PERCENTAGE + 500) /
				1000;
	/* 5. Find number of lines in bottom margin: */
	bottom_margin = top_margin;

	/* 6. If interlace is required, then set variable interlace: */
	if (interlaced)
		interlace = 1;
	else
		interlace = 0;

	/* 7. Estimate the Horizontal frequency */
	{
		tmp1 = (1000000  - MIN_VSYNC_PLUS_BP * vfieldrate_rqd) / 500;
		tmp2 = (vdisplay_rnd + 2 * top_margin + GTF_MIN_V_PORCH) *
				2 + interlace;
		hfreq_est = (tmp2 * 1000 * vfieldrate_rqd) / tmp1;
	}

	/* 8. Find the number of lines in V sync + back porch */
	/* [V SYNC+BP] = RINT(([MIN VSYNC+BP] * hfreq_est / 1000000)) */
	vsync_plus_bp = MIN_VSYNC_PLUS_BP * hfreq_est / 1000;
	vsync_plus_bp = (vsync_plus_bp + 500) / 1000;
	/*  9. Find the number of lines in V back porch alone: */
	vback_porch = vsync_plus_bp - V_SYNC_RQD;
	/*  10. Find the total number of lines in Vertical field period: */
	vtotal_lines = vdisplay_rnd + top_margin + bottom_margin +
			vsync_plus_bp + GTF_MIN_V_PORCH;
	/*  11. Estimate the Vertical field frequency: */
	vfieldrate_est = hfreq_est / vtotal_lines;
	/*  12. Find the actual horizontal period: */
	hperiod = 1000000 / (vfieldrate_rqd * vtotal_lines);

	/*  13. Find the actual Vertical field frequency: */
	vfield_rate = hfreq_est / vtotal_lines;
	/*  14. Find the Vertical frame frequency: */
	if (interlaced)
		vframe_rate = vfield_rate / 2;
	else
		vframe_rate = vfield_rate;
	/*  15. Find number of pixels in left margin: */
	if (margins)
		left_margin = (hdisplay_rnd * GTF_MARGIN_PERCENTAGE + 500) /
				1000;
	else
		left_margin = 0;

	/* 16.Find number of pixels in right margin: */
	right_margin = left_margin;
	/* 17.Find total number of active pixels in image and left and right */
	total_active_pixels = hdisplay_rnd + left_margin + right_margin;
	/* 18.Find the ideal blanking duty cycle from blanking duty cycle */
	ideal_duty_cycle = GTF_C_PRIME * 1000 -
				(GTF_M_PRIME * 1000000 / hfreq_est);
	/* 19.Find the number of pixels in the blanking time to the nearest
	 * double character cell: */
	hblank = total_active_pixels * ideal_duty_cycle /
			(100000 - ideal_duty_cycle);
	hblank = (hblank + GTF_CELL_GRAN) / (2 * GTF_CELL_GRAN);
	hblank = hblank * 2 * GTF_CELL_GRAN;
	/* 20.Find total number of pixels: */
	total_pixels = total_active_pixels + hblank;
	/* 21.Find pixel clock frequency: */
	pixel_freq = total_pixels * hfreq_est / 1000;
	/* Stage 1 computations are now complete; I should really pass
	 * the results to another function and do the Stage 2 computations,
	 * but I only need a few more values so I'll just append the
	 * computations here for now */
	/* 17. Find the number of pixels in the horizontal sync period: */
	hsync = H_SYNC_PERCENT * total_pixels / 100;
	hsync = (hsync + GTF_CELL_GRAN / 2) / GTF_CELL_GRAN;
	hsync = hsync * GTF_CELL_GRAN;
	/* 18. Find the number of pixels in horizontal front porch period */
	hfront_porch = hblank / 2 - hsync;
	/*  36. Find the number of lines in the odd front porch period: */
	vodd_front_porch_lines = GTF_MIN_V_PORCH ;

	/* finally, pack the results in the mode struct */
	drm_mode->hdisplay = hdisplay_rnd;
	drm_mode->hsync_start = hdisplay_rnd + hfront_porch;
	drm_mode->hsync_end = drm_mode->hsync_start + hsync;
	drm_mode->htotal = total_pixels;
	drm_mode->vdisplay = vdisplay_rnd;
	drm_mode->vsync_start = vdisplay_rnd + vodd_front_porch_lines;
	drm_mode->vsync_end = drm_mode->vsync_start + V_SYNC_RQD;
	drm_mode->vtotal = vtotal_lines;

	drm_mode->clock = pixel_freq;

	if (interlaced) {
		drm_mode->vtotal *= 2;
		drm_mode->flags |= DRM_MODE_FLAG_INTERLACE;
	}

	drm_mode_set_name(drm_mode);
	if (GTF_M == 600 && GTF_2C == 80 && GTF_K == 128 && GTF_2J == 40)
		drm_mode->flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC;
	else
		drm_mode->flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC;

	return drm_mode;
}

/**
 * drm_gtf_mode - create the modeline based on the GTF algorithm
 * @hdisplay: hdisplay size
 * @vdisplay: vdisplay size
 * @vrefresh: vrefresh rate.
 * @interlaced: whether to compute an interlaced mode
 * @margins: desired margin (borders) size
 *
 * return the modeline based on GTF algorithm
 *
 * This function is to create the modeline based on the GTF algorithm.
 * Generalized Timing Formula is derived from:
 *
 *	GTF Spreadsheet by Andy Morrish (1/5/97)
 *	available at https://www.vesa.org
 *
 * And it is copied from the file of xserver/hw/xfree86/modes/xf86gtf.c.
 * What I have done is to translate it by using integer calculation.
 * I also refer to the function of fb_get_mode in the file of
 * drivers/video/fbmon.c
 *
 * Standard GTF parameters::
 *
 *     M = 600
 *     C = 40
 *     K = 128
 *     J = 20
 *
 * Returns:
 * The modeline based on the GTF algorithm stored in a drm_display_mode object.
 * The display mode object is allocated with drm_mode_create(). Returns NULL
 * when no mode could be allocated.
 */
struct drm_display_mode *
drm_gtf_mode(int hdisplay, int vdisplay, int vrefresh,
	     bool interlaced, int margins)
{
	return drm_gtf_mode_complex(hdisplay, vdisplay, vrefresh,
				    interlaced, margins,
				    600, 40 * 2, 128, 20 * 2);
}

/**
 * drm_cvt_mode -create a modeline based on the CVT algorithm
 * @hdisplay: hdisplay size
 * @vdisplay: vdisplay size
 * @vrefresh: vrefresh rate
 * @reduced: whether to use reduced blanking
 * @interlaced: whether to compute an interlaced mode
 * @margins: whether to add margins (borders)
 *
 * This function is called to generate the modeline based on CVT algorithm
 * according to the hdisplay, vdisplay, vrefresh.
 * It is based from the VESA(TM) Coordinated Video Timing Generator by
 * Graham Loveridge April 9, 2003 available at
 * http://www.elo.utfsm.cl/~elo212/docs/CVTd6r1.xls
 *
 * And it is copied from xf86CVTmode in xserver/hw/xfree86/modes/xf86cvt.c.
 * What I have done is to translate it by using integer calculation.
 *
 * Returns:
 * The modeline based on the CVT algorithm stored in a drm_display_mode object.
 * The display mode object is allocated with drm_mode_create(). Returns NULL
 * when no mode could be allocated.
 */
struct drm_display_mode *drm_cvt_mode(int hdisplay,
				      int vdisplay, int vrefresh,
				      bool reduced, bool interlaced, bool margins)
{
#define HV_FACTOR			1000
	/* 1) top/bottom margin size (% of height) - default: 1.8, */
#define	CVT_MARGIN_PERCENTAGE		18
	/* 2) character cell horizontal granularity (pixels) - default 8 */
#define	CVT_H_GRANULARITY		8
	/* 3) Minimum vertical porch (lines) - default 3 */
#define	CVT_MIN_V_PORCH			3
	/* 4) Minimum number of vertical back porch lines - default 6 */
#define	CVT_MIN_V_BPORCH		6
	/* Pixel Clock step (kHz) */
#define CVT_CLOCK_STEP			250
	struct drm_display_mode *drm_mode;
	unsigned int vfieldrate, hperiod;
	int hdisplay_rnd, hmargin, vdisplay_rnd, vmargin, vsync;
	int interlace;
	u64 tmp;

	if (!hdisplay || !vdisplay)
		return NULL;

	/* allocate the drm_display_mode structure. If failure, we will
	 * return directly
	 */
	drm_mode = drm_mode_create();
	if (!drm_mode)
		return NULL;

	/* the CVT default refresh rate is 60Hz */
	if (!vrefresh)
		vrefresh = 60;

	/* the required field fresh rate */
	if (interlaced)
		vfieldrate = vrefresh * 2;
	else
		vfieldrate = vrefresh;

	/* horizontal pixels */
	hdisplay_rnd = hdisplay - (hdisplay % CVT_H_GRANULARITY);

	/* determine the left&right borders */
	hmargin = 0;
	if (margins) {
		hmargin = hdisplay_rnd * CVT_MARGIN_PERCENTAGE / 1000;
		hmargin -= hmargin % CVT_H_GRANULARITY;
	}
	/* find the total active pixels */
	drm_mode->hdisplay = hdisplay_rnd + 2 * hmargin;

	/* find the number of lines per field */
	if (interlaced)
		vdisplay_rnd = vdisplay / 2;
	else
		vdisplay_rnd = vdisplay;

	/* find the top & bottom borders */
	vmargin = 0;
	if (margins)
		vmargin = vdisplay_rnd * CVT_MARGIN_PERCENTAGE / 1000;

	drm_mode->vdisplay = vdisplay + 2 * vmargin;

	/* Interlaced */
	if (interlaced)
		interlace = 1;
	else
		interlace = 0;

	/* Determine VSync Width from aspect ratio */
	if (!(vdisplay % 3) && ((vdisplay * 4 / 3) == hdisplay))
		vsync = 4;
	else if (!(vdisplay % 9) && ((vdisplay * 16 / 9) == hdisplay))
		vsync = 5;
	else if (!(vdisplay % 10) && ((vdisplay * 16 / 10) == hdisplay))
		vsync = 6;
	else if (!(vdisplay % 4) && ((vdisplay * 5 / 4) == hdisplay))
		vsync = 7;
	else if (!(vdisplay % 9) && ((vdisplay * 15 / 9) == hdisplay))
		vsync = 7;
	else /* custom */
		vsync = 10;

	if (!reduced) {
		/* simplify the GTF calculation */
		/* 4) Minimum time of vertical sync + back porch interval (µs)
		 * default 550.0
		 */
		int tmp1, tmp2;
#define CVT_MIN_VSYNC_BP	550
		/* 3) Nominal HSync width (% of line period) - default 8 */
#define CVT_HSYNC_PERCENTAGE	8
		unsigned int hblank_percentage;
		int vsyncandback_porch, __maybe_unused vback_porch, hblank;

		/* estimated the horizontal period */
		tmp1 = HV_FACTOR * 1000000  -
				CVT_MIN_VSYNC_BP * HV_FACTOR * vfieldrate;
		tmp2 = (vdisplay_rnd + 2 * vmargin + CVT_MIN_V_PORCH) * 2 +
				interlace;
		hperiod = tmp1 * 2 / (tmp2 * vfieldrate);

		tmp1 = CVT_MIN_VSYNC_BP * HV_FACTOR / hperiod + 1;
		/* 9. Find number of lines in sync + backporch */
		if (tmp1 < (vsync + CVT_MIN_V_PORCH))
			vsyncandback_porch = vsync + CVT_MIN_V_PORCH;
		else
			vsyncandback_porch = tmp1;
		/* 10. Find number of lines in back porch */
		vback_porch = vsyncandback_porch - vsync;
		drm_mode->vtotal = vdisplay_rnd + 2 * vmargin +
				vsyncandback_porch + CVT_MIN_V_PORCH;
		/* 5) Definition of Horizontal blanking time limitation */
		/* Gradient (%/kHz) - default 600 */
#define CVT_M_FACTOR	600
		/* Offset (%) - default 40 */
#define CVT_C_FACTOR	40
		/* Blanking time scaling factor - default 128 */
#define CVT_K_FACTOR	128
		/* Scaling factor weighting - default 20 */
#define CVT_J_FACTOR	20
#define CVT_M_PRIME	(CVT_M_FACTOR * CVT_K_FACTOR / 256)
#define CVT_C_PRIME	((CVT_C_FACTOR - CVT_J_FACTOR) * CVT_K_FACTOR / 256 + \
			 CVT_J_FACTOR)
		/* 12. Find ideal blanking duty cycle from formula */
		hblank_percentage = CVT_C_PRIME * HV_FACTOR - CVT_M_PRIME *
					hperiod / 1000;
		/* 13. Blanking time */
		if (hblank_percentage < 20 * HV_FACTOR)
			hblank_percentage = 20 * HV_FACTOR;
		hblank = drm_mode->hdisplay * hblank_percentage /
			 (100 * HV_FACTOR - hblank_percentage);
		hblank -= hblank % (2 * CVT_H_GRANULARITY);
		/* 14. find the total pixels per line */
		drm_mode->htotal = drm_mode->hdisplay + hblank;
		drm_mode->hsync_end = drm_mode->hdisplay + hblank / 2;
		drm_mode->hsync_start = drm_mode->hsync_end -
			(drm_mode->htotal * CVT_HSYNC_PERCENTAGE) / 100;
		drm_mode->hsync_start += CVT_H_GRANULARITY -
			drm_mode->hsync_start % CVT_H_GRANULARITY;
		/* fill the Vsync values */
		drm_mode->vsync_start = drm_mode->vdisplay + CVT_MIN_V_PORCH;
		drm_mode->vsync_end = drm_mode->vsync_start + vsync;
	} else {
		/* Reduced blanking */
		/* Minimum vertical blanking interval time (µs)- default 460 */
#define CVT_RB_MIN_VBLANK	460
		/* Fixed number of clocks for horizontal sync */
#define CVT_RB_H_SYNC		32
		/* Fixed number of clocks for horizontal blanking */
#define CVT_RB_H_BLANK		160
		/* Fixed number of lines for vertical front porch - default 3*/
#define CVT_RB_VFPORCH		3
		int vbilines;
		int tmp1, tmp2;
		/* 8. Estimate Horizontal period. */
		tmp1 = HV_FACTOR * 1000000 -
			CVT_RB_MIN_VBLANK * HV_FACTOR * vfieldrate;
		tmp2 = vdisplay_rnd + 2 * vmargin;
		hperiod = tmp1 / (tmp2 * vfieldrate);
		/* 9. Find number of lines in vertical blanking */
		vbilines = CVT_RB_MIN_VBLANK * HV_FACTOR / hperiod + 1;
		/* 10. Check if vertical blanking is sufficient */
		if (vbilines < (CVT_RB_VFPORCH + vsync + CVT_MIN_V_BPORCH))
			vbilines = CVT_RB_VFPORCH + vsync + CVT_MIN_V_BPORCH;
		/* 11. Find total number of lines in vertical field */
		drm_mode->vtotal = vdisplay_rnd + 2 * vmargin + vbilines;
		/* 12. Find total number of pixels in a line */
		drm_mode->htotal = drm_mode->hdisplay + CVT_RB_H_BLANK;
		/* Fill in HSync values */
		drm_mode->hsync_end = drm_mode->hdisplay + CVT_RB_H_BLANK / 2;
		drm_mode->hsync_start = drm_mode->hsync_end - CVT_RB_H_SYNC;
		/* Fill in VSync values */
		drm_mode->vsync_start = drm_mode->vdisplay + CVT_RB_VFPORCH;
		drm_mode->vsync_end = drm_mode->vsync_start + vsync;
	}
	/* 15/13. Find pixel clock frequency (kHz for xf86) */
	tmp = drm_mode->htotal; /* perform intermediate calcs in u64 */
	tmp *= HV_FACTOR * 1000;
	do_div(tmp, hperiod);
	tmp -= drm_mode->clock % CVT_CLOCK_STEP;
	drm_mode->clock = tmp;
	/* 18/16. Find actual vertical frame frequency */
	/* ignore - just set the mode flag for interlaced */
	if (interlaced) {
		drm_mode->vtotal *= 2;
		drm_mode->flags |= DRM_MODE_FLAG_INTERLACE;
	}
	/* Fill the mode line name */
	drm_mode_set_name(drm_mode);
	if (reduced)
		drm_mode->flags |= (DRM_MODE_FLAG_PHSYNC |
					DRM_MODE_FLAG_NVSYNC);
	else
		drm_mode->flags |= (DRM_MODE_FLAG_PVSYNC |
					DRM_MODE_FLAG_NHSYNC);

	return drm_mode;
}
