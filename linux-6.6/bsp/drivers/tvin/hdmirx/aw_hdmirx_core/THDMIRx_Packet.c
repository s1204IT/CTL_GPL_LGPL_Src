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
#include "../aw_hdmirx_define.h"
#include "THDMIRx_PortCtx.h"
#include "THDMIRx_Packet.h"
#include "hardware/THDMIRx_TV_Driver.h"

// calculate CEA861 checksum and compare against DataByte[0]. Data size in HeaderByte[2]
// over all packet header + payload sum should be 0.
// 0 is correct
bool HdmiRxPacket_Check_InfoFrame_Checksum(const ThdmiRxPacket *pPacket)
{
	unsigned int i, checksum = 0;

	for (i = 0; i < 3; i++) {
		checksum += pPacket->HeaderByte[i];
	}
	for (i = 0; i <= (unsigned int)(pPacket->HeaderByte[2] & 0x1F); i++) {
		checksum += pPacket->PacketByte[i];
	}
	return ((checksum & 0xFF) == 0) ? true : false;
}

bool HdmiRxPacket_Parse_AVI(const ThdmiRxPacket *pPacket, ThdmiRxAVIInfoFrame *pAVIInfoFrame)
{
	U32 Type, Version, Length, S, R, Y, A, M, C, Q, EC, VIC, PR, CN, YQ, Range, Top, Bottom, Left, Right;
	bool B0, B1, SC0, SC1, ITC;

	CHECK_POINTER(pPacket, false);
	CHECK_POINTER(pAVIInfoFrame, false);

	// Parse header
	Type = pPacket->HeaderByte[0];

	if ((eHdmiRxPacketType)Type != ThdmiRxPacketType_AVI) {
		hdmirx_err("Not an AVIInfoFrame!\n");
		return false;
	}

	Version = pPacket->HeaderByte[1];
	Length = pPacket->HeaderByte[2] & 0x1F;

	if (Length < 13) {
		hdmirx_err("AVIInfoFrame Length is Wrong!\n");
		return false;
	}

	// Parse fields
	Y = (pPacket->PacketByte[1] >> 5) & 0x07;
	A = (pPacket->PacketByte[1] & 0x10) ? true : false;
	R = (pPacket->PacketByte[2] & 0x0F);
	B0 = (pPacket->PacketByte[1] & 0x04) ? true : false;
	B1 = (pPacket->PacketByte[1] & 0x08) ? true : false;
	S = pPacket->PacketByte[1] & 0x03;
	C = (pPacket->PacketByte[2] >> 6) & 0x03;
	M = (pPacket->PacketByte[2] >> 4) & 0x03;
	SC0 = (pPacket->PacketByte[3] & 0x01) ? true : false;
	SC1 = (pPacket->PacketByte[3] & 0x02) ? true : false;

	if (Version >= 2) {
		Q = (pPacket->PacketByte[3] >> 2) & 0x03;
		EC = (pPacket->PacketByte[3] >> 4) & 0x07;
		ITC = (pPacket->PacketByte[3] & 0x80) ? true : false;
		VIC = pPacket->PacketByte[4] & 0xFF;
		PR = pPacket->PacketByte[5] & 0x0F;
		CN = (pPacket->PacketByte[5] >> 4) & 0x03;
		YQ = (pPacket->PacketByte[5] >> 6) & 0x03;
	} else {
		Q = EC = ITC = VIC = PR = CN = YQ = 0;
	}

	Top = (pPacket->PacketByte[7] << 8) | pPacket->PacketByte[6];
	Bottom = (pPacket->PacketByte[9] << 8) | pPacket->PacketByte[8];
	Left = (pPacket->PacketByte[11] << 8) | pPacket->PacketByte[10];
	Right = (pPacket->PacketByte[13] << 8) | pPacket->PacketByte[12];

	// Debug info
	PACK_INF("	  +-----------------------------------------------+");
	PACK_INF("	| 2016 AVI Info Frame, Type 0x%02X Version 0x%02X Len %2u |", Type, Version, Length);
	PACK_INF("	  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
	PACK_INF("	 |	 Y=%lu (%s) Q=%x\n", Y,
		(Y == 0) ? " RGB " : (Y == 1) ? "4:2:2" : (Y == 2) ? "4:4:4" : (Y == 3) ? "4:2:0"
																					: (Y == 6) ? " IDO "
																							:  // Further defined by interface organisation
																						"invld",
		Q);
	PACK_INF("	  +-----+-----+-----+-----+-----+-----+-----+-----+\n");

	// Convert fields to Sigma Designs types
	if (pAVIInfoFrame != NULL) {
		memset(pAVIInfoFrame, 0, sizeof(ThdmiRxAVIInfoFrame));
		pAVIInfoFrame->Version = Version;
		pAVIInfoFrame->VIC = VIC;
		pAVIInfoFrame->PixelRepetition = PR;
		pAVIInfoFrame->AFDStyle = (pPacket->PacketByte[2] & 0x0F);

		switch (M) {
		case 1:
			pAVIInfoFrame->AspectCode = kVideoAspectCode_4_3;
			break;
		case 2:
			pAVIInfoFrame->AspectCode = kVideoAspectCode_16_9;
			break;
		default:
			pAVIInfoFrame->AspectCode = kVideoAspectCode_4_3;
			break;
		}

		// Color Space
		pAVIInfoFrame->ColorSpace.Model = hdmiRx_ColorFmt_RESERVED;
		pAVIInfoFrame->ColorSpace.Range = hdmiRx_RESERVED;
		pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_NotIndicated;

		switch (C) {
		case 1:
			pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_BT601;
			break;
		case 2:
			pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_BT709;
			break;
		case 3:
			switch (EC) {
			case 1:
				pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_BT709;
				break;
			case 0:
			case 2:
			case 3:
				pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_BT601;
				break;
			case 5:
				pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_BT2020_C;
				break;
			case 6:
				pAVIInfoFrame->ColorSpace.ColorMatrix = hdmiRx_ColorMatrix_BT2020_NC;
				break;
			default:
				break;
			}
		default:
			break;
		}
		PACK_INF("AVI Packet: C=0x%x EC=0x%x\n", C, EC);

		// check RGB/YUV Q range
		if (Y == 0) {  // RGB
			Range = Q;
		} else {  // YUV
			Range = YQ + 1;
		}

		switch (Range) {
		case 0:
			pAVIInfoFrame->ColorSpace.Range = hdmiRx_Depends_On_Video_Format;
			break;
		case 1:
			pAVIInfoFrame->ColorSpace.Range = hdmiRx_LIMITED_RANGE;
			break;
		case 2:
			pAVIInfoFrame->ColorSpace.Range = hdmiRx_FULL_RANGE;
			break;
		default:
			pAVIInfoFrame->ColorSpace.Range = hdmiRx_RESERVED;
			break;
		}
		switch (Y) {
		case 0:
			pAVIInfoFrame->ColorSpace.Model = hdmiRx_ColorFmt_RGB;
			break;
		case 3:
			pAVIInfoFrame->ColorSpace.Model = hdmiRx_ColorFmt_YUV420;
			break;
		case 1:
			pAVIInfoFrame->ColorSpace.Model = hdmiRx_ColorFmt_YUV422;
			break;
		case 2:
			pAVIInfoFrame->ColorSpace.Model = hdmiRx_ColorFmt_YUV444;
			break;
		}

		pAVIInfoFrame->ITContent = ITC;
		pAVIInfoFrame->ITContentType = (eHdmiRxITContentType)CN;
	}

	return HdmiRxPacket_Check_InfoFrame_Checksum(pPacket);
}

bool HdmiRxPacket_AVI_Handle(ThdmiRxPacketContext *pPacket)
{
	U8 index = 0;
	bool ret = false;
	index = PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_AVI);
	PACK_INF("index = %d\n", index);
	ret = HdmiRx_Packet_Read(ThdmiRxPacketType_AVI, &pPacket->packet[index]);
	if (ret) {
		ThdmiRxAVIInfoFrame CurAVIInfoFrame;
		if (HdmiRxPacket_Parse_AVI(&pPacket->packet[index], &CurAVIInfoFrame)) {
			if (memcmp(&CurAVIInfoFrame, &pPacket->tAVIInfoFrame, sizeof(ThdmiRxAVIInfoFrame)) != 0) {
				pPacket->tAVIInfoFrame = CurAVIInfoFrame;  // no equal, assign new value
			}
		} else
		ret = false;
	}

	return ret;
}

