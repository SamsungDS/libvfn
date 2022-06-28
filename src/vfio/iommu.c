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
#include <unistd.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/vfio.h>

#include <vfn/support/atomic.h>
#include <vfn/support/compiler.h>
#include <vfn/support/log.h>
#include <vfn/support/mem.h>
#include <vfn/support/mutex.h>
#include <vfn/trace.h>
#include <vfn/vfio/iommu.h>
#include <vfn/vfio/state.h>

#include "ccan/compiler/compiler.h"
#include "ccan/list/list.h"
#include "ccan/minmax/minmax.h"

#include "iommu.h"

#define SKIPLIST_LEVELS 8

#define skip_list_top(h, k) \
	list_top(&(h)->heads[k], struct vfio_iova_map_entry, list[k])
#define skip_list_next(h, n, k) \
	list_next(&(h)->heads[k], (n), list[k])
#define skip_list_add_after(h, p, n, k) \
	list_add_after(&(h)->heads[k], &(p)->list[k], &(n)->list[k])
#define skip_list_add(h, n, k) \
	list_add(&(h)->heads[k], &(n)->list[k])
#define skip_list_del_from(h, n, k) \
	list_del_from(&(h)->heads[k], &(n)->list[k])
#define skip_list_for_each_safe(h, n, next) \
	list_for_each_safe(&(h)->heads[0], n, next, list[0])

struct vfio_iova_map_entry {
	struct vfio_iommu_mapping mapping;

	struct list_node list[SKIPLIST_LEVELS];
};

struct vfio_iova_map {
	pthread_mutex_t lock;

	int height;

	struct vfio_iova_map_entry nil, sentinel;
	struct list_head heads[SKIPLIST_LEVELS];
};

static void vfio_iova_map_init(struct vfio_iova_map *map)
{
	pthread_mutex_init(&map->lock, NULL);

	map->height = 0;
	map->nil.mapping.vaddr = (void *)UINT64_MAX;

	for (int k = 0; k < SKIPLIST_LEVELS; k++) {
		list_head_init(&map->heads[k]);

		skip_list_add(map, &map->nil, k);
		skip_list_add(map, &map->sentinel, k);
	}
}

static UNNEEDED void vfio_iova_map_clear(struct vfio_iova_map *map)
{
	__autolock(&map->lock);

	struct vfio_iova_map_entry *n, *next;

	skip_list_for_each_safe(map, n, next) {
		skip_list_del_from(map, n, 0);

		if (n != &map->nil && n != &map->sentinel)
			free(n);
	}
}

static struct vfio_iova_map_entry *__vfio_iova_map_find_path(struct vfio_iova_map *map, void *vaddr,
							     struct vfio_iova_map_entry **path)
{
	struct vfio_iova_map_entry *next, *p = &map->sentinel;
	int k = map->height;

	do {
		next = skip_list_next(map, p, k);
		while (vaddr >= next->mapping.vaddr + next->mapping.len) {
			p = next;
			next = skip_list_next(map, p, k);
		}

		if (path)
			path[k] = p;
	} while (--k >= 0);

	p = skip_list_next(map, p, 0);

	if (vaddr >= p->mapping.vaddr && vaddr < p->mapping.vaddr + p->mapping.len)
		return p;

	return NULL;
}

static struct vfio_iova_map_entry *vfio_iova_map_find_path(struct vfio_iova_map *map, void *vaddr,
							   struct vfio_iova_map_entry **path)
{
	__autolock(&map->lock);

	return __vfio_iova_map_find_path(map, vaddr, path);
}

struct vfio_iommu_mapping *vfio_iommu_find_mapping(struct vfio_iommu_state *iommu, void *vaddr)
{
	struct vfio_iova_map_entry *n;

	n = vfio_iova_map_find_path(iommu->map, vaddr, NULL);
	if (!n)
		return NULL;

	return &n->mapping;
}

static inline unsigned int random_level(void)
{
	unsigned int k = 0;

	while (k < SKIPLIST_LEVELS - 1 && (rand() > (RAND_MAX / 2)))
		k++;

	return k;
}

static void __vfio_iova_map_add(struct vfio_iova_map *map, struct vfio_iova_map_entry *n,
				struct vfio_iova_map_entry *update[SKIPLIST_LEVELS])
{
	int k;

	k = random_level();
	if (k > map->height) {
		k = ++(map->height);
		update[k] = &map->sentinel;
	}

