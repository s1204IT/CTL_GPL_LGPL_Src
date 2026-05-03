/*
 * v821_wlan/debug.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * Debug info implementation for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/debugfs.h>

#include "xr_version.h"
#include "os_dep/os_intf.h"
#include "xradio.h"
#include "debug.h"

#define XRADIO_DBG_DEFAULT (XRADIO_DBG_ALWY | XRADIO_DBG_ERROR | XRADIO_DBG_WARN)
//#define XRADIO_DBG_DEFAULT (XRADIO_DBG_ALWY | XRADIO_DBG_ERROR | XRADIO_DBG_WARN | XRADIO_DBG_NIY | XRADIO_DBG_MSG | XRADIO_DBG_TRC)

u8 dbg_common = XRADIO_DBG_DEFAULT;
u8 dbg_cfg = XRADIO_DBG_DEFAULT;
u8 dbg_iface = XRADIO_DBG_DEFAULT;
u8 dbg_plat = XRADIO_DBG_DEFAULT;
u8 dbg_queue = XRADIO_DBG_DEFAULT;
u8 dbg_io = XRADIO_DBG_DEFAULT;//XRADIO_DBG_DEFAULT;
u8 dbg_txrx = XRADIO_DBG_DEFAULT;
u8 dbg_cmd = XRADIO_DBG_DEFAULT;

#define xradio_dbg xradio_printk

/*for ip data*/
#define PF_TCP 0x0010
#define PF_UDP 0x0020
#define PF_DHCP 0x0040
#define PF_ICMP 0x0080

#define PF_IPADDR 0x2000 /*ip address of ip packets.*/
#define PF_UNKNWN 0x4000 /*print frames of unknown flag.*/
#define PF_RX 0x8000	 /*0:TX, 1:RX. So, need to add PF_RX in Rx path.*/

#ifndef IPPROTO_IP
#define IPPROTO_IP 0
#endif

#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP 1
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

#ifndef IPPROTO_UDP
#define IPPROTO_UDP 17
#endif

#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806
#define ETH_P_IPV6 0x86DD

#define PT_MSG_PUT(f, ...)                                  \
	do {                                                    \
		if (flags & (f))                                    \
			proto_msg += sprintf(proto_msg, __VA_ARGS__);   \
	} while (0)

#define IS_PROTO_PRINT (proto_msg != (char *)&protobuf[0])

#define LLC_LEN 14
#define LLC_TYPE_OFF 12 /*Ether type offset*/
#define IP_PROTO_OFF 9	/*protocol offset*/
#define IP_S_ADD_OFF 12
#define IP_D_ADD_OFF 16
#define UDP_LEN 8
/*DHCP*/
#define DHCP_BOOTP_C 68
#define DHCP_BOOTP_S 67
#define UDP_BOOTP_LEN 236 /*exclude "Options:64"*/
#define BOOTP_OPS_LEN 64
#define DHCP_MAGIC 0x63825363
#define DHCP_DISCOVER 0x01
#define DHCP_OFFER 0x02
#define DHCP_REQUEST 0x03
#define DHCP_DECLINE 0x04
#define DHCP_ACK 0x05
#define DHCP_NACK 0x06
#define DHCP_RELEASE 0x07
/*ARP*/
#define ARP_REQUEST 0x0001
#define ARP_RESPONSE 0x0002
#define ARP_TYPE_OFFSET 6

/*IP/IPV6/ARP layer...*/
static inline bool is_ip(u8 *eth_data)
{
	/* 0x0800 */
	return (bool)(*(u16 *)(eth_data + LLC_TYPE_OFF) == cpu_to_be16(ETH_P_IP));
}

static inline bool is_ipv6(u8 *eth_data)
{
	/* 0x08dd */
	return (bool)(*(u16 *)(eth_data + LLC_TYPE_OFF) == cpu_to_be16(ETH_P_IPV6));
}

static inline bool is_arp(u8 *eth_data)
{
	/* 0x0806 */
	return (bool)(*(u16 *)(eth_data + LLC_TYPE_OFF) == cpu_to_be16(ETH_P_ARP));
}

static inline bool is_tcp(u8 *eth_data)
{
	return (bool)(eth_data[LLC_LEN + IP_PROTO_OFF] == IPPROTO_TCP);
}

static inline bool is_udp(u8 *eth_data)
{
	return (bool)(eth_data[LLC_LEN + IP_PROTO_OFF] == IPPROTO_UDP);
}

static inline bool is_icmp(u8 *eth_data)
{
	return (bool)(eth_data[LLC_LEN + IP_PROTO_OFF] == IPPROTO_ICMP);
}

