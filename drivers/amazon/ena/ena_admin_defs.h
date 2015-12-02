/******************************************************************************
Copyright (C) 2015 Annapurna Labs Ltd.

This file may be licensed under the terms of the Annapurna Labs Commercial
License Agreement.

Alternatively, this file can be distributed under the terms of the GNU General
Public License V2 as published by the Free Software Foundation and can be
found at http://www.gnu.org/licenses/gpl-2.0.html

Alternatively, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met:

    *  Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

    *  Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#ifndef _ENA_ADMIN_H_
#define _ENA_ADMIN_H_

/* admin commands opcodes */
enum ena_admin_aq_opcode {
	/* create submission queue */
	ena_admin_create_sq = 1,

	/* destroy submission queue */
	ena_admin_destroy_sq = 2,

	/* create completion queue */
	ena_admin_create_cq = 3,

	/* destroy completion queue */
	ena_admin_destroy_cq = 4,

	/* suspend submission queue */
	ena_admin_suspend_sq = 5,

	/* resume submission queue */
	ena_admin_resume_sq = 6,

	/* flush submission queue */
	ena_admin_flush_sq = 7,

	/* get capabilities of particular feature */
	ena_admin_get_feature = 8,

	/* get capabilities of particular feature */
	ena_admin_set_feature = 9,

	/* enabling events in AENQ */
	ena_admin_async_event_request = 10,

	/* get statistics */
	ena_admin_get_stats = 11,
};

/* privileged amdin commands opcodes */
enum ena_admin_aq_opcode_privileged {
	/* get device capabilities */
	ena_admin_identify = 48,

	/* configure device */
	ena_admin_configure_pf_device = 49,

	/* setup SRIOV PCIe Virtual Function capabilities */
	ena_admin_setup_vf = 50,

	/* load firmware to the controller */
	ena_admin_load_firmware = 52,

	/* commit previously loaded firmare */
	ena_admin_commit_firmware = 53,

	/* quiesce virtual function */
	ena_admin_quiesce_vf = 54,

	/* load virtual function from migrates context */
	ena_admin_migrate_vf = 55,
};

/* admin command completion status codes */
enum ena_admin_aq_completion_status {
	/* Request completed successfully */
	ena_admin_success = 0,

	/* no resources to satisfy request */
	ena_admin_resource_allocation_failure = 1,

	/* Bad opcode in request descriptor */
	ena_admin_bad_opcode = 2,

	/* Unsupported opcode in request descriptor */
	ena_admin_unsupported_opcode = 3,

	/* Wrong request format */
	ena_admin_malformed_request = 4,

	/*
	 * One of parameters is not valid. Provided in ACQ entry
	 * extended_status
	 */
	ena_admin_illegal_parameter = 5,

	/* unexpected error */
	ena_admin_unknown_error = 6,
};

/* get/set feature subcommands opcodes */
enum ena_admin_aq_feature_id {
	/* list of all supported attributes/capabilities in the ENA */
	ena_admin_device_attributes = 1,

	/* max number of supported queues per for every queues type */
	ena_admin_max_queues_num = 2,

	/* low latency queues capabilities (max entry size, depth) */
	ena_admin_llq_config = 3,

	/* power management capabilities */
	ena_admin_power_management_config = 4,

	/* MAC address filters support, multicast, broadcast, and promiscous */
	ena_admin_mac_filters_config = 5,

	/* VLAN membership, frame format, etc.  */
	ena_admin_vlan_config = 6,

	/*
	 * Available size for various on-chip memory resources, accessible
	 * by the driver
	 */
	ena_admin_on_device_memory_config = 7,

	/* L2 bridging capabilities inside ENA */
	ena_admin_l2_bridg_config = 8,

	/* L3 routing capabilities inside ENA */
	ena_admin_l3_router_config = 9,

	/* Receive Side Scaling (RSS) function */
	ena_admin_rss_hash_function = 10,

	/* stateless TCP/UDP/IP offload capabilities. */
	ena_admin_stateless_offload_config = 11,

	/* Multiple tuples flow table configuration */
	ena_admin_rss_redirection_table_config = 12,

	/* Data center bridging (DCB) capabilities */
	ena_admin_dcb_config = 13,

	/* max MTU, current MTU */
	ena_admin_mtu = 14,

	/*
	 * Virtual memory address translation capabilities for userland
	 * queues
	 */
	ena_admin_va_translation_config = 15,

	/* traffic class capabilities */
	ena_admin_tc_config = 16,

	/* traffic class capabilities */
	ena_admin_encryption_config = 17,

	/* Receive Side Scaling (RSS) hash input */
	ena_admin_rss_hash_input = 18,

	/* overlay tunnels configuration */
	ena_admin_tunnel_config = 19,

	/* interrupt moderation: count,interval,adaptive */
	ena_admin_interrupt_moderation = 20,

	/* 1588v2 and Timing configuration */
	ena_admin_1588_config = 21,

	/* End-to-End invariant CRC configuration */
	ena_admin_e2e_crc_config = 22,

	/*
	 * Packet Header format templates configuration for input and
	 * output parsers
	 */
	ena_admin_pkt_header_templates_config = 23,

	/* Direct Data Placement (DDP) configuration */
	ena_admin_ddp_config = 24,