bool HdmiRxPacket_GetPixelRepetition(ThdmiRxPacketContext *pCtx)
{
	if (pCtx->tAVIInfoFrame.PixelRepetition) {
		return true;
	}
	return false;
}

U16 HdmiRxPacket_GetColorFormat(ThdmiRxPacketContext *pCtx, U16 CurVideoMode)
{
	U16 fmt = HDMI_IN_RGB444;

	switch (pCtx->tAVIInfoFrame.ColorSpace.Model) {
	case hdmiRx_ColorFmt_YUV422:
		fmt = HDMI_IN_YUV422;
		break;
	case hdmiRx_ColorFmt_YUV444:
		fmt = HDMI_IN_YUV444;
		break;
	case hdmiRx_ColorFmt_YUV420:
		fmt = HDMI_IN_YUV420;
		break;
	case hdmiRx_ColorFmt_RGB:
	default:
		if ((VideoMode_e)CurVideoMode != VIDEO_MODE_DVI) {
			fmt = HDMI_IN_RGB444;
		} else {
			fmt = DVI_IN_RGB444;
		}
		break;
	}
	return fmt;
}

bool HdmiRxPacket_GetHdr10Plus_Status(struct HDMIRx_PacketContext PacketCtx)
{
	return PacketCtx.ctx.tHDR10PlusFrame.bValid;
}

