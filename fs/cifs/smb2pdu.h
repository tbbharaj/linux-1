/* SPDX-License-Identifier: LGPL-2.1 */
/*
 *
 *   Copyright (c) International Business Machines  Corp., 2009, 2013
 *                 Etersoft, 2012
 *   Author(s): Steve French (sfrench@us.ibm.com)
 *              Pavel Shilovsky (pshilovsky@samba.org) 2012
 *
 */

#ifndef _SMB2PDU_H
#define _SMB2PDU_H

#include <net/sock.h>
#include "cifsacl.h"

/* 52 transform hdr + 64 hdr + 88 create rsp */
#define SMB2_TRANSFORM_HEADER_SIZE 52
#define MAX_SMB2_HDR_SIZE 204

/* The total header size for SMB2 read and write */
#define SMB2_READWRITE_PDU_HEADER_SIZE (48 + sizeof(struct smb2_hdr))

/* See MS-SMB2 2.2.43 */
struct smb2_rdma_transform {
	__le16 RdmaDescriptorOffset;
	__le16 RdmaDescriptorLength;
	__le32 Channel; /* for values see channel description in smb2 read above */
	__le16 TransformCount;
	__le16 Reserved1;
	__le32 Reserved2;
} __packed;

/* TransformType */
#define SMB2_RDMA_TRANSFORM_TYPE_ENCRYPTION	0x0001
#define SMB2_RDMA_TRANSFORM_TYPE_SIGNING	0x0002

struct smb2_rdma_crypto_transform {
	__le16	TransformType;
	__le16	SignatureLength;
	__le16	NonceLength;
	__u16	Reserved;
	__u8	Signature[]; /* variable length */
	/* u8 Nonce[] */
	/* followed by padding */
} __packed;

/*
 *	Definitions for SMB2 Protocol Data Units (network frames)
 *
 *  See MS-SMB2.PDF specification for protocol details.
 *  The Naming convention is the lower case version of the SMB2
 *  command code name for the struct. Note that structures must be packed.
 *
 */

#define COMPOUND_FID 0xFFFFFFFFFFFFFFFFULL

#define SYMLINK_ERROR_TAG 0x4c4d5953

struct smb2_symlink_err_rsp {
	__le32 SymLinkLength;
	__le32 SymLinkErrorTag;
	__le32 ReparseTag;
	__le16 ReparseDataLength;
	__le16 UnparsedPathLength;
	__le16 SubstituteNameOffset;
	__le16 SubstituteNameLength;
	__le16 PrintNameOffset;
	__le16 PrintNameLength;
	__le32 Flags;
	__u8  PathBuffer[];
} __packed;

/* SMB 3.1.1 and later dialects. See MS-SMB2 section 2.2.2.1 */
struct smb2_error_context_rsp {
	__le32 ErrorDataLength;
	__le32 ErrorId;
	__u8  ErrorContextData; /* ErrorDataLength long array */
} __packed;

/* ErrorId values */
#define SMB2_ERROR_ID_DEFAULT		0x00000000
#define SMB2_ERROR_ID_SHARE_REDIRECT	cpu_to_le32(0x72645253)	/* "rdRS" */

/* Defines for Type field below (see MS-SMB2 2.2.2.2.2.1) */
#define MOVE_DST_IPADDR_V4	cpu_to_le32(0x00000001)
#define MOVE_DST_IPADDR_V6	cpu_to_le32(0x00000002)

struct move_dst_ipaddr {
	__le32 Type;
	__u32  Reserved;
	__u8   address[16]; /* IPv4 followed by 12 bytes rsvd or IPv6 address */
} __packed;

struct share_redirect_error_context_rsp {
	__le32 StructureSize;
	__le32 NotificationType;
	__le32 ResourceNameOffset;
	__le32 ResourceNameLength;
	__le16 Reserved;
	__le16 TargetType;
	__le32 IPAddrCount;
	struct move_dst_ipaddr IpAddrMoveList[];
	/* __u8 ResourceName[] */ /* Name of share as counted Unicode string */
} __packed;

<<<<<<< HEAD
#define SMB2_CLIENT_GUID_SIZE 16

struct smb2_negotiate_req {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize; /* Must be 36 */
	__le16 DialectCount;
	__le16 SecurityMode;
	__le16 Reserved;	/* MBZ */
	__le32 Capabilities;
	__u8   ClientGUID[SMB2_CLIENT_GUID_SIZE];
	/* In SMB3.02 and earlier next three were MBZ le64 ClientStartTime */
	__le32 NegotiateContextOffset; /* SMB3.1.1 only. MBZ earlier */
	__le16 NegotiateContextCount;  /* SMB3.1.1 only. MBZ earlier */
	__le16 Reserved2;
	__le16 Dialects[4]; /* BB expand this if autonegotiate > 4 dialects */
} __packed;

/* Dialects */
#define SMB10_PROT_ID 0x0000 /* local only, not sent on wire w/CIFS negprot */
#define SMB20_PROT_ID 0x0202
#define SMB21_PROT_ID 0x0210
#define SMB30_PROT_ID 0x0300
#define SMB302_PROT_ID 0x0302
#define SMB311_PROT_ID 0x0311
#define BAD_PROT_ID   0xFFFF

