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

#define log_fmt(fmt) "iommu/context: " fmt

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include <sys/stat.h>

#include "vfn/iommu.h"
#include "vfn/support.h"

#include "context.h"

#define IOVA_MIN 0x10000
#define IOVA_MAX_39BITS (1ULL << 39)

static inline bool __iommufd_is_available(void)
{
	struct stat sb;

	if (stat("/dev/vfio/devices", &sb) || !S_ISDIR(sb.st_mode))
		return false;
	if (stat("/dev/iommu", &sb))
		return false;

	return true;
}

struct iommu_ctx *iommu_get_default_context(void)
{
	if (!__iommufd_is_available())
		goto fallback;

	if (getenv("VFN_IOMMU_FORCE_VFIO"))
		goto fallback;

	return iommufd_get_default_iommu_context();

fallback:
	return vfio_get_default_iommu_context();
}

struct iommu_ctx *iommu_get_context(const char *name)
{
	if (!__iommufd_is_available())
		goto fallback;

	if (getenv("VFN_IOMMU_FORCE_VFIO"))
		goto fallback;

	return iommufd_get_iommu_context(name);

fallback:
	return vfio_get_iommu_context(name);
}

void iommu_ctx_init(struct iommu_ctx *ctx)
{
	ctx->nranges = 1;
	ctx->iova_ranges = znew_t(struct iommu_iova_range, ctx->nranges);

	/*
	 * For vfio, if we end up not being able to get a list of allowed
	 * iova ranges, be conservative.
	 */
	ctx->iova_ranges[0].start = IOVA_MIN;
	ctx->iova_ranges[0].last = IOVA_MAX_39BITS - 1;

	skiplist_init(&ctx->map.list);
	pthread_rwlock_init(&ctx->map.lock, NULL);
}

bool iommu_ctx_is_iommufd(struct iommu_ctx *ctx)
{
	return ctx->iommufd;
}
