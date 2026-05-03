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
#ifndef __TFD_DEFS_H
#define __TFD_DEFS_H


/* Version Definition */

/* Attribute Definition */
#define TFD_VERSION                                        0x17218018
#define TFD_TSE_DATA_ENDIAN_TYPE                           0x00000001
#define AT_SINGLE                                          0x0000
#define AI_SINGLE_DEFAULT                                  0x00000000

#define AT_SIGNAL_CHANNEL                                  0x0001
#define AI_SIGNAL_CHANNEL_MPEG1                            0x00010000
#define AI_SIGNAL_CHANNEL_NONE                             0x00010001
#define AI_SIGNAL_CHANNEL_HDTV1                            0x00010002
#define AI_SIGNAL_CHANNEL_HDTV2                            0x00010003
#define AI_SIGNAL_CHANNEL_PC                               0x00010004
#define AI_SIGNAL_CHANNEL_TV                               0x00010005
#define AI_SIGNAL_CHANNEL_CVBS1                            0x00010006
#define AI_SIGNAL_CHANNEL_CVBS2                            0x00010007
#define AI_SIGNAL_CHANNEL_CVBS3                            0x00010008
#define AI_SIGNAL_CHANNEL_SVIDEO1                          0x00010009
#define AI_SIGNAL_CHANNEL_SVIDEO2                          0x0001000A
#define AI_SIGNAL_CHANNEL_SVIDEO3                          0x0001000B
#define AI_SIGNAL_CHANNEL_YCBCR1                           0x0001000C
#define AI_SIGNAL_CHANNEL_YCBCR2                           0x0001000D
#define AI_SIGNAL_CHANNEL_MPEG2                            0x00010016
#define AI_SIGNAL_CHANNEL_SCART                            0x0001000F
#define AI_SIGNAL_CHANNEL_SCART2                           0x00010010
#define AI_SIGNAL_CHANNEL_HDMI1                            0x00010011
#define AI_SIGNAL_CHANNEL_HDMI2                            0x00010013
#define AI_SIGNAL_CHANNEL_HDMI3                            0x00010014
#define AI_SIGNAL_CHANNEL_HDMI4                            0x00010015
#define AI_SIGNAL_CHANNEL_STILLIMAGE                       0x00010012
#define AI_SIGNAL_CHANNEL_WISELINK                         0x00010017
#define AG_SIGNAL_CHANNEL_TCD                              0x00018000
#define AG_SIGNAL_CHANNEL_CVBS                             0x00018001
#define AG_SIGNAL_CHANNEL_SVIDEO                           0x00018002
#define AG_SIGNAL_CHANNEL_YCBCR                            0x00018003
#define AG_SIGNAL_CHANNEL_SCART1                           0x00018004
#define AG_SIGNAL_CHANNEL_HDTV                             0x00018005
#define AG_SIGNAL_CHANNEL_ALL_CH                           0x00018006
#define AG_SIGNAL_CHANNEL_HDMI                             0x00018007
#define AG_SIGNAL_CHANNEL_MPEG                             0x00018008

#define AT_SIGNAL_MODE                                     0x0002
#define AI_SIGNAL_MODE_1080P                               0x00020000
#define AI_SIGNAL_MODE_MOVIE                               0x00020017
#define AI_SIGNAL_MODE_PHOTO                               0x00020001
#define AI_SIGNAL_MODE_UNKNOW                              0x00020002
#define AI_SIGNAL_MODE_NO_SIGNAL                           0x00020003
#define AI_SIGNAL_MODE_NTSC                                0x00020004
#define AI_SIGNAL_MODE_PAL                                 0x00020005
#define AI_SIGNAL_MODE_SECAM                               0x00020006
#define AI_SIGNAL_MODE_PALM                                0x00020007
#define AI_SIGNAL_MODE_PALCN                               0x00020008
#define AI_SIGNAL_MODE_NTSC443                             0x00020009
#define AI_SIGNAL_MODE_PAL60                               0x0002000A
#define AI_SIGNAL_MODE_480I                                0x0002000B
#define AI_SIGNAL_MODE_480IX2                              0x0002000C
#define AI_SIGNAL_MODE_480P                                0x0002000D
#define AI_SIGNAL_MODE_480PX2                              0x0002000E
#define AI_SIGNAL_MODE_576I                                0x0002000F
#define AI_SIGNAL_MODE_576IX2                              0x00020010
#define AI_SIGNAL_MODE_576P                                0x00020011
#define AI_SIGNAL_MODE_576PX2                              0x00020012
#define AI_SIGNAL_MODE_720P                                0x00020015
#define AI_SIGNAL_MODE_1080I                               0x00020016
#define AI_SIGNAL_MODE_1440P2560                           0x00020095
#define AI_SIGNAL_MODE_2160P1920                           0x00020094
#define AI_SIGNAL_MODE_2160P3840                           0x0002008F
#define AI_SIGNAL_MODE_2160P4096                           0x00020093
#define AI_SIGNAL_MODE_640_350                             0x00020042
#define AI_SIGNAL_MODE_640_400                             0x00020043
#define AI_SIGNAL_MODE_640_480                             0x00020044
#define AI_SIGNAL_MODE_720_400                             0x00020045
#define AI_SIGNAL_MODE_720_576                             0x00020046
#define AI_SIGNAL_MODE_800_600                             0x00020047
#define AI_SIGNAL_MODE_832_624                             0x00020048
#define AI_SIGNAL_MODE_848_480                             0x00020049
#define AI_SIGNAL_MODE_852_480                             0x0002004A
#define AI_SIGNAL_MODE_960_600                             0x0002004B
#define AI_SIGNAL_MODE_1024_768                            0x0002004C
#define AI_SIGNAL_MODE_1152_864                            0x0002004D
#define AI_SIGNAL_MODE_1152_870                            0x0002004E
#define AI_SIGNAL_MODE_1152_900                            0x0002004F
#define AI_SIGNAL_MODE_1280_720                            0x00020050
#define AI_SIGNAL_MODE_1280_768                            0x00020051
#define AI_SIGNAL_MODE_1280_800                            0x00020052
#define AI_SIGNAL_MODE_1280_960                            0x00020053
#define AI_SIGNAL_MODE_1280_1024                           0x00020054
#define AI_SIGNAL_MODE_1360_765                            0x0002008A
#define AI_SIGNAL_MODE_1360_768                            0x00020055
#define AI_SIGNAL_MODE_1600_1200                           0x00020056
#define AI_SIGNAL_MODE_1920_1080                           0x00020057
#define AI_SIGNAL_MODE_1920_1200                           0x0002008D
#define AI_SIGNAL_MODE_1365_1024                           0x00020058
#define AI_SIGNAL_MODE_1366_768                            0x00020059
#define AI_SIGNAL_MODE_1440_900                            0x0002005A
#define AI_SIGNAL_MODE_1400_1050                           0x0002008E
#define AI_SIGNAL_MODE_1680_1050                           0x0002005B
#define AI_SIGNAL_MODE_1792_1344                           0x0002005C
#define AI_SIGNAL_MODE_1856_1392                           0x0002005D
#define AI_SIGNAL_MODE_2560_1440                           0x00020096
#define AI_SIGNAL_MODE_3840_2160                           0x00020091
#define AI_SIGNAL_MODE_4096_2160                           0x00020092
#define AI_SIGNAL_MODE_MAC_640_480                         0x0002005E
#define AI_SIGNAL_MODE_MAC_832_624                         0x0002005F
#define AI_SIGNAL_MODE_MAC_1024_768                        0x00020060
#define AI_SIGNAL_MODE_MAC_1152_864                        0x00020061
#define AI_SIGNAL_MODE_MAC_1152_870                        0x00020062
#define AI_SIGNAL_MODE_720_480I                            0x00020086
#define AI_SIGNAL_MODE_720_240P                            0x00020087
#define AI_SIGNAL_MODE_720_288P                            0x00020089
#define AI_SIGNAL_MODE_640_480P                            0x00020088
#define AI_SIGNAL_MODE_240PX2                              0x0002008B
#define AI_SIGNAL_MODE_288PX2                              0x0002008C
#define AG_SIGNAL_MODE_OTHERSD_576IP                       0x00028001
#define AG_SIGNAL_MODE_OTHERSD_480IP                       0x00028002
#define AG_SIGNAL_MODE_MPEG_SD                             0x00028003
#define AG_SIGNAL_MODE_HSD_VSD                             0x00028004
#define AG_SIGNAL_MODE_HMD_VSD                             0x00028005
#define AG_SIGNAL_MODE_HHD_VSD                             0x00028006
#define AG_SIGNAL_MODE_HD                                  0x00028007
#define AG_SIGNAL_MODE_480I_SIGNAL                         0x00028008
#define AG_SIGNAL_MODE_576I_SIGNAL                         0x00028009
#define AG_SIGNAL_MODE_I_SIGNAL                            0x0002800A
#define AG_SIGNAL_MODE_P_SIGNAL                            0x0002800B
#define AG_SIGNAL_MODE_MAC_SIGNAL                          0x0002800C
#define AG_SIGNAL_MODE_PC_SIGANL                           0x0002800D
#define AG_SIGNAL_MODE_PC_SIGANL_HD                        0x0002800E
#define AG_SIGNAL_MODE_PC_SIGANL_LD                        0x0002800F
#define AG_SIGNAL_MODE_PC_SIGNAL_VRES_SUPERHD              0x00028025
#define AG_SIGNAL_MODE_ALL_PCSIGNALS                       0x00028027
#define AG_SIGNAL_MODE_TCD_SIGNAL                          0x00028010
#define AG_SIGNAL_MODE_TCD3_50                             0x00028011
#define AG_SIGNAL_MODE_TCD3_60                             0x00028012
#define AI_SIGNAL_MODE_DTV_720_480_I                       0x00020068
#define AI_SIGNAL_MODE_DTV_720_576_I                       0x00020069
#define AI_SIGNAL_MODE_DTV_720_1080_I                      0x0002006A
#define AI_SIGNAL_MODE_DTV_1280_480_I                      0x0002006B
#define AI_SIGNAL_MODE_DTV_1280_576_I                      0x0002006C
#define AI_SIGNAL_MODE_DTV_1280_1080_I                     0x0002006D
#define AI_SIGNAL_MODE_DTV_1920_480_I                      0x0002006E
#define AI_SIGNAL_MODE_DTV_1920_576_I                      0x0002006F
#define AI_SIGNAL_MODE_DTV_1920_1080_I                     0x00020070
#define AI_SIGNAL_MODE_DTV_720_480_P                       0x00020071
#define AI_SIGNAL_MODE_DTV_720_576_P                       0x00020072
#define AI_SIGNAL_MODE_DTV_720_720_P                       0x00020073
#define AI_SIGNAL_MODE_DTV_720_1080_P                      0x00020074
#define AI_SIGNAL_MODE_DTV_1280_480_P                      0x00020075
#define AI_SIGNAL_MODE_DTV_1280_576_P                      0x00020076
#define AI_SIGNAL_MODE_DTV_1280_720_P                      0x00020077
#define AI_SIGNAL_MODE_DTV_1280_1080_P                     0x00020078
#define AI_SIGNAL_MODE_DTV_1920_480_P                      0x00020079
#define AI_SIGNAL_MODE_DTV_1920_576_P                      0x0002007A
#define AI_SIGNAL_MODE_DTV_1920_720_P                      0x0002007B
#define AI_SIGNAL_MODE_DTV_1920_1080_P                     0x0002007C
#define AI_SIGNAL_MODE_DTV_3840_2160_P                     0x00020090
#define AG_SIGNAL_MODE_DTV_SIGNAL                          0x00028013
#define AG_SIGNAL_MODE_DTV_V_SD                            0x00028014
#define AG_SIGNAL_MODE_DTV_V_MD                            0x00028015
#define AG_SIGNAL_MODE_DTV_V_HD                            0x00028016
#define AG_SIGNAL_MODE_DTV_H_SD                            0x00028017
#define AG_SIGNAL_MODE_DTV_H_MD                            0x00028018
#define AG_SIGNAL_MODE_DTV_H_HD                            0x00028019
#define AG_SIGNAL_MODE_DTV_V_SD_I                          0x0002801A
#define AG_SIGNAL_MODE_DTV_V_SD_480I                       0x0002801B
#define AG_SIGNAL_MODE_DTV_V_SD_576I                       0x0002801C
#define AG_SIGNAL_MODE_DTV_V_SD_P                          0x0002801D
#define AG_SIGNAL_MODE_DTV_V_SD_480P                       0x0002801E
#define AG_SIGNAL_MODE_DTV_V_SD_576P                       0x0002801F
#define AG_SIGNAL_MODE_DTV_V_HD_I                          0x00028020
#define AG_SIGNAL_MODE_DTV_V_MD_P                          0x00028021
#define AG_SIGNAL_MODE_DTV_V_HD_P                          0x00028022
#define AG_SIGNAL_MODE_DTV_I                               0x00028023
#define AG_SIGNAL_MODE_DTV_P                               0x00028024
#define AI_SIGNAL_MODE_STILLIMAGESIGNAL                    0x0002007D
#define AI_SIGNAL_MODE_480P_HV                             0x0002007E
#define AI_SIGNAL_MODE_720P_HV                             0x00020080
#define AI_SIGNAL_MODE_576P_HV                             0x0002007F
#define AI_SIGNAL_MODE_1080I_HV                            0x00020081
#define AI_SIGNAL_MODE_1080P_HV                            0x00020082
#define AI_SIGNAL_MODE_480I_HV                             0x00020083
#define AI_SIGNAL_MODE_576I_HV                             0x00020084
#define AI_SIGNAL_MODE_SIGNAL_FREE_RUN                     0x00020085
#define AG_SIGNAL_MODE_UHD                                 0x00028026

