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

#ifndef ENA_ETH_COM_H_
#define ENA_ETH_COM_H_

#include "ena_com.h"

#define ENA_MAX_PUSH_PKT_SIZE 128

struct ena_com_tx_ctx {
	struct ena_com_tx_meta ena_meta;
	struct ena_com_buf *ena_bufs;
	/* For LLQ, header buffer - pushed to the device mem space */
	void *push_header;

	enum ena_eth_io_l3_proto_index l3_proto;
	enum ena_eth_io_l4_proto_index l4_proto;
	u16 num_bufs;
	u16 req_id;
	/* For regular queue, indicate the size of the header
	 * For LLQ, indicate the size of the pushed buffer
	 */
	u16 header_len;

	bool meta_valid;
	bool tso_enable;
	bool l3_csum_enable;
	bool l4_csum_enable;
	bool l4_csum_partial;
	bool tunnel_ctrl;
};

struct ena_com_rx_ctx {
	struct ena_com_buf *ena_bufs;
	enum ena_eth_io_l3_proto_index l3_proto;
	enum ena_eth_io_l4_proto_index l4_proto;
	bool l3_csum_err;
	bool l4_csum_err;
	/* fragmented packet */
	bool frag;
	u16 hash_frag_csum;
	int max_bufs;
};

static inline struct ena_eth_io_rx_cdesc_base *ena_com_get_next_rx_cdesc(
	struct ena_com_io_cq *io_cq)
{
	struct ena_eth_io_rx_cdesc_base *cdesc;
	u16 expected_phase, head_masked;
	u16 desc_phase;

	head_masked = io_cq->head & (io_cq->q_depth - 1);
	expected_phase = io_cq->phase;

	cdesc = (struct ena_eth_io_rx_cdesc_base *)(io_cq->cdesc_addr.virt_addr
			+ (head_masked * io_cq->cdesc_entry_size_in_bytes));

	desc_phase = (cdesc->status & ENA_ETH_IO_RX_CDESC_BASE_PHASE_MASK) >>
			ENA_ETH_IO_RX_CDESC_BASE_PHASE_SHIFT;

	if (desc_phase != expected_phase)
		return NULL;

	return cdesc;
}

static inline void ena_com_cq_inc_head(struct ena_com_io_cq *io_cq)
{
	io_cq->head++;

	/* Switch phase bit in case of wrap around */
	if (unlikely((io_cq->head & (io_cq->q_depth - 1)) == 0))
		io_cq->phase = 1 - io_cq->phase;
}

static inline void *get_sq_desc(struct ena_com_io_sq *io_sq)
{
	u16 tail_masked;
	u32 offset;

	tail_masked = io_sq->tail & (io_sq->q_depth - 1);

	offset = tail_masked * io_sq->desc_entry_size;

	return io_sq->desc_addr.virt_addr + offset;
}

static inline void ena_com_copy_curr_sq_desc_to_dev(struct ena_com_io_sq *io_sq)
{
	u16 tail_masked = io_sq->tail & (io_sq->q_depth - 1);
	u32 offset = tail_masked * io_sq->desc_entry_size;

	/* In case this queue isn't a LLQ */
	if (io_sq->mem_queue_type == ENA_MEM_QUEUE_TYPE_HOST_MEMORY)
		return;

	memcpy(io_sq->desc_addr.pbuf_dev_addr + offset,
	       io_sq->desc_addr.virt_addr + offset,
	       io_sq->desc_entry_size);
}

static inline void ena_com_sq_update_tail(struct ena_com_io_sq *io_sq)
{
	io_sq->tail++;

	/* Switch phase bit in case of wrap around */
	if (unlikely((io_sq->tail & (io_sq->q_depth - 1)) == 0))
		io_sq->phase = 1 - io_sq->phase;
}

