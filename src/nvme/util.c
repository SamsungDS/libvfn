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

#include "ccan/minmax/minmax.h"
#include "types.h"

#include "crc64table.h"

uint64_t nvme_crc64(uint64_t crc, const unsigned char *buffer, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		crc = (crc >> 8) ^ crc64_nvme_table[(crc & 0xff) ^ buffer[i]];

	return crc ^ (uint64_t)~0;
}

int nvme_set_errno_from_cqe(struct nvme_cqe *cqe)
{
	errno = le16_to_cpu(cqe->sfp) >> 1 ? EIO : 0;

	return errno ? -1 : 0;
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
	iova_t iova;
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

/*
 * PRP list chaining cursor.
 *
 * A PRP list page holds at most @max_prps entries. When a buffer requires more
 * entries than fit in a single page, the list is chained: the last slot of a
 * non-final page holds the iova of the next page. The cursor tracks the page
 * (@page) and the slot within it (@slot) so that callers building a list across
 * multiple segments (see __map_prp_first / __map_prp_append) share a single
 * position.
 *
 * @prplists is a contiguous array of @nprplists pages, each one MPS page in
 * size, that the caller has allocated and mapped. Because the array is
 * contiguous in both virtual and iova space, page @p sits at
 * @prplists + @p * @max_prps and its iova is @prplist_iova + @p * pagesize.
 */
struct __prp_cursor {
	leint64_t *prplists;
	uint64_t prplist_iova;
	int nprplists;
	int page;
	int slot;
};

static inline void __prp_cursor_init(struct __prp_cursor *c, leint64_t *prplists,
				     uint64_t prplist_iova, int nprplists)
{
	c->prplists = prplists;
	c->prplist_iova = prplist_iova;
	c->nprplists = nprplists;
	c->page = 0;
	c->slot = 0;
}

/*
 * Number of data PRP entries that may be stored across @nprplists pages: the
 * final page contributes @max_prps entries, every other page contributes
 * @max_prps - 1 (its last slot is consumed by the chain link).
 */
static inline int __prplist_capacity(int nprplists, int max_prps)
{
	return (nprplists - 1) * (max_prps - 1) + max_prps;
}

/*
 * Append one data PRP entry to the chain.
 *
 * A page holds @max_prps entries; the last one (index @max_prps - 1) of a
 * non-final page is repurposed as the chain link to the next page. To avoid
 * reserving that slot prematurely (which would needlessly chain a list that
 * ends exactly at a page boundary), chaining is deferred until the slot is
 * actually required: when the cursor sits at the last slot of a non-final page
 * and a further entry is appended, the link is written into that last slot and
 * the entry is placed at the start of the next page instead.
 *
 * Return: ``0`` on success, ``-1`` if @prplists has been exhausted.
 */
static inline int __prp_cursor_put(struct __prp_cursor *c, uint64_t iova, int max_prps,
				   int pageshift)
{
	leint64_t *page;

	/*
	 * The cursor is parked at the last slot of a non-final page. Writing the
	 * incoming entry there would leave no room for a chain link if yet
	 * another entry follows, so consume this slot as the link to the next
	 * page and continue there. If no further entry ever follows, the slot
	 * stays unused (harmless) and no chain is created.
	 */
	if (c->slot == max_prps - 1 && c->page + 1 < c->nprplists) {
		page = c->prplists + (c->page << (pageshift - 3));
		page[max_prps - 1] =
			cpu_to_le64(c->prplist_iova + ((uint64_t)(c->page + 1) << pageshift));

		c->page++;
		c->slot = 0;
	}

	if (c->page >= c->nprplists || c->slot >= max_prps) {
		errno = EINVAL;
		return -1;
	}

	page = c->prplists + (c->page << (pageshift - 3));
	page[c->slot++] = cpu_to_le64(iova);

	return 0;
}

static inline int __map_prp_first(leint64_t *prp1, struct __prp_cursor *c, iova_t iova,
				  size_t len, int pageshift)
{
	size_t pagesize = 1 << pageshift;
	int max_prps = 1 << (pageshift - 3);

	/* number of prps required to map the buffer */
	int prpcount = 1;

	*prp1 = cpu_to_le64(iova);

	/* account for what is covered with the first prp */
	len -= min_t(size_t, len, pagesize - (iova & (pagesize - 1)));

	/* any residual just adds more prps */
	if (len)
		prpcount += (int)ALIGN_UP(len, pagesize) >> pageshift;

	/* prp1 is stored in the command, so only prpcount - 1 go into the list */
	if (prpcount - 1 > __prplist_capacity(c->nprplists, max_prps)) {
		errno = EINVAL;
		return -1;
	}

	if (prpcount > 1 && !ALIGNED(iova, pagesize))
		/* align down to simplify loop below */
		iova = ALIGN_DOWN(iova, pagesize);

	/*
	 * Map the remaining parts of the buffer into prp2/prplist. iova will be
	 * aligned from the above, which simplifies this.
	 */
	for (int i = 1; i < prpcount; i++) {
		if (__prp_cursor_put(c, iova + ((uint64_t)i << pageshift), max_prps, pageshift))
			return -1;
	}

	return prpcount;
}

static inline int __map_prp_append(struct __prp_cursor *c, iova_t iova, size_t len, int max_prps,
				   int pageshift)
{
	int prpcount = max_t(int, 1, (int)len >> pageshift);
	size_t pagesize = 1 << pageshift;

	if (!ALIGNED(iova, pagesize)) {
		log_error("unaligned iova 0x%" PRIx64 "\n", iova);

		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < prpcount; i++) {
		if (__prp_cursor_put(c, iova + ((uint64_t)i << pageshift), max_prps, pageshift))
			return -1;
	}

	return prpcount;
}

static inline void __set_prp2(leint64_t *prp2, leint64_t prplist_iova, leint64_t *prplist,
			      int prpcount)
{
	if (prpcount == 2)
		*prp2 = prplist[0];
	else if (prpcount > 2)
		*prp2 = prplist_iova;
	else
		*prp2 = 0x0;
}

int nvme_map_prp(struct nvme_ctrl *ctrl, leint64_t *prplists, int nprplists,
		 union nvme_cmd *cmd, iova_t iova, size_t len)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);
	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	struct __prp_cursor c;
	iova_t prplist_iova;
	int prpcount;

	if (nprplists < 1) {
		errno = EINVAL;
		return -1;
	}

	if (!iommu_translate_vaddr(ctx, prplists, &prplist_iova)) {
		errno = EFAULT;
		return -1;
	}

	__prp_cursor_init(&c, prplists, prplist_iova, nprplists);

	prpcount = __map_prp_first(&cmd->dptr.prp1, &c, iova, len, pageshift);
	if (prpcount < 0) {
		errno = EINVAL;
		return -1;
	}

	__set_prp2(&cmd->dptr.prp2, cpu_to_le64(prplist_iova), prplists, prpcount);

	return 0;
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

	__autovar_s(iommu_dmabuf) buffer = {};

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

