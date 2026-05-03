/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
  This file is used to export some structs of vencoder to user space, mainly for rt_media.

  @author eric_wang
  @date 20241011
*/
#ifndef _VENCODER_BASE_H_
#define _VENCODER_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FACTOR_MAX_NUM 8

typedef struct VencRect {
	int nLeft;
	int nTop;
	int nWidth;
	int nHeight;
} VencRect;

typedef struct VencCropCfg {
	int      bEnable;
	VencRect Rect;
} VencCropCfg;

typedef enum eVencEncppScalerRatio {
	VENC_ISP_SCALER_0       = 0, //not scaler
	VENC_ISP_SCALER_EIGHTH  = 1, //scaler 1/8 write back
	VENC_ISP_SCALER_HALF    = 2, //scaler 1/2 write back
	VENC_ISP_SCALER_QUARTER = 3, //scaler 1/4 write back
} eVencEncppScalerRatio;

typedef struct sWbYuvParam {
	unsigned int bEnableWbYuv;
	unsigned int nWbBufferNum;
	unsigned int bEnableCrop;
	VencRect     sWbYuvcrop;
	eVencEncppScalerRatio scalerRatio;
} sWbYuvParam;

typedef struct {
	unsigned char ulargeFrameFlag;
	unsigned int ulargeFrameIdx;
	unsigned int uFrameRegionLevelNum[4];
} VencRegion3dResult;

typedef struct {
	//for region strategy
	unsigned int  uRegionWidth;                 // range[16~width], RegionWidth = regionHeightDiv16 << 4
	unsigned int  uRegionHeight;                // range[16~height], RegionHeight = regionHeightDiv16 << 4
	unsigned int  uRegionHorNum;                // range[1~16], RegionHorNum = width / RegionWidth
	unsigned int  uRegionVerNum;                // range[1~height/RegionHeight], RegionVerNum = height / RegionHeight
	unsigned int  uRegionTotalNum;              // range[1~], Total number of region area
	unsigned char e3dRegionEnable;              // range[0~1], region 3d enable
	unsigned char e3dRegionAutoMode;            // range[0~1], region auto mode
	unsigned char e3dRegionUpDownExpandEn;      // range[0~1], region 3d up and down area expand enable
	unsigned char e3dRegionLeftRightExpandEn;   // range[0~1], region 3d left and right area expand enable
	unsigned char histogramRegionMvLevelReadEn; // range[0~1], enable to read mv level of the region histogram
	unsigned char histogramRegionMvLevelWriteEn;// range[0~1], enable to write mv level of the region histogram
	unsigned char regionHeightDiv16;            // range[1~255], the height of the region histogram in mb size
	unsigned char regionWidthDiv16;             // range[1~255], the width of the region histogram in mb size
	unsigned char smallMvLevelMaxnum[2];        // range[0~3], the maximun number of small mv, [0]:hor,[1]:ver
	unsigned int  regionMvLevelRatioTh[3];      // range[0~256], move strength level threshold for region mv histogram statistics
												// 8bit decimal, equal to 256 means the integer 1
} VencRegion3dParam;

typedef struct Venc3dFilterParam{
	unsigned char e3dEn;                    // range[0~1], enc 3d total enable
	unsigned char e3dDefaultDis;            // range[0~1], 0:default parameters, 1:setting parameters
	unsigned char e3dLumaEn;                // range[0~1], 3d luma enable
	unsigned char e3dChromaEn;              // range[0~1], 3d chroma enable
	unsigned char e3dMoveStatusEn;          // range[0~1], use move status enable
	unsigned char e3dPixDiffEn;             // range[0~1], adjustment of coef pix level enable
	unsigned char e3dCoefAutoMode;          // range[0~1], ref coef auto mode
	unsigned char e3dMaxCoef;               // range[0~16], maximum weight of ref block 3d filter
	unsigned char e3dMinCoef;               // range[0~16], minimum weight of ref block 3d filter
	unsigned char noiseModeEn;              // range[0~1], denoise mode enable
	int           lambdaOffset;             // range[-8~7], lambda offset use for Ime search,
											// denoise mode: lambda = lambda + lambda_offset * noise_estimate_val
	unsigned char noiseEstVal;              // range[0~127], noise estimate value
	unsigned char e3dStatusTh[3];           // range[0~3], move status threshold when e3dMoveStatusEn is 1
	unsigned char e3dMoveStrengthMvTh[3];   // range[0~255], move strength level mv threshold
	unsigned char e3dAutoModeAvgTh[10];     // range[0~255], residual avg threshold when e3dCoefAutoMode is 1
	unsigned char e3dAutoModeCoef[11];      // range[0~16], ref block coefficient when e3dCoefAutoMode is 1

	VencRegion3dParam  sParam;
} Venc3dFilterParam;

typedef struct s2DfilterParam{
	unsigned char enable_2d_filter;
	unsigned char filter_strength_uv; //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_strength_y;  //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_th_uv;       //* range[0~15], advice: 2
	unsigned char filter_th_y;        //* range[0~15], advice: 2
} s2DfilterParam;