static inline bool is_dhcp(u8 *eth_data)
{
	u8 *ip_hdr = eth_data + LLC_LEN;

	if (!is_ip(eth_data))
		return (bool)0;
	if (ip_hdr[IP_PROTO_OFF] == IPPROTO_UDP) {
		u8 *udp_hdr = ip_hdr + ((ip_hdr[0] & 0xf) << 2); /*ihl:words*/
		/* DHCP client or DHCP server*/
		return (bool)((((udp_hdr[0] << 8) | udp_hdr[1]) == DHCP_BOOTP_C) ||
						(((udp_hdr[0] << 8) | udp_hdr[1]) == DHCP_BOOTP_S));
	}
	return (bool)0;
}

void xradio_parse_frame(u8 *eth_data, u8 tx, u32 flags, u16 len)
{
	char protobuf[512] = {0};
	char *proto_msg = &protobuf[0];

	if (!is_ip(eth_data))
		xradio_dbg(XRADIO_DBG_ALWY, "wlan0 %s: not ip data\n", tx ? "TX" : "RX");

	if (is_ip(eth_data)) {
		u8 *ip_hdr = eth_data + LLC_LEN;
		u8 *ipaddr_s = ip_hdr + IP_S_ADD_OFF;
		u8 *ipaddr_d = ip_hdr + IP_D_ADD_OFF;
		u8 *proto_hdr = ip_hdr + ((ip_hdr[0] & 0xf) << 2); /*ihl:words*/

		if (is_tcp(eth_data)) {
			PT_MSG_PUT(PF_TCP, "TCP%s%s, src=%d, dest=%d, seq=0x%08x, ack=0x%08x",
						(proto_hdr[13] & 0x01) ? "(F)" : "",
						(proto_hdr[13] & 0x02) ? "(S)" : "",
						(proto_hdr[0] << 8) | proto_hdr[1],
						(proto_hdr[2] << 8) | proto_hdr[3],
						(proto_hdr[4] << 24) | (proto_hdr[5] << 16) |
						(proto_hdr[6] << 8) | proto_hdr[7],
						(proto_hdr[8] << 24) | (proto_hdr[9] << 16) |
						(proto_hdr[10] << 8) | proto_hdr[11]);

		} else if (is_udp(eth_data)) {
			if (is_dhcp(eth_data)) {
				u8 Options_len = BOOTP_OPS_LEN;

				u32 dhcp_magic = cpu_to_be32(DHCP_MAGIC);
				u8 *dhcphdr = proto_hdr + UDP_LEN + UDP_BOOTP_LEN;

				while (Options_len) {
					if (*(u32 *)dhcphdr == dhcp_magic)
						break;
					dhcphdr++;
					Options_len--;
				}
				PT_MSG_PUT(PF_DHCP, "DHCP, Opt=%d, MsgType=%d", *(dhcphdr + 4), *(dhcphdr + 6));
			} else {
				PT_MSG_PUT(PF_UDP, "UDP, source=%d, dest=%d", (proto_hdr[0] << 8) | proto_hdr[1],
						(proto_hdr[2] << 8) | proto_hdr[3]);
			}
		} else if (is_icmp(eth_data)) {
			PT_MSG_PUT(PF_ICMP, "ICMP%s%s, Seq=%d", (proto_hdr[0] == 8) ? "(ping)" : "",
					(proto_hdr[0] == 0) ? "(reply)" : "", (proto_hdr[6] << 8) | proto_hdr[7]);
		} else {
			PT_MSG_PUT(PF_UNKNWN, "unknown IP type=%d", *(ip_hdr + IP_PROTO_OFF));
		}
		if (IS_PROTO_PRINT) {
			PT_MSG_PUT(PF_IPADDR, "-%d.%d.%d.%d(s)", ipaddr_s[0], ipaddr_s[1], ipaddr_s[2], ipaddr_s[3]);
			PT_MSG_PUT(PF_IPADDR, "-%d.%d.%d.%d(d)", ipaddr_d[0], ipaddr_d[1], ipaddr_d[2], ipaddr_d[3]);
			xradio_dbg(XRADIO_DBG_ALWY, "wlan0 %s: %s len:%d\n", tx ? "TX" : "RX", protobuf, len);
		}
	}
}

static int xradio_version_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "Driver Label:%s\n", XRADIO_VERSION);
	return 0;
}

static int xradio_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, &xradio_version_show, inode->i_private);
}

static int xradio_generic_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations fops_version = {
	.open = xradio_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.owner = THIS_MODULE,
};

extern u16 txparse_flags;
extern u16 rxparse_flags;

static ssize_t xradio_parse_flags_set(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[30] = { 0 };
	char *start = &buf[0];
	char *endptr = NULL;

	count = (count > 29 ? 29 : count);
	if (!count)
		return -EINVAL;
	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	txparse_flags = simple_strtoul(buf, &endptr, 16);
	start = endptr + 1;
	if (start < buf + count)
		rxparse_flags = simple_strtoul(start, &endptr, 16);

	txparse_flags &= 0x7fff;
	rxparse_flags &= 0x7fff;

	xradio_dbg(XRADIO_DBG_ALWY, "txparse=0x%04x, rxparse=0x%04x\n", txparse_flags, rxparse_flags);
	return count;
}

