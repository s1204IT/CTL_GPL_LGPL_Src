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
#include "../../aw_hdmirx_define.h"
#include "../aw_hdmirx_core_drv.h"
#include "THDMIRx_TV_Driver.h"
#include "../THDMIRx_System.h"
#include "ThdmiRxIPReg.h"
#include "../THDMIRx_Event.h"
#include "../THDMIRx_PortCtx.h"

void HdmiRx_Audio_set_regbase(uintptr_t reg_base)
{
	vs_base = reg_base;
	link_base = reg_base + 0x40000;
	phy_base = reg_base + 0x800;
	audio_pll_base = reg_base + 0x80000;
	AUDIO_INF("vs_base = %p, link_base = %p, phy_base = %p, audio_pll_base = %p\n", (void *)vs_base, (void *)link_base, (void *)phy_base, (void *)audio_pll_base);
}

U32 HdmiRx_Audio_GetAudioPLLBase(void)
{
	return HDMIRX_APLL_REG_BASE;
}

void HdmiRx_Audio_PowerDownAPLL(void)
{
	U32 tmp = 0;
	/* Power down PLL */
	tmp = HDMI_CTS_CLK | PLL_HDMI_AUDIO_SRC_EN | PLL_HDMI_AUDIO_BLOCK_EN | PLL_HDMI_AUDIO_PD;

	HdmiRx_SetRegu8_Byu32(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0, tmp);

	AUDIO_INF("HdmiRxAudioPllPowerDown success!\n");
}

void HdmiRx_Audio_PowerUpAPLL(void)
{
	// set audio PLL active
	HDMIRx_WriteRegMaskU8(audio_pll_base + PLL_HDMI_AUDIO_CON2_CTL_0, PLL_HDMI_AUDIO_PD, 0);

	// set clk_hdmi_audio enable,
	// select clk_hdmi_audio from PLL
	HDMIRx_WriteRegMaskU8(audio_pll_base + CLK_HDMI_AUDIO_CTL_0, EN_CLK_HDMI_AUDIO | SEL_CLK_HDMI_AUDIO, EN_CLK_HDMI_AUDIO | SEL_PLL);

	AUDIO_INF("HdmiRxAudioPllPowerUp success!\n");
}