typedef struct {
	int en_d2d_limit;
	int d2d_level[6];
} VencVe2IspD2DLimit;

typedef struct {
	int dis_default_d2d;
	int d2d_min_lv_th;
	int d2d_max_lv_th;
	int d2d_min_lv_level;
	int d2d_max_lv_level;
	int dis_default_d3d;
	int d3d_min_lv_th;
	int d3d_max_lv_th;
	int d3d_min_lv_level;
	int d3d_max_lv_level;
} VencRotVe2Isp;

typedef struct {
	int dis_default_para;
	int mode;
	int en_gop_clip;
	float gop_bit_ratio_th[3];
	float coef_th[5][2];
} VencTargetBitsClipParam;

typedef enum  eGdcPerspFunc {
	Gdc_Persp_Only,
	Gdc_Persp_LDC
} eGdcPerspFunc;

typedef enum eGdcLensDistModel {
	Gdc_DistModel_WideAngle,
	Gdc_DistModel_FishEye
} eGdcLensDistModel;

typedef enum eGdcMountType {
	Gdc_Mount_Top,
	Gdc_Mount_Wall,
	Gdc_Mount_Bottom
} eGdcMountType;

typedef enum eGdcWarpType {
	Gdc_Warp_LDC,
	Gdc_Warp_LDC_Pro, //* new warp type
	Gdc_Warp_Pano180,
	Gdc_Warp_Pano360,
	Gdc_Warp_Normal,
	Gdc_Warp_Fish2Wide,
	Gdc_Warp_Perspective,
	Gdc_Warp_BirdsEye,
	Gdc_Warp_User,
	Gdc_Warp_Zoom,
} eGdcWarpType;

typedef enum {
	CAMERA_ADAPTIVE_STATIC = 0,
	CAMERA_FORCE_STATIC = 1,
	CAMERA_FORCE_MOVING = 2,
	CAMERA_ADAPTIVE_MOVING_AND_STATIC = 3,
	CAMERA_STATUS_NUM
} eCameraStatus;

typedef struct {
	unsigned char  bGDC_en;
	eGdcWarpType   eWarpMode;
	eGdcMountType  eMountMode;
	unsigned int *lut_data_buf;  //* just valid when eWarpMode is Gdc_Warp_LDC_Pro
	unsigned int   lut_data_size; //* just valid when eWarpMode is Gdc_Warp_LDC_Pro
	unsigned char  bMirror;
	unsigned int   calib_widht;
	unsigned int   calib_height;
	float fx;
	float fy;
	float cx;
	float cy;
	float fx_scale;
	float fy_scale;
	float cx_scale;
	float cy_scale;
	eGdcLensDistModel  eLensDistModel;
	float distCoef_wide_ra[3];
	float distCoef_wide_ta[2];
	float distCoef_fish_k[4];
	int centerOffsetX;
	int centerOffsetY;
	int rotateAngle;
	int radialDistortCoef;
	int trapezoidDistortCoef;
	int fanDistortCoef;
	int pan;
	int tilt;
	int zoomH;
	int zoomV;
	int scale;
	int innerRadius;
	float roll;
	float pitch;
	float yaw;
	eGdcPerspFunc  perspFunc;
	float perspectiveProjMat[9];
	int birdsImg_width;
	int birdsImg_height;
	float mountHeight;
	float roiDist_ahead;
	float roiDist_left;
	float roiDist_right;
	float roiDist_bottom;

	int peaking_en;
	int peaking_clamp;
	int peak_m;
	int th_strong_edge;
	int peak_weights_strength;
} sGdcParam;

#define REGIONLINK_REG_TBL_LENGTH 21

typedef struct sRegionLinkParamDynamic{
	unsigned short mad_cmp_th[3];                    // range[0~4095]
	unsigned short tex_mtb_max_val;                  // range[0~4095]
	unsigned short motion_mtb_max_val;               // range[0~4095]
	unsigned short motion_recover_berfore_i_max_val; // range[0~4095]
	unsigned short motion_recover_after_i_max_val;   // range[0~4095]

	unsigned short tex_mtb_curve[REGIONLINK_REG_TBL_LENGTH];    // range[0~4095]
	unsigned short motion_mtb_curve[REGIONLINK_REG_TBL_LENGTH]; // range[0~4095]
} sRegionLinkParamDynamic;

