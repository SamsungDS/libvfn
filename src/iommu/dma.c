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

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "ccan/minmax/minmax.h"

#include "vfn/support.h"
#include "vfn/iommu.h"

#include "context.h"

static int iova_cmp(const void *vaddr, const struct skiplist_node *n)
{
	struct iova_mapping *m = container_of_var(n, m, list);

	if (vaddr < m->vaddr)
		return -1;
	else if (vaddr >= m->vaddr + m->len)
		return 1;

	return 0;
}

static int iova_map_add(struct iova_map *map, void *vaddr, size_t len, uint64_t iova)
{
	__autolock(&map->lock);

	struct skiplist_node *update[SKIPLIST_LEVELS] = {};
	struct iova_mapping *m;

	if (!len) {
		errno = EINVAL;
		return -1;
	}

	if (skiplist_find(&map->list, vaddr, iova_cmp, update)) {
		errno = EEXIST;
		return -1;
	}

	m = znew_t(struct iova_mapping, 1);

	m->vaddr = vaddr;
	m->len = len;
	m->iova = iova;

	skiplist_link(&map->list, &m->list, update);

	return 0;
}

static void iova_map_remove(struct iova_map *map, void *vaddr)
{
	__autolock(&map->lock);

	struct skiplist_node *n, *update[SKIPLIST_LEVELS] = {};

	n = skiplist_find(&map->list, vaddr, iova_cmp, update);
	if (!n)
		return;

	skiplist_erase(&map->list, n, update);
}

static struct iova_mapping *iova_map_find(struct iova_map *map, void *vaddr)
{
	__autolock(&map->lock);

	return container_of_or_null(skiplist_find(&map->list, vaddr, iova_cmp, NULL),
				    struct iova_mapping, list);
}

static void iova_map_clear_with(struct iova_map *map, skiplist_iter_fn fn, void *opaque)
{
	__autolock(&map->lock);

	skiplist_clear_with(&map->list, fn, opaque);
}

static void iova_map_clear(struct iova_map *map)
{
	iova_map_clear_with(map, NULL, NULL);
}

void iova_map_destroy(struct iova_map *map)
{
       iova_map_clear(map);

       free(map->iova_ranges);

       memset(map, 0x0, sizeof(*map));
}

static bool iova_map_translate(struct iova_map *map, void *vaddr, uint64_t *iova)
{
	struct iova_mapping *m = iova_map_find(map, vaddr);

	if (m) {
		*iova = m->iova + (vaddr - m->vaddr);
		return true;
	}

	return false;
}

int iova_map_reserve(struct iova_map *map, size_t len, uint64_t *iova)
{
	__autolock(&map->lock);

	if (!ALIGNED(len, __VFN_PAGESIZE)) {
		log_debug("len is not page aligned\n");
		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < map->nranges; i++) {
		struct iommu_iova_range *r = &map->iova_ranges[i];
		uint64_t next = map->next;

		if (r->last < next)
			continue;

		next = max_t(uint64_t, next, r->start);

		if (next > r->last || r->last - next + 1 < len)
			continue;

		map->next = next + len;

		*iova = next;

		return 0;
	}

	errno = ENOMEM;
	return -1;
}

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
