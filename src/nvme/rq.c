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

#include "iommu/context.h"
#include "types.h"

int nvme_rq_map_prp(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd,
		    uint64_t iova, size_t len)
{
	return nvme_map_prp(ctrl, rq->page.vaddr, cmd, iova, len);
}

int nvme_rq_mapv_prp(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd,
		     struct iovec *iov, int niov)
{
	return nvme_mapv_prp(ctrl, rq->page.vaddr, cmd, iov, niov);
}

int nvme_rq_mapv_sgl(struct nvme_ctrl *ctrl, struct nvme_rq *rq, union nvme_cmd *cmd,
		     struct iovec *iov, int niov)
{
	return nvme_mapv_sgl(ctrl, rq->page.vaddr, cmd, iov, niov);
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