static inline int ena_com_write_header(struct ena_com_io_sq *io_sq,
				       u8 *head_src, u16 header_len)
{
	u16 tail_masked = io_sq->tail & (io_sq->q_depth - 1);
	u8 *dev_head_addr =
		io_sq->header_addr + (tail_masked * ENA_MAX_PUSH_PKT_SIZE);

	if (io_sq->mem_queue_type != ENA_MEM_QUEUE_TYPE_DEVICE_MEMORY)
		return 0;

	ENA_ASSERT(io_sq->header_addr, "header address is NULL\n");

	if (unlikely(header_len > ENA_MAX_PUSH_PKT_SIZE)) {
		ena_trc_err("header size is too large\n");
		return -EINVAL;
	}

	memcpy(dev_head_addr, head_src, header_len);

	return 0;
}

static inline struct ena_eth_io_rx_cdesc_base *
	ena_com_rx_cdesc_idx_to_ptr(struct ena_com_io_cq *io_cq, u16 idx)
{
	idx &= (io_cq->q_depth - 1);
	return (struct ena_eth_io_rx_cdesc_base *)(io_cq->cdesc_addr.virt_addr +
			idx * io_cq->cdesc_entry_size_in_bytes);
}

static inline int ena_com_cdesc_rx_pkt_get(struct ena_com_io_cq *io_cq,
					   u16 *first_cdesc_idx,
					   u16 *nb_hw_desc)
{
	struct ena_eth_io_rx_cdesc_base *cdesc;
	u16 count = 0, head_masked;
	u32 last = 0;

	do {
		cdesc = ena_com_get_next_rx_cdesc(io_cq);
		if (!cdesc)
			break;

		ena_com_cq_inc_head(io_cq);
		count++;
		last = (cdesc->status & ENA_ETH_IO_RX_CDESC_BASE_LAST_MASK) >>
			ENA_ETH_IO_RX_CDESC_BASE_LAST_SHIFT;
	} while (!last);

	if (last) {
		*first_cdesc_idx = io_cq->cur_rx_pkt_cdesc_start_idx;
		count += io_cq->cur_rx_pkt_cdesc_count;

		head_masked = io_cq->head & (io_cq->q_depth - 1);

		io_cq->cur_rx_pkt_cdesc_count = 0;
		io_cq->cur_rx_pkt_cdesc_start_idx = head_masked;

		ena_trc_dbg("ena q_id: %d packets were completed. first desc idx %u descs# %d\n",
			    io_cq->qid, *first_cdesc_idx, count);
	} else {
		io_cq->cur_rx_pkt_cdesc_count += count;
		count = 0;
	}

	*nb_hw_desc = count;
	return 0;
}

static inline bool ena_com_meta_desc_changed(struct ena_com_io_sq *io_sq,
					     struct ena_com_tx_ctx *ena_tx_ctx)
{
	int rc;

	if (ena_tx_ctx->meta_valid) {
		rc = memcmp(&io_sq->cached_tx_meta,
			    &ena_tx_ctx->ena_meta,
			    sizeof(struct ena_com_tx_meta));

		if (unlikely(rc != 0))
			return true;
	}

	return false;
}

static inline void ena_com_create_and_store_tx_meta_desc(
	struct ena_com_io_sq *io_sq,
	struct ena_com_tx_ctx *ena_tx_ctx)
{
	struct ena_eth_io_tx_meta_desc *meta_desc = NULL;
	struct ena_com_tx_meta *ena_meta = &ena_tx_ctx->ena_meta;

	meta_desc = get_sq_desc(io_sq);
	memset(meta_desc, 0x0, sizeof(struct ena_eth_io_tx_meta_desc));

	meta_desc->len_ctrl |= ENA_ETH_IO_TX_META_DESC_META_DESC_MASK;

	if (ena_tx_ctx->tunnel_ctrl) {
		/* Bits 0-2 */
		meta_desc->word3 |= (ena_meta->l3_outer_hdr_offset <<
			ENA_ETH_IO_TX_META_DESC_OUTR_L3_OFF_LO_SHIFT) &
			ENA_ETH_IO_TX_META_DESC_OUTR_L3_OFF_LO_MASK;
		/* Bits 3-4 */
		meta_desc->len_ctrl |= ((ena_meta->l3_outer_hdr_offset >> 0x3)
			<< ENA_ETH_IO_TX_META_DESC_OUTR_L3_OFF_HI_SHIFT) &
			ENA_ETH_IO_TX_META_DESC_OUTR_L3_OFF_HI_MASK;

		meta_desc->word3 |= (ena_meta->l3_outer_hdr_len <<
			ENA_ETH_IO_TX_META_DESC_OUTR_L3_HDR_LEN_WORDS_SHIFT) &
			ENA_ETH_IO_TX_META_DESC_OUTR_L3_HDR_LEN_WORDS_MASK;
	}