/* SecurityMode flags */
#define	SMB2_NEGOTIATE_SIGNING_ENABLED	0x0001
#define SMB2_NEGOTIATE_SIGNING_REQUIRED	0x0002
#define SMB2_SEC_MODE_FLAGS_ALL		0x0003

/* Capabilities flags */
#define SMB2_GLOBAL_CAP_DFS		0x00000001
#define SMB2_GLOBAL_CAP_LEASING		0x00000002 /* Resp only New to SMB2.1 */
#define SMB2_GLOBAL_CAP_LARGE_MTU	0X00000004 /* Resp only New to SMB2.1 */
#define SMB2_GLOBAL_CAP_MULTI_CHANNEL	0x00000008 /* New to SMB3 */
#define SMB2_GLOBAL_CAP_PERSISTENT_HANDLES 0x00000010 /* New to SMB3 */
#define SMB2_GLOBAL_CAP_DIRECTORY_LEASING  0x00000020 /* New to SMB3 */
#define SMB2_GLOBAL_CAP_ENCRYPTION	0x00000040 /* New to SMB3 */
/* Internal types */
#define SMB2_NT_FIND			0x00100000
#define SMB2_LARGE_FILES		0x00200000


/* Negotiate Contexts - ContextTypes. See MS-SMB2 section 2.2.3.1 for details */
#define SMB2_PREAUTH_INTEGRITY_CAPABILITIES	cpu_to_le16(1)
#define SMB2_ENCRYPTION_CAPABILITIES		cpu_to_le16(2)
#define SMB2_COMPRESSION_CAPABILITIES		cpu_to_le16(3)
#define SMB2_NETNAME_NEGOTIATE_CONTEXT_ID	cpu_to_le16(5)
#define SMB2_TRANSPORT_CAPABILITIES		cpu_to_le16(6)
#define SMB2_RDMA_TRANSFORM_CAPABILITIES	cpu_to_le16(7)
#define SMB2_SIGNING_CAPABILITIES		cpu_to_le16(8)
#define SMB2_POSIX_EXTENSIONS_AVAILABLE		cpu_to_le16(0x100)

struct smb2_neg_context {
	__le16	ContextType;
	__le16	DataLength;
	__le32	Reserved;
	/* Followed by array of data */
} __packed;

#define SMB311_LINUX_CLIENT_SALT_SIZE			32
/* Hash Algorithm Types */
#define SMB2_PREAUTH_INTEGRITY_SHA512	cpu_to_le16(0x0001)
#define SMB2_PREAUTH_HASH_SIZE 64

/*
 * SaltLength that the server send can be zero, so the only three required
 * fields (all __le16) end up six bytes total, so the minimum context data len
 * in the response is six bytes which accounts for
 *
 *      HashAlgorithmCount, SaltLength, and 1 HashAlgorithm.
 */
#define MIN_PREAUTH_CTXT_DATA_LEN 6

struct smb2_preauth_neg_context {
	__le16	ContextType; /* 1 */
	__le16	DataLength;
	__le32	Reserved;
	__le16	HashAlgorithmCount; /* 1 */
	__le16	SaltLength;
	__le16	HashAlgorithms; /* HashAlgorithms[0] since only one defined */
	__u8	Salt[SMB311_LINUX_CLIENT_SALT_SIZE];
} __packed;

/* Encryption Algorithms Ciphers */
#define SMB2_ENCRYPTION_AES128_CCM	cpu_to_le16(0x0001)
#define SMB2_ENCRYPTION_AES128_GCM	cpu_to_le16(0x0002)
/* we currently do not request AES256_CCM since presumably GCM faster */
#define SMB2_ENCRYPTION_AES256_CCM      cpu_to_le16(0x0003)
#define SMB2_ENCRYPTION_AES256_GCM      cpu_to_le16(0x0004)

/* Min encrypt context data is one cipher so 2 bytes + 2 byte count field */
#define MIN_ENCRYPT_CTXT_DATA_LEN	4
struct smb2_encryption_neg_context {
	__le16	ContextType; /* 2 */
	__le16	DataLength;
	__le32	Reserved;
	/* CipherCount usally 2, but can be 3 when AES256-GCM enabled */
	__le16	CipherCount; /* AES128-GCM and AES128-CCM by default */
	__le16	Ciphers[3];
} __packed;

/* See MS-SMB2 2.2.3.1.3 */
#define SMB3_COMPRESS_NONE	cpu_to_le16(0x0000)
#define SMB3_COMPRESS_LZNT1	cpu_to_le16(0x0001)
#define SMB3_COMPRESS_LZ77	cpu_to_le16(0x0002)
#define SMB3_COMPRESS_LZ77_HUFF	cpu_to_le16(0x0003)
/* Pattern scanning algorithm See MS-SMB2 3.1.4.4.1 */
#define SMB3_COMPRESS_PATTERN	cpu_to_le16(0x0004) /* Pattern_V1 */

/* Compression Flags */
#define SMB2_COMPRESSION_CAPABILITIES_FLAG_NONE		cpu_to_le32(0x00000000)
#define SMB2_COMPRESSION_CAPABILITIES_FLAG_CHAINED	cpu_to_le32(0x00000001)