bool HdmiRx_Audio_CalculateDividerSettings(void *pAudioCtx, void *pSTRetParams)
{
#if 1
	/*
	 *
	 *     my %pDivLookup = (
	 *     32 => 0x8,
	 *     16 => 0x11,
	 *     8  => 0xb,
	 *     4  => 0x2,
	 *     );
	 */
	ThdmiRxAudioContext *pParams = (ThdmiRxAudioContext *)pAudioCtx;
	ThdmiRxAudioPllDividerParams *pstRetParams = (ThdmiRxAudioPllDividerParams *)pSTRetParams;
	U32 ui32CalcTMDSClk = 0x00;
	U32 ui32TempLoopCount = 0;
	U32 ui32InputFrequency = 0x00;
	U32 ui32OutputFrequency = 0x00;
	bool bXdivCalcSuccess = false;
	bool bPdivCalcSuccess = false;
	U32 aui32PcalcTable[] = {32, 16, 8, 4};
	U32 aui32PDivFactorTable[] = {0x08, 0x11, 0x0B, 0x02};
	U32 ui32ScratchPad1 = 0x00;
	U32 ui32MFactInteger;
	U32 ui32MFactRemainder = 0x00;
	U32 fMDiv;
	U32 fMFract;
	//float fMDiv;
	//float fMFract;

	// # Calculate TMDS Frequency
	// my $tmds = ( 128 * $args{fs} * $args{CTS} ) / $args{N};

	ui32OutputFrequency = 128 * pParams->fs;

	ui32CalcTMDSClk = (U32)(ui32OutputFrequency * ((U32)pParams->CTS / pParams->N));

	AUDIO_INF(" AUDIO PLL CALC: Stage 0 TMDS=%x OutputFs=%x InterRslt=%x \n", ui32CalcTMDSClk, ui32OutputFrequency,
		((ui32OutputFrequency)*pParams->CTS));

	/*
	 *
	 * # Calculate xDiv
	 * foreach my $index ( 11 .. 14 ) {
	 *
	 * my $x = ( $tmds / $args{CTS} ) * ( 2 ** $index );
		*
	 * if( $x > 2800000 ) {
	 *     $xDiv = $index - 11;
	 *     last;
	 *     }
	 * }
	 *
	 */

	for (ui32TempLoopCount = 11; ui32TempLoopCount <= 14; ++ui32TempLoopCount) {
		ui32InputFrequency = (U32)((((U32)ui32CalcTMDSClk / pParams->CTS)) * (BASE2POW(ui32TempLoopCount)));
		if (ui32InputFrequency > 2800000) {
			pstRetParams->xDiv = ui32TempLoopCount - 11;
			AUDIO_INF(" AUDIO PLL CALC: Stage 1 XDiv =%x SPad=%d LCount=%x Cond Pass !!!\n", pstRetParams->xDiv, ui32InputFrequency,
				ui32TempLoopCount);
			bXdivCalcSuccess = true;
			break;
		} else {
			AUDIO_INF(" AUDIO PLL CALC: Stage 1 SPad=%x LCount=%x Cond Fail !!!\n", ui32InputFrequency, ui32TempLoopCount);
		}
	}

	/*
	 *     # Calculate Input & Output Frequencies
	 *    my $f_in  = ( $tmds / $args{CTS} ) * ( 2 ** ( $xDiv + 11 ) );
	 * my $f_out = $args{fs} * 128;
	 */

	if (bXdivCalcSuccess == false) {
		// DBG_ASSERT( bXdivCalcSuccess == true );
		hdmirx_wrn("AUDIO PLL CALC: bXdivCalc Failed !!!\n");
		return false;
	}

	AUDIO_INF("AUDIO PLL CALC: Stage 2 Fin=%d Fout=%d\n", ui32InputFrequency, ui32OutputFrequency);

	/*
	 *     # Calculate pDiv & mDiv
	 * foreach my $p ( 32, 16, 8, 4 ) {
	 *         my $m = ( $f_out * $p ) / $f_in;
	 *         if( ( $m >= 27 ) && ( $m <= 92 ) ) {
	 *             $mDiv = $m;
	 *             $pDiv = $pDivLookup{$p};
	 *             last;
	 *     }
	 * }
	 */

	for (ui32TempLoopCount = 0; ui32TempLoopCount < 4; ++ui32TempLoopCount) {
		ui32ScratchPad1 = (U32)(ui32OutputFrequency * ((U32)aui32PcalcTable[ui32TempLoopCount] / ui32InputFrequency));

		/*
			* CRPR ID: bl33#5585 - select HDMI Format 1080p60@ 12 bit in Astro Generator
			* select AC3 with sampling freq=32khz externally from Audio precision Connecting
			* to Astro Generator, No audio is heard
			* In Above sequence Received N and CTS values are [N:6272][CTS:113696] but expected values are
			* [N:4096][CTS:74250] as per the standard. In the Calculation of mDiv and pDiv factors,
			* Resultant value of mDiv based on above N and CTS, is falling beyod lower assumed range of
			* ( 27 < mDiv < 92 ). Range is Modified to ( 24 < mDiv < 92 ) and PLL is configured
			* with this modified range, able to hear the audio under reported sequence.
		*/
		// if ( ui32ScratchPad1 >= 27 &&  ui32ScratchPad1 <= 92 )
		if (ui32ScratchPad1 >= 24 && ui32ScratchPad1 <= 92) {
			pstRetParams->mDiv = ui32ScratchPad1;
			pstRetParams->pDiv = aui32PDivFactorTable[ui32TempLoopCount];

			bPdivCalcSuccess = true;

			AUDIO_INF(" AUDIO PLL CALC: Stage 3 mDiv=%d pDiv=%d LoopCnt=%d Calc Pass !!!\n", pstRetParams->mDiv, pstRetParams->pDiv,
				ui32TempLoopCount);

			break;

		} else {
			AUDIO_INF(" AUDIO PLL CALC: Stage 3 LoopCnt=%d Scpad=%u Calc Prog !!!\n", ui32TempLoopCount, ui32ScratchPad1);
		}
	}

	if (bPdivCalcSuccess == false) {
		// DBG_ASSERT( bPdivCalcSuccess == true );
		hdmirx_wrn("AUDIO PLL CALC: bPdivCalc Failed !!!\n");
		return false;
	}

	if (ui32TempLoopCount > 3) {
		hdmirx_wrn("HdmiRx_Audio_CalculateDividerSettings error\n");
		return false;
	}

	fMDiv = ((U32)ui32OutputFrequency * aui32PcalcTable[ui32TempLoopCount]) / ui32InputFrequency;
	ui32MFactInteger = (U32)fMDiv;
	fMFract = fMDiv - ui32MFactInteger;
	AUDIO_INF(" AUDIO PLL CALC: Stage 4 MDiv=%f MFract=%f MInt=%u \n", fMDiv, fMFract, ui32MFactInteger);

	for (ui32TempLoopCount = 1; ui32TempLoopCount < 15; ++ui32TempLoopCount) {
		if (fMFract >= (1.0 / BASE2POW(ui32TempLoopCount))) {
			ui32MFactRemainder |= BASE2POW(15 - ui32TempLoopCount);
			fMFract -= (1.0 / BASE2POW(ui32TempLoopCount));

			AUDIO_INF(" AUDIO PLL CALC: Stage 5 MFract=%f Mrem=%d aftr Corr LoopCnt=%d !!!\n", fMFract, ui32MFactRemainder, ui32TempLoopCount);
		} else {
			AUDIO_INF(" AUDIO PLL CALC: Stage 5 LoopCnt=%d No Corr \n", ui32TempLoopCount);
		}
	}

	/*
		*
		# Create Results Hash
			my %results = (
				xDiv  => $xDiv,
				pDiv  => $pDiv,
				mDiv  => $mDiv,
				mInt  => $mInt,
				mFrac => $mFrac,
			);
	*
	*/

	// pstRetParams->xDiv  = pParams->xDiv;
	// pstRetParams->pDiv  = pParams->pDiv;
	// pstRetParams->mDiv  = pParams->mDiv;
	pstRetParams->mInt = ui32MFactInteger;
	pstRetParams->mFrac = ui32MFactRemainder;

	AUDIO_INF(" AUDIO PLL CALC DBG :: Stage 6 xDiv=%d pDiv=%d mDiv=%d mInt=%d mRem=%d \n", pstRetParams->xDiv, pstRetParams->pDiv,
		pstRetParams->mDiv, pstRetParams->mInt, pstRetParams->mFrac);
#endif
	return true;
}