	meta_desc->len_ctrl |= ENA_ETH_IO_TX_META_DESC_EXT_VALID_MASK;

	/* bits 0-9 of the mss */
	meta_desc->word2 |= (ena_meta->mss <<
		ENA_ETH_IO_TX_META_DESC_MSS_LO_SHIFT) &
		ENA_ETH_IO_TX_META_DESC_MSS_LO_MASK;
	/* bits 10-13 of the mss */
	meta_desc->len_ctrl |= ((ena_meta->mss >> 10) <<
		ENA_ETH_IO_TX_META_DESC_MSS_HI_PTP_SHIFT) &
		ENA_ETH_IO_TX_META_DESC_MSS_HI_PTP_MASK;

	/* Extended meta desc */
	meta_desc->len_ctrl |= ENA_ETH_IO_TX_META_DESC_ETH_META_TYPE_MASK;
	meta_desc->len_ctrl |= ENA_ETH_IO_TX_META_DESC_META_STORE_MASK;
	meta_desc->len_ctrl |= (io_sq->phase <<
		ENA_ETH_IO_TX_META_DESC_PHASE_SHIFT) &
		ENA_ETH_IO_TX_META_DESC_PHASE_MASK;

	meta_desc->len_ctrl |= ENA_ETH_IO_TX_META_DESC_FIRST_MASK;
	meta_desc->word2 |= ena_meta->l3_hdr_len &
		ENA_ETH_IO_TX_META_DESC_L3_HDR_LEN_MASK;
	meta_desc->word2 |= (ena_meta->l3_hdr_offset <<
		ENA_ETH_IO_TX_META_DESC_L3_HDR_OFF_SHIFT) &
		ENA_ETH_IO_TX_META_DESC_L3_HDR_OFF_MASK;

	meta_desc->word2 |= (ena_meta->l4_hdr_len <<
		ENA_ETH_IO_TX_META_DESC_L4_HDR_LEN_IN_WORDS_SHIFT) &
		ENA_ETH_IO_TX_META_DESC_L4_HDR_LEN_IN_WORDS_MASK;

	meta_desc->len_ctrl |= ENA_ETH_IO_TX_META_DESC_META_STORE_MASK;

	/* Cached the meta desc */
	memcpy(&io_sq->cached_tx_meta, ena_meta,
	       sizeof(struct ena_com_tx_meta));

	ena_com_copy_curr_sq_desc_to_dev(io_sq);
	ena_com_sq_update_tail(io_sq);
}

static inline void ena_com_rx_set_flags(struct ena_com_rx_ctx *ena_rx_ctx,
					struct ena_eth_io_rx_cdesc_base *cdesc)
{
	ena_rx_ctx->l3_proto = cdesc->status &
		ENA_ETH_IO_RX_CDESC_BASE_L3_PROTO_IDX_MASK;
	ena_rx_ctx->l4_proto =
		(cdesc->status & ENA_ETH_IO_RX_CDESC_BASE_L4_PROTO_IDX_MASK) >>
		ENA_ETH_IO_RX_CDESC_BASE_L4_PROTO_IDX_SHIFT;
	ena_rx_ctx->l3_csum_err =
		(cdesc->status & ENA_ETH_IO_RX_CDESC_BASE_L3_CSUM_ERR_MASK) >>
		ENA_ETH_IO_RX_CDESC_BASE_L3_CSUM_ERR_SHIFT;
	ena_rx_ctx->l4_csum_err =
		(cdesc->status & ENA_ETH_IO_RX_CDESC_BASE_L4_CSUM_ERR_MASK) >>
		ENA_ETH_IO_RX_CDESC_BASE_L4_CSUM_ERR_SHIFT;
	ena_rx_ctx->hash_frag_csum =
		(cdesc->word2 & ENA_ETH_IO_RX_CDESC_BASE_HASH_FRAG_CSUM_MASK) >>
		ENA_ETH_IO_RX_CDESC_BASE_HASH_FRAG_CSUM_SHIFT;
	ena_rx_ctx->frag =
		(cdesc->status & ENA_ETH_IO_RX_CDESC_BASE_IPV4_FRAG_MASK) >>
		ENA_ETH_IO_RX_CDESC_BASE_IPV4_FRAG_SHIFT;

