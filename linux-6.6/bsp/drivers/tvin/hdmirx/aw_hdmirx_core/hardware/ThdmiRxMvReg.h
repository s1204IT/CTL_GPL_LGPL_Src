#ifndef _THDCPRXMVREG_H_
#define _THDCPRXMVREG_H_

#define HDCPRX_MV_P0_BASE_ADDR 0x19C01200
#define HDCPRX_MV_P1_BASE_ADDR 0x19C01400
#define HDCPRX_MV_P2_BASE_ADDR 0x19C01600
#define HDCPRX_MV_P3_BASE_ADDR 0x19C01800

#define HDCPRX_MV_TOP_BASE_ADDR 0x1000

#if 1
#define MVPORT_MULTIVIEW_PORT_SYNC_RESET (0x00)
#define MVPORT_SYSTEM_PD_LIB_MV _BIT3_
/*
0: No reset
1: Reset multiview block
*/
#define MVPORT_SYSTEM_PD_CLK_10BIT _BIT2_
/*
0: No reset
1: All other clkbus domain
*/
#define MVPORT_SYSTEM_PD_HDCP _BIT1_
/*
0: No reset
1: Reset HDCP block (clkbus domain)
*/
#define MVPORT_SYSTEM_PD_DDC _BIT0_
/*
    0: No reset
    1: Reset I2C slave block
*/

#define MVPORT_HDCP_22_RX_Message_Control_1 (0x10)

// HDCP22 related.
#define MVPORT_HDCP_22_RX_Message_Control_1 (0x10)
// '7:0 hdcp22_i2crx_length[7:0] Number of bytes received
#define MVPORT_HDCP_22_RX_Message_Control_2 (0x11)
#define MVPORT_HDCP22_I2CRX_LENGTH_8_9 (_BIT0_ | _BIT1_)
//'1:0 hdcp22_i2crx_length[9:8]

#define MVPORT_HDCP_22_RX_Message_Control_3 (0x12)
#define MVPORT_HDCP22_I2CRX_RESET_WPTR _BIT0_
/*
0: Do nothing
1: Reset write pointer to 0
*/
#define MVPORT_HDCP22_I2CRX_RESET_RPTR _BIT1_
/*0: Do nothing
 *1: Reset read pointer to 0
*/
#define MVPORT_HDCP22_I2CRX_SEL_DEBUG_PORT _BIT6_
/*0: Message will be available to read from offset 0x80 (Read_Message)
1: Message will be available to read from offset 0xC0 (dbg area)
*/
#define MVPORT_HDCP22_I2CRX_TRIG _BIT7_
/*
   0: Do nothing
   1: Enable I2C transaction.  All I2C parameters (length, message_size, and data) need to be loaded prior to setting this bit.
   */

#define MVPORT_HDCP_22_MESSAGE_LOAD_WINDOW (0x15)
#define MVPORT_HDCP_22_MESSAGE_UNLOAD_WINDOW (0x16)
//'7:0 hdcp22_i2crx_rcv_message Read received message from this register.  Read multiple times.

#define MVPORT_HDCP_22_INTERRUPT_REASON (0x17)
#define MVPORT_HDCP22_I2CRX_INT_MESSAGE_RCVED _BIT0_
// Message was received and it's ready to be read by software
#define MVPORT_HDCP22_I2CRX_INT_MESSAGE_SENT _BIT1_
// Message was sent successfully and ready to accept and send next message.
#define MVPORT_HDCP22_I2CRX_INT_MESSAGE_ERROR _BIT2_
// Message was not sent successfully.

#define MVPORT_HDCP_22_INTERRUPT_MASK (0x18)
#define MVPORT_HDCP22_I2CRX_INT_MASK_MESSAGE_RCVED _BIT0_
/*bit0
 *  0: Interrupt disabled (Interrupt reason will not be set)
 *  1: Interrupt enabled (Interrupt reason will be set)
**/
#define MVPORT_HDCP22_I2CRX_INT_MASK_MESSAGE_SENT _BIT1_
#define MVPORT_HDCP22_I2CRX_INT_MASK_MESSAGE_ERROR _BIT2_
#define MVPORT_I2C_SEND_MESSAGE_BYTE_COUNT_READY _BIT7_
/*bit7
 *  0: Cleared automatically by HW after message is sent.
 *  1: After loading 2 bytes message count, set this bit to present both bytes at once over SCDCS.
**/
#define MVPORT_HDCP2VERSION (0x1A)
#define MVPORT_HDCP22_ENABLE _BIT2_
#define MVPORT_HDCP2RXSTATUS_1 (0x1B)

