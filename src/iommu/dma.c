// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2023 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#define log_fmt(fmt) "iommu/dma: " fmt

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "ccan/list/list.h"

#include "vfn/iommu.h"
#include "vfn/support.h"

#include "util/iova_map.h"
#include "context.h"

int iommu_map_vaddr(struct iommu_ctx *ctx, void *vaddr, size_t len, uint64_t *iova,
		    unsigned long flags)
{
	uint64_t _iova;

	if (iova_map_translate(&ctx->map, vaddr, &_iova))
		goto out;

	if (flags & IOMMU_MAP_FIXED_IOVA) {
		_iova = *iova;
	} else if (ctx->flags & IOMMU_F_REQUIRE_IOVA && iova_map_reserve(&ctx->map, len, &_iova)) {
		log_debug("failed to allocate iova\n");
		return -1;
	}

	if (ctx->ops.dma_map(ctx, vaddr, len, &_iova, flags)) {
		log_debug("failed to map dma\n");
		return -1;
	}

	if (iova_map_add(&ctx->map, vaddr, len, _iova)) {
		log_debug("failed to add mapping\n");
		return -1;
	}

out:
	if (iova)
		*iova = _iova;

	return 0;
}

int iommu_unmap_vaddr(struct iommu_ctx *ctx, void *vaddr, size_t *len)
{
	struct iova_mapping *m;

	m = iova_map_find(&ctx->map, vaddr);
	if (!m) {
		errno = ENOENT;
		return -1;
	}

	if (len)
		*len = m->len;

	if (ctx->ops.dma_unmap(ctx, m->iova, m->len)) {
		log_debug("failed to unmap dma\n");
		return -1;
	}

	iova_map_remove(&ctx->map, m->vaddr);

	return 0;
}

int iommu_get_iova_ranges(struct iommu_ctx *ctx, struct iommu_iova_range **ranges)
{
	*ranges = ctx->map.iova_ranges;

	return ctx->map.nranges;
}