#define AT_SIGNAL_FRAMERATE                                0x0003
#define AI_SIGNAL_FRAMERATE_60                             0x00030000
#define AI_SIGNAL_FRAMERATE_23_97                          0x0003000B
#define AI_SIGNAL_FRAMERATE_24                             0x00030001
#define AI_SIGNAL_FRAMERATE_25                             0x00030002
#define AI_SIGNAL_FRAMERATE_29_97                          0x00030003
#define AI_SIGNAL_FRAMERATE_30                             0x00030004
#define AI_SIGNAL_FRAMERATE_43                             0x00030005
#define AI_SIGNAL_FRAMERATE_47_952                         0x0003001A
#define AI_SIGNAL_FRAMERATE_48                             0x00030006
#define AI_SIGNAL_FRAMERATE_50                             0x00030007
#define AI_SIGNAL_FRAMERATE_56                             0x00030008
#define AI_SIGNAL_FRAMERATE_59_934                         0x00030009
#define AI_SIGNAL_FRAMERATE_59_94                          0x0003000A
#define AI_SIGNAL_FRAMERATE_67                             0x0003000C
#define AI_SIGNAL_FRAMERATE_70                             0x0003000D
#define AI_SIGNAL_FRAMERATE_72                             0x0003000E
#define AI_SIGNAL_FRAMERATE_75                             0x0003000F
#define AI_SIGNAL_FRAMERATE_82                             0x00030010
#define AI_SIGNAL_FRAMERATE_85                             0x00030011
#define AI_SIGNAL_FRAMERATE_90                             0x00030012
#define AI_SIGNAL_FRAMERATE_100                            0x00030013
#define AI_SIGNAL_FRAMERATE_119_88                         0x00030019
#define AI_SIGNAL_FRAMERATE_120                            0x00030014
#define AG_SIGNAL_FRAMERATE_ALLEXCEPT120                   0x00038003
#define AG_SIGNAL_FRAMERATE_PC                             0x00038000
#define AG_SIGNAL_FRAMERATE_VIDEO                          0x00038001
#define AI_SIGNAL_FRAMERATE_8                              0x00030015
#define AI_SIGNAL_FRAMERATE_12                             0x00030016
#define AI_SIGNAL_FRAMERATE_15                             0x00030017
#define AI_SIGNAL_FRAMERATE_16                             0x00030018
#define AG_SIGNAL_FRAMERATE_LOWFRAMERATE                   0x00038002
#define AG_SIGNAL_FRAMERATE_HIGHFRAMERATE                  0x00038004
#define AG_SIGNAL_FRAMERATE_FRAMERATEOVER30HZ              0x00038005

#define AT_SIGNAL_FORMAT                                   0x001D
#define AI_SIGNAL_FORMAT_YUV422_101010                     0x001D0000
#define AI_SIGNAL_FORMAT_RGB_888                           0x001D0007
#define AI_SIGNAL_FORMAT_RGB_101010                        0x001D0001
#define AI_SIGNAL_FORMAT_RGB_121212                        0x001D0002
#define AI_SIGNAL_FORMAT_YUV444_888                        0x001D0003
#define AI_SIGNAL_FORMAT_YUV444_101010                     0x001D0004
#define AI_SIGNAL_FORMAT_YUV444_121212                     0x001D0005
#define AI_SIGNAL_FORMAT_YUV422_888                        0x001D0006
#define AI_SIGNAL_FORMAT_YUV422_121212                     0x001D0008
#define AI_SIGNAL_FORMAT_YUV420_888                        0x001D0009
#define AI_SIGNAL_FORMAT_YUV420_101010                     0x001D000A
#define AI_SIGNAL_FORMAT_YUV420_121212                     0x001D000B
#define AG_SIGNAL_FORMAT_RGB                               0x001D8000
#define AG_SIGNAL_FORMAT_YUV444                            0x001D8001
#define AG_SIGNAL_FORMAT_YUV422                            0x001D8002
#define AG_SIGNAL_FORMAT_YUV420                            0x001D8003
#define AG_SIGNAL_FORMAT_YUV                               0x001D8004
#define AG_SIGNAL_FORMAT_ALL_444                           0x001D8005
#define AG_SIGNAL_FORMAT_ALL_FORMAT                        0x001D8006

#define AT_SIGNAL_RANGE                                    0x003A
#define AI_SIGNAL_RANGE_LIMIT_RANGE                        0x003A0000
#define AI_SIGNAL_RANGE_FULL_RANGE                         0x003A0001

#define AT_SIGNAL_COLORSPACE                               0x0044
#define AI_SIGNAL_COLORSPACE_BT709                         0x00440000
#define AI_SIGNAL_COLORSPACE_BT601                         0x00440001
#define AI_SIGNAL_COLORSPACE_BT2020_NCLYCC                 0x00440002
#define AI_SIGNAL_COLORSPACE_BT2020_CLYCC                  0x00440009
#define AI_SIGNAL_COLORSPACE_XVYCC                         0x00440003
#define AI_SIGNAL_COLORSPACE_RGB                           0x00440004
#define AG_SIGNAL_COLORSPACE_ALL_SIGNAL_CS                 0x00448001

#define AT_VINCAP_ICSC                                     0x0065
#define AI_VINCAP_ICSC_BYPASS                              0x00650000
#define AI_VINCAP_ICSC_BT601                               0x00650001
#define AI_VINCAP_ICSC_BT709                               0x00650002
#define AI_VINCAP_ICSC_BT2020NCLYCC                        0x00650003

#define AT_VPROC_CSC1                                      0x0066
#define AI_VPROC_CSC1_BYPASS                               0x00660000
#define AI_VPROC_CSC1_BT601                                0x00660001
#define AI_VPROC_CSC1_BT709                                0x00660002
#define AI_VPROC_CSC1_BT2020NCLYCC                         0x00660003

#define AT_VPROC_ICSC2                                     0x0067
#define AI_VPROC_ICSC2_BYPASS                              0x00670000
#define AI_VPROC_ICSC2_BT601                               0x00670001
#define AI_VPROC_ICSC2_BT709                               0x00670002
#define AI_VPROC_ICSC2_BT2020NCLYCC                        0x00670003

#define AT_VPROC_CSC3                                      0x0068
#define AI_VPROC_CSC3_BYPASS                               0x00680000
#define AI_VPROC_CSC3_BT601                                0x00680001
#define AI_VPROC_CSC3_BT709                                0x00680002
#define AI_VPROC_CSC3_BT2020NCLYCC                         0x00680003

#define AT_V_INCAP_MP_FORMAT                               0x000A
#define AI_V_INCAP_MP_FORMAT_YUV422_101010                 0x000A0000
#define AI_V_INCAP_MP_FORMAT_YUV422_888                    0x000A0001
#define AI_V_INCAP_MP_FORMAT_YUV422_1088                   0x000A0003
#define AI_V_INCAP_MP_FORMAT_YUV422_121212                 0x000A000C
#define AI_V_INCAP_MP_FORMAT_YUV444_888                    0x000A0004
#define AI_V_INCAP_MP_FORMAT_YUV444_101010                 0x000A0007
#define AI_V_INCAP_MP_FORMAT_RGB444_888                    0x000A0008
#define AI_V_INCAP_MP_FORMAT_RGB444_101010                 0x000A0009
#define AG_V_INCAP_MP_FORMAT_YUV422_VINCAP_MP              0x000A8000
#define AG_V_INCAP_MP_FORMAT_YUV444_VINCAP_MP              0x000A8001
#define AG_V_INCAP_MP_FORMAT_RGB444_VINCAP_MP              0x000A8002

#define AT_V_INCAP_PIP_FORMAT                              0x0025
#define AI_V_INCAP_PIP_FORMAT_YUV422_101010                0x00250000
#define AI_V_INCAP_PIP_FORMAT_YUV422_888                   0x00250001
#define AI_V_INCAP_PIP_FORMAT_YUV422_1088                  0x00250002
#define AI_V_INCAP_PIP_FORMAT_YUV444_888                   0x00250004
#define AI_V_INCAP_PIP_FORMAT_YUV444_101010                0x00250005
#define AI_V_INCAP_PIP_FORMAT_RGB444_888                   0x00250006
#define AI_V_INCAP_PIP_FORMAT_RGB444_101010                0x00250007
#define AG_V_INCAP_PIP_FORMAT_YUV422_VINCAP_PIP            0x00258000
#define AG_V_INCAP_PIP_FORMAT_YUV444_VINCAP_PIP            0x00258001
#define AG_V_INCAP_PIP_FORMAT_RGB444_VINCAP_PIP            0x00258002

#define AT_V_NR_MP_FORMAT                                  0x0022
#define AI_V_NR_MP_FORMAT_YUV422_101010                    0x00220000
#define AI_V_NR_MP_FORMAT_YUV422_1088                      0x00220001
#define AI_V_NR_MP_FORMAT_YUV422_888                       0x00220002
#define AI_V_NR_MP_FORMAT_YUV444_888                       0x00220003
#define AI_V_NR_MP_FORMAT_YUV444_101010                    0x00220004
#define AI_V_NR_MP_FORMAT_RGB444_888                       0x00220005
#define AI_V_NR_MP_FORMAT_RGB444_101010                    0x00220006
#define AG_V_NR_MP_FORMAT_YUV422_VNR                       0x00228000
#define AG_V_NR_MP_FORMAT_YUV444_VNR                       0x00228001
#define AG_V_NR_MP_FORMAT_RGB444_VNR                       0x00228002
#define AI_V_NR_MP_FORMAT_YUV420_101010                    0x00220008
#define AI_V_NR_MP_FORMAT_YUV420_1088                      0x00220009
#define AI_V_NR_MP_FORMAT_YUV420_888                       0x0022000A
#define AG_V_NR_MP_FORMAT_YUV420_VNR                       0x00228003

#define AT_V_DETN_MP_FORMAT                                0x004A
#define AI_V_DETN_MP_FORMAT_YUV422_101010                  0x004A0000
#define AI_V_DETN_MP_FORMAT_YUV422_1088                    0x004A0001
#define AI_V_DETN_MP_FORMAT_YUV422_888                     0x004A0002
#define AI_V_DETN_MP_FORMAT_YUV444_888                     0x004A0003
#define AI_V_DETN_MP_FORMAT_YUV444_101010                  0x004A0004
#define AI_V_DETN_MP_FORMAT_RGB444_888                     0x004A0005
#define AI_V_DETN_MP_FORMAT_RGB444_101010                  0x004A0006
#define AG_V_DETN_MP_FORMAT_YUV422_VDETN                   0x004A8000
#define AG_V_DETN_MP_FORMAT_YUV444_VDETN                   0x004A8001
#define AG_V_DETN_MP_FORMAT_RGB444_VDETN                   0x004A8002
#define AG_V_DETN_MP_FORMAT_YUV420_VDETN                   0x00000010
#define AI_V_DETN_MP_FORMAT_YUV420_101010                  0x004A0007
#define AI_V_DETN_MP_FORMAT_YUV420_1088                    0x004A0008
#define AI_V_DETN_MP_FORMAT_YUV420_888                     0x004A0009

#define AT_V_SCALAR_MP_FORMAT                              0x0023
#define AI_V_SCALAR_MP_FORMAT_YUV422_101010                0x00230000
#define AI_V_SCALAR_MP_FORMAT_YUV422_1088                  0x00230001
#define AI_V_SCALAR_MP_FORMAT_YUV422_888                   0x00230002
#define AI_V_SCALAR_MP_FORMAT_YUV444_888                   0x00230003
#define AI_V_SCALAR_MP_FORMAT_YUV444_101010                0x00230004
#define AI_V_SCALAR_MP_FORMAT_RGB444_888                   0x00230005
#define AI_V_SCALAR_MP_FORMAT_RGB444_101010                0x00230006
#define AG_V_SCALAR_MP_FORMAT_YUV422_VSCALAR               0x00238000
#define AG_V_SCALAR_MP_FORMAT_YUV444_VSCALAR               0x00238001
#define AG_V_SCALAR_MP_FORMAT_RGB444_VSCALAR               0x00238002

#define AT_V_PROC_MP_FORMAT                                0x004B
#define AI_V_PROC_MP_FORMAT_YUV422_101010                  0x004B0000
#define AI_V_PROC_MP_FORMAT_YUV422_1088                    0x004B0001
#define AI_V_PROC_MP_FORMAT_YUV422_888                     0x004B0002
#define AI_V_PROC_MP_FORMAT_YUV444_888                     0x004B0003
#define AI_V_PROC_MP_FORMAT_YUV444_101010                  0x004B0004
#define AI_V_PROC_MP_FORMAT_RGB444_888                     0x004B0005
#define AI_V_PROC_MP_FORMAT_RGB444_101010                  0x004B0006
#define AG_V_PROC_MP_FORMAT_YUV422_VPROC                   0x004B8000
#define AG_V_PROC_MP_FORMAT_YUV444_VPROC                   0x004B8001
#define AG_V_PROC_MP_FORMAT_RGB444_VPROC                   0x004B8002

#define AT_V_BLEND_MP_FORMAT                               0x004C
#define AI_V_BLEND_MP_FORMAT_YUV422_101010                 0x004C0000
#define AI_V_BLEND_MP_FORMAT_YUV422_1088                   0x004C0001
#define AI_V_BLEND_MP_FORMAT_YUV422_888                    0x004C0002
#define AI_V_BLEND_MP_FORMAT_YUV444_888                    0x004C0003
#define AI_V_BLEND_MP_FORMAT_YUV444_101010                 0x004C0004
#define AI_V_BLEND_MP_FORMAT_RGB444_888                    0x004C0005
#define AI_V_BLEND_MP_FORMAT_RGB444_101010                 0x004C0006
#define AG_V_BLEND_MP_FORMAT_YUV422_VBLEND_MP              0x004C8000
#define AG_V_BLEND_MP_FORMAT_YUV444_VBLEND_MP              0x004C8001
#define AG_V_BLEND_MP_FORMAT_RGB444_VBLEND_MP              0x004C8002