#define MVPORT_HDCP2RXSTATUS_2 (0x1C)
#define MVPORT_HDCP22_MESSAGE_SIZE (_BIT0_ | _BIT1_)
#define MVPORT_HDCP22_READY _BIT2_
#define MVPORT_HDCP22_REAUTH_REQ _BIT3_
#define MVPORT_RSVD (_BIT4_ | _BIT5_ | _BIT6_ | _BIT7_)

#define MVPORT_HDCP_22_Power (0x20)

#define MVPORT_HDCP22_RAM_PD_RX_SEND_MSG _BIT2_
/**/
#define MVPORT_HDCP22_RAM_PD_RX_RCV_MSG _BIT3_
/**/
#define MVPORT_HDCP22_RAM_PD_AES _BIT4_
/**/
#define MVPORT_HDCP22_CONTROL_RE_RCV _BIT6_
/**/
#define MVPORT_HDCP22_SYNC_RESET_HDCP22 _BIT7_
/* SYSTEM_PD*/

#define MVPORT_HDCP22_UPDATE_KEY (0x22)
#define MVPORT_HDCP22_AUTHENTICATED _BIT0_
/*
0: Not authenticated
1: Authenticated
*/
#define MVPORT_HDCP22_UPDATE_AES_KEY _BIT1_
/* SOFTWARE WRITE 1 TO UPDATE THE RANDOM IV VALUE TO NEWLY WRITTEN VALUE.  RANDOM IV IS LATCHED AND USED FOR THE FOLLOWING SESSION.  WITHOUT
 * THIS BIT, RANDOM IV WILL NOT BE UPDATED.*/
#define MVPORT_HDCP22_UPDATE_RIV _BIT2_
/* SOFTWARE WRITE 1 TO UPDATE THE RANDOM IV VALUE TO NEWLY WRITTEN VALUE.  RANDOM IV IS LATCHED AND USED FOR THE FOLLOWING SESSION.  WITHOUT
 * THIS BIT, RANDOM IV WILL NOT BE UPDATED.*/
#define MVPORT_HDCP22_SWAP_AES_P_INPUT _BIT3_
/* SWAP P (RANDOM_IV || FRAMENUMBER || DATANUMBER) BYTES TO COVER LITTLE/BIG ENDIANESS*/
#define MVPORT_HDCP22_SWAP_AES_OUTPUT _BIT4_
/* SWAP AES-CTR OUTPUT BYTES TO COVER LITTLE/BIG ENDIANESS*/

#define MVPORT_HDCP_22_SESSION_KEY_0 (0x30)
#define MVPORT_HDCP_22_RANDOM_IV_0 (0x40)

#define MVPORT_MVP_VHTOTAL_0 (0x50)
#define MVPORT_MVP_VHTOTAL_1 (0x51)
#define MVPORT_MVP_VHTOTAL_2 (0x52)
#define MVPORT_MVP_VHTOTAL_3 (0x53)

#define MVPORT_MVP_CONFIG (0x54)
#define MVPORT_VH_QUATER (_BIT7_ | _BIT6_ | _BIT5_ | _BIT4_)
/**/
/*
If vh_total value is not integer, use this bits accordingly.
4'b0000: *.0 (integer)
4'b0001: *.25
4'b0011: *.5
4'b0111: *.75
all others are not allowed
These are mainly used for deep color formats.
I.e., for 480p 10bit, vh_total_in = vtotal*htotal*1.25=563062.5, so vh_total_in = 563062, vh_quater = 4'b0011
*/
#define MVPORT_SAFE_WINDOW_PIX (_BIT3_ | _BIT2_ | _BIT1_ | _BIT0_)
/**/
/*
Indicates keep out window for port switching.  After software issues port switch, HW executes port switch avoiding this period.
It should avoid near framekey generation, which takes place right after enc_en signaling.
4'h0: safe_window = 0*256 tmds clocks before and after enc_en pulse
4'h1: safe_window = 1*256 tmds clocks before and after enc_en pulse
4'h2: safe_window = 2*256 tmds clocks before and after enc_en pulse
etc.  Must be at least 1.  Default is 2.
Note that definition of safe_window_pix is different from safe_window in the link side register.  They serve the same purpose, but different
units are used.
*/