	/* Wake on LAN configuration */
	ena_admin_wol_config = 25,

	/* AENQ configuration */
	ena_admin_aenq_config = 26,

	/* Link configuration */
	ena_admin_link_config = 27,

	/* Host attributes configuration */
	ena_admin_host_attr_config = 28,

	/* Number of valid opcodes */
	ena_admin_features_opcode_num = 32,
};

/* descriptors and headers placement */
enum ena_admin_placement_policy_type {
	/* descriptors and headers are in OS memory */
	ena_admin_placement_policy_host = 1,

	/*
	 * descriptors and headers in device memory (a.k.a Low Latency
	 * Queue)
	 */
	ena_admin_placement_policy_dev = 3,
};

/* link speeds */
enum ena_admin_link_types {
	ena_admin_link_speed_1G = 0x1,

	ena_admin_link_speed_2_half_G = 0x2,

	ena_admin_link_speed_5G = 0x4,

	ena_admin_link_speed_10G = 0x8,

	ena_admin_link_speed_25G = 0x10,

	ena_admin_link_speed_40G = 0x20,

	ena_admin_link_speed_50G = 0x40,

	ena_admin_link_speed_100G = 0x80,

	ena_admin_link_speed_200G = 0x100,

	ena_admin_link_speed_400G = 0x200,
};

/* completion queue update policy */
enum ena_admin_completion_policy_type {
	/* cqe for each sq descriptor */
	ena_admin_completion_policy_desc = 0,

	/* cqe upon request in sq descriptor */
	ena_admin_completion_policy_desc_on_denamd = 1,

	/*
	 * current queue head pointer is updated in OS memory upon sq
	 * descriptor request
	 */
	ena_admin_completion_policy_head_on_deman = 2,

	/*
	 * current queue head pointer is updated in OS memory for each sq
	 * descriptor
	 */
	ena_admin_completion_policy_head = 3,
};

/* type of get statistics command */
enum ena_admin_get_stats_type {
	/* Basic statistics */
	ena_admin_get_stats_type_basic = 0,

	/* Extended statistics */
	ena_admin_get_stats_type_extended = 1,
};

/* scope of get statistics command */
enum ena_admin_get_stats_scope {
	ena_admin_specific_queue = 0,

	ena_admin_eth_traffic = 1,
};

/* ENA Admin Queue (AQ) common descriptor */
struct ena_admin_aq_common_desc {
	/* word 0 : */
	/*
	 * command identificator to associate it with the completion
	 * 11:0 : command_id
	 * 15:12 : reserved12
	 */
	u16 command_id;

	/* as appears in ena_aq_opcode */
	u8 opcode;

	/*
	 * 0 : phase
	 * 1 : ctrl_data - control buffer address valid
	 * 2 : ctrl_data_indirect - control buffer address
	 *    points to list of pages with addresses of control
	 *    buffers
	 * 7:3 : reserved3
	 */
	u8 flags;
};

/*
 * used in ena_aq_entry. Can point directly to control data, or to a page
 * list chunk. Used also at the end of indirect mode page list chunks, for
 * chaining.
 */
struct ena_admin_ctrl_buff_info {
	/*
	 * word 0 : indicates length of the buffer pointed by
	 * control_buffer_address.
	 */
	u32 length;

	/* words 1:2 : points to control buffer (direct or indirect) */
	struct ena_common_mem_addr address;
};

/* submission queue full identification */
struct ena_admin_sq {
	/* word 0 : */
	/* queue id */
	u16 sq_idx;

	/*
	 * 4:0 : sq_type - 0x1 - ethernet queue; 0x2 - fabric
	 *    queue; 0x3 fabric queue with RDMA; 0x4 - DPDK queue
	 * 7:5 : sq_direction - 0x1 - Tx; 0x2 - Rx; 0x3 - SRQ
	 */
	u8 sq_identity;

	u8 reserved1;
};

/* AQ entry format */
struct ena_admin_aq_entry {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1:3 :  */
	union {
		/* command specific inline data */
		u32 inline_data_w1[3];

		/*
		 * words 1:3 : points to control buffer (direct or
		 * indirect, chained if needed)
		 */
		struct ena_admin_ctrl_buff_info control_buffer;
	} u;

	/* command specific inline data */
	u32 inline_data_w4[12];
};

/* ENA Admin Completion Queue (ACQ) common descriptor */
struct ena_admin_acq_common_desc {
	/* word 0 : */
	/*
	 * command identifier to associate it with the aq descriptor
	 * 11:0 : command_id
	 * 15:12 : reserved12
	 */
	u16 command;

	/* status of request execution */
	u8 status;

	/*
	 * 0 : phase
	 * 7:1 : reserved1
	 */
	u8 flags;

	/* word 1 : */
	/* provides additional info */
	u16 extended_status;

	/*
	 * submission queue head index, serves as a hint what AQ entries can
	 *    be revoked
	 */
	u16 sq_head_indx;
};

/* ACQ entry format */
struct ena_admin_acq_entry {
	/* words 0:1 :  */
	struct ena_admin_acq_common_desc acq_common_descriptor;

	/* response type specific data */
	u32 response_specific_data[14];
};

/*
 * ENA AQ Create Submission Queue command. Placed in control buffer pointed
 * by AQ entry
 */
