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

static int __rq_max_prps;

static void __attribute__((constructor)) init_max_prps(void)
{
	__rq_max_prps = (int)(sysconf(_SC_PAGESIZE) / sizeof(uint64_t) + 1);

	log_debug("max prps is %d\n", __rq_max_prps);
}

static inline int __map_first(leint64_t *prp1, leint64_t *prplist, uint64_t iova, size_t len)
{
	/* number of prps required to map the buffer */
	int prpcount = (int)len >> __VFN_PAGESHIFT;

	*prp1 = cpu_to_le64(iova);

	/*
	 * If prpcount is at least one and the iova is not aligned to the page
	 * size, we need one more prp than what we calculated above.
	 * Additionally, we align the iova down to a page size boundary,
	 * simplifying the following loop.
	 */
	if (prpcount && !ALIGNED(iova, __VFN_PAGESIZE)) {
		iova = ALIGN_DOWN(iova, __VFN_PAGESIZE);
		prpcount++;
	}

	if (prpcount > __rq_max_prps) {
		errno = EINVAL;
		return 0;
	}

	/*
	 * Map the remaining parts of the buffer into prp2/prplist. iova will be
	 * aligned from the above, which simplifies this.
	 */
	for (int i = 1; i < prpcount; i++)
		prplist[i - 1] = cpu_to_le64(iova + (i << __VFN_PAGESHIFT));

	/*
	 * prpcount may be zero if the buffer length was less than the page
	 * size, so clamp it to 1 in that case.
	 */
	return clamp_t(int, prpcount, 1, prpcount);
}

static inline int __map_aligned(leint64_t *prplist, int prpcount, uint64_t iova)
{
	/*
	 * __map_aligned is used exclusively for mapping into the prplist
	 * entries where addresses must be page size aligned.
	 */
	assert(ALIGNED(iova, __VFN_PAGESIZE));

	for (int i = 0; i < prpcount; i++)
		prplist[i] = cpu_to_le64(iova + (i << __VFN_PAGESHIFT));

	return prpcount;
}

int nvme_rq_map_prp(struct nvme_rq *rq, union nvme_cmd *cmd, uint64_t iova, size_t len)
{
	int prpcount;
	leint64_t *prplist = rq->page.vaddr;

	prpcount = __map_first(&cmd->dptr.prp1, prplist, iova, len);
	if (!prpcount) {
		errno = EINVAL;
		return -1;
	}

	if (prpcount == 2)
		cmd->dptr.prp2 = prplist[0];
	else if (prpcount > 2)
		cmd->dptr.prp2 = cpu_to_le64(rq->page.iova);
	else
		cmd->dptr.prp2 = 0x0;

	return 0;
}

int nvme_rq_mapv_prp(struct nvme_rq *rq, union nvme_cmd *cmd, struct iovec *iov, int niov)
{
	int prpcount, _prpcount;
	leint64_t *prplist = rq->page.vaddr;
	uint64_t iova = (uint64_t)iov->iov_base;
	size_t len = iov->iov_len;

	/* map the first segment */
	prpcount = __map_first(&cmd->dptr.prp1, prplist, iova, len);

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
	if (!(prpcount == 1 || niov == 1 || ALIGNED(iova + len, __VFN_PAGESIZE))) {
		log_error("iov[0].iov_base/len invalid\n");

		goto invalid;
	}

	/* map remaining iovec entries; these must be page size aligned */
	for (int i = 1; i < niov; i++) {
		iova = (uint64_t)iov[i].iov_base;
		len = iov[i].iov_len;

		_prpcount = max_t(int, 1, (int)len >> __VFN_PAGESHIFT);

		if (prpcount + _prpcount > __rq_max_prps) {
			log_error("too many prps required\n");

			goto invalid;
		}


		if (!ALIGNED(iova, __VFN_PAGESIZE)) {
			log_error("unaligned iov[%u].iov_base (0x%"PRIx64")\n", i, iova);

			goto invalid;
		}

		/* all entries but the last must have a page size aligned len */
		if (i < niov - 1 && !ALIGNED(len, __VFN_PAGESIZE)) {
			log_error("unaligned iov[%u].len (%zu)\n", i, len);

			goto invalid;
		}

		prpcount += __map_aligned(&prplist[prpcount - 1], _prpcount, iova);
	}

	if (prpcount == 2)
		cmd->dptr.prp2 = prplist[0];
	else if (prpcount > 2)
		cmd->dptr.prp2 = cpu_to_le64(rq->page.iova);
	else
		cmd->dptr.prp2 = 0x0;

	return 0;

invalid:
	errno = EINVAL;
	return -1;
}

int nvme_rq_spin(struct nvme_rq *rq, struct nvme_cqe *cqe_copy)
{
	struct nvme_cq *cq = rq->sq->cq;
	struct nvme_cqe cqe;

	nvme_cq_get_cqes(cq, &cqe, 1);
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
