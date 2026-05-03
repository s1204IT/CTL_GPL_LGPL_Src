/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
******************************************************************************
* @file    ms7200_typedef.h
* @author
* @version V1.0.0
* @date    24-Nov-2020
* @brief   Definitions for typedefs.
*
* Copyright (c) 2009-2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#ifndef __MACROSILICON_MS7200_COMMON_TYPEDEF_H__
#define __MACROSILICON_MS7200_COMMON_TYPEDEF_H__

/*
** Global typedefs.
*/
#ifndef MS_GLOBAL_DEFINE
#define MS_GLOBAL_DEFINE

#ifndef NULL
#define NULL ((void *)0)
#endif

// For ARM platform
#if defined (_PLATFORM_ARM_)
#define  __CODE const
#define  __XDATA
#define  __DATA
#define __IDATA
#define  __NEAR
#define  __IO volatile


typedef _Bool BOOL;

#elif defined (__STD_GCC__)
#define  __CODE const
#define  __XDATA
#define  __DATA
#define __IDATA
#define  __NEAR
#define  __IO volatile


typedef _Bool BOOL;

#elif defined (_PLATFORM_WINDOWS_)
#define  __CODE
#define  __XDATA
#define  __DATA
#define __IDATA
#define  __NEAR
#define  __IO

#elif defined (__KEIL_C__)
#define __CODE code
#define __XDATA xdata
#define __DATA data
#define __IDATA idata
#define __NEAR
#define __IO volatile

//bool bype
typedef bit BOOL;

#elif defined (__CSMC__)
#define __CODE const
#define __XDATA
#define __DATA
#define __IDATA
#define __NEAR (@near)
#define __IO volatile

//bool bype
typedef _Bool BOOL;
#elif defined (_IAR_)
#define __CODE const
#define __XDATA
#define __DATA
#define __IDATA
#define __NEAR (@near)
#define __IO volatile

//bool bype
typedef _Bool BOOL;
#endif // end of compiler platform define


//unsigned integer type
typedef unsigned char UINT8;
typedef char          CHAR;
typedef unsigned short UINT16;

//signed integer type
typedef signed char INT8;
typedef signed short INT16;

//32bit type
#if defined (_PLATFORM_ARM_) || defined (_PLATFORM_WINDOWS_)
typedef unsigned int UINT32;
typedef signed int INT32;
#else
typedef unsigned long int UINT32;
typedef signed long int INT32;
#endif

#define VOID void

// #define FALSE 0
// #define TRUE  1

#define DISABLE 0
#define ENABLE  1

#define LOW     0
#define HIGH    1

#define OFF     0
#define ON      1


// Helper macros.
#define _UNUSED_(arg)     ((arg) = (arg))

#ifndef _countof
#define _countof(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))
#endif

#ifndef max
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)   (((a) < (b)) ? (a) : (b))
#endif

/*
* generic mask macro definitions
*/
#define MSRT_BIT0                   (0x01)
#define MSRT_BIT1                   (0x02)
#define MSRT_BIT2                   (0x04)
#define MSRT_BIT3                   (0x08)
#define MSRT_BIT4                   (0x10)
#define MSRT_BIT5                   (0x20)
#define MSRT_BIT6                   (0x40)
#define MSRT_BIT7                   (0x80)
#define MSRT_MSB8BITS               MSRT_BIT7
#define MSRT_LSB                    MSRT_BIT0

// Bit7 ~ Bit0
#define MSRT_BITS7_6                (0xC0)
#define MSRT_BITS7_5                (0xE0)
#define MSRT_BITS7_4                (0xF0)
#define MSRT_BITS7_3                (0xF8)
#define MSRT_BITS7_2                (0xFC)
#define MSRT_BITS7_1                (0xFE)
#define MSRT_BITS7_0                (0xff)