int nvme_mapv_prp(struct nvme_ctrl *ctrl, leint64_t *prplists, int nprplists,
		  union nvme_cmd *cmd, struct iovec *iov, int niov)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	size_t len = iov->iov_len;
	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	size_t pagesize = 1 << pageshift;
	int max_prps = 1 << (pageshift - 3);
	int ret, prpcount;
	iova_t iova, prplist_iova;
	struct __prp_cursor c;

	if (nprplists < 1) {
		errno = EINVAL;
		return -1;
	}

	if (!iommu_translate_vaddr(ctx, iov->iov_base, &iova)) {
		errno = EFAULT;
		return -1;
	}

	if (!iommu_translate_vaddr(ctx, prplists, &prplist_iova)) {
		errno = EFAULT;
		return -1;
	}

	__prp_cursor_init(&c, prplists, prplist_iova, nprplists);

	/* map the first segment */
	prpcount = __map_prp_first(&cmd->dptr.prp1, &c, iova, len, pageshift);
	if (prpcount < 0)
		goto invalid;

	/*
	 * At this point, one of three conditions must hold:
	 *
	 *   a) a single prp entry was set up by __map_first, or
	 *   b) the iovec only has a single entry, or
	 *   c) the first buffer ends on a page size boundary
	 *
	 * If none holds, the buffer(s) within the iovec cannot be mapped given
	 * the PRP alignment requirements.
	 */
	if (!(prpcount == 1 || niov == 1 || ALIGNED(iova + len, pagesize))) {
		log_error("iov[0].iov_base/len invalid\n");

		goto invalid;
	}

	/* map remaining iovec entries; these must be page size aligned */
	for (int i = 1; i < niov; i++) {
		if (!iommu_translate_vaddr(ctx, iov[i].iov_base, &iova)) {
			errno = EFAULT;
			return -1;
		}

		len = iov[i].iov_len;

		/* all entries but the last must have a page size aligned len */
		if (i < niov - 1 && !ALIGNED(len, pagesize)) {
			log_error("unaligned iov[%u].len (%zu)\n", i, len);

			goto invalid;
		}

		ret = __map_prp_append(&c, iova, len, max_prps, pageshift);
		if (ret < 0)
			goto invalid;

		prpcount += ret;
	}

	__set_prp2(&cmd->dptr.prp2, cpu_to_le64(prplist_iova), prplists, prpcount);

	return 0;

invalid:
	errno = EINVAL;
	return -1;
}

int nvme_mapv_iova_prp(struct nvme_ctrl *ctrl, leint64_t *prplists, int nprplists,
		       union nvme_cmd *cmd, struct iova_vec *iov, int niov)
{
	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	size_t pagesize = 1 << pageshift;
	int max_prps = 1 << (pageshift - 3);
	iova_t prplist_iova;
	struct __prp_cursor c;
	int ret, prpcount;

	if (nprplists < 1) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * @prplists is described by a physical (iova) address in the command,
	 * but the caller still provides a kernel-accessible mapping so that the
	 * list entries can be filled in; derive its iova from the vaddr.
	 */
	if (!iommu_translate_vaddr(__iommu_ctx(ctrl), prplists, &prplist_iova)) {
		errno = EFAULT;
		return -1;
	}

