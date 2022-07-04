/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#ifndef LIBVFN_NVME_TYPES_H
#define LIBVFN_NVME_TYPES_H

enum nvme_sgld_type {
	NVME_SGLD_TYPE_DATA_BLOCK	= 0x0,
	NVME_SGLD_TYPE_BIT_BUCKET	= 0x1,
	NVME_SGLD_TYPE_SEGMENT		= 0x2,
	NVME_SGLD_TYPE_LAST_SEGMENT	= 0x3,
};

struct nvme_sgld {
	leint64_t addr;
	leint32_t len;
	uint8_t   rsvd12[3];
	uint8_t   type;
};
__static_assert(sizeof(struct nvme_sgld) == 16);

#define NVME_SGLD_TYPE_MASK 0xf
#define NVME_SGLD_TYPE_SHIFT 4

union nvme_dptr {
	struct {
		leint64_t prp1;
		leint64_t prp2;
	};

	struct nvme_sgld sgl;
};
__static_assert(sizeof(union nvme_dptr) == 16);

struct nvme_cmd_identify {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t nsid;
	leint64_t rsvd2[2];
	union nvme_dptr dptr;
	uint8_t   cns;
	uint8_t   rsvd3;
	leint16_t ctrlid;
	uint8_t   rsvd11[3];
	uint8_t   csi;
	leint32_t rsvd12[4];
};
__static_assert(sizeof(struct nvme_cmd_identify) == 64);

/**
 * enum nvme_cmd_create_q_flags - Create queue flags
 * @NVME_Q_PC: Queue is physical contiguous
 * @NVME_CQ_IEN: Interrupts enabled
 * @NVME_SQ_QPRIO_LOW: Queue priority class low
 * @NVME_SQ_QPRIO_MEDIUM: Queue priority class medium
 * @NVME_SQ_QPRIO_HIGH: Queue priority class high
 * @NVME_SQ_QPRIO_URGENT: Queue priority class urgent
 */
enum nvme_cmd_create_q_flags {
	NVME_Q_PC		= 1 << 0,
	NVME_CQ_IEN		= 1 << 1,
	NVME_SQ_QPRIO_LOW	= 3 << 1,
	NVME_SQ_QPRIO_MEDIUM	= 2 << 1,
	NVME_SQ_QPRIO_HIGH	= 1 << 1,
	NVME_SQ_QPRIO_URGENT	= 0 << 1,
};

struct nvme_cmd_create_cq {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t rsvd1[5];
	leint64_t prp1;
	leint64_t rsvd8;
	leint16_t qid;
	leint16_t qsize;
	leint16_t qflags;
	leint16_t iv;
	leint32_t rsvd12[4];
};
__static_assert(sizeof(struct nvme_cmd_create_cq) == 64);

struct nvme_cmd_create_sq {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t rsvd1[5];
	leint64_t prp1;
	leint64_t rsvd8;
	leint16_t qid;
	leint16_t qsize;
	leint16_t qflags;
	leint16_t cqid;
	leint32_t rsvd12[4];
};
__static_assert(sizeof(struct nvme_cmd_create_sq) == 64);

struct nvme_cmd_delete_q {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t rsvd1[9];
	leint16_t qid;
	leint16_t rsvd10;
	leint32_t rsvd11[5];
};
__static_assert(sizeof(struct nvme_cmd_delete_q) == 64);

struct nvme_cmd_features {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t nsid;
	leint64_t rsvd2[2];
	union nvme_dptr dptr;
	uint8_t   fid;
	uint8_t   sel;
	uint16_t  rsvd42;
	leint32_t cdw11;
	leint32_t cdw12;
	leint32_t cdw13;
	leint32_t cdw14;
	leint32_t cdw15;
};
__static_assert(sizeof(struct nvme_cmd_features) == 64);

struct nvme_cmd_log {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t nsid;
	leint64_t rsvd2[2];
	union nvme_dptr dptr;
	uint8_t   lid;
	uint8_t   lsp;
	leint16_t numdl;
	leint16_t numdu;
	leint16_t lsi;
	leint32_t lpol;
	leint32_t lpou;
	union {
		struct {
			uint8_t rsvd14[3];
			uint8_t csi;
		};