typedef struct sRegionLinkParamStatic{
	unsigned char link_print_en;
	unsigned char region_link_en;            // equal to mIspAndVeLinkageEnable
	unsigned char tex_detect_en;
	unsigned char motion_detect_en;
	unsigned char regionLinkAdaptParamEn;
	unsigned char motion_hor_dir_expand_en;
	unsigned char motion_ver_dir_expand_en;
	unsigned char motion_dir_expand_adp_en;
	unsigned char motion_soft_th_en;
	unsigned char motion_recover_en;
	unsigned char motion_recover_grad_en;
	unsigned char temporal_w_i_en;
	unsigned char temporal_w_p_en;
	unsigned char tex_para_select_mode;        // 0:default, 1:define
	unsigned char motion_para_select_mode;     // 0:default, 1:define
	unsigned char tex_ratio_th[2];             // range[0~100]
	unsigned char motion_hor_l_expand_num;     // range[0~7]
	unsigned char motion_hor_r_expand_num;     // range[0~7]
	unsigned char motion_ver_u_expand_num;     // range[0~7]
	unsigned char motion_ver_d_expand_num;     // range[0~7]
	unsigned char motion_soft_ratio_th[2];     // range[0~100], [0]: motion soft low th, [1]: motion soft up th
	unsigned char motion_recover_speed;        // range[0~50]
	unsigned char motion_recover_before_i_num; // range[0~idrPeroid-1], idrPeroid-1 < 255
	unsigned char motion_recover_after_i_num;  // range[0~idrPeroid-1], idrPeroid-1 < 255
	unsigned char motion_recover_grad_ratio;   // range[0~100]
	unsigned short texture_weight[3];          // range[0~256], [0]: flat, [1]: normal, [2]: complex
	unsigned short cur_frame_ratio;            // range[0~256]
	unsigned short motion_largemv_th;
} sRegionLinkParamStatic;

typedef struct sRegionLinkParam{
	sRegionLinkParamDynamic mDynamicParam;
	sRegionLinkParamStatic  mStaticParam;
} sRegionLinkParam;

typedef enum VENC_CODING_MODE {
	VENC_FRAME_CODING         = 0,
	VENC_FIELD_CODING         = 1,
} VENC_CODING_MODE;

typedef enum VENC_CODEC_TYPE {
	VENC_CODEC_H264,
	VENC_CODEC_JPEG,
	VENC_CODEC_H264_VER2,
	VENC_CODEC_H265,
	VENC_CODEC_VP8,
	VENC_CODEC_CNT,
} VENC_CODEC_TYPE;

typedef enum VENC_PIXEL_FMT {
	VENC_PIXEL_YUV420SP = 0,
	VENC_PIXEL_YVU420SP = 1,
	VENC_PIXEL_YUV420P = 2,
	VENC_PIXEL_YVU420P = 3,
	VENC_PIXEL_YUV422SP = 4,
	VENC_PIXEL_YVU422SP = 5,
	VENC_PIXEL_YUV422P = 6,
	VENC_PIXEL_YVU422P = 7,
	VENC_PIXEL_YUYV422 = 8,
	VENC_PIXEL_UYVY422 = 9,
	VENC_PIXEL_YVYU422 = 10,
	VENC_PIXEL_VYUY422 = 11,
	VENC_PIXEL_ARGB = 12,
	VENC_PIXEL_RGBA = 13,
	VENC_PIXEL_ABGR = 14,
	VENC_PIXEL_BGRA = 15,
	VENC_PIXEL_TILE_32X32 = 16,
	VENC_PIXEL_TILE_128X32 = 17,
	VENC_PIXEL_AFBC_AW = 18,
	VENC_PIXEL_LBC_AW = 19, //* for v5v200 and newer ic //19
} VENC_PIXEL_FMT;

typedef enum VENC_OUTPUT_FMT {
	VENC_OUTPUT_SAME_AS_INPUT = 0,
	VENC_OUTPUT_YUV420,
	VENC_OUTPUT_YUV444,
	VENC_OUTPUT_YUV422,
} VENC_OUTPUT_FMT;

/**
 * H264 profile types
 */
typedef enum VENC_H264PROFILETYPE {
	VENC_H264ProfileBaseline  = 66,         /**< Baseline profile */
	VENC_H264ProfileMain      = 77,         /**< Main profile */
	VENC_H264ProfileHigh      = 100,           /**< High profile */
} VENC_H264PROFILETYPE;

/**
 * H264 level types
 */
typedef enum VENC_H264LEVELTYPE {
	VENC_H264Level1   = 10,     /**< Level 1 */
	VENC_H264Level11  = 11,     /**< Level 1.1 */
	VENC_H264Level12  = 12,     /**< Level 1.2 */
	VENC_H264Level13  = 13,     /**< Level 1.3 */
	VENC_H264Level2   = 20,     /**< Level 2 */
	VENC_H264Level21  = 21,     /**< Level 2.1 */
	VENC_H264Level22  = 22,     /**< Level 2.2 */
	VENC_H264Level3   = 30,     /**< Level 3 */
	VENC_H264Level31  = 31,     /**< Level 3.1 */
	VENC_H264Level32  = 32,     /**< Level 3.2 */
	VENC_H264Level4   = 40,     /**< Level 4 */
	VENC_H264Level41  = 41,     /**< Level 4.1 */
	VENC_H264Level42  = 42,     /**< Level 4.2 */
	VENC_H264Level5   = 50,     /**< Level 5 */
	VENC_H264Level51  = 51,     /**< Level 5.1 */
	VENC_H264Level52  = 52,     /**< Level 5.2 */
	VENC_H264LevelDefault = 0
} VENC_H264LEVELTYPE;

