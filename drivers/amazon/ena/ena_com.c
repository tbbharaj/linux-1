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

#include "ena_com.h"
#include "ena_gen_info.h"

/*****************************************************************************/
/*****************************************************************************/

/* Timeout in micro-sec */
#define ADMIN_CMD_TIMEOUT_US (10 * 1000000)

#define ENA_ASYNC_QUEUE_DEPTH 4
#define ENA_ADMIN_QUEUE_DEPTH 32

/* TODO get spec version from ena_defs */
/* Minimal spec 0.9 */
#define MIN_ENA_VER (((0) << ENA_REGS_VERSION_MAJOR_VERSION_SHIFT) | (9))

#define ENA_CTRL_MAJOR		0
#define ENA_CTRL_MINOR		0
#define ENA_CTRL_SUB_MINOR	1

#define MIN_ENA_CTRL_VER \
	(((ENA_CTRL_MAJOR) << \
	(ENA_REGS_CONTROLLER_VERSION_MAJOR_VERSION_SHIFT)) | \
	((ENA_CTRL_MINOR) << \
	(ENA_REGS_CONTROLLER_VERSION_MINOR_VERSION_SHIFT)) | \
	(ENA_CTRL_SUB_MINOR))

#define ENA_DMA_ADDR_TO_UINT32_LOW(x)	((u32)((u64)(x)))
#define ENA_DMA_ADDR_TO_UINT32_HIGH(x)	((u32)(((u64)(x)) >> 32))

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

enum ena_cmd_status {
	ENA_CMD_ILLIGAL,
	ENA_CMD_SUBMITTED,
	ENA_CMD_COMPLETED,
	/* Abort - canceled by the driver */
	ENA_CMD_ABORTED,
	/* fail - failed to execute (by the HW) */
	ENA_CMD_FAILED
} ____cacheline_aligned;

struct ena_comp_ctx {
	struct completion wait_event;
	struct ena_admin_acq_entry *user_cqe;
	u32 comp_size;
	enum ena_cmd_status status;
	/* status from the device */
	u8 comp_status;
	u8 cmd_opcode;
	bool occupied;
};

static inline void ena_com_mem_addr_set(struct ena_common_mem_addr *ena_addr,
					dma_addr_t addr)
{
	ena_addr->mem_addr_low = (u32)addr;
	ena_addr->mem_addr_high = (addr >> 32) & 0xffff;

	ENA_ASSERT((addr >> ENA_MAX_PHYS_ADDR_SIZE_BITS) == 0,
		   "Invalid addr (address have more than 48 bits");
}

static int ena_com_admin_init_sq(struct ena_com_admin_queue *queue)
{
	queue->sq.entries =
		dma_alloc_coherent(queue->q_dmadev,
				   ADMIN_SQ_SIZE(queue->q_depth),
				   &queue->sq.dma_addr,
				   GFP_KERNEL | __GFP_ZERO);

	if (!queue->sq.entries)
		return -ENOMEM;

	queue->sq.head = 0;
	queue->sq.tail = 0;
	queue->sq.phase = 1;

	queue->sq.db_addr = NULL;

	return 0;
}

static int ena_com_admin_init_cq(struct ena_com_admin_queue *queue)
{
	queue->cq.entries =
		dma_alloc_coherent(queue->q_dmadev,
				   ADMIN_CQ_SIZE(queue->q_depth),
				   &queue->cq.dma_addr,
				   GFP_KERNEL | __GFP_ZERO);
	if (!queue->cq.entries)
		return -ENOMEM;

	queue->cq.head = 0;
	queue->cq.phase = 1;

	return 0;
}

static int ena_com_admin_init_aenq(struct ena_com_dev *dev,
				   struct ena_aenq_handlers *aenq_handlers)
{
	u32 aenq_caps;

	dev->aenq.q_depth = ENA_ASYNC_QUEUE_DEPTH;
	dev->aenq.entries =
		dma_alloc_coherent(dev->dmadev,
				   ADMIN_AENQ_SIZE(dev->aenq.q_depth),
				   &dev->aenq.dma_addr,
				   GFP_KERNEL | __GFP_ZERO);
	if (!dev->aenq.entries)
		return -ENOMEM;

	dev->aenq.head = dev->aenq.q_depth;
	dev->aenq.phase = 1;

	dev->reg_bar->aenq_base_lo =
		ENA_DMA_ADDR_TO_UINT32_LOW(dev->aenq.dma_addr);
	dev->reg_bar->aenq_base_hi =
		ENA_DMA_ADDR_TO_UINT32_HIGH(dev->aenq.dma_addr);

	aenq_caps = 0;
	aenq_caps |= dev->aenq.q_depth & ENA_REGS_AENQ_CAPS_AENQ_DEPTH_MASK;
	aenq_caps |= (sizeof(struct ena_admin_aenq_entry) <<
		ENA_REGS_AENQ_CAPS_AENQ_ENTRY_SIZE_SHIFT) &
		ENA_REGS_AENQ_CAPS_AENQ_ENTRY_SIZE_MASK;
	writel(aenq_caps, &dev->reg_bar->aenq_caps);

	dev->aenq.aenq_handlers = aenq_handlers;

	return 0;
}

static inline void comp_ctxt_release(struct ena_com_admin_queue *queue,
				     struct ena_comp_ctx *comp_ctx)
{
	comp_ctx->occupied = false;
	atomic_dec(&queue->outstanding_cmds);
}

static struct ena_comp_ctx *get_comp_ctxt(struct ena_com_admin_queue *queue,
					  u16 command_id, bool capture)
{
	if (unlikely(command_id >= queue->q_depth)) {
		ena_trc_err("command id is larger than the queue size. cmd_id: %u queue size %d\n",
			    command_id, queue->q_depth);
		return ERR_PTR(-EINVAL);
	}

	if (unlikely(queue->comp_ctx[command_id].occupied && capture))
		return ERR_PTR(-ENOSPC);

	if (capture) {
		atomic_inc(&queue->outstanding_cmds);
		queue->comp_ctx[command_id].occupied = true;
	}

	return &queue->comp_ctx[command_id];
}

static struct ena_comp_ctx *__ena_com_submit_admin_cmd(
		struct ena_com_admin_queue *admin_queue,
		struct ena_admin_aq_entry *cmd,
		size_t cmd_size_in_bytes,
		struct ena_admin_acq_entry *comp,
		size_t comp_size_in_bytes)
{
	struct ena_comp_ctx *comp_ctx;
	u16 tail_masked, cmd_id;
	u16 queue_size_mask;
	u16 cnt;

	queue_size_mask = admin_queue->q_depth - 1;

	tail_masked = admin_queue->sq.tail & queue_size_mask;

	/* In case of queue FULL */
	cnt = admin_queue->sq.tail - admin_queue->sq.head;
	if (cnt >= admin_queue->q_depth) {
		ena_trc_dbg("admin queue is FULL (tail %d head %d depth: %d)\n",
			    admin_queue->sq.tail,
			    admin_queue->sq.head,
			    admin_queue->q_depth);
		return ERR_PTR(-ENOSPC);
	}

	cmd_id = admin_queue->curr_cmd_id;
	admin_queue->curr_cmd_id = (admin_queue->curr_cmd_id + 1) &
		queue_size_mask;

	cmd->aq_common_descriptor.flags |= admin_queue->sq.phase &
		ENA_ADMIN_AQ_COMMON_DESC_PHASE_MASK;

	cmd->aq_common_descriptor.command_id |= cmd_id &
		ENA_ADMIN_AQ_COMMON_DESC_COMMAND_ID_MASK;

	comp_ctx = get_comp_ctxt(admin_queue, cmd_id, true);
	if (unlikely(IS_ERR(comp_ctx)))
		return comp_ctx;

	comp_ctx->status = ENA_CMD_SUBMITTED;
	comp_ctx->comp_size = (u32)comp_size_in_bytes;
	comp_ctx->user_cqe = comp;
	comp_ctx->cmd_opcode = cmd->aq_common_descriptor.opcode;

	reinit_completion(&comp_ctx->wait_event);

	memcpy(&admin_queue->sq.entries[tail_masked], cmd, cmd_size_in_bytes);

	admin_queue->sq.tail++;

	if (unlikely((admin_queue->sq.tail & queue_size_mask) == 0))
		admin_queue->sq.phase = 1 - admin_queue->sq.phase;

	writel(admin_queue->sq.tail, admin_queue->sq.db_addr);

	return comp_ctx;
}