struct smb2_compression_capabilities_context {
	__le16	ContextType; /* 3 */
	__le16  DataLength;
	__u32	Reserved;
	__le16	CompressionAlgorithmCount;
	__u16	Padding;
	__u32	Flags;
	__le16	CompressionAlgorithms[3];
} __packed;

/*
 * For smb2_netname_negotiate_context_id See MS-SMB2 2.2.3.1.4.
 * Its struct simply contains NetName, an array of Unicode characters
 */
struct smb2_netname_neg_context {
	__le16	ContextType; /* 5 */
	__le16	DataLength;
	__le32	Reserved;
	__le16	NetName[]; /* hostname of target converted to UCS-2 */
} __packed;

/*
 * For rdma transform capabilities context see MS-SMB2 2.2.3.1.6
 * and 2.2.4.1.5
 */

/* RDMA Transform IDs */
#define SMB2_RDMA_TRANSFORM_NONE	0x0000
#define SMB2_RDMA_TRANSFORM_ENCRYPTION	0x0001

struct smb2_rdma_transform_capabilities_context {
	__le16	ContextType; /* 7 */
	__le16  DataLength;
	__u32	Reserved;
	__le16	TransformCount;
	__u16	Reserved1;
	__u32	Reserved2;
	__le16	RDMATransformIds[1];
} __packed;

/* Signing algorithms */
#define SIGNING_ALG_HMAC_SHA256	0
#define SIGNING_ALG_AES_CMAC	1
#define SIGNING_ALG_AES_GMAC	2

struct smb2_signing_capabilities {
	__le16	ContextType; /* 8 */
	__le16	DataLength;
	__u32	Reserved;
	__le16	SigningAlgorithmCount;
	__le16	SigningAlgorithms[];
} __packed;

#define POSIX_CTXT_DATA_LEN	16
struct smb2_posix_neg_context {
	__le16	ContextType; /* 0x100 */
	__le16	DataLength;
	__le32	Reserved;
	__u8	Name[16]; /* POSIX ctxt GUID 93AD25509CB411E7B42383DE968BCD7C */
} __packed;

struct smb2_negotiate_rsp {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 65 */
	__le16 SecurityMode;
	__le16 DialectRevision;
	__le16 NegotiateContextCount;	/* Prior to SMB3.1.1 was Reserved & MBZ */
	__u8   ServerGUID[16];
	__le32 Capabilities;
	__le32 MaxTransactSize;
	__le32 MaxReadSize;
	__le32 MaxWriteSize;
	__le64 SystemTime;	/* MBZ */
	__le64 ServerStartTime;
	__le16 SecurityBufferOffset;
	__le16 SecurityBufferLength;
	__le32 NegotiateContextOffset;	/* Pre:SMB3.1.1 was reserved/ignored */
	__u8   Buffer[1];	/* variable length GSS security buffer */
} __packed;

/* Flags */
#define SMB2_SESSION_REQ_FLAG_BINDING		0x01
#define SMB2_SESSION_REQ_FLAG_ENCRYPT_DATA	0x04

struct smb2_sess_setup_req {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize; /* Must be 25 */
	__u8   Flags;
	__u8   SecurityMode;
	__le32 Capabilities;
	__le32 Channel;
	__le16 SecurityBufferOffset;
	__le16 SecurityBufferLength;
	__u64 PreviousSessionId;
	__u8   Buffer[1];	/* variable length GSS security buffer */
} __packed;

/* Currently defined SessionFlags */
#define SMB2_SESSION_FLAG_IS_GUEST	0x0001
#define SMB2_SESSION_FLAG_IS_NULL	0x0002
#define SMB2_SESSION_FLAG_ENCRYPT_DATA	0x0004
struct smb2_sess_setup_rsp {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize; /* Must be 9 */
	__le16 SessionFlags;
	__le16 SecurityBufferOffset;
	__le16 SecurityBufferLength;
	__u8   Buffer[1];	/* variable length GSS security buffer */
} __packed;

struct smb2_logoff_req {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 4 */
	__le16 Reserved;
} __packed;

struct smb2_logoff_rsp {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 4 */
	__le16 Reserved;
} __packed;

/* Flags/Reserved for SMB3.1.1 */
#define SMB2_TREE_CONNECT_FLAG_CLUSTER_RECONNECT cpu_to_le16(0x0001)
#define SMB2_TREE_CONNECT_FLAG_REDIRECT_TO_OWNER cpu_to_le16(0x0002)
#define SMB2_TREE_CONNECT_FLAG_EXTENSION_PRESENT cpu_to_le16(0x0004)

struct smb2_tree_connect_req {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 9 */
	__le16 Flags; /* Reserved MBZ for dialects prior to SMB3.1.1 */
	__le16 PathOffset;
	__le16 PathLength;
	__u8   Buffer[1];	/* variable length */
} __packed;

/* See MS-SMB2 section 2.2.9.2 */
/* Context Types */
#define SMB2_RESERVED_TREE_CONNECT_CONTEXT_ID 0x0000
#define SMB2_REMOTED_IDENTITY_TREE_CONNECT_CONTEXT_ID cpu_to_le16(0x0001)