bool HdmiRx_Audio_InitAPLL(void *pAudioContex)
{
	// TODO, please implement this function according to rmhdmirx_Cfg_InitAudioPll
	struct HDMIRx_AudioContext *pCtx = (struct HDMIRx_AudioContext *)pAudioContex;
	ThdmiRxAudioPllDividerParams stAudioPLLDivFact = {0, 0, 0, 0, 0};
	U32 tmp = 0;
	// stAudioPLLDivFact.mDiv = 0;  // avoid no use variable

	if (!HdmiRx_Audio_CalculateDividerSettings(&pCtx->ctx, &stAudioPLLDivFact)) {
		AUDIO_INF("%s: HdmiRxAudio_InitAPLL HdmiRxAudio_CalculateDividerSettings Failed\n", __func__);
		return false;
	}
	/* Power down PLL */
	HdmiRx_Audio_PowerDownAPLL();
	HdmiRx_SetRegu8_Byu32(audio_pll_base, PLL_HDMI_AUDIO_CON1_CTL_0, AUDIO_PLL_CON1_CTL_VALUE);
	/* write P-divider */
	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0);
	tmp |= PLL_HDMI_AUDIO_DIRECTI;     // set bypass
	tmp &= ~PLL_HDMI_AUDIO_PDEC_MASK;  // clear pDiv part
	tmp |= (((U32)stAudioPLLDivFact.pDiv) << 9);
	HdmiRx_SetRegu8_Byu32(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0, tmp);
	/* Power up PLL */
	HdmiRx_Audio_PowerUpAPLL();

	/* wait for ~300us */
	do {
		tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_STA_CTL_0);
	} while (tmp & PLL_HDMI_AUDIO_BLOCKED);

	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0);
	tmp &= ~PLL_HDMI_AUDIO_BLOCK_EN;
	HdmiRx_SetRegu8_Byu32(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0, tmp);

	/* Write M-divider */
	tmp = HDMI_CTS_CLK;
	tmp |= (((U32)stAudioPLLDivFact.mFrac) & 0x7FFF);
	tmp |= ((((U32)stAudioPLLDivFact.mInt) & 0x1FF) << 15);
	HdmiRx_SetRegu8_Byu32(audio_pll_base, HDMI_AUDIO_PLLFRAC_CTL_0, tmp);

	tmp |= HDMI_AUDIO_PLLFRAC_REQ;
	HdmiRx_SetRegu8_Byu32(audio_pll_base, HDMI_AUDIO_PLLFRAC_CTL_0, tmp);

	/* Write X-divider */
	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, CGU_MISC_CTL_0);
	tmp |= SEL_HDMI_NDIV;
	tmp &= ~HDMI_X_DIVIDE_MASK;
	tmp |= (((U32)stAudioPLLDivFact.xDiv) << 0x01);
	AUDIO_INF("%s init set 0x%x xDiv = 0x%x\n", __func__, tmp, ((U32)stAudioPLLDivFact.xDiv));

	return true;
}

