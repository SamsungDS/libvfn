/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include "ccan/list/list.h"

#define IOVA_MAX_39BITS (1ULL << 39)

#define SKIPLIST_LEVELS 8

struct iova_mapping {
	void *vaddr;
	size_t len;
	uint64_t iova;

	struct list_node list[SKIPLIST_LEVELS];
};

struct iova_map_list {
	pthread_mutex_t lock;

	int height;

	struct iova_mapping nil, sentinel;
	struct list_head heads[SKIPLIST_LEVELS];
};

struct iova_map {
	int nranges;
	struct iommu_iova_range *iova_ranges;

	pthread_mutex_t lock;

	uint64_t next;

	struct iova_map_list list;
};

typedef void (*iova_map_iter_fn)(void *opaque, struct iova_mapping *m);

void iova_map_init(struct iova_map *map);
void iova_map_destroy(struct iova_map *map);

int iova_map_add(struct iova_map *map, void *vaddr, size_t len, uint64_t iova);
void iova_map_remove(struct iova_map *map, void *vaddr);

int iova_map_reserve(struct iova_map *map, size_t len, uint64_t *iova);
bool iova_map_translate(struct iova_map *map, void *vaddr, uint64_t *iova);
struct iova_mapping *iova_map_find(struct iova_map *map, void *vaddr);

void iova_map_clear(struct iova_map *map);
void iova_map_clear_with(struct iova_map *map, iova_map_iter_fn fn, void *opaque);