#define MVPORT_MVP_MUX_CONTROL_1 (0x55)
#define EN_INPUT_SYNC_SUB _BIT7_
#define EN_OUTPUT_SYNC_SUB _BIT6_
#define EN_OUTPUT_DATA_SUB _BIT5_
#define CONN_SUB _BIT4_
#define MVPORT_MVP_MUX_SUB_MASK (EN_INPUT_SYNC_SUB | EN_OUTPUT_SYNC_SUB | EN_OUTPUT_DATA_SUB | CONN_SUB)

#define EN_INPUT_SYNC_MAIN _BIT3_
#define EN_OUTPUT_SYNC_MAIN _BIT2_
#define EN_OUTPUT_DATA_MAIN _BIT1_
#define CONN_MAIN _BIT0_
#define MVPORT_MVP_MUX_MAIN_MASK (EN_INPUT_SYNC_MAIN | EN_OUTPUT_SYNC_MAIN | EN_OUTPUT_DATA_MAIN | CONN_MAIN)

#define MVPORT_MVP_MUX_CONTROL_2 (0x56)
#define MVPORT_SLOW_CLKBUS _BIT7_
/*
0: clkbus frequency is same as tmds_clk (must be cleared when dbg_bypass_mode=1 or bypass_mode=1)
1: clkbus is 1/4 frequency of tmds_clk (Set when 4K2Kp60)
*/
#define MVPORT_DBG_BYPASS_MODE _BIT6_
/*
0: Use the normal 4-1 glitch free MUX
1: Use bypass_mode without the glitch free MUX
*/
#define MVPORT_SEL_HDCP22 _BIT5_
/*
0: Using HDCP 1.4 encryption or no encryption
1: Using HDCP 2.2 encryption
*/
#define MVPORT_BYPASS_MODE _BIT4_
/*
0: When 4K2Kp60 mode with RXC = 1/4 of tmds_clk, then use "0" to pass tmds_clk as clk_mv
1: For all other cases, use clkbus for clk_mv
*/
#define MVPORT_EN_AUTO_SW_CTRL _BIT3_
/*
0: Force MV switch using below registers (debug only).
1: Wait for safe_window to switch.
*/
#define MVPORT_PORT_SEL_MAN _BIT2_
/*
0: ENC_EN comes from stream
1: ENC_EN comes from MV's enc_en generator
*/
#define MVPORT_PORT_SEL_SUB_MAN _BIT1_
/**/
#define MVPORT_PORT_SEL_MAIN_MAN _BIT0_
/*
Used to select which data stream comes into MV block
*/

#define MVPORT_MVP_I2C_CONTROL (0x57)
#define MVPORT_DDCSCL_OUT_OVERRIDE _BIT0_  // Bit 0 ddcscl_out_override: Used to clear HDCP2.2 RX packet byte count
#define MVPORT_HDCP22_DEFAULT_OFFSET _BIT1_
#define MVPORT_DDCSCL_IN_MOM _BIT4_
#define MVPORT_DDCSDA_IN_MON _BIT5_
#define MVPORT_CLR_HDCP22_PKT_LEN _BIT6_
/* : USED TO CLEAR HDCP2.2 RX RECEIVED PACKET U8 COUNT
0: Enable i2c_rx_length counting.
1: Clear i2c_rx_length.  This bit is cleared by HW in the next cycle.
*/

#define MVPORT_MVP_HDCP14_INTERRUPT_UPDATE_MASK (0x67)
#define MVP_INT_UPDATE_HDCP14_ENC_DIS _BIT3_
/**/
#define MVP_INT_UPDATE_HDCP14_ENC_START _BIT2_
/**/
#define MVP_INT_UPDATE_HDCP14_AUTHENTICATED _BIT1_
/**/
#define MVP_INT_UPDATE_HDCP14_AKSV_UPDATED _BIT0_
/**/

