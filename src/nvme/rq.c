// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
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

#include <sys/mman.h>

#include <support/log.h>
#include <support/mem.h>

#include "vfn/nvme.h"

void nvme_rq_map_prp(struct nvme_rq *rq, union nvme_cmd *cmd, uint64_t iova, size_t len)
{
	leint64_t *prplist = rq->page.vaddr;
	unsigned int i;

	assert(ALIGNED(iova, PAGESIZE));

	cmd->dptr.prp1 = cpu_to_le64(iova);

	for (i = 1; i < (len >> PAGESHIFT); i++)
		prplist[i-1] = cpu_to_le64(iova + (i << PAGESHIFT));

	if (i == 2)
		cmd->dptr.prp2 = prplist[0];
	else if (i > 2)
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

			__debug("cqe status 0x%"PRIx16"\n", status & 0x7ff);
		}

		return nvme_set_errno_from_cqe(cqe);
	}

	return 0;
}
