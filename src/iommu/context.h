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

struct iommu_ctx;

struct iommu_ctx_ops {
	/* container/ioas ops */
	int (*dma_map)(struct iommu_ctx *ctx, void *vaddr, size_t len, uint64_t *iova,
		       unsigned long flags);
	int (*dma_unmap)(struct iommu_ctx *ctx, uint64_t iova, size_t len);

	/* device ops */
	int (*get_device_fd)(struct iommu_ctx *ctx, const char *bdf);
};

struct iommu_ctx {
	struct iova_map map;
	struct iommu_ctx_ops ops;

#define IOMMU_F_REQUIRE_IOVA (1 << 0)
	unsigned int flags;
};

struct iommu_ctx *iommu_get_default_context(void);

struct iommu_ctx *vfio_get_default_iommu_context(void);
struct iommu_ctx *vfio_get_iommu_context(const char *name);

#ifdef HAVE_VFIO_DEVICE_BIND_IOMMUFD
struct iommu_ctx *iommufd_get_default_iommu_context(void);
struct iommu_ctx *iommufd_get_iommu_context(const char *name);
#endif