struct tree_connect_contexts {
	__le16 ContextType;
	__le16 DataLength;
	__le32 Reserved;
	__u8   Data[];
} __packed;

/* Remoted identity tree connect context structures - see MS-SMB2 2.2.9.2.1 */
struct smb3_blob_data {
	__le16 BlobSize;
	__u8   BlobData[];
} __packed;

/* Valid values for Attr */
#define SE_GROUP_MANDATORY		0x00000001
#define SE_GROUP_ENABLED_BY_DEFAULT	0x00000002
#define SE_GROUP_ENABLED		0x00000004
#define SE_GROUP_OWNER			0x00000008
#define SE_GROUP_USE_FOR_DENY_ONLY	0x00000010
#define SE_GROUP_INTEGRITY		0x00000020
#define SE_GROUP_INTEGRITY_ENABLED	0x00000040
#define SE_GROUP_RESOURCE		0x20000000
#define SE_GROUP_LOGON_ID		0xC0000000

/* struct sid_attr_data is SidData array in BlobData format then le32 Attr */

struct sid_array_data {
	__le16 SidAttrCount;
	/* SidAttrList - array of sid_attr_data structs */
} __packed;

struct luid_attr_data {

} __packed;

/*
 * struct privilege_data is the same as BLOB_DATA - see MS-SMB2 2.2.9.2.1.5
 * but with size of LUID_ATTR_DATA struct and BlobData set to LUID_ATTR DATA
 */

struct privilege_array_data {
	__le16 PrivilegeCount;
	/* array of privilege_data structs */
} __packed;

struct remoted_identity_tcon_context {
	__le16 TicketType; /* must be 0x0001 */
	__le16 TicketSize; /* total size of this struct */
	__le16 User; /* offset to SID_ATTR_DATA struct with user info */
	__le16 UserName; /* offset to null terminated Unicode username string */
	__le16 Domain; /* offset to null terminated Unicode domain name */
	__le16 Groups; /* offset to SID_ARRAY_DATA struct with group info */
	__le16 RestrictedGroups; /* similar to above */
	__le16 Privileges; /* offset to PRIVILEGE_ARRAY_DATA struct */
	__le16 PrimaryGroup; /* offset to SID_ARRAY_DATA struct */
	__le16 Owner; /* offset to BLOB_DATA struct */
	__le16 DefaultDacl; /* offset to BLOB_DATA struct */
	__le16 DeviceGroups; /* offset to SID_ARRAY_DATA struct */
	__le16 UserClaims; /* offset to BLOB_DATA struct */
	__le16 DeviceClaims; /* offset to BLOB_DATA struct */
	__u8   TicketInfo[]; /* variable length buf - remoted identity data */
} __packed;

struct smb2_tree_connect_req_extension {
	__le32 TreeConnectContextOffset;
	__le16 TreeConnectContextCount;
	__u8  Reserved[10];
	__u8  PathName[]; /* variable sized array */
	/* followed by array of TreeConnectContexts */
} __packed;

struct smb2_tree_connect_rsp {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 16 */
	__u8   ShareType;  /* see below */
	__u8   Reserved;
	__le32 ShareFlags; /* see below */
	__le32 Capabilities; /* see below */
	__le32 MaximalAccess;
} __packed;

/* Possible ShareType values */
#define SMB2_SHARE_TYPE_DISK	0x01
#define SMB2_SHARE_TYPE_PIPE	0x02
#define	SMB2_SHARE_TYPE_PRINT	0x03

/*
 * Possible ShareFlags - exactly one and only one of the first 4 caching flags
 * must be set (any of the remaining, SHI1005, flags may be set individually
 * or in combination.
 */
#define SMB2_SHAREFLAG_MANUAL_CACHING			0x00000000
#define SMB2_SHAREFLAG_AUTO_CACHING			0x00000010
#define SMB2_SHAREFLAG_VDO_CACHING			0x00000020
#define SMB2_SHAREFLAG_NO_CACHING			0x00000030
#define SHI1005_FLAGS_DFS				0x00000001
#define SHI1005_FLAGS_DFS_ROOT				0x00000002
#define SHI1005_FLAGS_RESTRICT_EXCLUSIVE_OPENS		0x00000100
#define SHI1005_FLAGS_FORCE_SHARED_DELETE		0x00000200
#define SHI1005_FLAGS_ALLOW_NAMESPACE_CACHING		0x00000400
#define SHI1005_FLAGS_ACCESS_BASED_DIRECTORY_ENUM	0x00000800
#define SHI1005_FLAGS_FORCE_LEVELII_OPLOCK		0x00001000
#define SHI1005_FLAGS_ENABLE_HASH_V1			0x00002000
#define SHI1005_FLAGS_ENABLE_HASH_V2			0x00004000
#define SHI1005_FLAGS_ENCRYPT_DATA			0x00008000
#define SMB2_SHAREFLAG_IDENTITY_REMOTING		0x00040000 /* 3.1.1 */
#define SHI1005_FLAGS_ALL				0x0004FF33