	ena_trc_dbg("ena_rx_ctx->l3_proto %d ena_rx_ctx->l4_proto %d\nena_rx_ctx->l3_csum_err %d ena_rx_ctx->l4_csum_err %d\nhash frag %d frag: %d cdesc_status: %x\n",
		    ena_rx_ctx->l3_proto,
		    ena_rx_ctx->l4_proto,
		    ena_rx_ctx->l3_csum_err,
		    ena_rx_ctx->l4_csum_err,
		    ena_rx_ctx->hash_frag_csum,
		    ena_rx_ctx->frag,
		    cdesc->status);
}

/*****************************************************************************/
/*****************************     API      **********************************/
/*****************************************************************************/

static inline int ena_com_sq_empty_space(struct ena_com_io_sq *io_sq)
{
	u16 tail, next_to_comp, cnt;

	next_to_comp = io_sq->next_to_comp;
	tail = io_sq->tail;
	cnt = tail - next_to_comp;

	return io_sq->q_depth - 1 - cnt;
}

static inline int ena_com_write_sq_doorbell(struct ena_com_io_sq *io_sq)
{
	u16 tail;

	tail = io_sq->tail;

	ena_trc_dbg("write db for queue: %d tail: %d\n", io_sq->qid, tail);

	writel(tail, io_sq->db_addr);

	return 0;
}