#define AT_V_BLEND_PIP_FORMAT                              0x004D
#define AI_V_BLEND_PIP_FORMAT_YUV422_101010                0x004D0000
#define AI_V_BLEND_PIP_FORMAT_YUV422_1088                  0x004D0001
#define AI_V_BLEND_PIP_FORMAT_YUV422_888                   0x004D0002
#define AI_V_BLEND_PIP_FORMAT_YUV444_888                   0x004D0003
#define AI_V_BLEND_PIP_FORMAT_YUV444_101010                0x004D0004
#define AI_V_BLEND_PIP_FORMAT_RGB444_888                   0x004D0005
#define AI_V_BLEND_PIP_FORMAT_RGB444_101010                0x004D0006
#define AG_V_BLEND_PIP_FORMAT_YUV444_VBLEND_PIP            0x004D8001
#define AG_V_BLEND_PIP_FORMAT_RGB444_VBLEND_PIP            0x004D8002
#define AG_V_BLEND_PIP_FORMAT_YUV422_VBLEND_PIP            0x004D8000

#define AT_V_FRC_MP_FORMAT                                 0x0045
#define AI_V_FRC_MP_FORMAT_YUV422_101010                   0x00450000
#define AI_V_FRC_MP_FORMAT_YUV422_1088                     0x00450001
#define AI_V_FRC_MP_FORMAT_YUV422_888                      0x00450002
#define AI_V_FRC_MP_FORMAT_YUV444_888                      0x00450003
#define AI_V_FRC_MP_FORMAT_YUV444_101010                   0x00450004
#define AI_V_FRC_MP_FORMAT_RGB444_888                      0x00450005
#define AI_V_FRC_MP_FORMAT_RGB444_101010                   0x00450006
#define AG_V_FRC_MP_FORMAT_YUV422_VFRC                     0x00458000
#define AG_V_FRC_MP_FORMAT_YUV444_VFRC                     0x00458001
#define AG_V_FRC_MP_FORMAT_RGB444_VFRC                     0x00458002

#define AT_V_PANEL_MP_FORMAT                               0x0046
#define AI_V_PANEL_MP_FORMAT_YUV422_101010                 0x00460000
#define AI_V_PANEL_MP_FORMAT_YUV422_1088                   0x00460001
#define AI_V_PANEL_MP_FORMAT_YUV422_888                    0x00460002
#define AI_V_PANEL_MP_FORMAT_YUV444_888                    0x00460003
#define AI_V_PANEL_MP_FORMAT_YUV444_101010                 0x00460004
#define AI_V_PANEL_MP_FORMAT_RGB444_888                    0x00460005
#define AI_V_PANEL_MP_FORMAT_RGB444_101010                 0x00460006
#define AG_V_PANEL_MP_FORMAT_YUV422_VPANEL                 0x00468000
#define AG_V_PANEL_MP_FORMAT_YUV444_VPANEL                 0x00468001
#define AG_V_PANEL_MP_FORMAT_RGB444_VPANEL                 0x00468002

#define AT_MPPIP                                           0x0005
#define AI_MPPIP_MP                                        0x00050000
#define AI_MPPIP_PIP                                       0x00050001
#define AI_MPPIP_CVBS_OUT                                  0x00050002

#define AT_OUTPUT_RESOLUTION                               0x0006
#define AI_OUTPUT_RESOLUTION_3840_2160                     0x00060000
#define AI_OUTPUT_RESOLUTION_720_480                       0x00060001
#define AI_OUTPUT_RESOLUTION_720_576                       0x00060002
#define AI_OUTPUT_RESOLUTION_1280_720                      0x00060003
#define AI_OUTPUT_RESOLUTION_1920_1080                     0x00060004
#define AI_OUTPUT_RESOLUTION_2560_1080                     0x00060005
#define AI_OUTPUT_RESOLUTION_1024_768                      0x00060007
#define AI_OUTPUT_RESOLUTION_1366_768                      0x00060008
#define AI_OUTPUT_RESOLUTION_1600_900                      0x00060009
#define AI_OUTPUT_RESOLUTION_4096_2160                     0x0006000A
#define AI_OUTPUT_RESOLUTION_640_360                       0x0006000B
#define AI_OUTPUT_RESOLUTION_854_480                       0x0006000C

#define AT_OUTPUT_FRAMERATE                                0x0041
#define AI_OUTPUT_FRAMERATE_120HZ                          0x00410000
#define AI_OUTPUT_FRAMERATE_200HZ                          0x00410001
#define AI_OUTPUT_FRAMERATE_240HZ                          0x00410002
#define AI_OUTPUT_FRAMERATE_100HZ                          0x00410003
#define AI_OUTPUT_FRAMERATE_96HZ                           0x00410004
#define AI_OUTPUT_FRAMERATE_60HZ                           0x00410005
#define AI_OUTPUT_FRAMERATE_50HZ                           0x00410006
#define AI_OUTPUT_FRAMERATE_48HZ                           0x00410007
#define AI_OUTPUT_FRAMERATE_30HZ                           0x00410008
#define AI_OUTPUT_FRAMERATE_25HZ                           0x00410009
#define AI_OUTPUT_FRAMERATE_24HZ                           0x0041000A
#define AG_OUTPUT_FRAMERATE_FRAMERATE_60                   0x00418000
#define AG_OUTPUT_FRAMERATE_FRAMERATE_120                  0x00418001
#define AG_OUTPUT_FRAMERATE_FRAMERATE_240                  0x00418002
#define AG_OUTPUT_FRAMERATE_ALL_FRAMERATE                  0x00418003

#define AT_OUTPUT_MAXFRAMERATE                             0x0062
#define AI_OUTPUT_MAXFRAMERATE_60HZ                        0x00620000
#define AI_OUTPUT_MAXFRAMERATE_120HZ                       0x00620001

#define AT_OUTPUT_TIMINGMODE                               0x0040
#define AI_OUTPUT_TIMINGMODE_FIXED_HTOTAL_ONLY             0x00400000
#define AI_OUTPUT_TIMINGMODE_FIXED_HVTOTAL                 0x00400001

#define AT_MEMORY_SIZE                                     0x0008
#define AI_MEMORY_SIZE_16M                                 0x00080000
#define AI_MEMORY_SIZE_32M                                 0x00080001
#define AI_MEMORY_SIZE_64M                                 0x00080002
#define AI_MEMORY_SIZE_128M                                0x00080003
#define AI_MEMORY_SIZE_256M                                0x00080004
#define AI_MEMORY_SIZE_384M                                0x00080005
#define AI_MEMORY_SIZE_512M                                0x00080006
#define AI_MEMORY_SIZE_768M                                0x00080007
#define AI_MEMORY_SIZE_1024M                               0x00080008

#define AT_WINDOW                                          0x0009
#define AI_WINDOW__MPS_169_SUPERWIDE_0                     0x00090000
#define AI_WINDOW__MPS_169_WIDE_1                          0x00090001
#define AI_WINDOW__MPS_169_PANORAMA_2                      0x00090002
#define AI_WINDOW__MPS_169_SUBZOOM_3                       0x00090003
#define AI_WINDOW__MPS_169_ZOOM_4                          0x00090004
#define AI_WINDOW__MPS_169_149_ZOOM_5                      0x00090005
#define AI_WINDOW__MPS_169_43_6                            0x00090006
#define AI_WINDOW__MPS_43_NORMAL_7                         0x00090007
#define AI_WINDOW__MPS_43_169_ZOOM_8                       0x00090008
#define AI_WINDOW__MPS_AUTO_10                             0x00090009
#define AI_WINDOW__MPS_FULL_12                             0x0009000A
#define AI_WINDOW__MPS_EPG_13                              0x0009000B
#define AI_WINDOW__MPS_169_ZOOM_POPL_36                    0x0009000C
#define AI_WINDOW__MPS_169_ZOOM_POPR_68                    0x0009000D
#define AI_WINDOW__MPS_149_ZOOM_POPL_37                    0x0009000E
#define AI_WINDOW__MPS_149_ZOOM_69                         0x0009000F
#define AI_WINDOW__MPS_POP_LEFT_32                         0x00090010
#define AI_WINDOW__MPS_POP_RIGHT_64                        0x00090011
#define AI_WINDOW__MPS_PANORAMA_POPL_43                    0x00090012
#define AI_WINDOW__MPS_PANORAMA_POPR_75                    0x00090013
#define AI_WINDOW__MPS_FULL_POPL_44                        0x00090014
#define AI_WINDOW__MPS_FULL_POPR_76                        0x00090015
#define AI_WINDOW__MPS_169_149_15                          0x00090016
#define AI_WINDOW__MPS_149_IN_43_169                       0x00090017
#define AI_WINDOW__MPS_43_LETTERBOX_                       0x00090018
#define AI_WINDOW__MPS_2211_WIDE_51                        0x00090019
#define AI_WINDOW__MPS_DOTBYDOT_V_52                       0x0009001A
#define AI_WINDOW__PIPS_UPPERLEFT_1_0                      0x0009001B
#define AI_WINDOW__PIPS_UPPERRIGHT_1_1                     0x0009001C
#define AI_WINDOW__PIPS_LOWERRIGHT_1_2                     0x0009001D
#define AI_WINDOW__PIPS_LOWERLEFT_1_3                      0x0009001E
#define AI_WINDOW__PIPS_UPPERLEFT_2_4                      0x0009001F
#define AI_WINDOW__PIPS_UPPERRIGHT_2_5                     0x00090020
#define AI_WINDOW__PIPS_LOWERRIGHT_2_6                     0x00090021
#define AI_WINDOW__PIPS_LOWERLEFT_2_7                      0x00090022
#define AI_WINDOW__PIPS_UPPERLEFT_3_8                      0x00090023
#define AI_WINDOW__PIPS_UPPERRIGHT_3_9                     0x00090024
#define AI_WINDOW__PIPS_LOWERRIGHT_3_10                    0x00090025
#define AI_WINDOW__PIPS_LOWERLEFT_3_11                     0x00090026
#define AI_WINDOW__PIPS_UPPERLEFT_4_12                     0x00090027
#define AI_WINDOW__PIPS_UPPERRIGHT_4_13                    0x00090028
#define AI_WINDOW__PIPS_LOWERRIGHT_4_14                    0x00090029
#define AI_WINDOW__PIPS_LOWERLEFT_4_15                     0x0009002A
#define AI_WINDOW__PIPS_FULL_SCREEN_20                     0x0009002B
#define AI_WINDOW__PIPS_TELETEXT_30                        0x0009002C
#define AI_WINDOW__PIPS_POP_LEFT_96                        0x0009002D
#define AI_WINDOW__PIPS_POP_RIGHT_128                      0x0009002E
#define AI_WINDOW__PIPS_POP_LEFT_4B3N_102                  0x0009002F
#define AI_WINDOW__PIPS_POP_RIGHT_4B3N_134                 0x00090030
#define AI_WINDOW__PIPS_FULL_POPL_108                      0x00090031
#define AI_WINDOW__PIPS_FULL_POPR_140                      0x00090032
#define AG_WINDOW_WINDOW_POP                               0x00098000

#define AT_CVBSOUT_INPUT_CHANNEL                           0x000B
#define AI_CVBSOUT_INPUT_CHANNEL_HDTV1                     0x000B0000
#define AI_CVBSOUT_INPUT_CHANNEL_HDTV2                     0x000B0001
#define AI_CVBSOUT_INPUT_CHANNEL_PC                        0x000B0002
#define AI_CVBSOUT_INPUT_CHANNEL_TV                        0x000B0003
#define AI_CVBSOUT_INPUT_CHANNEL_CVBS1                     0x000B0004
#define AI_CVBSOUT_INPUT_CHANNEL_CVBS2                     0x000B0005
#define AI_CVBSOUT_INPUT_CHANNEL_CVBS3                     0x000B0006
#define AI_CVBSOUT_INPUT_CHANNEL_SVIDEO1                   0x000B0007
#define AI_CVBSOUT_INPUT_CHANNEL_SVIDEO2                   0x000B0008
#define AI_CVBSOUT_INPUT_CHANNEL_SVIDEO3                   0x000B0009
#define AI_CVBSOUT_INPUT_CHANNEL_YCBCR1                    0x000B000A
#define AI_CVBSOUT_INPUT_CHANNEL_YCBCR2                    0x000B000B
#define AI_CVBSOUT_INPUT_CHANNEL_MPEG1                     0x000B000C
#define AI_CVBSOUT_INPUT_CHANNEL_SCART1                    0x000B000D
#define AI_CVBSOUT_INPUT_CHANNEL_SCART2_CVBS               0x000B000E
#define AI_CVBSOUT_INPUT_CHANNEL_SCART2_YC                 0x000B000F
#define AI_CVBSOUT_INPUT_CHANNEL_HDMI                      0x000B0010
#define AI_CVBSOUT_INPUT_CHANNEL_SVP_MUX_MP1920_1080       0x000B0011
#define AI_CVBSOUT_INPUT_CHANNEL_SVP_MUX_MP1366_768        0x000B0012
#define AI_CVBSOUT_INPUT_CHANNEL_SVP_MUX_PIP               0x000B0013
#define AI_CVBSOUT_INPUT_CHANNEL_CVBSOUT_OFF               0x000B0014
#define AI_CVBSOUT_INPUT_CHANNEL_MPEG2                     0x000B0015

#define AT_CVBSOUT_OUTPUT_SIGNAL                           0x000D
#define AI_CVBSOUT_OUTPUT_SIGNAL_PAL50                     0x000D0000
#define AI_CVBSOUT_OUTPUT_SIGNAL_PALM                      0x000D0001
#define AI_CVBSOUT_OUTPUT_SIGNAL_PALNC                     0x000D0002
#define AI_CVBSOUT_OUTPUT_SIGNAL_PAL60                     0x000D0003
#define AI_CVBSOUT_OUTPUT_SIGNAL_NTSC443                   0x000D0004
#define AI_CVBSOUT_OUTPUT_SIGNAL_NTSC_J                    0x000D0005
#define AI_CVBSOUT_OUTPUT_SIGNAL_NTSC_M                    0x000D0006
#define AI_CVBSOUT_OUTPUT_SIGNAL_PALN                      0x000D0007