static inline int ena_com_init_comp_ctxt(struct ena_com_admin_queue *queue)
{
	size_t size = queue->q_depth * sizeof(struct ena_comp_ctx);
	struct ena_comp_ctx *comp_ctx;
	u16 i;

	queue->comp_ctx = devm_kzalloc(queue->q_dmadev, size, GFP_KERNEL);
	if (unlikely(!queue->comp_ctx))
		return -ENOMEM;

	for (i = 0; i < queue->q_depth; i++) {
		comp_ctx = get_comp_ctxt(queue, i, false);
		init_completion(&comp_ctx->wait_event);
	}

	return 0;
}

static struct ena_comp_ctx *ena_com_submit_admin_cmd(
		struct ena_com_admin_queue *admin_queue,
		struct ena_admin_aq_entry *cmd,
		size_t cmd_size_in_bytes,
		struct ena_admin_acq_entry *comp,
		size_t comp_size_in_bytes)
{
	unsigned long flags;
	struct ena_comp_ctx *comp_ctx;

	spin_lock_irqsave(&admin_queue->q_lock, flags);
	if (unlikely(!admin_queue->running_state)) {
		spin_unlock_irqrestore(&admin_queue->q_lock, flags);
		return ERR_PTR(-ENODEV);
	}
	comp_ctx = __ena_com_submit_admin_cmd(admin_queue, cmd,
					      cmd_size_in_bytes,
					      comp,
					      comp_size_in_bytes);
	spin_unlock_irqrestore(&admin_queue->q_lock, flags);

	return comp_ctx;
}

static int ena_com_init_io_sq(struct ena_com_dev *ena_dev,
			      struct ena_com_io_sq *io_sq)
{
	size_t size;

	memset(&io_sq->desc_addr, 0x0, sizeof(struct ena_com_io_desc_addr));

	size = (io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX) ?
			IO_TX_SQ_SIZE(io_sq->q_depth) :
			IO_RX_SQ_SIZE(io_sq->q_depth);

	if (io_sq->mem_queue_type == ENA_MEM_QUEUE_TYPE_HOST_MEMORY)
		io_sq->desc_addr.virt_addr =
			dma_alloc_coherent(ena_dev->dmadev,
					   size,
					   &io_sq->desc_addr.phys_addr,
					   GFP_KERNEL | __GFP_ZERO);
	else
		io_sq->desc_addr.virt_addr =
			devm_kzalloc(ena_dev->dmadev, size, GFP_KERNEL);

	if (!io_sq->desc_addr.virt_addr)
		return -ENOMEM;

	io_sq->tail = 0;
	io_sq->next_to_comp = 0;
	io_sq->phase = 1;

	io_sq->desc_entry_size =
		(io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX) ?
		sizeof(struct ena_eth_io_tx_desc) :
		sizeof(struct ena_eth_io_rx_desc);

	return 0;
}

static int ena_com_init_io_cq(struct ena_com_dev *ena_dev,
			      struct ena_com_io_cq *io_cq)
{
	size_t size;

	memset(&io_cq->cdesc_addr, 0x0, sizeof(struct ena_com_io_desc_addr));

	size = (io_cq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX) ?
			IO_TX_CQ_SIZE(io_cq->q_depth) :
			IO_RX_CQ_SIZE(io_cq->q_depth);

	io_cq->cdesc_addr.virt_addr =
		dma_alloc_coherent(ena_dev->dmadev,
				   size,
				   &io_cq->cdesc_addr.phys_addr,
				   GFP_KERNEL | __GFP_ZERO);
	if (!io_cq->cdesc_addr.virt_addr)
		return -ENOMEM;

	io_cq->phase = 1;
	io_cq->head = 0;

	/* Use the basic completion descriptor for Rx */
	io_cq->cdesc_entry_size_in_bytes =
		(io_cq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX) ?
		sizeof(struct ena_eth_io_tx_cdesc) :
		sizeof(struct ena_eth_io_rx_cdesc_base);

	return 0;
}

static void ena_com_handle_single_admin_completion(
		struct ena_com_admin_queue *admin_queue,
		struct ena_admin_acq_entry *cqe)
{
	struct ena_comp_ctx *comp_ctx;
	u16 cmd_id;

	cmd_id = cqe->acq_common_descriptor.command &
		ENA_ADMIN_ACQ_COMMON_DESC_COMMAND_ID_MASK;

	comp_ctx = get_comp_ctxt(admin_queue, cmd_id, false);
	ENA_ASSERT(comp_ctx, "null comp_ctx\n");
	comp_ctx->status = ENA_CMD_COMPLETED;
	comp_ctx->comp_status = cqe->acq_common_descriptor.status;

	if (comp_ctx->user_cqe)
		memcpy(comp_ctx->user_cqe, (void *)cqe, comp_ctx->comp_size);

	complete(&comp_ctx->wait_event);
}

static void ena_com_handle_admin_completion(
		struct ena_com_admin_queue *admin_queue)
{
	struct ena_admin_acq_entry *cqe = NULL;
	u16 comp_num = 0;
	u16 head_masked;
	u8 phase;

	head_masked = admin_queue->cq.head & (admin_queue->q_depth - 1);
	phase = admin_queue->cq.phase;

	cqe = &admin_queue->cq.entries[head_masked];

	/* Go over all the completions */
	while ((cqe->acq_common_descriptor.flags &
			ENA_ADMIN_ACQ_COMMON_DESC_PHASE_MASK) == phase) {
		ena_com_handle_single_admin_completion(admin_queue, cqe);

		head_masked++;
		comp_num++;
		if (unlikely(head_masked == admin_queue->q_depth)) {
			head_masked = 0;
			phase = !phase;
		}

		cqe = &admin_queue->cq.entries[head_masked];
	}

	admin_queue->cq.head += comp_num;
	admin_queue->cq.phase = phase;
	admin_queue->sq.head += comp_num;
}

static int ena_com_comp_status_to_errno(u8 comp_status)
{
	if (unlikely(comp_status != 0))
		ena_trc_err("admin command failed[%u]\n", comp_status);

	if (unlikely(comp_status > ena_admin_unknown_error))
		return -EINVAL;

	switch (comp_status) {
	case ena_admin_success:
		return 0;
	case ena_admin_resource_allocation_failure:
		return -ENOMEM;
	case ena_admin_bad_opcode:
	case ena_admin_unsupported_opcode:
	case ena_admin_malformed_request:
	case ena_admin_illegal_parameter:
	case ena_admin_unknown_error:
		return -EINVAL;
	}

	return 0;
}

static int ena_com_wait_and_process_admin_cq_polling(
		struct ena_comp_ctx *comp_ctx,
		struct ena_com_admin_queue *admin_queue)
{
	unsigned long flags;
	u32 timeout;
	int ret;

	timeout = ((uint32_t)jiffies_to_usecs(jiffies)) + ADMIN_CMD_TIMEOUT_US;