bool HdmiRxPacket_GetHdrInfoFrame(U32 base, U8 *pHdrInfo)
{
	ThdmiRxPacket packet;
	ThdmiRxHDRInfoFrame HDRFrame;

	if (true == HdmiRx_Packet_ReadGen1(ThdmiRxPacketType_HDR, &packet)) {
		if (HdmiRxPacket_Parse_HDR(&packet, &HDRFrame)) {
			if (pHdrInfo) {
				memcpy(pHdrInfo, &HDRFrame, HDR_INFOFRAME_LEN);
			}
			return true;
		}
	}
	return false;
}

eHdmiRxEOTF HdmiRxPacket_GetHdrEOTF(void)
{
	ThdmiRxPacket packet;
	ThdmiRxHDRInfoFrame HDRFrame;

	if (true == HdmiRx_Packet_ReadGen1(ThdmiRxPacketType_HDR, &packet)) {
		if (HdmiRxPacket_Parse_HDR(&packet, &HDRFrame)) {
			return (eHdmiRxEOTF)HDRFrame.EOTF;
		}
	}
	return hdmiRx_EOTF_SDR;
}

eHdmiRxColorMatrix HdmiRxPacket_GetColorMatrix(struct HDMIRx_PacketContext PacketCtx)
{
	return PacketCtx.ctx.tAVIInfoFrame.ColorSpace.ColorMatrix;
}

// packet parse
bool HdmiRxPacket_GCP_Hanlde(ThdmiRxPacketContext *pPacket)
{
	// Don't use set_avmute & clear_avmute
	// we could directly read the register
	// Once hw don't provide the register read, we could parse pkt.
	U8 index = 0;
	bool ret = false;
	U32 Type;
	ThdmiRxGCPFrame CurGCPFrame;
	// Check target pointer
	if (pPacket == NULL)
		return false;
	index = PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_GCP);
	ret = HdmiRx_Packet_Read(ThdmiRxPacketType_GCP, &pPacket->packet[index]);
	if (ret) {
		// Parse header
		Type = pPacket->packet[index].HeaderByte[0];
		if ((eHdmiRxPacketType)Type != ThdmiRxPacketType_GCP)
			return false;
		// get gcp info from packet data
		// now not used
		//---------------------------
		if (memcmp(&CurGCPFrame, &pPacket->tGCPFrame, sizeof(ThdmiRxGCPFrame)) != 0) {
			pPacket->tGCPFrame = CurGCPFrame;  // no equal, assign new value
		}
	}

	return ret;
}


bool HdmiRxPacket_VSI_LLC_Handle(ThdmiRxPacketContext *pPacket)
{
	U8 index = 0;
	bool ret = false;
	index = PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_VSI);
	ret = HdmiRx_Packet_ReadGen2(ThdmiRxPacketType_VSI, &pPacket->packet[index]);
	if (ret) {
		ThdmiRxVSIInfoFrame CurVSIInfoFrame;
		// patch added for HDR10+
		if (true != HdmiRxPacket_Parse_VSI_HDR10_Plus(&pPacket->packet[index], &pPacket->tHDR10PlusFrame)) {
			PACK_INF("%s: Not HDR10 Plus!!\n", __func__);
			pPacket->tHDR10PlusFrame.bValid = false;
			if (HdmiRxPacket_Parse_VSI_LLC(&pPacket->packet[index], &CurVSIInfoFrame)) {
				if (memcmp(&CurVSIInfoFrame, &pPacket->tVSIInfoFrame, sizeof(ThdmiRxVSIInfoFrame)) != 0) {
					pPacket->tVSIInfoFrame = CurVSIInfoFrame;  // no equal, assign new value
				}
			}
		}
	}

	return ret;
}

bool HdmiRxPacket_AI_Handle(ThdmiRxPacketContext *pPacket)
{
	U8 index = 0;
	bool ret = false;
	U32 Type;
	// Check target pointer
	if (pPacket == NULL)
		return false;
	index = PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_AI);
	ret = HdmiRx_Packet_Read(ThdmiRxPacketType_AI, &pPacket->packet[index]);
	if (ret) {
		// Parse header
		Type = pPacket->packet[index].HeaderByte[0];
		if ((eHdmiRxPacketType)Type != ThdmiRxPacketType_AI)
			return false;
	}

	return ret;
}

