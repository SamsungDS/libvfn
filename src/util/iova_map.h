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

#include "skiplist.h"

#define IOVA_MAX_39BITS (1ULL << 39)

struct iova_mapping {
	void *vaddr;
	size_t len;
	uint64_t iova;

	struct skiplist_node list;
};

struct iova_map {
	int nranges;
	struct iommu_iova_range *iova_ranges;

	pthread_mutex_t lock;

	uint64_t next;

	struct skiplist list;
};

void iova_map_init(struct iova_map *map);
void iova_map_destroy(struct iova_map *map);

int iova_map_add(struct iova_map *map, void *vaddr, size_t len, uint64_t iova);
void iova_map_remove(struct iova_map *map, void *vaddr);

int iova_map_reserve(struct iova_map *map, size_t len, uint64_t *iova);
bool iova_map_translate(struct iova_map *map, void *vaddr, uint64_t *iova);
struct iova_mapping *iova_map_find(struct iova_map *map, void *vaddr);

void iova_map_clear(struct iova_map *map);
void iova_map_clear_with(struct iova_map *map, skiplist_iter_fn fn, void *opaque);