	while (comp_ctx->status == ENA_CMD_SUBMITTED) {
		if (((uint32_t)jiffies_to_usecs(jiffies)) > timeout) {
			/* ENA didn't have any completion */
			admin_queue->running_state = false;
			ret = -EPERM;
			goto err;
		}

		spin_lock_irqsave(&admin_queue->q_lock, flags);
		ena_com_handle_admin_completion(admin_queue);
		spin_unlock_irqrestore(&admin_queue->q_lock, flags);

		msleep(100);
	}

	if (unlikely(comp_ctx->status == ENA_CMD_ABORTED)) {
		ena_trc_err("Command was aborted\n");
		ret = -ENODEV;
		goto err;
	}

	ENA_ASSERT(comp_ctx->status == ENA_CMD_COMPLETED,
		   "Invalid comp status %d\n", comp_ctx->status);

	ret = ena_com_comp_status_to_errno(comp_ctx->comp_status);
err:

	comp_ctxt_release(admin_queue, comp_ctx);
	return ret;
}

static int ena_com_wait_and_process_admin_cq_interrupts(
		struct ena_comp_ctx *comp_ctx,
		struct ena_com_admin_queue *admin_queue)
{
	unsigned long flags;
	int ret = 0;

	wait_for_completion_timeout(&comp_ctx->wait_event,
				    usecs_to_jiffies(ADMIN_CMD_TIMEOUT_US));

	/* We have here 3 scenarios.
	 * 1) Timeout expired but nothing happened
	 * 2) The command was completed but we didn't get the MSI-X interrupt
	 * 3) The command completion and MSI-X were received successfully.
	 */
	if (unlikely(comp_ctx->status == ENA_CMD_SUBMITTED)) {
		spin_lock_irqsave(&admin_queue->q_lock, flags);
		ena_com_handle_admin_completion(admin_queue);
		spin_unlock_irqrestore(&admin_queue->q_lock, flags);

		if (comp_ctx->status == ENA_CMD_COMPLETED)
			ena_trc_err("The ena device have completion but the driver didn't receive any MSI-X interrupt (cmd %d)\n",
				    comp_ctx->cmd_opcode);
		else
			ena_trc_err("The ena device doensn't send any completion for the cmd (cmd %d)\n",
				    comp_ctx->cmd_opcode);

		admin_queue->running_state = false;
		ret = -EPERM;
		goto err;
	}

	ret = ena_com_comp_status_to_errno(comp_ctx->comp_status);
err:
	comp_ctxt_release(admin_queue, comp_ctx);
	return ret;
}

/* This method read the hardware device register through posting writes
 * and waiting for response
 * On timeout the function will return 0xFFFFFFFF
 */
static u32 ena_com_reg_bar_read32(struct ena_com_dev *ena_dev,
				  u16 offset)
{
	struct ena_com_mmio_read *mmio_read = &ena_dev->mmio_read;
	volatile struct ena_admin_ena_mmio_read_less_resp *read_resp =
		mmio_read->read_resp;
	u32 mmio_read_reg, ret;
	unsigned long flags;
	int i;

	if (unlikely(offset > sizeof(struct ena_regs_ena_registers))) {
		ena_trc_err("trying to read reg bar with invalid offset %x\n",
			    offset);
		return 0xFFFFFFFF;
	}

	might_sleep();

	spin_lock_irqsave(&mmio_read->lock, flags);
	mmio_read->seq_num++;

	read_resp->req_id = mmio_read->seq_num + 0xDEAD;
	mmio_read_reg = (offset << ENA_REGS_MMIO_READ_REG_OFF_SHIFT) &
			ENA_REGS_MMIO_READ_REG_OFF_MASK;
	mmio_read_reg |= mmio_read->seq_num &
			ENA_REGS_MMIO_READ_REQ_ID_MASK;

	writel(mmio_read_reg, &ena_dev->reg_bar->mmio_read);

	for (i = 0; i < ENA_REG_READ_TIMEOUT; i++) {
		if (read_resp->req_id == mmio_read->seq_num)
			break;

		udelay(1);
	}

	if (unlikely(i == ENA_REG_READ_TIMEOUT)) {
		ena_trc_err("reading reg failed for timeout. expected: req id[%hu] offset[%hu] actual: req id[%hu] offset[%hu]\n",
			    mmio_read->seq_num,
			    offset,
			    read_resp->req_id,
			    read_resp->reg_off);
		ret = 0xFFFFFFFF;
		goto err;
	}

	ENA_ASSERT(read_resp->reg_off == offset,
		   "Invalid MMIO read return value");

	ret = read_resp->reg_val;
err:
	spin_unlock_irqrestore(&mmio_read->lock, flags);

	return ret;
}

/* There are two types to wait for completion.
 * Polling mode - wait until the completion is available.
 * Async mode - wait on wait queue until the completion is ready
 * (or the timeout expired).
 * It is expected that the IRQ called ena_com_handle_admin_completion
 * to mark the completions.
 */
static int ena_com_wait_and_process_admin_cq(
		struct ena_comp_ctx *comp_ctx,
		struct ena_com_admin_queue *admin_queue)
{
	if (admin_queue->polling)
		return ena_com_wait_and_process_admin_cq_polling(comp_ctx,
			admin_queue);
	else
		return ena_com_wait_and_process_admin_cq_interrupts(comp_ctx,
			admin_queue);
}

static int ena_com_destroy_io_sq(struct ena_com_dev *ena_dev,
				 struct ena_com_io_sq *io_sq)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	struct ena_admin_aq_destroy_sq_cmd destroy_cmd;
	struct ena_admin_acq_destroy_sq_resp_desc destroy_resp;
	u8 direction;
	int ret;

	memset(&destroy_cmd, 0x0, sizeof(struct ena_admin_aq_destroy_sq_cmd));

	if (io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX)
		direction = ena_admin_sq_direction_tx;
	else
		direction = ena_admin_sq_direction_rx;

	destroy_cmd.sq.sq_identity |= (direction <<
		ENA_ADMIN_SQ_SQ_DIRECTION_SHIFT) &
		ENA_ADMIN_SQ_SQ_DIRECTION_MASK;

	destroy_cmd.sq.sq_idx = io_sq->idx;
	destroy_cmd.aq_common_descriptor.opcode = ena_admin_destroy_sq;

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)&destroy_cmd,
					    sizeof(destroy_cmd),
					    (struct ena_admin_acq_entry *)&destroy_resp,
					    sizeof(destroy_resp));

	if (unlikely(ret))
		ena_trc_err("failed to create io cq error: %d\n", ret);

	return ret;
}

static void ena_com_io_queue_free(struct ena_com_dev *ena_dev,
				  struct ena_com_io_sq *io_sq,
				  struct ena_com_io_cq *io_cq)
{
	size_t size;

	if (io_cq->cdesc_addr.virt_addr) {
		size = (io_cq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX) ?
				IO_TX_CQ_SIZE(io_cq->q_depth) :
				IO_RX_CQ_SIZE(io_cq->q_depth);

		dma_free_coherent(ena_dev->dmadev,
				  size,
				  io_cq->cdesc_addr.virt_addr,
				  io_cq->cdesc_addr.phys_addr);
		io_cq->cdesc_addr.virt_addr = NULL;
	}

	if (io_sq->desc_addr.virt_addr) {
		size = (io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX) ?
				IO_TX_SQ_SIZE(io_sq->q_depth) :
				IO_RX_SQ_SIZE(io_sq->q_depth);

		if (io_sq->mem_queue_type == ENA_MEM_QUEUE_TYPE_HOST_MEMORY)
			dma_free_coherent(ena_dev->dmadev,
					  size,
					  io_sq->desc_addr.virt_addr,
					  io_sq->desc_addr.phys_addr);
		else
			devm_kfree(ena_dev->dmadev, io_sq->desc_addr.virt_addr);

		io_sq->desc_addr.virt_addr = NULL;
	}
}

