#ifndef _HIF_BY_PASS_H_
#define _HIF_BY_PASS_H_

#include <generated/uapi/linux/version.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <net/mac80211.h>
#include <linux/wait.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>


#include "hwio.h"
#include "debug.h"

#define PATCH_NOT_DISABLE_WLAN_IRQ_IN_HANDLER	1

typedef void (*hifbypass_irq_handler)(void *priv);
struct hifbypass_priv {
	struct device        *pdev;
	struct device_node   *pdev_node;
	spinlock_t            lock;
	hifbypass_irq_handler      irq_handler;
	void                 *irq_priv;
	wait_queue_head_t     init_wq;
	int                   load_state;
};


struct device *hifbypass_driver_init(struct hifbypass_priv **hw_priv);
int hifbypass_irq_subscribe(struct hifbypass_priv *self,
				     hifbypass_irq_handler handler,
				     void *priv);
int hifbypass_irq_unsubscribe(struct hifbypass_priv *self);
void hifbypass_driver_deinit(void);


#define V821_WLAN_IRQ_FLAG  IRQF_SHARED


#define HIFBYPASS_UNLOAD   0
#define HIFBYPASS_LOAD     1

#endif