typedef struct VencH264ProfileLevel {
	VENC_H264PROFILETYPE    nProfile;
	VENC_H264LEVELTYPE        nLevel;
} VencH264ProfileLevel;

typedef struct VencQPRange {
	int    nMaxqp;
	int    nMinqp;
	int    nMaxPqp;
	int    nMinPqp;
	int    nQpInit;
	int    bEnMbQpLimit;
} VencQPRange;

/* support 8 ROI region */
typedef struct VencROIConfig {
	int                     bEnable;
	int                     index; /* (0~7) */
	int                     nQPoffset;
	unsigned char           roi_abs_flag;
	VencRect                sRect;
} VencROIConfig;

typedef struct VencFixQP {
	int                     bEnable;
	int                     nIQp;
	int                     nPQp;
} VencFixQP;

typedef enum {
	AW_CBR = 0,
	AW_VBR = 1,
	AW_AVBR = 2,
	AW_QPMAP = 3,
	AW_FIXQP = 4,
	AW_QVBR = 5,
} VENC_RC_MODE;

typedef enum  VENC_VIDEO_GOP_MODE {
	AW_NORMALP                    = 1,   //one p ref frame
	AW_ADVANCE_SINGLE             = 2,
	AW_DOUBLEP                    = 3,
	AW_SPECIAL_DOUBLE            = 4,
	AW_SPECIAL_SMARTP            = 5,   //double p ref frames and use virtual i frame,but virtual i ref virtual i
	AW_SMARTP                    = 6,   //double p ref frames and use virtual i frame
} VENC_VIDEO_GOP_MODE;

typedef enum {
	 VENC_H264_NALU_PSLICE = 1,                         /*PSLICE types*/
	 VENC_H264_NALU_ISLICE = 5,                         /*ISLICE types*/
	 VENC_H264_NALU_SEI    = 6,                         /*SEI types*/
	 VENC_H264_NALU_SPS    = 7,                         /*SPS types*/
	 VENC_H264_NALU_PPS    = 8,                         /*PPS types*/
	 VENC_H264_NALU_IPSLICE = 9,
	 VENC_H264_NALU_SVC    = 14,                        /*SVC Prefix types*/
} VENC_H264_NALU_TYPE;

typedef enum {
	 VENC_H265_NALU_PSLICE = 1,                         /*P SLICE types*/
	 VENC_H265_NALU_ISLICE = 19,                        /*I SLICE types*/
	 VENC_H265_NALU_VPS    = 32,                        /*VPS types*/
	 VENC_H265_NALU_SPS    = 33,                        /*SPS types*/
	 VENC_H265_NALU_PPS    = 34,                        /*PPS types*/
	 VENC_H265_NALU_SEI    = 39,                        /*SEI types*/
} VENC_H265_NALU_TYPE;

typedef enum {
	 VENC_JPEG_PACK_ECS = 5,                            /*ECS types*/
	 VENC_JPEG_PACK_APP = 6,                            /*APP types*/
	 VENC_JPEG_PACK_VDO = 7,                            /*VDO types*/
	 VENC_JPEG_PACK_PIC = 8,                            /*PIC types*/
} VENC_JPEGE_PACK_TYPE;

typedef union {
	VENC_H264_NALU_TYPE    eH264Type;               /*H264E NALU types*/
	VENC_JPEGE_PACK_TYPE   eJpegType;               /*JPEGE pack types*/
	VENC_H265_NALU_TYPE    eH265Type;               /*H264E NALU types*/
} VENC_OUTPUT_PACK_TYPE;

typedef struct {
	VENC_OUTPUT_PACK_TYPE  uType;
	unsigned int           nOffset;
	unsigned int           nLength;
} VencPackInfo;

#define MAX_OUTPUT_PACK_NUM (8)

typedef enum VENC_COLOR_SPACE {
	RESERVED0           = 0,
	VENC_BT709          = 1,  // include BT709-5, BT1361, IEC61966-2-1 IEC61966-2-4
	RESERVED1           = 2,
	RESERVED2           = 3,
	VENC_BT470          = 4,  // include BT470-6_SystemM,
	VENC_BT601          = 5,  // include BT470-6_SystemB, BT601-6_625, BT1358_625, BT1700_625
	VENC_SMPTE_170M     = 6,  // include SMPTE_170M, BT601-6_525, BT1358_525, BT1700_NTSC
	VENC_SMPTE_240M     = 7,  // include SMPTE_240M
	VENC_YCC            = 8,  // include GENERIC_FILM
	VENC_BT2020         = 9,  // include BT2020
	VENC_ST_428         = 10, // include SMPTE_428-1
} VENC_COLOR_SPACE;

typedef enum VENC_VIDEO_FORMAT {
	COMPONENT           = 0,
	PAL                 = 1,
	NTSC                = 2,
	SECAM               = 3,
	MAC                 = 4,
	DEFAULT             = 5,
} VENC_VIDEO_FORMAT;