#define MSRT_BITS6_5                (0x60)
#define MSRT_BITS6_4                (0x70)
#define MSRT_BITS6_2                (0x7c)
#define MSRT_BITS6_1                (0x7e)
#define MSRT_BITS6_0                (0x7f)

#define MSRT_BITS5_4                (0x30)
#define MSRT_BITS5_3                (0x38)
#define MSRT_BITS5_2                (0x3c)
#define MSRT_BITS5_0                (0x3f)

#define MSRT_BITS4_3                (0x18)
#define MSRT_BITS4_2                (0x1c)
#define MSRT_BITS4_1                (0x1e)
#define MSRT_BITS4_0                (0x1f)

#define MSRT_BITS3_2                (0x0C)
#define MSRT_BITS3_1                (0x0E)
#define MSRT_BITS3_0                (0x0F)

#define MSRT_BITS2_1                (0x06)
#define MSRT_BITS2_0                (0x07)

#define MSRT_BITS1_0                (0x03)

#endif


// 20121207, for video data type
#ifndef MS_TIMING_DEFINE
#define MS_TIMING_DEFINE

typedef struct _T_MS_VIDEO_SIZE_ {
    UINT16 u16_h;
    UINT16 u16_v;
} VIDEOSIZE_T;

typedef struct _T_MS7200_VIDEO_TIMING_ {
    UINT8           u8_polarity;
    UINT16          u16_htotal;
    UINT16          u16_vtotal;
    UINT16          u16_hactive;
    UINT16          u16_vactive;
    UINT16          u16_pixclk;     /*10000hz*/
    UINT16          u16_vfreq;      /*0.01hz*/
    UINT16          u16_hoffset;    /* h sync start to h active*/
    UINT16          u16_voffset;    /* v sync start to v active*/
    UINT16          u16_hsyncwidth;
    UINT16          u16_vsyncwidth;
} VIDEOTIMING_T;

typedef enum _E_SYNC_POLARITY_ {
    ProgrVNegHNeg = 0x01,
    ProgrVNegHPos = 0x03,
    ProgrVPosHNeg = 0x05,
    ProgrVPosHPos = 0x07,

    InterVNegHNeg = 0x00,
    InterVNegHPos = 0x02,
    InterVPosHNeg = 0x04,
    InterVPosHPos = 0x06
} SYNCPOLARITY_E;

typedef struct _T_WIN_BORDER_ {
    INT16 top;
    INT16 bottom;
    INT16 left;
    INT16 right;
} WINBORDER_T;

#endif

//HDMI video
#ifndef MS_HDMI_DEFINE
#define MS_HDMI_DEFINE

typedef enum _E_HDMI_VIDEO_CLK_REPEAT_ {
    HDMI_X1CLK      = 0x00,
    HDMI_X2CLK      = 0x01,
    HDMI_X3CLK      = 0x02,
    HDMI_X4CLK      = 0x03,
    HDMI_X5CLK      = 0x04,
    HDMI_X6CLK      = 0x05,
    HDMI_X7CLK      = 0x06,
    HDMI_X8CLK      = 0x07,
    HDMI_X9CLK      = 0x08,
    HDMI_X10CLK     = 0x09
} HDMI_CLK_RPT_E;

typedef enum _E_HDMI_VIDEO_ASPECT_RATIO_ {
    HDMI_4X3     = 0x01,
    HDMI_16X9    = 0x02
} HDMI_ASPECT_RATIO_E;


typedef enum _E_HDMI_VIDEO_SCAN_INFO_ {
    HDMI_OVERSCAN     = 0x01,    //television type
    HDMI_UNDERSCAN    = 0x02     //computer type
} HDMI_SCAN_INFO_E;

typedef enum _E_HDMI_COLOR_SPACE_ {
    HDMI_RGB        = 0x00,
    HDMI_YCBCR422   = 0x01,
    HDMI_YCBCR444   = 0x02,
    HDMI_YUV420     = 0x03
} HDMI_CS_E;