static int wait_for_reset_state(struct ena_com_dev *ena_dev,
				u32 timeout, u16 exp_state)
{
	u32 val, i;

	for (i = 0; i < timeout; i++) {
		val = ena_com_reg_bar_read32(ena_dev, ENA_REGS_DEV_STS_OFF);

		if (unlikely(val == 0xFFFFFFFF)) {
			ena_trc_err("Reg read timeout occur\n");
			return -ETIME;
		}

		if ((val & ENA_REGS_DEV_STS_RESET_IN_PROGRESS_MASK) ==
			exp_state)
			return 0;

		/* The resolution of the timeout is 100ms */
		msleep(100);
	}

	return -ETIME;
}

static int ena_com_get_feature(struct ena_com_dev *ena_dev,
			       struct ena_admin_get_feat_resp *get_resp,
			       enum ena_admin_aq_feature_id feature_id)
{
	struct ena_com_admin_queue *admin_queue;
	struct ena_admin_get_feat_cmd get_cmd;
	int ret;

	if (!ena_dev) {
		ena_trc_err("%s : ena_dev is NULL\n", __func__);
		return -ENODEV;
	}
	memset(&get_cmd, 0x0, sizeof(get_cmd));
	admin_queue = &ena_dev->admin_queue;

	get_cmd.aq_common_descriptor.opcode = ena_admin_get_feature;
	get_cmd.aq_common_descriptor.flags = 0;

	get_cmd.feat_common.feature_id = feature_id;

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)
					    &get_cmd,
					    sizeof(get_cmd),
					    (struct ena_admin_acq_entry *)
					    get_resp,
					    sizeof(*get_resp));

	if (unlikely(ret))
		ena_trc_err("Failed to get feature. error: %d\n", ret);

	return ret;
}

/*****************************************************************************/
/*******************************      API       ******************************/
/*****************************************************************************/

int ena_com_execute_admin_command(struct ena_com_admin_queue *admin_queue,
				  struct ena_admin_aq_entry *cmd,
				  size_t cmd_size,
				  struct ena_admin_acq_entry *comp,
				  size_t comp_size)
{
	struct ena_comp_ctx *comp_ctx;
	int ret = 0;

	comp_ctx = ena_com_submit_admin_cmd(admin_queue, cmd, cmd_size,
					    comp, comp_size);
	if (unlikely(IS_ERR(comp_ctx))) {
		ena_trc_err("Failed to submit command [%ld]\n",
			    PTR_ERR(comp_ctx));
		return PTR_ERR(comp_ctx);
	}

	ret = ena_com_wait_and_process_admin_cq(comp_ctx, admin_queue);
	if (unlikely(ret))
		ena_trc_err("Failed to process command. ret = %d\n", ret);

	return ret;
}

int ena_com_create_io_sq(struct ena_com_dev *ena_dev,
			 struct ena_com_io_sq *io_sq, u16 cq_idx)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	struct ena_admin_aq_create_sq_cmd create_cmd;
	struct ena_admin_acq_create_sq_resp_desc cmd_completion;
	u8 direction, policy;
	int ret;

	memset(&create_cmd, 0x0, sizeof(struct ena_admin_aq_create_sq_cmd));

	create_cmd.aq_common_descriptor.opcode = ena_admin_create_sq;
	create_cmd.aq_common_descriptor.flags = 0;

	create_cmd.sq_identity = 0;
	create_cmd.sq_identity |= ena_admin_eth &
		ENA_ADMIN_AQ_CREATE_SQ_CMD_SQ_TYPE_MASK;

	if (io_sq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX)
		direction = ena_admin_sq_direction_tx;
	else
		direction = ena_admin_sq_direction_rx;

	create_cmd.sq_identity |= (direction <<
		ENA_ADMIN_AQ_CREATE_SQ_CMD_SQ_DIRECTION_SHIFT) &
		ENA_ADMIN_AQ_CREATE_SQ_CMD_SQ_DIRECTION_MASK;

	switch (io_sq->mem_queue_type) {
	case ENA_MEM_QUEUE_TYPE_DEVICE_MEMORY:
		policy = ena_admin_placement_policy_dev;
		break;
	case ENA_MEM_QUEUE_TYPE_MIXED_MEMORY:
		ena_trc_err("Mixed mode placement policy is currently unsupported");
		return -EINVAL;
	case ENA_MEM_QUEUE_TYPE_HOST_MEMORY:
		policy = ena_admin_placement_policy_host;
		break;
	default:
		ena_trc_err("Invalid placement policy %u\n",
			    io_sq->mem_queue_type);
		return -EINVAL;
	}

	create_cmd.sq_caps_2 |= policy &
		ENA_ADMIN_AQ_CREATE_SQ_CMD_PLACEMENT_POLICY_MASK;

	create_cmd.sq_caps_2 |= (ena_admin_completion_policy_desc <<
		ENA_ADMIN_AQ_CREATE_SQ_CMD_COMPLETION_POLICY_SHIFT) &
		ENA_ADMIN_AQ_CREATE_SQ_CMD_COMPLETION_POLICY_MASK;

	create_cmd.sq_caps_3 |=
		ENA_ADMIN_AQ_CREATE_SQ_CMD_IS_PHYSICALLY_CONTIGUOUS_MASK;

	create_cmd.cq_idx = cq_idx;
	create_cmd.sq_depth = io_sq->q_depth;

	if (io_sq->mem_queue_type == ENA_MEM_QUEUE_TYPE_HOST_MEMORY)
		ena_com_mem_addr_set(&create_cmd.sq_ba,
				     io_sq->desc_addr.phys_addr);

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)&create_cmd,
					    sizeof(create_cmd),
					    (struct ena_admin_acq_entry *)&cmd_completion,
					    sizeof(cmd_completion));
	if (unlikely(ret)) {
		ena_trc_err("Failed to create IO SQ. error: %d\n", ret);
		return ret;
	}

	io_sq->idx = cmd_completion.sq_idx;
	io_sq->q_depth = cmd_completion.sq_actual_depth;

	if (io_sq->q_depth != cmd_completion.sq_actual_depth) {
		ena_trc_err("sq depth mismatch: requested[%u], result[%u]\n",
			    io_sq->q_depth,
			    cmd_completion.sq_actual_depth);
		return -ENOSPC;
	}

	io_sq->db_addr = (u32 *)((u8 *)ena_dev->reg_bar +
		cmd_completion.sq_doorbell_offset);

	if (io_sq->mem_queue_type == ENA_MEM_QUEUE_TYPE_DEVICE_MEMORY)
		io_sq->header_addr = (u8 *)((u8 *)ena_dev->mem_bar +
				cmd_completion.llq_headers_offset);

	if (io_sq->mem_queue_type != ENA_MEM_QUEUE_TYPE_HOST_MEMORY)
		io_sq->desc_addr.pbuf_dev_addr =
			(u8 *)((u8 *)ena_dev->mem_bar +
			cmd_completion.llq_descriptors_offset);

	ena_trc_dbg("created sq[%u], depth[%u]\n", io_sq->idx, io_sq->q_depth);

	return ret;
}