bool HdmiRxPacket_SPDI_Handle(ThdmiRxPacketContext *pPacket)
{
	U8 index = 0;
	bool ret = false;
	index = PACKET_TYPE_TO_INDEX(ThdmiRxPacketType_SPDI);
	ret = HdmiRx_Packet_Read(ThdmiRxPacketType_SPDI, &pPacket->packet[index]);
	if (ret) {
		U32 Length = pPacket->packet[index].HeaderByte[2] & 0x1F;
		if (Length == 8) {
			return HdmiRxPacket_Parse_SYNCSPDI(&pPacket->packet[index], &(pPacket->tFreeSyncSPDI));
		} else {
			return HdmiRxPacket_Parse_SPDI(&pPacket->packet[index]);
		}
	}

	return ret;
}

bool HdmiRxPacket_Parse_SYNCSPDI(const ThdmiRxPacket *pPacket, ThdmiRxFreeSyncSPDI *pFreeSyncInfo)
{
	U32 Type, Version, Length;
	U32 IEEE_OUI = 0;

	CHECK_POINTER(pPacket, false);
	CHECK_POINTER(pFreeSyncInfo, false);

	// Parse header
	Type = pPacket->HeaderByte[0];
	if ((eHdmiRxPacketType)Type != ThdmiRxPacketType_SPDI) {
		PACK_INF("%s: Not an SPDInfoFrame!\n", __func__);
		return false;
	}

	// check version
	Version = pPacket->HeaderByte[1];
	if (Version != 1) {
		PACK_INF("%s: Not an AMD VRR SPDInfoFrame!\n", __func__);
		return false;
	}

	// check length
	Length = pPacket->HeaderByte[2] & 0x1F;
	if (Length != 8) {
		PACK_INF("%s: Not an AMD VRR SPDInfoFrame!\n", __func__);
		return false;
	}

	// check OUI
	IEEE_OUI = (pPacket->PacketByte[3] << 16) | (pPacket->PacketByte[2] << 8) | pPacket->PacketByte[1];
	if (IEEE_OUI != FREESYNC_IEEE_OUI) {
		PACK_INF("%s: Not an AMD VRR SPDInfoFrame!\n", __func__);
		return false;
	}

	pFreeSyncInfo->bFreeSyncSupported = pPacket->PacketByte[6] & FREESYNC_SUPPORTED ? true : false;
	pFreeSyncInfo->bFreeSyncActive = pPacket->PacketByte[6] & FREESYNC_ACTIVE ? true : false;
	pFreeSyncInfo->bFreeSyncEnabled = pPacket->PacketByte[6] & FREESYNC_ENABLED ? true : false;

	pFreeSyncInfo->min_freq = pPacket->PacketByte[7];
	pFreeSyncInfo->max_freq = pPacket->PacketByte[8];

	return HdmiRxPacket_Check_InfoFrame_Checksum(pPacket);
}

bool HdmiRxPacket_Parse_SPDI(const ThdmiRxPacket *pPacket)
{
	U32 Type, Version, Length, i;
	eHdmiRxSrcDevInfo SourceDeviceInfo;
	U8 Vendor[9], Product[17];
	CHECK_POINTER(pPacket, false);

	// Parse header
	Type = pPacket->HeaderByte[0];

	if ((eHdmiRxPacketType)Type != ThdmiRxPacketType_SPDI) {
		PACK_INF("%s: Not an SPDInfoFrame!\n", __func__);
		return false;
	}

	Version = pPacket->HeaderByte[1];
	Length = pPacket->HeaderByte[2] & 0x1F;

	if (Length < 25) {
		return false;
	}

	// Parse fields
	for (i = 0; i < 8; i++)
		Vendor[i] = pPacket->PacketByte[i + 1];

	Vendor[8] = '\0';

	for (i = 0; i < 16; i++)
		Product[i] = pPacket->PacketByte[i + 9];

	Product[16] = '\0';
	SourceDeviceInfo = (eHdmiRxSrcDevInfo)pPacket->PacketByte[25];

	// Debug info
	PACK_INF("%s: +-----------------------------------------------+\n", __func__);
	PACK_INF("    | SPD Info Frame, Type 0x%02X Version 0x%02X Len %2u |\n", Type, Version, Length);
	PACK_INF("	 +-----------------------------------------------+\n");
	PACK_INF("	 | %-8s | %-16s | %-15s |\n", Vendor, Product,
		(SourceDeviceInfo == hdmiRx_SourceDevice_unknown)
			? "unknown"
			: (SourceDeviceInfo == hdmiRx_SourceDevice_DigitalSTB)
					? "Digital STB"
					: (SourceDeviceInfo == hdmiRx_SourceDevice_DVD)
						? "DVD"
						: (SourceDeviceInfo == hdmiRx_SourceDevice_DVHS)
							? "D-VHS"
							: (SourceDeviceInfo == hdmiRx_SourceDevice_HDDVideoRec)
								? "HDD VideoRec"
								: (SourceDeviceInfo == hdmiRx_SourceDevice_DVC)
									? "DVC"
									: (SourceDeviceInfo == hdmiRx_SourceDevice_DSC)
										? "DSC"
										: (SourceDeviceInfo == hdmiRx_SourceDevice_VideoCD)
											? "Video CD"
											: (SourceDeviceInfo == hdmiRx_SourceDevice_Game)
												? "Game"
												: (SourceDeviceInfo == hdmiRx_SourceDevice_PC)
													? "PC"
													: (SourceDeviceInfo == hdmiRx_SourceDevice_BluRay)
														? "Blu-Ray"
														: (SourceDeviceInfo == hdmiRx_SourceDevice_SACD)
															? "SACD"
															: (SourceDeviceInfo == hdmiRx_SourceDevice_HDDVD)
																? "HD-DVD"
																: (SourceDeviceInfo == hdmiRx_SourceDevice_PMP)
																	? "PMP"
																	: "reserved");
	PACK_INF("	 +-----------------------------------------------+\n");

	return HdmiRxPacket_Check_InfoFrame_Checksum(pPacket);
}