typedef enum _E_HDMI_COLOR_DEPTH_ {
    HDMI_COLOR_DEPTH_8BIT    = 0x00,
    HDMI_COLOR_DEPTH_10BIT   = 0x01,
    HDMI_COLOR_DEPTH_12BIT   = 0x02,
    HDMI_COLOR_DEPTH_16BIT   = 0x03
} HDMI_COLOR_DEPTH_E;

typedef enum _E_HDMI_COLORIMETRY_ {
    HDMI_COLORIMETRY_601    = 0x00,
    HDMI_COLORIMETRY_709    = 0x01,
    HDMI_COLORIMETRY_656    = 0x02,
    HDMI_COLORIMETRY_1120   = 0x03,
    HDMI_COLORIMETRY_SMPTE  = 0x04,
    HDMI_COLORIMETRY_XVYCC601 = 0x05,
    HDMI_COLORIMETRY_XVYCC709 = 0x06
} HDMI_COLORIMETRY_E;

//HDMI vendor specific
typedef enum _E_HDMI_VIDEO_FORMAT_ {
    HDMI_NO_ADD_FORMAT,
    HDMI_4Kx2K_FORMAT,
    HDMI_3D_FORMAT
} HDMI_VIDEO_FORMAT_E;

typedef enum _E_HDMI_4Kx2K_VIC_ {
    HDMI_4Kx2K_30HZ = 0x01,
    HDMI_4Kx2K_25HZ,
    HDMI_4Kx2K_24HZ,
    HDMI_4Kx2K_24HZ_SMPTE
} HDMI_4Kx2K_VIC_E;

typedef enum _E_HDMI_3D_STRUCTURE_ {
    HDMI_FRAME_PACKING,
    HDMI_FIELD_ALTERNATIVE,
    HDMI_LINE_ALTERNATIVE,
    HDMI_SIDE_BY_SIDE_FULL,
    L_DEPTH,
    L_DEPTH_GRAPHICS,
    SIDE_BY_SIDE_HALF = 8
} HDMI_3D_STRUCTURE_E;

//HDMI audio
typedef enum _E_HDMI_AUDIO_MODE_ {
    HDMI_AUD_MODE_AUDIO_SAMPLE  = 0x00,
    HDMI_AUD_MODE_HBR           = 0x01,
    HDMI_AUD_MODE_DSD           = 0x02,
    HDMI_AUD_MODE_DST           = 0x03
} HDMI_AUDIO_MODE_E;

typedef enum _E_HDMI_AUDIO_I2S_RATE_ {
    HDMI_AUD_RATE_44K1  = 0x00,
    HDMI_AUD_RATE_48K   = 0x02,
    HDMI_AUD_RATE_32K   = 0x03,
    HDMI_AUD_RATE_88K2  = 0x08,
    HDMI_AUD_RATE_96K   = 0x0A,
    HDMI_AUD_RATE_176K4 = 0x0C,
    HDMI_AUD_RATE_192K  = 0x0E
} HDMI_AUDIO_RATE_E;

typedef enum _E_HDMI_AUDIO_LENGTH_ {
    HDMI_AUD_LENGTH_16BITS    = 0x00,
    HDMI_AUD_LENGTH_20BITS    = 0x01,
    HDMI_AUD_LENGTH_24BITS    = 0x02
} HDMI_AUDIO_LENGTH_E;

typedef enum _E_HDMI_AUDIO_CHANNEL_ {
    HDMI_AUD_2CH    = 0x01,
    HDMI_AUD_3CH    = 0x02,
    HDMI_AUD_4CH    = 0x03,
    HDMI_AUD_5CH    = 0x04,
    HDMI_AUD_6CH    = 0x05,
    HDMI_AUD_7CH    = 0x06,
    HDMI_AUD_8CH    = 0x07
} HDMI_AUDIO_CHN_E;