int ena_com_create_io_cq(struct ena_com_dev *ena_dev,
			 struct ena_com_io_cq *io_cq)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	struct ena_admin_aq_create_cq_cmd create_cmd;
	struct ena_admin_acq_create_cq_resp_desc cmd_completion;
	int ret;

	memset(&create_cmd, 0x0, sizeof(struct ena_admin_aq_create_cq_cmd));

	create_cmd.aq_common_descriptor.opcode = ena_admin_create_cq;

	create_cmd.cq_caps_1 |= ena_admin_eth &
		ENA_ADMIN_AQ_CREATE_CQ_CMD_CQ_TYPE_MASK;
	create_cmd.cq_caps_2 |= (io_cq->cdesc_entry_size_in_bytes / 4) &
		ENA_ADMIN_AQ_CREATE_CQ_CMD_CQ_ENTRY_SIZE_WORDS_MASK;
	create_cmd.cq_caps_1 |=
		ENA_ADMIN_AQ_CREATE_CQ_CMD_INTERRUPT_MODE_ENABLED_MASK;

	create_cmd.msix_vector = io_cq->msix_vector;
	create_cmd.cq_depth = io_cq->q_depth;

	ena_com_mem_addr_set(&create_cmd.cq_ba, io_cq->cdesc_addr.phys_addr);

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)&create_cmd,
					    sizeof(create_cmd),
					    (struct ena_admin_acq_entry *)&cmd_completion,
					    sizeof(cmd_completion));
	if (unlikely(ret)) {
		ena_trc_err("Failed to create IO CQ. error: %d\n", ret);
		return ret;
	}

	io_cq->idx = cmd_completion.cq_idx;
	io_cq->db_addr = (u32 *)((u8 *)ena_dev->reg_bar +
		cmd_completion.cq_doorbell_offset);

	if (io_cq->q_depth != cmd_completion.cq_actual_depth) {
		ena_trc_err("completion actual queue size (%d) is differ from requested size (%d)\n",
			    cmd_completion.cq_actual_depth, io_cq->q_depth);
		return -ENOSPC;
	}

	io_cq->unmask_reg = (u32 *)((u8 *)ena_dev->reg_bar +
		cmd_completion.cq_interrupt_unmask_register);
	io_cq->unmask_val = cmd_completion.cq_interrupt_unmask_value;

	ena_trc_dbg("created cq[%u], depth[%u]\n", io_cq->idx, io_cq->q_depth);

	return ret;
}

int ena_com_get_io_handlers(struct ena_com_dev *ena_dev, u16 qid,
			    struct ena_com_io_sq **io_sq,
			    struct ena_com_io_cq **io_cq)
{
	if (qid >= ENA_TOTAL_NUM_QUEUES) {
		ena_trc_err("Invalid queue number %d but the max is %d\n",
			    qid, ENA_TOTAL_NUM_QUEUES);
		return -EINVAL;
	}

	*io_sq = &ena_dev->io_sq_queues[qid];
	*io_cq = &ena_dev->io_cq_queues[qid];

	return 0;
}

void ena_com_abort_admin_commands(struct ena_com_dev *ena_dev)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	struct ena_comp_ctx *comp_ctx;
	u16 i;

	for (i = 0; i < admin_queue->q_depth; i++) {
		comp_ctx = get_comp_ctxt(admin_queue, i, false);
		comp_ctx->status = ENA_CMD_ABORTED;

		complete(&comp_ctx->wait_event);
	}
}

void ena_com_wait_for_abort_completion(struct ena_com_dev *ena_dev)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	unsigned long flags;

	spin_lock_irqsave(&admin_queue->q_lock, flags);
	while (atomic_read(&admin_queue->outstanding_cmds) != 0) {
		spin_unlock_irqrestore(&admin_queue->q_lock, flags);
		msleep(20);
		spin_lock_irqsave(&admin_queue->q_lock, flags);
	}
	spin_unlock_irqrestore(&admin_queue->q_lock, flags);
}

bool ena_get_admin_running_state(struct ena_com_dev *ena_dev)
{
	return ena_dev->admin_queue.running_state;
}

int ena_com_destroy_io_cq(struct ena_com_dev *ena_dev,
			  struct ena_com_io_cq *io_cq)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	struct ena_admin_aq_destroy_cq_cmd destroy_cmd;
	struct ena_admin_acq_destroy_cq_resp_desc destroy_resp;
	int ret;

	memset(&destroy_cmd, 0x0, sizeof(struct ena_admin_aq_destroy_sq_cmd));

	destroy_cmd.cq_idx = io_cq->idx;
	destroy_cmd.aq_common_descriptor.opcode = ena_admin_destroy_cq;

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)&destroy_cmd,
					    sizeof(destroy_cmd),
					    (struct ena_admin_acq_entry *)&destroy_resp,
					    sizeof(destroy_resp));

	if (unlikely(ret))
		ena_trc_err("Failed to destroy IO CQ. error: %d\n", ret);

	return ret;
}

void ena_com_set_admin_running_state(struct ena_com_dev *ena_dev, bool state)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	unsigned long flags;

	spin_lock_irqsave(&admin_queue->q_lock, flags);
	ena_dev->admin_queue.running_state = state;
	spin_unlock_irqrestore(&admin_queue->q_lock, flags);
}

void ena_com_admin_aenq_enable(struct ena_com_dev *ena_dev)
{
	ENA_ASSERT(ena_dev->aenq.head == ena_dev->aenq.q_depth,
		   "Invliad AENQ state\n");

	/* Init head_db to mark that all entries in the queue
	 * are initially available
	 */
	writel(ena_dev->aenq.q_depth, &ena_dev->reg_bar->aenq_head_db);
}

int ena_com_get_dma_width(struct ena_com_dev *ena_dev)
{
	u32 caps = ena_com_reg_bar_read32(ena_dev, ENA_REGS_CAPS_OFF);
	int width;

	if (unlikely(caps == 0xFFFFFFFF)) {
		ena_trc_err("Reg read timeout occur\n");
		return -ETIME;
	}

	width = (caps & ENA_REGS_CAPS_DMA_ADDR_WIDTH_MASK) >>
		ENA_REGS_CAPS_DMA_ADDR_WIDTH_SHIFT;

	ena_trc_dbg("ENA dma width: %d\n", width);

	ENA_ASSERT(width > 0, "Invalid dma width: %d\n", width);

	return width;
}

int ena_com_validate_version(struct ena_com_dev *ena_dev)
{
	u32 ver;
	u32 ctrl_ver;
	u32 ctrl_ver_masked;
	/* Make sure the ENA version and the controller version are at least
	 * as the driver expects
	 */

	ver = ena_com_reg_bar_read32(ena_dev, ENA_REGS_VERSION_OFF);
	ctrl_ver = ena_com_reg_bar_read32(ena_dev,
					  ENA_REGS_CONTROLLER_VERSION_OFF);

	if (unlikely((ver == 0xFFFFFFFF) || (ctrl_ver == 0xFFFFFFFF))) {
		ena_trc_err("Reg read timeout occur\n");
		return -ETIME;
	}

	ena_trc_info("ena device version: %d.%d\n",
		     (ver & ENA_REGS_VERSION_MAJOR_VERSION_MASK) >>
		     ENA_REGS_VERSION_MAJOR_VERSION_SHIFT,
		     ver & ENA_REGS_VERSION_MINOR_VERSION_MASK);

	if (ver < MIN_ENA_VER) {
		ena_trc_err("ENA version is lower than the minimal version the driver supports\n");
		return -1;
	}

	ena_trc_info("ena controller version: %d.%d.%d implementation version %d\n",
		     (ctrl_ver & ENA_REGS_CONTROLLER_VERSION_MAJOR_VERSION_MASK)
		     >> ENA_REGS_CONTROLLER_VERSION_MAJOR_VERSION_SHIFT,
		     (ctrl_ver & ENA_REGS_CONTROLLER_VERSION_MINOR_VERSION_MASK)
		     >> ENA_REGS_CONTROLLER_VERSION_MINOR_VERSION_SHIFT,
		     (ctrl_ver & ENA_REGS_CONTROLLER_VERSION_SUBMINOR_VERSION_MASK),
		     (ctrl_ver & ENA_REGS_CONTROLLER_VERSION_IMPL_ID_MASK) >>
		     ENA_REGS_CONTROLLER_VERSION_IMPL_ID_SHIFT);

	ctrl_ver_masked =
		(ctrl_ver & ENA_REGS_CONTROLLER_VERSION_MAJOR_VERSION_MASK) |
		(ctrl_ver & ENA_REGS_CONTROLLER_VERSION_MINOR_VERSION_MASK) |
		(ctrl_ver & ENA_REGS_CONTROLLER_VERSION_SUBMINOR_VERSION_MASK);

	/* Validate the ctrl version without the implementation ID */
	if (ctrl_ver_masked < MIN_ENA_CTRL_VER) {
		ena_trc_err("ENA ctrl version is lower than the minimal ctrl version the driver supports\n");
		return -1;
	}

	return 0;
}