struct ena_admin_aq_create_sq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* word 1 : */
	/*
	 * 4:0 : sq_type - 0x1 - ethernet queue; 0x2 - fabric
	 *    queue; 0x3 fabric queue with RDMA; 0x4 - DPDK queue
	 * 7:5 : sq_direction - 0x1 - Tx; 0x2 - Rx; 0x3 - SRQ
	 */
	u8 sq_identity;

	/*
	 * 0 : virtual_addressing_support - whether the
	 *    specific queue is requested to handle Userland
	 *    virtual addresses, which burdens the ENA perfom VA
	 *    to Physical address translation
	 * 3:1 : traffic_class - Default traffic class for
	 *    packets transmitted on his queue
	 * 7:4 : rx_fixed_sgl_size - In case this value is
	 *    larger than 0, then each Rx packet, will consumed
	 *    a fixed number of Rx Descriptors equal to
	 *    rx_fixed_sgl_size, this feature will enable
	 *    capabilities like header split and aligning
	 *    packets to fixed numbers of pages. NOTE: that
	 *    queue depth is still in number of Rx Descriptors.
	 *    It is the programmer responsibility to make sure
	 *    each set of SGL has enough buffer to accept the
	 *    maximal receive packet length
	 */
	u8 sq_caps_1;

	/*
	 * 3:0 : placement_policy - Describing where the SQ
	 *    descriptor ring and the SQ packet headers reside:
	 *    0x1 - descriptors and headers are in OS memory,
	 *    0x3 - descriptors and headers in device memory
	 *    (a.k.a Low Latency Queue)
	 * 6:4 : completion_policy - Describing what policy
	 *    to use for generation completion entry (cqe) in
	 *    the CQ associated with this SQ: 0x0 - cqe for each
	 *    sq descriptor, 0x1 - cqe upon request in sq
	 *    descriptor, 0x2 - current queue head pointer is
	 *    updated in OS memory upon sq descriptor request
	 *    0x3 - current queue head pointer is updated in OS
	 *    memory for each sq descriptor
	 * 7 : reserved7
	 */
	u8 sq_caps_2;

	/*
	 * 0 : is_physically_contiguous - Described if the
	 *    queue ring memory is allocated in physical
	 *    contiguous pages or split.
	 * 7:1 : reserved1
	 */
	u8 sq_caps_3;

	/* word 2 : */
	/*
	 * associated completion queue id. This CQ must be created prior to
	 *    SQ creation
	 */
	u16 cq_idx;

	/* submission queue depth in # of entries */
	u16 sq_depth;

	/*
	 * words 3:4 : SQ physical base address in OS memory. This field
	 * should not be used for Low Latency queues. Has to be page
	 * aligned.
	 */
	struct ena_common_mem_addr sq_ba;

	/*
	 * words 5:6 : specifies queue head writeback location in OS
	 * memory. Valid if completion_policy is set to 0x3. Has to be
	 * cache aligned
	 */
	struct ena_common_mem_addr sq_head_writeback;

	/* word 7 : */
	/* protection domain - needed if address translation is supported */
	u16 pd;

	/* reserved */
	u16 reserved16_w8;

	/* word 8 : reserved word */
	u32 reserved0_w9;
};

/* submission queue direction */
enum ena_admin_sq_direction {
	ena_admin_sq_direction_tx = 1,

	ena_admin_sq_direction_rx = 2,

	/* Shared Receive queue */
	ena_admin_sq_direction_srq = 3,
};

/* submission queue type */
enum ena_admin_sq_type {
	/* ethernet queue */
	ena_admin_eth = 1,

	/* fabric queue */
	ena_admin_fabric = 2,

	/* fabric queue with RDMA */
	ena_admin_fabric_rdma = 3,

	/* DPDK queue */
	ena_admin_dpdk = 4,
};

/*
 * ENA Response for Create SQ Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_create_sq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;

	/* word 2 : */
	/* sq identifier */
	u16 sq_idx;

	/* sq depth in # of entries */
	u16 sq_actual_depth;

	/*
	 * word 3 : queue doorbell address as and offset to PCIe MMIO REG
	 * BAR
	 */
	u32 sq_doorbell_offset;

	/*
	 * word 4 : low latency queue ring base address as an offset to
	 * PCIe MMIO LLQ_MEM BAR
	 */
	u32 llq_descriptors_offset;

	/*
	 * word 5 : low latency queue headers' memory as an offset to PCIe
	 * MMIO LLQ_MEM BAR
	 */
	u32 llq_headers_offset;
};

/*
 * ENA AQ Destroy Submission Queue command. Placed in control buffer
 * pointed by AQ entry
 */
struct ena_admin_aq_destroy_sq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1 :  */
	struct ena_admin_sq sq;
};