typedef struct _T_HDMI_CONFIG_PARA_ {
    UINT8  u8_hdmi_flag;          // FALSE = dvi out;  TRUE = hdmi out
    UINT8  u8_vic;                // reference to CEA-861 VIC
    UINT16 u16_video_clk;         // TMDS video clk, uint 10000Hz
    UINT8  u8_clk_rpt;            // enum refer to HDMI_CLK_RPT_E. X2CLK = 480i/576i, others = X1CLK
    UINT8  u8_scan_info;          // enum refer to HDMI_SCAN_INFO_E
    UINT8  u8_aspect_ratio;       // enum refer to HDMI_ASPECT_RATIO_E
    UINT8  u8_color_space;        // enum refer to HDMI_CS_E
    UINT8  u8_color_depth;        // enum refer to HDMI_COLOR_DEPTH_E
    UINT8  u8_colorimetry;        // enum refer to HDMI_COLORIMETRY_E. IT601 = 480i/576i/480p/576p, ohters = IT709
    //
    UINT8  u8_video_format;       // enum refer to HDMI_VIDEO_FORMAT_E
    UINT8  u8_4Kx2K_vic;          // enum refer to HDMI1.4 extented resolution transmission
    UINT8  u8_3D_structure;       // enum refer to HDMI_3D_STRUCTURE_E
    //
    UINT8  u8_audio_mode;         // enum refer to HDMI_AUDIO_MODE_E
    UINT8  u8_audio_rate;         // enum refer to HDMI_AUDIO_RATE_E
    UINT8  u8_audio_bits;         // enum refer to HDMI_AUDIO_LENGTH_E
    UINT8  u8_audio_channels;     // enum refer to HDMI_AUDIO_CHN_E
    UINT8  u8_audio_speaker_locations;  // 0~255, refer to CEA-861 audio infoframe, BYTE4
} HDMI_CONFIG_T;
#endif

#ifndef MS_HDMIRX_DEFINE
#define MS_HDMIRX_DEFINE

//HDMI RX module define
typedef enum {
    HDMI_RX_CH0,
    HDMI_RX_CH1,
    HDMI_RX_CH2
} e_RxChannel;

typedef enum {
    OVERSAMPLE_DESIGNED,
    OVERSAMPLE_NETLIST,
    PI
} e_RxPhy;

typedef enum {
    AUTO_MODE,
    MANUAL_MODE
} e_RxPhyMode;

typedef struct {
    UINT8  u8OffsetP;
    UINT8  u8OffsetN;
} OFFSET_T;

typedef enum {
    RX_AUDIO_DLL_MODE          = 0x00, //user use
    RX_AUDIO_DLL_TO_PLL_MODE   = 0x01, //user use
    RX_AUDIO_DLL_TO_2XPLL_MODE = 0x02,
    RX_AUDIO_PLL_MODE          = 0x03,
    RX_AUDIO_PLL_FREERUN_MODE  = 0x04
} e_RxAudioClkMode;

typedef enum {
    MDT_ProgrVNegHNeg = 0x01,
    MDT_ProgrVNegHPos = 0x03,
    MDT_ProgrVPosHNeg = 0x05,
    MDT_ProgrVPosHPos = 0x07,

    MDT_InterVNegHNeg = 0x00,
    MDT_InterVNegHPos = 0x02,
    MDT_InterVPosHNeg = 0x04,
    MDT_InterVPosHPos = 0x06
} e_RxMdtPol;


typedef struct TERRORCOUNTER {
//    BOOL b;
    UINT16 u16Count;
} ERRORCOUNTER;

//hdmi_rx_reset_ctrl
#define HDMI_RX_CTRL_MAIN  (0x00001UL)
#define HDMI_RX_CTRL_MODET (0x00002UL)
#define HDMI_RX_CTRL_HDMI  (0x00004UL)
#define HDMI_RX_CTRL_BUS   (0x00008UL)
#define HDMI_RX_CTRL_AUD   (0x00010UL)
#define HDMI_RX_CTRL_PDEC  (0x00020UL) //20180607, PACKET DECODER reset
#define HDMI_RX_CTRL_PIXEL (0x00040UL)
#define HDMI_RX_CTRL_VID   (0x00080UL)
#define HDMI_RX_CTRL_HDCP  (0x00100UL)
#define HDMI_RX_CTRL_TMDS  (0x10000UL)


