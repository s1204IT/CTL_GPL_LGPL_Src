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
#ifndef __DISPLAY_DATA_TYPE_H__
#define __DISPLAY_DATA_TYPE_H__
#include <linux/types.h>


typedef unsigned long long  u64_t;
typedef long long           s64_t;
typedef unsigned int        u32_t;
typedef signed int          s32_t;
typedef unsigned short      u16_t;
typedef signed short        s16_t;
typedef unsigned char       u8_t;
typedef signed char         s8_t;
typedef char                char_t;
//typedef float               float_t;

typedef unsigned int        uint_t;
typedef signed int          sint_t;

/*
 * TODO: should be removed
 * */
#define U64                 u64_t
#define S64                 s64_t
#define U32                 u32_t
#define S32                 s32_t
#define U16                 u16_t
#define S16                 s16_t
#define U8                  u8_t
#define S8                  s8_t
#define CHAR                char_t
//typedef double DOUBLE;


/*
#if !defined(size_t)
typedef unsigned long       size_t;
#endif
*/

#ifndef NULL
	#ifdef __cplusplus
		#define NULL (0)
	#else
		#define NULL ((void *)0)
	#endif
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef CONST
	#define CONST const
#endif

#ifndef INLINE
#define INLINE inline
#endif


typedef enum _HalTColorSpace {
	kHalColorSpace_BT601,
	kHalColorSpace_BT709,
	kHalColorSpace_BT2020_NCLYCC,
	kHalColorSpace_BT2020_CLYCC,
	kHalColorSpace_xvYCC,
	kHalColorSpace_RGB,
	kHalColorSpace_Max,
} THalColorSpace;

typedef enum {
	kHalColorFormat_YUV420_888,
	kHalColorFormat_YUV420_1088,
	kHalColorFormat_YUV420_101010,
	kHalColorFormat_YUV420_121212,

	kHalColorFormat_YUV422_888,
	kHalColorFormat_YUV422_1088,
	kHalColorFormat_YUV422_101010,
	kHalColorFormat_YUV422_121212,

	kHalColorFormat_YUV444_888,
	kHalColorFormat_YUV444_101010,
	kHalColorFormat_YUV444_121212,

	kHalColorFormat_RGB_888,
	kHalColorFormat_RGB_101010,
	kHalColorFormat_RGB_121212,
	kHalColorFormat_RGB_565,

	kHalColorFormat_Max,
} THalColorFormat;

typedef enum _THalHdrScheme {
	kHalHdrScheme_Sdr,
	kHalHdrScheme_Hdr10,
	kHalHdrScheme_Hdr10Plus,
	kHalHdrScheme_Sl_Hdr1,
	kHalHdrScheme_Sl_Hdr2,
	kHalHdrScheme_Hlg,
	kHalHdrScheme_DbVision,
	kHalHdrScheme_Max,
} THalHdrScheme;

/*
 * for WCE
 * */
#define MAX_WINDOW_SIZE_H (1920 * 16)
#define MAX_WINDOW_SIZE_V (1080 * 16)

typedef struct _THalWin {
	s32_t h_start;
	u32_t h_size;
	s32_t v_start;
	u32_t v_size;
} THalWin;

typedef enum {
	kHalDisplayAspectRatio_Auto,          // AFD, PAR, AR, WSS
	kHalDisplayAspectRatio_KeepSourceAr,  // display aspect ratio remains the same as source
	kHalDisplayAspectRatio_Full,
	kHalDisplayAspectRatio_16_9,
	kHalDisplayAspectRatio_4_3,
	kHalDisplayAspectRatio_Zoom,
	kHalDisplayAspectRatio_Max,
} THalDisplayAspectRatio;

typedef enum _THalDisplayMirrorMode {
	kHalDisplayMirrorMode_Disable,
	kHalDisplayMirrorMode_H,
	kHalDisplayMirrorMode_V,
	kHalDisplayMirrorMode_HV,
	kHalDisplayMirrorMode_Max,
} THalDisplayMirrorMode;