void ena_com_admin_destroy(struct ena_com_dev *ena_dev)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;

	if (!admin_queue)
		return;

	if (admin_queue->comp_ctx)
		devm_kfree(ena_dev->dmadev, admin_queue->comp_ctx);
	admin_queue->comp_ctx = NULL;

	if (admin_queue->sq.entries)
		dma_free_coherent(ena_dev->dmadev,
				  ADMIN_SQ_SIZE(admin_queue->q_depth),
				  admin_queue->sq.entries,
				  admin_queue->sq.dma_addr);
	admin_queue->sq.entries = NULL;

	if (admin_queue->cq.entries)
		dma_free_coherent(ena_dev->dmadev,
				  ADMIN_CQ_SIZE(admin_queue->q_depth),
				  admin_queue->cq.entries,
				  admin_queue->cq.dma_addr);
	admin_queue->cq.entries = NULL;

	if (ena_dev->aenq.entries)
		dma_free_coherent(ena_dev->dmadev,
				  ADMIN_AENQ_SIZE(ena_dev->aenq.q_depth),
				  ena_dev->aenq.entries,
				  ena_dev->aenq.dma_addr);
	ena_dev->aenq.entries = NULL;
}

void ena_com_set_admin_polling_mode(struct ena_com_dev *ena_dev, bool polling)
{
	ena_dev->admin_queue.polling = polling;
}

bool ena_com_get_admin_polling_mode(struct ena_com_dev *ena_dev)
{
	return ena_dev->admin_queue.polling;
}

int ena_com_mmio_reg_read_request_init(struct ena_com_dev *ena_dev)
{
	struct ena_com_mmio_read *mmio_read = &ena_dev->mmio_read;

	spin_lock_init(&mmio_read->lock);
	mmio_read->read_resp =
		dma_alloc_coherent(ena_dev->dmadev,
				   sizeof(*mmio_read->read_resp),
				   &mmio_read->read_resp_dma_addr,
				   GFP_KERNEL | __GFP_ZERO);
	if (unlikely(!mmio_read->read_resp))
		return -ENOMEM;

	ena_com_mmio_reg_read_request_write_dev_addr(ena_dev);

	mmio_read->read_resp->req_id = 0x0;
	mmio_read->seq_num = 0x0;

	return 0;
}

void ena_com_mmio_reg_read_request_destroy(struct ena_com_dev *ena_dev)
{
	struct ena_com_mmio_read *mmio_read = &ena_dev->mmio_read;

	ena_dev->reg_bar->mmio_resp_lo = ENA_DMA_ADDR_TO_UINT32_LOW(0x0);
	ena_dev->reg_bar->mmio_resp_hi = ENA_DMA_ADDR_TO_UINT32_HIGH(0x0);

	dma_free_coherent(ena_dev->dmadev,
			  sizeof(*mmio_read->read_resp),
			  mmio_read->read_resp,
			  mmio_read->read_resp_dma_addr);

	mmio_read->read_resp = NULL;
}

void ena_com_mmio_reg_read_request_write_dev_addr(struct ena_com_dev *ena_dev)
{
	struct ena_com_mmio_read *mmio_read = &ena_dev->mmio_read;

	ena_dev->reg_bar->mmio_resp_lo =
		ENA_DMA_ADDR_TO_UINT32_LOW(mmio_read->read_resp_dma_addr);
	ena_dev->reg_bar->mmio_resp_hi =
		ENA_DMA_ADDR_TO_UINT32_HIGH(mmio_read->read_resp_dma_addr);
}

int ena_com_admin_init(struct ena_com_dev *ena_dev,
		       struct ena_aenq_handlers *aenq_handlers,
		       bool init_spinlock)
{
	struct ena_com_admin_queue *admin_queue = &ena_dev->admin_queue;
	u32 aq_caps, acq_caps, dev_sts;
	int ret;

	ena_trc_info("ena_defs : Version:[%s] Build date [%s]",
		     ENA_GEN_COMMIT, ENA_GEN_DATE);

	dev_sts = ena_com_reg_bar_read32(ena_dev, ENA_REGS_DEV_STS_OFF);

	if (unlikely(dev_sts == 0xFFFFFFFF)) {
		ena_trc_err("Reg read timeout occur\n");
		return -ETIME;
	}

	if (!(dev_sts & ENA_REGS_DEV_STS_READY_MASK)) {
		ena_trc_err("Device isn't ready, abort com init\n");
		return -1;
	}

	admin_queue->q_depth = ENA_ADMIN_QUEUE_DEPTH;

	admin_queue->q_dmadev = ena_dev->dmadev;
	admin_queue->polling = false;
	admin_queue->curr_cmd_id = 0;

	atomic_set(&admin_queue->outstanding_cmds, 0);

	if (init_spinlock)
		spin_lock_init(&admin_queue->q_lock);

	ret = ena_com_init_comp_ctxt(admin_queue);
	if (ret)
		goto error;

	ret = ena_com_admin_init_sq(admin_queue);
	if (ret)
		goto error;

	ret = ena_com_admin_init_cq(admin_queue);
	if (ret)
		goto error;

	admin_queue->sq.db_addr = (void __iomem *)&ena_dev->reg_bar->aq_db;

	ena_dev->reg_bar->aq_base_lo =
		ENA_DMA_ADDR_TO_UINT32_LOW(admin_queue->sq.dma_addr);
	ena_dev->reg_bar->aq_base_hi =
		ENA_DMA_ADDR_TO_UINT32_HIGH(admin_queue->sq.dma_addr);

	ena_dev->reg_bar->acq_base_lo =
		ENA_DMA_ADDR_TO_UINT32_LOW(admin_queue->cq.dma_addr);
	ena_dev->reg_bar->acq_base_hi =
		ENA_DMA_ADDR_TO_UINT32_HIGH(admin_queue->cq.dma_addr);

	aq_caps = 0;
	aq_caps |= admin_queue->q_depth & ENA_REGS_AQ_CAPS_AQ_DEPTH_MASK;
	aq_caps |= (sizeof(struct ena_admin_aq_entry) <<
			ENA_REGS_AQ_CAPS_AQ_ENTRY_SIZE_SHIFT) &
			ENA_REGS_AQ_CAPS_AQ_ENTRY_SIZE_MASK;

	acq_caps = 0;
	acq_caps |= admin_queue->q_depth & ENA_REGS_ACQ_CAPS_ACQ_DEPTH_MASK;
	acq_caps |= (sizeof(struct ena_admin_acq_entry) <<
		ENA_REGS_ACQ_CAPS_ACQ_ENTRY_SIZE_SHIFT) &
		ENA_REGS_ACQ_CAPS_ACQ_ENTRY_SIZE_MASK;

	writel(aq_caps, &ena_dev->reg_bar->aq_caps);
	writel(acq_caps, &ena_dev->reg_bar->acq_caps);
	ret = ena_com_admin_init_aenq(ena_dev, aenq_handlers);
	if (ret)
		goto error;

	admin_queue->running_state = true;

	return 0;
error:
	ena_com_admin_destroy(ena_dev);

	return ret;
}