typedef enum _E_HDMRX_INT_INDEX_ {
    RX_INT_INDEX_PDEC,
    RX_INT_INDEX_MDT,
    RX_INT_INDEX_HDMI,
    RX_INT_INDEX_AUDFIFO,
    RX_INT_INDEX_5V
} HDMIRX_INT_INDEX_E;

typedef enum _E_MDT_INTRRUPT_STATUS_ {
    MDT_HS_ACT_ISTS         = 0x001,
    MDT_VS_ACT_ISTS         = 0x002,
    MDT_DE_ACTIVITY_ISTS    = 0x004,
    MDT_ILACE_ISTS          = 0x008,
    MDT_HTOT32_CLK_ISTS     = 0x010,
    MDT_HS_CLK_ISTS         = 0x020,
    MDT_HACT_PIX_ISTS       = 0x040,
    MDT_VTOT_CLK_ISTS       = 0x080,
    MDT_VS_CLK_ISTS         = 0x100,
    MDT_VACT_LIN_ISTS       = 0x200,
    MDT_VTOT_LIN_ISTS       = 0x400,
    MDT_VOFS_LIN_ISTS       = 0x800,
    MDT_STB_ISTS            = 0x1000,
    MDT_USTB_ISTS           = 0x2000,

    //
    MDT_ALL_ISTS            = 0xffff
} MDTISTS_E;

typedef enum _E_AUDIO_FIFO_STATUS_ {
    AFIF_TH_MIN_STS         = 0x001,
    AFIF_TH_MAX_STS         = 0x002,
    AFIF_THS_PASS_STS       = 0x004,
    AFIF_UNDERFL_STS        = 0x008,
    AFIF_OVERFL_STS         = 0x010
} AUDIO_FIFO_STATUS_E;

//hdmi_rx_packet_ists
#define PD_FIFO_TH_MIN_PASS_ISTS   (1UL << 0)
#define PD_FIFO_TH_MAX_PASS_ISTS   (1UL << 1)
#define PD_FIFO_TH_START_PASS_ISTS (1UL << 2)
#define PD_FIFO_UNDERFL_ISTS       (1UL << 3)
#define PD_FIFO_OVERFL_ISTS        (1UL << 4)
#define PD_FIFO_NEW_ENTRY_ISTS     (1UL << 8)
#define NTSCVBI_RCV_ISTS           (1UL << 11)
#define NTSCVBI_CKS_CHG_ISTS       (1UL << 12)
#define AMP_CHG_ISTS               (1UL << 13)
#define AMP_RCV_ISTS               (1UL << 14)
#define VSI_RCV_ISTS               (1UL << 15)
#define GCP_RCV_ISTS               (1UL << 16)
#define ACR_RCV_ISTS               (1UL << 17)
#define AVI_RCV_ISTS               (1UL << 18)
#define AIF_RCV_ISTS               (1UL << 19)
#define GMD_RCV_ISTS               (1UL << 20)
#define GCP_AV_MUTE_CHG_ISTS       (1UL << 21)
#define ACR_CTS_CHG_ISTS           (1UL << 22)
#define ACR_N_CHG_ISTS             (1UL << 23)
#define AVI_CKS_CHG_ISTS           (1UL << 24)
#define AIF_CKS_CHG_ISTS           (1UL << 25)
#define GMD_CKS_CHG_ISTS           (1UL << 26)
#define VSI_CKS_CHG_ISTS           (1UL << 27)
#define DVIDET_ISTS                (1UL << 28)
#define AUD_TYPE_CHG_ISTS          (1UL << 29)
#define DRM_RCV_ISTS               (1UL << 30)
#define DRM_CKS_CHG_ISTS           (1UL << 31)
#define PDEC_ALL_ISTS              (0xffffffffUL)