/*
 * ENA Response for Destroy SQ Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_destroy_sq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;
};

/* ENA AQ Create Completion Queue command */
struct ena_admin_aq_create_cq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* word 1 : */
	/*
	 * 4:0 : cq_type - 0x1 - eth cq; 0x2 - fabric cq; 0x3
	 *    fabric cq with RDMA; 0x4 - DPDK cq
	 * 5 : interrupt_mode_enabled - if set, cq operates
	 *    in interrupt mode, otherwise - polling
	 * 7:6 : reserved6
	 */
	u8 cq_caps_1;

	/*
	 * 4:0 : cq_entry_size_words - size of CQ entry in
	 *    32-bit words, valid values: 4, 8.
	 * 7:5 : reserved7
	 */
	u8 cq_caps_2;

	/* completion queue depth in # of entries */
	u16 cq_depth;

	/* word 2 : msix vector assigned to this cq */
	u32 msix_vector;

	/*
	 * words 3:4 : cq physical base address in OS memory. CQ must be
	 * physically contiguous
	 */
	struct ena_common_mem_addr cq_ba;
};

/*
 * ENA Response for Create CQ Command. Appears in ACQ entry as response
 * specific data
 */
struct ena_admin_acq_create_cq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;

	/* word 2 : */
	/* cq identifier */
	u16 cq_idx;

	/* actual cq depth in # of entries */
	u16 cq_actual_depth;

	/* word 3 : doorbell address as an offset to PCIe MMIO REG BAR */
	u32 cq_doorbell_offset;

	/*
	 * word 4 : completion head doorbell address as an offset to PCIe
	 * MMIO REG BAR
	 */
	u32 cq_head_db_offset;

	/*
	 * word 5 : interrupt unmask register address as an offset into
	 * PCIe MMIO REG BAR
	 */
	u32 cq_interrupt_unmask_register;

	/* word 6 : value to be written into interrupt unmask register */
	u32 cq_interrupt_unmask_value;

	/*
	 * word 7 : interrupt moderation register address as an offset into
	 * PCIe MMIO REG BAR. 1 usec granularity
	 */
	u32 cq_interrupt_moderation_register;
};

/*
 * ENA AQ Destroy Completion Queue command. Placed in control buffer
 * pointed by AQ entry
 */
struct ena_admin_aq_destroy_cq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* word 1 : */
	/* associated queue id. */
	u16 cq_idx;

	u16 reserved1;
};

/*
 * ENA Response for Destroy CQ Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_destroy_cq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;
};

/*
 * ENA AQ Suspend Submission Queue command. Placed in control buffer
 * pointed by AQ entry
 */
struct ena_admin_aq_suspend_sq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1 :  */
	struct ena_admin_sq sq;
};

/*
 * ENA Response for Suspend SQ Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_suspend_sq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;
};

/*
 * ENA AQ Resume Submission Queue command. Placed in control buffer pointed
 * by AQ entry
 */
struct ena_admin_aq_resume_sq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1 :  */
	struct ena_admin_sq sq;
};

/*
 * ENA Response for Resume SQ Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_resume_sq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;
};

/*
 * ENA AQ Flush Submission Queue command. Placed in control buffer pointed
 * by AQ entry
 */
struct ena_admin_aq_flush_sq_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1 :  */
	struct ena_admin_sq sq;
};

/*
 * ENA Response for Flush SQ Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_flush_sq_resp_desc {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;
};

/*
 * ENA AQ Get Statistics command. Extended statistics are placed in control
 * buffer pointed by AQ entry
 */
struct ena_admin_aq_get_stats_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1:3 :  */
	union {
		/* command specific inline data */
		u32 inline_data_w1[3];

		/*
		 * words 1:3 : points to control buffer (direct or
		 * indirect, chained if needed)
		 */
		struct ena_admin_ctrl_buff_info control_buffer;
	} u;

	/* word 4 : */
	/* stats type as defined in enum ena_admin_get_stats_type */
	u8 type;

	/* stats scope defined in enum ena_admin_get_stats_scope */
	u8 scope;

	u16 reserved3;

	/* word 5 : */
	/* queue id. used when scope is specific_queue */
	u16 queue_idx;

	/*
	 * device id, value 0xFFFF means mine. only privileged device can get
	 *    stats of other device
	 */
	u16 device_id;
};

/* Basic Statistics Command. */
struct ena_admin_basic_stats {
	/* word 0 :  */
	u32 tx_bytes_low;

	/* word 1 :  */
	u32 tx_bytes_high;

	/* word 2 :  */
	u32 tx_pkts_low;

	/* word 3 :  */
	u32 tx_pkts_high;

	/* word 4 :  */
	u32 rx_bytes_low;

	/* word 5 :  */
	u32 rx_bytes_high;

	/* word 6 :  */
	u32 rx_pkts_low;

	/* word 7 :  */
	u32 rx_pkts_high;

	/* word 8 :  */
	u32 rx_drops_low;

	/* word 9 :  */
	u32 rx_drops_high;
};

/*
 * ENA Response for Get Statistics Command. Appears in ACQ entry as
 * response_specific_data
 */
struct ena_admin_acq_get_stats_resp {
	/* words 0:1 : Common Admin Queue completion descriptor */
	struct ena_admin_acq_common_desc acq_common_desc;

	/* words 2:11 :  */
	struct ena_admin_basic_stats basic_stats;
};

/*
 * ENA Get/Set Feature common descriptor. Appears as inline word in
 * ena_aq_entry
 */
struct ena_admin_get_set_feature_common_desc {
	/* word 0 : */
	/*
	 * 1:0 : select - 0x1 - current value; 0x3 - default
	 *    value
	 * 7:3 : reserved3
	 */
	u8 flags;

	/* as appears in ena_feature_id */
	u8 feature_id;