typedef struct VencJpegVideoSignal {
	VENC_COLOR_SPACE src_colour_primaries;
	VENC_COLOR_SPACE dst_colour_primaries;
} VencJpegVideoSignal;

typedef struct VencH264VideoSignal {
	VENC_VIDEO_FORMAT video_format;

	unsigned char full_range_flag;

	VENC_COLOR_SPACE src_colour_primaries;
	VENC_COLOR_SPACE dst_colour_primaries;
} VencH264VideoSignal;

typedef struct VencH264VideoTiming {
	  unsigned long num_units_in_tick;
	  unsigned long time_scale;
	  unsigned int fixed_frame_rate_flag;

} VencH264VideoTiming;

typedef struct VencCyclicIntraRefresh {
	int                     bEnable;
	int                     nBlockNumber;
} VencCyclicIntraRefresh;

typedef struct VencAdvancedRefParam {
	unsigned char              bAdvancedRefEn;   //advanced ref frame mode, 0:not use , 1:use
	unsigned int               nBase;       //base frame num
	unsigned int               nEnhance;    //enhance frame num
	unsigned char              bRefBaseEn;  //ctrl base frame ref base frame, 0:enable, 1:disable
} VencAdvancedRefParam;

typedef struct VencGopParam {
	unsigned char              bUseGopCtrlEn;   //use user set gop mode
	VENC_VIDEO_GOP_MODE        eGopMode;        //gop mode
	unsigned int               nVirtualIFrameInterval;
	unsigned int               nSpInterval;     //user set special p frame ref interval
	VencAdvancedRefParam       sRefParam;       //user set advanced ref frame mode
} VencGopParam;

typedef enum VENC_SUPERFRAME_MODE {
	VENC_SUPERFRAME_NONE,
	VENC_SUPERFRAME_DISCARD,
	VENC_SUPERFRAME_REENCODE,
} VENC_SUPERFRAME_MODE;

typedef enum VENC_SUPERFRAME_PRIORITY {
	VENC_SUPERF_RPQ_BALANCE = 0,
	VENC_SUPERF_BIT_RATE_FIRST,
	VENC_SUPERF_FRAME_BITS_FIRST,
	VENC_SUPERFE_BUTT,
} VENC_SUPERFRAME_PRIORITY;

typedef struct VencSuperFrameConfig {
	VENC_SUPERFRAME_MODE    eSuperFrameMode;
	unsigned int            nMaxIFrameBits;
	unsigned int            nMaxPFrameBits;
	unsigned int            nMaxBFrameBits;
	VENC_SUPERFRAME_PRIORITY eSuperPriority;
	int                     nMaxRencodeTimes;
	float                   nMaxP2IFrameBitsRatio;
} VencSuperFrameConfig;

typedef struct VencBitRateRange {
	int            bitRateMax;
	int            bitRateMin;
	float          fRangeRatioTh;
	int            nQualityTh;
	int            nQualityStep[2];
	int            nSubQualityDelay;
	int            nAddQualityDelay;
	int            nMinQuality;
	int            nMaxQuality;
} VencBitRateRange;

typedef struct VencH265TimingS {
	//0:stream without timing info; 1:stream with timing info
	unsigned char       timing_info_present_flag;
	unsigned int        num_units_in_tick;       //time_scale/frameRate
	unsigned int        time_scale;             //1second is  average divided by time_scale
	unsigned int        num_ticks_poc_diff_one; //num ticks of diff frame
} VencH265TimingS;

typedef enum VENC_OVERLAY_ARGB_TYPE {
	VENC_OVERLAY_ARGB_MIN    = -1,
	VENC_OVERLAY_ARGB8888    = 0,
	VENC_OVERLAY_ARGB4444   = 1,
	VENC_OVERLAY_ARGB1555   = 2,
	VENC_OVERLAY_ARGB_MAX    = 3,
} VENC_OVERLAY_ARGB_TYPE;

typedef enum VENC_OVERLAY_TYPE {
	NORMAL_OVERLAY          = 0,    //normal overlay
	COVER_OVERLAY           = 1,    //use the setting yuv to cover region
	LUMA_REVERSE_OVERLAY    = 2,    //normal overlay and luma reverse
} VENC_OVERLAY_TYPE;

typedef struct VencOverlayCoverYuvS {
	 unsigned char       use_cover_yuv_flag; //1:use the cover yuv; 0:transform the argb data to yuv ,then cover
	 unsigned char       cover_y; //the value of cover y
	 unsigned char       cover_u; //the value of cover u
	 unsigned char       cover_v; //the value of cover v
} VencOverlayCoverYuvS;

typedef struct {
	unsigned char                 reserve_zero : 2;
	unsigned char                 constraint_5 : 1;
	unsigned char                 constraint_4 : 1;
	unsigned char                 constraint_3 : 1;
	unsigned char                 constraint_2 : 1;
	unsigned char                 constraint_1 : 1;
	unsigned char                 constraint_0 : 1;
} VencH264ConstraintFlag;