//hdmi_rx_hdmi_ists
#define FREQ_LOCK_ISTS                  (1UL << 4)
#define FREQ_UNLOCK_ISTS                (1UL << 5)
#define CLK_CHANGE_ISTS                 (1UL << 6)
#define HS_POL_ADJ_ISTS                 (1UL << 8)
#define VS_POL_ADJ_ISTS                 (1UL << 9)
#define CTL0_CHANGE_ISTS                (1UL << 10)
#define CTL1_CHANGE_ISTS                (1UL << 11)
#define CTL2_CHANGE_ISTS                (1UL << 12)
#define CTL3_CHANGE_ISTS                (1UL << 13)
#define DCM_GCP_ZERO_FIELDS_PASS_ISTS   (1UL << 14)
#define DCM_PH_DIFF_CNT_OVERFL_ISTS     (1UL << 15)
#define DCM_CURRENT_MODE_CHG_ISTS       (1UL << 16)
#define SCDCCFGCHANGE_ISTS              (1UL << 17)
#define SCDCSCSTATUSCHANGE_ISTS         (1UL << 18)
#define SCDCTMDSCFGCHANGE_ISTS          (1UL << 19)
#define CEAVID_FULL_ISTS                (1UL << 20)
#define CEAVID_EMPTY_ISTS               (1UL << 21)
#define AKSV_RCV_ISTS                   (1UL << 25)
#define VSYNC_ACT_EDGE_ISTS             (1UL << 26)
#define VS_THR_REACHED_ISTS             (1UL << 27)
#define I2CMPDONE_ISTS                  (1UL << 28)
#define I2CMPNACK_ISTS                  (1UL << 29)
#define I2CMP_ARBLOST_ISTS              (1UL << 30)
#define HDMI_ALL_ISTS                   (0xffffffffUL)

//RX_INT_INDEX_5V
#define RX_INT_FLAG_5V_UNPLUG       (0x04)
#define RX_INT_FLAG_5V_PLUG         (0x08)

#endif

//dvout
#ifndef MS_DVOUT_DEFINE
#define MS_DVOUT_DEFINE

typedef struct _T_DVOUT_CONFIG {
    UINT8 u8_cs_mode;  //refer to DVOUT_CS_MODE_E
    UINT8 u8_bw_mode;  //refer to DVOUT_BW_MODE_E
    UINT8 u8_dr_mode;  //refer to DVOUT_DR_MODE_E
    UINT8 u8_sy_mode;  //refer to DVOUT_SY_MODE_E
} DVOUT_CONFIG_T;

typedef enum _E_DVOUT_CS_MODE_ {
    DVOUT_CS_MODE_RGB,
    DVOUT_CS_MODE_YUV444,
    DVOUT_CS_MODE_YUV422
} DVOUT_CS_MODE_E;

typedef enum _E_DVOUT_BW_MODE_ {
    DVOUT_BW_MODE_16_24BIT,
    DVOUT_BW_MODE_8_12BIT
} DVOUT_BW_MODE_E;

typedef enum _E_DVOUT_DR_MODE_ {
    DVOUT_DR_MODE_SDR,
    DVOUT_DR_MODE_DDR
} DVOUT_DR_MODE_E;

typedef enum _E_DVOUT_SY_MODE_ {
    DVOUT_SY_MODE_HSVSDE,      // 8/16/24-bit BT601
    DVOUT_SY_MODE_EMBANDED,    // 16-bit BT1120 or 8bit BT656
    DVOUT_SY_MODE_2XEMBANDED,  // 8-bit BT1120
    DVOUT_SY_MODE_BTAT1004     // 16-bit BTA-T1004
} DVOUT_SY_MODE_E;
#endif

#endif  // __MACROSILICON_MS7200_COMMON_TYPEDEF_H__