#define MVP_HDCP14_INTERRUPT_REASON (0x68)
#define MVP_INT_HDCP14_ENC_DIS _BIT3_
/**/
#define MVP_INT_HDCP14_ENC_START _BIT2_
/**/
#define MVP_INT_HDCP14_AUTHENTICATED _BIT1_
/**/
#define MVP_INT_HDCP14_AKSV_UPDATED _BIT0_
/**/

#define MVP_HDCP14_INTERRUPT_OUTPUT_ENABLE (0x69)
#define MVP_INT_EN_HDCP14_ENC_DIS _BIT3_
/**/
#define MVP_INT_EN_HDCP14_ENC_START _BIT2_
/**/
#define MVP_INT_EN_HDCP14_AUTHENTICATED _BIT1_
/**/
#define MVP_INT_EN_HDCP14_AKSV_UPDATED _BIT0_
/**/

#define MVP_SWITCH_INTERRUPT_REASON (0x6b)
#define MVP_INT_SUB_ERR _BIT3_
/**/
#define MVP_INT_SUB_DONE _BIT2_
/**/
#define MVP_INT_MAIN_ERR _BIT1_
/**/
#define MVP_INT_MAIN_DONE _BIT0_
/**/

#define MVPORT_MVP_SWITCH_STATUS (0x6d)
#define MVPORT_CONN_STATUS_SUB (_BIT3_ | _BIT4_ | _BIT5_)
/*
Bit.5:3 conn_status_sub
bit2 - 0: sub connection change no error
bit2 - 1: sub connection change error
bit1 - 0: sub connection change is done
bit1 - 1: sub connection change is in progress
bit0 - 0: sub is disconnected
Bit0 - 1: sub is connected
*/
#define MVPORT_CONN_STATUS_SUB_CONNECT_ERROR _BIT5_
#define MVPORT_CONN_STATUS_SUB_CONNECT_PROG _BIT4_
#define MVPORT_CONN_STATUS_SUB_CONNECT _BIT3_

#define MVPORT_CONN_STATUS_MAIN (_BIT0_ | _BIT1_ | _BIT2_)
/*
Bit.2:0 conn_status_main
bit2 - 0: main connection change no error
bit2 - 1: main connection change error

bit1 - 0: main connection change is done
bit1 - 1: main connection change is in progress

bit0 - 0: main is disconnected
Bit0 - 1: main is connected
*/
#define MVPORT_CONN_STATUS_MAIN_CONNECT_ERROR _BIT2_
#define MVPORT_CONN_STATUS_MAIN_CONNECT_PROG _BIT1_
#define MVPORT_CONN_STATUS_MAIN_CONNECT _BIT0_

#define MVPORT_MVP_HDCP_1_4_CONTROL (0x81)
#define MVPORT_HDCP_NO_DECRYPT_OVERRIDE _BIT5_
/**/
#define MVPORT_FORCE_RI_ERR _BIT4_
/**/
#define MVPORT_RESET_RETRY_CNT _BIT3_
/**/
#define MVPORT_RI_RST_CTL _BIT2_
/**/
#define MVPORT_ECO_SEL _BIT1_
/**/
#define MVPORT_REVERT_QUICK_DEC _BIT0_
/**/

#define MVP_HDCP_1_4_STATUS_0 (0x82)
#define HDCP14_AUTHENTICATED _BIT6_
/**/
#define HDCP14_AKSV_UPDATED _BIT5_
/**/
#define HDCP14_ENC_ON _BIT4_
/**/
#define HDCP14_NUM_AUTH_RETRIED (_BIT3_ | _BIT2_ | _BIT1_ | _BIT0_)
/**/

#define MVPORT_HDCP_SHORT_READ (0x87)
#define MVPORT_SEL_HDCP22_DEFAULT_OFFSET _BIT0_
/*
0: Select HDCP1.4's default offset during HDCP short read (Clear this bit after detecting aksv_updated interrupt.)
1: Select HDCP2.2's default offset during HDCP short read (Set at idle and whenever HDCP1.4 is not on.)
*/