typedef struct  // PAR
{
	u16_t wPar_X;  /**< par x */
	u16_t wPar_Y;  /**< par y */
	u32_t dwHSize; /**< Format Display HSize */
	u32_t dwVSize; /**< Format Display VSize */
} TPARParameter;

typedef enum {
	EQ_AUTO,   /**< Auto eq */
	EQ_LOW,    /**< Low eq */
	EQ_MIDDLE, /**< Middle eq */
	EQ_HIGN,   /**< High eq */
	EQ_STRONG, /**< Strong eq */
} HdmiEq_t;

typedef enum tagHDMI_INFORMAT {
	HDMI_IN_RGB444,      /**< HDMI in format, RGB444 */
	HDMI_IN_YUV444,      /**< HDMI in format, YUV444 */
	HDMI_IN_YUV422,      /**< HDMI in format, YUV422 */
	DVI_IN_RGB444,       /**< HDMI in format, RGB444 */
	HDMI_IN_YUV420,      /**< HDMI in format, YUV422 */
	NONE_VIDEO_IN_FORMAT /**< None video in format */
} HDMI_INFORMAT;

typedef enum {
	AUDIO_44KHZ = 0,         /**< HDMI audio sample rate, 44KHz */
	AUDIO_NOT_INDICATED = 1, /**< HDMI audio sample rate, not indicated */
	AUDIO_48KHZ = 2,         /**< HDMI audio sample rate, 48KHz */
	AUDIO_32KHZ = 3,         /**< HDMI audio sample rate, 32KHz */
	AUDIO_22KHZ = 4,         /**< HDMI audio sample rate, 22KHz */
	AUDIO_24KHZ = 6,         /**< HDMI audio sample rate, 24KHz */
	AUDIO_88KHZ = 8,         /**< HDMI audio sample rate, 88KHz */
	AUDIO_96KHZ = 10,        /**< HDMI audio sample rate, 96KHz */
	AUDIO_176KHZ = 12,       /**< HDMI audio sample rate, 176KHz */
	AUDIO_192KHZ = 14,       /**< HDMI audio sample rate, 192KHz */
} HDMI_FSRATE;

typedef enum {
	HDMI_COLORDEPTH_LEGACY = 0,
	HDMI_COLORDEPTH_24BIT = 1,
	HDMI_COLORDEPTH_30BIT = 2,
	HDMI_COLORDEPTH_36BIT = 3,
	HDMI_COLORDEPTH_48BIT = 4
} HDMI_COLORDEPTH;

typedef struct {
	u8_t bHdmiEnable;            /**< HDMI Enable */
	HDMI_INFORMAT eInputFormat;  /**< HDMI Video Format */
	HDMI_FSRATE eFsRate;         /**< HDMI Audio Sample Rate */
	u8_t bIsPcm;                 /**< HDMI Audio PCM or Not */
	u32_t acpLevel;              /**< HDMI ACP Level */
	HDMI_COLORDEPTH eColorDepth; /**<  HDMI color depth */
} SvpHDMIStatus_t;

struct vbi_buff_info {
	unsigned long buffno; /**< vbi buff no */
	void *p;              /**< vbi buffer */
};

typedef struct _tagPQCFG_ID_BUF {
	u32_t CFG_ID;
	u32_t CFG_BUF_Addr;
	u32_t CFG_BUF_Size;
} PQCFG_ID_BUF_t;

typedef struct {
	u32_t Size;   // BYTEs = sizeof(TargetPanelPQInfo_t)
	u32_t Tmax;   // will  div 10000; 100nits --10000nits
	u32_t Tmin;   // will  div 10000  0 nits -- 100 nits
	u32_t Gamma;  // will  div 10000  1.8 -- 2.4

	u32_t version;
	u32_t boost;

	u32_t Pri_Rx;  // will  div 10000
	u32_t Pri_Ry;  // will  div 10000
	u32_t Pri_Gx;  // will  div 10000
	u32_t Pri_Gy;  // will  div 10000
	u32_t Pri_Bx;  // will  div 10000
	u32_t Pri_By;  // will  div 10000
	u32_t Pri_Wx;  // will  div 10000
	u32_t Pri_Wy;  // will  div 10000

    u8_t PanelinfoOrigin;  // 0: from default; 1: from database; 2: from API
} TargetPanelPQInfo_t;

