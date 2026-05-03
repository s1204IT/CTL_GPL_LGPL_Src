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
#ifndef _THDMIRX_PACKET_H_
#define _THDMIRX_PACKET_H_
#include "THDMIRx_System.h"
#include "include/display_datatype.h"

// HDMI data island packet and CEA 861 InfoFrame packet (binary representation)
#define HB_SIZE 3
#define PB_SIZE 28
#define PACKET_TYPE_TO_INDEX(type) ((U8)(((U8)(type) <= 0x0AU) ? (U8)(type) : 0x0BU + ((U8)((type)-1) & 0x7FU)))

#define RMEDID_IEEE_OUI_HDMI 0x000C03        // HDMI Licensing, LLC
#define RMEDID_IEEE_OUI_HDMI_FORUM 0xD85DC4  // HDMI Forum Licensing, LLC
#define RMEDID_IEEE_OUI_HDMI_DbVisionLowLatency 0x00D046  // for DbVision Low Latency mode

#define FREESYNC_ACTIVE _BIT2_
#define FREESYNC_ENABLED _BIT1_
#define FREESYNC_SUPPORTED _BIT0_
#define FREESYNC_IEEE_OUI 0x00001A

typedef struct _ThdmiRxPacket {
	U8 HeaderByte[HB_SIZE];
	U8 PacketByte[PB_SIZE];
} ThdmiRxPacket;

typedef enum _eHdmiRxITContentType {
	hdmiRx_ITContentType_Graphics = 0,
	hdmiRx_ITContentType_Photo,
	hdmiRx_ITContentType_Cinema,
	hdmiRx_ITContentType_Game,
} eHdmiRxITContentType;

typedef enum _eHdmiRxQrangeType {
	hdmiRx_Depends_On_Video_Format,
	hdmiRx_LIMITED_RANGE,
	hdmiRx_FULL_RANGE,
	hdmiRx_RESERVED,
} eHdmiRxQrangeType;

typedef enum _eHdmiRxColorFmt {
	hdmiRx_ColorFmt_RESERVED = 0,
	hdmiRx_ColorFmt_RGB,
	hdmiRx_ColorFmt_YUV422,
	hdmiRx_ColorFmt_YUV444,
	hdmiRx_ColorFmt_YUV420,
} eHdmiRxColorFmt;

typedef enum _eHdmiRxEOTF {
	hdmiRx_EOTF_SDR = 0,
	hdmiRx_EOTF_HDR,
	hdmiRx_EOTF_SMPTE_ST,
	hdmiRx_EOTF_HLG,
	hdmiRx_EOTF_RESERVED,
} eHdmiRxEOTF;

/** Source device type */
typedef enum _HdmiRxSrcDevInfo {
	hdmiRx_SourceDevice_unknown = 0,
	hdmiRx_SourceDevice_DigitalSTB,
	hdmiRx_SourceDevice_DVD,
	hdmiRx_SourceDevice_DVHS,
	hdmiRx_SourceDevice_HDDVideoRec,
	hdmiRx_SourceDevice_DVC,
	hdmiRx_SourceDevice_DSC,
	hdmiRx_SourceDevice_VideoCD,
	hdmiRx_SourceDevice_Game,
	hdmiRx_SourceDevice_PC,
	hdmiRx_SourceDevice_BluRay,
	hdmiRx_SourceDevice_SACD,
	hdmiRx_SourceDevice_HDDVD,
	hdmiRx_SourceDevice_PMP,
} eHdmiRxSrcDevInfo;

typedef enum _eHdmiRxColorMatrix {
	hdmiRx_ColorMatrix_NotIndicated = 0,
	hdmiRx_ColorMatrix_BT601,  // Bt.470-6, Bt.601-6, Bt.1358, Bt.1700, IEC61966-2-4 601 ( 0.299 ; 0.114 )
	hdmiRx_ColorMatrix_BT709,  // Bt.709-5, Bt.1361, IEC61966-2-4 709, RP177 ( 0.2126 ; 0.0722 )
	hdmiRx_ColorMatrix_BT2020_C,
	hdmiRx_ColorMatrix_BT2020_NC,
} eHdmiRxColorMatrix;

