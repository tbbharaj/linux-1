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
#ifndef _ENA_REGS_H_
#define _ENA_REGS_H_

/* ENA Global Registers, for the entire PCIe function */
struct ena_regs_ena_registers {
	/* word 0 : */
	/* ENA specification version [RO]
	 * 7:0 : minor_version - Minor version
	 * 15:8 : major_version - Major version.
	 * 31:16 : reserved16
	 */
	u32 version;

	/* word 1 : */
	/* ENA controller version [RO]
	 * 7:0 : subminor_version - Sub Minor version
	 * 15:8 : minor_version - Minor version
	 * 23:16 : major_version - Major version.
	 * 31:24 : impl_id - implementation
	 */
	u32 controller_version;

	/* word 2 : */
	/* capabilities register [RO]
	 * 0 : contiguous_queue_required - If set, requires
	 *    that each queue ring occupies a contiguous
	 *    physical memory space.
	 * 5:1 : reset_timeout - Max amount of time the
	 *    driver should wait for ENA Reset to finish in
	 *    resultion of 100ms.
	 * 7:6 : reserved6
	 * 15:8 : dma_addr_width - DMA address width. Number
	 *    of bits of the DMA address supported by the
	 *    Controller.
	 * 31:16 : reserved16
	 */
	u32 caps;

	/* word 3 : capabilities extended register [RO] */
	u32 caps_ext;

	/* word 4 : admin queue base address bits [31:0] [WO] */
	u32 aq_base_lo;

	/* word 5 : admin queue base address bits [63:32] [WO] */
	u32 aq_base_hi;

	/* word 6 : */
	/* admin queue capabilities register [WO]
	 * 15:0 : aq_depth - admin queue depth in entries.
	 *    must be power of 2
	 * 31:16 : aq_entry_size - admin queue entry size in
	 *    32-bit words
	 */
	u32 aq_caps;

	/* word 7 :  */
	u32 reserved;

	/* word 8 : admin completion queue base address bits [31:0]. [WO] */
	u32 acq_base_lo;

	/* word 9 : admin completion queue base address bits [63:32]. [WO] */
	u32 acq_base_hi;

	/* word 10 : */
	/* admin completion queue capabilities register [WO]
	 * 15:0 : acq_depth - admin completion queue depth in
	 *    entries
	 * 31:16 : acq_entry_size - admin completion queue
	 *    entry size in 32-bit words
	 */
	u32 acq_caps;

	/* word 11 : AQ Doorbell. incremented by number of new added
	 * entries, written value should wrap-around on 2^16 [WO]
	 */
	u32 aq_db;

	/* word 12 : ACQ tail pointer, indicates where new completions will
	 * be placed [RO]
	 */
	u32 acq_tail;

	/* word 13 : */
	/* Asynchronous Event Notification Queue capabilities register [WO]
	 * 15:0 : aenq_depth - queue depth in entries
	 * 31:16 : aenq_entry_size - queue entry size in
	 *    32-bit words
	 */
	u32 aenq_caps;

	/* word 14 : Asynchronous Event Notification Queue base address
	 * bits [31:0] [WO]
	 */
	u32 aenq_base_lo;

	/* word 15 : Asynchronous Event Notification Queue base address
	 * bits [63:32] [WO]
	 */
	u32 aenq_base_hi;

	/* word 16 : AENQ Head Doorbell, indicates the entries that have
	 * been processed by the host [WO]
	 */
	u32 aenq_head_db;

	/* word 17 : AENQ tail pointer, indicates where new entries will be
	 * placed [RO]
	 */
	u32 aenq_tail;

	/* word 18 :  */
	u32 reserved_48;

	/* word 19 :  [RW] */
	u32 intr_mask;

	/* word 20 :  */
	u32 reserved_50;

	/* word 21 : */
	/* Device Control Register, some of these features may not be
	 *    implemented or supported for a given client [WO]
	 * 0 : dev_reset - If set, indicates request for a
	 *    reset, this bit will only be cleared when the
	 *    reset operation finished and can not be cleared by
	 *    writing 0 to it.
	 * 1 : aq_restart - Used in case AQ is not
	 *    responsive: Once set, the it will be auto-cleared
	 *    once process is done. The status of the restart is
	 *    indicated in the status register.
	 * 2 : quiescent - If set, indicates a request for
	 *    suspending of I/O, Admin and Async Events
	 *    handling, this bit will only be cleared when the
	 *    quiescen process is finished.
	 * 3 : io_resume - If set, indicates request to
	 *    resume traffic processing.
	 * 31:4 : reserved4
	 */
	u32 dev_ctl;