#define AT_CVBSOUT_OUTPUT_CHANNEL                          0x000C
#define AI_CVBSOUT_OUTPUT_CHANNEL_CVBSOUT1                 0x000C0000
#define AI_CVBSOUT_OUTPUT_CHANNEL_CVBSOUT2                 0x000C0001
#define AI_CVBSOUT_OUTPUT_CHANNEL_CVBSOUT_SVIDEO           0x000C0002

#define AT_SOG                                             0x0013
#define AI_SOG_SOG_FALSE                                   0x00130000
#define AI_SOG_SOG_TRUE                                    0x00130001

#define AT_MIRRORTYPE                                      0x0014
#define AI_MIRRORTYPE_HVNOMIRROR                           0x00140000
#define AI_MIRRORTYPE_H_MIRROR                             0x00140001
#define AI_MIRRORTYPE_V_MIRROR                             0x00140002
#define AI_MIRRORTYPE_HV_MIRROR                            0x00140003

#define AT_3DTV_INPUT_MODE                                 0x0018
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_OFF                  0x00180000
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_2D_TO_3D             0x0018000A
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_FRAMEPACKING         0x00180009
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_SIDEBYSIDE_HALF      0x00180001
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_SIDEBYSIDE_FULL      0x0018000B
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_TOP_BOTTOM           0x00180002
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_LINE_ALTERNATIVE     0x00180004
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_FIELD_ALTERNATIVE    0x0018000C
#define AG_3DTV_INPUT_MODE_3DTV_INPUT_ON                   0x00188000
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_COLUMN_INTERLACE     0x0018000D
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_CHECK_BOARD          0x0018000E
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_RED_CYAN             0x0018000F
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_GREEN_PINK           0x00180010
#define AI_3DTV_INPUT_MODE_3DTV_INPUT_BLUE_YELLOW          0x00180011

#define AT_3DTV_OUTPUT_MODE                                0x0019
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_BYPASS             0x00190000
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_TEMPORAL_LRLR      0x00199000
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_TEMPORAL_LRLR_LARGEVS  0x0019000A
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_TEMPORAL_LLRR      0x00190008
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_TEMPORAL_LBRB      0x00190009
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_PR_LINE_INTERLACE  0x00190002
#define AG_3DTV_OUTPUT_MODE_3DTV_OUTPUT_ON                 0x00198000
#define AG_3DTV_OUTPUT_MODE_3DTV_OUTPUT_SG                 0x00198001
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_CHECK_BOARD        0x0019000B
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_RED_CYAN           0x0019000C
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_GREEN_PINK         0x0019000D
#define AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_BLUE_YELLOW        0x0019000E

#define AT_2DDIMMING_MODE                                  0x001B
#define AI_2DDIMMING_MODE_NON_2DDPANEL                     0x001B0000
#define AI_2DDIMMING_MODE_2D_DIMMING                       0x001B0003
#define AI_2DDIMMING_MODE_ZEROD_DIMMING                    0x001B0004
#define AI_2DDIMMING_MODE_FAKED_DIMMING                    0x001B0005
#define AI_2DDIMMING_MODE_OFF                              0x001B0001
#define AG_2DDIMMING_MODE_2DDPANEL                         0x001B8000

#define AT_2DD_SEGSIZE                                     0x0020
#define AI_2DD_SEGSIZE_4_8                                 0x00200009
#define AI_2DD_SEGSIZE_14_16                               0x00200000
#define AI_2DD_SEGSIZE_12_14                               0x00200002
#define AI_2DD_SEGSIZE_16_18                               0x00200003
#define AI_2DD_SEGSIZE_18_16                               0x00200004
#define AI_2DD_SEGSIZE_20X12                               0x0020000A
#define AI_2DD_SEGSIZE_FAKE_12_14                          0x00200005
#define AI_2DD_SEGSIZE_FAKE_14_16                          0x00200006
#define AI_2DD_SEGSIZE_FAKE_16_18                          0x00200007
#define AI_2DD_SEGSIZE_FAKE_18_16                          0x00200008

#define AT_V_INCAP_PIP_COM_RATIO                           0x004F
#define AI_V_INCAP_PIP_COM_RATIO_PACK_1                    0x004F0000
#define AI_V_INCAP_PIP_COM_RATIO_1D_COM_1_5625             0x004F0001

#define AT_V_NR_MP_COM_RATIO                               0x0028
#define AI_V_NR_MP_COM_RATIO_1D_COM_1_5625                 0x00280000
#define AI_V_NR_MP_COM_RATIO_PACK_1                        0x00280001
#define AI_V_NR_MP_COM_RATIO_2D_COM_2_35                   0x00280002

#define AT_V_SCALAR_MP_COM_RATIO                           0x002A
#define AI_V_SCALAR_MP_COM_RATIO_1D_COM_1_5625             0x002A0000
#define AI_V_SCALAR_MP_COM_RATIO_PACK_1                    0x002A0001
#define AI_V_SCALAR_MP_COM_RATIO_2D_COM_2_35               0x002A0002

#define AT_V_FRC_MP_COM_RATIO                              0x0029
#define AI_V_FRC_MP_COM_RATIO_2D_COM_2_35                  0x00290000
#define AI_V_FRC_MP_COM_RATIO_PACK_1                       0x00290001

#define AT_V_PANEL_MP_COM_RATIO                            0x002B
#define AI_V_PANEL_MP_COM_RATIO_1D_COM_1_5625              0x002B0000
#define AI_V_PANEL_MP_COM_RATIO_PACK_1                     0x002B0001

#define AT_SIGNAL_COM_RATIIO                               0x004E
#define AI_SIGNAL_COM_RATIIO_PACK_1                        0x004E0000
#define AI_SIGNAL_COM_RATIIO_BLK_COM_8X2                   0x004E0003
#define AI_SIGNAL_COM_RATIIO_BLK_COM_16X2                  0x004E0004
#define AI_SIGNAL_COM_RATIIO_BLK_COM_8X4                   0x004E0005
#define AI_SIGNAL_COM_RATIIO_BLK_COM_TAIL                  0x004E0006
#define AG_SIGNAL_COM_RATIIO_SIGNAL_COM_ALL                0x004E8001

#define AT_3D_GLASSES_TIMING                               0x002E
#define AI_3D_GLASSES_TIMING_120HZ_74M                     0x002E0009
#define AI_3D_GLASSES_TIMING_120HZ_86M                     0x002E000A
#define AI_3D_GLASSES_TIMING_120HZ_100M                    0x002E000B
#define AI_3D_GLASSES_TIMING_240HZ_VBO                     0x002E000C
#define AI_3D_GLASSES_TIMING_240HZ_LVDS                    0x002E000D
#define AI_3D_GLASSES_TIMING_96HZ_100M                     0x002E000E
#define AI_3D_GLASSES_TIMING_60HZ_74M                      0x002E000F
#define AI_3D_GLASSES_TIMING_240HZ_LVDS_332M               0x002E0010
#define AI_3D_GLASSES_TIMING_120HZ_74M_21_9                0x002E0011

#define AT_2D_TO_3D_MODE                                   0x002F
#define AI_2D_TO_3D_MODE_HW                                0x002F0000
#define AI_2D_TO_3D_MODE_SW                                0x002F0001

#define AT_VBO_BYTES_MODE                                  0x0030
#define AI_VBO_BYTES_MODE_3_BYTES                          0x00300003
#define AI_VBO_BYTES_MODE_4_BYTES                          0x00300004
#define AI_VBO_BYTES_MODE_5_BYTES                          0x00300005

#define AT_VBO_DIVIDER_MODE                                0x0031
#define AI_VBO_DIVIDER_MODE_NORMAL                         0x00310001
#define AI_VBO_DIVIDER_MODE_DIVIDERBY2                     0x00310002
#define AI_VBO_DIVIDER_MODE_DIVIDERBY4                     0x00310003

#define AT_SVP_WORK_MODE                                   0x0032
#define AI_SVP_WORK_MODE_SVP_ASFRONTEND                    0x00320000
#define AI_SVP_WORK_MODE_SVP_STANDALONE                    0x00320001
#define AI_SVP_WORK_MODE_SVP_STB                           0x00320002

#define AT_2D3DMODE                                        0x0033
#define AI_2D3DMODE_2D                                     0x00330000
#define AI_2D3DMODE_3D                                     0x00330001
#define AI_2D3DMODE_2D3D                                   0x00330002

#define AT_21_9_SCALER_MODE                                0x0034
#define AI_21_9_SCALER_MODE_OFF                            0x00340000
#define AI_21_9_SCALER_MODE_ON                             0x00340001

#define AT_LOWLATENCY                                      0x0035
#define AI_LOWLATENCY_OFF                                  0x00350000
#define AI_LOWLATENCY_ON                                   0x00350001

#define AT_POWERSAVINGMODE                                 0x0036
#define AI_POWERSAVINGMODE_OFF                             0x00360000
#define AI_POWERSAVINGMODE_ON                              0x00360001

#define AT_MP_CHANNEL_MEMORY                               0x0037
#define AI_MP_CHANNEL_MEMORY_DTV1                          0x00370000
#define AI_MP_CHANNEL_MEMORY_DTV2                          0x00370001
#define AI_MP_CHANNEL_MEMORY_ATV1                          0x00370002
#define AI_MP_CHANNEL_MEMORY_ATV2                          0x00370003
#define AI_MP_CHANNEL_MEMORY_ATV1_444                      0x00370004
#define AI_MP_CHANNEL_MEMORY_ATV2_444                      0x00370005
#define AI_MP_CHANNEL_MEMORY_NONE                          0x00370006
#define AG_MP_CHANNEL_MEMORY_VIDEO_MEMORY                  0x00378000
#define AG_MP_CHANNEL_MEMORY_PC_MEMORY                     0x00378001

#define AT_PIP_CHANNEL_MEMORY                              0x0038
#define AI_PIP_CHANNEL_MEMORY_DTV1                         0x00380000
#define AI_PIP_CHANNEL_MEMORY_DTV2                         0x00380001
#define AI_PIP_CHANNEL_MEMORY_ATV1                         0x00380002
#define AI_PIP_CHANNEL_MEMORY_ATV2                         0x00380003
#define AI_PIP_CHANNEL_MEMORY_ATV1_444                     0x00380004
#define AI_PIP_CHANNEL_MEMORY_ATV2_444                     0x00380005
#define AI_PIP_CHANNEL_MEMORY_DTV1_444                     0x00380006
#define AI_PIP_CHANNEL_MEMORY_DTV2_444                     0x00380007
#define AI_PIP_CHANNEL_MEMORY_NONE                         0x00380008
#define AG_PIP_CHANNEL_MEMORY_VIDEO_MEMORY                 0x00388000
#define AG_PIP_CHANNEL_MEMORY_PC_MEMORY                    0x00388001

#define AT_VIEW_MODE                                       0x0039
#define AI_VIEW_MODE_MP_ONLY                               0x00390000
#define AI_VIEW_MODE_MP_PIP                                0x00390001
#define AI_VIEW_MODE_MP_PAP                                0x00390002
#define AI_VIEW_MODE_MP_3D                                 0x00390003
#define AG_VIEW_MODE_NEED_PIP                              0x00398000

#define AT_SSC_MODE                                        0x003B
#define AI_SSC_MODE_SSC_OFF                                0x003B0001
#define AI_SSC_MODE_SSC_ON                                 0x003B0002

#define AT_INITIAL_PANEL                                   0x003C
#define AI_INITIAL_PANEL_INITIAL_PANEL_ON                  0x003C0000
#define AI_INITIAL_PANEL_INITIAL_PANEL_OFF                 0x003C0001

#define AT_LVDS_SPEEDMODE                                  0x003D
#define AI_LVDS_SPEEDMODE_HIGH_SPEEDMODE                   0x003D0002
#define AI_LVDS_SPEEDMODE_LOW_SPEEDMODE                    0x003D0003

#define AT_LVDS_OUTPUT                                     0x003E
#define AI_LVDS_OUTPUT_LVDS1_A                             0x003E0002
#define AI_LVDS_OUTPUT_LVDS1_B                             0x003E0003
#define AI_LVDS_OUTPUT_LVDS1                               0x003E0004
#define AI_LVDS_OUTPUT_LVDS2_C                             0x003E0005
#define AI_LVDS_OUTPUT_LVDS2_D                             0x003E0006
#define AI_LVDS_OUTPUT_LVDS2                               0x003E0007
#define AI_LVDS_OUTPUT_DUAL_LVDS                           0x003E0008

#define AT_LVDS_FORMAT                                     0x0069
#define AI_LVDS_FORMAT_VESA_8BIT                           0x00690000
#define AI_LVDS_FORMAT_VESA_10BIT                          0x00690001
#define AI_LVDS_FORMAT_JEIDA_8BIT                          0x00690002
#define AI_LVDS_FORMAT_JEIDA_10BIT                         0x00690003
#define AI_LVDS_FORMAT_VESA_6BIT                           0x00690004
#define AI_LVDS_FORMAT_VESA_9BIT                           0x00690005
#define AI_LVDS_FORMAT_JEIDA_6BIT                          0x00690006
#define AI_LVDS_FORMAT_JEIDA_9BIT                          0x00690007

#define AT_OUTPUT_TO_VBO                                   0x0047
#define AI_OUTPUT_TO_VBO_ON                                0x00470000
#define AI_OUTPUT_TO_VBO_OFF                               0x00470001

#define AT_OUTPUT_TO_HDMI                                  0x0048
#define AI_OUTPUT_TO_HDMI_OFF                              0x00480000
#define AI_OUTPUT_TO_HDMI_ON_HDMI_FROM_PIP                 0x00480001
#define AI_OUTPUT_TO_HDMI_ON_HDMI_FROM_MP                  0x00480002

