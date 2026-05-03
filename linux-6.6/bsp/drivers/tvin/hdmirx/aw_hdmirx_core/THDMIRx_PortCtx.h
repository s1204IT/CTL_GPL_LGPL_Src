/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2024 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmirx driver.
 *
 * Copyright (C) 2024 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _PORT_CONTEXT_
#define _PORT_CONTEXT_
#include "THDMIRx_System.h"
#include "hardware/THDMIRx_TV_Driver.h"
#include "THDMIRx_DisplayModuleCtx.h"

// video
// match level table
static U8 const MatchLevelTable[] = {
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_VPOL_ | _MATCH_HWIDTH_ | _MATCH_VWIDTH_ | _MATCH_HFREQENCY_ |
		_MATCH_VFREQENCY_,                                                          /* Level 0 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_VPOL_ | _MATCH_VWIDTH_,   /* Level 1 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_VPOL_ | _MATCH_HWIDTH_,   /* Level 2 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_VPOL_,                    /* Level 3 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_VPOL_ | _MATCH_HWIDTH_ | _MATCH_VWIDTH_, /* Level 4 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_HWIDTH_ | _MATCH_VWIDTH_, /* Level 5 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_VPOL_ | _MATCH_VWIDTH_,                  /* Level 6 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_VPOL_ | _MATCH_HWIDTH_,                  /* Level 7 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_VWIDTH_,                  /* Level 8 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_ | _MATCH_HWIDTH_,                  /* Level 9 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_VPOL_,                                   /* Level 10 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HPOL_,                                   /* Level 11 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HWIDTH_ | _MATCH_VWIDTH_,                /* Level 12 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_VWIDTH_,                                 /* Level 13 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_ | _MATCH_HWIDTH_,                                 /* Level 14 */
	_MATCH_HFREQ_ | _MATCH_VFREQ_,                                                  /* Level 15 */
};

/****************audio************/
#define AUDIO_FIFO_ERR_THRESHOLD 10
#define AUDIO_PKT_ERR_THRESHOLD 10
#define AUDIO_PLL_CON1_CTL_VALUE 0x0D326667
#define PLL_HDMI_AUDIO_PDEC_MASK 0x0000FE00
#define PLL_HDMI_AUDIO_PDEC_START_BIT 9
#define HDMI_AUDIO_PLLFRAC_CTRL_MASK 0x0000FE00
#define HDMI_AUDIO_FRAC_CTL_M_MASK 0x00FF8000

#define SHIFT_ONE_BYTE 8

#define BASE2POW(__Expr__) ((U32)(0x01 << (__Expr__)))

typedef struct _ThdmiRxAudioContext {
	U32 CTS;
	U32 N;
	U32 fs;  // fs from channel status

	U32 audioPLLfs;  // fs from audio PLL
	U8 audioType;	 // audio type

	U32 fifoErrCnt;
	U32 audioPktErrCnt;
} ThdmiRxAudioContext;

typedef struct _ThdmiRxAudioPllDividerParams {
	U32 xDiv;
	U32 pDiv;
	U32 mDiv;
	U32 mInt;
	U32 mFrac;
} ThdmiRxAudioPllDividerParams;

struct HDMIRx_AudioContext {
	ThdmiRxAudioContext ctx;
	ThdmiRxAudioPllDividerParams AudioPllDividerParams;
	U32 sui32PrevN;
	U32 sui32PrevFs;
	U32 Audio_PLL_Base;
};

/****************video************/
// default lock_sync_num hsync/vsync
#define LOCK_SYNC_HSYNC_COUNT (0x02)
#define LOCK_SYNC_VSYNC_COUNT (0x02)

// default window param
#define DEFAULT_HS_TO_HDE 0x4E
#define DEFAULT_HACTIVE 0x2D0
#define DEFAULT_VS_TO_VDE 0x2A
#define DEFAULT_VACTIVE 0x1E0
#define DEFAULT_HTOTAL 0x35A
#define DEFAULT_VTOTAL 0x20D
#define DEFAULT_ROWBYTE 0
#define DEFAULT_HRES_PERCENTAGE 100
#define DEFAULT_VRES_PERCENTAGE 100

#define MAX_MODE_CNT 255
#define BEST_MATCH_LEVEL 0

#define HDMI_V_RANGE_FREQUENCY_ 500
#define HDMI_V_RANGE_FREQUENCY_ABOVE_30HZ_ 1000

