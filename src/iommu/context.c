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

#include "vfn/support.h"

#include "util/iova_map.h"
#include "context.h"

#ifdef HAVE_VFIO_DEVICE_BIND_IOMMUFD
static bool __iommufd_broken;

static void __attribute__((constructor)) __check_iommufd_broken(void)
{
	struct stat sb;

	if (stat("/dev/vfio/devices", &sb) || !S_ISDIR(sb.st_mode)) {
		log_info("iommufd broken; probably missing CONFIG_VFIO_DEVICE_CDEV=y\n");

		__iommufd_broken = true;
	}
}
#endif

struct iommu_ctx *iommu_get_default_context(void)
{
#ifdef HAVE_VFIO_DEVICE_BIND_IOMMUFD
	if (__iommufd_broken)
		goto fallback;

	return iommufd_get_default_iommu_context();

fallback:
#endif
	return vfio_get_default_iommu_context();
}

struct iommu_ctx *iommu_get_context(const char *name)
{
#ifdef HAVE_VFIO_DEVICE_BIND_IOMMUFD
	if (__iommufd_broken)
		goto fallback;

	return iommufd_get_iommu_context(name);

fallback:
#endif
	return vfio_get_iommu_context(name);
}