// SCDC related.
#define MVP_SCDCS_CONTROL_0 (0x5a)

#define MVP_LLED_RR_EN (_BIT6_)
/*
0: Disable Read Request due to LLED update
1: Enable Read Request due to LLED update
*/
#define MVP_SEL_AUTO_STATUS _BIT4_
/*
0: Manually write locked values into registers.
1: Use signal_valid and phy_cdr_locked for DDC's status register.
*/
#define MVP_RR_ENABLE _BIT0_
/*
[0] rr_enable
Read request enable
*/

#define MVPORT_SCDC_SOURCE_VERSION (0x102)

#define MVPORT_SCDC_UPDATE_0 (0x110)
#define MVPORT_STATUS_UPDATE _BIT0_
#define MVPORT_CED_UPDATE _BIT1_
#define MVPORT_RR_TEST _BIT2_

#define MVPORT_SCDC_TMDS_CONFIG (0x120)
#define MVPORT_TMDS_BIT_CLOCK_RATIO _BIT1_
#define MVPORT_TMDS_BIT_CLOCK_RATIO_SHIFT (1)
#define MVPORT_SCRAMBLINE_ENABLE _BIT0_

#define MVPORT_SCDC_SCRAMBLE_STATUS (0x121)
#define MVPORT_SCRAMBLING_STATUS _BIT0_

// For Tx
#define MVPORT_SCDC_CONFIG_0 (0x130)
#define MVPORT_RR_ENABLE _BIT0_

#define MVPORT_SCDC_STATUS_0 (0x140)
#define MVPORT_CLOCK_DETECTED _BIT0_
#define MVPORT_CH0_LOCKED _BIT1_
#define MVPORT_CH1_LOCKED _BIT2_
#define MVPORT_CH2_LOCKED _BIT3_

#define MVPORT_SCDC_TEST_CONFIG_0 (0x1C0)

#endif

/*********MVP top level reg*************/
#define MVPORT_MULTIVIEW_CONTROL (0x0)

#define MVPORT_EN_MV_MODE _BIT7_
/*
0: Disable Multi-view function
1: Enable Multi-view function
*/
#define MVPORT_EN_MV_CLOCKS _BIT6_
/*
0: Disable all clocks going into MV block except for accessing the lib_mv_top registers.
1: Enable all clocks going into MV block
*/
#define MVPORT_MULTIVIEW_MUX_CONTROL (0x1)
#if 0
#define MVPORT_SEL_INST_1_PHY_DATA _BIT7_
/*
0: Select dig_top_0 PHY data output, pclk, and tclk for FPGA debug path
1: Select dig_top_1 PHY data output, pclk, and tclk for FPGA debug path
*/
#define MVPORT_SEL_INST_1_AUPLL _BIT6_
/*
0: Select dig_top_0 register outputs for audio pll params.
Also, select dig_top_0 audio data outputs for HBR_MIF.
1: Select dig_top_1 register outputs for audio pll params.
Also, select dig_top_1 audio data outputs for HBR_MIF.
*/
#define MVPORT_RESERVED _BIT5_
/**/
#define MVPORT_SEL_INST_1_REG_OUTPUT _BIT4_
/*
0: Select dig_top_0 register outputs for phy_clk_en, ARC, and get_hdcp_key params.
1: Select dig_top_1 register outputs for phy_clk_en, ARC, and get_hdcp_key params.
*/
#endif
#define MVPORT_EN_INST_1_OUTPUT _BIT3_
/*
0: Disable sda_out when en_mv_mode=0, and disable data output to PHY(control_area, rx_area, code_err, audio_fifo pointers) from dig_top_1
(sub link)
1: Enable sda_out when en_mv_mode=0, and enable data output to PHY(control_area, rx_area, code_err, audio_fifo pointers)  from dig_top_1
(sub link)
*/
#define MVPORT_EN_INST_0_OUTPUT _BIT2_
/*
0: Disable sda_out when en_mv_mode=0, and disable data output to PHY(control_area, rx_area, code_err, audio_fifo pointers) from dig_top_0
(main link)
1: Enable sda_out when en_mv_mode=0, and enable data output to PHY(control_area, rx_area, code_err, audio_fifo pointers)  from dig_top_0
(main link)
*/
#define MVPORT_EN_INST_1_INPUT _BIT1_
/*
0: Disable data input (from PHY, all clocks, from hdmiapll) to dig_top_1 (sub link)
1: Enable data input (from PHY, all clocks, from hdmiapll) to dig_top_1 (sub link)
*/
#define MVPORT_EN_INST_0_INPUT _BIT0_
/*
0: Disable data input (from PHY, all clocks, from hdmiapll) to dig_top_0 (main link)
1: Enable data input (from PHY, all clocks, from hdmiapll) to dig_top_0 (main link)
*/