static inline int ena_com_prepare_tx(struct ena_com_io_sq *io_sq,
				     struct ena_com_tx_ctx *ena_tx_ctx,
				     int *nb_hw_desc)
{
	struct ena_eth_io_tx_desc *desc = NULL;
	struct ena_com_buf *ena_bufs = ena_tx_ctx->ena_bufs;
	void *push_header = ena_tx_ctx->push_header;
	u16 header_len = ena_tx_ctx->header_len;
	u16 num_bufs = ena_tx_ctx->num_bufs;
	int total_desc, i, rc;
	bool have_meta;

	ENA_ASSERT(io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX,
		   "wrong Q type");

	/* num_bufs +1 for potential meta desc */
	if (ena_com_sq_empty_space(io_sq) < (num_bufs + 1))
		return -ENOMEM;

	have_meta = ena_tx_ctx->meta_valid && ena_com_meta_desc_changed(io_sq,
			ena_tx_ctx);
	if (have_meta)
		ena_com_create_and_store_tx_meta_desc(io_sq, ena_tx_ctx);

	/* start with pushing the header (if needed) */
	rc = ena_com_write_header(io_sq, push_header, header_len);
	if (unlikely(rc))
		return rc;

	desc = get_sq_desc(io_sq);
	memset(desc, 0x0, sizeof(struct ena_eth_io_tx_desc));

	/* Set first desc when we don't have meta descriptor */
	if (!have_meta)
		desc->len_ctrl |= ENA_ETH_IO_TX_DESC_FIRST_MASK;

	desc->buff_addr_hi_hdr_sz |= (header_len <<
		ENA_ETH_IO_TX_DESC_PUSH_BUFFER_LENGTH_SHIFT) &
		ENA_ETH_IO_TX_DESC_PUSH_BUFFER_LENGTH_MASK;
	desc->len_ctrl |= (io_sq->phase << ENA_ETH_IO_TX_DESC_PHASE_SHIFT) &
		ENA_ETH_IO_TX_DESC_PHASE_MASK;

	desc->len_ctrl |= ENA_ETH_IO_TX_DESC_COMP_REQ_MASK;

	/* Bits 0-9 */
	desc->meta_ctrl |= (ena_tx_ctx->req_id <<
		ENA_ETH_IO_TX_DESC_REQ_ID_LO_SHIFT) &
		ENA_ETH_IO_TX_DESC_REQ_ID_LO_MASK;

	/* Bits 10-15 */
	desc->len_ctrl |= ((ena_tx_ctx->req_id >> 10) <<
		ENA_ETH_IO_TX_DESC_REQ_ID_HI_SHIFT) &
		ENA_ETH_IO_TX_DESC_REQ_ID_HI_MASK;

	if (ena_tx_ctx->meta_valid) {
		desc->meta_ctrl |= (ena_tx_ctx->tso_enable <<
			ENA_ETH_IO_TX_DESC_TSO_EN_SHIFT) &
			ENA_ETH_IO_TX_DESC_TSO_EN_MASK;
		desc->meta_ctrl |= ena_tx_ctx->l3_proto &
			ENA_ETH_IO_TX_DESC_L3_PROTO_IDX_MASK;
		desc->meta_ctrl |= (ena_tx_ctx->l4_proto <<
			ENA_ETH_IO_TX_DESC_L4_PROTO_IDX_SHIFT) &
			ENA_ETH_IO_TX_DESC_L4_PROTO_IDX_MASK;
		desc->meta_ctrl |= (ena_tx_ctx->l3_csum_enable <<
			ENA_ETH_IO_TX_DESC_L3_CSUM_EN_SHIFT) &
			ENA_ETH_IO_TX_DESC_L3_CSUM_EN_MASK;
		desc->meta_ctrl |= (ena_tx_ctx->l4_csum_enable <<
			ENA_ETH_IO_TX_DESC_L4_CSUM_EN_SHIFT) &
			ENA_ETH_IO_TX_DESC_L4_CSUM_EN_MASK;
		desc->meta_ctrl |= (ena_tx_ctx->l4_csum_partial <<
			ENA_ETH_IO_TX_DESC_L4_CSUM_PARTIAL_SHIFT) &
			ENA_ETH_IO_TX_DESC_L4_CSUM_PARTIAL_MASK;
		desc->meta_ctrl |= (ena_tx_ctx->tunnel_ctrl <<
			ENA_ETH_IO_TX_DESC_TUNNEL_CTRL_SHIFT) &
			ENA_ETH_IO_TX_DESC_TUNNEL_CTRL_MASK;
	}

	for (i = 0; i < num_bufs; i++) {
		/* The first desc share the same desc as the header */
		if (likely(i != 0)) {
			ena_com_copy_curr_sq_desc_to_dev(io_sq);
			ena_com_sq_update_tail(io_sq);

			desc = get_sq_desc(io_sq);
			memset(desc, 0x0, sizeof(struct ena_eth_io_tx_desc));

			desc->len_ctrl |= (io_sq->phase <<
				ENA_ETH_IO_TX_DESC_PHASE_SHIFT) &
				ENA_ETH_IO_TX_DESC_PHASE_MASK;
		}

		desc->len_ctrl |= ena_bufs->len &
			ENA_ETH_IO_TX_DESC_LENGTH_MASK;

		desc->buff_addr_hi_hdr_sz |= (ena_bufs->paddr >> 32) &
			ENA_ETH_IO_TX_DESC_ADDR_HI_MASK;
		desc->buff_addr_lo = ena_bufs->paddr & 0xFFFFFFFF;
		ena_bufs++;
	}

	/* set the last desc indicator */
	desc->len_ctrl |= ENA_ETH_IO_TX_DESC_LAST_MASK;

	ena_com_copy_curr_sq_desc_to_dev(io_sq);

	ena_com_sq_update_tail(io_sq);

	total_desc = max_t(u16, num_bufs, 1);
	total_desc += have_meta ? 1 : 0;

	*nb_hw_desc = total_desc;
	return 0;
}