/**
* VBI mode enumeration
*/
typedef enum _tagVBIMode {
	_VBI_OFF_ = 0x00,           /**<VBI off*/
	_VBI_CC_ONLY_ = 0x01,       /**<VBI CC only*/
	_VBI_VCHIP_ONLY_ = 0x02,    /**<VBI vchip only*/
	_VBI_TELETEXT_ONLY_ = 0x04, /**<VBI teletext only*/
	_VBI_ALL_DATA_ = 0x08,      /**<VBI all data*/
	_VBI_PARSE_FLAG_ = 0x10     /**<VBI parse flag*/
} VBIMode_e;

typedef enum _TDeviceType {
	kDeviceType_Atv,
	kDeviceType_Cvbs,
	kDeviceType_Hdmi,
	kDeviceType_VideoDec,
	kDeviceType_Image,
	kDeviceType_Dummy,
	kDeviceType_Max,
} TDeviceType;

typedef enum _TDeviceId {
	kDeviceId_TCD3,
	kDeviceId_HDMI1,
	kDeviceId_HDMI2,
	kDeviceId_VideoDec,
	kDeviceId_Image,
	kDeviceId_Dummy,
	kDeviceId_Max,
} TDeviceId;

typedef enum _TSourceId {
	kSourceId_Dummy,
	kSourceId_VideoDec,  // DTV/USB/OTT
	kSourceId_Image,
	kSourceId_HDMI_1,
	kSourceId_HDMI_2,
	kSourceId_HDMI_3,
	kSourceId_HDMI_4,
	kSourceId_CVBS_1,
	kSourceId_CVBS_2,
	kSourceId_CVBS_3,
	kSourceId_ATV,
	kSourceId_Max,
} TSourceId;

typedef struct _TColorRgb {
	u32_t R;
	u32_t G;
	u32_t B;
} TColorRgb;

typedef enum _TColorFormat {
	kColorFormat_YUV420_888,
	kColorFormat_YUV420_1088,
	kColorFormat_YUV420_101010,
	kColorFormat_YUV420_121212,

	kColorFormat_YUV422_888,
	kColorFormat_YUV422_1088,
	kColorFormat_YUV422_101010,
	kColorFormat_YUV422_121212,

	kColorFormat_YUV444_888,
	kColorFormat_YUV444_101010,
	kColorFormat_YUV444_121212,

	kColorFormat_RGB_888,
	kColorFormat_RGB_101010,
	kColorFormat_RGB_121212,

	kColorFormat_YUV420_101010_P010_Low,        // AFBD P010 Low 10bits commponents
	kColorFormat_YUV420_101010_P010_High,       // AFBD P010 High 10bits commponents

	kColorFormat_DETN_ALPHA_1_BIT,
	kColorFormat_Max,
} TColorFormat;

typedef enum _TColorSpace {
	kColorSpace_BT601,
	kColorSpace_BT709,
	kColorSpace_BT2020_NCLYCC,
	kColorSpace_BT2020_CLYCC,
	kColorSpace_xvYCC,
	kColorSpace_RGB,
	kColorSpace_Max,
} TColorSpace;

typedef enum _THdrScheme {
	kHdrScheme_Sdr,
	kHdrScheme_Hdr10,
	kHdrScheme_Hdr10Plus,
	kHdrScheme_Hdr10Sgmip,
	kHdrScheme_Sl_Hdr1,
	kHdrScheme_Sl_Hdr2,
	kHdrScheme_Hlg,
	kHdrScheme_DbVision,
	kHdrScheme_DbVisionLowLatency,
	kHdrScheme_Max,
} THdrScheme;

typedef enum _TCompressFormat {
	kCompressFormat_PACK = 0,
	kCompressFormat_1D,
	kCompressFormat_2D,
	kCompressFormat_BLOCK_8x2,
	kCompressFormat_BLOCK_16x2,
	kCompressFormat_BLOCK_8x4,
	kCompressFormat_BLOCK_TAIL,
	kCompressFormat_Max,
} TCompressFormat;