#define MVTOP_INTERRUPT (0x05)
#define MVTOP_INTERRUPT_TOP_0 _BIT4_
#define MVTOP_INTERRUPT_TOP_1 _BIT5_
#define MVTOP_INTERRUPT_TOP_2 _BIT6_
#define MVTOP_INTERRUPT_TOP_3 _BIT7_

#define MVTOP_INTERRUPT_ENABLE (0x06)
#define MVTOP_INTERRUPT_ENABLE_TOP_0 _BIT4_
#define MVTOP_INTERRUPT_ENABLE_TOP_1 _BIT5_
#define MVTOP_INTERRUPT_ENABLE_TOP_2 _BIT6_
#define MVTOP_INTERRUPT_ENABLE_TOP_3 _BIT7_

#define MVTOP_AUDIO_MUTE_INTERRUPT_ENABLE_0 (0x0A)

#define MVTOP_AUDIO_MUTE_INTERRUPT_ENABLE_1 (0x0B)

#define MVTOP_INTERRUPT_INDEX (0x13)

#define MVTOP_ICG_CONTROL_0 (0x14)
#define EN_PHY_CLK _BIT0_
#define EN_PHY_REPEAT_CLK _BIT1_

#define MVTOP_ICG_CONTROL_1 (0x15)
#define EN_HDMIAPLL_0 _BIT0_
#define EN_ACLK128_0 _BIT1_
#define EN_AUDIO_LGC_0 _BIT2_
#define EN_AUDIO_CLK_0 _BIT3_
#define EN_ACLK_ROOT_0 _BIT4_
#define EN_MCLK_0 _BIT5_
#define EN_MCLK_ROOT_0 _BIT6_

#define MVTOP_ICG_CONTROL_2 (0x16)
#define EN_PCLK2X_0 _BIT0_
#define EN_TMDS10_DIV2_CLK_0 _BIT1_
#define EN_IDCLK_OUT_REPEAT_ROOT_0 _BIT2_
#define EN_IDCLK_OUT_REPEAT_LEAF_0 _BIT3_
#define EN_IDCLK_OUT_ROOT_0 _BIT4_
#define EN_IDCLK_OUT_LEAF_0 _BIT5_
#define EN_IDCLK_OUT_EDR_ROOT_0 _BIT6_
#define EN_IDCLK_OUT_EDR_LEAF_0 _BIT7_

#define MVTOP_ICG_CONTROL_3 (0x17)
#define EN_VCLKB25_0 _BIT0_
#define EN_TCLK50_0 _BIT1_
#define EN_CLKBUS_0 _BIT2_
#define EN_IDCLK_OUT_420_0 _BIT3_
#define EN_CLKCPU_0 _BIT4_
#define EN_CLKREF_0 _BIT5_

#define MVTOP_ICG_CONTROL_4 (0x18)
#define EN_HDMIAPLL_1 _BIT0_
#define EN_ACLK128_1 _BIT1_
#define EN_AUDIO_LGC_1 _BIT2_
#define EN_AUDIO_CLK_1 _BIT3_
#define EN_ACLK_ROOT_1 _BIT4_
#define EN_MCLK_1 _BIT5_
#define EN_MCLK_ROOT_1 _BIT6_

