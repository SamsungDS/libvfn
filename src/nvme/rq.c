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

#include <linux/vfio.h>

#include <vfn/support/align.h>
#include <vfn/support/atomic.h>
#include <vfn/support/barrier.h>
#include <vfn/support/compiler.h>
#include <vfn/support/endian.h>
#include <vfn/support/log.h>
#include <vfn/support/mmio.h>
#include <vfn/support/mem.h>
#include <vfn/trace.h>
#include <vfn/vfio.h>
#include <vfn/nvme/types.h>
#include <vfn/nvme/queue.h>
#include <vfn/nvme/ctrl.h>
#include <vfn/nvme/rq.h>
#include <vfn/nvme/util.h>

#include "ccan/minmax/minmax.h"

void nvme_rq_map_prp(struct nvme_rq *rq, union nvme_cmd *cmd, uint64_t iova, size_t len)
{
	unsigned int prpcount = len >> __VFN_PAGESHIFT;
	leint64_t *prplist = rq->page.vaddr;

	cmd->dptr.prp1 = cpu_to_le64(iova);

	if (!ALIGNED(iova, __VFN_PAGESIZE)) {
		iova = ALIGN_DOWN(iova, __VFN_PAGESIZE);
		prpcount++;
	}

	for (unsigned int i = 1; i < prpcount; i++)
		prplist[i - 1] = cpu_to_le64(iova + (i << __VFN_PAGESHIFT));

	if (prpcount == 2)
		cmd->dptr.prp2 = prplist[0];
	else if (prpcount > 2)
		cmd->dptr.prp2 = cpu_to_le64(rq->page.iova);
	else
		cmd->dptr.prp2 = 0x0;
}

int nvme_rq_poll(struct nvme_rq *rq, void *cqe_copy)
{
	struct nvme_cq *cq = rq->sq->cq;
	struct nvme_cqe *cqe;

	cqe = nvme_cq_poll(cq);

	if (cqe_copy)
		memcpy(cqe_copy, cqe, 1 << NVME_CQES);

	if (cqe->cid != rq->cid) {
		errno = EAGAIN;
		return -1;
	}

	if (!nvme_cqe_ok(cqe)) {
		if (__logv(LOG_DEBUG)) {
			uint16_t status = le16_to_cpu(cqe->sfp) >> 1;

			__debug("cqe status 0x%" PRIx16 "\n", status & 0x7ff);
		}

		return nvme_set_errno_from_cqe(cqe);
	}

	return 0;
}