int ena_com_create_io_queue(struct ena_com_dev *ena_dev,
			    u16 qid,
			    enum queue_direction direction,
			    enum ena_com_memory_queue_type mem_queue_type,
			    u32 msix_vector,
			    u16 queue_size)
{
	struct ena_com_io_sq *io_sq = &ena_dev->io_sq_queues[qid];
	struct ena_com_io_cq *io_cq = &ena_dev->io_cq_queues[qid];
	int ret = 0;

	memset(io_sq, 0x0, sizeof(struct ena_com_io_sq));
	memset(io_cq, 0x0, sizeof(struct ena_com_io_cq));

	/* Init CQ */
	io_cq->q_depth = queue_size;
	io_cq->direction = direction;
	io_cq->qid = qid;

	io_cq->msix_vector = msix_vector;

	io_sq->q_depth = queue_size;
	io_sq->direction = direction;
	io_sq->qid = qid;

	io_sq->mem_queue_type = mem_queue_type;

	ret = ena_com_init_io_sq(ena_dev, io_sq);
	if (ret)
		goto error;
	ret = ena_com_init_io_cq(ena_dev, io_cq);
	if (ret)
		goto error;

	ret = ena_com_create_io_cq(ena_dev, io_cq);
	if (ret)
		goto error;

	ret = ena_com_create_io_sq(ena_dev, io_sq, io_cq->idx);
	if (ret)
		goto error;

	return 0;
error:
	ena_com_io_queue_free(ena_dev, io_sq, io_cq);
	return ret;
}

void ena_com_destroy_io_queue(struct ena_com_dev *ena_dev, u16 qid)
{
	struct ena_com_io_sq *io_sq = &ena_dev->io_sq_queues[qid];
	struct ena_com_io_cq *io_cq = &ena_dev->io_cq_queues[qid];

	ena_com_destroy_io_sq(ena_dev, io_sq);
	ena_com_destroy_io_cq(ena_dev, io_cq);

	ena_com_io_queue_free(ena_dev, io_sq, io_cq);
}

int ena_com_get_link_params(struct ena_com_dev *ena_dev,
			    struct ena_admin_get_feat_resp *resp)
{
	return ena_com_get_feature(ena_dev, resp, ena_admin_link_config);
}

int ena_com_get_dev_attr_feat(struct ena_com_dev *ena_dev,
			      struct ena_com_dev_get_features_ctx *get_feat_ctx)
{
	struct ena_admin_get_feat_resp get_resp;
	int rc;

	rc = ena_com_get_feature(ena_dev, &get_resp,
				 ena_admin_device_attributes);
	if (rc)
		return rc;

	memcpy(&get_feat_ctx->dev_attr, &get_resp.u.dev_attr,
	       sizeof(get_resp.u.dev_attr));

	rc = ena_com_get_feature(ena_dev, &get_resp,
				 ena_admin_max_queues_num);
	if (rc)
		return rc;

	memcpy(&get_feat_ctx->max_queues, &get_resp.u.max_queue,
	       sizeof(get_resp.u.max_queue));

	rc = ena_com_get_feature(ena_dev, &get_resp,
				 ena_admin_aenq_config);
	if (rc)
		return rc;

	memcpy(&get_feat_ctx->aenq, &get_resp.u.aenq,
	       sizeof(get_resp.u.aenq));

	rc = ena_com_get_feature(ena_dev, &get_resp,
				 ena_admin_stateless_offload_config);
	if (rc)
		return rc;

	memcpy(&get_feat_ctx->offload, &get_resp.u.offload,
	       sizeof(get_resp.u.offload));

	return 0;
}

void ena_com_admin_q_comp_intr_handler(struct ena_com_dev *ena_dev)
{
	ena_com_handle_admin_completion(&ena_dev->admin_queue);
}

/* ena_handle_specific_aenq_event:
 * return the handler that is relevant to the specific event group
 */
static ena_aenq_handler ena_com_get_specific_aenq_cb(struct ena_com_dev *dev,
						     u16 group)
{
	struct ena_aenq_handlers *aenq_handlers = dev->aenq.aenq_handlers;

	if ((group < ENA_MAX_HANDLERS) && aenq_handlers->handlers[group])
		return aenq_handlers->handlers[group];

	return aenq_handlers->unimplemented_handler;
}

/* ena_aenq_intr_handler:
 * handles the aenq incoming events.
 * pop events from the queue and apply the specific handler
 */
void ena_com_aenq_intr_handler(struct ena_com_dev *dev, void *data)
{
	struct ena_admin_aenq_entry *aenq_e;
	struct ena_admin_aenq_common_desc *aenq_common;
	struct ena_com_aenq *aenq  = &dev->aenq;
	ena_aenq_handler handler_cb;
	u16 masked_head, processed = 0;
	u8 phase;

	masked_head = aenq->head & (aenq->q_depth - 1);
	phase = aenq->phase;
	aenq_e = &aenq->entries[masked_head]; /* Get first entry */
	aenq_common = &aenq_e->aenq_common_desc;

	/* Go over all the events */
	while ((aenq_common->flags & ENA_ADMIN_AENQ_COMMON_DESC_PHASE_MASK) ==
		phase) {
		ena_trc_dbg("AENQ! Group[%x] Syndrom[%x] timestamp: [%llus]\n",
			    aenq_common->group,
			    aenq_common->syndrom,
			    (u64)aenq_common->timestamp_low +
			    ((u64)aenq_common->timestamp_high << 32));

		/* Handle specific event*/
		handler_cb = ena_com_get_specific_aenq_cb(dev,
							  aenq_common->group);
		handler_cb(data, aenq_e); /* call the actual event handler*/

		/* Get next event entry */
		masked_head++;
		processed++;

		if (unlikely(masked_head == aenq->q_depth)) {
			masked_head = 0;
			phase = !phase;
		}
		aenq_e = &aenq->entries[masked_head];
		aenq_common = &aenq_e->aenq_common_desc;
	}

	aenq->head += processed;
	aenq->phase = phase;
	/* update ena-device for the last processed event */
	if (processed)
		writel((u32)aenq->head, &dev->reg_bar->aenq_head_db);
}

int ena_com_dev_reset(struct ena_com_dev *ena_dev)
{
	u32 stat, timeout, cap;
	int rc;

	stat = ena_com_reg_bar_read32(ena_dev, ENA_REGS_DEV_STS_OFF);
	cap = ena_com_reg_bar_read32(ena_dev, ENA_REGS_CAPS_OFF);

	if (unlikely((stat == 0xFFFFFFFF) || (cap == 0xFFFFFFFF))) {
		ena_trc_err("Reg read32 timeout occur\n");
		return -ETIME;
	}

	if ((stat & ENA_REGS_DEV_STS_READY_MASK) == 0) {
		ena_trc_err("Device isn't ready, can't reset device\n");
		return -EINVAL;
	}

	timeout = (cap & ENA_REGS_CAPS_RESET_TIMEOUT_MASK) >>
			ENA_REGS_CAPS_RESET_TIMEOUT_SHIFT;
	if (timeout == 0) {
		ena_trc_err("Invalid timeout value\n");
		return -EINVAL;
	}

	/* If the register read failed */
	if (unlikely((stat == 0xFFFFFFFF) || (cap == 0xFFFFFFFF)))
		return -ETIME;

	/* start reset */
	writel(ENA_REGS_DEV_CTL_DEV_RESET_MASK, &ena_dev->reg_bar->dev_ctl);

	/* Write again the MMIO read request address */
	ena_com_mmio_reg_read_request_write_dev_addr(ena_dev);

	rc = wait_for_reset_state(ena_dev, timeout,
				  ENA_REGS_DEV_STS_RESET_IN_PROGRESS_MASK);
	if (rc != 0) {
		ena_trc_err("Reset indication didn't turn on\n");
		return rc;
	}

	/* reset done */
	writel(0, &ena_dev->reg_bar->dev_ctl);
	rc = wait_for_reset_state(ena_dev, timeout, 0);
	if (rc != 0) {
		ena_trc_err("Reset indication didn't turn off\n");
		return rc;
	}

	return 0;
}