	__prp_cursor_init(&c, prplists, prplist_iova, nprplists);

	/* map the first segment */
	prpcount = __map_prp_first(&cmd->dptr.prp1, &c, iov[0].iova, iov[0].len, pageshift);
	if (prpcount < 0)
		goto invalid;

	/*
	 * At this point, one of three conditions must hold:
	 *
	 *   a) a single prp entry was set up by __map_first, or
	 *   b) the iovec only has a single entry, or
	 *   c) the first buffer ends on a page size boundary
	 *
	 * If none holds, the buffer(s) within the iovec cannot be mapped given
	 * the PRP alignment requirements.
	 */
	if (!(prpcount == 1 || niov == 1 || ALIGNED(iov[0].iova + iov[0].len, pagesize))) {
		log_error("iov[0].iova/len invalid\n");

		goto invalid;
	}

	/* map remaining iovec entries; these must be page size aligned */
	for (int i = 1; i < niov; i++) {
		/* all entries but the last must have a page size aligned len */
		if (i < niov - 1 && !ALIGNED(iov[i].len, pagesize)) {
			log_error("unaligned iov[%u].len (%zu)\n", i, iov[i].len);

			goto invalid;
		}

		ret = __map_prp_append(&c, iov[i].iova, iov[i].len, max_prps, pageshift);
		if (ret < 0)
			goto invalid;

		prpcount += ret;
	}

	__set_prp2(&cmd->dptr.prp2, cpu_to_le64(prplist_iova), prplists, prpcount);

	return 0;

invalid:
	errno = EINVAL;
	return -1;
}

static inline void __sgl_data(struct nvme_sgld *sgld, iova_t iova, size_t len)
{
	sgld->addr = cpu_to_le64(iova);
	sgld->len = cpu_to_le32((uint32_t)len);

	sgld->type = NVME_SGLD_TYPE_DATA_BLOCK << 4;
}

static inline void __sgl_segment(struct nvme_sgld *sgld, iova_t iova, int n)
{
	sgld->addr = cpu_to_le64(iova);
	sgld->len = cpu_to_le32(n << 4);

	sgld->type = NVME_SGLD_TYPE_LAST_SEGMENT << 4;
}

int nvme_mapv_sgl(struct nvme_ctrl *ctrl, struct nvme_sgld *seg, iova_t seg_iova,
		  union nvme_cmd *cmd, struct iovec *iov, int niov)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	int max_sglds = 1 << (pageshift - 4);
	int dword_align = ctrl->flags & NVME_CTRL_F_SGLS_DWORD_ALIGNMENT;

	iova_t iova;

	if (niov == 1) {
		if (!iommu_translate_vaddr(ctx, iov->iov_base, &iova)) {
			errno = EFAULT;
			return -1;
		}

		__sgl_data(&cmd->dptr.sgl, iova, iov->iov_len);

		return 0;
	}

	if (niov > max_sglds) {
		errno = EINVAL;
		return -1;
	}

	__sgl_segment(&cmd->dptr.sgl, seg_iova, niov);

	for (int i = 0; i < niov; i++) {
		if (!iommu_translate_vaddr(ctx, iov[i].iov_base, &iova)) {
			errno = EFAULT;
			return -1;
		}

		if (dword_align && (iova & 0x3)) {
			errno = EINVAL;
			return -1;
		}

		__sgl_data(&seg[i], iova, iov[i].iov_len);
	}

	cmd->flags |= NVME_FIELD_SET(NVME_CMD_FLAGS_PSDT_SGL_MPTR_CONTIG, CMD_FLAGS_PSDT);

	return 0;
}

int nvme_mapv_iova_sgl(struct nvme_ctrl *ctrl, struct nvme_sgld *seg, iova_t seg_iova,
		  union nvme_cmd *cmd, struct iova_vec *iov, int niov)
{
	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	int max_sglds = 1 << (pageshift - 4);
	int dword_align = ctrl->flags & NVME_CTRL_F_SGLS_DWORD_ALIGNMENT;

	if (niov == 1) {
		__sgl_data(&cmd->dptr.sgl, iov->iova, iov->len);
		return 0;
	}

	if (niov > max_sglds) {
		errno = EINVAL;
		return -1;
	}

	__sgl_segment(&cmd->dptr.sgl, seg_iova, niov);

	for (int i = 0; i < niov; i++) {
		if (dword_align && (iov[i].iova & 0x3)) {
			errno = EINVAL;
			return -1;
		}

		__sgl_data(&seg[i], iov[i].iova, iov[i].len);
	}

	cmd->flags |= NVME_FIELD_SET(NVME_CMD_FLAGS_PSDT_SGL_MPTR_CONTIG, CMD_FLAGS_PSDT);

	return 0;
}

int nvme_get_vf_cntlid(struct nvme_ctrl *ctrl, int vfnum, uint16_t *cntlid)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	union nvme_cmd cmd;
	struct nvme_secondary_ctrl_list *list;

	__autovar_s(iommu_dmabuf) buffer = {};

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