#define MVTOP_ICG_CONTROL_5 (0x19)
#define EN_PCLK2X_1 _BIT0_
#define EN_TMDS10_DIV2_CLK_1 _BIT1_
#define EN_IDCLK_OUT_REPEAT_ROOT_1 _BIT2_
#define EN_IDCLK_OUT_REPEAT_LEAF_1 _BIT3_
#define EN_IDCLK_OUT_ROOT_1 _BIT4_
#define EN_IDCLK_OUT_LEAF_1 _BIT5_
#define EN_IDCLK_OUT_EDR_ROOT_1 _BIT6_
#define EN_IDCLK_OUT_EDR_LEAF_1 _BIT7_

#define MVTOP_ICG_CONTROL_6 (0x1A)
#define EN_VCLKB25_1 _BIT0_
#define EN_TCLK50_1 _BIT1_
#define EN_CLKBUS_1 _BIT2_
#define EN_IDCLK_OUT_420_1 _BIT3_
#define EN_CLKCPU_1 _BIT4_
#define EN_CLKREF_1 _BIT5_

#define MVTOP_ICG_CONTROL_7 (0x1B)
#define EN_MV_CLK_0 _BIT0_
#define EN_MV_CLK_1 _BIT1_
#define EN_MV_CLK_2 _BIT2_
#define EN_MV_CLK_3 _BIT3_

#define MVTOP_MUX_CONTROL_2 (0x1C)
#define SEL_INST_PHY_DATA (_BIT7_ | _BIT6_)
#define SEL_INST_AUPLL (_BIT5_ | _BIT4_)
#define SEL_INST_PHYREG_OUTPUT (_BIT3_ | _BIT2_)
#define SEL_INST_REG_OUTPUT (_BIT1_ | _BIT0_)

#define MVTOP_MUX_CONTROL_3 (0x1D)
#define EN_INST_2_INPUT _BIT0_
#define EN_INST_3_INPUT _BIT1_
#define EN_INST_2_OUTPUT _BIT2_
#define EN_INST_3_OUTPUT _BIT3_

#define MVTOP_MUX42_MASTER_SELECT (0x4A)
#define USE_MASTER_SELECT_1 _BIT6_
/*
0: Do not use master_select_1 bits
1: Use master_select_1 to select 4:2 MUX for normal_video_1, edr_1, repeat_1, phy_dump_1, and audio_1.
*/
#define MASTER_SELECT_1 (_BIT5_ | _BIT4_)
/*
00: Map dig_top_0 output to normal_video_1/edr_1/repeat_1/phy_dump_1/audio_1
01: Map dig_top_1 output to normal_video_1/edr_1/repeat_1/phy_dump_1/audio_1
10: Map dig_top_2 output to normal_video_1/edr_1/repeat_1/phy_dump_1/audio_1
11: Map dig_top_3 output to normal_video_1/edr_1/repeat_1/phy_dump_1/audio_1
*/
#define USE_MASTER_SELECT_0 _BIT2_
/*
0: Do not use master_select_0 bits
1: Use master_select_0 to select 4:2 MUX for normal_video_0/v_panel_420, edr_0, repeat_0, phy_dump_0, and audio_0.
*/
#define MASTER_SELECT_0 (_BIT1_ | _BIT0_)
/*
00: Map dig_top_0 output to normal_video_0/v_panel_420/edr_0/repeat_0/phy_dump_0/audio_0
01: Map dig_top_1 output to normal_video_0/v_panel_420/edr_0/repeat_0/phy_dump_0/audio_0
10: Map dig_top_2 output to normal_video_0/v_panel_420/edr_0/repeat_0/phy_dump_0/audio_0
11: Map dig_top_3 output to normal_video_0/v_panel_420/edr_0/repeat_0/phy_dump_0/audio_0
*/

#define MVTOP_MUX42_INDIVIDUAL_SELECT_0 (0x4B)
#define INDIVIDUAL_PORT_SELECT_FOR_NORMAL_VIDEO_PATH_1 (_BIT3_ | _BIT2_)
#define INDIVIDUAL_PORT_SELECT_FOR_NORMAL_VIDEO_PATH_0 (_BIT1_ | _BIT0_)