/* Possible share capabilities */
#define SMB2_SHARE_CAP_DFS	cpu_to_le32(0x00000008) /* all dialects */
#define SMB2_SHARE_CAP_CONTINUOUS_AVAILABILITY cpu_to_le32(0x00000010) /* 3.0 */
#define SMB2_SHARE_CAP_SCALEOUT	cpu_to_le32(0x00000020) /* 3.0 */
#define SMB2_SHARE_CAP_CLUSTER	cpu_to_le32(0x00000040) /* 3.0 */
#define SMB2_SHARE_CAP_ASYMMETRIC cpu_to_le32(0x00000080) /* 3.02 */
#define SMB2_SHARE_CAP_REDIRECT_TO_OWNER cpu_to_le32(0x00000100) /* 3.1.1 */

struct smb2_tree_disconnect_req {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 4 */
	__le16 Reserved;
} __packed;

struct smb2_tree_disconnect_rsp {
	struct smb2_sync_hdr sync_hdr;
	__le16 StructureSize;	/* Must be 4 */
	__le16 Reserved;
} __packed;

/* File Attrubutes */
#define FILE_ATTRIBUTE_READONLY			0x00000001
#define FILE_ATTRIBUTE_HIDDEN			0x00000002
#define FILE_ATTRIBUTE_SYSTEM			0x00000004
#define FILE_ATTRIBUTE_DIRECTORY		0x00000010
#define FILE_ATTRIBUTE_ARCHIVE			0x00000020
#define FILE_ATTRIBUTE_NORMAL			0x00000080
#define FILE_ATTRIBUTE_TEMPORARY		0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE		0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT		0x00000400
#define FILE_ATTRIBUTE_COMPRESSED		0x00000800
#define FILE_ATTRIBUTE_OFFLINE			0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED	0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED		0x00004000
#define FILE_ATTRIBUTE_INTEGRITY_STREAM		0x00008000
#define FILE_ATTRIBUTE_NO_SCRUB_DATA		0x00020000

/* Oplock levels */
#define SMB2_OPLOCK_LEVEL_NONE		0x00
#define SMB2_OPLOCK_LEVEL_II		0x01
#define SMB2_OPLOCK_LEVEL_EXCLUSIVE	0x08
#define SMB2_OPLOCK_LEVEL_BATCH		0x09
#define SMB2_OPLOCK_LEVEL_LEASE		0xFF
/* Non-spec internal type */
#define SMB2_OPLOCK_LEVEL_NOCHANGE	0x99

/* Desired Access Flags */
#define FILE_READ_DATA_LE		cpu_to_le32(0x00000001)
#define FILE_WRITE_DATA_LE		cpu_to_le32(0x00000002)
#define FILE_APPEND_DATA_LE		cpu_to_le32(0x00000004)
#define FILE_READ_EA_LE			cpu_to_le32(0x00000008)
#define FILE_WRITE_EA_LE		cpu_to_le32(0x00000010)
#define FILE_EXECUTE_LE			cpu_to_le32(0x00000020)
#define FILE_READ_ATTRIBUTES_LE		cpu_to_le32(0x00000080)
#define FILE_WRITE_ATTRIBUTES_LE	cpu_to_le32(0x00000100)
#define FILE_DELETE_LE			cpu_to_le32(0x00010000)
#define FILE_READ_CONTROL_LE		cpu_to_le32(0x00020000)
#define FILE_WRITE_DAC_LE		cpu_to_le32(0x00040000)
#define FILE_WRITE_OWNER_LE		cpu_to_le32(0x00080000)
#define FILE_SYNCHRONIZE_LE		cpu_to_le32(0x00100000)
#define FILE_ACCESS_SYSTEM_SECURITY_LE	cpu_to_le32(0x01000000)
#define FILE_MAXIMAL_ACCESS_LE		cpu_to_le32(0x02000000)
#define FILE_GENERIC_ALL_LE		cpu_to_le32(0x10000000)
#define FILE_GENERIC_EXECUTE_LE		cpu_to_le32(0x20000000)
#define FILE_GENERIC_WRITE_LE		cpu_to_le32(0x40000000)
#define FILE_GENERIC_READ_LE		cpu_to_le32(0x80000000)

/* ShareAccess Flags */
#define FILE_SHARE_READ_LE		cpu_to_le32(0x00000001)
#define FILE_SHARE_WRITE_LE		cpu_to_le32(0x00000002)
#define FILE_SHARE_DELETE_LE		cpu_to_le32(0x00000004)
#define FILE_SHARE_ALL_LE		cpu_to_le32(0x00000007)

/* CreateDisposition Flags */
#define FILE_SUPERSEDE_LE		cpu_to_le32(0x00000000)
#define FILE_OPEN_LE			cpu_to_le32(0x00000001)
#define FILE_CREATE_LE			cpu_to_le32(0x00000002)
#define	FILE_OPEN_IF_LE			cpu_to_le32(0x00000003)
#define FILE_OVERWRITE_LE		cpu_to_le32(0x00000004)
#define FILE_OVERWRITE_IF_LE		cpu_to_le32(0x00000005)

