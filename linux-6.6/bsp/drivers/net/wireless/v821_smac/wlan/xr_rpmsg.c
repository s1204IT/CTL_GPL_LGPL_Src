#include "xradio.h"
#include "debug.h"
#include "wsm.h"
#include "xr_func.h"
#include <linux/semaphore.h>


#ifdef USE_RPMSG_IN_LINUX
static struct rpmsg_device *xradio_rpmsg_dev;
static struct semaphore rpmsg_lock;

void xradio_up(struct semaphore *sem)
{
	if (sem->count)
		sem->count = 1;
	else
		up(sem);
}

int xradio_func_call_extern_phy(int func_id, void *data)
{
	char *buff;
	u16 frame_ctrl = 0;
	int len = 0;
	int ret = 0;

	down(&rpmsg_lock);

	switch (func_id) {
	case XRADIO_FUNC_CALL_INIT_ID:
	case XRADIO_FUNC_CALL_DEINIT_ID:
	case XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID:
		buff = kzalloc(2, GFP_KERNEL);
		frame_ctrl |=  XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) | func_id;
		break;
	case XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID: {
		struct wsm_hdr *wsm = (struct wsm_hdr *)(((struct wsm_buf *)(data))->begin);
		len = wsm->len;
		buff = kzalloc(len + 2, GFP_KERNEL);
		memcpy(buff + 2, wsm, len);
		frame_ctrl |=  XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) | func_id;
		break;
		}
	case XRADIO_FUNC_CALL_VERSION_CHECK_ID:
		buff = kzalloc(WSM_FW_LABEL + 2, GFP_KERNEL);
		memcpy(buff + 2, data, WSM_FW_LABEL);
		frame_ctrl |=  XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) | func_id;
		len = WSM_FW_LABEL;
		break;
	case XRADIO_FUNC_CALL_INIT_ID_ETF:
	case XRADIO_FUNC_CALL_DEINIT_ID_ETF:
	case XRADIO_FUNC_CALL_HOST_FUNC_HANDLE_ID_ETF:
		buff = kzalloc(2, GFP_KERNEL);
		frame_ctrl |=  XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) | func_id;
		break;
	case XRADIO_FUNC_CALL_INFO_INDICATION_HANDLE_ID_ETF: {
		struct wsm_hdr *wsm = (struct wsm_hdr *)(((struct wsm_buf *)(data))->begin);
		len = wsm->len;
		buff = kzalloc(len + 2, GFP_KERNEL);
		memcpy(buff + 2, wsm, len);
		frame_ctrl |=  XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) | func_id;
		break;
		}
	case XRADIO_FUNC_CALL_VERSION_CHECK_ID_ETF:
		buff = kzalloc(WSM_FW_LABEL + 2, GFP_KERNEL);
		memcpy(buff + 2, data, WSM_FW_LABEL);
		frame_ctrl |=  XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) | func_id;
		len = WSM_FW_LABEL;
		break;
	default:
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "error func id %d\n", func_id);
		xradio_up(&rpmsg_lock);
		return -1;
	}

	len += 2;
	memcpy(buff, &frame_ctrl, 2);
	ret = rpmsg_send(xradio_rpmsg_dev->ept, buff, len);
	if (ret) {
		xradio_up(&rpmsg_lock);
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "rpmsg send to slave error %d\n", ret);
	}
	xradio_rpmsg_printk(XRADIO_DBG_MSG, "rpmsg_send data lenth is %d id 0x%04x\n", len, frame_ctrl);
	down(&rpmsg_lock);
	xradio_up(&rpmsg_lock);
	kfree(buff);
	return ret;
}

static int rpmsg_xradio_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	unsigned short id;

	if (len != 2) {
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "receive error msg\n");
		return -1;
	}

	id = *((unsigned short *)(data));

	if (id & XRADIO_FUNC_CALL_SLAVE_TO_HOST_ID_MASK) {
		xradio_up(&rpmsg_lock);
		xradio_rpmsg_printk(XRADIO_DBG_MSG, "receive slave msg_id 0x%04x\n", id);
		return 0;
	} else {
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "receive error msg_id 0x%04x\n", id);
		return -1;
	}

}


static int rpmsg_xradio_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_device *prpdev;

	prpdev = devm_kzalloc(&rpdev->dev, sizeof(struct rpmsg_device), GFP_KERNEL);
	if (prpdev == NULL) {
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "%s devm_kzalloc fail\n", __func__);
		return -1;
	}

	prpdev->dev = rpdev->dev;
	prpdev->ept = rpdev->ept;

	xradio_rpmsg_printk(XRADIO_DBG_ALWY, "rpmsg xradio advice has added\n");
	xradio_rpmsg_dev = prpdev;

	return 0;
}


static void rpmsg_xradio_remove(struct rpmsg_device *rpdev)
{
	xradio_rpmsg_printk(XRADIO_DBG_ALWY, "rpmsg xradio client driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_xradio_id_table[] = {
	{ .name	= XRADIO_RPMSG_NAME },
	{ },
};

static struct rpmsg_driver rpmsg_xradio_client = {
	.drv.name	= XRADIO_RPMSG_NAME,
	.id_table	= rpmsg_driver_xradio_id_table,
	.probe		= rpmsg_xradio_probe,
	.callback	= rpmsg_xradio_cb,
	.remove		= rpmsg_xradio_remove,
};

int xradio_rpmsg_init(void)
{
	int ret = 0;
	short id;

	ret = register_rpmsg_driver(&rpmsg_xradio_client);
	if (ret) {
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "register_rpmsg_driver fail %d\n", ret);
		return ret;
	}
	if (!xradio_rpmsg_dev) {
		xradio_rpmsg_printk(XRADIO_DBG_ERROR, "xradio_rpmsg_init no pair rpmsg endpoint\n");
		return -EINVAL;
	}
	sema_init(&rpmsg_lock, 1);

	id = XRADIO_FUNC_CALL_HOST_TO_SLAVE(1) |  XRADIO_FUNC_CALL_SLAVE_TO_HOST_CONNECTED_ID;
	rpmsg_send(xradio_rpmsg_dev->ept, &id, 2);
	xradio_rpmsg_printk(XRADIO_DBG_ALWY, "xradio rpmsg creat succeed\n");

	return 0;
}

void xradio_rpmsg_deinit(void)
{
	unregister_rpmsg_driver(&rpmsg_xradio_client);
}

#endif