// video timing invalid time out count
#define HDMIRX_DETECT_TIMING_TIMEOUT_COUNT (5)

// video timing  stable detect counter
#define HDMIRX_DETECT_VIDEO_DONE (-1)
#define HDMIRX_DETECT_VIDEO_START 0
#define HDMIRX_DETECT_VIDEO_CNT 5
#define HDMIRX_DETECT_TIMING_TICK (300)

#define HDMIRX_INVALID_SIGNAL_INDEX 0xFF

// signal detect counter
#define HDMIRX_DETECT_CNT_DONE (-1)
#define HDMIRX_DETECT_CNT_START 0
#define HDMIRX_DETECT_STABLE_CNT 5
#define HDMIRX_DETECT_SIGNAL_TICK (300)

// signal detect mask
#define HDMIRX_SIGNAL_NO_CHANGE 0x0
#define SIGNAL_DIFF_MASK 0x00000001
#define FRAME_RATE_DIFF_MASK 0x00000002
#define COLOR_FORMAT_DIFF_MASK 0x00000004
#define EDR_MODE_DIFF_MASK 0x00000008
#define HDMIRX_3DINFO_DIFF_MASK 0x00000010
#define HDR_MODE_DIFF_MASK 0x00000020
#define DOBLY_LOWLATENCY_DIFF_MASK 0x00000040
#define COLOR_MATRIX_DIFF_MASK 0x00000080
#define VRR_MODE_DIFF_MASK 0x00000100
#define HDR10_PLUS_MODE_DIFF_MASK 0x00000200

#define HDMIRX_PIXEL_FREQ_RANGE (500)  // K
#define HDMIRX_FIELD_FREQ_RANGE (20)

#define HDMIRX_HBACK_PORCH_RANGE (5)
#define HDMIRX_VBACK_PORCH_RANGE (10)

#define HDMIRX_HACTIVE_RANGE (10)
#define HDMIRX_VACTIVE_RANGE (10)

#define HDMIRX_HTOTAL_RANGE (10)
#define HDMIRX_VTOTAL_RANGE (10)

// color depth define
typedef enum _eHdmiRxColorDepth {
	COLOR_DEPTH_NONE = 0,
	COLOR_DEPTH_8BIT,
	COLOR_DEPTH_10BIT,
	COLOR_DEPTH_12BIT,
	COLOR_DEPTH_16BIT
} eHdmiRxColorDepth;

// hdmi output mode
typedef enum _eVideoMode {
	VIDEO_MODE_UNKNOWN = 0,
	VIDEO_MODE_HDMI = 1,
	VIDEO_MODE_DVI = 2,
} VideoMode_e;

typedef enum _eHdmiRxVideoState {
	HDMIRX_VIDEO_STATE_NONE = 0, /**< No source connected. */
	HDMIRX_VIDEO_STATE_MEASURE,  /**< Video parameters are measured. */
	HDMIRX_VIDEO_STATE_UNSTABLE, /**< The video input is currently unstable. */
	HDMIRX_VIDEO_STATE_STABLE,   /**< The video input is stable. */
} eHdmiRxVideoState;

// video timing parameters
typedef struct _ThdmRxTimingParam {
	bool bInterlaced; /**< Indicates if the source sends interlaced video. */
	U32 pixelFreq;    /**< Measured pixel frequency. */
	//float fieldFreq;  /**< Measured field refresh rate. */
	U32 fieldFreq; /* 1*1000 */

	U16 offsetFirstActiveLine;  /**< Measured vertical offset. */
	U16 numTotalFrameLines;     /**< Measured total number of lines per frame. */
	U16 numActiveLines;         /**< Measured active number of lines per frame. */
	U16 offsetFirstActivePixel; /**< Measured horizontal offset. */
	U16 numTotalPixelsPerLine;  /**< Measured total number of pixels per line. */
	U16 numActivePixelsPerLine; /**< Measured active number of pixesl per line. */

	U16 rawHTotal;
	U16 rawVTotal;

	U16 hsyncWidth;
	U16 vsyncWidth;

	U16 hdelay;
	U16 vdelay;
	U16 hblank;
	U16 vblank;

	eHdmiRxColorDepth eColorDepth;
	U16 bpixRep;

	bool bVsyncPolarity;
	bool bHsyncPolarity;
} ThdmRxTimingParam;