	/* reserved16 */
	u16 reserved16;
};

/* ENA Device Attributes Feature descriptor. */
struct ena_admin_device_attr_feature_desc {
	/* word 0 : implementation id */
	u32 impl_id;

	/* word 1 : device version */
	u32 device_version;

	/*
	 * word 2 : bit map of which bits are supported value of 1
	 * indicated that this feature is supported and can perform SET/GET
	 * for it
	 */
	u32 supported_features;

	/* word 3 :  */
	u32 reserved3;

	/*
	 * word 4 : Indicates how many bits are used physical address
	 * access. Typically 48
	 */
	u32 phys_addr_width;

	/*
	 * word 5 : Indicates how many bits are used virtual address
	 * access. Typically 48
	 */
	u32 virt_addr_width;

	/* unicast MAC address (in Network byte order) */
	u8 mac_addr[6];

	u8 reserved7[2];

	/* word 8 : Max supported MTU value */
	u32 max_mtu;
};

/* ENA Max Queues Feature descriptor. */
struct ena_admin_queue_feature_desc {
	/* word 0 : Max number of submission queues (including LLQs) */
	u32 max_sq_num;

	/* word 1 : Max submission queue depth */
	u32 max_sq_depth;

	/* word 2 : Max number of completion queues */
	u32 max_cq_num;

	/* word 3 : Max completion queue depth */
	u32 max_cq_depth;

	/* word 4 : Max number of LLQ submission queues */
	u32 max_llq_num;

	/* word 5 : Max submission queue depth of LLQ */
	u32 max_llq_depth;

	/* word 6 : Max header size for LLQ */
	u32 max_llq_header_size;
};

/* ENA MTU Set Feature descriptor. */
struct ena_admin_set_feature_mtu_desc {
	/* word 0 : mtu size including L2 */
	u32 mtu;
};

/* ENA host attributes Set Feature descriptor. */
struct ena_admin_set_feature_host_attr_desc {
	/* word 0 : driver version */
	u32 driver_version;

	/*
	 * words 1:2 : 4KB of dying gasp log. This buffer is filled on
	 * fatal error.
	 */
	struct ena_common_mem_addr dying_gasp_log;
};

/* ENA Interrupt Moderation metrics. */
struct ena_admin_intr_moder_metrics_desc {
	/* word 0 : */
	u16 count;

	/* interval in us */
	u16 interval;
};

/* ENA Link Get Feature descriptor. */
struct ena_admin_get_feature_link_desc {
	/* word 0 : Link speed in Mb */
	u32 speed;

	/*
	 * word 1 : supported speeds (bit field of enum ena_admin_link
	 * types)
	 */
	u32 supported;

	/* word 2 : */
	/*
	 * 0 : autoneg - auto negotiation
	 * 1 : duplex - Full Duplex
	 * 31:2 : reserved2
	 */
	u32 flags;
};

/* ENA Interrupt Moderation Set Feature descriptor. */
struct ena_admin_set_feature_intr_moder_desc {
	/* word 0 : */
	/* associated queue id. */
	u16 cq_idx;

	/* 2:0 : sq_direction - 0x1 - Tx; 0x2 - Rx */
	u8 queue_identity;

	u8 reserved1;

	/* word 1 : */
	/*
	 * 0 : enable
	 * 31:1 : reserved1
	 */
	u32 flags;

	/* words 2 :  */
	struct ena_admin_intr_moder_metrics_desc intr_moder_metrics;
};

/* ENA AENQ Feature descriptor. */
struct ena_admin_feature_aenq_desc {
	/* word 0 : bitmask for AENQ groups the device can report */
	u32 supported_groups;

	/* word 1 : bitmask for AENQ groups to report */
	u32 enabled_groups;
};

/* ENA Stateless Offload Feature descriptor. */
struct ena_admin_feature_offload_desc {
	/* word 0 : */
	/*
	 * Trasmit side stateless offload
	 * 0 : TX_L3_csum_ipv4 - IPv4 checksum
	 * 1 : TX_L4_ipv4_csum_part - TCP/UDP over IPv4
	 *    checksum, the checksum field should be initialized
	 *    with pseudo header checksum
	 * 2 : TX_L4_ipv4_csum_full - TCP/UDP over IPv4
	 *    checksum
	 * 3 : TX_L4_ipv6_csum_part - TCP/UDP over IPv6
	 *    checksum, the checksum field should be initialized
	 *    with pseudo header checksum
	 * 4 : TX_L4_ipv6_csum_full - TCP/UDP over IPv6
	 *    checksum
	 * 5 : tso_ipv4 - TCP/IPv4 Segmentation Offloading
	 * 6 : tso_ipv6 - TCP/IPv6 Segmentation Offloading
	 * 7 : tso_ecn - TCP Segmentation with ECN
	 */
	u32 tx;

	/* word 1 : */
	/*
	 * Receive side supported stateless offload
	 * 0 : RX_L3_csum_ipv4 - IPv4 checksum
	 * 1 : RX_L4_ipv4_csum - TCP/UDP/IPv4 checksum
	 * 2 : RX_L4_ipv6_csum - TCP/UDP/IPv6 checksum
	 */
	u32 rx_supported;