	/* word 22 : */
	/* Device Status Register [RO]
	 * 0 : ready - device ready to received admin commands
	 * 1 : aq_restart_in_progress - this bit is set while
	 *    aq_restart in process
	 * 2 : aq_restart_finished - this bit is set only
	 *    after aq_restart process finished, and will be
	 *    auto-cleared one aq_restart in control register is
	 *    invoked
	 * 3 : reset_in_progress - this bit is set while ENA
	 *    reset is going
	 * 4 : reset_finished - this bit is set when ENA
	 *    reset is finished. It is auto-cleared one reset is
	 *    invoked in control register
	 * 5 : fatal_error
	 * 6 : quiescent_state_in_progress - A process to
	 *    quiescent ENA is in progress
	 * 7 : quiescent_state_achieved - This bit is set
	 *    once the quiescent state is achieved, and it is
	 *    auto-cleared once the quiescent_start
	 * 31:8 : reserved8
	 */
	u32 dev_sts;

	/* word 23 : */
	/* MMIO Read Less Register. host must initialize the mmio_resp_lo/hi
	 *    before issueing new register read request [WO]
	 * 15:0 : req_id - request id
	 * 31:16 : reg_off - register offset
	 */
	u32 mmio_reg_read;

	/* word 24 : read response address bits [31:3], bits [2:0] must set
	 * to 0 [WO]
	 */
	u32 mmio_resp_lo;

	/* word 25 : read response address bits [64:32] [WO] */
	u32 mmio_resp_hi;

	/* word 26 : */
	/* RSS Indirection table entry update register [WO]
	 * 15:0 : index - entry index
	 * 31:16 : cq_idx - cq identifier
	 */
	u32 rss_ind_entry_update;
};

/* admin interrupt register */
#define ENA_REGS_ADMIN_INTERRUPT_ACQ	0x1 /* Admin Completion queue */
#define ENA_REGS_ADMIN_INTERRUPT_AENQ	0x2 /* Async Event Notification Queue */

/* ena_registers offsets */
#define ENA_REGS_VERSION_OFF		0x0
#define ENA_REGS_CONTROLLER_VERSION_OFF		0x4
#define ENA_REGS_CAPS_OFF		0x8
#define ENA_REGS_CAPS_EXT_OFF		0xc
#define ENA_REGS_AQ_BASE_LO_OFF		0x10
#define ENA_REGS_AQ_BASE_HI_OFF		0x14
#define ENA_REGS_AQ_CAPS_OFF		0x18
#define ENA_REGS_ACQ_BASE_LO_OFF		0x20
#define ENA_REGS_ACQ_BASE_HI_OFF		0x24
#define ENA_REGS_ACQ_CAPS_OFF		0x28
#define ENA_REGS_AQ_DB_OFF		0x2c
#define ENA_REGS_ACQ_TAIL_OFF		0x30
#define ENA_REGS_AENQ_CAPS_OFF		0x34
#define ENA_REGS_AENQ_BASE_LO_OFF		0x38
#define ENA_REGS_AENQ_BASE_HI_OFF		0x3c
#define ENA_REGS_AENQ_HEAD_DB_OFF		0x40
#define ENA_REGS_AENQ_TAIL_OFF		0x44
#define ENA_REGS_INTR_MASK_OFF		0x4c
#define ENA_REGS_DEV_CTL_OFF		0x54
#define ENA_REGS_DEV_STS_OFF		0x58
#define ENA_REGS_MMIO_REG_READ_OFF		0x5c
#define ENA_REGS_MMIO_RESP_LO_OFF		0x60
#define ENA_REGS_MMIO_RESP_HI_OFF		0x64
#define ENA_REGS_RSS_IND_ENTRY_UPDATE_OFF		0x68

/* version register */
#define ENA_REGS_VERSION_MINOR_VERSION_MASK		0xff
#define ENA_REGS_VERSION_MAJOR_VERSION_SHIFT		8
#define ENA_REGS_VERSION_MAJOR_VERSION_MASK		0xff00

/* controller_version register */
#define ENA_REGS_CONTROLLER_VERSION_SUBMINOR_VERSION_MASK		0xff
#define ENA_REGS_CONTROLLER_VERSION_MINOR_VERSION_SHIFT		8
#define ENA_REGS_CONTROLLER_VERSION_MINOR_VERSION_MASK		0xff00
#define ENA_REGS_CONTROLLER_VERSION_MAJOR_VERSION_SHIFT		16
#define ENA_REGS_CONTROLLER_VERSION_MAJOR_VERSION_MASK		0xff0000
#define ENA_REGS_CONTROLLER_VERSION_IMPL_ID_SHIFT		24
#define ENA_REGS_CONTROLLER_VERSION_IMPL_ID_MASK		0xff000000