#define AT_OUTPUT_TO_WINSOR                                0x0049
#define AI_OUTPUT_TO_WINSOR_OFF                            0x00490000
#define AI_OUTPUT_TO_WINSOR_BLENDER_ON                     0x00490001
#define AI_OUTPUT_TO_WINSOR_SCALAR_ON                      0x00490002

#define AT_ENHANCEDVIDEOMODE                               0x003F
#define AI_ENHANCEDVIDEOMODE_SDR                           0x003F0000
#define AI_ENHANCEDVIDEOMODE_EDR                           0x003F0001
#define AI_ENHANCEDVIDEOMODE_HDR10                         0x003F0002
#define AI_ENHANCEDVIDEOMODE_HDR10_SGMIP                   0x003F0003
#define AI_ENHANCEDVIDEOMODE_HDR10PLUS                     0x003F0005
#define AI_ENHANCEDVIDEOMODE_HLG                           0x003F0004
#define AI_ENHANCEDVIDEOMODE_EDR_LL                        0x003F0006
#define AI_ENHANCEDVIDEOMODE_HDR_PSL                       0x003F0007
#define AI_ENHANCEDVIDEOMODE_HDR_ITMO                      0x003F0008
#define AG_ENHANCEDVIDEOMODE_ALL_MODE                      0x003F8000
#define AG_ENHANCEDVIDEOMODE_NORMAL_MODE                   0x003F8001

#define AT_SEPARATEDOSD                                    0x0042
#define AI_SEPARATEDOSD_SEPARATEDOSD_ON                    0x00420000
#define AI_SEPARATEDOSD_SEPARATEDOSD_OFF                   0x00420001

#define AT_VBO_TX_LANES                                    0x0043
#define AI_VBO_TX_LANES_VBO_TX_2_LANES                     0x00430000
#define AI_VBO_TX_LANES_VBO_TX_4_LANES                     0x00430001
#define AI_VBO_TX_LANES_VBO_TX_8_LANES                     0x00430002
#define AI_VBO_TX_LANES_VBO_TX_16_LANES                    0x00430003

#define AT_HDMI_OUTPUT1_COLORSPACE                         0x005B
#define AI_HDMI_OUTPUT1_COLORSPACE_BT709                   0x005B0000
#define AI_HDMI_OUTPUT1_COLORSPACE_BT601                   0x005B0001
#define AI_HDMI_OUTPUT1_COLORSPACE_BT2020_NCLYCC           0x005B0002
#define AI_HDMI_OUTPUT1_COLORSPACE_BT2020_CLYCC            0x005B0003
#define AI_HDMI_OUTPUT1_COLORSPACE_RGB                     0x005B0004

#define AT_HDMI_OUTPUT1_RANGE                              0x005C
#define AI_HDMI_OUTPUT1_RANGE_LIMIT_RANGE                  0x005C0000
#define AI_HDMI_OUTPUT1_RANGE_FULL_RANGE                   0x005C0001

#define AT_FRC_SCHEDULE_MODE                               0x005D
#define AI_FRC_SCHEDULE_MODE_SW                            0x005D0000
#define AI_FRC_SCHEDULE_MODE_HW                            0x005D0001

#define AT_HDMIRX_OUTPUT_CHANNEL                           0x005E
#define AI_HDMIRX_OUTPUT_CHANNEL_MAIN                      0x005E0000
#define AI_HDMIRX_OUTPUT_CHANNEL_SUB                       0x005E0001

#define AT_USER_FORMAT                                     0x0060
#define AI_USER_FORMAT_YUV422_101010                       0x00600000
#define AI_USER_FORMAT_AUTO                                0x00600007
#define AI_USER_FORMAT_RGB_888                             0x00600009
#define AI_USER_FORMAT_RGB_101010                          0x00600001
#define AI_USER_FORMAT_RGB_121212                          0x00600002
#define AI_USER_FORMAT_YUV444_888                          0x00600003
#define AI_USER_FORMAT_YUV444_101010                       0x00600004
#define AI_USER_FORMAT_YUV444_121212                       0x00600005
#define AI_USER_FORMAT_YUV422_888                          0x00600006
#define AI_USER_FORMAT_YUV422_121212                       0x00600008
#define AG_USER_FORMAT_ALL_444                             0x00608000
#define AG_USER_FORMAT_YUV422                              0x00608001
#define AG_USER_FORMAT_ALL_USER_FORMAT                     0x00608002

#define AT_FREESYNCPATH                                    0x0061
#define AI_FREESYNCPATH_OFF                                0x00610000
#define AI_FREESYNCPATH_ON                                 0x00610001

#define AT_HDMI_REPEAT_STATUS                              0x0063
#define AI_HDMI_REPEAT_STATUS_REPEAT_ON                    0x00630000
#define AI_HDMI_REPEAT_STATUS_REPEAT_OFF                   0x00630001

#define AT_SWPLL_VPANEL                                    0x0064
#define AI_SWPLL_VPANEL_HW_PLL                             0x00640000
#define AI_SWPLL_VPANEL_SW_PLL_VTOTAL                      0x00640001
#define AI_SWPLL_VPANEL_SW_PLL_CLOCK                       0x00640002

#define AT_NTSC_PEDESTAL                                   0x006A
#define AI_NTSC_PEDESTAL_ON                                0x006A0000
#define AI_NTSC_PEDESTAL_OFF                               0x006A0001

#define AT_PANEL                                           0x1000
#define AI_PANEL_CMO32_WXGA                                0x10000000
#define AI_PANEL_LG42_1080P                                0x10000001
#define AI_PANEL_LG32_WXGA                                 0x10000002
#define AI_PANEL_AUO42_WXGA120                             0x10000003
#define AI_PANEL_LG42_WXGA120                              0x10000004
#define AI_PANEL_CMO37_1080P                               0x10000005
#define AI_PANEL_CMO20_WXGA                                0x10000006
#define AI_PANEL_LG42_WXGA120_B                            0x10000007
#define AI_PANEL_PHILIP37_WXGA                             0x10000008
#define AI_PANEL_LG42_1080P120                             0x10000009
#define AI_PANEL_LG47_1080P120                             0x1000000A
#define AI_PANEL_SLCD_1080P120                             0x1000000B
#define AI_PANEL_AUO_1080P120                              0x1000000C
#define AI_PANEL_CMO_1080P120                              0x1000000D
#define AI_PANEL_LG42_1080P_WUNSA1                         0x1000000E
#define AI_PANEL_LG42_1080P_VESA10BIT                      0x1000000F
#define AI_PANEL_SEC_LCD_WXGA120                           0x10000010
#define AI_PANEL_SEC_LCD_1080P                             0x10000011
#define AI_PANEL_SEC_LCD_WXGA                              0x10000012
#define AI_PANEL_SEC_PDP_1080P                             0x10000013
#define AI_PANEL_SEC_PDP_WXGA                              0x10000014
#define AI_PANEL_SEC_PDP_XGA                               0x10000015
#define AI_PANEL_SEC_LED_1080P                             0x10000016
#define AI_PANEL_SEC_1600_900                              0x10000017
#define AI_PANEL_LG50_1080P120_21_9_LC500CWD               0x10000018
#define AI_PANEL_SAMSUNG37_VBO_FHD240                      0x10000019
#define AI_PANEL_CMO23_1080P120_VESA8BIT_SPLITMODE         0x1000001A
#define AI_PANEL_SHARP_VBO_FHD240_21_9                     0x1000001B
#define AI_PANEL_SHARP_VBO_FHD240_16_9                     0x1000001C
#define AI_PANEL_CMO_UD60                                  0x1000001D
#define AI_PANEL_AUO55_UD60                                0x1000001E
#define AI_PANEL_AUO65_UD120                               0x10000027
#define AI_PANEL_METZ_UD60_LG43_V16                        0x10000028
#define AI_PANEL_METZ_UD60_LG49_V17                        0x10000029
#define AI_PANEL_METZ_UD60_LG55_V17                        0x1000002A

#define AI_PLATFORM_TV                                     0x1001000B
#define AI_PLATFORM_TV_2UMAC                               0x10010011
#define AI_PLATFORM_STB                                    0x1001000C
#define AI_PLATFORM_TV_FRONTEND                            0x10010012
#define AI_PLATFORM_PROJECTOR                              0x10010013

#define AT_CUSTOMER                                        0x1002
#define AI_CUSTOMER_SDK                                    0x10020000
#define AI_CUSTOMER_INTST                                  0x10020011
#define AI_CUSTOMER_AJTOY                                  0x10020001
#define AI_CUSTOMER_AKTAG                                  0x10020002
#define AI_CUSTOMER_AJTHP                                  0x10020003
#define AI_CUSTOMER_NASOU                                  0x10020008
#define AI_CUSTOMER_ACUPV                                  0x10020009
#define AI_CUSTOMER_EGMOE                                  0x1002000A
#define AI_CUSTOMER_ACIIE                                  0x1002000B
#define AI_CUSTOMER_NAWIO                                  0x1002000D
#define AI_CUSTOMER_ACUCL                                  0x1002000E
#define AI_CUSTOMER_ETWEL                                  0x1002000F
#define AI_CUSTOMER_EGNTZ                                  0x10020010

#define AT_PANELID                                         0x1003
#define AI_PANELID_PANELID_0X0001                          0x10030001
#define AI_PANELID_PANELID_0X0010                          0x10030010
#define AI_PANELID_PANELID_0X0011                          0x10030011
#define AI_PANELID_PANELID_0X0012                          0x10030012
#define AI_PANELID_PANELID_0X0013                          0x10030013
#define AI_PANELID_PANELID_0X0014                          0x10030014
#define AI_PANELID_PANELID_0X0015                          0x10030015
#define AI_PANELID_PANELID_0X0016                          0x10030016
#define AI_PANELID_PANELID_0X0017                          0x10030017
#define AI_PANELID_PANELID_0X0018                          0x10030018
#define AI_PANELID_PANELID_0X0019                          0x10030019
#define AI_PANELID_PANELID_0X001A                          0x1003001A
#define AI_PANELID_PANELID_0X0020                          0x10030020
#define AI_PANELID_PANELID_0X0021                          0x10030021
#define AI_PANELID_PANELID_0X0022                          0x10030022
#define AI_PANELID_PANELID_0X0023                          0x10030023
#define AI_PANELID_PANELID_0X0024                          0x10030024
#define AI_PANELID_PANELID_0X0025                          0x10030025
#define AI_PANELID_PANELID_0X0030                          0x10030030
#define AI_PANELID_PANELID_0X0040                          0x10030040
#define AI_PANELID_PANELID_0X0043                          0x10030043
#define AI_PANELID_PANELID_0X0044                          0x10030044
#define AI_PANELID_PANELID_0X0045                          0x10030045
#define AI_PANELID_PANELID_0X0500                          0x10030500
#define AI_PANELID_PANELID_0X0600                          0x10030600
#define AI_PANELID_PANELID_0X0601                          0x10030601
#define AI_PANELID_PANELID_0X0602                          0x10030602
#define AI_PANELID_PANELID_0X0603                          0x10030603
#define AI_PANELID_PANELID_0X0604                          0x10030604
#define AI_PANELID_PANELID_0X0605                          0x10030605
#define AI_PANELID_PANELID_0X0606                          0x10030606
#define AI_PANELID_PANELID_0X0607                          0x10030607
#define AI_PANELID_PANELID_0X0608                          0x10030608
#define AI_PANELID_PANELID_0X0609                          0x10030609
#define AI_PANELID_PANELID_0X060A                          0x1003060A
#define AI_PANELID_PANELID_0X060B                          0x1003060B
#define AI_PANELID_PANELID_0X0700                          0x10030700
#define AI_PANELID_PANELID_0X0701                          0x10030701
#define AI_PANELID_PANELID_0X0702                          0x10030702
#define AI_PANELID_PANELID_0X0703                          0x10030703
#define AI_PANELID_PANELID_0X0704                          0x10030704
#define AI_PANELID_PANELID_0X0705                          0x10030705
#define AI_PANELID_PANELID_0X0780                          0x10030780
#define AI_PANELID_PANELID_0X0781                          0x10030781
#define AI_PANELID_PANELID_0X0782                          0x10030782
#define AI_PANELID_PANELID_0X07C0                          0x100307C0
#define AI_PANELID_PANELID_0X07C1                          0x100307C1

#define AT_CHIP                                            0x2000
#define AI_CHIP_TV303                                      0x20000009

#define AT_MHEG_OSD                                        0x3000
#define AI_MHEG_OSD_FULL                                   0x30000000
#define AI_MHEG_OSD_TTX_RIGHT                              0x30000001
#define AI_MHEG_OSD_TTX_LEFT                               0x30000002
#define AI_MHEG_OSD_SUBTITLE                               0x30000003
#define AI_MHEG_OSD_MHEG                                   0x30000004

#define AT_UI_PICTUREMODES                                 0x3001
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_VIVID           0x30010000
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_STANDARD        0x30010001
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_MILD            0x30010002
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_GAME            0x30010003
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_CALIBRATED      0x30010005
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_CALIBRATED_DARK 0x30010006
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_COMPUTER        0x30010007
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_CINEMA          0x30010008
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_HOME            0x30010009
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_SPORTS          0x3001000A
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_SHOP            0x3001000B
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_ANIMATION       0x3001000C
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_HDR             0x30010012
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_GRAPHIC         0x30010013
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_EXPERT_1        0x30010014
#define AI_UI_PICTUREMODES_UI_PICTUREMODES_EXPERT_2        0x30010015
#define AG_UI_PICTUREMODES_COMPUTERMODES                   0x30018002
#define AG_UI_PICTUREMODES_NOCOMPUTERMODES                 0x30018000
#define AG_UI_PICTUREMODES_ALL_PICTUREMODES                0x30018001