	/* word 2 : */
	/* Receive side enabled stateless offload */
	u32 rx_enabled;
};

/* hash functions */
enum ena_admin_hash_functions {
	/* Toeplitz hash */
	ena_admin_toeplitz = 1,

	/* CRC32 hash */
	ena_admin_crc32 = 2,
};

/* ENA RSS Flow Hash Function */
struct ena_admin_feature_rss_flow_hash_function {
	/* word 0 : */
	/*
	 * supported hash functions
	 * 7:0 : funcs - supported hash functions (bitmask
	 *    accroding to ena_admin_hash_functions)
	 */
	u32 supported_func;

	/* word 1 : */
	/*
	 * selected hash func
	 * 7:0 : selected_func - selected hash function
	 *    (bitmask accroding to ena_admin_hash_functions)
	 */
	u32 selected_func;

	/* word 2 : initial value */
	u32 init_val;

	/* Toeplitz keys */
	u32 key[10];
};

/* RSS flow hash protocols */
enum ena_admin_flow_hash_proto {
	/* tcp/ipv4 */
	ena_admin_rss_tcp4 = 0,

	/* udp/ipv4 */
	ena_admin_rss_udp4 = 1,

	/* tcp/ipv6 */
	ena_admin_rss_tcp6 = 2,

	/* udp/ipv6 */
	ena_admin_rss_udp6 = 3,

	/* ipv4 not tcp/udp */
	ena_admin_rss_ip4 = 4,

	/* ipv6 not tcp/udp */
	ena_admin_rss_ip6 = 5,

	/* fragmented ipv4 */
	ena_admin_rss_ip4_frag = 6,

	/* not ipv4/6 */
	ena_admin_rss_not_ip = 7,

	/* max number of protocols */
	ena_admin_rss_proto_count = 16,
};

/* RSS flow hash fields */
enum ena_admin_flow_hash_fields {
	/* Ethernet Dest Addr */
	ena_admin_rss_l2_da = 0,

	/* Ethernet Src Addr */
	ena_admin_rss_l2_sa = 1,

	/* ipv4/6 Dest Addr */
	ena_admin_rss_l3_da = 2,

	/* ipv4/6 Src Addr */
	ena_admin_rss_l3_sa = 5,

	/* tcp/udp Dest Port */
	ena_admin_rss_l4_dp = 6,

	/* tcp/udp Src Port */
	ena_admin_rss_l4_sp = 7,
};

/* hash input fields for flow protocol */
struct ena_admin_proto_input {
	/* word 0 : */
	/* flow hash fields (bitwise according to ena_admin_flow_hash_fields) */
	u16 fields;

	/*
	 * 0 : inner - for tunneled packet, select the fields
	 *    from inner header
	 */
	u16 flags;
};

/* ENA RSS hash control buffer structure */
struct ena_admin_feature_rss_hash_control {
	/* supported input fields */
	struct ena_admin_proto_input supported_input_fields[ena_admin_rss_proto_count];

	/* selected input fields */
	struct ena_admin_proto_input selected_input_fields[ena_admin_rss_proto_count];

	/* supported input fields for inner header */
	struct ena_admin_proto_input supported_inner_input_fields[ena_admin_rss_proto_count];

	/* selected input fields */
	struct ena_admin_proto_input selected_inner_input_fields[ena_admin_rss_proto_count];
};

/* ENA RSS flow hash input */
struct ena_admin_feature_rss_flow_hash_input {
	/* word 0 : */
	/*
	 * supported hash input sorting
	 * 1 : L3_sort - support swap L3 addresses if DA
	 *    smaller than SA
	 * 2 : L4_sort - support swap L4 ports if DP smaller
	 *    SP
	 */
	u16 supported_input_sort;

	/*
	 * enabled hash input sorting
	 * 1 : enable_L3_sort - enable swap L3 addresses if
	 *    DA smaller than SA
	 * 2 : enable_L4_sort - enable swap L4 ports if DP
	 *    smaller than SP
	 */
	u16 enabled_input_sort;
};

/* ENA RSS redirection table entry */
struct ena_admin_rss_redirection_table_entry {
	/* word 0 : */
	/* cq identifier */
	u16 cq_idx;

	u16 reserved;
};

/* ENA RSS redirection table */
struct ena_admin_feature_rss_redirection_table {
	/* word 0 : */
	/* min supported table size (2^min_size) */
	u16 min_size;

	/* max supported table size (2^max_size) */
	u16 max_size;

	/* word 1 : */
	/* table size (2^size) */
	u16 size;

	u16 reserved;
};

/* ENA Get Feature command */
struct ena_admin_get_feat_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1 :  */
	struct ena_admin_get_set_feature_common_desc feat_common;

	/* words 2:15 :  */
	union {
		/* raw words */
		u32 raw[14];
	} u;
};

/* ENA Get Feature command response */
struct ena_admin_get_feat_resp {
	/* words 0:1 :  */
	struct ena_admin_acq_common_desc acq_common_desc;

	/* words 2:15 :  */
	union {
		/* raw words */
		u32 raw[14];

		/* words 2:10 : Get Device Attributes */
		struct ena_admin_device_attr_feature_desc dev_attr;

		/* words 2:5 : Max queues num */
		struct ena_admin_queue_feature_desc max_queue;