bool HdmiRxPacket_Parse_HDR(const ThdmiRxPacket *pPacket, ThdmiRxHDRInfoFrame *pHDRInfoFrame)
{
	U32 Type, Version, Length;

	CHECK_POINTER(pPacket, false);
	CHECK_POINTER(pHDRInfoFrame, false);

	// Parse header
	Type = pPacket->HeaderByte[0];

	if (Type != (U8)ThdmiRxPacketType_HDR) {
		// log_i("Not an HDRInfoFrame!\n");
		return false;
	}

	Version = pPacket->HeaderByte[1];
	Length = pPacket->HeaderByte[2] & 0x1F;

	if (Length < 26) {
		return false;
	}

	// Convert fields to Sigma Designs types
	memset(pHDRInfoFrame, 0, sizeof(ThdmiRxHDRInfoFrame));
	memcpy(pHDRInfoFrame, &pPacket->PacketByte[1], Length);
	if ((pHDRInfoFrame->EOTF != (U8)hdmiRx_EOTF_SMPTE_ST) && (pHDRInfoFrame->EOTF != (U8)hdmiRx_EOTF_HLG)) {
		return false;
	}

/*
	// Debug info
	hdmirx_inf("    +-----------------------------------------------+");
	hdmirx_inf("	 | HDR Info Frame, Type 0x%02X Version 0x%02X Len %2u |", Type, Version, Length);
	hdmirx_inf("    +-----+-----+-----+-----+-----+-----+-----+-----+\n");
	hdmirx_inf("	 |	EOTF (%x)  Metadata Descriptor(%x)\n", pHDRInfoFrame->EOTF, pHDRInfoFrame->MetaDataDescriptorID);
	hdmirx_inf("	 |	display_primaires_x[0]=(%x,%x)\n", pHDRInfoFrame->primaries[0].x, pHDRInfoFrame->primaries[0].y);
	hdmirx_inf("	 |	display_primaires_x[1]=(%x,%x)\n", pHDRInfoFrame->primaries[1].x, pHDRInfoFrame->primaries[1].y);
	hdmirx_inf("	 |	display_primaires_x[2]=(%x,%x)\n", pHDRInfoFrame->primaries[2].x, pHDRInfoFrame->primaries[2].y);
	hdmirx_inf("	 |	white point=(%x,%x)\n", pHDRInfoFrame->WhitePointX, pHDRInfoFrame->WhitePointY);
	hdmirx_inf("	 |	max_display_mastering_luminance=(%x) min_display_mastering_luminance=(%x)\n", pHDRInfoFrame->MaxDisplayMasteringLum, pHDRInfoFrame->MinDisplayMasteringLum);
	hdmirx_inf("	 |	MaxCLL=(%x) MAXFALL=(%x)\n", pHDRInfoFrame->MaxContentLightLevel, pHDRInfoFrame->MaxFrameAverageLightLevel);
	hdmirx_inf("    +-----+-----+-----+-----+-----+-----+-----+-----+\n");
*/

	return HdmiRxPacket_Check_InfoFrame_Checksum(pPacket);
}