typedef enum _TDisplayAspectRatio {
	kDisplayAspectRatio_Auto,          // AFD, PAR, AR, WSS
	kDisplayAspectRatio_KeepSourceAr,  // display aspect ratio remains the same as source
	kDisplayAspectRatio_Full,
	kDisplayAspectRatio_16_9,
	kDisplayAspectRatio_4_3,
	kDisplayAspectRatio_Zoom,
	kDisplayAspectRatio_Max,
} TDisplayAspectRatio;

typedef struct _TWin {
	s32_t h_start;
	s32_t h_size;
	s32_t v_start;
	s32_t v_size;
} TWin;

typedef struct _TWinSize {
	s32_t h_size;
	s32_t v_size;
} TWinSize;

typedef struct _TActiveWin {
	TWin full;
	TWin crop;
} TActiveWin;

typedef struct _TDisplayCfg {
	TActiveWin source;
	TActiveWin destination;
	TDisplayAspectRatio aspect_ratio;
} TDisplayCfg;

typedef enum _TDisplayMirrorMode {
	kDisplayMirrorMode_Disable,
	kDisplayMirrorMode_H,
	kDisplayMirrorMode_V,
	kDisplayMirrorMode_HV,
	kDisplayMirrorMode_Max,
} TDisplayMirrorMode;

typedef struct _TSignalTiming {
	u32_t h_total;
	u32_t v_total;
	u32_t h_active;
	u32_t v_active;
	u32_t hde_start;      // from HS rising edge to HDE rising edge
	u32_t vde_start;      // from VS rising edge to VDE rising edge
} TSignalTiming;

typedef enum _TCanvasAspectCode {
	kCanvasAspectCode_4_3,
	kCanvasAspectCode_16_9,
	kCanvasAspectCode_21_9,
	kCanvasAspectCode_Max,
	kCanvasAspectCode_Invalid,
} TCanvasAspectCode;

typedef enum _TVideoAspectCode {
	kVideoAspectCode_4_3,       // 4:3
	kVideoAspectCode_16_9,      // 16:9
	kVideoAspectCode_221_1,     // 2.21:1
	kVideoAspectCode_Max,
	kVideoAspectCode_Square,    // 1:1
	kVideoAspectCode_Invalid,
} TVideoAspectCode;

typedef struct _TVideoPictureAspectRatio {
	s32_t horizontal;
	s32_t vertical;
} TVideoPictureAspectRatio;

typedef enum _TDisplayAfd {
	kDisplayAfd_0000 = 0,   // 1000
	kDisplayAfd_0001 = 1,   // 1001
	kDisplayAfd_0010 = 2,   // 1010
	kDisplayAfd_0011 = 3,   // 1011
	kDisplayAfd_0100 = 4,   // 1100
	kDisplayAfd_0101 = 5,   // 1101
	kDisplayAfd_0110 = 6,   // 1110
	kDisplayAfd_0111 = 7,   // 1111
	kDisplayAfd_1000 = 8,   // 1000
	kDisplayAfd_1001 = 9,   // 1001
	kDisplayAfd_1010 = 10,  // 1010
	kDisplayAfd_1011 = 11,  // 1011
	kDisplayAfd_1100 = 12,  // 1100
	kDisplayAfd_1101 = 13,  // 1101
	kDisplayAfd_1110 = 14,  // 1110
	kDisplayAfd_1111 = 15,  // 1111
	kDisplayAfd_Max,
	kDisplayAfd_Invalid
} TDisplayAfd;

typedef struct _TVideoDisplayInfo {
	bool b_par_valid;
	TVideoPictureAspectRatio par;
	u32_t afd;
} TVideoDisplayInfo;

typedef enum _TPicMode {
	kPictureMode_Vivid,
	kPictureMode_Standard,
	kPictureMode_Mild,
	kPictureMode_Game,
	kPictureMode_Calibrated,
	kPictureMode_CalibratedDark,
	kPictureMode_Computer,
	kPictureMode_Cinema,
	kPictureMode_Home,
	kPictureMode_Sports,
	kPictureMode_Shop,
	kPictureMode_Animation,
	kPictureMode_HDR,
	kPictureMode_Graphic,
	kPictureMode_Expert1,
	kPictureMode_Expert2,
	kPictureMode_Max,
} TPicMode;