#define AT_AUTOMODE                                        0x3002
#define AI_AUTOMODE_DETECTSD                               0x30020000
#define AI_AUTOMODE_DETECTHD                               0x30020001
#define AI_AUTOMODE_DETECTXGA                              0x30020002
#define AI_AUTOMODE_DETECTMAC                              0x30020003

#define AT_UI_COLORTEMP                                    0x3003
#define AI_UI_COLORTEMP_UI_COLOURTEMP_COOL                 0x30030000
#define AI_UI_COLORTEMP_UI_COLOURTEMP_WARM                 0x30030001
#define AI_UI_COLORTEMP_UI_COLOURTEMP_NORMAL               0x30030002
#define AI_UI_COLORTEMP_UI_COLOURTEMP_USER                 0x30030003
#define AI_UI_COLORTEMP_UI_COLORTEMP_WARMER                0x30030004
#define AI_UI_COLORTEMP_UI_COLORTEMP_COOLER                0x30030005
#define AI_UI_COLORTEMP_UI_COLORTEMP_EXPERT_1              0x30030006
#define AI_UI_COLORTEMP_UI_COLORTEMP_EXPERT_2              0x30030007
#define AI_UI_COLORTEMP_UI_COLORTEMP_COMPUTER              0x30030008

#define AT_UI_SETPICTURES                                  0x3004
#define AI_UI_SETPICTURES_UI_SETSATURATION                 0x30040000
#define AI_UI_SETPICTURES_UI_SETBRIGHTNESS                 0x30040001
#define AI_UI_SETPICTURES_UI_SETSHARPNESS                  0x30040002
#define AI_UI_SETPICTURES_UI_SETCONTRAST                   0x30040003
#define AI_UI_SETPICTURES_UI_SETTINT                       0x30040004
#define AI_UI_SETPICTURES_UI_SETBACKLIGHT                  0x30040005
#define AG_UI_SETPICTURES_UI_SETPICTURES_ALL               0x30048000

#define AT_UI_SETPRO                                       0x3005
#define AI_UI_SETPRO_UI_SETBSTRETCH                        0x30050000
#define AI_UI_SETPRO_UI_SETGSTRETCH                        0x30050001
#define AI_UI_SETPRO_UI_SETRSTRETCH                        0x30050002
#define AI_UI_SETPRO_UI_SETDCI                             0x30050003
#define AI_UI_SETPRO_UI_SETPROSHARPNESS                    0x30050004
#define AI_UI_SETPRO_UI_SETSKINTONE                        0x30050005
#define AG_UI_SETPRO_UI_SETPRO_ALL                         0x30058000

#define AT_UI_SETCOLORTEMP                                 0x3006
#define AI_UI_SETCOLORTEMP_UI_SETCOLORTEMP_RED             0x30060000
#define AI_UI_SETCOLORTEMP_UI_SETCOLORTEMP_GREEN           0x30060001
#define AI_UI_SETCOLORTEMP_UI_SETCOLORTEMP_BLUE            0x30060002

#define AT_WINDOWCFG                                       0x3007
#define AI_WINDOWCFG_MOTIONWINDOW1                         0x30070000
#define AI_WINDOWCFG_MOTIONWINDOW2                         0x30070001
#define AI_WINDOWCFG_MOTIONWINDOW3                         0x30070002
#define AI_WINDOWCFG_APLDCLWIN                             0x30070003
#define AI_WINDOWCFG_MEMCTOPWIN                            0x30070004
#define AI_WINDOWCFG_NOFBWIN                               0x30070005
#define AI_WINDOWCFG_SRCCRTCWIN                            0x30070006
#define AI_WINDOWCFG_MPEGCRTCWIN                           0x30070007
#define AI_WINDOWCFG_PIPAPLDCLWIN                          0x30070008
#define AI_WINDOWCFG_BWEXTWIN                              0x30070009
#define AI_WINDOWCFG_PIPBWEXTWIN                           0x3007000A
#define AI_WINDOWCFG_PPMOTIONWINDOW1                       0x3007000C
#define AI_WINDOWCFG_PPMOTIONWINDOW2                       0x3007000D
#define AI_WINDOWCFG_PPMOTIONWINDOW3                       0x3007000E
#define AI_WINDOWCFG_MPMEASWIN                             0x3007000F
#define AI_WINDOWCFG_PPMEASWIN                             0x30070010
#define AI_WINDOWCFG_APLDEMOWIN                            0x30070011
#define AI_WINDOWCFG_PEAKINGLOCODEMOWIN                    0x30070012
#define AI_WINDOWCFG_NRDEMOWIN                             0x30070013
#define AI_WINDOWCFG_MNRDEMOWIN                            0x30070014
#define AI_WINDOWCFG_DEINTERLACEDEMOWIN                    0x30070015

#define AT_UI_ADVPICTURES                                  0x3008
#define AI_UI_ADVPICTURES_UI_OFF                           0x30080000
#define AI_UI_ADVPICTURES_UI_LOW                           0x30080001
#define AI_UI_ADVPICTURES_UI_MEDIUM                        0x30080002
#define AI_UI_ADVPICTURES_UI_HIGH                          0x30080003
#define AI_UI_ADVPICTURES_UI_AUTO                          0x30080004

#define AT_CRT_LOCKONOFF                                   0x3009
#define AI_CRT_LOCKONOFF_50P                               0x30090000
#define AI_CRT_LOCKONOFF_60P                               0x30090001
#define AI_CRT_LOCKONOFF_50P_FREERUN                       0x30090002
#define AI_CRT_LOCKONOFF_60P_FREERUN                       0x30090003

#define AT_OSDWINDOW                                       0x300A
#define AI_OSDWINDOW_MENU                                  0x300A0000
#define AI_OSDWINDOW_SUBTITLE                              0x300A0001
#define AI_OSDWINDOW_TTX                                   0x300A0002
#define AI_OSDWINDOW_MHEG                                  0x300A0003

#define AT_SEQUENCE                                        0x300B
#define AI_SEQUENCE_0                                      0x300B0000
#define AI_SEQUENCE_1                                      0x300B0001
#define AI_SEQUENCE_2                                      0x300B0002
#define AI_SEQUENCE_3                                      0x300B0003
#define AI_SEQUENCE_4                                      0x300B0004
#define AI_SEQUENCE_5                                      0x300B0005
#define AI_SEQUENCE_6                                      0x300B0006
#define AI_SEQUENCE_7                                      0x300B0007
#define AI_SEQUENCE_8                                      0x300B0008
#define AI_SEQUENCE_9                                      0x300B0009
#define AI_SEQUENCE_10                                     0x300B000A
#define AI_SEQUENCE_11                                     0x300B000B
#define AI_SEQUENCE_12                                     0x300B000C
#define AI_SEQUENCE_13                                     0x300B000D
#define AI_SEQUENCE_14                                     0x300B000E
#define AI_SEQUENCE_15                                     0x300B000F
#define AI_SEQUENCE_16                                     0x300B0010
#define AI_SEQUENCE_17                                     0x300B0011
#define AI_SEQUENCE_18                                     0x300B0012
#define AI_SEQUENCE_19                                     0x300B0013
#define AI_SEQUENCE_20                                     0x300B0014
#define AI_SEQUENCE_21                                     0x300B0015
#define AI_SEQUENCE_22                                     0x300B0016
#define AI_SEQUENCE_23                                     0x300B0017
#define AI_SEQUENCE_24                                     0x300B0018
#define AI_SEQUENCE_25                                     0x300B0019
#define AI_SEQUENCE_26                                     0x300B001A
#define AI_SEQUENCE_27                                     0x300B001B
#define AI_SEQUENCE_28                                     0x300B001C
#define AI_SEQUENCE_29                                     0x300B001D
#define AI_SEQUENCE_30                                     0x300B001E
#define AI_SEQUENCE_31                                     0x300B001F
#define AI_SEQUENCE_32                                     0x300B0020
#define AI_SEQUENCE_33                                     0x300B0021
#define AI_SEQUENCE_34                                     0x300B0022
#define AI_SEQUENCE_35                                     0x300B0023
#define AI_SEQUENCE_36                                     0x300B0024
#define AI_SEQUENCE_37                                     0x300B0025
#define AI_SEQUENCE_38                                     0x300B0026
#define AI_SEQUENCE_39                                     0x300B0027
#define AI_SEQUENCE_40                                     0x300B0028
#define AI_SEQUENCE_41                                     0x300B0029
#define AI_SEQUENCE_42                                     0x300B002A
#define AI_SEQUENCE_43                                     0x300B002B
#define AI_SEQUENCE_44                                     0x300B002C
#define AI_SEQUENCE_45                                     0x300B002D
#define AI_SEQUENCE_46                                     0x300B002E
#define AI_SEQUENCE_47                                     0x300B002F
#define AI_SEQUENCE_48                                     0x300B0030
#define AI_SEQUENCE_49                                     0x300B0031
#define AI_SEQUENCE_50                                     0x300B0032
#define AI_SEQUENCE_51                                     0x300B0033
#define AI_SEQUENCE_52                                     0x300B0034
#define AI_SEQUENCE_53                                     0x300B0035
#define AI_SEQUENCE_54                                     0x300B0036
#define AI_SEQUENCE_55                                     0x300B0037
#define AI_SEQUENCE_56                                     0x300B0038
#define AI_SEQUENCE_57                                     0x300B0039
#define AI_SEQUENCE_58                                     0x300B003A
#define AI_SEQUENCE_59                                     0x300B003B
#define AI_SEQUENCE_60                                     0x300B003C
#define AI_SEQUENCE_61                                     0x300B003D
#define AI_SEQUENCE_62                                     0x300B003E
#define AI_SEQUENCE_63                                     0x300B003F
#define AI_SEQUENCE_64                                     0x300B0040
#define AI_SEQUENCE_65                                     0x300B0041
#define AI_SEQUENCE_66                                     0x300B0042
#define AI_SEQUENCE_67                                     0x300B0043
#define AI_SEQUENCE_68                                     0x300B0044
#define AI_SEQUENCE_69                                     0x300B0045
#define AI_SEQUENCE_70                                     0x300B0046
#define AI_SEQUENCE_71                                     0x300B0047
#define AI_SEQUENCE_72                                     0x300B0048
#define AI_SEQUENCE_73                                     0x300B0049
#define AI_SEQUENCE_74                                     0x300B004A
#define AI_SEQUENCE_75                                     0x300B004B
#define AI_SEQUENCE_76                                     0x300B004C
#define AI_SEQUENCE_77                                     0x300B004D
#define AI_SEQUENCE_78                                     0x300B004E
#define AI_SEQUENCE_79                                     0x300B004F
#define AI_SEQUENCE_80                                     0x300B0050
#define AI_SEQUENCE_81                                     0x300B0051
#define AI_SEQUENCE_82                                     0x300B0052
#define AI_SEQUENCE_83                                     0x300B0053
#define AI_SEQUENCE_84                                     0x300B0054
#define AI_SEQUENCE_85                                     0x300B0055
#define AI_SEQUENCE_86                                     0x300B0056
#define AI_SEQUENCE_87                                     0x300B0057
#define AI_SEQUENCE_88                                     0x300B0058
#define AI_SEQUENCE_89                                     0x300B0059
#define AI_SEQUENCE_90                                     0x300B005A
#define AI_SEQUENCE_91                                     0x300B005B
#define AI_SEQUENCE_92                                     0x300B005C
#define AI_SEQUENCE_93                                     0x300B005D
#define AI_SEQUENCE_94                                     0x300B005E
#define AI_SEQUENCE_95                                     0x300B005F
#define AI_SEQUENCE_96                                     0x300B0060
#define AI_SEQUENCE_97                                     0x300B0061
#define AI_SEQUENCE_98                                     0x300B0062
#define AI_SEQUENCE_99                                     0x300B0063
#define AI_SEQUENCE_100                                    0x300B0064
#define AI_SEQUENCE_101                                    0x300B0065
#define AI_SEQUENCE_102                                    0x300B0066
#define AI_SEQUENCE_103                                    0x300B0067
#define AI_SEQUENCE_104                                    0x300B0068
#define AI_SEQUENCE_105                                    0x300B0069
#define AI_SEQUENCE_106                                    0x300B006A
#define AI_SEQUENCE_107                                    0x300B006B
#define AI_SEQUENCE_108                                    0x300B006C
#define AI_SEQUENCE_109                                    0x300B006D
#define AI_SEQUENCE_110                                    0x300B006E
#define AI_SEQUENCE_111                                    0x300B006F
#define AI_SEQUENCE_112                                    0x300B0070
#define AI_SEQUENCE_113                                    0x300B0071
#define AI_SEQUENCE_114                                    0x300B0072
#define AI_SEQUENCE_115                                    0x300B0073
#define AI_SEQUENCE_116                                    0x300B0074
#define AI_SEQUENCE_117                                    0x300B0075
#define AI_SEQUENCE_118                                    0x300B0076
#define AI_SEQUENCE_119                                    0x300B0077
#define AI_SEQUENCE_120                                    0x300B0078
#define AI_SEQUENCE_121                                    0x300B0079
#define AI_SEQUENCE_122                                    0x300B007A
#define AI_SEQUENCE_123                                    0x300B007B
#define AI_SEQUENCE_124                                    0x300B007C
#define AI_SEQUENCE_125                                    0x300B007D
#define AI_SEQUENCE_126                                    0x300B007E
#define AI_SEQUENCE_127                                    0x300B007F
#define AI_SEQUENCE_128                                    0x300B0080
#define AI_SEQUENCE_129                                    0x300B0081
#define AI_SEQUENCE_130                                    0x300B0082
#define AI_SEQUENCE_131                                    0x300B0083
#define AI_SEQUENCE_132                                    0x300B0084
#define AI_SEQUENCE_133                                    0x300B0085
#define AI_SEQUENCE_134                                    0x300B0086
#define AI_SEQUENCE_135                                    0x300B0087
#define AI_SEQUENCE_136                                    0x300B0088
#define AI_SEQUENCE_137                                    0x300B0089
#define AI_SEQUENCE_138                                    0x300B008A
#define AI_SEQUENCE_139                                    0x300B008B
#define AI_SEQUENCE_140                                    0x300B008C
#define AI_SEQUENCE_141                                    0x300B008D
#define AI_SEQUENCE_142                                    0x300B008E
#define AI_SEQUENCE_143                                    0x300B008F
#define AI_SEQUENCE_144                                    0x300B0090
#define AI_SEQUENCE_145                                    0x300B0091
#define AI_SEQUENCE_146                                    0x300B0092
#define AI_SEQUENCE_147                                    0x300B0093
#define AI_SEQUENCE_148                                    0x300B0094
#define AI_SEQUENCE_149                                    0x300B0095
#define AI_SEQUENCE_150                                    0x300B0096
#define AI_SEQUENCE_151                                    0x300B0097
#define AI_SEQUENCE_152                                    0x300B0098
#define AI_SEQUENCE_153                                    0x300B0099
#define AI_SEQUENCE_154                                    0x300B009A
#define AI_SEQUENCE_155                                    0x300B009B
#define AI_SEQUENCE_156                                    0x300B009C
#define AI_SEQUENCE_157                                    0x300B009D
#define AI_SEQUENCE_158                                    0x300B009E
#define AI_SEQUENCE_159                                    0x300B009F
#define AI_SEQUENCE_160                                    0x300B00A0
#define AI_SEQUENCE_161                                    0x300B00A1
#define AI_SEQUENCE_162                                    0x300B00A2
#define AI_SEQUENCE_163                                    0x300B00A3
#define AI_SEQUENCE_164                                    0x300B00A4
#define AI_SEQUENCE_165                                    0x300B00A5
#define AI_SEQUENCE_166                                    0x300B00A6
#define AI_SEQUENCE_167                                    0x300B00A7
#define AI_SEQUENCE_168                                    0x300B00A8
#define AI_SEQUENCE_169                                    0x300B00A9
#define AI_SEQUENCE_170                                    0x300B00AA
#define AI_SEQUENCE_171                                    0x300B00AB
#define AI_SEQUENCE_172                                    0x300B00AC
#define AI_SEQUENCE_173                                    0x300B00AD
#define AI_SEQUENCE_174                                    0x300B00AE
#define AI_SEQUENCE_175                                    0x300B00AF
#define AI_SEQUENCE_176                                    0x300B00B0
#define AI_SEQUENCE_177                                    0x300B00B1
#define AI_SEQUENCE_178                                    0x300B00B2
#define AI_SEQUENCE_179                                    0x300B00B3
#define AI_SEQUENCE_180                                    0x300B00B4
#define AI_SEQUENCE_181                                    0x300B00B5
#define AI_SEQUENCE_182                                    0x300B00B6
#define AI_SEQUENCE_183                                    0x300B00B7
#define AI_SEQUENCE_184                                    0x300B00B8
#define AI_SEQUENCE_185                                    0x300B00B9
#define AI_SEQUENCE_186                                    0x300B00BA
#define AI_SEQUENCE_187                                    0x300B00BB
#define AI_SEQUENCE_188                                    0x300B00BC
#define AI_SEQUENCE_189                                    0x300B00BD
#define AI_SEQUENCE_190                                    0x300B00BE
#define AI_SEQUENCE_191                                    0x300B00BF
#define AI_SEQUENCE_192                                    0x300B00C0
#define AI_SEQUENCE_193                                    0x300B00C1
#define AI_SEQUENCE_194                                    0x300B00C2
#define AI_SEQUENCE_195                                    0x300B00C3
#define AI_SEQUENCE_196                                    0x300B00C4
#define AI_SEQUENCE_197                                    0x300B00C5
#define AI_SEQUENCE_198                                    0x300B00C6
#define AI_SEQUENCE_199                                    0x300B00C7
#define AI_SEQUENCE_200                                    0x300B00C8
#define AI_SEQUENCE_201                                    0x300B00C9
#define AI_SEQUENCE_202                                    0x300B00CA
#define AI_SEQUENCE_203                                    0x300B00CB
#define AI_SEQUENCE_204                                    0x300B00CC
#define AI_SEQUENCE_205                                    0x300B00CD
#define AI_SEQUENCE_206                                    0x300B00CE
#define AI_SEQUENCE_207                                    0x300B00CF
#define AI_SEQUENCE_208                                    0x300B00D0
#define AI_SEQUENCE_209                                    0x300B00D1
#define AI_SEQUENCE_210                                    0x300B00D2
#define AI_SEQUENCE_211                                    0x300B00D3
#define AI_SEQUENCE_212                                    0x300B00D4
#define AI_SEQUENCE_213                                    0x300B00D5
#define AI_SEQUENCE_214                                    0x300B00D6
#define AI_SEQUENCE_215                                    0x300B00D7
#define AI_SEQUENCE_216                                    0x300B00D8
#define AI_SEQUENCE_217                                    0x300B00D9
#define AI_SEQUENCE_218                                    0x300B00DA
#define AI_SEQUENCE_219                                    0x300B00DB
#define AI_SEQUENCE_220                                    0x300B00DC
#define AI_SEQUENCE_221                                    0x300B00DD
#define AI_SEQUENCE_222                                    0x300B00DE
#define AI_SEQUENCE_223                                    0x300B00DF
#define AI_SEQUENCE_224                                    0x300B00E0
#define AI_SEQUENCE_225                                    0x300B00E1