bool HdmiRxPacket_Parse_VSI_HDR10_Plus(const ThdmiRxPacket *pPacket, ThdmiRx_HDR10PlusInfoFrame *pHdr10PlusInfo)
{
	U8 i = 0;

	// check Type
	if ((eHdmiRxPacketType)pPacket->HeaderByte[0] != ThdmiRxPacketType_VSI) {
		PACK_INF("pPacket->HeaderByte[0] = 0x%x\n", pPacket->HeaderByte[0]);
		return false;
	}

	// check Sum
	if (!HdmiRxPacket_Check_InfoFrame_Checksum(pPacket)) {
		PACK_INF("%s: Check sum error!\n", __func__);
		return false;
	}

	// check Version
	if (pPacket->HeaderByte[1] != 1) {
		PACK_INF("%s: pPacket->HeaderByte[1] = 0x%x!\n", __func__, pPacket->HeaderByte[1]);
		return false;
	}

	// check Length
	if ((U8)(pPacket->HeaderByte[2] & 0x1F) != HDR10_PLUS_VSI_LEN) {
		PACK_INF("%s: pPacket->HeaderByte[2] = 0x%x!\n", __func__, pPacket->HeaderByte[2]);
		return false;
	}

	// check IEEE
	if (HDR10_PLUS_IEEE_OUI != ((pPacket->PacketByte[3] << 16) | (pPacket->PacketByte[2] << 8) | (pPacket->PacketByte[1]))) {
		PACK_INF("%s: IEEE = 0x%x\n", __func__, ((pPacket->PacketByte[3] << 16) | (pPacket->PacketByte[2] << 8) | (pPacket->PacketByte[1])));
		return false;
	}

	pHdr10PlusInfo->application_version = (pPacket->PacketByte[4] & (_BIT7_ | _BIT6_)) >> 6;
	pHdr10PlusInfo->targeted_system_display_maximum_luminance = (pPacket->PacketByte[4] & TSDML_MASK) >> 1;
	pHdr10PlusInfo->average_maxrgb = pPacket->PacketByte[5];
	PACK_INF("%s: pHdr10PlusInfo->application_version = 0x%x\n", __func__, pHdr10PlusInfo->application_version);
	PACK_INF("%s: pHdr10PlusInfo->targeted_system_display_maximum_luminance = 0x%x\n", __func__, pHdr10PlusInfo->targeted_system_display_maximum_luminance);
	PACK_INF("%s: pHdr10PlusInfo->average_maxrgb = 0x%x\n", __func__, pHdr10PlusInfo->average_maxrgb);

	for (i = 0; i < DISTRIBUTION_VALUES_CNT; i++) {
		pHdr10PlusInfo->distribution_values[i] = pPacket->PacketByte[6 + i];
		PACK_INF("%s: pHdr10PlusInfo->distribution_values[%d] = %d\n", __func__, i, pHdr10PlusInfo->distribution_values[i]);
	}

	pHdr10PlusInfo->num_bezier_curve_anchors = (pPacket->PacketByte[15] & NBCA_MASK) >> 4;

	pHdr10PlusInfo->knee_point_x = ((pPacket->PacketByte[15] & KPX_HI_MASK) << 6) | ((pPacket->PacketByte[16] & KPX_LO_MASK) >> 2);

	pHdr10PlusInfo->knee_point_y = ((pPacket->PacketByte[16] & (_BIT1_ | _BIT0_)) << 8) | (pPacket->PacketByte[17]);
	PACK_INF("%s: pHdr10PlusInfo->num_bezier_curve_anchors = %d\n", __func__, pHdr10PlusInfo->num_bezier_curve_anchors);
	PACK_INF("%s: pHdr10PlusInfo->knee_point_x = 0x%x\n", __func__, pHdr10PlusInfo->knee_point_x);
	PACK_INF("%s: pHdr10PlusInfo->knee_point_y = 0x%x\n", __func__, pHdr10PlusInfo->knee_point_y);

	if (pHdr10PlusInfo->num_bezier_curve_anchors > DISTRIBUTION_VALUES_CNT) {
		pHdr10PlusInfo->num_bezier_curve_anchors = DISTRIBUTION_VALUES_CNT;
		PACK_INF("%s: Warning:invalid num_bezier_curve_anchors(%d)\n", __func__, pHdr10PlusInfo->num_bezier_curve_anchors);
	}

	for (i = 0; i < pHdr10PlusInfo->num_bezier_curve_anchors; i++) {
		pHdr10PlusInfo->bezier_curve_anchors[i] = pPacket->PacketByte[18 + i];
		PACK_INF("%s: pHdr10PlusInfo->bezier_curve_anchors[%d] = 0x%x\n", __func__, i, pHdr10PlusInfo->bezier_curve_anchors[i]);
	}

	pHdr10PlusInfo->graphics_overlay_flag = (pPacket->PacketByte[27] & _BIT6_) >> 7;
	pHdr10PlusInfo->no_delay_flag = (pPacket->PacketByte[27] & _BIT7_) >> 6;
	PACK_INF("%s: pHdr10PlusInfo->graphics_overlay_flag = %d\n", __func__, pHdr10PlusInfo->graphics_overlay_flag);
	PACK_INF("%s: pHdr10PlusInfo->no_delay_flag = %d\n", __func__, pHdr10PlusInfo->no_delay_flag);

	pHdr10PlusInfo->bValid = true;
	return true;
}