static ssize_t xradio_parse_flags_get(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[100];
	size_t size = 0;

	sprintf(buf, "txparse=0x%04x, rxparse=0x%04x\n", txparse_flags, rxparse_flags);
	size = strlen(buf);

	return simple_read_from_buffer(user_buf, count, ppos, buf, size);
}

static const struct file_operations fops_parse_flags = {
	.open = xradio_generic_open,
	.write = xradio_parse_flags_set,
	.read = xradio_parse_flags_get,
	.llseek = default_llseek,
};

static ssize_t xradio_dbg_level_set(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[30] = { 0 };
	char *start = &buf[0];
	char *endptr = NULL;

	count = (count > 29 ? 29 : count);
	if (!count)
		return -EINVAL;
	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	dbg_txrx = simple_strtoul(buf, &endptr, 16);
	start = endptr + 1;
	if (start < buf + count)
		dbg_cmd = simple_strtoul(start, &endptr, 16);

	xradio_dbg(XRADIO_DBG_ALWY, "dbg_txrx=0x%02x, dbg_cmd=0x%02x\n", dbg_txrx, dbg_cmd);
	return count;
}

static ssize_t xradio_dbg_level_get(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[100];
	size_t size = 0;

	sprintf(buf, "dbg_txrx=0x%02x, dbg_cmd=0x%02x\n", dbg_txrx, dbg_cmd);
	size = strlen(buf);

	return simple_read_from_buffer(user_buf, count, ppos, buf, size);
}

static const struct file_operations fops_dbg_level = {
	.open = xradio_generic_open,
	.write = xradio_dbg_level_set,
	.read = xradio_dbg_level_get,
	.llseek = default_llseek,
};

#ifdef XRADIO_RPBUF_PERF_TEST
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include "uapi/linux/sched/types.h"
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include "linux/sched/types.h"
#else
#include "linux/sched/rt.h"
#endif

#define RP_TEST_DATA_LEN          1500
#define RP_TEST_DATA_DBG          0

#define PERF_TIME_PER_SEC         1000 // in 1ms
#define PERF_TIME()               ((u32)jiffies)

static u32 rp_test_cnt, rp_test_mode, rp_rx_cnt;
static struct task_struct *rp_test_tx_thread;
static u32 data_total_cnt;
static u32 last_time;

int xradio_tx_data_process(struct xradio_plat *priv, struct sk_buff *skb, u8 if_type);

static __inline void rpbuf_speed_cal(u32 len)
{
	u32 speed;
	u32 integer_part;
	u32 time;

	data_total_cnt += len;
	time = PERF_TIME() - last_time;

	if (time >= 1000) {
		last_time = PERF_TIME();
		/* Mbits/sec */
		speed = data_total_cnt * 8 / 1000 / time;
		data_total_cnt = 0;

		integer_part = speed;
		xradio_dbg(XRADIO_DBG_ALWY, "[perf] %u %s\n", integer_part, "Mb/s");
	}

}

int xradio_rp_test_rx_process(struct sk_buff *skb)
{
	if ((rp_test_mode == 1) && skb) {
		rp_rx_cnt++;
		rpbuf_speed_cal(skb->len);
		kfree_skb(skb);
		return 0;
	}

	return -1;
}

static int rp_test_tx_task(void *data)
{
	struct xradio_hw *xr_hw = data;
	u32 tx_seq = 0, cnt = rp_test_cnt;
	struct sk_buff *skb = NULL;
	int ret, put;
	int prio = 0;

	if (prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	sched_set_fifo_low(current);
#else
	struct sched_param param;
	param.sched_priority = (prio < MAX_RT_PRIO) ? prio : (MAX_RT_PRIO - 1);
	sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}
	xradio_dbg(XRADIO_DBG_ALWY, "rp_test cnt=%d\n", cnt);

	while (rp_test_cnt) {
		u32 *seq;

		if (!skb) {
			put = 0;
			skb = __dev_alloc_skb(RP_TEST_DATA_LEN, GFP_KERNEL);
		}
		if (skb) {
			if (!put) {
				put = 1;
				skb_put(skb, RP_TEST_DATA_LEN);
				seq = (void *)skb->data;
				*seq = tx_seq++;
#if RP_TEST_DATA_DBG
				{
					u8 *dbg_data = ((u8 *)skb->data) + 4;
					int i;

					for (i = 0; i < (RP_TEST_DATA_LEN - 4); i++)
						dbg_data[i] = i & 0xFF;
				}
#endif
			}
			ret = xradio_tx_data_process(xr_hw->plat, skb, 0);
			if (ret) {
				while (xradio_k_atomic_read(&xr_hw->tx_data_pause_master)) {
					msleep(1);
				}
			}
			skb = NULL;
			cnt--;
			if (cnt == 0)
				break;
		}
	}
	if (skb)
		kfree_skb(skb);

	xradio_dbg(XRADIO_DBG_ALWY, "rp_test_tx_task end g:%d f:%d\n", rp_test_cnt, cnt);

	return 0;
}