#define AT_MPPIP_CHANNEL                                   0x300D
#define AI_MPPIP_CHANNEL_MPEG_MPEG                         0x300D0000
#define AI_MPPIP_CHANNEL_MPEG_TV                           0x300D0001
#define AI_MPPIP_CHANNEL_MPEG_PC                           0x300D0002
#define AI_MPPIP_CHANNEL_MPEG_SCART                        0x300D0003
#define AI_MPPIP_CHANNEL_MPEG_HDTV                         0x300D0004
#define AI_MPPIP_CHANNEL_MPEG_SVIDEO                       0x300D0005
#define AI_MPPIP_CHANNEL_HDMI_CVBS                         0x300D0006
#define AI_MPPIP_CHANNEL_HDMI_MPEG                         0x300D0007
#define AI_MPPIP_CHANNEL_HDMI_TV                           0x300D0008
#define AI_MPPIP_CHANNEL_HDMI_PC                           0x300D0009
#define AI_MPPIP_CHANNEL_HDMI_SCART                        0x300D000A
#define AI_MPPIP_CHANNEL_HDMI_HDTV                         0x300D000B
#define AI_MPPIP_CHANNEL_HDMI_SVIDEO                       0x300D000C
#define AI_MPPIP_CHANNEL_TV_PC                             0x300D000D
#define AI_MPPIP_CHANNEL_TV_SCART                          0x300D000E
#define AI_MPPIP_CHANNEL_TV_HDTV                           0x300D000F
#define AI_MPPIP_CHANNEL_TV_SVIDEO                         0x300D0010
#define AI_MPPIP_CHANNEL_TV_CVBS                           0x300D0011

#define AT_HDMITX_SIGNALPATH                               0x300E
#define AI_HDMITX_SIGNALPATH_VIDEOSOURCE                   0x300E0000
#define AI_HDMITX_SIGNALPATH_HDMIRX                        0x300E0001
#define AI_HDMITX_SIGNALPATH_V_BLENDER_PIP                 0x300E0002
#define AI_HDMITX_SIGNALPATH_V_BLENDER_EDR                 0x300E0003
#define AI_HDMITX_SIGNALPATH_V_BLENDER_VIDEO               0x300E0004
#define AI_HDMITX_SIGNALPATH_HDMIRX1                       0x300E0005

#define AT_HDMITX_SIGNALTYPE                               0x300F
#define AI_HDMITX_SIGNALTYPE_1080P60                       0x300F0000
#define AI_HDMITX_SIGNALTYPE_720P60                        0x300F0001
#define AI_HDMITX_SIGNALTYPE_720P50                        0x300F0002
#define AI_HDMITX_SIGNALTYPE_576P60                        0x300F0003
#define AI_HDMITX_SIGNALTYPE_576P50                        0x300F0004
#define AI_HDMITX_SIGNALTYPE_480P60                        0x300F0005
#define AI_HDMITX_SIGNALTYPE_4K2KP24                       0x300F0006
#define AI_HDMITX_SIGNALTYPE_2160P24                       0x300F0007
#define AI_HDMITX_SIGNALTYPE_4K2KP30                       0x300F0008
#define AI_HDMITX_SIGNALTYPE_2160P30                       0x300F0009
#define AI_HDMITX_SIGNALTYPE_4K2KP60                       0x300F000A
#define AI_HDMITX_SIGNALTYPE_2160P60                       0x300F000B
#define AI_HDMITX_SIGNALTYPE_1080P50                       0x300F000C
#define AI_HDMITX_SIGNALTYPE_4K2KP50                       0x300F000D
#define AI_HDMITX_SIGNALTYPE_2160P50                       0x300F000E
#define AI_HDMITX_SIGNALTYPE_4K2KP25                       0x300F000F
#define AI_HDMITX_SIGNALTYPE_2160P25                       0x300F0010

#define AT_HDMITX_COLORBITDEPTH                            0x3010
#define AI_HDMITX_COLORBITDEPTH_8BIT                       0x30100000
#define AI_HDMITX_COLORBITDEPTH_10BIT                      0x30100001
#define AI_HDMITX_COLORBITDEPTH_12BIT                      0x30100002

#define AT_HDMITX_INPUTCOLORSPACE                          0x3011
#define AI_HDMITX_INPUTCOLORSPACE_RGB                      0x30110000
#define AI_HDMITX_INPUTCOLORSPACE_YCBCR444                 0x30110001
#define AI_HDMITX_INPUTCOLORSPACE_YCBCR422                 0x30110002
#define AI_HDMITX_INPUTCOLORSPACE_YCBCR420                 0x30110003

#define AT_HDMITX_OUTPUTCOLORSPACE                         0x3012
#define AI_HDMITX_OUTPUTCOLORSPACE_RGB                     0x30120000
#define AI_HDMITX_OUTPUTCOLORSPACE_YCBCR444                0x30120001
#define AI_HDMITX_OUTPUTCOLORSPACE_YCBCR422                0x30120002
#define AI_HDMITX_OUTPUTCOLORSPACE_YCBCR420                0x30120003

#define AT_UI_TNR                                          0x3013
#define AI_UI_TNR_UI_TNR_OFF                               0x30130000
#define AI_UI_TNR_UI_TNR_LOW                               0x30130001
#define AI_UI_TNR_UI_TNR_MEDIUM                            0x30130002
#define AI_UI_TNR_UI_TNR_HIGH                              0x30130003
#define AI_UI_TNR_UI_TNR_AUTO                              0x30130004
#define AI_UI_TNR_UI_TNR_LOWER                             0x30130005
#define AI_UI_TNR_UI_TNR_HIGHER                            0x30130006
#define AI_UI_TNR_UI_TNR_AUTO_LOW                          0x30130007
#define AI_UI_TNR_UI_TNR_AUTO_MEDIUM                       0x30130008
#define AI_UI_TNR_UI_TNR_AUTO_HIGH                         0x30130009
#define AI_UI_TNR_UI_TNR_AUTO_LOWER                        0x3013000A
#define AI_UI_TNR_UI_TNR_AUTO_HIGHER                       0x3013000B

#define AT_UI_SNR                                          0x3014
#define AI_UI_SNR_UI_SNR_OFF                               0x30140000
#define AI_UI_SNR_UI_SNR_LOW                               0x30140001
#define AI_UI_SNR_UI_SNR_MEDIUM                            0x30140002
#define AI_UI_SNR_UI_SNR_HIGH                              0x30140003
#define AI_UI_SNR_UI_SNR_AUTO                              0x30140004
#define AI_UI_SNR_UI_SNR_LOWER                             0x30140005
#define AI_UI_SNR_UI_SNR_HIGHER                            0x30140006
#define AI_UI_SNR_UI_SNR_AUTO_LOW                          0x30140007
#define AI_UI_SNR_UI_SNR_AUTO_MEDIUM                       0x30140008
#define AI_UI_SNR_UI_SNR_AUTO_HIGH                         0x30140009
#define AI_UI_SNR_UI_SNR_AUTO_LOWER                        0x3014000A
#define AI_UI_SNR_UI_SNR_AUTO_HIGHER                       0x3014000B

#define AT_UI_BLACKEXTENSION                               0x3015
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_OFF         0x30150000
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_LOW         0x30150001
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_MEDIUM      0x30150002
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_HIGH        0x30150003
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_AUTO        0x30150004
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_LOWER       0x30150005
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_HIGHER      0x30150006
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_AUTO_LOW    0x30150007
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_AUTO_MEDIUM 0x30150008
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_AUTO_HIGH   0x30150009
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_AUTO_LOWER  0x3015000A
#define AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_AUTO_HIGHER 0x3015000B

#define AT_UI_WHITEEXTENSION                               0x3016
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_OFF         0x30160000
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_LOW         0x30160001
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_MEDIUM      0x30160002
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_HIGH        0x30160003
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_AUTO        0x30160004
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_LOWER       0x30160005
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_HIGHER      0x30160006
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_AUTO_LOW    0x30160007
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_AUTO_MEDIUM 0x30160008
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_AUTO_HIGH   0x30160009
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_AUTO_LOWER  0x3016000A
#define AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_AUTO_HIGHER 0x3016000B

#define AT_UI_PURECINEMA                                   0x3017
#define AI_UI_PURECINEMA_UI_PURECINEMA_OFF                 0x30170000
#define AI_UI_PURECINEMA_UI_PURECINEMA_ON                  0x30170001

