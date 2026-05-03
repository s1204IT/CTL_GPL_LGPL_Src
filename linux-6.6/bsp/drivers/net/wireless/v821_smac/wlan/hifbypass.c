#include "hifbypass.h"
#include "asm/ptrace.h"


struct hifbypass_priv self_hifbypass;
struct device_node *pdev_node;
u32 hifbypass_irq_num;

#ifdef USE_ADDRESS_REMAP_IN_LINUX
volatile void *HIF_WLAN_STATE_VIR;
volatile void *HIF_OV_CTRL_VIR;
volatile void *HIF_OV_MISC_REG_VIR;
volatile void *WLAN_APB_PWR_CTRL_BASE_VIR;
volatile void *CCM_MOD_RST_CTRL_VIR;
volatile void *PAS_SHARE_RAM_ADDR_BASE;
volatile void *AHB_SHARE_RAM_ADDR_BASE;
volatile void *CCU_AON_DCXO_CFG_VIR;

int reg_addr_remap(void)
{
	HIF_WLAN_STATE_VIR = (void *)ioremap(HIF_WLAN_STATE_REL, 4);
	HIF_OV_CTRL_VIR = (void *)ioremap(HIF_OV_CTRL_REL, 4);
	HIF_OV_MISC_REG_VIR = (void *)ioremap(HIF_OV_MISC_REG_REL, 0xB0);
	WLAN_APB_PWR_CTRL_BASE_VIR = (void *)ioremap(WLAN_APB_PWR_CTRL_BASE_REL, 0x100);
	CCM_MOD_RST_CTRL_VIR = (void *)ioremap(CCM_MOD_RST_CTRL_REL, 4);
	PAS_SHARE_RAM_ADDR_BASE = (void *)ioremap(SILICON_PAC_BASE_ADDRESS, V821_PAS_RAM_SIZE);
	AHB_SHARE_RAM_ADDR_BASE = (void *)ioremap(SILICON_AHB_MEMORY_ADDRESS, V821_AHB_RAM_SIZE);
	CCU_AON_DCXO_CFG_VIR = (void *)ioremap(CCU_AON_DCXO_CFG_REL, 4);

	if (!HIF_WLAN_STATE_VIR | !HIF_OV_CTRL_VIR | !HIF_OV_MISC_REG_VIR |
		!WLAN_APB_PWR_CTRL_BASE_VIR | !CCM_MOD_RST_CTRL_VIR | !AHB_SHARE_RAM_ADDR_BASE |
		!PAS_SHARE_RAM_ADDR_BASE | !CCU_AON_DCXO_CFG_VIR) {
		hifbypass_printk(XRADIO_DBG_ERROR, " ioremap fail\n");
		return -1;
	}

	return 0;
}

void reg_addr_unmap(void)
{
	iounmap(HIF_WLAN_STATE_VIR);
	iounmap(HIF_OV_CTRL_VIR);
	iounmap(HIF_OV_MISC_REG_VIR);
	iounmap(WLAN_APB_PWR_CTRL_BASE_VIR);
	iounmap(CCM_MOD_RST_CTRL_VIR);
	iounmap(PAS_SHARE_RAM_ADDR_BASE);
	iounmap(AHB_SHARE_RAM_ADDR_BASE);
}
#endif


int hifbypass_irq_unsubscribe(struct hifbypass_priv *self)
{
	int ret = 0;
	unsigned long flags;

	hifbypass_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	if (!self->irq_handler) {
		hifbypass_printk(XRADIO_DBG_ERROR, "%s:irq_handler is NULL!\n", __func__);
		return 0;
	}

	devm_free_irq(self->pdev, hifbypass_irq_num, self);

	spin_lock_irqsave(&self->lock, flags);
	self->irq_priv = NULL;
	self->irq_handler = NULL;
	spin_unlock_irqrestore(&self->lock, flags);

	return ret;
}


static irqreturn_t xradio_bypass_irq_handler(int irq, void *hifbypass_priv)
{
	struct hifbypass_priv *self = (struct hifbypass_priv *)hifbypass_priv;
	unsigned long flags;

	SYS_BUG(!self);
	spin_lock_irqsave(&self->lock, flags);
	if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	spin_unlock_irqrestore(&self->lock, flags);
	return IRQ_HANDLED;
}


