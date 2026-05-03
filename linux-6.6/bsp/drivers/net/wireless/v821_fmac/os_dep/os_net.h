#ifndef __OS_NET_H__
#define __OS_NET_H__

#include "xradio_platform.h"

int xradio_add_net_dev(struct xradio_plat *priv, u8 if_type, u8 if_id, char *name);

void xradio_remove_net_dev(struct xradio_plat *priv);

void xradio_net_tx_pause(struct xradio_plat *priv);

void xradio_net_tx_resume(struct xradio_plat *priv);

int xradio_net_data_input(struct xradio_plat *priv, struct sk_buff *skb);

#ifdef STA_AP_COEXIST
int xradio_net_data_input_ap(struct xradio_plat *priv, struct sk_buff *skb);
#endif

int xradio_net_set_mac(struct xradio_plat *priv, u8 *mac);
#endif