	do {
		skip_list_add_after(map, update[k], n, k);
	} while (--k >= 0);
}

static int vfio_iova_map_add(struct vfio_iova_map *map, void *vaddr, size_t len, uint64_t iova)
{
	__autolock(&map->lock);

	struct vfio_iova_map_entry *n, *update[SKIPLIST_LEVELS] = {};

	if (__vfio_iova_map_find_path(map, vaddr, update)) {
		errno = EEXIST;
		return -1;
	}

	n = zmalloc(sizeof(*n));
	if (!n) {
		errno = ENOMEM;
		return -1;
	}

	n->mapping.vaddr = vaddr;
	n->mapping.len = len;
	n->mapping.iova = iova;

	__vfio_iova_map_add(map, n, update);

	return 0;
}

static void __vfio_iova_map_remove(struct vfio_iova_map *map, struct vfio_iova_map_entry *p,
				   struct vfio_iova_map_entry *update[SKIPLIST_LEVELS])
{
	struct vfio_iova_map_entry *next;

	for (int r = 0; r <= map->height; r++) {
		next = skip_list_next(map, update[r], r);
		if (next != p)
			break;

		skip_list_del_from(map, p, r);
	}

	free(p);

	/* reduce height if possible */
	while (map->height && skip_list_next(map, &map->sentinel, map->height) == &map->nil)
		map->height--;
}

static int vfio_iova_map_remove(struct vfio_iova_map *map, void *vaddr)
{
	__autolock(&map->lock);

	struct vfio_iova_map_entry *p, *update[SKIPLIST_LEVELS] = {};

	p = __vfio_iova_map_find_path(map, vaddr, update);
	if (!p) {
		errno = ENOENT;
		return -1;
	}

	__vfio_iova_map_remove(map, p, update);

	return 0;
}

void vfio_iommu_init(struct vfio_iommu_state *iommu)
{
	iommu->map = zmalloc(sizeof(struct vfio_iova_map));

	vfio_iova_map_init(iommu->map);

	pthread_mutex_init(&iommu->lock, NULL);

	iommu->nranges = 1;
	iommu->top = VFIO_IOVA_MIN;
	iommu->bottom = VFIO_IOVA_MAX;

	iommu->iova_ranges = zmalloc(sizeof(struct vfio_iova_range));
	iommu->iova_ranges[0].start = VFIO_IOVA_MIN;
	iommu->iova_ranges[0].end = VFIO_IOVA_MAX - 1;
}

int vfio_iommu_close(struct vfio_iommu_state *iommu)
{
	if (vfio_iommu_unmap_all(iommu))
		return -1;

	free(iommu->map);
	free(iommu->iova_ranges);

	memset(iommu, 0x0, sizeof(*iommu));

	return 0;
}

int vfio_iommu_add_mapping(struct vfio_iommu_state *iommu, void *vaddr, size_t len, uint64_t iova)
{
	if (!len) {
		errno = EINVAL;
		return -1;
	}

	if (vfio_iova_map_add(iommu->map, vaddr, len, iova))
		return -1;

	return 0;
}

void vfio_iommu_remove_mapping(struct vfio_iommu_state *iommu, void *vaddr)
{
	vfio_iova_map_remove(iommu->map, vaddr);
}

int vfio_iommu_map_dma(struct vfio_iommu_state *iommu, void *vaddr, size_t len, uint64_t iova)
{
	struct vfio_state *vfio = container_of(iommu, struct vfio_state, iommu);

	struct vfio_iommu_type1_dma_map dma_map = {
		.argsz = sizeof(dma_map),
		.vaddr = (uintptr_t)vaddr,
		.size  = len,
		.iova  = iova,
		.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE,
	};

	if (!ALIGNED(((uintptr_t)vaddr | len | iova), __VFN_PAGESIZE)) {
		__debug("vaddr, len or iova not page aligned\n");
		errno = EINVAL;
		return -1;
	}

	if (ioctl(vfio->container, VFIO_IOMMU_MAP_DMA, &dma_map)) {
		__debug("failed to map dma\n");
		return -1;
	}

	return 0;
}