		/* words 2:3 : AENQ configuration */
		struct ena_admin_feature_aenq_desc aenq;

		/* words 2:4 : Get Link configuration */
		struct ena_admin_get_feature_link_desc link;

		/* words 2:4 : offload configuration */
		struct ena_admin_feature_offload_desc offload;

		/* words 2:14 : rss flow hash function */
		struct ena_admin_feature_rss_flow_hash_function flow_hash_func;

		/* words 2 : rss flow hash input */
		struct ena_admin_feature_rss_flow_hash_input flow_hash_input;

		/* words 2:3 : rss redirection table */
		struct ena_admin_feature_rss_redirection_table redirection_table;
	} u;
};

/* ENA Set Feature command */
struct ena_admin_set_feat_cmd {
	/* words 0 :  */
	struct ena_admin_aq_common_desc aq_common_descriptor;

	/* words 1 :  */
	struct ena_admin_get_set_feature_common_desc feat_common;

	/* words 2:15 :  */
	union {
		/* raw words */
		u32 raw[14];

		/* words 2 : mtu size */
		struct ena_admin_set_feature_mtu_desc mtu;

		/* words 2:4 : host attributes */
		struct ena_admin_set_feature_host_attr_desc host_attr;

		/* words 2:4 : interrupt moderation */
		struct ena_admin_set_feature_intr_moder_desc intr_moder;

		/* words 2:3 : AENQ configuration */
		struct ena_admin_feature_aenq_desc aenq;

		/* words 2:14 : rss flow hash function */
		struct ena_admin_feature_rss_flow_hash_function flow_hash_func;

		/* words 2 : rss flow hash input */
		struct ena_admin_feature_rss_flow_hash_input flow_hash_input;

		/* words 2:3 : rss redirection table */
		struct ena_admin_feature_rss_redirection_table redirection_table;
	} u;
};

/* ENA Set Feature command response */
struct ena_admin_set_feat_resp {
	/* words 0:1 :  */
	struct ena_admin_acq_common_desc acq_common_desc;

	/* words 2:15 :  */
	union {
		/* raw words */
		u32 raw[14];
	} u;
};

/* ENA Asynchronous Event Notification Queue descriptor.  */
struct ena_admin_aenq_common_desc {
	/* word 0 : */
	u16 group;

	u16 syndrom;

	/* word 1 : */
	/* 0 : phase */
	u8 flags;

	u8 reserved1[3];

	/* word 2 : Timestamp LSB */
	u32 timestamp_low;

	/* word 3 : Timestamp MSB */
	u32 timestamp_high;
};

/* asynchronous event notification groups */
enum ena_admin_aenq_group {
	/* Link State Change */
	ena_admin_link_change = 0,

	ena_admin_fatal_error = 1,

	ena_admin_warning = 2,

	ena_admin_notification = 3,

	ena_admin_keep_alive = 4,

	ena_admin_aenq_groups_num = 5,
};

/* syndrom of AENQ warning group */
enum ena_admin_aenq_warning_syndrom {
	ena_admin_thermal = 0,

	ena_admin_logging_fifo = 1,

	ena_admin_dirty_page_logging_fifo = 2,

	ena_admin_malicious_mmio_access = 3,

	ena_admin_cq_full = 4,
};

/* syndorm of AENQ notification group */
enum ena_admin_aenq_notification_syndrom {
	ena_admin_suspend = 0,

	ena_admin_resume = 1,
};

/* ENA Asynchronous Event Notification generic descriptor.  */
struct ena_admin_aenq_entry {
	/* words 0:3 :  */
	struct ena_admin_aenq_common_desc aenq_common_desc;

	/* command specific inline data */
	u32 inline_data_w4[12];
};

/* ENA Asynchronous Event Notification Queue Link Change descriptor.  */
struct ena_admin_aenq_link_change_desc {
	/* words 0:3 :  */
	struct ena_admin_aenq_common_desc aenq_common_desc;

	/* word 4 : */
	/* 0 : link_status */
	u32 flags;
};

/* ENA MMIO Readless response interface */
struct ena_admin_ena_mmio_read_less_resp {
	/* word 0 : */
	/* request id */
	u16 req_id;

	/* register offset */
	u16 reg_off;

	/* word 1 : value is valid when poll is cleared */
	u32 reg_val;
};

/* aq_common_desc */
#define ENA_ADMIN_AQ_COMMON_DESC_COMMAND_ID_MASK		GENMASK(12, 0)
#define ENA_ADMIN_AQ_COMMON_DESC_PHASE_MASK		BIT(0)
#define ENA_ADMIN_AQ_COMMON_DESC_CTRL_DATA_SHIFT		1
#define ENA_ADMIN_AQ_COMMON_DESC_CTRL_DATA_MASK		BIT(1)
#define ENA_ADMIN_AQ_COMMON_DESC_CTRL_DATA_INDIRECT_SHIFT		2
#define ENA_ADMIN_AQ_COMMON_DESC_CTRL_DATA_INDIRECT_MASK		BIT(2)

/* sq */
#define ENA_ADMIN_SQ_SQ_TYPE_MASK		GENMASK(5, 0)
#define ENA_ADMIN_SQ_SQ_DIRECTION_SHIFT		5
#define ENA_ADMIN_SQ_SQ_DIRECTION_MASK		GENMASK(8, 5)