bool HdmiRx_Audio_UpdateAPLL(void *pAudioCtx)
{
	U8 pDiv, xDiv;
	U16 mDiv;
	U32 tmp;
	ThdmiRxAudioContext *pParams = (ThdmiRxAudioContext *)pAudioCtx;
	ThdmiRxAudioPllDividerParams stAudioPLLDivFact = {0, 0, 0, 0, 0};

	stAudioPLLDivFact.mDiv = 0;  // avoid no use variable

	if (pParams == NULL) {
		return false;
	}

	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, CGU_MISC_CTL_0);
	xDiv = (tmp & HDMI_X_DIVIDE_MASK) >> 1;

	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0);
	pDiv = (tmp & PLL_HDMI_AUDIO_PDEC_MASK) >> 9;

	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, HDMI_AUDIO_PLLFRAC_CTL_0);
	mDiv = (tmp & HDMI_AUDIO_PLLFRAC_CTRL_MASK) >> 15;
	if (!HdmiRx_Audio_CalculateDividerSettings(pParams, &stAudioPLLDivFact)) {
		return false;
	}

	if ((stAudioPLLDivFact.xDiv == xDiv) && (stAudioPLLDivFact.pDiv == pDiv)) {
	} else {
		/* Write P-divider */
		tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0);
		tmp |= PLL_HDMI_AUDIO_PREQ;
		tmp &= ~PLL_HDMI_AUDIO_PDEC_MASK;
		tmp |= (((U32)stAudioPLLDivFact.pDiv) << 9);

		HdmiRx_SetRegu8_Byu32(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0, tmp);

		do {
			tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0);

		} while (tmp & PLL_HDMI_AUDIO_PREQ);

		/* Write X-divider */
		tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, CGU_MISC_CTL_0);
		tmp |= SEL_HDMI_NDIV;
		tmp &= ~HDMI_X_DIVIDE_MASK;
		tmp |= (((U32)stAudioPLLDivFact.xDiv) << 0x01);

		AUDIO_INF("@@ update set 0x18 0x%x xDiv = 0x%x", tmp, ((U32)stAudioPLLDivFact.xDiv));

		HdmiRx_SetRegu8_Byu32(audio_pll_base, CGU_MISC_CTL_0, tmp);

		AUDIO_INF("AUDIO PLL CALC DBG :: Updated!\n");
	}
	return true;
}

bool HdmiRx_Audio_ResetAPLL(void)
{
	// TODO, please implement this function according to rmhdmirx_Cfg_ResetAudioPll
	U32 tmp = 0;

	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0);
	tmp |= (PLL_HDMI_AUDIO_PD | PLL_HDMI_AUDIO_BLOCK_EN);
	HdmiRx_SetRegu8_Byu32(audio_pll_base, PLL_HDMI_AUDIO_CON2_CTL_0, tmp);

	tmp = 0;
	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, HDMI_AUDIO_PLLFRAC_CTL_0);
	tmp |= HDMI_AUDIO_PLLFRAC_PD;
	HdmiRx_SetRegu8_Byu32(audio_pll_base, HDMI_AUDIO_PLLFRAC_CTL_0, tmp);

	tmp = 0;

	tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, CGU_MISC_CTL_0);
	tmp &= ~SEL_HDMI_NDIV;
	HdmiRx_SetRegu8_Byu32(audio_pll_base, CGU_MISC_CTL_0, tmp);

	return true;
}

