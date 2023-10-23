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

#include "util/iova_map.h"

#ifndef HAVE_VFIO_DEVICE_BIND_IOMMUFD
struct vfio_group {
	int fd;
	struct vfio_container *container;

	char *path;
};
#endif // HAVE_VFIO_DEVICE_BIND_IOMMUFD

#define VFN_MAX_VFIO_GROUPS 64

struct vfio_container {
	int fd;
	struct iova_map map;
#ifndef HAVE_VFIO_DEVICE_BIND_IOMMUFD
	struct vfio_group groups[VFN_MAX_VFIO_GROUPS];
#else
	uint32_t ioas_id;
#endif // HAVE_VFIO_DEVICE_BIND_IOMMUFD

#define IOMMU_F_REQUIRE_IOVA (1 << 0)
	unsigned int flags;
};

extern struct vfio_container vfio_default_container;

int vfio_get_device_fd(struct vfio_container *vfio, const char *bdf);
int vfio_do_map_dma(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t *iova,
		    unsigned long flags);
int vfio_do_unmap_dma(struct vfio_container *vfio, size_t len, uint64_t iova);