/* CreateOptions Flags */
#define FILE_DIRECTORY_FILE_LE		cpu_to_le32(0x00000001)
/* same as #define CREATE_NOT_FILE_LE	cpu_to_le32(0x00000001) */
#define FILE_WRITE_THROUGH_LE		cpu_to_le32(0x00000002)
#define FILE_SEQUENTIAL_ONLY_LE		cpu_to_le32(0x00000004)
#define FILE_NO_INTERMEDIATE_BUFFERRING_LE cpu_to_le32(0x00000008)
#define FILE_SYNCHRONOUS_IO_ALERT_LE	cpu_to_le32(0x00000010)
#define FILE_SYNCHRONOUS_IO_NON_ALERT_LE	cpu_to_le32(0x00000020)
#define FILE_NON_DIRECTORY_FILE_LE	cpu_to_le32(0x00000040)
#define FILE_COMPLETE_IF_OPLOCKED_LE	cpu_to_le32(0x00000100)
#define FILE_NO_EA_KNOWLEDGE_LE		cpu_to_le32(0x00000200)
#define FILE_RANDOM_ACCESS_LE		cpu_to_le32(0x00000800)
#define FILE_DELETE_ON_CLOSE_LE		cpu_to_le32(0x00001000)
#define FILE_OPEN_BY_FILE_ID_LE		cpu_to_le32(0x00002000)
#define FILE_OPEN_FOR_BACKUP_INTENT_LE	cpu_to_le32(0x00004000)
#define FILE_NO_COMPRESSION_LE		cpu_to_le32(0x00008000)
#define FILE_RESERVE_OPFILTER_LE	cpu_to_le32(0x00100000)
#define FILE_OPEN_REPARSE_POINT_LE	cpu_to_le32(0x00200000)
#define FILE_OPEN_NO_RECALL_LE		cpu_to_le32(0x00400000)
#define FILE_OPEN_FOR_FREE_SPACE_QUERY_LE cpu_to_le32(0x00800000)

#define FILE_READ_RIGHTS_LE (FILE_READ_DATA_LE | FILE_READ_EA_LE \
			| FILE_READ_ATTRIBUTES_LE)
#define FILE_WRITE_RIGHTS_LE (FILE_WRITE_DATA_LE | FILE_APPEND_DATA_LE \
			| FILE_WRITE_EA_LE | FILE_WRITE_ATTRIBUTES_LE)
#define FILE_EXEC_RIGHTS_LE (FILE_EXECUTE_LE)

/* Impersonation Levels. See MS-WPO section 9.7 and MSDN-IMPERS */
#define IL_ANONYMOUS		cpu_to_le32(0x00000000)
#define IL_IDENTIFICATION	cpu_to_le32(0x00000001)
#define IL_IMPERSONATION	cpu_to_le32(0x00000002)
#define IL_DELEGATE		cpu_to_le32(0x00000003)

/* Create Context Values */
#define SMB2_CREATE_EA_BUFFER			"ExtA" /* extended attributes */
#define SMB2_CREATE_SD_BUFFER			"SecD" /* security descriptor */
#define SMB2_CREATE_DURABLE_HANDLE_REQUEST	"DHnQ"
#define SMB2_CREATE_DURABLE_HANDLE_RECONNECT	"DHnC"
#define SMB2_CREATE_ALLOCATION_SIZE		"AISi"
#define SMB2_CREATE_QUERY_MAXIMAL_ACCESS_REQUEST "MxAc"
#define SMB2_CREATE_TIMEWARP_REQUEST		"TWrp"
#define SMB2_CREATE_QUERY_ON_DISK_ID		"QFid"
#define SMB2_CREATE_REQUEST_LEASE		"RqLs"
#define SMB2_CREATE_DURABLE_HANDLE_REQUEST_V2	"DH2Q"
#define SMB2_CREATE_DURABLE_HANDLE_RECONNECT_V2	"DH2C"
#define SMB2_CREATE_APP_INSTANCE_ID	0x45BCA66AEFA7F74A9008FA462E144D74
#define SMB2_CREATE_APP_INSTANCE_VERSION 0xB982D0B73B56074FA07B524A8116A010
#define SVHDX_OPEN_DEVICE_CONTEX	0x9CCBCF9E04C1E643980E158DA1F6EC83
#define SMB2_CREATE_TAG_POSIX		0x93AD25509CB411E7B42383DE968BCD7C

/* Flag (SMB3 open response) values */
#define SMB2_CREATE_FLAG_REPARSEPOINT 0x01

=======
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
/*
 * Maximum number of iovs we need for an open/create request.
 * [0] : struct smb2_create_req
 * [1] : path
 * [2] : lease context
 * [3] : durable context
 * [4] : posix context
 * [5] : time warp context
 * [6] : query id context
 * [7] : compound padding
 */
#define SMB2_CREATE_IOV_SIZE 8

/*
 * Maximum size of a SMB2_CREATE response is 64 (smb2 header) +
 * 88 (fixed part of create response) + 520 (path) + 208 (contexts) +
 * 2 bytes of padding.
 */
#define MAX_SMB2_CREATE_RESPONSE_SIZE 880