bool HdmiRx_Audio_SetAPLL(void *pAudioContext)
{
	U32 tmp = 0;
	struct HDMIRx_AudioContext *pAudioCtx = (struct HDMIRx_AudioContext *)pAudioContext;
	ThdmiRxAudioContext stTempAudioPLLParams;
	memset(&stTempAudioPLLParams, 0, sizeof(ThdmiRxAudioContext));
	ThdmiRxAudioContext *pParams = &stTempAudioPLLParams;

	pParams->N = pAudioCtx->ctx.N;
	pParams->CTS = pAudioCtx->ctx.CTS;
	pParams->fs = pAudioCtx->ctx.fs;

	if ((pAudioCtx->sui32PrevN != pAudioCtx->ctx.N) || (pAudioCtx->sui32PrevFs != pAudioCtx->ctx.fs)) {
		AUDIO_INF("Audio: N change from 0x%x to 0x%x,	fs change from 0x%x to %x, CTS = 0x%x\n", pAudioCtx->sui32PrevN, pAudioCtx->ctx.N,
			pAudioCtx->sui32PrevFs, pAudioCtx->ctx.fs, pAudioCtx->ctx.CTS);
		HdmiRx_Audio_ResetAPLL();
		pAudioCtx->sui32PrevN = pAudioCtx->ctx.N;
		pAudioCtx->sui32PrevFs = pAudioCtx->ctx.fs;

		/* Init required ? */
		tmp = HdmiRx_GetRegu32_ByRegu8(audio_pll_base, HDMI_AUDIO_PLLFRAC_CTL_0);
		if ((tmp & HDMI_AUDIO_PLLFRAC_PD) == HDMI_AUDIO_PLLFRAC_PD) {
			AUDIO_INF("%s: Calling Init ", __FUNCTION__);
			HdmiRx_Audio_InitAPLL(pParams);
		} else {
			AUDIO_INF("%s: Calling Update", __FUNCTION__);
			HdmiRx_Audio_UpdateAPLL(pParams);
		}
	}
	return true;
}

bool HdmiRx_Audio_GetFsFromAPLL(void *pAudioContext)
{
	U8 data;
	struct HDMIRx_AudioContext *pAudioCtx = (struct HDMIRx_AudioContext *)pAudioContext;
	data = HDMIRx_ReadRegU8(link_base + AUDIO_FREQ_DET_STATUS);

	switch ((data & APLL_FS_MASK) >> APLL_FS_SHIFT) {
	case APLL_FS_32K:
		pAudioCtx->ctx.audioPLLfs = 32;
		break;
	case APLL_FS_44K:
		pAudioCtx->ctx.audioPLLfs = 44;
		break;
	case APLL_FS_48K:
		pAudioCtx->ctx.audioPLLfs = 48;
		break;
	case APLL_FS_88K:
		pAudioCtx->ctx.audioPLLfs = 88;
		break;
	case APLL_FS_96K:
		pAudioCtx->ctx.audioPLLfs = 96;
		break;
	case APLL_FS_176K:
		pAudioCtx->ctx.audioPLLfs = 176;
		break;
	case APLL_FS_192K:
		pAudioCtx->ctx.audioPLLfs = 192;
		break;
	default:
		pAudioCtx->ctx.audioPLLfs = 32;
	break;
	}
	return true;
}

bool HdmiRx_Audio_Get_N_CTS(void *pAudioContext)
{
	U32 n, cts;
	struct HDMIRx_AudioContext *pAudioCtx = (struct HDMIRx_AudioContext *)pAudioContext;
	n = HDMIRx_ReadRegU8(link_base + N_REGISTER_0);
	n |= (HDMIRx_ReadRegU8(link_base + N_REGISTER_1) << SHIFT_ONE_BYTE);
	n |= ((HDMIRx_ReadRegU8(link_base + N_REGISTER_2) & 0xF) << (SHIFT_ONE_BYTE * 2));

	cts = HDMIRx_ReadRegU8(link_base + CTS_REGISTER_0);
	cts |= (HDMIRx_ReadRegU8(link_base + CTS_REGISTER_1) << SHIFT_ONE_BYTE);
	cts |= ((HDMIRx_ReadRegU8(link_base + CTS_REGISTER_2) & 0xF) << (SHIFT_ONE_BYTE * 2));

	AUDIO_INF("%s: Audio N 0x%x CTS 0x%x", __func__, n, cts);
	pAudioCtx->ctx.N = n;
	pAudioCtx->ctx.CTS = cts;
	HdmiRx_Audio_GetFsFromAPLL(pAudioCtx);

	return true;
}

