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

#define NVME_FIELD_GET(value, name) \
	(((value) >> NVME_##name##_SHIFT) & NVME_##name##_MASK)

#define NVME_FIELD_SET(value, name) \
	(((value) & NVME_##name##_MASK) << NVME_##name##_SHIFT)

enum nvme_constants {
	NVME_IDENTIFY_DATA_SIZE		= 4096,
};

enum nvme_reg {
	NVME_REG_CAP			= 0x0000,
	NVME_REG_CC			= 0x0014,
	NVME_REG_CSTS			= 0x001c,
	NVME_REG_AQA			= 0x0024,
	NVME_REG_ASQ			= 0x0028,
	NVME_REG_ACQ			= 0x0030,
	NVME_REG_CMBLOC			= 0x0038,
	NVME_REG_CMBSZ			= 0x003c,
	NVME_REG_CMBMSC			= 0x0050,
};

enum nvme_cap {
	NVME_CAP_MQES_SHIFT		= 0,
	NVME_CAP_MQES_MASK		= 0xffff,
	NVME_CAP_TO_SHIFT		= 24,
	NVME_CAP_TO_MASK		= 0xff,
	NVME_CAP_DSTRD_SHIFT		= 32,
	NVME_CAP_DSTRD_MASK		= 0xf,
	NVME_CAP_CSS_SHIFT		= 37,
	NVME_CAP_CSS_MASK		= 0xff,
	NVME_CAP_MPSMIN_SHIFT		= 48,
	NVME_CAP_MPSMIN_MASK		= 0xf,
	NVME_CAP_MPSMAX_SHIFT		= 52,
	NVME_CAP_MPSMAX_MASK		= 0xf,
	NVME_CAP_CMBS_SHIFT		= 57,
	NVME_CAP_CMBS_MASK		= 0x1,

	NVME_CAP_CSS_CSI		= 1 << 6,
	NVME_CAP_CSS_ADMIN		= 1 << 7,
};

enum nvme_cc {
	NVME_CC_EN_SHIFT		= 0,
	NVME_CC_EN_MASK			= 0x1,
	NVME_CC_CSS_SHIFT		= 4,
	NVME_CC_CSS_MASK		= 0x7,
	NVME_CC_MPS_SHIFT		= 7,
	NVME_CC_MPS_MASK		= 0xf,
	NVME_CC_AMS_SHIFT		= 11,
	NVME_CC_AMS_MASK		= 0x7,
	NVME_CC_SHN_SHIFT		= 14,
	NVME_CC_SHN_MASK		= 0x3,
	NVME_CC_IOSQES_SHIFT		= 16,
	NVME_CC_IOSQES_MASK		= 0xf,
	NVME_CC_IOCQES_SHIFT		= 20,
	NVME_CC_IOCQES_MASK		= 0xf,

	NVME_CC_SHN_NONE		= 0,
	NVME_CC_AMS_RR			= 0,
	NVME_CC_CSS_CSI			= 6,
	NVME_CC_CSS_ADMIN		= 7,
	NVME_CC_CSS_NVM			= 0,
};

enum nvme_csts {
	NVME_CSTS_RDY_SHIFT		= 0,
	NVME_CSTS_RDY_MASK		= 0x1,
};

enum nvme_cmbloc {
	NVME_CMBLOC_BIR_SHIFT		= 0,
	NVME_CMBLOC_BIR_MASK		= 0x7,
	NVME_CMBLOC_OFST_SHIFT		= 12,
	NVME_CMBLOC_OFST_MASK		= 0xfffff,
};

enum nvme_cmbsz {
	NVME_CMBSZ_SZU_SHIFT	= 8,
	NVME_CMBSZ_SZU_MASK	= 0xf,
	NVME_CMBSZ_SZ_SHIFT	= 12,
	NVME_CMBSZ_SZ_MASK	= 0xfffff,
	NVME_CMBSZ_SZU_4K	= 0,
	NVME_CMBSZ_SZU_64K	= 1,
	NVME_CMBSZ_SZU_1M	= 2,
	NVME_CMBSZ_SZU_16M	= 3,
	NVME_CMBSZ_SZU_256M	= 4,
	NVME_CMBSZ_SZU_4G	= 5,
	NVME_CMBSZ_SZU_64G	= 6,
};

static inline __u64 nvme_cmb_size(__u32 cmbsz)
{
	return ((__u64)NVME_FIELD_GET(cmbsz, CMBSZ_SZ)) *
		(1ULL << (12 + 4 * NVME_FIELD_GET(cmbsz, CMBSZ_SZU)));
}

enum nvme_cmbmsc {
	NVME_CMBMSC_CRE_SHIFT		= 0,
	NVME_CMBMSC_CRE_MASK		= 0x1,
	NVME_CMBMSC_CMSE_SHIFT		= 1,
	NVME_CMBMSC_CMSE_MASK		= 0x1,
	NVME_CMBMSC_CBA_SHIFT		= 12,
};
static const __u64 NVME_CMBMSC_CBA_MASK = 0xfffffffffffffull;

enum nvme_feat {
	NVME_FEAT_NRQS_NSQR_SHIFT	= 0,
	NVME_FEAT_NRQS_NSQR_MASK	= 0xffff,
	NVME_FEAT_NRQS_NCQR_SHIFT	= 16,
	NVME_FEAT_NRQS_NCQR_MASK	= 0xffff,
};

enum nvme_fid {
	NVME_FEAT_FID_NUM_QUEUES	= 0x07,
};

enum nvme_admin_opcode {
	NVME_ADMIN_DELETE_SQ            = 0x00,
	NVME_ADMIN_CREATE_SQ            = 0x01,
	NVME_ADMIN_DELETE_CQ		= 0x04,
	NVME_ADMIN_CREATE_CQ            = 0x05,
	NVME_ADMIN_IDENTIFY		= 0x06,
	NVME_ADMIN_SET_FEATURES         = 0x09,
	NVME_ADMIN_ASYNC_EVENT          = 0x0c,
	NVME_ADMIN_DBCONFIG		= 0x7c,
};

enum nvme_identify_cns {
	NVME_IDENTIFY_CNS_CTRL		= 0x01,
};

enum nvme_identify_ctrl_offset {
	NVME_IDENTIFY_CTRL_OACS		= 256,
	NVME_IDENTIFY_CTRL_SGLS		= 536,
};

enum nvme_identify_ctrl_oacs {
	NVME_IDENTIFY_CTRL_OACS_DBCONFIG = 1 << 8,
};

enum nvme_identify_ctrl_sgls {
	NVME_IDENTIFY_CTRL_SGLS_ALIGNMENT_SHIFT	= 0,
	NVME_IDENTIFY_CTRL_SGLS_ALIGNMENT_MASK	= 0x3,

	NVME_IDENTIFY_CTRL_SGLS_ALIGNMENT_NONE	= 0x1,
	NVME_IDENTIFY_CTRL_SGLS_ALIGNMENT_DWORD	= 0x2,
};
