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

#include "ccan/compiler/compiler.h"
#include "ccan/list/list.h"
#include "ccan/minmax/minmax.h"

#include "iova_map.h"

#define skiplist_top(h, k) \
	list_top(&(h)->heads[k], struct iova_mapping, list[k])
#define skiplist_next(h, n, k) \
	list_next(&(h)->heads[k], (n), list[k])
#define skiplist_add_after(h, p, n, k) \
	list_add_after(&(h)->heads[k], &(p)->list[k], &(n)->list[k])
#define skiplist_add(h, n, k) \
	list_add(&(h)->heads[k], &(n)->list[k])
#define skiplist_del_from(h, n, k) \
	list_del_from(&(h)->heads[k], &(n)->list[k])
#define skiplist_for_each_safe(h, n, next) \
	list_for_each_safe(&(h)->heads[0], n, next, list[0])

static void iova_map_list_init(struct iova_map_list *list)
{
	pthread_mutex_init(&list->lock, NULL);

	list->height = 0;
	list->nil.vaddr = (void *)UINT64_MAX;

	for (int k = 0; k < SKIPLIST_LEVELS; k++) {
		list_head_init(&list->heads[k]);

		skiplist_add(list, &list->nil, k);
		skiplist_add(list, &list->sentinel, k);
	}
}

static void iova_map_list_clear_with(struct iova_map_list *list, iova_map_iter_fn fn, void *opaque)
{
	__autolock(&list->lock);

	struct iova_mapping *m, *next;

	skiplist_for_each_safe(list, m, next) {
		skiplist_del_from(list, m, 0);

		if (m == &list->nil && m == &list->sentinel)
			continue;

		if (fn)
			fn(opaque, m);

		free(m);
	}
}

static struct iova_mapping *__iova_map_list_find_path(struct iova_map_list *list, void *vaddr,
						      struct iova_mapping **path)
{
	struct iova_mapping *next, *p = &list->sentinel;
	int k = list->height;

	do {
		next = skiplist_next(list, p, k);
		while (vaddr >= next->vaddr + next->len) {
			p = next;
			next = skiplist_next(list, p, k);
		}

		if (path)
			path[k] = p;
	} while (--k >= 0);

	p = skiplist_next(list, p, 0);

	if (vaddr >= p->vaddr && vaddr < p->vaddr + p->len)
		return p;

	return NULL;
}

static struct iova_mapping *iova_map_list_find_path(struct iova_map_list *list, void *vaddr,
						    struct iova_mapping **path)
{
	__autolock(&list->lock);

	return __iova_map_list_find_path(list, vaddr, path);
}

static inline int random_level(void)
{
	int k = 0;

	while (k < SKIPLIST_LEVELS - 1 && (rand() > (RAND_MAX / 2)))
		k++;

	return k;
}

static void __iova_map_list_add(struct iova_map_list *list, struct iova_mapping *m,
				struct iova_mapping *update[SKIPLIST_LEVELS])
{
	int k;

	k = random_level();
	if (k > list->height) {
		k = ++(list->height);
		update[k] = &list->sentinel;
	}

	do {
		skiplist_add_after(list, update[k], m, k);
	} while (--k >= 0);
}

static int iova_map_list_add(struct iova_map_list *list, void *vaddr, size_t len, uint64_t iova)
{
	__autolock(&list->lock);

	struct iova_mapping *m, *update[SKIPLIST_LEVELS] = {};

	if (__iova_map_list_find_path(list, vaddr, update)) {
		errno = EEXIST;
		return -1;
	}

	m = znew_t(struct iova_mapping, 1);
	if (!m) {
		errno = ENOMEM;
		return -1;
	}

	m->vaddr = vaddr;
	m->len = len;
	m->iova = iova;

	__iova_map_list_add(list, m, update);

	return 0;
}

static void __iova_map_list_remove(struct iova_map_list *list, struct iova_mapping *m,
				   struct iova_mapping *update[SKIPLIST_LEVELS])
{
	struct iova_mapping *next;

	for (int r = 0; r <= list->height; r++) {
		next = skiplist_next(list, update[r], r);
		if (next != m)
			break;

		skiplist_del_from(list, m, r);
	}

	free(m);

	/* reduce height if possible */
	while (list->height && skiplist_next(list, &list->sentinel, list->height) == &list->nil)
		list->height--;
}

static int iova_map_list_remove(struct iova_map_list *list, void *vaddr)
{
	__autolock(&list->lock);

	struct iova_mapping *m, *update[SKIPLIST_LEVELS] = {};

	m = __iova_map_list_find_path(list, vaddr, update);
	if (!m) {
		errno = ENOENT;
		return -1;
	}

	__iova_map_list_remove(list, m, update);

	return 0;
}

void iova_map_init(struct iova_map *map)
{
	iova_map_list_init(&map->list);

	pthread_mutex_init(&map->lock, NULL);

	map->nranges = 1;
	map->next = __VFN_IOVA_MIN;

	map->iova_ranges = znew_t(struct iova_range, 1);
	map->iova_ranges[0].start = __VFN_IOVA_MIN;
	map->iova_ranges[0].end = IOVA_MAX_39BITS - 1;
}

void iova_map_clear_with(struct iova_map *map, iova_map_iter_fn fn, void *opaque)
{
	iova_map_list_clear_with(&map->list, fn, opaque);
}

void iova_map_clear(struct iova_map *map)
{
	iova_map_list_clear_with(&map->list, NULL, NULL);
}

void iova_map_destroy(struct iova_map *map)
{
	iova_map_clear(map);

	free(map->iova_ranges);

	memset(map, 0x0, sizeof(*map));
}

int iova_map_add(struct iova_map *map, void *vaddr, size_t len, uint64_t iova)
{
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
		struct iova_range *r = &map->iova_ranges[i];
		uint64_t next = map->next;

		if (r->end < next)
			continue;

		next = max_t(uint64_t, next, r->start);

		if (next > r->end || r->end - next + 1 < len)
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
	return iova_map_list_find_path(&map->list, vaddr, NULL);
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