// mode: 0: TX test, 1:RX test, 2: show RX cnt
#define RP_TEST_MODE_TX                0
#define RP_TEST_MODE_RX                1
#define RP_TEST_MODE_GET_RX_CNT        2
static ssize_t xradio_rptest_set(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[50] = { 0 };
	char *endptr = NULL;
	u32 mode, test = 0;
	char *start = &buf[0];

	count = (count > 50 ? 50 : count);
	if (!count)
		return -EINVAL;
	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	mode = simple_strtoul(buf, &endptr, 10);
	start = endptr + 1;
	if (start < buf + count)
		test = simple_strtoul(start, &endptr, 10);

	if (mode == RP_TEST_MODE_GET_RX_CNT) {
		if (test) {
			xradio_dbg(XRADIO_DBG_ALWY, "rx_cnt: %d, clean cnt\n", rp_rx_cnt);
			rp_rx_cnt = 0;
		} else {
			xradio_dbg(XRADIO_DBG_ALWY, "rx_cnt: %d, no clean cnt\n", rp_rx_cnt);
		}
		goto end;
	}

	xradio_dbg(XRADIO_DBG_ALWY, "mode:%d test cnt=%d\n", mode, test);
	rp_test_mode = mode;

	if (mode == RP_TEST_MODE_TX) {
		if (test) {
			rp_test_tx_thread = kthread_run(rp_test_tx_task, (void *)&xradio_get_platform()->hw_priv,
							"rp_test_tx_thread");
			if (IS_ERR(rp_test_tx_thread)) {
				rp_test_tx_thread  = NULL;
				xradio_dbg(XRADIO_DBG_ALWY, "tx_thread run fail\n");
				return -1;
			}
			rp_test_cnt = test;
			xradio_dbg(XRADIO_DBG_ALWY, "tx rp_test cnt=%d\n", rp_test_cnt);
		} else if (rp_test_cnt) {
			rp_test_cnt = test;
		}
	}

end:
	return count;
}

static ssize_t xradio_rptest_get(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[100];
	size_t size = 0;

	sprintf(buf, "rp_test_cnt:%d\n", rp_test_cnt);
	size = strlen(buf);

	return simple_read_from_buffer(user_buf, count, ppos, buf, size);
}

static const struct file_operations fops_rp_test = {
	.open = xradio_generic_open,
	.write = xradio_rptest_set,
	.read = xradio_rptest_get,
	.llseek = default_llseek,
};
#endif

int xradio_debug_init_common(struct xradio_hw *xradio_hw)
{
	int ret = -ENOMEM;
	struct xradio_debug_common *d = NULL;

#define ERR_LINE  do { goto err; } while (0)

	d = xradio_k_zmalloc(sizeof(struct xradio_debug_common));
	if (!d) {
		xradio_dbg(XRADIO_DBG_ERROR, "malloc failed.\n");
		return ret;
	}
	xradio_hw->debug = d;

	d->debugfs_phy = debugfs_create_dir("xradio", NULL);
	if (!d->debugfs_phy)
		ERR_LINE;

	if (!debugfs_create_file("version", S_IRUSR, d->debugfs_phy, xradio_hw, &fops_version))
		ERR_LINE;

	if (!debugfs_create_file("parse_flags", S_IRUSR | S_IWUSR, d->debugfs_phy, xradio_hw, &fops_parse_flags))
		ERR_LINE;

	if (!debugfs_create_file("dbg_level", S_IRUSR | S_IWUSR, d->debugfs_phy, xradio_hw, &fops_dbg_level))
		ERR_LINE;
#ifdef XRADIO_RPBUF_PERF_TEST
	if (!debugfs_create_file("rp_test", S_IRUSR | S_IWUSR, d->debugfs_phy, xradio_hw, &fops_rp_test))
		ERR_LINE;
#endif

	return 0;
err:
	debugfs_remove_recursive(d->debugfs_phy);
	xradio_k_free(d);
	xradio_hw->debug = NULL;
	return ret;
}

void xradio_debug_deinit_common(struct xradio_hw *xradio_hw)
{
	struct xradio_debug_common *d = xradio_hw->debug;

	if (d) {
		xradio_hw->debug = NULL;
		xradio_k_free(d);
	}
}