typedef enum _TVideoRangeMode {
	kVideoRangeMode_Auto,
	kVideoRangeMode_Limit,
	kVideoRangeMode_Full,
} TVideoRangeMode;

typedef enum _TLowLatencyMode {
	kLowLatencyMode_On,
	kLowLatencyMode_Off,
	kLowLatencyMode_Auto,
} TLowLatencyMode;

typedef struct _TSignalInfo {
	TSourceId    source_id;
	u32_t        signal_id;
	u32_t        frame_rate;
	bool         b_interlace;
	TColorFormat color_format;
	u32_t        color_space;
	THdrScheme   hdr_scheme;
	u32_t        *p_hdr_static_metadata;
	u32_t        *p_hdr_dynamic_metadata;

	union {
		struct {
			u32_t  frame_rate_orig;
			u32_t  compress_mode;
			u32_t  buffer_stride;  // in Byte.
			TWin   pic_size;
			TWin   pic_size_origin;
			bool b_full_range;
			TVideoDisplayInfo stream_display_info;

			bool enable_display_cfg;
			TDisplayCfg display_cfg;
		} video_dec;

		struct {
			u32_t buffer_stride;
			u32_t buffer_addr_y;
			u32_t buffer_addr_c;
			TWin pic_size;
		} image;

		struct {
			bool b_full_range;   // for HDMI, full range / limit range
			bool b_timing_valid;
			TSignalTiming   timing;
			bool b_dvi_mode;
			u32_t tmds_clock;
			u8_t prmode;
			u32_t audio_fs;
			u32_t fifoErrCnt;
			u32_t audioPktErrCnt;
		} hdmi;

		struct {
			bool b_timing_valid;
			TSignalTiming   timing;
			u32_t uid;
		} cvbs;

	} ext;


	//VideoOutputFormat_e    compress_mode
	//TAFDParameter          AFDPara;
} TSignalInfo;

typedef enum _TAtvStd {
	kAtvStd_NoSignal = 0,
	kAtvStd_NTSCM    = 1 << 0,
	kAtvStd_NTSC443  = 1 << 1,
	kAtvStd_PAL60    = 1 << 2,
	kAtvStd_PAL50    = 1 << 3,
	kAtvStd_PALM     = 1 << 4,
	kAtvStd_SECAM    = 1 << 5,
	kAtvStd_PALNC    = 1 << 6,
	kAtvStd_Unknow   = 1 << 7,
	kAtvStd_All      = 0xffffffff,
} TAtvStd;

typedef enum _TAtvRegion {
	kAtvRegion_US,
	kAtvRegion_EU,
	kAtvRegion_CN,
	kAtvRegion_LA,
	kAtvRegion_SA,
	kAtvRegion_All,
	kAtvRegion_Max
} TAtvRegion;

/// TODO: should be removed
typedef enum tagEOutputType {
	_OUTPUT_MP_ = 0,
	_OUTPUT_PIP_,
	_OUTPUT_MAX_,
} EOutputType;
/** Main Picture Window */
#define _WT_MP_ (_OUTPUT_MP_)
/** Picture In Picture Window */
#define _WT_PP_ (_OUTPUT_PIP_)



#define EXTERN_C
#define TFCSYSTEM_API
#define TFCDISPLAY_API


/*
 * TODO: should be removed
 * */
typedef int               RETURN_TYPE;
#define SYS_NOERROR       0
#define SYS_FAILED        1
#define SYS_NULL_POINTER  2
#define SYS_SMALL_BUFFER  3
#define SYS_NO_MEMORY     4
#define SYS_BAD_PARAM     5
#define SYS_NOT_SUPPORT   6
#define SYS_NO_FUNCTCION  7


/*
 * TODO: should be removed
 * */
