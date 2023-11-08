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

#define log_fmt(fmt) "util/iova: " fmt

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
#include <unistd.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <vfn/support.h>
#include <vfn/iommu.h>

#include "ccan/minmax/minmax.h"

#include "iova_map.h"

static int iova_cmp(const void *vaddr, const struct skiplist_node *n)
{
	struct iova_mapping *m = container_of_var(n, m, list);

	if (vaddr < m->vaddr)
		return -1;
	else if (vaddr >= m->vaddr + m->len)
		return 1;

	return 0;
}

static int iova_map_list_add(struct skiplist *list, void *vaddr, size_t len, uint64_t iova)
{
	struct skiplist_node *update[SKIPLIST_LEVELS] = {};
	struct iova_mapping *m;

	if (skiplist_find(list, vaddr, iova_cmp, update)) {
		errno = EEXIST;
		return -1;
	}

	m = znew_t(struct iova_mapping, 1);

	m->vaddr = vaddr;
	m->len = len;
	m->iova = iova;

	skiplist_link(list, &m->list, update);

	return 0;
}

static void iova_map_list_remove(struct skiplist *list, void *vaddr)
{
	struct skiplist_node *n, *update[SKIPLIST_LEVELS] = {};

	n = skiplist_find(list, vaddr, iova_cmp, update);
	if (!n)
		return;

	skiplist_erase(list, n, update);
}

void iova_map_init(struct iova_map *map)
{
	skiplist_init(&map->list);

	pthread_mutex_init(&map->lock, NULL);

	map->nranges = 1;
	map->next = __VFN_IOVA_MIN;

	map->iova_ranges = znew_t(struct iommu_iova_range, 1);
	map->iova_ranges[0].start = __VFN_IOVA_MIN;
	map->iova_ranges[0].last = IOVA_MAX_39BITS - 1;
}

void iova_map_clear_with(struct iova_map *map, skiplist_iter_fn fn, void *opaque)
{
	__autolock(&map->lock);

	skiplist_clear_with(&map->list, fn, opaque);
}

void iova_map_clear(struct iova_map *map)
{
	iova_map_clear_with(map, NULL, NULL);
}

void iova_map_destroy(struct iova_map *map)
{
	iova_map_clear(map);

	free(map->iova_ranges);

	memset(map, 0x0, sizeof(*map));
}

int iova_map_add(struct iova_map *map, void *vaddr, size_t len, uint64_t iova)
{
	__autolock(&map->lock);

	if (!len) {
		errno = EINVAL;
		return -1;
	}

	if (iova_map_list_add(&map->list, vaddr, len, iova))
		return -1;

	return 0;
}

void iova_map_remove(struct iova_map *map, void *vaddr)
{
	__autolock(&map->lock);

	iova_map_list_remove(&map->list, vaddr);
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

struct iova_mapping *iova_map_find(struct iova_map *map, void *vaddr)
{
	__autolock(&map->lock);

	return container_of_or_null(skiplist_find(&map->list, vaddr, iova_cmp, NULL),
				    struct iova_mapping, list);
}

bool iova_map_translate(struct iova_map *map, void *vaddr, uint64_t *iova)
{
	struct iova_mapping *m = iova_map_find(map, vaddr);

	if (m) {
		*iova = m->iova + (vaddr - m->vaddr);
		return true;
	}

	return false;
}