typedef struct {
	unsigned int                  en_force_conf;
	unsigned int                  left_offset;
	unsigned int                  right_offset;
	unsigned int                  top_offset;
	unsigned int                  bottom_offset;
} VencForceConfWin;

typedef struct {
	unsigned char *pBuffer;
	unsigned int  nBufLen;
	unsigned int  nDataLen;
	unsigned int  nType;
} VencSeiData;

typedef struct {
	unsigned int nSeiNum;
	VencSeiData  *pSeiData;
} VencSeiParam;

typedef enum {
	BUF_IDLE,
	BUF_STANDBY,
	BUF_OCCUPY,
} VENC_BUF_STATUS;

typedef struct {
	unsigned char *pBuffer;
	unsigned int  nBufLen;
	unsigned int  nDataLen;
	unsigned int  nFrameRate;
	VENC_BUF_STATUS eStatus;
} VencInsertData;

typedef enum {
	PRODUCT_STATIC_IPC = 0,
	PRODUCT_MOVING_IPC = 1,
	PRODUCT_DOORBELL = 2,
	PRODUCT_CDR = 3,
	PRODUCT_SDV = 4,
	PRODUCT_PROJECTION = 5,
	PRODUCT_UAV = 6,       // Unmanned Aerial Vehicle
	PRODUCT_NUM,
} eVencProductMode;

typedef enum {
	VENC_H265ProfileMain        = 1,
	VENC_H265ProfileMain10      = 2,
	VENC_H265ProfileMainStill   = 3
} VENC_H265PROFILETYPE;

typedef enum {
	VENC_H265Level1   = 30,     /**< Level 1 */
	VENC_H265Level2  = 60,     /**< Level 2 */
	VENC_H265Level21  = 63,     /**< Level 2.1 */
	VENC_H265Level3  = 90,     /**< Level 3 */
	VENC_H265Level31   = 93,     /**< Level 3.1 */
	VENC_H265Level4  = 120,      /**< Level 4 */
	VENC_H265Level41  = 123,     /**< Level 4.1 */
	VENC_H265Level5  = 150,     /**< Level 5 */
	VENC_H265Level51   = 153,     /**< Level 5.1 */
	VENC_H265Level52  = 156,     /**< Level 5.2 */
	VENC_H265Level6  = 180,     /**< Level 6 */
	VENC_H265Level61   = 183,     /**< Level 6.1 */
	VENC_H265Level62  = 186,     /**< Level 6.2 */
	VENC_H265LevelDefault = 0
} VENC_H265LEVELTYPE;

typedef enum {
	VENC_ST_DIS_WDR      = 0,
	VENC_ST_EN_WDR       = 1,
	VENC_ST_NONE
} eSensorType;

typedef struct {
	VENC_H265PROFILETYPE    nProfile;
	VENC_H265LEVELTYPE        nLevel;
} VencH265ProfileLevel;

typedef struct {
	unsigned int            uMaxBitRate;
	unsigned int            nMovingTh;      //range[1,31], 1:all frames are moving,
											//            31:have no moving frame, default: 20
	int                     nQuality;       //range[1,20], 1:worst quality, 20:best quality
	int                     nIFrmBitsCoef;   //range[1, 20], 1:worst quality, 20:best quality
	int                     nPFrmBitsCoef;   //range[1, 50], 1:worst quality, 50:best quality
} VencVbrParam;

typedef struct {
	unsigned qp                    : 6;  // [05:00]
	unsigned skip                  : 1;  // [06]
	unsigned qp_skip_en            : 1;  // [07]
	unsigned lambda                : 8;  // [15:08]
	unsigned skip_bias             : 6;  // [21:16]
	unsigned reserve_22            : 1;  // [22]
	unsigned skip_bias_en          : 1;  // [23]
	unsigned inter_bias            : 6;  // [29:24]
	unsigned reserve_30            : 1;  // [30]
	unsigned inter_bias_en         : 1;  // [31]
} VencMbModeLambdaQpMap;

typedef struct {
	unsigned inter8_cost_factor    : 6;  // [05:00]
	unsigned inter16_cost_factor   : 6;  // [11:06]
	unsigned inter_cost_factor_en  : 1;  // [12]
	unsigned intra4_cost_factor    : 6;  // [18:13]
	unsigned intra8_cost_factor    : 6;  // [24:19]
	unsigned reserve_30_25         : 6;  // [30:25]
	unsigned intra_cost_factor_en  : 1;  // [31]
} VencMbSplitMap;

typedef struct {
	VencMbModeLambdaQpMap  low32;
	VencMbSplitMap         high32;
} VencMbMap;

typedef struct {
	unsigned char mode_ctrl_en;
	unsigned char qp_map_en;
	unsigned char lambda_map_en;
	unsigned char mode_map_en;
	unsigned char split_map_en;
	unsigned char *p_map_info;
} VencMBModeCtrl;