typedef long  HRESULT;
#define SUCCEEDED(Status)         ((HRESULT)(Status)     >= 0)
#define FAILED(Status)            ((HRESULT)(Status)     <  0)
#define E_UNEXPECTED              ((HRESULT)0x8000FFFFL)
#define E_NOTIMPL                 ((HRESULT)0x80004001L)
#define E_OUTOFMEMORY             ((HRESULT)0x8007000EL)
#define E_INVALIDARG              ((HRESULT)0x80070057L)
#define E_NOINTERFACE             ((HRESULT)0x80004002L)
#define E_POINTER                 ((HRESULT)0x80004003L)
#define E_HANDLE                  ((HRESULT)0x80070006L)
#define E_ABORT                   ((HRESULT)0x80004004L)
#define E_FAIL                    ((HRESULT)0x80004005L)
#define E_ACCESSDENIED            ((HRESULT)0x80070005L)
#define S_OK                      ((HRESULT)0x00000000L)
#define S_FALSE                   ((HRESULT)0x00000001L)
#define REGDB_E_CLASSNOTREG       ((HRESULT)0x80040154L)
#define CLASS_E_NOAGGREGATION     ((HRESULT)0x80040110L)
#define CLASS_E_CLASSNOTAVAILABLE ((HRESULT)0x80040111L)