int vfio_iommu_unmap_dma(struct vfio_iommu_state *iommu, size_t len, uint64_t iova)
{
	struct vfio_state *vfio = container_of(iommu, struct vfio_state, iommu);

	struct vfio_iommu_type1_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap),
		.size = len,
		.iova = iova,
	};

	__trace(VFIO_IOMMU_UNMAP_DMA) {
		__emit("iova 0x%" PRIx64 " len %zu\n", iova, len);
	}

	if (ioctl(vfio->container, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)) {
		__debug("failed to unmap dma\n");
		return -1;
	}

	return 0;
}

int vfio_iommu_unmap_all(struct vfio_iommu_state *iommu)
{
#ifdef VFIO_UNMAP_ALL
	struct vfio_state *vfio = container_of(iommu, struct vfio_state, iommu);

	struct vfio_iommu_type1_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap),
		.flags = VFIO_DMA_UNMAP_FLAG_ALL,
	};

	if (ioctl(vfio->container, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)) {
		__debug("failed to unmap dma\n");
		return -1;
	}

	vfio_iova_map_clear(iommu->map);
#else
	struct vfio_iova_map_entry *n, *next;
	struct vfio_iova_map *map = iommu->map;

	__autolock(&map->lock);

	skip_list_for_each_safe(map, n, next) {
		skip_list_del_from(map, n, 0);

		if (n != &map->nil && n != &map->sentinel) {
			if (vfio_iommu_unmap_dma(iommu, n->mapping.len, n->mapping.iova))
				__debug("failed to unmap dma: len %zu iova 0x%" PRIx64 "\n",
					n->mapping.len, n->mapping.iova);
			free(n);
		}
	}
#endif

	return 0;
}

int vfio_iommu_allocate_sticky_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova)
{
	__autolock(&iommu->lock);

	if (!ALIGNED(len, __VFN_PAGESIZE)) {
		__debug("len is not page aligned\n");
		errno = EINVAL;
		return -1;
	}

	for (unsigned int i = 0; i < iommu->nranges; i++) {
		struct vfio_iova_range *r = &iommu->iova_ranges[i];
		uint64_t top = iommu->top;

		if (r->end < top)
			continue;

		top = max_t(uint64_t, top, r->start);

		if (top > r->end || r->end - top + 1 < len)
			continue;

		iommu->top = top + len;

		*iova = top;

		return 0;
	}

	errno = ENOMEM;
	return -1;
}

int vfio_iommu_allocate_ephemeral_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova)
{
	__autolock(&iommu->lock);

	if (!ALIGNED(len, __VFN_PAGESIZE)) {
		__debug("len is not page aligned\n");
		errno = EINVAL;
		return -1;
	}

	for (int i = iommu->nranges - 1; i >= 0; i--) {
		struct vfio_iova_range *r = &iommu->iova_ranges[i];
		uint64_t bottom = iommu->bottom;

		if (r->start > bottom + 1)
			continue;

		bottom = min_t(uint64_t, bottom, r->end + 1);
		*iova = bottom - len;

		if ((bottom - len) < r->start || (bottom - len) - r->start < len)
			continue;

		iommu->bottom = bottom - len;
		iommu->nephemeral++;

		return 0;
	}

	errno = ENOMEM;
	return -1;
}

int vfio_iommu_allocate_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova,
			     bool ephemeral)
{
	if (ephemeral)
		return vfio_iommu_allocate_ephemeral_iova(iommu, len, iova);

	return vfio_iommu_allocate_sticky_iova(iommu, len, iova);
}

void vfio_iommu_recycle_ephemeral_iovas(struct vfio_iommu_state *iommu)
{
	__trace(VFIO_IOMMU_RECYCLE_EPHEMERAL_IOVAS) {
		__emit("iova range [0x%" PRIx64 "; 0x%llx]\n", iommu->bottom, VFIO_IOVA_MAX);
	}

	iommu->bottom = VFIO_IOVA_MAX;
}

int vfio_iommu_unmap_ephemeral_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t iova)
{
	if (vfio_iommu_unmap_dma(iommu, len, iova))
		return -1;

	if (--iommu->nephemeral == 0)
		vfio_iommu_recycle_ephemeral_iovas(iommu);

	return 0;
}

bool vfio_iommu_vaddr_to_iova(struct vfio_iommu_state *iommu, void *vaddr, uint64_t *iova)
{
	struct vfio_iommu_mapping *m = vfio_iommu_find_mapping(iommu, vaddr);

	if (m) {
		*iova = m->iova + (vaddr - m->vaddr);
		return true;
	}

	return false;
}