typedef struct {
	VENC_RC_MODE            eRcMode;
	unsigned char           bLowBitrateBeginFlag;
	unsigned char           bUseSetMadThrdFlag;
	unsigned char           uMadThrdI[12]; //range 0-255
	unsigned char           uMadThrdP[12]; //range 0-255
	unsigned char           uMadThrdB[12]; //no support
	unsigned int            uStatTime;      //range [1,10], default:1
	unsigned int            uMinIQp;
	int                     nMaxReEncodeTimes; //default use one time

	VencVbrParam            sVbrParam;      //valid only at AW_VBR/AW_AVBR
	VencFixQP               sFixQp;         //valid only at AW_FIXQP
	VencMBModeCtrl          sQpMap;         //valid only at AW_QPMAP

	unsigned int            uRowQpDelta; //no support
	unsigned int            uDirectionThrd; //no support
	unsigned int            uQpDeltaLevelI; //no support
	unsigned int            uQpDeltaLevelP; //no support
	unsigned int            uQpDeltaLevelB; //no support
	unsigned int            uInputFrmRate;  //no support
	unsigned int            uOutputFrmRate; //no support
	unsigned int            uFluctuateLevel;//no support
	unsigned int            uMinIprop;      //no support
	unsigned int            uMaxIprop;      //no support
} VencRcParam;

typedef struct VencH264Param {
	VencH264ProfileLevel    sProfileLevel;
	int                     bEntropyCodingCABAC; /* 0:CAVLC 1:CABAC*/
	VencQPRange               sQPRange;
	int                     nFramerate; /* fps*/
	int                     nSrcFramerate; /* fps*/
	int                     nBitrate;   /* bps*/
	int                     nMaxKeyInterval;
	VENC_CODING_MODE        nCodingMode;
	VencGopParam            sGopParam;
	VencRcParam             sRcParam;
	int breduce_refrecmem;
} VencH264Param;

typedef struct {
	int                     idr_period;
	VencH265ProfileLevel    sProfileLevel;
	VencQPRange               sQPRange;
	int                     nFramerate; /* fps*/
	int                     nSrcFramerate; /* fps*/
	int                     nBitrate;   /* bps*/
	int                     nIntraPeriod;
	int                     nGopSize;
	int                     nQPInit; /* qp of first IDR_frame if use rate control */
	VencRcParam             sRcParam;
	VencGopParam            sGopParam;
	int breduce_refrecmem;
} VencH265Param;

typedef struct {
	// all these average value is for mb16x16
	unsigned int avg_mad;
	unsigned int avg_md;
	unsigned int avg_sse;
	unsigned int avg_qp;
	double avg_psnr;
	unsigned char *p_mb_mad_qp_sse;
	unsigned char *p_mb_mv;
} VencMBSumInfo;

typedef struct {
	int pix_x_bgn;
	int pix_x_end;
	int pix_y_bgn;
	int pix_y_end;
	int thumb_x_bgn;
	int thumb_x_end;
	int thumb_y_bgn;
	int thumb_y_end;
	int thumb_pix_num;
	int total_num;
	int intra_num;
	int large_mv_num;
	int small_mv_num;
	int zero_mv_num;
	int large_mad_num;
	int is_motion;
} VencMotionSearchRegion;

typedef struct {
	int total_region_num;
	int motion_region_num;
	VencMotionSearchRegion *region;
} VencMotionSearchResult;

typedef struct {
	int en_motion_search;
	int dis_default_para;
	int hor_region_num;
	int ver_region_num;
	int en_check_mv;            // range[0, 1]
	int en_check_mad;           // range[0, 1]
	int en_morpholog;           // range[0, 1]
	int large_mv_th;            // range[0, 511]
	int large_mad_th;           // range[0, 255]
	int background_weight;      // range[0, 15]
	float large_mv_ratio_th;    // range[0, 100.0f] include intra and large mv
	float non_zero_mv_ratio_th; // range[0, 100.0f] include intra, large mv and samll mv
	float large_mad_ratio_th;   // range[0, 100.0f]
} VencMotionSearchParam;

typedef void *VideoEncoder;

typedef struct {
	int is_static;
	VideoEncoder *pVideoEnc;
	VENC_CODEC_TYPE codecType;
	int parsingStatic;
	int parsingDynamic;
	int channnel_id;
	unsigned int        nInputWidth;
	unsigned int        nInputHeight;
	unsigned int        nDstWidth;
	unsigned int        nDstHeight;
	VENC_PIXEL_FMT      eInputFormat;
	VENC_OUTPUT_FMT     eOutputFormat;
	unsigned int        fps;
	VencQPRange qp_range;
	int bit_rate;
	VencH264Param       h264Param;
	unsigned int vbv_size;
	VencForceConfWin conf_win;
	int jpeg_quality;
	VencFixQP           fixQP;
	VencTargetBitsClipParam bits_clip_param;
	VencVbrParam            sVbrParam;
	s2DfilterParam m2DfilterParam;
	Venc3dFilterParam m3DfilterParam;
	int mb_rc_level;
	float        weak_text_th;
	VencH264VideoTiming mH264VideoTiming;
} VencParamFromFiles;