/* caps register */
#define ENA_REGS_CAPS_CONTIGUOUS_QUEUE_REQUIRED_MASK		0x1
#define ENA_REGS_CAPS_RESET_TIMEOUT_SHIFT		1
#define ENA_REGS_CAPS_RESET_TIMEOUT_MASK		0x3e
#define ENA_REGS_CAPS_DMA_ADDR_WIDTH_SHIFT		8
#define ENA_REGS_CAPS_DMA_ADDR_WIDTH_MASK		0xff00

/* aq_caps register */
#define ENA_REGS_AQ_CAPS_AQ_DEPTH_MASK		0xffff
#define ENA_REGS_AQ_CAPS_AQ_ENTRY_SIZE_SHIFT		16
#define ENA_REGS_AQ_CAPS_AQ_ENTRY_SIZE_MASK		0xffff0000

/* acq_caps register */
#define ENA_REGS_ACQ_CAPS_ACQ_DEPTH_MASK		0xffff
#define ENA_REGS_ACQ_CAPS_ACQ_ENTRY_SIZE_SHIFT		16
#define ENA_REGS_ACQ_CAPS_ACQ_ENTRY_SIZE_MASK		0xffff0000

/* aenq_caps register */
#define ENA_REGS_AENQ_CAPS_AENQ_DEPTH_MASK		0xffff
#define ENA_REGS_AENQ_CAPS_AENQ_ENTRY_SIZE_SHIFT		16
#define ENA_REGS_AENQ_CAPS_AENQ_ENTRY_SIZE_MASK		0xffff0000

/* dev_ctl register */
#define ENA_REGS_DEV_CTL_DEV_RESET_MASK		0x1
#define ENA_REGS_DEV_CTL_AQ_RESTART_SHIFT		1
#define ENA_REGS_DEV_CTL_AQ_RESTART_MASK		0x2
#define ENA_REGS_DEV_CTL_QUIESCENT_SHIFT		2
#define ENA_REGS_DEV_CTL_QUIESCENT_MASK		0x4
#define ENA_REGS_DEV_CTL_IO_RESUME_SHIFT		3
#define ENA_REGS_DEV_CTL_IO_RESUME_MASK		0x8

/* dev_sts register */
#define ENA_REGS_DEV_STS_READY_MASK		0x1
#define ENA_REGS_DEV_STS_AQ_RESTART_IN_PROGRESS_SHIFT		1
#define ENA_REGS_DEV_STS_AQ_RESTART_IN_PROGRESS_MASK		0x2
#define ENA_REGS_DEV_STS_AQ_RESTART_FINISHED_SHIFT		2
#define ENA_REGS_DEV_STS_AQ_RESTART_FINISHED_MASK		0x4
#define ENA_REGS_DEV_STS_RESET_IN_PROGRESS_SHIFT		3
#define ENA_REGS_DEV_STS_RESET_IN_PROGRESS_MASK		0x8
#define ENA_REGS_DEV_STS_RESET_FINISHED_SHIFT		4
#define ENA_REGS_DEV_STS_RESET_FINISHED_MASK		0x10
#define ENA_REGS_DEV_STS_FATAL_ERROR_SHIFT		5
#define ENA_REGS_DEV_STS_FATAL_ERROR_MASK		0x20
#define ENA_REGS_DEV_STS_QUIESCENT_STATE_IN_PROGRESS_SHIFT		6
#define ENA_REGS_DEV_STS_QUIESCENT_STATE_IN_PROGRESS_MASK		0x40
#define ENA_REGS_DEV_STS_QUIESCENT_STATE_ACHIEVED_SHIFT		7
#define ENA_REGS_DEV_STS_QUIESCENT_STATE_ACHIEVED_MASK		0x80

/* mmio_reg_read register */
#define ENA_REGS_MMIO_REG_READ_REQ_ID_MASK		0xffff
#define ENA_REGS_MMIO_REG_READ_REG_OFF_SHIFT		16
#define ENA_REGS_MMIO_REG_READ_REG_OFF_MASK		0xffff0000

/* rss_ind_entry_update register */
#define ENA_REGS_RSS_IND_ENTRY_UPDATE_INDEX_MASK		0xffff
#define ENA_REGS_RSS_IND_ENTRY_UPDATE_CQ_IDX_SHIFT		16
#define ENA_REGS_RSS_IND_ENTRY_UPDATE_CQ_IDX_MASK		0xffff0000

#endif /*_ENA_REGS_H_ */
