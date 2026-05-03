/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * MIPI DSI Bus
 *
 * Copyright (C) 2012-2013, Samsung Electronics, Co., Ltd.
 * Andrzej Hajda <a.hajda@samsung.com>
 */

#ifndef __DRM_MIPI_DSI_H__
#define __DRM_MIPI_DSI_H__

#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

#define	MIPI_DSI_COMPRESSION_MODE 0x07
#define MIPI_DSI_PICTURE_PARAMETER_SET	0x0a
#define MIPI_DSI_COMPRESSED_PIXEL_STREAM 0x0b

/* request ACK from peripheral */
#define MIPI_DSI_MSG_REQ_ACK	BIT(0)
/* use Low Power Mode to transmit message */
#define MIPI_DSI_MSG_USE_LPM	BIT(1)

/* DSI mode flags */

/* video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/* video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/* video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/* enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/* disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_HFP		BIT(5)
/* disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_HBP		BIT(6)
/* disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_HSA		BIT(7)
/* flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/* disable EoT packets in HS mode */
#define MIPI_DSI_MODE_NO_EOT_PACKET	BIT(9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/* transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

#define DSI_DEV_NAME_SIZE		20

#define MIPI_DSI_EN_3DFIFO		BIT(21)

#define MIPI_DSI_SLAVE_MODE		BIT(22)


#define MIPI_DSI_DCS_POWER_MODE_DISPLAY BIT(2)
#define MIPI_DSI_DCS_POWER_MODE_NORMAL  BIT(3)
#define MIPI_DSI_DCS_POWER_MODE_SLEEP   BIT(4)
#define MIPI_DSI_DCS_POWER_MODE_PARTIAL BIT(5)
#define MIPI_DSI_DCS_POWER_MODE_IDLE    BIT(6)

#endif /* __DRM_MIPI_DSI__ */