typedef struct _ThdmiRxColorSpace {
	eHdmiRxColorFmt Model;
	eHdmiRxQrangeType Range;
	eHdmiRxColorMatrix ColorMatrix;
} ThdmiRxColorSpace;

/** All information for the Auxiliary Video Information InfoFrame */
typedef struct _ThdmiRxAVIInfoFrame {
	U8 Version;      ///< Set to 0x02 until further notice
	U32 VIC;         ///< EIA/CEA-861 Video ID Code (0 if not HDMI_xxx)
	bool ITContent;  ///< TRUE=Request display to do one-on-one pixel mapping
	U8 PixelRepetition;
	U8 AspectCode;
	U8 AFDStyle;

	ThdmiRxColorSpace ColorSpace;
	eHdmiRxITContentType ITContentType;  ///< content type, if ITContent is TRUE
} ThdmiRxAVIInfoFrame;

typedef struct _ThdmiRxVSIInfoFrame {
	U32 HDMI_VIC;
	U32 HDMI_Video_Format;

	// b3DOn-- redundance info, just for convenience
	// if video_format is 2 that means b3DOn is on.
	U8 b3DOn;
	// e_3D_Struct _3D_Struct;
	// e_3D_Ext_Data _3D_ExtData;
	bool DbVision_LowLatency;

} ThdmiRxVSIInfoFrame;

struct ThdmiRxDisplayPrimaries {
	U16 x;
	U16 y;
};

typedef struct _ThdmiRxFreeSyncSPDI {
	bool bFreeSyncSupported;
	bool bFreeSyncEnabled;
	bool bFreeSyncActive;
	U8 min_freq;
	U8 max_freq;
} ThdmiRxFreeSyncSPDI;

typedef struct _ThdmiRx_HDRInfoFrame {
	U8 EOTF;
	U8 MetaDataDescriptorID;
	struct ThdmiRxDisplayPrimaries primaries[3];
	U16 WhitePointX;
	U16 WhitePointY;
	U16 MaxDisplayMasteringLum;
	U16 MinDisplayMasteringLum;
	U16 MaxContentLightLevel;
	U16 MaxFrameAverageLightLevel;
} ThdmiRxHDRInfoFrame;
#define HDR_INFOFRAME_LEN 26

// added for HDR10+
#define HDR10_PLUS_VSI_LEN 27
#define HDR10_PLUS_IEEE_OUI 0x90848B
#define DISTRIBUTION_VALUES_CNT 9
#define BEZIER_CURVE_ANCHORS_CNT 9

#define TSDML_MASK (0x3E)
#define NBCA_MASK (0xF0)
#define KPX_HI_MASK (0x0F)
#define KPX_LO_MASK (0xFC)

typedef struct _ThdmiRx_HDR10PlusInfoFrame {
	bool bValid;
	U8 application_version;
	U8 targeted_system_display_maximum_luminance;
	U8 average_maxrgb;
	U8 distribution_values[DISTRIBUTION_VALUES_CNT];
	U8 num_bezier_curve_anchors;
	U16 knee_point_x;
	U16 knee_point_y;
	U8 bezier_curve_anchors[BEZIER_CURVE_ANCHORS_CNT];
	U8 graphics_overlay_flag;
	U8 no_delay_flag;
} ThdmiRx_HDR10PlusInfoFrame;

typedef struct _ThdmiRxACRInfoFrame {
	// Don't read ACR packet field
	// we could directly read the register
	// Once hw don't provide the register read, we could to parse pkt.
	// Just for compartiable
	U32 N;
	U32 CTS;
} ThdmiRxACRFrame;

