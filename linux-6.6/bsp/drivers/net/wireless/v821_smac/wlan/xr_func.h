/*
 * Data Transmission thread for XRadio drivers
 *
 * Copyright (c) 2013, XRadio
 * Author: XRadio
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef XRADIO_FUNC_H
#define XRADIO_FUNC_H

#include "platform.h"
#include "wsm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char Phy_Version[];

#if (CONFIG_WLAN_FUNC_CALL_SUPPORT)
#ifdef USE_RPMSG_IN_LINUX
#include "xr_rpmsg.h"

#define xradio_func_call_init()   						xradio_func_call_extern_phy(XRADIO_FUNC_CALL_INIT_ID, NULL)
#define xradio_func_call_deinit()						xradio_func_call_extern_phy(XRADIO_FUNC_CALL_DEINIT_ID, NULL)
#define xradio_fw_call_host_func_handle()				xradio_func_call_extern_phy(XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID, NULL)
#define xradio_func_call_info_indication_handle(buf)	xradio_func_call_extern_phy(XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID, buf)
#define xradio_func_call_version_check(fw_version)		xradio_func_call_extern_phy(XRADIO_FUNC_CALL_VERSION_CHECK_ID, fw_version)

#define xradio_func_call_init_etf()   						xradio_func_call_extern_phy(XRADIO_FUNC_CALL_INIT_ID_ETF, NULL)
#define xradio_func_call_deinit_etf()						xradio_func_call_extern_phy(XRADIO_FUNC_CALL_DEINIT_ID_ETF, NULL)
#define xradio_fw_call_host_func_handle_etf()				xradio_func_call_extern_phy(XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID_ETF, NULL)
#define xradio_func_call_info_indication_handle_etf(buf)	xradio_func_call_extern_phy(XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID_ETF, buf)
#define xradio_func_call_version_check_etf(fw_version)		xradio_func_call_extern_phy(XRADIO_FUNC_CALL_VERSION_CHECK_ID_ETF, fw_version)

#else
int xradio_func_call_init(void);
void xradio_func_call_deinit(void);
void xradio_fw_call_host_func_handle(void);
int xradio_func_call_info_indication_handle(struct wsm_buf *buf);
void xradio_func_call_version_check(char *fw_version);

int xradio_func_call_init_etf(void);
void xradio_func_call_deinit_etf(void);
void xradio_fw_call_host_func_handle_etf(void);
int xradio_func_call_info_indication_handle_etf(struct wsm_buf *buf);
void xradio_func_call_version_check_etf(char *fw_version);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* XRADIO_BH_H */
