#ifndef __XR_RPMSG_H__
#define __XR_RPMSG_H__

#ifdef USE_RPMSG_IN_LINUX

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/aw_rpmsg.h>
#include <uapi/linux/rpmsg.h>
#include <linux/mutex.h>
#include <linux/slab.h>


#define XRADIO_RPMSG_NAME "xradio_rpmsg"


#define XRADIO_FUNC_CALL_HOST_TO_SLAVE(n)					(n<<15)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_ID_MASK              (3<<13)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_CONNECTED_ID     	(1<<13)
#define XRADIO_FUNC_CALL_SLAVE_TO_HOST_ACK_ID     			(1<<14)
#define XRADIO_FUNC_CALL_INIT_ID     						(1)
#define XRADIO_FUNC_CALL_DEINIT_ID     						(2)
#define XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID 				(3)
#define XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID     		(4)
#define XRADIO_FUNC_CALL_VERSION_CHECK_ID     				(5)
#define XRADIO_FUNC_CALL_INIT_ID_ETF     						(6)
#define XRADIO_FUNC_CALL_DEINIT_ID_ETF     						(7)
#define XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID_ETF 				(8)
#define XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID_ETF     		(9)
#define XRADIO_FUNC_CALL_VERSION_CHECK_ID_ETF     				(10)

int xradio_func_call_extern_phy(int func_id, void *data);
int xradio_rpmsg_init(void);
void xradio_rpmsg_deinit(void);


#endif
#endif