/*
 * Copyright 2015 Amazon.com, Inc. or its affiliates.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ENA_H
#define ENA_H

#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>

#include "ena_com.h"
#include "ena_eth_com.h"

/* 1 for AENQ + ADMIN */
#define ENA_MAX_MSIX_VEC(io_queues)	(1 + (io_queues))

#define ENA_REG_BAR			0
#define ENA_MEM_BAR			2

#define ENA_DEFAULT_TX_DESCS	(1024)
#define ENA_DEFAULT_RX_DESCS	(1024)

#if ((ENA_DEFAULT_TX_DESCS / 4) < (MAX_SKB_FRAGS + 2))
#define ENA_TX_WAKEUP_THRESH		(ENA_DEFAULT_TX_SW_DESCS / 4)
#else
#define ENA_TX_WAKEUP_THRESH		(MAX_SKB_FRAGS + 2)
#endif
#define ENA_DEFAULT_SMALL_PACKET_LEN		(128 - NET_IP_ALIGN)

/* minimum the buffer size to 600 to avoid situation the mtu will be changed
 * from too little buffer to very big one and then the number of buffer per
 * packet could reach the maximum ENA_PKT_MAX_BUFS
 */
#define ENA_DEFAULT_MIN_RX_BUFF_ALLOC_SIZE 600

#define ENA_NAME_MAX_LEN	20
#define ENA_IRQNAME_SIZE	40

#define ENA_PKT_MAX_BUFS	19

#define ENA_RX_THASH_TABLE_SIZE	256

/* The number of tx packet completions that will be handled each napi poll
 * cycle is ring_size / ENA_TX_POLL_BUDGET_DEVIDER.
 */
#define ENA_TX_POLL_BUDGET_DEVIDER	4

/* Refill Rx queue when number of available descriptors is below
 * QUEUE_SIZE / ENA_RX_REFILL_THRESH_DEVIDER
 */
#define ENA_RX_REFILL_THRESH_DEVIDER	8

#define ENA_TX_RING_IDX_NEXT(idx, ring_size) (((idx) + 1) & ((ring_size) - 1))

#define ENA_RX_RING_IDX_NEXT(idx, ring_size) (((idx) + 1) & ((ring_size) - 1))
#define ENA_RX_RING_IDX_ADD(idx, n, ring_size) \
	(((idx) + (n)) & ((ring_size) - 1))

#define ENA_IO_TXQ_IDX(q)	(2 * (q))
#define ENA_IO_RXQ_IDX(q)	(2 * (q) + 1)

#define ENA_MGMNT_IRQ_IDX		0
#define ENA_IO_IRQ_FIRST_IDX		1
#define ENA_IO_IRQ_IDX(q)		(ENA_IO_IRQ_FIRST_IDX + (q))

/* ENA device should send keep alive msg every 1 sec.
 * We wait for 3 sec just to be on the safe side.
 */
#define ENA_DEVICE_KALIVE_TIMEOUT	(3 * HZ)

#define ENA_RX_RSS_TABLE_SIZE	ENA_RX_THASH_TABLE_SIZE

struct ena_irq {
	irq_handler_t handler;
	void *data;
	u32 vector;
	cpumask_t affinity_hint_mask;
	char name[ENA_IRQNAME_SIZE];
};

struct ena_napi {
	struct napi_struct napi ____cacheline_aligned;
	struct ena_ring *tx_ring;
	struct ena_ring *rx_ring;
#ifndef HAVE_NETDEV_NAPI_LIST
	struct net_device poll_dev;
#endif /* HAVE_NETDEV_NAPI_LIST */
	u32 qid;
};

struct ena_tx_buffer {
	struct sk_buff *skb;
	/* num of ena desc for this specific skb
	 * (includes data desc and metadata desc)
	 */
	u32 tx_descs;
	/* num of buffers used by this skb */
	u32 num_of_bufs;
	struct ena_com_buf bufs[ENA_PKT_MAX_BUFS];
} ____cacheline_aligned;

struct ena_rx_buffer {
	struct sk_buff *skb;
	struct page *page;
	u8 *data;
	u32 data_size;
	u32 frag_size; /* used in rx skb allocation */
	u32 page_offset;
	struct ena_com_buf ena_buf;
} ____cacheline_aligned;

struct ena_ring {
	/* Holds the empty requests for TX out of order completions */
	u16 *free_tx_ids;
	union {
		struct ena_tx_buffer *tx_buffer_info; /* contex of tx packet */
		struct ena_rx_buffer *rx_buffer_info; /* contex of rx packet */
	};

	/* cache ptr to avoid using the adapter */
	struct device *dev;
	struct pci_dev *pdev;
	struct napi_struct *napi;
	struct net_device *netdev;
	struct ena_com_io_cq *ena_com_io_cq;
	struct ena_com_io_sq *ena_com_io_sq;

	u16 next_to_use;
	u16 next_to_clean;
	u16 rx_small_copy_len;
	u16 qid;
	u16 mtu;

	int ring_size; /* number of tx/rx_buffer_info's entries */

	/* Count how many times the driver wasn't able to allocate new pages */
	u32 alloc_fail_cnt;
	u32 bad_checksum;

	enum ena_com_memory_queue_type tx_mem_queue_type;

	struct ena_com_buf ena_bufs[ENA_PKT_MAX_BUFS];
} ____cacheline_aligned;

/* adapter specific private data structure */
struct ena_adapter {
	struct ena_com_dev *ena_dev;
	/* OS defined structs */
	struct net_device *netdev;
	struct pci_dev *pdev;

	u32 msix_enabled;

	/* rx packets that shorter that this len will be copied to the skb
	 * header
	 */
	u32 small_copy_len;
	u32 max_mtu;

	int num_queues;

	struct msix_entry *msix_entries;
	int msix_vecs;

	u32 tx_usecs, rx_usecs; /* interrupt moderation */
	u32 tx_frames, rx_frames; /* interrupt moderation */

	u32 tx_ring_size;
	u32 rx_ring_size;

	/* RSS*/
	u8 rss_ind_tbl[ENA_RX_RSS_TABLE_SIZE];

	u32 msg_enable;

	u8 mac_addr[ETH_ALEN];

	char name[ENA_NAME_MAX_LEN];
	bool link_status;

	bool up;

	/* TX */
	struct ena_ring tx_ring[ENA_MAX_NUM_IO_QUEUES]
		____cacheline_aligned_in_smp;

	/* RX */
	struct ena_ring rx_ring[ENA_MAX_NUM_IO_QUEUES]
		____cacheline_aligned_in_smp;

	struct ena_napi ena_napi[ENA_MAX_NUM_IO_QUEUES];

	struct ena_irq irq_tbl[ENA_MAX_MSIX_VEC(ENA_MAX_NUM_IO_QUEUES)];

	/* watchdog timer */
	struct work_struct reset_task;
	struct work_struct suspend_io_task;
	struct work_struct resume_io_task;
	struct timer_list watchdog_timer;
};

#endif /* !(ENA_H) */