#define SMB2_LEASE_READ_CACHING_HE	0x01
#define SMB2_LEASE_HANDLE_CACHING_HE	0x02
#define SMB2_LEASE_WRITE_CACHING_HE	0x04

struct create_durable {
	struct create_context ccontext;
	__u8   Name[8];
	union {
		__u8  Reserved[16];
		struct {
			__u64 PersistentFileId;
			__u64 VolatileFileId;
		} Fid;
	} Data;
} __packed;

/* See MS-SMB2 2.2.13.2.11 */
/* Flags */
#define SMB2_DHANDLE_FLAG_PERSISTENT	0x00000002
struct durable_context_v2 {
	__le32 Timeout;
	__le32 Flags;
	__u64 Reserved;
	__u8 CreateGuid[16];
} __packed;

struct create_durable_v2 {
	struct create_context ccontext;
	__u8   Name[8];
	struct durable_context_v2 dcontext;
} __packed;

/* See MS-SMB2 2.2.13.2.12 */
struct durable_reconnect_context_v2 {
	struct {
		__u64 PersistentFileId;
		__u64 VolatileFileId;
	} Fid;
	__u8 CreateGuid[16];
	__le32 Flags; /* see above DHANDLE_FLAG_PERSISTENT */
} __packed;

/* See MS-SMB2 2.2.14.2.9 */
struct create_on_disk_id {
	struct create_context ccontext;
	__u8   Name[8];
	__le64 DiskFileId;
	__le64 VolumeId;
	__u32  Reserved[4];
} __packed;

/* See MS-SMB2 2.2.14.2.12 */
struct durable_reconnect_context_v2_rsp {
	__le32 Timeout;
	__le32 Flags; /* see above DHANDLE_FLAG_PERSISTENT */
} __packed;

struct create_durable_handle_reconnect_v2 {
	struct create_context ccontext;
	__u8   Name[8];
	struct durable_reconnect_context_v2 dcontext;
	__u8   Pad[4];
} __packed;

/* See MS-SMB2 2.2.13.2.5 */
struct crt_twarp_ctxt {
	struct create_context ccontext;
	__u8	Name[8];
	__le64	Timestamp;

} __packed;

/* See MS-SMB2 2.2.13.2.9 */
struct crt_query_id_ctxt {
	struct create_context ccontext;
	__u8	Name[8];
} __packed;

struct crt_sd_ctxt {
	struct create_context ccontext;
	__u8	Name[8];
	struct smb3_sd sd;
} __packed;


#define COPY_CHUNK_RES_KEY_SIZE	24
struct resume_key_req {
	char ResumeKey[COPY_CHUNK_RES_KEY_SIZE];
	__le32	ContextLength;	/* MBZ */
	char	Context[];	/* ignored, Windows sets to 4 bytes of zero */
} __packed;

/* this goes in the ioctl buffer when doing a copychunk request */
struct copychunk_ioctl {
	char SourceKey[COPY_CHUNK_RES_KEY_SIZE];
	__le32 ChunkCount; /* we are only sending 1 */
	__le32 Reserved;
	/* array will only be one chunk long for us */
	__le64 SourceOffset;
	__le64 TargetOffset;
	__le32 Length; /* how many bytes to copy */
	__u32 Reserved2;
} __packed;

struct copychunk_ioctl_rsp {
	__le32 ChunksWritten;
	__le32 ChunkBytesWritten;
	__le32 TotalBytesWritten;
} __packed;

/* See MS-FSCC 2.3.29 and 2.3.30 */
struct get_retrieval_pointer_count_req {
	__le64 StartingVcn; /* virtual cluster number (signed) */
} __packed;

struct get_retrieval_pointer_count_rsp {
	__le32 ExtentCount;
} __packed;

/*
 * See MS-FSCC 2.3.33 and 2.3.34
 * request is the same as get_retrieval_point_count_req struct above
 */
struct smb3_extents {
	__le64 NextVcn;
	__le64 Lcn; /* logical cluster number */
} __packed;

struct get_retrieval_pointers_refcount_rsp {
	__le32 ExtentCount;
	__u32  Reserved;
	__le64 StartingVcn;
	struct smb3_extents extents[];
} __packed;

struct fsctl_set_integrity_information_req {
	__le16	ChecksumAlgorithm;
	__le16	Reserved;
	__le32	Flags;
} __packed;

struct fsctl_get_integrity_information_rsp {
	__le16	ChecksumAlgorithm;
	__le16	Reserved;
	__le32	Flags;
	__le32	ChecksumChunkSizeInBytes;
	__le32	ClusterSizeInBytes;
} __packed;

/* Integrity ChecksumAlgorithm choices for above */
#define	CHECKSUM_TYPE_NONE	0x0000
#define	CHECKSUM_TYPE_CRC64	0x0002
#define CHECKSUM_TYPE_UNCHANGED	0xFFFF	/* set only */

/* Integrity flags for above */
#define FSCTL_INTEGRITY_FLAG_CHECKSUM_ENFORCEMENT_OFF	0x00000001

/* See MS-DFSC 2.2.2 */
struct fsctl_get_dfs_referral_req {
	__le16 MaxReferralLevel;
	__u8 RequestFileName[];
} __packed;