static int ena_get_dev_stats(struct ena_com_dev *ena_dev,
			     struct ena_admin_aq_get_stats_cmd *get_cmd,
			     struct ena_admin_acq_get_stats_resp *get_resp,
			     enum ena_admin_get_stats_type type)
{
	struct ena_com_admin_queue *admin_queue;
	int ret = 0;

	if (!ena_dev) {
		ena_trc_err("%s : ena_dev is NULL\n", __func__);
		return -ENODEV;
	}

	admin_queue = &ena_dev->admin_queue;

	get_cmd->aq_common_descriptor.opcode = ena_admin_get_stats;
	get_cmd->aq_common_descriptor.flags = 0;
	get_cmd->type = type;

	ret =  ena_com_execute_admin_command(admin_queue,
					     (struct ena_admin_aq_entry *)get_cmd,
					     sizeof(*get_cmd),
					     (struct ena_admin_acq_entry *)get_resp,
					     sizeof(*get_resp));

	if (unlikely(ret))
		ena_trc_err("Failed to get stats. error: %d\n", ret);

	return ret;
}

int ena_com_get_dev_basic_stats(struct ena_com_dev *ena_dev,
				struct ena_admin_basic_stats *stats)
{
	int ret = 0;
	struct ena_admin_aq_get_stats_cmd get_cmd;
	struct ena_admin_acq_get_stats_resp get_resp;

	memset(&get_cmd, 0x0, sizeof(get_cmd));
	ret = ena_get_dev_stats(ena_dev, &get_cmd, &get_resp,
				ena_admin_get_stats_type_basic);
	if (likely(ret == 0))
		memcpy(stats, &get_resp.basic_stats,
		       sizeof(get_resp.basic_stats));

	return ret;
}

int ena_com_get_dev_extended_stats(struct ena_com_dev *ena_dev, char *buff,
				   u32 len)
{
	int ret = 0;
	struct ena_admin_aq_get_stats_cmd get_cmd;
	struct ena_admin_acq_get_stats_resp get_resp;
	u8 __iomem *virt_addr;
	dma_addr_t phys_addr;

	virt_addr = dma_alloc_coherent(ena_dev->dmadev,
				       len,
				       &phys_addr,
				       GFP_KERNEL | __GFP_ZERO);
	if (!virt_addr) {
		ret = -ENOMEM;
		goto done;
	}
	memset(&get_cmd, 0x0, sizeof(get_cmd));
	ena_com_mem_addr_set(&get_cmd.u.control_buffer.address, phys_addr);
	get_cmd.u.control_buffer.length = len;

	get_cmd.device_id = ena_dev->stats_func;
	get_cmd.queue_idx = ena_dev->stats_queue;

	ret = ena_get_dev_stats(ena_dev, &get_cmd, &get_resp,
				ena_admin_get_stats_type_extended);
	if (ret < 0)
		goto free_ext_stats_mem;

	ret = snprintf(buff, len, "%s", (char *)virt_addr);

free_ext_stats_mem:
	dma_free_coherent(ena_dev->dmadev, len, virt_addr, phys_addr);
done:
	return ret;
}

int ena_com_set_dev_mtu(struct ena_com_dev *ena_dev, int mtu)
{
	struct ena_com_admin_queue *admin_queue;
	struct ena_admin_set_feat_cmd cmd;
	struct ena_admin_set_feat_resp resp;
	int ret = 0;

	if (unlikely(!ena_dev)) {
		ena_trc_err("%s : ena_dev is NULL\n", __func__);
		return -ENODEV;
	}

	memset(&cmd, 0x0, sizeof(cmd));
	admin_queue = &ena_dev->admin_queue;

	cmd.aq_common_descriptor.opcode = ena_admin_set_feature;
	cmd.aq_common_descriptor.flags = 0;
	cmd.feat_common.feature_id = ena_admin_mtu;
	cmd.u.mtu.mtu = mtu;

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)&cmd,
					    sizeof(cmd),
					    (struct ena_admin_acq_entry *)&resp,
					    sizeof(resp));

	if (unlikely(ret)) {
		ena_trc_err("Failed to set mtu %d. error: %d\n", mtu, ret);
		return -EINVAL;
	}
	return 0;
}

int ena_com_set_interrupt_moderation(struct ena_com_dev *ena_dev, int qid,
				     bool enable, u16 count, u16 interval)
{
	struct ena_com_io_cq *io_cq = &ena_dev->io_cq_queues[qid];

	struct ena_com_admin_queue *admin_queue;
	struct ena_admin_set_feat_cmd cmd;
	struct ena_admin_set_feat_resp resp;
	u8 direction;
	int ret = 0;

	if (unlikely(!ena_dev)) {
		ena_trc_err("%s : ena_dev is NULL\n", __func__);
		return -ENODEV;
	}

	memset(&cmd, 0x0, sizeof(cmd));
	admin_queue = &ena_dev->admin_queue;

	cmd.aq_common_descriptor.opcode = ena_admin_set_feature;
	cmd.aq_common_descriptor.flags = 0;
	cmd.feat_common.feature_id = ena_admin_interrupt_moderation;
	cmd.u.intr_moder.cq_idx = io_cq->idx;
	if (io_cq->direction == ENA_COM_IO_QUEUE_DIRECTION_TX)
		direction = ena_admin_sq_direction_tx;
	else
		direction = ena_admin_sq_direction_rx;

	cmd.u.intr_moder.queue_identity |= direction &
		ENA_ADMIN_SET_FEATURE_INTR_MODER_DESC_SQ_DIRECTION_MASK;

	if (enable) {
		cmd.u.intr_moder.flags |=
			ENA_ADMIN_SET_FEATURE_INTR_MODER_DESC_ENABLE_MASK;
		cmd.u.intr_moder.intr_moder_metrics.count = count;
		cmd.u.intr_moder.intr_moder_metrics.interval = interval;
	}

	ret = ena_com_execute_admin_command(admin_queue,
					    (struct ena_admin_aq_entry *)&cmd,
					    sizeof(cmd),
					    (struct ena_admin_acq_entry *)&resp,
					    sizeof(resp));

	if (unlikely(ret)) {
		ena_trc_err("Failed to set interrupt moderation %d\n", ret);
		return -EINVAL;
	}
	return 0;
}

int ena_com_get_offload_settings(struct ena_com_dev *ena_dev,
				 struct ena_admin_feature_offload_desc *offload)
{
	int ret;
	struct ena_admin_get_feat_resp resp;

	ret = ena_com_get_feature(ena_dev, &resp,
				  ena_admin_stateless_offload_config);
	if (unlikely(ret)) {
		ena_trc_err("Failed to get offload capabilities %d\n", ret);
		return -EINVAL;
	}

	memcpy(offload, &resp.u.offload, sizeof(resp.u.offload));

	return 0;
}
