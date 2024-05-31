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

#define log_fmt(fmt) "nvme/rq: " fmt

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
#include <vfn/trace.h>
#include <vfn/vfio.h>
#include <vfn/nvme.h>

#include "ccan/minmax/minmax.h"

#include "iommu/context.h"
#include "types.h"

static inline int __map_prp_first(leint64_t *prp1, leint64_t *prplist, uint64_t iova, size_t len,
			      int pageshift)
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

	if (prpcount > 1 && !ALIGNED(iova, pagesize))
		/* align down to simplify loop below */
		iova = ALIGN_DOWN(iova, pagesize);

	if (prpcount > max_prps) {
		errno = EINVAL;
		return 0;
	}

	/*
	 * Map the remaining parts of the buffer into prp2/prplist. iova will be
	 * aligned from the above, which simplifies this.
	 */
	for (int i = 1; i < prpcount; i++)
		prplist[i - 1] = cpu_to_le64(iova + ((uint64_t)i << pageshift));

	/*
	 * prpcount may be zero if the buffer length was less than the page
	 * size, so clamp it to 1 in that case.
	 */
	return clamp_t(int, prpcount, 1, prpcount);
}

static inline int __map_prp_append(leint64_t *prplist, uint64_t iova, size_t len, int max_prps,
				   int pageshift)
{
	int prpcount = max_t(int, 1, (int)len >> pageshift);
	size_t pagesize = 1 << pageshift;

	if (prpcount > max_prps) {
		log_error("too many prps required\n");

		errno = EINVAL;
		return -1;
	}

	if (!ALIGNED(iova, pagesize)) {
		log_error("unaligned iova 0x%" PRIx64 "\n", iova);

		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < prpcount; i++)
		prplist[i] = cpu_to_le64(iova + ((uint64_t)i << pageshift));

	return prpcount;
}

static inline void __set_prp2(leint64_t *prp2, leint64_t prplist, leint64_t prplist0, int prpcount)
{
	if (prpcount == 2)
		*prp2 = prplist0;
	else if (prpcount > 2)
		*prp2 = prplist;
	else
		*prp2 = 0x0;
}

int nvme_rq_map_prp(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd, uint64_t iova,
		    size_t len)
{
	int prpcount;
	leint64_t *prplist = rq->page.vaddr;
	int pageshift = __mps_to_pageshift(ctrl->config.mps);

	prpcount = __map_prp_first(&cmd->dptr.prp1, prplist, iova, len, pageshift);
	if (!prpcount) {
		errno = EINVAL;
		return -1;
	}

	__set_prp2(&cmd->dptr.prp2, cpu_to_le64(rq->page.iova), prplist[0], prpcount);

	return 0;
}

int nvme_rq_mapv_prp(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd,
		     struct iovec *iov, int niov)
{
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	leint64_t *prplist = rq->page.vaddr;
	size_t len = iov->iov_len;
	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	size_t pagesize = 1 << pageshift;
	int max_prps = 1 << (pageshift - 3);
	int prpcount;
	uint64_t iova;

	if (!iommu_translate_vaddr(ctx, iov->iov_base, &iova)) {
		errno = EFAULT;
		return -1;
	}

	/* map the first segment */
	prpcount = __map_prp_first(&cmd->dptr.prp1, prplist, iova, len, pageshift);

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

		prpcount += __map_prp_append(&prplist[prpcount - 1], iova, len, max_prps - prpcount,
					     pageshift);
	}

	__set_prp2(&cmd->dptr.prp2, cpu_to_le64(rq->page.iova), prplist[0], prpcount);

invalid:
	errno = EINVAL;
	return -1;
}

static inline void __sgl_data(struct nvme_sgld *sgld, uint64_t iova, size_t len)
{
	sgld->addr = cpu_to_le64(iova);
	sgld->len = cpu_to_le32((uint32_t)len);

	sgld->type = NVME_SGLD_TYPE_DATA_BLOCK << 4;
}

static inline void __sgl_segment(struct nvme_sgld *sgld, uint64_t iova, int n)
{
	sgld->addr = cpu_to_le64(iova);
	sgld->len = cpu_to_le32(n << 4);

	sgld->type = NVME_SGLD_TYPE_LAST_SEGMENT << 4;
}

int nvme_rq_mapv_sgl(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd,
		     struct iovec *iov, int niov)
{
	struct nvme_sgld *seg = rq->page.vaddr;
	struct iommu_ctx *ctx = __iommu_ctx(ctrl);

	int pageshift = __mps_to_pageshift(ctrl->config.mps);
	int max_sglds = 1 << (pageshift - 4);
	int dword_align = ctrl->flags & NVME_CTRL_F_SGLS_DWORD_ALIGNMENT;

	uint64_t iova;

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

	__sgl_segment(&cmd->dptr.sgl, rq->page.iova, niov);

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

int nvme_rq_mapv(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd,
		 struct iovec *iov, int niov)
{
	struct nvme_sq *sq = rq->sq;

	if ((ctrl->flags & NVME_CTRL_F_SGLS_SUPPORTED) == 0 || sq->id == 0)
		return nvme_rq_mapv_prp(ctrl, rq, cmd, iov, niov);

	return nvme_rq_mapv_sgl(ctrl, rq, cmd, iov, niov);
}

int nvme_rq_wait(struct nvme_rq *rq, struct nvme_cqe *cqe_copy, struct timespec *ts)
{
	struct nvme_cq *cq = rq->sq->cq;
	struct nvme_cqe cqe;

	if (nvme_cq_wait_cqes(cq, &cqe, 1, ts) != 1)
		return -1;

	nvme_cq_update_head(cq);

	if (cqe_copy)
		memcpy(cqe_copy, &cqe, sizeof(*cqe_copy));

	if (cqe.cid != rq->cid) {
		errno = EAGAIN;
		return -1;
	}

	if (!nvme_cqe_ok(&cqe)) {
		if (logv(LOG_DEBUG)) {
			uint16_t status = le16_to_cpu(cqe.sfp) >> 1;

			log_debug("cqe status 0x%" PRIx16 "\n", status & 0x7ff);
		}

		return nvme_set_errno_from_cqe(&cqe);
	}

	return 0;
}

int nvme_rq_spin(struct nvme_rq *rq, struct nvme_cqe *cqe_copy)
{
	return nvme_rq_wait(rq, cqe_copy, NULL);
}