static inline int ena_com_rx_pkt(struct ena_com_io_cq *io_cq,
				 struct ena_com_io_sq *io_sq,
				 struct ena_com_rx_ctx *ena_rx_ctx)
{
	struct ena_com_buf *ena_buf = &ena_rx_ctx->ena_bufs[0];
	struct ena_eth_io_rx_cdesc_base *cdesc = NULL;
	u16 cdesc_idx = 0;
	u16 nb_hw_desc;
	u16 i;
	int rc;

	ENA_ASSERT(io_cq->direction == ENA_COM_IO_QUEUE_DIRECTION_RX,
		   "wrong Q type");

	rc = ena_com_cdesc_rx_pkt_get(io_cq, &cdesc_idx, &nb_hw_desc);
	if (rc || (nb_hw_desc == 0))
		return 0;

	ena_trc_dbg(
		"fetch rx packet: queue %d completed desc: %d\n",
		io_cq->qid, nb_hw_desc);

	ENA_ASSERT(nb_hw_desc <= ena_rx_ctx->max_bufs,
		   "Too many RX cdescs (%d) > MAX(%d)\n",
		   nb_hw_desc, ena_rx_ctx->max_bufs);

	for (i = 0; i < nb_hw_desc; i++) {
		cdesc = ena_com_rx_cdesc_idx_to_ptr(io_cq, cdesc_idx + i);

		ena_buf->len = cdesc->length;
		ena_buf++;
	}

	/* Update SQ head ptr */
	io_sq->next_to_comp += nb_hw_desc;

	ena_trc_dbg("[%s][QID#%d] Updating SQ head to: %d\n", __func__,
		    io_sq->qid, io_sq->next_to_comp);

	/* Get rx flags from the last pkt */
	ena_com_rx_set_flags(ena_rx_ctx, cdesc);

	return nb_hw_desc;
}

static inline int ena_com_add_single_rx_desc(struct ena_com_io_sq *io_sq,
					     struct ena_com_buf *ena_buf)
{
	struct ena_eth_io_rx_desc *desc;

	ENA_ASSERT(io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_RX,
		   "wrong Q type");

	if (unlikely(ena_com_sq_empty_space(io_sq) == 0))
		return -1;

	desc = get_sq_desc(io_sq);
	memset(desc, 0x0, sizeof(struct ena_eth_io_rx_desc));

	desc->length = ena_buf->len;

	desc->ctrl |= ENA_ETH_IO_RX_DESC_FIRST_MASK;
	desc->ctrl |= ENA_ETH_IO_RX_DESC_LAST_MASK;
	desc->ctrl |= io_sq->phase & ENA_ETH_IO_RX_DESC_PHASE_MASK;
	desc->ctrl |= ENA_ETH_IO_RX_DESC_COMP_REQ_MASK;

	desc->req_id = io_sq->tail;

	desc->buff_addr_lo = ena_buf->paddr & 0xFFFFFFFF;
	desc->buff_addr_hi = (ena_buf->paddr >> 32) & 0xFFFF;

	ena_com_sq_update_tail(io_sq);

	return 0;
}

static inline void ena_com_unmask_intr(struct ena_com_io_cq *io_cq)
{
	writel(io_cq->unmask_val, io_cq->unmask_reg);
}

static inline int ena_com_tx_comp_req_id_get(struct ena_com_io_cq *io_cq,
					     u16 *req_id)
{
	u8 expected_phase, cdesc_phase;
	struct ena_eth_io_tx_cdesc *cdesc;
	u16 masked_head;

	masked_head = io_cq->head & (io_cq->q_depth - 1);
	expected_phase = io_cq->phase;

	cdesc = (struct ena_eth_io_tx_cdesc *)(io_cq->cdesc_addr.virt_addr
		+ (masked_head * io_cq->cdesc_entry_size_in_bytes));

	cdesc_phase = cdesc->flags & ENA_ETH_IO_TX_CDESC_PHASE_MASK;
	if (cdesc_phase != expected_phase)
		return -1;

	ena_com_cq_inc_head(io_cq);

	*req_id = cdesc->req_id;

	return 0;
}

static inline void ena_com_comp_ack(struct ena_com_io_sq *io_sq, u16 elem)
{
	io_sq->next_to_comp += elem;
}

#endif /* ENA_ETH_COM_H_ */