typedef struct _ThdmiRxVideoContext {
	// char cStableCnt;
	// char cVidStableCnt;
	VideoMode_e CurVideoMode;
	U32 dwLastChangeStatus;

	U32 dwCurSignal;
	U32 dwCurFrameRate;
	U16 wCurVidFormat;
	U8 ucCurSignalIndex;

	eHdmiRxEOTF CurHdrMode;
	U32 dwLastHdrEventTime;
	eHdmiRxColorMatrix wColorMatrix;

	bool bCurEdrMode;
	bool bNewEdrMode;
	bool bCurDbVisionLowLatencyMode;
	bool bNewDbVisionLowLatencyMode;
	// THDMI3DInfo tCur3DInfo;
	bool bVrrMode;
	bool bHDR10Plus;

	eHdmiRxVideoState eVideoState;
	ThdmRxTimingParam tTimingParam;
} ThdmiRxVideoContext;

typedef struct _ThdmiRxTmdsCtx {
	bool bFreqDetStart;
	U32 dwCntVal;
} ThdmiRxTmdsCtx;

typedef struct _ThdmiRxSCDCContext {
	U8 ucTMDSBitClockRatio;
	U8 ucScramblingEnable;
} ThdmiRxSCDCContext;

struct HDMIRx_VideoContext {
	//static Video_DriverCallBack_t Video_DriverCallBack;
	ThdmiRxVideoContext ctx;
	ThdmiRxTmdsCtx TmdsCtx;
	ThdmiRxSCDCContext SCDCContext;
	U32 DetectTimingValidTick;  //比对时序时判断的标志 TimingMonitorTask
	U32 DetectSignalValidTick;  //比对信号   signalmonitor
	U32 DetectTiming_TimeOutCount;
};


/*****StateMachineContext********/
// hdmirx states
typedef enum _eHdmiRxState {
	HDMIRX_STATE_NONE = 0,
	HDMIRX_STATE_INIT,

	HDMIRX_STATE_SLEEP,
	HDMIRX_STATE_IDLE,
	HDMIRX_STATE_ACTIVE,
	HDMIRX_STATE_RUNNING
} HdmiRxState_e;

struct HDMIRx_StateMachineContext {
	HdmiRxState_e HdmiRxState;
	bool bStateStart;
};


/*****StateMachineContext********/
struct HDMIRX_TaskContext {
	bool HDCP_TaskActive;
	bool StateMachine_TaskActive;
	bool Event_TaskActive;
};

typedef enum _tagPortPathState {
	// PortPathState_HDCP_NotAuthentic,
	// PortPathState_HDCP_Authentic,
	PortPathState_LinkDisconnect,
	PortPathState_LinkDisconnecting,
	PortPathState_LinkConnecting,
	PortPathState_LinkConnected,
	PortPathState_VideoOut,
	// PortPathState_No5V,
} PortPathState_e;

struct HDMIRx_PortContext {
	PortPathState_e PortPathState;
	struct HDMIRx_VideoContext videoCtx;
	struct HDMIRx_AudioContext audioCtx;
	struct HDMIRx_StateMachineContext StateMachineCtx;
	struct HDMIRx_PacketContext PacketCtx;
	struct HDMIRX_TaskContext TaskCtx;
	U8 PortID;/* 1 2*/
	LinkBaseID_e LinkID; /* 0 */
	U32 LinkBase;    //0x06840000
	U32 PhyBase;
	U32 HDCPBase;   //0x06840000
	U32 MVBase;
	bool isHDCPRunning;
	bool isActivePort;
	bool is5V;
	bool bAVMute;
	U32 AVMuteTick;
	U32 HDCP22_ReAuthTimeOutTick;
	U32 HDCP14_ReAuthTimeOutTick;

// patch flag for HW issue
#if HDMRIX_422_PATCH
		bool isPatchRun_422;
#endif

};

/*audio*/
void audioContextclear(U8 PortID);

bool AudioMonitor(void *pPortContex);

/*video*/
bool SignalMonitor(U8 PortID, U8 SignalIndex, U32 SignalID, void *pPortContex);

bool CheckTimingDiff(U8 PortID, ThdmRxTimingParam *pOldTiming, ThdmRxTimingParam *pNewTiming);

void HDMIRx_PortCtx_Contextclear(U8 PortID);

void HDMIRx_PortContextinit(U8 port_id, struct HDMIRx_PortContext *core);

#endif /* _PORT_CONTEXT_ */