bool HdmiRxPacket_Parse_VSI_LLC(const ThdmiRxPacket *pPacket, ThdmiRxVSIInfoFrame *pVSIInfoFrame)
{
// HDMI_Video_Format = 1: HDMI extended resolution format follows
#define HDMI_VSI_EXT_FORMAT 1
// HDMI_Video_Format = 2: HDMI 3D format structure follows
#define HDMI_VSI_3D_FORMAT 2
#define HDMI_VSI_DOBLY_LOWLATENCY_FORMAT 0x03
#define HDMI_VSI_DOBLY_LOWLATENCY_FORMAT_MASK 0x03

	CHECK_POINTER(pPacket, false);
	CHECK_POINTER(pVSIInfoFrame, false);

	//[31:0]:reserved for crc value of EDR_3DLUT sub_lut_7
	U32 VR_DV_CTRL_0 = 313;  // 0x50014E4
	U32 n = 0;
	U32 Type, Version, Length, IEEE;
	U32 HDMI_3D_Structure = 0, HDMI_3D_Ext_Data = 0;
	U8 B1, B2, B3;
	U8 Flags;
	// Parse header
	Type = pPacket->HeaderByte[0];

	if ((eHdmiRxPacketType)Type != ThdmiRxPacketType_VSI) {
		PACK_INF("%s: Not an VSI InfoFrame!\n", __func__);
		return false;
	}

	if (!HdmiRxPacket_Check_InfoFrame_Checksum(pPacket)) {
		PACK_INF("%s: VSI InfoFrame checksum is false!\n", __func__);
		return false;
	}

	Version = pPacket->HeaderByte[1];
	Length = pPacket->HeaderByte[2] & 0x1F;

	if (Length < 4) {
		// log_i("Insufficient size for an VendorSpecificInfoFrame! %lu byte instead of 4\n", Length);
		return false;
	}

	// Parse IEEE ID
	IEEE = 0;
	B1 = pPacket->PacketByte[++n];
	B2 = pPacket->PacketByte[++n];
	B3 = pPacket->PacketByte[++n];
	IEEE = B1 | (B2 << 8) | (B3 << 16);

	if (IEEE != RMEDID_IEEE_OUI_HDMI_DbVisionLowLatency) {
		/// log_i("Wrong IEEE ID! %06X instead of %6X\n", IEEE, RMEDID_IEEE_OUI_HDMI_DbVisionLowLatency);
		// return false;
	}

	// Parse fields
	Flags = pPacket->PacketByte[++n];
	/*
	HDMIRx_ReadRegU32(0x5001004);
	if (HDMIRx_ReadRegU32(VR_DV_CTRL_0) & _BIT0_) {  // disable LL detection; enable manunal adjustment
	} else {
	*/
		if (HDMI_VSI_DOBLY_LOWLATENCY_FORMAT == (Flags & HDMI_VSI_DOBLY_LOWLATENCY_FORMAT_MASK)) {
			pVSIInfoFrame->DbVision_LowLatency = true;
			// tHdmiRxContext.tVideoCtx.bNewDbVisionLowLatencyMode = true;
			// log_i("NO_HDMI_VSI_DOBLY_LOWLATENCY_FORMAT Flags)Flags:0x%x bNewDbVisionLowLatencyMode0x%x\n", Flags,
			// tHdmiRxContext.tVideoCtx.bNewDbVisionLowLatencyMode);
		} else {
			pVSIInfoFrame->DbVision_LowLatency = false;
			// tHdmiRxContext.tVideoCtx.bNewDbVisionLowLatencyMode = false;
			// log_i("NO_HDMI_VSI_DOBLY_LOWLATENCY_FORMAT Flags)Flags:0x%x B1:0x%x B2:0x%x B3:0x%x\n",Flags,B1,B2,B3);
		}
	//}
	pVSIInfoFrame->HDMI_Video_Format = (Flags >> 5) & 0x7;

	switch (pVSIInfoFrame->HDMI_Video_Format) {
	case HDMI_VSI_EXT_FORMAT: {
		if (Length < ++n) {
			PACK_INF("%s: Insufficient size, missing HDMI VIC\n", __func__);
			return false;
		}

		pVSIInfoFrame->HDMI_VIC = pPacket->PacketByte[n];
		pVSIInfoFrame->b3DOn = false;
	} break;

	case HDMI_VSI_3D_FORMAT: {
		if (Length < ++n) {
			PACK_INF("%s: Insufficient size, missing HDMI 3D Structure\n", __func__);
			return false;
		}

		HDMI_3D_Structure = (pPacket->PacketByte[n] >> 4) & 0xF;  // 3D_Structure

		if (HDMI_3D_Structure >= 8) {
			if (Length < ++n) {
				PACK_INF("%s: Insufficient size, missing HDMI 3D ExtData\n", __func__);
				return false;
			}

			HDMI_3D_Ext_Data = (pPacket->PacketByte[n] >> 4) & 0xF;  // 3D_Ext_Data
		}

		pVSIInfoFrame->b3DOn = true;
	} break;

	default:
		pVSIInfoFrame->b3DOn = false;
		break;
	}

	// Debug info
	PACK_INF("%s:  +------------------------------------------------+\n", __func__);
	PACK_INF("    | HDMI VSI InfoFrame, Type 0x%02X Ver. 0x%02X Len %2u |\n", Type, Version, Length);
	switch (pVSIInfoFrame->HDMI_Video_Format) {
	case HDMI_VSI_EXT_FORMAT: {  // HDMI_Video_Format = 1: HDMI extended resolution format follows
		PACK_INF("	 +------------------------------------------------+\n");
		PACK_INF("    | HDMI_VIC %2lu: %s							 |\n", pVSIInfoFrame->HDMI_VIC,
			(pVSIInfoFrame->HDMI_VIC == 1) ? "2160p30" : (pVSIInfoFrame->HDMI_VIC == 2) ? "2160p25" : (pVSIInfoFrame->HDMI_VIC == 3) ? "2160p24" : (pVSIInfoFrame->HDMI_VIC == 4) ? "4K2K24 "
																											: "Unknown");
	} break;
	case HDMI_VSI_3D_FORMAT: {  // HDMI_Video_Format = 2: HDMI 3D format structure follows
		PACK_INF("	 +------------------------------------------------+\n");
		PACK_INF("    | 3D_Structure %2lu: %s				|\n", HDMI_3D_Structure,
				(HDMI_3D_Structure == 0) ? "FramePacking	  " : (HDMI_3D_Structure == 1)
																		? "FieldAlternative"
																		: (HDMI_3D_Structure == 2)
																				? "LineAlternative "
																				: (HDMI_3D_Structure == 3)
																					? "SideBySideFull  "
																					: (HDMI_3D_Structure == 4)
																						? "L + Depth	   "
																						: (HDMI_3D_Structure == 5)
																							? "L + Graphics	 "
																							: (HDMI_3D_Structure == 6)
																								? "TopAndBottom    "
																								: (HDMI_3D_Structure == 8)
																									? "SideBySideHalf  "
																									: "Unknown		 ");
			// pVSIInfoFrame->_3D_Struct = (e_3D_Struct)HDMI_3D_Structure;

			if (HDMI_3D_Structure >= 8) {
				PACK_INF("	 | 3D_Ext_Data %2lu: %s 			  |\n", HDMI_3D_Ext_Data,
					(HDMI_3D_Ext_Data == 0) ? "Horizontal OL OR" : (HDMI_3D_Ext_Data == 1)
																		? "Horizontal OL ER"
																		: (HDMI_3D_Ext_Data == 2)
																			? "Horizontal EL OR"
																			: (HDMI_3D_Ext_Data == 3)
																				? "Horizontal EL ER"
																				: (HDMI_3D_Ext_Data == 4)
																					? "Quincunx OL OR	"
																					: (HDMI_3D_Ext_Data == 5)
																						? "Quincunx OL ER  "
																						: (HDMI_3D_Ext_Data == 6)
																							? "Quincunx EL OR	"
																							: (HDMI_3D_Ext_Data == 7)
																								? "Quincunx EL ER  "
																								: "Unknown		  ");
				// pVSIInfoFrame->_3D_ExtData = (e_3D_Ext_Data)HDMI_3D_Ext_Data;
			}
		} break;
	}

	/*
	if (HDMI_Video_Format == 2) {  // HDMI_Video_Format = 2: HDMI 3D format structure follows
		*pHDMI_3D_Structure_Mask_Ext_Data = (1 << HDMI_3D_Structure) | (HDMI_3D_Ext_Data << 16);
	} else {
		*pHDMI_3D_Structure_Mask_Ext_Data = 0;
	}
	*/

	PACK_INF("+------------------------------------------------+\n");

	return true;
}