bool HdmiRx_Audio_GetFsFromCS(void *pAudioContext)
{
	U8 data;
	struct HDMIRx_AudioContext *pAudioCtx = (struct HDMIRx_AudioContext *)pAudioContext;
	data = HDMIRx_ReadRegU8(link_base + AUDIO_CHANNEL_STATUS4);

	switch (data & CS_SAMPLE_FREQ) {
	case 2:
		pAudioCtx->ctx.fs = 48000;
		break;
	case 3:
		pAudioCtx->ctx.fs = 32000;
		break;
	case 8:
		pAudioCtx->ctx.fs = 82000;
		break;
	case 0xa:
		pAudioCtx->ctx.fs = 88200;
		break;
	case 0xc:
		pAudioCtx->ctx.fs = 176400;
		break;
	case 0xe:
		pAudioCtx->ctx.fs = 192000;
		break;
	case 0x9:
		pAudioCtx->ctx.fs = 192000;
	break;
	default:
		pAudioCtx->ctx.fs = 44100;
		break;
	}

	return true;
}

bool HdmiRx_Audio_ResetAudioBlock(void *pAudioContext)
{
	struct HDMIRx_AudioContext *pAudioCtx = (struct HDMIRx_AudioContext *)pAudioContext;
	ThdmiRxAudioContext stTempAudioPLLParams;
	ThdmiRxAudioContext *pParams = &stTempAudioPLLParams;
	U32 tmp = 0;

	HdmiRx_Audio_GetFsFromAPLL(pAudioCtx);

	pParams->N = pAudioCtx->ctx.N;
	pParams->CTS = pAudioCtx->ctx.CTS;
	pParams->fs = pAudioCtx->ctx.audioPLLfs * 1000;

	tmp = SYSTEM_PD_AUDIO | PM_MSK_PHY_CDR_LK | PM_MSK_SIGNAL | PM_MSK_5V;
	/*reset system_pd_audio*/
	HDMIRx_WriteRegU8(link_base + SYSTEM_CONTROL_3, tmp);  // SYSTEM_PD_AUDIO SYSTEM_PD_AUDIO_T

	/*reset system_pd_audio_t*/
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO_T, SYSTEM_PD_AUDIO_T);

	HdmiRx_Audio_ResetAPLL();

	HdmiRx_Audio_InitAPLL(pParams);

	/*clear reset system_pd_audio_t*/
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO_T, 0);

	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO, 0);
	return true;
}

void HdmiRx_Audio_HBR_Patch(U8 audio_type)
{
	if (audio_type & REC_HBR) {
		HDMIRx_WriteRegU8(link_base + AUDIO_PLL_DEBUG, 0xFF);
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_FREQ_DET_CONTROL, SEL_APLL_LOL, 0x10);
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_PLL_CONTROL_1, APLL_STABLE_OVERRIDE, APLL_STABLE_OVERRIDE);
	} else {
		HDMIRx_WriteRegU8(link_base + AUDIO_PLL_DEBUG, 0x00);
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_FREQ_DET_CONTROL, SEL_APLL_LOL, 0x0);
		HDMIRx_WriteRegMaskU8(link_base + AUDIO_PLL_CONTROL_1, APLL_STABLE_OVERRIDE, APLL_STABLE_OVERRIDE);
	}
}

U8 HdmiRx_Audio_GetType(void)
{
	return HDMIRx_ReadRegU8(link_base + AUDIO_FORMAT_CONTROL) & REC_TYPES;
}

void HdmiRx_Audio_I2S_EnableOutput(bool isEnable)
{
	HDMIRx_WriteRegMaskU8(link_base + AUDIO_FORMAT_CONTROL, I2S_EN, isEnable ? I2S_EN : 0);
}

void HdmiRx_Audio_PD_Reset(void)
{
	AUDIO_INF("reset system_pd_audio_t!!!\n");
	/*reset system_pd_audio_t*/
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO, SYSTEM_PD_AUDIO);
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO_T, SYSTEM_PD_AUDIO_T);
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO_T, 0);
	HDMIRx_WriteRegMaskU8(link_base + SYSTEM_CONTROL_3, SYSTEM_PD_AUDIO, 0);
}