#define AT_UI_BLUESTRECH                                   0x3018
#define AI_UI_BLUESTRECH_UI_BLUESTRECH_OFF                 0x30180000
#define AI_UI_BLUESTRECH_UI_BLUESTRECH_LOW                 0x30180001
#define AI_UI_BLUESTRECH_UI_BLUESTRECH_MEDIUM              0x30180002
#define AI_UI_BLUESTRECH_UI_BLUESTRECH_HIGH                0x30180003
#define AI_UI_BLUESTRECH_UI_BLUESTRECH_LOWER               0x30180004
#define AI_UI_BLUESTRECH_UI_BLUESTRECH_HIGHER              0x30180005

#define AT_UI_SKINTONE                                     0x3019
#define AI_UI_SKINTONE_UI_SKINTONE_MEDIUM_OFF              0x30190000
#define AI_UI_SKINTONE_UI_SKINTONE_LOW                     0x30190001
#define AI_UI_SKINTONE_UI_SKINTONE_HIGH                    0x30190002
#define AI_UI_SKINTONE_UI_SKINTONE_LOWER                   0x30190003
#define AI_UI_SKINTONE_UI_SKINTONE_HIGHER                  0x30190004

#define AT_UI_DCI                                          0x301A
#define AI_UI_DCI_UI_DCI_OFF                               0x301A0000
#define AI_UI_DCI_UI_DCI_LOW                               0x301A0001
#define AI_UI_DCI_UI_DCI_MEDIUM                            0x301A0002
#define AI_UI_DCI_UI_DCI_HIGH                              0x301A0003
#define AI_UI_DCI_UI_DCI_AUTO                              0x301A0004
#define AI_UI_DCI_UI_DCI_LOWER                             0x301A0005
#define AI_UI_DCI_UI_DCI_HIGHER                            0x301A0006

#define AT_UI_VIDEOMODE                                    0x301B
#define AI_UI_VIDEOMODE_UI_VIDEOMODE_ON                    0x301B0000
#define AI_UI_VIDEOMODE_UI_COMPUTERMODE_ON                 0x301B0001
#define AG_UI_VIDEOMODE_ALL_VIDEOMODES                     0x301B8000


typedef struct tagStructTFDAttrItem {
	unsigned int    dwAttrID;
	unsigned int    dwAttrItemID;
} TFDAttrItem;

#ifdef TFDATTRLIST_DECLARE
#undef TFDATTRLIST_DECLARE
const TFDAttrItem    TFDAttrList[] = {
	{AT_SINGLE,                               AI_SINGLE_DEFAULT},
	{AT_SIGNAL_CHANNEL,                       AI_SIGNAL_CHANNEL_MPEG1},
	{AT_SIGNAL_MODE,                          AI_SIGNAL_MODE_1080P},
	{AT_SIGNAL_FRAMERATE,                     AI_SIGNAL_FRAMERATE_60},
	{AT_SIGNAL_FORMAT,                        AI_SIGNAL_FORMAT_YUV422_101010},
	{AT_SIGNAL_RANGE,                         AI_SIGNAL_RANGE_LIMIT_RANGE},
	{AT_SIGNAL_COLORSPACE,                    AI_SIGNAL_COLORSPACE_BT709},
	{AT_VINCAP_ICSC,                          AI_VINCAP_ICSC_BYPASS},
	{AT_VPROC_CSC1,                           AI_VPROC_CSC1_BYPASS},
	{AT_VPROC_ICSC2,                          AI_VPROC_ICSC2_BYPASS},
	{AT_VPROC_CSC3,                           AI_VPROC_CSC3_BYPASS},
	{AT_V_INCAP_MP_FORMAT,                    AI_V_INCAP_MP_FORMAT_YUV422_101010},
	{AT_V_INCAP_PIP_FORMAT,                   AI_V_INCAP_PIP_FORMAT_YUV422_101010},
	{AT_V_NR_MP_FORMAT,                       AI_V_NR_MP_FORMAT_YUV422_101010},
	{AT_V_DETN_MP_FORMAT,                     AI_V_DETN_MP_FORMAT_YUV422_101010},
	{AT_V_SCALAR_MP_FORMAT,                   AI_V_SCALAR_MP_FORMAT_YUV422_101010},
	{AT_V_PROC_MP_FORMAT,                     AI_V_PROC_MP_FORMAT_YUV422_101010},
	{AT_V_BLEND_MP_FORMAT,                    AI_V_BLEND_MP_FORMAT_YUV422_101010},
	{AT_V_BLEND_PIP_FORMAT,                   AI_V_BLEND_PIP_FORMAT_YUV422_101010},
	{AT_V_FRC_MP_FORMAT,                      AI_V_FRC_MP_FORMAT_YUV422_101010},
	{AT_V_PANEL_MP_FORMAT,                    AI_V_PANEL_MP_FORMAT_YUV422_101010},
	{AT_MPPIP,                                AI_MPPIP_MP},
	{AT_OUTPUT_RESOLUTION,                    AI_OUTPUT_RESOLUTION_3840_2160},
	{AT_OUTPUT_FRAMERATE,                     AI_OUTPUT_FRAMERATE_120HZ},
	{AT_OUTPUT_MAXFRAMERATE,                  AI_OUTPUT_MAXFRAMERATE_60HZ},
	{AT_OUTPUT_TIMINGMODE,                    AI_OUTPUT_TIMINGMODE_FIXED_HTOTAL_ONLY},
	{AT_MEMORY_SIZE,                          AI_MEMORY_SIZE_16M},
	{AT_WINDOW,                               AI_WINDOW__MPS_169_SUPERWIDE_0},
	{AT_CVBSOUT_INPUT_CHANNEL,                AI_CVBSOUT_INPUT_CHANNEL_HDTV1},
	{AT_CVBSOUT_OUTPUT_SIGNAL,                AI_CVBSOUT_OUTPUT_SIGNAL_PAL50},
	{AT_CVBSOUT_OUTPUT_CHANNEL,               AI_CVBSOUT_OUTPUT_CHANNEL_CVBSOUT1},
	{AT_SOG,                                  AI_SOG_SOG_FALSE},
	{AT_MIRRORTYPE,                           AI_MIRRORTYPE_HVNOMIRROR},
	{AT_3DTV_INPUT_MODE,                      AI_3DTV_INPUT_MODE_3DTV_INPUT_OFF},
	{AT_3DTV_OUTPUT_MODE,                     AI_3DTV_OUTPUT_MODE_3DTV_OUTPUT_BYPASS},
	{AT_2DDIMMING_MODE,                       AI_2DDIMMING_MODE_NON_2DDPANEL},
	{AT_2DD_SEGSIZE,                          AI_2DD_SEGSIZE_4_8},
	{AT_V_INCAP_PIP_COM_RATIO,                AI_V_INCAP_PIP_COM_RATIO_PACK_1},
	{AT_V_NR_MP_COM_RATIO,                    AI_V_NR_MP_COM_RATIO_1D_COM_1_5625},
	{AT_V_SCALAR_MP_COM_RATIO,                AI_V_SCALAR_MP_COM_RATIO_1D_COM_1_5625},
	{AT_V_FRC_MP_COM_RATIO,                   AI_V_FRC_MP_COM_RATIO_2D_COM_2_35},
	{AT_V_PANEL_MP_COM_RATIO,                 AI_V_PANEL_MP_COM_RATIO_1D_COM_1_5625},
	{AT_SIGNAL_COM_RATIIO,                    AI_SIGNAL_COM_RATIIO_PACK_1},
	{AT_3D_GLASSES_TIMING,                    AI_3D_GLASSES_TIMING_120HZ_74M},
	{AT_2D_TO_3D_MODE,                        AI_2D_TO_3D_MODE_HW},
	{AT_VBO_BYTES_MODE,                       AI_VBO_BYTES_MODE_3_BYTES},
	{AT_VBO_DIVIDER_MODE,                     AI_VBO_DIVIDER_MODE_NORMAL},
	{AT_SVP_WORK_MODE,                        AI_SVP_WORK_MODE_SVP_ASFRONTEND},
	{AT_2D3DMODE,                             AI_2D3DMODE_2D},
	{AT_21_9_SCALER_MODE,                     AI_21_9_SCALER_MODE_OFF},
	{AT_LOWLATENCY,                           AI_LOWLATENCY_OFF},
	{AT_POWERSAVINGMODE,                      AI_POWERSAVINGMODE_OFF},
	{AT_MP_CHANNEL_MEMORY,                    AI_MP_CHANNEL_MEMORY_DTV1},
	{AT_PIP_CHANNEL_MEMORY,                   AI_PIP_CHANNEL_MEMORY_DTV1},
	{AT_VIEW_MODE,                            AI_VIEW_MODE_MP_ONLY},
	{AT_SSC_MODE,                             AI_SSC_MODE_SSC_OFF},
	{AT_INITIAL_PANEL,                        AI_INITIAL_PANEL_INITIAL_PANEL_ON},
	{AT_LVDS_SPEEDMODE,                       AI_LVDS_SPEEDMODE_HIGH_SPEEDMODE},
	{AT_LVDS_OUTPUT,                          AI_LVDS_OUTPUT_LVDS1_A},
	{AT_LVDS_FORMAT,                          AI_LVDS_FORMAT_VESA_8BIT},
	{AT_OUTPUT_TO_VBO,                        AI_OUTPUT_TO_VBO_ON},
	{AT_OUTPUT_TO_HDMI,                       AI_OUTPUT_TO_HDMI_OFF},
	{AT_OUTPUT_TO_WINSOR,                     AI_OUTPUT_TO_WINSOR_OFF},
	{AT_ENHANCEDVIDEOMODE,                    AI_ENHANCEDVIDEOMODE_SDR},
	{AT_SEPARATEDOSD,                         AI_SEPARATEDOSD_SEPARATEDOSD_ON},
	{AT_VBO_TX_LANES,                         AI_VBO_TX_LANES_VBO_TX_2_LANES},
	{AT_HDMI_OUTPUT1_COLORSPACE,              AI_HDMI_OUTPUT1_COLORSPACE_BT709},
	{AT_HDMI_OUTPUT1_RANGE,                   AI_HDMI_OUTPUT1_RANGE_LIMIT_RANGE},
	{AT_FRC_SCHEDULE_MODE,                    AI_FRC_SCHEDULE_MODE_SW},
	{AT_HDMIRX_OUTPUT_CHANNEL,                AI_HDMIRX_OUTPUT_CHANNEL_MAIN},
	{AT_USER_FORMAT,                          AI_USER_FORMAT_YUV422_101010},
	{AT_FREESYNCPATH,                         AI_FREESYNCPATH_OFF},
	{AT_HDMI_REPEAT_STATUS,                   AI_HDMI_REPEAT_STATUS_REPEAT_ON},
	{AT_SWPLL_VPANEL,                         AI_SWPLL_VPANEL_HW_PLL},
	{AT_NTSC_PEDESTAL,                        AI_NTSC_PEDESTAL_ON},
	{AT_MHEG_OSD,                             AI_MHEG_OSD_FULL},
	{AT_UI_PICTUREMODES,                      AI_UI_PICTUREMODES_UI_PICTUREMODES_VIVID},
	{AT_AUTOMODE,                             AI_AUTOMODE_DETECTSD},
	{AT_UI_COLORTEMP,                         AI_UI_COLORTEMP_UI_COLOURTEMP_COOL},
	{AT_UI_SETPICTURES,                       AI_UI_SETPICTURES_UI_SETSATURATION},
	{AT_UI_SETPRO,                            AI_UI_SETPRO_UI_SETBSTRETCH},
	{AT_UI_SETCOLORTEMP,                      AI_UI_SETCOLORTEMP_UI_SETCOLORTEMP_RED},
	{AT_WINDOWCFG,                            AI_WINDOWCFG_MOTIONWINDOW1},
	{AT_UI_ADVPICTURES,                       AI_UI_ADVPICTURES_UI_OFF},
	{AT_CRT_LOCKONOFF,                        AI_CRT_LOCKONOFF_50P},
	{AT_OSDWINDOW,                            AI_OSDWINDOW_MENU},
	{AT_SEQUENCE,                             AI_SEQUENCE_0},
	{AT_MPPIP_CHANNEL,                        AI_MPPIP_CHANNEL_MPEG_MPEG},
	{AT_HDMITX_SIGNALPATH,                    AI_HDMITX_SIGNALPATH_VIDEOSOURCE},
	{AT_HDMITX_SIGNALTYPE,                    AI_HDMITX_SIGNALTYPE_1080P60},
	{AT_HDMITX_COLORBITDEPTH,                 AI_HDMITX_COLORBITDEPTH_8BIT},
	{AT_HDMITX_INPUTCOLORSPACE,               AI_HDMITX_INPUTCOLORSPACE_RGB},
	{AT_HDMITX_OUTPUTCOLORSPACE,              AI_HDMITX_OUTPUTCOLORSPACE_RGB},
	{AT_UI_TNR,                               AI_UI_TNR_UI_TNR_OFF},
	{AT_UI_SNR,                               AI_UI_SNR_UI_SNR_OFF},
	{AT_UI_BLACKEXTENSION,                    AI_UI_BLACKEXTENSION_UI_BLACKEXTENSION_OFF},
	{AT_UI_WHITEEXTENSION,                    AI_UI_WHITEEXTENSION_UI_WHITEEXTENSION_OFF},
	{AT_UI_PURECINEMA,                        AI_UI_PURECINEMA_UI_PURECINEMA_OFF},
	{AT_UI_BLUESTRECH,                        AI_UI_BLUESTRECH_UI_BLUESTRECH_OFF},
	{AT_UI_SKINTONE,                          AI_UI_SKINTONE_UI_SKINTONE_MEDIUM_OFF},
	{AT_UI_DCI,                               AI_UI_DCI_UI_DCI_OFF},
	{AT_UI_VIDEOMODE,                         AI_UI_VIDEOMODE_UI_VIDEOMODE_ON},
};
#else
extern const TFDAttrItem TFDAttrList[102];
#endif


#endif

