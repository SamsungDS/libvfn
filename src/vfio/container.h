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

struct vfio_group {
	int fd;
	struct vfio_container *container;

	const char *path;
};

#define VFN_MAX_VFIO_GROUPS 64

struct vfio_container {
	int fd;
	struct vfio_group groups[VFN_MAX_VFIO_GROUPS];
	struct iommu_state iommu;
};

extern struct vfio_container vfio_default_container;

int vfio_get_group_fd(struct vfio_container *vfio, const char *path);
int vfio_get_device_fd(struct vfio_container *vfio, const char *bdf);
