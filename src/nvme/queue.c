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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>

#include <linux/vfio.h>

#include <vfn/support.h>
#include <vfn/trace.h>
#include <vfn/nvme.h>

#include "ccan/time/time.h"

void nvme_cq_get_cqes(struct nvme_cq *cq, struct nvme_cqe *cqes, int n)
{
	struct nvme_cqe *cqe;

	do {
		cqe = nvme_cq_get_cqe(cq);
		if (!cqe)
			continue;

		n--;

		if (cqes)
			memcpy(cqes++, cqe, sizeof(*cqe));
	} while (n > 0);
}

int nvme_cq_wait_cqes(struct nvme_cq *cq, struct nvme_cqe *cqes, int n, struct timespec *ts)
{
	struct nvme_cqe *cqe;
	struct timerel rel;
	uint64_t timeout;
	int m = n;

	if (!ts) {
		nvme_cq_get_cqes(cq, cqes, n);

		return n;
	}

	rel.ts = *ts;

	timeout = get_ticks() + time_to_usec(rel) * (__vfn_ticks_freq / 1000000ULL);

	do {
		cqe = nvme_cq_get_cqe(cq);
		if (!cqe)
			continue;

		m--;

		if (cqes)
			memcpy(cqes++, cqe, sizeof(*cqe));
	} while (m > 0 && get_ticks() < timeout);

	if (m > 0)
		errno = ETIMEDOUT;

	return n - m;
}