#define MVTOP_MUX42_INDIVIDUAL_SELECT_1 (0x4C)
#define INDIVIDUAL_PORT_SELECT_FOR_REPEATER_VIDEO_PATH_1 (_BIT7_ | _BIT6_)
#define INDIVIDUAL_PORT_SELECT_FOR_REPEATER_VIDEO_PATH_0 (_BIT5_ | _BIT4_)
#define INDIVIDUAL_PORT_SELECT_FOR_EDR_VIDEO_PATH_1 (_BIT3_ | _BIT2_)
#define INDIVIDUAL_PORT_SELECT_FOR_EDR_VIDEO_PATH_0 (_BIT1_ | _BIT0_)

#define MVTOP_MUX42_INDIVIDUAL_SELECT_2 (0x4D)
#define INDIVIDUAL_PORT_SELECT_FOR_AUDIO_PATH_1 (_BIT7_ | _BIT6_)
#define INDIVIDUAL_PORT_SELECT_FOR_AUDIO_PATH_0 (_BIT5_ | _BIT4_)
#define INDIVIDUAL_PORT_SELECT_FOR_PHY_DUMP_PATH_1 (_BIT3_ | _BIT2_)
#define INDIVIDUAL_PORT_SELECT_FOR_PHY_DUMP_PATH_0 (_BIT1_ | _BIT0_)

#define MVTOP_ICG_CONTROL_8 (0x50)
#define EN_HDMIAPLL_2 _BIT0_
#define EN_ACLK128_2 _BIT1_
#define EN_AUDIO_LGC_2 _BIT2_
#define EN_AUDIO_CLK_2 _BIT3_
#define EN_ACLK_ROOT_2 _BIT4_
#define EN_MCLK_2 _BIT5_
#define EN_MCLK_ROOT_2 _BIT6_

#define MVTOP_ICG_CONTROL_9 (0x51)
#define EN_PCLK2X_2 _BIT0_
#define EN_TMDS10_DIV2_CLK_2 _BIT1_
#define EN_IDCLK_OUT_REPEAT_ROOT_2 _BIT2_
#define EN_IDCLK_OUT_REPEAT_LEAF_2 _BIT3_
#define EN_IDCLK_OUT_ROOT_2 _BIT4_
#define EN_IDCLK_OUT_LEAF_2 _BIT5_
#define EN_IDCLK_OUT_EDR_ROOT_2 _BIT6_
#define EN_IDCLK_OUT_EDR_LEAF_2 _BIT7_

#define MVTOP_ICG_CONTROL_10 (0x52)
#define EN_VCLKB25_2 _BIT0_
#define EN_TCLK50_2 _BIT1_
#define EN_CLKBUS_2 _BIT2_
#define EN_IDCLK_OUT_420_2 _BIT3_
#define EN_CLKCPU_2 _BIT4_
#define EN_CLKREF_2 _BIT5_

#define MVTOP_ICG_CONTROL_11 (0x53)
#define EN_HDMIAPLL_3 _BIT0_
#define EN_ACLK128_3 _BIT1_
#define EN_AUDIO_LGC_3 _BIT2_
#define EN_AUDIO_CLK_3 _BIT3_
#define EN_ACLK_ROOT_3 _BIT4_
#define EN_MCLK_3 _BIT5_
#define EN_MCLK_ROOT_3 _BIT6_

#define MVTOP_ICG_CONTROL_12 (0x54)
#define EN_PCLK2X_3 _BIT0_
#define EN_TMDS10_DIV2_CLK_3 _BIT1_
#define EN_IDCLK_OUT_REPEAT_ROOT_3 _BIT2_
#define EN_IDCLK_OUT_REPEAT_LEAF_3 _BIT3_
#define EN_IDCLK_OUT_ROOT_3 _BIT4_
#define EN_IDCLK_OUT_LEAF_3 _BIT5_
#define EN_IDCLK_OUT_EDR_ROOT_3 _BIT6_
#define EN_IDCLK_OUT_EDR_LEAF_3 _BIT7_

#define MVTOP_ICG_CONTROL_13 (0x55)
#define EN_VCLKB25_3 _BIT0_
#define EN_TCLK50_3 _BIT1_
#define EN_CLKBUS_3 _BIT2_
#define EN_IDCLK_OUT_420_3 _BIT3_
#define EN_CLKCPU_3 _BIT4_
#define EN_CLKREF_3 _BIT5_

#endif