/* DFS response is struct get_dfs_refer_rsp */

/* See MS-SMB2 2.2.31.3 */
struct network_resiliency_req {
	__le32 Timeout;
	__le32 Reserved;
} __packed;
/* There is no buffer for the response ie no struct network_resiliency_rsp */

#define RSS_CAPABLE	cpu_to_le32(0x00000001)
#define RDMA_CAPABLE	cpu_to_le32(0x00000002)

#define INTERNETWORK	cpu_to_le16(0x0002)
#define INTERNETWORKV6	cpu_to_le16(0x0017)

struct network_interface_info_ioctl_rsp {
	__le32 Next; /* next interface. zero if this is last one */
	__le32 IfIndex;
	__le32 Capability; /* RSS or RDMA Capable */
	__le32 Reserved;
	__le64 LinkSpeed;
	__le16 Family;
	__u8 Buffer[126];
} __packed;

struct iface_info_ipv4 {
	__be16 Port;
	__be32 IPv4Address;
	__be64 Reserved;
} __packed;

struct iface_info_ipv6 {
	__be16 Port;
	__be32 FlowInfo;
	__u8   IPv6Address[16];
	__be32 ScopeId;
} __packed;

#define NO_FILE_ID 0xFFFFFFFFFFFFFFFFULL /* general ioctls to srv not to file */

struct compress_ioctl {
	__le16 CompressionState; /* See cifspdu.h for possible flag values */
} __packed;

/*
 * Maximum number of iovs we need for an ioctl request.
 * [0] : struct smb2_ioctl_req
 * [1] : in_data
 */
#define SMB2_IOCTL_IOV_SIZE 2

/*
 *	PDU query infolevel structure definitions
 *	BB consider moving to a different header
 */

struct smb2_file_full_ea_info { /* encoding of response for level 15 */
	__le32 next_entry_offset;
	__u8   flags;
	__u8   ea_name_length;
	__le16 ea_value_length;
	char   ea_data[]; /* \0 terminated name plus value */
} __packed; /* level 15 Set */

struct smb2_file_reparse_point_info {
	__le64 IndexNumber;
	__le32 Tag;
} __packed;

struct smb2_file_network_open_info {
	__le64 CreationTime;
	__le64 LastAccessTime;
	__le64 LastWriteTime;
	__le64 ChangeTime;
	__le64 AllocationSize;
	__le64 EndOfFile;
	__le32 Attributes;
	__le32 Reserved;
} __packed; /* level 34 Query also similar returned in close rsp and open rsp */

/* See MS-FSCC 2.4.21 */
struct smb2_file_id_information {
	__le64	VolumeSerialNumber;
	__u64  PersistentFileId; /* opaque endianness */
	__u64  VolatileFileId; /* opaque endianness */
} __packed; /* level 59 */

/* See MS-FSCC 2.4.18 */
struct smb2_file_id_extd_directory_info {
	__le32 NextEntryOffset;
	__u32 FileIndex;
	__le64 CreationTime;
	__le64 LastAccessTime;
	__le64 LastWriteTime;
	__le64 ChangeTime;
	__le64 EndOfFile;
	__le64 AllocationSize;
	__le32 FileAttributes;
	__le32 FileNameLength;
	__le32 EaSize; /* EA size */
	__le32 ReparsePointTag; /* valid if FILE_ATTR_REPARSE_POINT set in FileAttributes */
	__le64 UniqueId; /* inode num - le since Samba puts ino in low 32 bit */
	char FileName[1];
} __packed; /* level 60 */

extern char smb2_padding[7];

/* equivalent of the contents of SMB3.1.1 POSIX open context response */
struct create_posix_rsp {
	u32 nlink;
	u32 reparse_tag;
	u32 mode;
	struct cifs_sid owner; /* var-sized on the wire */
	struct cifs_sid group; /* var-sized on the wire */
} __packed;

#define SMB2_QUERY_DIRECTORY_IOV_SIZE 2

/*
 * SMB2-only POSIX info level for query dir
 *
 * See posix_info_sid_size(), posix_info_extra_size() and
 * posix_info_parse() to help with the handling of this struct.
 */
struct smb2_posix_info {
	__le32 NextEntryOffset;
	__u32 Ignored;
	__le64 CreationTime;
	__le64 LastAccessTime;
	__le64 LastWriteTime;
	__le64 ChangeTime;
	__le64 EndOfFile;
	__le64 AllocationSize;
	__le32 DosAttributes;
	__le64 Inode;
	__le32 DeviceId;
	__le32 Zero;
	/* beginning of POSIX Create Context Response */
	__le32 HardLinks;
	__le32 ReparseTag;
	__le32 Mode;
	/*
	 * var sized owner SID
	 * var sized group SID
	 * le32 filenamelength
	 * u8  filename[]
	 */
} __packed;

/*
 * Parsed version of the above struct. Allows direct access to the
 * variable length fields
 */
struct smb2_posix_info_parsed {
	const struct smb2_posix_info *base;
	size_t size;
	struct cifs_sid owner;
	struct cifs_sid group;
	int name_len;
	const u8 *name;
};

#endif				/* _SMB2PDU_H */
