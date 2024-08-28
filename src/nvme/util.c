// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include <assert.h>
#include <byteswap.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/uio.h>

#include <linux/vfio.h>

#include <vfn/support.h>
#include <vfn/vfio.h>
#include <vfn/trace.h>
#include <vfn/nvme.h>

#include "types.h"

#include "crc64table.h"

uint64_t nvme_crc64(uint64_t crc, const unsigned char *buffer, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		crc = (crc >> 8) ^ crc64_nvme_table[(crc & 0xff) ^ buffer[i]];

	return crc ^ (uint64_t)~0;
}

int nvme_aer(struct nvme_ctrl *ctrl, void *opaque)
{
	struct nvme_rq *rq;
	union nvme_cmd cmd = { .opcode = NVME_ADMIN_ASYNC_EVENT };

	rq = nvme_rq_acquire_atomic(ctrl->adminq.sq);
	if (!rq) {
		errno = EBUSY;
		return -1;
	}

	cmd.cid = rq->cid | NVME_CID_AER;
	rq->opaque = opaque;

	/* rq_exec overwrites the command identifier, so use sq_exec */
	nvme_sq_exec(ctrl->adminq.sq, &cmd);

	return 0;
}

int nvme_sync(struct nvme_ctrl *ctrl, struct nvme_sq *sq, union nvme_cmd *sqe, void *buf,
	      size_t len, struct nvme_cqe *cqe_copy)
{
	struct nvme_cqe cqe;
	struct nvme_rq *rq;
	uint64_t iova;
	bool do_unmap = false;
	int ret = 0;

	if (buf) {
		struct iommu_ctx *ctx = __iommu_ctx(ctrl);

		if (!iommu_translate_vaddr(ctx, buf, &iova)) {
			do_unmap = true;

			if (iommu_map_vaddr(ctx, buf, len, &iova, IOMMU_MAP_EPHEMERAL)) {
				log_debug("failed to map vaddr\n");
				return -1;
			}
		}
	}

	rq = nvme_rq_acquire_atomic(sq);
	if (!rq)
		return -1;

	if (buf) {
		ret = nvme_rq_map_prp(ctrl, rq, sqe, iova, len);
		if (ret) {
			goto release_rq;
		}
	}

	nvme_rq_exec(rq, sqe);

	while (nvme_rq_spin(rq, &cqe) < 0) {
		if (errno == EAGAIN) {
			log_error("SPURIOUS CQE (cq %" PRIu16 " cid %" PRIu16 ")\n",
				  rq->sq->cq->id, cqe.cid);

			continue;
		}

		ret = -1;

		break;
	}

	if (cqe_copy)
		memcpy(cqe_copy, &cqe, 1 << NVME_CQES);

release_rq:
	nvme_rq_release_atomic(rq);

	if (buf && do_unmap)
		log_fatal_if(iommu_unmap_vaddr(__iommu_ctx(ctrl), buf, NULL),
			     "iommu_unmap_vaddr\n");

	return ret;
}

int nvme_admin(struct nvme_ctrl *ctrl, union nvme_cmd *sqe, void *buf, size_t len,
	       struct nvme_cqe *cqe_copy)
{
	return nvme_sync(ctrl, ctrl->adminq.sq, sqe, buf, len, cqe_copy);
}

static int nvme_virt_mgmt(struct nvme_ctrl *ctrl, uint16_t cntlid, enum nvme_virt_mgmt_rt rt,
			  enum nvme_virt_mgmt_act act, uint16_t nr)
{
	union nvme_cmd cmd = {
		.opcode = NVME_ADMIN_VIRT_MGMT,

		.cdw10 = cpu_to_le32(cntlid << 16 | rt << 8 | act),
		.cdw11 = cpu_to_le32(nr),
	};

	return nvme_admin(ctrl, &cmd, NULL, 0, NULL);
}

int nvme_vm_assign_max_flexible(struct nvme_ctrl *ctrl, uint16_t scid)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	union nvme_cmd cmd;
	struct nvme_primary_ctrl_cap *cap;

	__autovar_s(iommu_dmabuf) buffer;

	if (iommu_get_dmabuf(ctx, &buffer, NVME_IDENTIFY_DATA_SIZE, IOMMU_MAP_EPHEMERAL))
		return -1;

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = NVME_ADMIN_IDENTIFY,
		.cns = NVME_IDENTIFY_CNS_PRIMARY_CTRL_CAP,
	};

	if (nvme_admin(ctrl, &cmd, buffer.vaddr, buffer.len, NULL))
		return -1;

	cap = buffer.vaddr;

	if (nvme_virt_mgmt(ctrl, scid, NVME_VIRT_MGMT_RESOURCE_TYPE_VQ,
			   NVME_VIRT_MGMT_ACTION_SECONDARY_ASSIGN_FLEXIBLE,
			   le16_to_cpu(cap->vqfrsm)))
		return -1;

	if (nvme_virt_mgmt(ctrl, scid, NVME_VIRT_MGMT_RESOURCE_TYPE_VI,
			   NVME_VIRT_MGMT_ACTION_SECONDARY_ASSIGN_FLEXIBLE,
			   le16_to_cpu(cap->vifrsm)))
		return -1;

	return 0;
}

int nvme_get_vf_cntlid(struct nvme_ctrl *ctrl, int vfnum, uint16_t *cntlid)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	union nvme_cmd cmd;
	struct nvme_secondary_ctrl_list *list;

	__autovar_s(iommu_dmabuf) buffer;

	if (iommu_get_dmabuf(ctx, &buffer, NVME_IDENTIFY_DATA_SIZE, IOMMU_MAP_EPHEMERAL))
		return -1;

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = NVME_ADMIN_IDENTIFY,
		.cns = NVME_IDENTIFY_CNS_SECONDARY_CTRL_LIST,
	};

	if (nvme_admin(ctrl, &cmd, buffer.vaddr, buffer.len, NULL))
		return -1;

	list = buffer.vaddr;

	for (uint8_t i = 0; i < list->num; i++) {
		struct nvme_secondary_ctrl *sctrl = &list->sc_entry[i];

		if (le16_to_cpu(sctrl->vfn) == vfnum) {
			*cntlid = le16_to_cpu(sctrl->scid);

			return 0;
		}
	}

	errno = ENOENT;

	return -1;
}

int nvme_vm_set_online(struct nvme_ctrl *ctrl, uint16_t scid)
{
	return nvme_virt_mgmt(ctrl, scid, 0, NVME_VIRT_MGMT_ACTION_SECONDARY_ONLINE, 0);
}

int nvme_vm_set_offline(struct nvme_ctrl *ctrl, uint16_t scid)
{
	return nvme_virt_mgmt(ctrl, scid, 0, NVME_VIRT_MGMT_ACTION_SECONDARY_OFFLINE, 0);
}
