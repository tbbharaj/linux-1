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

#ifndef ENA_COM
#define ENA_COM

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/gfp.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include "ena_includes.h"

#define ena_trc_dbg(format, arg...) \
	pr_debug("[ENA_COM: %s] " format, __func__, ##arg)
#define ena_trc_info(format, arg...) \
	pr_info("[ENA_COM: %s] " format, __func__, ##arg)
#define ena_trc_warn(format, arg...) \
	pr_warn("[ENA_COM: %s] " format, __func__, ##arg)
#define ena_trc_err(format, arg...) \
	pr_err("[ENA_COM: %s] " format, __func__, ##arg)

#define ENA_ASSERT(cond, format, arg...)				\
	do {								\
		if (unlikely(!(cond))) {				\
			ena_trc_err(					\
				"Assert failed on %s:%s:%d:" format,	\
				__FILE__, __func__, __LINE__, ##arg);	\
			WARN_ON(cond);					\
		}							\
	} while (0)

#define ENA_MAX_NUM_IO_QUEUES		128U
/* We need to queues for each IO (on for Tx and one for Rx) */
#define ENA_TOTAL_NUM_QUEUES		(2 * (ENA_MAX_NUM_IO_QUEUES))

#define ENA_MAX_HANDLERS 256

#define ENA_MAX_PHYS_ADDR_SIZE_BITS 48

#define ENA_MAC_LEN 6

/* Unit in usec */
#define ENA_REG_READ_TIMEOUT 5000

#define ADMIN_SQ_SIZE(depth)	((depth) * sizeof(struct ena_admin_aq_entry))
#define ADMIN_CQ_SIZE(depth)	((depth) * sizeof(struct ena_admin_acq_entry))
#define ADMIN_AENQ_SIZE(depth)	((depth) * sizeof(struct ena_admin_aenq_entry))

#define IO_TX_SQ_SIZE(depth)	((depth) * sizeof(struct ena_eth_io_tx_desc))
#define IO_TX_CQ_SIZE(depth)	((depth) * sizeof(struct ena_eth_io_tx_cdesc))

#define IO_RX_SQ_SIZE(depth) ((depth) * sizeof(struct ena_eth_io_rx_desc))
#define IO_RX_CQ_SIZE(depth) ((depth) * sizeof(struct ena_eth_io_rx_cdesc_ext))

/*****************************************************************************/
/*****************************************************************************/

enum queue_direction {
	ENA_COM_IO_QUEUE_DIRECTION_TX,
	ENA_COM_IO_QUEUE_DIRECTION_RX
};

enum ena_com_memory_queue_type {
	/* descriptors and headers are located on the host OS memory
	 */
	ENA_MEM_QUEUE_TYPE_HOST_MEMORY = 0x1,
	/* descriptors located on device memory
	 * and headers located on host OS memory
	 */
	ENA_MEM_QUEUE_TYPE_MIXED_MEMORY = 0x2,
	/* descriptors and headers are copied to the device memory */
	ENA_MEM_QUEUE_TYPE_DEVICE_MEMORY = 0x3,
	ENA_MEM_QUEUE_TYPE_MAX_TYPES = ENA_MEM_QUEUE_TYPE_DEVICE_MEMORY
};

struct ena_com_buf {
	dma_addr_t paddr; /**< Buffer physical address */
	u16 len; /**< Buffer length in bytes */
};

struct ena_com_io_desc_addr {
	u8 __iomem *pbuf_dev_addr; /* LLQ address */
	u8 __iomem *virt_addr;
	dma_addr_t phys_addr;
};

struct ena_com_tx_meta {
	u16 mss;
	u16 l3_hdr_len;
	u16 l3_hdr_offset;
	u16 l3_outer_hdr_len; /* In words */
	u16 l3_outer_hdr_offset;
	u16 l4_hdr_len; /* In words */
};

struct ena_com_io_cq {
	struct ena_com_io_desc_addr cdesc_addr;

	u32 __iomem *db_addr;

	/* The offset of the interrupt unmask register */
	u32 __iomem *unmask_reg;

	/* The value to write to the above register to unmask
	 * the interrupt of this queue
	 */
	u32 unmask_val;
	u32 msix_vector;

	enum queue_direction direction;

	/* holds the number of cdesc of the current packet */
	u16 cur_rx_pkt_cdesc_count;
	/* save the firt cdesc idx of the current packet */
	u16 cur_rx_pkt_cdesc_start_idx;

	u16 q_depth;
	u16 qid;

	u16 idx;
	u16 head;
	u8 phase;
	u8 cdesc_entry_size_in_bytes;

} ____cacheline_aligned;

struct ena_com_io_sq {
	struct ena_com_io_desc_addr desc_addr;

	u32 __iomem *db_addr;
	u8 __iomem *header_addr;

	enum queue_direction direction;
	enum ena_com_memory_queue_type mem_queue_type;

	u32 msix_vector;
	struct ena_com_tx_meta cached_tx_meta;

	u16 q_depth;
	u16 qid;

	u16 idx;
	u16 tail;
	u16 next_to_comp;
	u8 phase;
	u8 desc_entry_size;
} ____cacheline_aligned;

struct ena_com_admin_cq {
	struct ena_admin_acq_entry *entries;
	dma_addr_t dma_addr;

	u16 head;
	u8 phase;
};

struct ena_com_admin_sq {
	struct ena_admin_aq_entry *entries;
	dma_addr_t dma_addr;

	u32 __iomem *db_addr;

	u16 head;
	u16 tail;
	u8 phase;

};

struct ena_com_admin_queue {
	void *q_dmadev;
	spinlock_t q_lock; /* spinlock for the admin queue */
	struct ena_comp_ctx *comp_ctx;
	u16 q_depth;
	struct ena_com_admin_cq cq;
	struct ena_com_admin_sq sq;

	/* Indicate if the admin queue should poll for completion */
	bool polling;

	u16 curr_cmd_id;

	/* Indicate that the ena was initialized and can
	 * process new admin commands
	 */
	bool running_state;

	/* Count the number of outstanding admin commands */
	atomic_t outstanding_cmds;
};

struct ena_aenq_handlers;

struct ena_com_aenq {
	u16 head;
	u8 phase;
	struct ena_admin_aenq_entry *entries;
	dma_addr_t dma_addr;
	u16 q_depth;
	struct ena_aenq_handlers *aenq_handlers;
};

struct ena_com_mmio_read {
	struct ena_admin_ena_mmio_read_less_resp *read_resp;
	dma_addr_t read_resp_dma_addr;
	u16 seq_num;
	/* spin lock to ensure a single outstanding read */
	spinlock_t lock;
};

/* Each ena_dev is a PCI function. */
struct ena_com_dev {
	struct ena_com_admin_queue admin_queue;
	struct ena_com_aenq aenq;
	struct ena_com_io_cq io_cq_queues[ENA_TOTAL_NUM_QUEUES];
	struct ena_com_io_sq io_sq_queues[ENA_TOTAL_NUM_QUEUES];
	struct ena_regs_ena_registers __iomem *reg_bar;
	void __iomem *mem_bar;
	void *dmadev;

	enum ena_com_memory_queue_type tx_mem_queue_type;

	u16 stats_func; /* Selected function for extended statistic dump */
	u16 stats_queue; /* Selected queue for extended statistic dump */

	struct ena_com_mmio_read mmio_read;
};

struct ena_com_dev_get_features_ctx {
	struct ena_admin_queue_feature_desc max_queues;
	struct ena_admin_device_attr_feature_desc dev_attr;
	struct ena_admin_feature_aenq_desc aenq;
	struct ena_admin_feature_offload_desc offload;
};

/*****************************************************************************/
/*****************************************************************************/

int ena_com_get_link_params(struct ena_com_dev *ena_dev,
			    struct ena_admin_get_feat_resp *resp);

int ena_com_get_io_handlers(struct ena_com_dev *ena_dev, u16 qid,
			    struct ena_com_io_sq **io_sq,
			    struct ena_com_io_cq **io_cq);

int ena_com_mmio_reg_read_request_init(struct ena_com_dev *ena_dev);

void ena_com_mmio_reg_read_request_destroy(struct ena_com_dev *ena_dev);

void ena_com_mmio_reg_read_request_write_dev_addr(struct ena_com_dev *ena_dev);

int ena_com_get_dma_width(struct ena_com_dev *ena_dev);

int ena_com_validate_version(struct ena_com_dev *ena_dev);

void ena_com_set_admin_running_state(struct ena_com_dev *ena_dev, bool state);

bool ena_com_get_admin_running_state(struct ena_com_dev *ena_dev);

int ena_com_set_interrupt_moderation(struct ena_com_dev *ena_dev, int qid,
				     bool enable, u16 count, u16 interval);

void ena_com_set_admin_polling_mode(struct ena_com_dev *ena_dev, bool polling);

bool ena_com_get_ena_admin_polling_mode(struct ena_com_dev *ena_dev);

int ena_com_admin_init(struct ena_com_dev *ena_dev,
		       struct ena_aenq_handlers *aenq_handlers,
		       bool init_spinlock);

void ena_com_admin_aenq_enable(struct ena_com_dev *ena_dev);

int ena_com_get_dev_attr_feat(struct ena_com_dev *ena_dev,
			      struct ena_com_dev_get_features_ctx
			      *get_feat_ctx);

int ena_com_get_dev_basic_stats(struct ena_com_dev *ena_dev,
				struct ena_admin_basic_stats *stats);

int ena_com_set_dev_mtu(struct ena_com_dev *ena_dev, int mtu);

int ena_com_create_io_queue(struct ena_com_dev *ena_dev, u16 qid,
			    enum queue_direction direction,
			    enum ena_com_memory_queue_type mem_queue_type,
			    u32 msix_vector,
			    u16 queue_size);

void ena_com_admin_destroy(struct ena_com_dev *ena_dev);

void ena_com_destroy_io_queue(struct ena_com_dev *ena_dev, u16 qid);

void ena_com_admin_q_comp_intr_handler(struct ena_com_dev *ena_dev);

void ena_com_admin_queue_completion_int_handler(struct ena_com_dev *ena_dev);

int ena_com_create_io_cq(struct ena_com_dev *ena_dev,
			 struct ena_com_io_cq *io_cq);

int ena_com_create_io_sq(struct ena_com_dev *ena_dev,
			 struct ena_com_io_sq *io_sq, u16 cq_idx);

int ena_com_destroy_io_cq(struct ena_com_dev *ena_dev,
			  struct ena_com_io_cq *io_cq);

int ena_com_execute_admin_command(struct ena_com_admin_queue *admin_queue,
				  struct ena_admin_aq_entry *cmd,
				  size_t cmd_size,
				  struct ena_admin_acq_entry *cmd_comp,
				  size_t cmd_comp_size);

void ena_com_abort_admin_commands(struct ena_com_dev *ena_dev);

void ena_com_wait_for_abort_completion(struct ena_com_dev *ena_dev);

void ena_com_aenq_intr_handler(struct ena_com_dev *dev, void *data);

typedef void (*ena_aenq_handler)(void *data,
	struct ena_admin_aenq_entry *aenq_e);

/* Holds all aenq handlers. Indexed by AENQ event group */
struct ena_aenq_handlers {
	ena_aenq_handler handlers[ENA_MAX_HANDLERS];
	ena_aenq_handler unimplemented_handler;
};

int ena_com_dev_reset(struct ena_com_dev *ena_dev);

int ena_com_get_offload_settings(struct ena_com_dev *ena_dev,
				 struct ena_admin_feature_offload_desc
				 *offload);

#endif /* !(ENA_COM) */