/* acq_common_desc */
#define ENA_ADMIN_ACQ_COMMON_DESC_COMMAND_ID_MASK		GENMASK(12, 0)
#define ENA_ADMIN_ACQ_COMMON_DESC_PHASE_MASK		BIT(0)

/* aq_create_sq_cmd */
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_SQ_TYPE_MASK		GENMASK(5, 0)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_SQ_DIRECTION_SHIFT		5
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_SQ_DIRECTION_MASK		GENMASK(8, 5)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_VIRTUAL_ADDRESSING_SUPPORT_MASK		BIT(0)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_TRAFFIC_CLASS_SHIFT		1
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_TRAFFIC_CLASS_MASK		GENMASK(4, 1)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_RX_FIXED_SGL_SIZE_SHIFT		4
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_RX_FIXED_SGL_SIZE_MASK		GENMASK(8, 4)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_PLACEMENT_POLICY_MASK		GENMASK(4, 0)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_COMPLETION_POLICY_SHIFT		4
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_COMPLETION_POLICY_MASK		GENMASK(7, 4)
#define ENA_ADMIN_AQ_CREATE_SQ_CMD_IS_PHYSICALLY_CONTIGUOUS_MASK		BIT(0)

/* aq_create_cq_cmd */
#define ENA_ADMIN_AQ_CREATE_CQ_CMD_CQ_TYPE_MASK		GENMASK(5, 0)
#define ENA_ADMIN_AQ_CREATE_CQ_CMD_INTERRUPT_MODE_ENABLED_SHIFT		5
#define ENA_ADMIN_AQ_CREATE_CQ_CMD_INTERRUPT_MODE_ENABLED_MASK		BIT(5)
#define ENA_ADMIN_AQ_CREATE_CQ_CMD_CQ_ENTRY_SIZE_WORDS_MASK		GENMASK(5, 0)

/* get_set_feature_common_desc */
#define ENA_ADMIN_GET_SET_FEATURE_COMMON_DESC_SELECT_MASK		GENMASK(2, 0)

/* get_feature_link_desc */
#define ENA_ADMIN_GET_FEATURE_LINK_DESC_AUTONEG_MASK		BIT(0)
#define ENA_ADMIN_GET_FEATURE_LINK_DESC_DUPLEX_SHIFT		1
#define ENA_ADMIN_GET_FEATURE_LINK_DESC_DUPLEX_MASK		BIT(1)

/* set_feature_intr_moder_desc */
#define ENA_ADMIN_SET_FEATURE_INTR_MODER_DESC_SQ_DIRECTION_MASK		GENMASK(3, 0)
#define ENA_ADMIN_SET_FEATURE_INTR_MODER_DESC_ENABLE_MASK		BIT(0)

/* feature_offload_desc */
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L3_CSUM_IPV4_MASK		BIT(0)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV4_CSUM_PART_SHIFT		1
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV4_CSUM_PART_MASK		BIT(1)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV4_CSUM_FULL_SHIFT		2
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV4_CSUM_FULL_MASK		BIT(2)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV6_CSUM_PART_SHIFT		3
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV6_CSUM_PART_MASK		BIT(3)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV6_CSUM_FULL_SHIFT		4
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TX_L4_IPV6_CSUM_FULL_MASK		BIT(4)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TSO_IPV4_SHIFT		5
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TSO_IPV4_MASK		BIT(5)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TSO_IPV6_SHIFT		6
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TSO_IPV6_MASK		BIT(6)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TSO_ECN_SHIFT		7
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_TSO_ECN_MASK		BIT(7)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_RX_L3_CSUM_IPV4_MASK		BIT(0)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_RX_L4_IPV4_CSUM_SHIFT		1
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_RX_L4_IPV4_CSUM_MASK		BIT(1)
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_RX_L4_IPV6_CSUM_SHIFT		2
#define ENA_ADMIN_FEATURE_OFFLOAD_DESC_RX_L4_IPV6_CSUM_MASK		BIT(2)

/* feature_rss_flow_hash_function */
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_FUNCTION_FUNCS_MASK		GENMASK(8, 0)
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_FUNCTION_SELECTED_FUNC_MASK		GENMASK(8, 0)

/* proto_input */
#define ENA_ADMIN_PROTO_INPUT_INNER_MASK		BIT(0)

/* feature_rss_flow_hash_input */
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_L3_SORT_SHIFT		1
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_L3_SORT_MASK		BIT(1)
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_L4_SORT_SHIFT		2
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_L4_SORT_MASK		BIT(2)
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_ENABLE_L3_SORT_SHIFT		1
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_ENABLE_L3_SORT_MASK		BIT(1)
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_ENABLE_L4_SORT_SHIFT		2
#define ENA_ADMIN_FEATURE_RSS_FLOW_HASH_INPUT_ENABLE_L4_SORT_MASK		BIT(2)

/* aenq_common_desc */
#define ENA_ADMIN_AENQ_COMMON_DESC_PHASE_MASK		BIT(0)

/* aenq_link_change_desc */
#define ENA_ADMIN_AENQ_LINK_CHANGE_DESC_LINK_STATUS_MASK		BIT(0)

#endif /*_ENA_ADMIN_H_ */