/*
* TODO:   should be removed.
* */
/*
#define _BIT0_    (1<<0)
#define _BIT1_    (1<<1)
#define _BIT2_    (1<<2)
#define _BIT3_    (1<<3)
#define _BIT4_    (1<<4)
#define _BIT5_    (1<<5)
#define _BIT6_    (1<<6)
#define _BIT7_    (1<<7)
#define _BIT8_    (1<<8)
#define _BIT9_    (1<<9)
#define _BIT10_   (1<<10)
#define _BIT11_   (1<<11)
#define _BIT12_   (1<<12)
#define _BIT13_   (1<<13)
#define _BIT14_   (1<<14)
#define _BIT15_   (1<<15)
#define _BIT16_   (1<<16)
#define _BIT17_   (1<<17)
#define _BIT18_   (1<<18)
#define _BIT19_   (1<<19)
#define _BIT20_   (1<<20)
#define _BIT21_   (1<<21)
#define _BIT22_   (1<<22)
#define _BIT23_   (1<<23)
#define _BIT24_   (1<<24)
#define _BIT25_   (1<<25)
#define _BIT26_   (1<<26)
#define _BIT27_   (1<<27)
#define _BIT28_   (1<<28)
#define _BIT29_   (1<<29)
#define _BIT30_   (1<<30)
#define _BIT31_   (1<<31)
*/
#define _REG_00_ 0x00
#define _REG_01_ 0x01
#define _REG_02_ 0x02
#define _REG_03_ 0x03
#define _REG_04_ 0x04
#define _REG_05_ 0x05
#define _REG_06_ 0x06
#define _REG_07_ 0x07
#define _REG_08_ 0x08
#define _REG_09_ 0x09
#define _REG_0A_ 0x0A
#define _REG_0B_ 0x0B
#define _REG_0C_ 0x0C
#define _REG_0D_ 0x0D
#define _REG_0E_ 0x0E
#define _REG_0F_ 0x0F
#define _REG_10_ 0x10
#define _REG_11_ 0x11
#define _REG_12_ 0x12
#define _REG_13_ 0x13
#define _REG_14_ 0x14
#define _REG_15_ 0x15
#define _REG_16_ 0x16
#define _REG_17_ 0x17
#define _REG_18_ 0x18
#define _REG_19_ 0x19
#define _REG_1A_ 0x1A
#define _REG_1B_ 0x1B
#define _REG_1C_ 0x1C
#define _REG_1D_ 0x1D
#define _REG_1E_ 0x1E
#define _REG_1F_ 0x1F
#define _REG_20_ 0x20
#define _REG_21_ 0x21
#define _REG_22_ 0x22
#define _REG_23_ 0x23
#define _REG_24_ 0x24
#define _REG_25_ 0x25
#define _REG_26_ 0x26
#define _REG_27_ 0x27
#define _REG_28_ 0x28
#define _REG_29_ 0x29
#define _REG_2A_ 0x2A
#define _REG_2B_ 0x2B
#define _REG_2C_ 0x2C
#define _REG_2D_ 0x2D
#define _REG_2E_ 0x2E
#define _REG_2F_ 0x2F
#define _REG_30_ 0x30
#define _REG_31_ 0x31
#define _REG_32_ 0x32
#define _REG_33_ 0x33
#define _REG_34_ 0x34
#define _REG_35_ 0x35
#define _REG_36_ 0x36
#define _REG_37_ 0x37
#define _REG_38_ 0x38
#define _REG_39_ 0x39
#define _REG_3A_ 0x3A
#define _REG_3B_ 0x3B
#define _REG_3C_ 0x3C
#define _REG_3D_ 0x3D
#define _REG_3E_ 0x3E
#define _REG_3F_ 0x3F
#define _REG_40_ 0x40
#define _REG_41_ 0x41
#define _REG_42_ 0x42
#define _REG_43_ 0x43
#define _REG_44_ 0x44
#define _REG_45_ 0x45
#define _REG_46_ 0x46
#define _REG_47_ 0x47
#define _REG_48_ 0x48
#define _REG_49_ 0x49
#define _REG_4A_ 0x4A
#define _REG_4B_ 0x4B
#define _REG_4C_ 0x4C
#define _REG_4D_ 0x4D
#define _REG_4E_ 0x4E
#define _REG_4F_ 0x4F
#define _REG_50_ 0x50
#define _REG_51_ 0x51
#define _REG_52_ 0x52
#define _REG_53_ 0x53
#define _REG_54_ 0x54
#define _REG_55_ 0x55
#define _REG_56_ 0x56
#define _REG_57_ 0x57
#define _REG_58_ 0x58
#define _REG_59_ 0x59
#define _REG_5A_ 0x5A
#define _REG_5B_ 0x5B
#define _REG_5C_ 0x5C
#define _REG_5D_ 0x5D
#define _REG_5E_ 0x5E
#define _REG_5F_ 0x5F
#define _REG_60_ 0x60
#define _REG_61_ 0x61
#define _REG_62_ 0x62
#define _REG_63_ 0x63
#define _REG_64_ 0x64
#define _REG_65_ 0x65
#define _REG_66_ 0x66
#define _REG_67_ 0x67
#define _REG_68_ 0x68
#define _REG_69_ 0x69
#define _REG_6A_ 0x6A
#define _REG_6B_ 0x6B
#define _REG_6C_ 0x6C
#define _REG_6D_ 0x6D
#define _REG_6E_ 0x6E
#define _REG_6F_ 0x6F
#define _REG_70_ 0x70
#define _REG_71_ 0x71
#define _REG_72_ 0x72
#define _REG_73_ 0x73
#define _REG_74_ 0x74
#define _REG_75_ 0x75
#define _REG_76_ 0x76
#define _REG_77_ 0x77
#define _REG_78_ 0x78
#define _REG_79_ 0x79
#define _REG_7A_ 0x7A
#define _REG_7B_ 0x7B
#define _REG_7C_ 0x7C
#define _REG_7D_ 0x7D
#define _REG_7E_ 0x7E
#define _REG_7F_ 0x7F
#define _REG_80_ 0x80
#define _REG_81_ 0x81
#define _REG_82_ 0x82
#define _REG_83_ 0x83
#define _REG_84_ 0x84
#define _REG_85_ 0x85
#define _REG_86_ 0x86
#define _REG_87_ 0x87
#define _REG_88_ 0x88
#define _REG_89_ 0x89
#define _REG_8A_ 0x8A
#define _REG_8B_ 0x8B
#define _REG_8C_ 0x8C
#define _REG_8D_ 0x8D
#define _REG_8E_ 0x8E
#define _REG_8F_ 0x8F
#define _REG_90_ 0x90
#define _REG_91_ 0x91
#define _REG_92_ 0x92
#define _REG_93_ 0x93
#define _REG_94_ 0x94
#define _REG_95_ 0x95
#define _REG_96_ 0x96
#define _REG_97_ 0x97
#define _REG_98_ 0x98
#define _REG_99_ 0x99
#define _REG_9A_ 0x9A
#define _REG_9B_ 0x9B
#define _REG_9C_ 0x9C
#define _REG_9D_ 0x9D
#define _REG_9E_ 0x9E
#define _REG_9F_ 0x9F
#define _REG_A0_ 0xA0
#define _REG_A1_ 0xA1
#define _REG_A2_ 0xA2
#define _REG_A3_ 0xA3
#define _REG_A4_ 0xA4
#define _REG_A5_ 0xA5
#define _REG_A6_ 0xA6
#define _REG_A7_ 0xA7
#define _REG_A8_ 0xA8
#define _REG_A9_ 0xA9
#define _REG_AA_ 0xAA
#define _REG_AB_ 0xAB
#define _REG_AC_ 0xAC
#define _REG_AD_ 0xAD
#define _REG_AE_ 0xAE
#define _REG_AF_ 0xAF
#define _REG_B0_ 0xB0
#define _REG_B1_ 0xB1
#define _REG_B2_ 0xB2
#define _REG_B3_ 0xB3
#define _REG_B4_ 0xB4
#define _REG_B5_ 0xB5
#define _REG_B6_ 0xB6
#define _REG_B7_ 0xB7
#define _REG_B8_ 0xB8
#define _REG_B9_ 0xB9
#define _REG_BA_ 0xBA
#define _REG_BB_ 0xBB
#define _REG_BC_ 0xBC
#define _REG_BD_ 0xBD
#define _REG_BE_ 0xBE
#define _REG_BF_ 0xBF
#define _REG_C0_ 0xC0
#define _REG_C1_ 0xC1
#define _REG_C2_ 0xC2
#define _REG_C3_ 0xC3
#define _REG_C4_ 0xC4
#define _REG_C5_ 0xC5
#define _REG_C6_ 0xC6
#define _REG_C7_ 0xC7
#define _REG_C8_ 0xC8
#define _REG_C9_ 0xC9
#define _REG_CA_ 0xCA
#define _REG_CB_ 0xCB
#define _REG_CC_ 0xCC
#define _REG_CD_ 0xCD
#define _REG_CE_ 0xCE
#define _REG_CF_ 0xCF
#define _REG_D0_ 0xD0
#define _REG_D1_ 0xD1
#define _REG_D2_ 0xD2
#define _REG_D3_ 0xD3
#define _REG_D4_ 0xD4
#define _REG_D5_ 0xD5
#define _REG_D6_ 0xD6
#define _REG_D7_ 0xD7
#define _REG_D8_ 0xD8
#define _REG_D9_ 0xD9
#define _REG_DA_ 0xDA
#define _REG_DB_ 0xDB
#define _REG_DC_ 0xDC
#define _REG_DD_ 0xDD
#define _REG_DE_ 0xDE
#define _REG_DF_ 0xDF
#define _REG_E0_ 0xE0
#define _REG_E1_ 0xE1
#define _REG_E2_ 0xE2
#define _REG_E3_ 0xE3
#define _REG_E4_ 0xE4
#define _REG_E5_ 0xE5
#define _REG_E6_ 0xE6
#define _REG_E7_ 0xE7
#define _REG_E8_ 0xE8
#define _REG_E9_ 0xE9
#define _REG_EA_ 0xEA
#define _REG_EB_ 0xEB
#define _REG_EC_ 0xEC
#define _REG_ED_ 0xED
#define _REG_EE_ 0xEE
#define _REG_EF_ 0xEF
#define _REG_F0_ 0xF0
#define _REG_F1_ 0xF1
#define _REG_F2_ 0xF2
#define _REG_F3_ 0xF3
#define _REG_F4_ 0xF4
#define _REG_F5_ 0xF5
#define _REG_F6_ 0xF6
#define _REG_F7_ 0xF7
#define _REG_F8_ 0xF8
#define _REG_F9_ 0xF9
#define _REG_FA_ 0xFA
#define _REG_FB_ 0xFB
#define _REG_FC_ 0xFC
#define _REG_FD_ 0xFD
#define _REG_FE_ 0xFE
#define _REG_FF_ 0xFF

#endif