static int xradio_request_bypass_irq(struct device *dev, void *hifbypass_priv)
{
	int ret = -1;

	hifbypass_irq_num = of_irq_get(self_hifbypass.pdev_node, 0);
	if (!hifbypass_irq_num) {
		hifbypass_printk(XRADIO_DBG_ERROR, "get irq num failed!\n");
		return -1;
	}

	ret = devm_request_irq(dev, hifbypass_irq_num,
					(irq_handler_t)xradio_bypass_irq_handler,
					V821_WLAN_IRQ_FLAG, "xradio_irq", hifbypass_priv);

	return ret;
}

int hifbypass_irq_subscribe(struct hifbypass_priv *self,
				     hifbypass_irq_handler handler,
				     void *priv)
{
	int ret = 0;
	unsigned long flags;


	if (!handler)
		return -EINVAL;
	hifbypass_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	spin_lock_irqsave(&self->lock, flags);
	self->irq_priv = priv;
	self->irq_handler = handler;
	spin_unlock_irqrestore(&self->lock, flags);

	ret = xradio_request_bypass_irq(self->pdev, self);
	if (!ret)
		hifbypass_printk(XRADIO_DBG_NIY, "%s:xradio_request_bypass_irq success.\n",
					__func__);
	else
		hifbypass_printk(XRADIO_DBG_ERROR, "%s:xradio_request_bypass_irq failed(%d).\n",
				__func__, ret);

	return ret;
}


static int hifbypass_probe(struct platform_device *pdev)
{
	hifbypass_printk(XRADIO_DBG_ALWY, "hifbypass device has found\n");

	self_hifbypass.pdev = &(pdev->dev);
	self_hifbypass.load_state = HIFBYPASS_LOAD;
	self_hifbypass.pdev_node = of_find_compatible_node(NULL, NULL, "allwinner,sun300wi-sip-wifi");

	xradio_hif_bypass_enable();
	xradio_wlan_power(1);
	return 0;
}

static int hifbypass_remove(struct platform_device *pdev)
{
	hifbypass_printk(XRADIO_DBG_TRC, "hifbypass device has remove");
	self_hifbypass.load_state = HIFBYPASS_UNLOAD;
	return 0;
}

static const struct of_device_id hifbypass_ids[] = {
	{
		.compatible = "allwinner,sun300wi-sip-wifi",
	},
	{},
};

//MODULE_DEVICE_TABLE(of, hifbypass_ids);

static struct platform_driver hifbypass_driver = {
	.driver = {
		.name = "hifbypass",
		.of_match_table = hifbypass_ids,
	},
	.probe = hifbypass_probe,
	.remove = hifbypass_remove,
};


struct device *hifbypass_driver_init(struct hifbypass_priv **hw_priv)
{
	int ret;
	struct device *pdev;

	if (self_hifbypass.load_state == HIFBYPASS_LOAD) {
		hifbypass_printk(XRADIO_DBG_ERROR, "hifbypass driver had register");
		return NULL;
	}
	hifbypass_printk(XRADIO_DBG_MSG, "platform_driver_register start\n");

	ret = platform_driver_register(&hifbypass_driver);
	if (ret) {
		hifbypass_printk(XRADIO_DBG_ERROR, "hifbypass driver register error %d", ret);
		return NULL;
	}
	hifbypass_printk(XRADIO_DBG_ALWY, "platform_driver_register succeed\n");

	*hw_priv = &self_hifbypass;
	pdev = self_hifbypass.pdev;
	return pdev;
}

void hifbypass_driver_deinit(void)
{
	hifbypass_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	if (self_hifbypass.load_state != HIFBYPASS_UNLOAD) {
		platform_driver_unregister(&hifbypass_driver);
		xradio_hif_bypass_disable();
		xradio_wlan_power(0);	/*power down.*/
		self_hifbypass.load_state = HIFBYPASS_UNLOAD;
	} else {
		hifbypass_printk(XRADIO_DBG_ERROR, "%s hifbypass did not init!\n", __func__);
	}
}


