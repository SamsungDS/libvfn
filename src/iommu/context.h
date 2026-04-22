/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2023 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include "util/skiplist.h"

struct iommu_ctx;

struct iommu_ctx_ops {
	/* container/ioas ops */
	int (*iova_reserve)(struct iommu_ctx *ctx, size_t len, iova_t *iova,
			    unsigned long flags);
	int (*iova_reserve_align)(struct iommu_ctx *ctx, size_t len, size_t align,
				  iova_t *iova, unsigned long flags);
	void (*iova_put_ephemeral)(struct iommu_ctx *ctx);
	int (*dma_map)(struct iommu_ctx *ctx, void *vaddr, size_t len, iova_t *iova,
		       unsigned long flags);
	int (*dma_unmap)(struct iommu_ctx *ctx, iova_t iova, size_t len);
	int (*dma_unmap_all)(struct iommu_ctx *ctx);

	/* device ops */
	int (*get_device_fd)(struct iommu_ctx *ctx, const char *bdf);
	int (*put_device_fd)(struct iommu_ctx *ctx, const char *bdf);
};

struct iova_mapping {
	void *vaddr;
	size_t len;
	iova_t iova;

	unsigned long flags;

	struct skiplist_node list;
};

struct iova_map {
	pthread_rwlock_t lock;
	struct skiplist list;
};

struct iommu_ctx {
	struct iova_map map;
	struct iommu_ctx_ops ops;

	int nranges;
	struct iommu_iova_range *iova_ranges;
	iova_t iova_max;
	iova_t iova_next_same;

	bool iommufd;
};

struct iommu_ctx *iommu_get_default_context(void);

struct iommu_ctx *vfio_get_default_iommu_context(void);
struct iommu_ctx *vfio_get_iommu_context(const char *name);

struct iommu_ctx *iommufd_get_default_iommu_context(void);
struct iommu_ctx *iommufd_get_iommu_context(const char *name);

void iommu_ctx_init(struct iommu_ctx *ctx);
int iommu_iova_range_to_string(struct iommu_iova_range *range, char **str);

/*
 * iommu_alloc_same_iova() mmaps memory at an address equal to the iova, so
 * the address returned by the same-iova allocator must fit within whatever
 * virtual address space the kernel/architecture actually gives userspace,
 * which can be much narrower than what the IOMMU reports as a valid iova
 * (e.g. AMD-Vi's default page tables grow dynamically and report an aperture
 * up to UINT64_MAX).
 *
 * 47 bits is the size of the default (4-level page table) address space on
 * x86-64, and is also the default map window on 5-level systems until a
 * process opts into the larger range, so it is a safe lower bound across
 * kernel configurations.
 */
#define IOMMU_MAX_SAME_IOVA ((iova_t)((1ULL << 47) - 1))

static inline void iommu_init_next_same(struct iommu_ctx *ctx)
{
	iova_t max = ctx->iova_max;

	if (max > IOMMU_MAX_SAME_IOVA)
		max = IOMMU_MAX_SAME_IOVA;

	ctx->iova_next_same = (max / 4) + 1;
}