typedef struct _ThdmiRxGCPFrame {
	// don't use set_avmute & clear_avmute
	// we could directly read the register
	// Once hw don't provide the register read, we could to parse pkt.
	// Just for compartiable
	bool set_avmute;
	bool clear_avmute;
} ThdmiRxGCPFrame;
/**
	Packet type for HeaderByte[0]
	HDMI, LLC. and HDMI Forum: 0x00..0x7F ("Packet")
	CEA: 0x80..0xFF ("InfoFrame", implies checksum in PacketByte[0])
*/
typedef enum _eHdmiRxPacketType {
	// HDMI Packets
	ThdmiRxPacketType_Null = 0x00,
	ThdmiRxPacketType_ACR = 0x01,
	ThdmiRxPacketType_AudioSample = 0x02,
	ThdmiRxPacketType_GCP = 0x03,
	ThdmiRxPacketType_ACP = 0x04,
	ThdmiRxPacketType_ISRC1 = 0x05,
	ThdmiRxPacketType_ISRC2 = 0x06,
	ThdmiRxPacketType_OneBitAudio = 0x07,
	ThdmiRxPacketType_DSTAudio = 0x08,
	ThdmiRxPacketType_HBRAudio = 0x09,
	ThdmiRxPacketType_GamutMetadata = 0x0A,

	// CEA 861 InfoFrames
	ThdmiRxPacketType_VSI = 0x81,   // VSI
	ThdmiRxPacketType_AVI = 0x82,   // AVI
	ThdmiRxPacketType_SPDI = 0x83,  // SPDI
	ThdmiRxPacketType_AI = 0x84,    // AudioInfo
	ThdmiRxPacketType_MPEG = 0x85,  // MPEG
	ThdmiRxPacketType_VBI = 0x86,   // VBI
	ThdmiRxPacketType_HDR = 0x87,   // HDR
} eHdmiRxPacketType;

#define PACKET_CNT 32

typedef struct _ThdmiRxPacketContext {
	ThdmiRxPacket packet[PACKET_CNT];
	ThdmiRxAVIInfoFrame tAVIInfoFrame;
	ThdmiRxVSIInfoFrame tVSIInfoFrame;
	ThdmiRxACRFrame tACRFrame;
	ThdmiRxGCPFrame tGCPFrame;
	ThdmiRxHDRInfoFrame tHDRFrame;
	ThdmiRxFreeSyncSPDI tFreeSyncSPDI;

	ThdmiRx_HDR10PlusInfoFrame tHDR10PlusFrame;
} ThdmiRxPacketContext;

struct HDMIRx_PacketContext {
	ThdmiRxPacketContext ctx;
};
bool HdmiRxPacket_AVI_Handle(ThdmiRxPacketContext *pPacket);

bool HdmiRxPacket_GetPixelRepetition(ThdmiRxPacketContext *pCtx);

U16 HdmiRxPacket_GetColorFormat(ThdmiRxPacketContext *pCtx, U16 CurVideoMode);

eHdmiRxEOTF HdmiRxPacket_GetHdrEOTF(void);

bool HdmiRxPacket_GetHdr10Plus_Status(struct HDMIRx_PacketContext PacketCtx);

bool HdmiRxPacket_AI_Handle(ThdmiRxPacketContext *pPacket);

bool HdmiRxPacket_SPDI_Handle(ThdmiRxPacketContext *pPacket);

bool HdmiRxPacket_VSI_LLC_Handle(ThdmiRxPacketContext *pPacket);

bool HdmiRxPacket_GCP_Hanlde(ThdmiRxPacketContext *pPacket);

bool HdmiRxPacket_Parse_SPDI(const ThdmiRxPacket *pPacket);

bool HdmiRxPacket_Parse_SYNCSPDI(const ThdmiRxPacket *pPacket, ThdmiRxFreeSyncSPDI *pFreeSyncInfo);

bool HdmiRxPacket_Parse_HDR(const ThdmiRxPacket *pPacket, ThdmiRxHDRInfoFrame *pHDRInfoFrame);

bool HdmiRxPacket_Parse_VSI_HDR10_Plus(const ThdmiRxPacket *pPacket, ThdmiRx_HDR10PlusInfoFrame *pHdr10PlusInfo);

bool HdmiRxPacket_Parse_VSI_LLC(const ThdmiRxPacket *pPacket, ThdmiRxVSIInfoFrame *pVSIInfoFrame);

eHdmiRxColorMatrix HdmiRxPacket_GetColorMatrix(struct HDMIRx_PacketContext PacketCtx);

#endif /* _THDMIRX_PACKET_H_ */