typedef struct InsideVbrKeyPar{
	unsigned int         uVbrOptEn;
	unsigned int         uPrintVbrTraceInfoEn;
	unsigned int         uPrintLogInfoEn;
	int                  PrintRegInfoFrameNum;
	int                  nIFrameRcInfoNum;
	int                  nPFrameRcInfoNum;
	int                  nSaveBitQpTh;
	int                  nBoostBitQpTh;
	int                  nBppLevelNum;
	int                  nIPSliceQpMaxGap;
	int                  nMvStaLevelNum;
	int                  nStaMvTh[FACTOR_MAX_NUM];
	int                  nAvgMvFactor;
	int                  nAvgQpRatio;
	int                  nDeltaQp;
	int                  nMaxHistoryFrameNum[2];
	float                fInitAvgMovingLevel;
	float                fAvgMoveLevelRatio;
	float                fMinRatio[2];//0:use for I slice,1:use for P slice
	float                fMaxRatio[2];//0:use for I slice,1:use for P slice
	float                fMinRaioTh[2];
	float                fMaxRatioTh[2];
	float                fBppLevel[2][FACTOR_MAX_NUM];
	float                fAdjustQpBppTh[2][FACTOR_MAX_NUM];
	float                fAdjustMaxQp[2][FACTOR_MAX_NUM];
	float                fMoveFrameLevelTh;
	float                fStaticFrameLevelTh;
	float                fluctuationFactorTh;
	unsigned char        uIframeMadThOpt[12];
	unsigned char        uPframeMadThOpt[12];
} InsideVbrKeyPar;

typedef struct InsideVbrMosAicOptPar{
	//mosaic opt param
	unsigned int         uConstantLargeMoveNumTh;
	unsigned int         uConstantStaticNumTh;
	unsigned int         uMethodSelect;//0:constand move frame adjust sliceqp;
									   //1:constand move frame and MoveToStatic frame adjust slice qp
									   //2:adjust target bits
									   //3:no opeartion
	unsigned int         uAdjustSliceQpPeriod;
	unsigned int         uSliceQpLimitTh;
	int                  nMosAicSliceDeltaQp;
	float                nScaleDeltaRatio;
} InsideVbrMosAicOptPar;

typedef enum VENC_RC_PRIORITY {
	VENC_RC_RPQ_BALANCE_Q = 0,       // balance bit rate and quality, default value
	VENC_RC_RPQ_BALANCE_R,
	VENC_RC_AVERAGE_BIT_RATE_FIRST,  // Average bitrate is guaranteed first
	VENC_RC_RT_BIT_RATE_FIRST,       // Instantaneous bitrate is guaranteed first
	VENC_RC_BUTT,
} VENC_RC_PRIORITY;

typedef struct InsideVbrMoveToStaticPar{
	unsigned int         uConstandMoveToStaticNumTh;
	int                  nMoveToStaticSliceQpDelta[FACTOR_MAX_NUM];
	int                  nMoveToStaticDelayNum[4];
	float                fFrameMvLevelTh[FACTOR_MAX_NUM];
} InsideVbrMoveToStaticPar;

typedef struct InsideClipQpFunc{
	unsigned char closeMbRcEn;
	unsigned char MoveStatusTh;
	unsigned int  MbQpTightLimitEn;
	int           uAddDeltaQp;
	unsigned int  uSliceQpMaxTh;
} InsideClipQpFunc;

typedef struct InsideInstaneousBitRatePar{
	int           nFactorLevelNum;
	float         ExceedTarBrRatio;//exceed max target bitrate*ExceedTarBrRatio need to save bits
	float         SmallTarBrRatio; //increase bits if the intantenus bitrate is less than target bitrate*smallTarBrRatio
	float         recodeExceedTarBrRatio;
	float         fgAdjustFactorTh[FACTOR_MAX_NUM];
	float         fgAdjustFactor[FACTOR_MAX_NUM];
} InsideInstaneousBitRatePar;

typedef struct InsideVbrOptPar{
	InsideVbrKeyPar            vbrKeyPar;
	InsideVbrMosAicOptPar      vbrMosAicOptPar;
	InsideVbrMoveToStaticPar   vbrMoveToStaticPar;
	InsideInstaneousBitRatePar vbrInstaneousBRPar;
	InsideClipQpFunc           mclipQpOpt;
} InsideVbrOptPar;

typedef struct VencVbrOptParam {
	int                  nIPratio;
	int                  nIntraPeriodNumInVbv;
	unsigned char        enable_instaneousBR;
	float                max_instaneousBR;
	unsigned int         peroid_instaneousBR;
	unsigned char        recodeIsliceQpEn;
	unsigned int         uMosAicOptEn;
	unsigned int         uSaveBitRateEn;
	unsigned int         uClassifyMadThAdjustEn;
	unsigned int         uMoveToStaticOptEn;
	InsideVbrOptPar      pVbrOptPar;
	VENC_RC_PRIORITY     sRcPriority;
} VencVbrOptParam;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _VENCODER_BASE_H_ */