		leint32_t cdw14;
	};
	leint32_t cdw15;
};
__static_assert(sizeof(struct nvme_cmd_log) == 64);

struct nvme_cmd_rw {
	uint8_t   opcode;
	uint8_t   flags;
	uint16_t  cid;
	leint32_t nsid;
	leint32_t cdw2;
	leint32_t cdw3;
	leint64_t mptr;
	union nvme_dptr dptr;
	leint64_t slba;
	leint16_t nlb;
	leint16_t control;
	leint16_t dsm;
	leint32_t reftag;
	leint16_t apptag;
	leint16_t appmask;
};
__static_assert(sizeof(struct nvme_cmd_rw) == 64);

/**
 * union nvme_cmd - Generic NVMe command
 * @opcode: Opcode of the command to be executed
 * @flags: Command flags (i.e. PSDT, FUSE)
 * @cid: Command identifier
 * @nsid: Namespace identifier
 * @cdw2: 2nd command dword
 * @cdw3: 3rd command dword
 * @mptr: Metadata PRP/SGL pointer
 * @dptr: Data PRP/SGL pointer (see &union nvme_dptr)
 * @cdw10: 10th command dword
 * @cdw11: 11th command dword
 * @cdw12: 12th command dword
 * @cdw13: 13th command dword
 * @cdw14: 14th command dword
 * @cdw15: 15th command dword
 */
union nvme_cmd {
	struct {
		uint8_t   opcode;
		uint8_t   flags;
		uint16_t  cid;
		leint32_t nsid;
		leint32_t cdw2;
		leint32_t cdw3;
		leint64_t mptr;
		union nvme_dptr dptr;
		leint32_t cdw10;
		leint32_t cdw11;
		leint32_t cdw12;
		leint32_t cdw13;
		leint32_t cdw14;
		leint32_t cdw15;
	};

	/**
	 * @identify: See &struct nvme_cmd_identify
	 */
	struct nvme_cmd_identify identify;

	/**
	 * @create_cq: See &struct nvme_cmd_create_cq
	 */
	struct nvme_cmd_create_cq create_cq;

	/**
	 * @create_sq: See &struct nvme_cmd_create_sq
	 */
	struct nvme_cmd_create_sq create_sq;

	/**
	 * @delete_q: See &struct nvme_cmd_delete_q
	 */
	struct nvme_cmd_delete_q delete_q;

	/**
	 * @features: See &struct nvme_cmd_features
	 */
	struct nvme_cmd_features features;

	/**
	 * @log: See &struct nvme_cmd_log
	 */
	struct nvme_cmd_log log;

	/**
	 * @rw: See &struct nvme_cmd_rw
	 */
	struct nvme_cmd_rw rw;
};

enum nvme_cmd_flags {
	NVME_CMD_FLAGS_PSDT_PRP			= 0x0,
	NVME_CMD_FLAGS_PSDT_SGL_MPTR_CONTIG	= 0x1,
	NVME_CMD_FLAGS_PSDT_SGL_MPTR_SGL	= 0x2,
};

#define NVME_CMD_FLAGS_PSDT_MASK 0x3
#define NVME_CMD_FLAGS_PSDT_SHIFT 6

struct nvme_cqe {
	union {
		struct {
			leint32_t dw0;
			leint32_t dw1;
		};

		leint64_t qw0;
	};

	leint16_t sqhd;
	leint16_t sqid;
	uint16_t  cid;
	leint16_t sfp; /* status field and phase */
};
__static_assert(sizeof(struct nvme_cqe) == 16);

#define NVME_AEN_TYPE(dw0) ((dw0 >>  0) & 0x7)
#define NVME_AEN_INFO(dw0) ((dw0 >>  8) & 0xff)
#define NVME_AEN_LID(dw0)  ((dw0 >> 16) & 0xff)

typedef void (*cqe_handler)(struct nvme_cqe *cqe);

struct nvme_crc64_pi_tuple {
	beint64_t guard;
	beint16_t apptag;
	uint8_t   sr[6];
};
__static_assert(sizeof(struct nvme_crc64_pi_tuple) == 16);

#endif /* LIBVFN_NVME_TYPES_H */
