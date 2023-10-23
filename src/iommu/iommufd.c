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

#define log_fmt(fmt) "iommu/iommufd: " fmt

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/types.h>
#include <linux/iommufd.h>
#include <linux/vfio.h>

#include "vfn/support/atomic.h"
#include "vfn/support/compiler.h"
#include "vfn/support/log.h"
#include "vfn/support/mem.h"
#include "vfn/trace.h"
#include "vfn/pci.h"

#include "vfn/vfio/container.h"
#include "vfn/iommu/dma.h"

#include "container.h"

struct vfio_container vfio_default_container = {};

static int vfio_configure_iommu(struct vfio_container *vfio)
{
	int ret;
	struct iommu_ioas_iova_ranges iova_ranges = {
		.size = sizeof(iova_ranges),
		.ioas_id = vfio->ioas_id,
		.num_iovas = 0,
	};

	if (vfio->map.nranges)
		return 0;

	iova_map_init(&vfio->map);

	ret = ioctl(vfio->fd, IOMMU_IOAS_IOVA_RANGES, &iova_ranges);
	if (ret && errno == EMSGSIZE) {
		size_t iova_range_len = iova_ranges.num_iovas * sizeof(struct iommu_iova_range);

		vfio->map.nranges = iova_ranges.num_iovas;
		vfio->map.iova_ranges = realloc(vfio->map.iova_ranges, iova_range_len);
		iova_ranges.allowed_iovas = (uintptr_t)&vfio->map.iova_ranges->start;

		if (ioctl(vfio->fd, IOMMU_IOAS_IOVA_RANGES, &iova_ranges))
			goto ioas_ranges_error;
	} else {
ioas_ranges_error:
		log_error("could not get ioas iova ranges %d\n", errno);
		return -1;
	}

	if (logv(LOG_INFO)) {
		for (int i = 0; i < vfio->map.nranges; i++) {
			struct iova_range *r = &vfio->map.iova_ranges[i];

			log_info("iova range %d is [0x%" PRIx64 "; 0x%" PRIx64 "]\n", i, r->start, r->end);
		}
	}

	return 0;
}

static int __vfio_iommufd_get_dev_fd(const char *bdf)
{
	__autofree char *vfio_id = NULL;
	__autofree char *path = NULL;
	int devfd;

	vfio_id = pci_get_device_vfio_id(bdf);
	if (!vfio_id) {
		log_debug("could not determine the vfio device id for %s\n", bdf);
		return -1;
	}

	if (asprintf(&path, "/dev/vfio/devices/%s", vfio_id) < 0) {
		log_debug("asprintf failed\n");
		return -1;
	}

	devfd = open(path, O_RDWR);
	if (devfd < 0)
		log_error("could not open %s\n", path);

	return devfd;
}

int vfio_get_device_fd(struct vfio_container *vfio, const char *bdf)
{
	int devfd, saved_errno;
	struct vfio_device_bind_iommufd bind = {
		.argsz = sizeof(bind),
		.flags = 0
	};

	struct iommu_ioas_alloc alloc_data = {
		.size = sizeof(alloc_data),
		.flags = 0,
	};

	struct vfio_device_attach_iommufd_pt attach_data = {
		.argsz = sizeof(attach_data),
		.flags = 0,
	};

	devfd = __vfio_iommufd_get_dev_fd(bdf);
	if (devfd < 0) {
		saved_errno = errno;
		log_error("failed to get the cdev file descriptor %s\n", strerror(saved_errno));
		return devfd;
	}

	bind.iommufd = vfio->fd;
	if (ioctl(devfd, VFIO_DEVICE_BIND_IOMMUFD, &bind)) {
		saved_errno = errno;
		log_error("could not bind iommufd %d to cdev %d, %s\n",
			  vfio->fd, devfd, strerror(saved_errno));
		goto close_dev;
	}

	if (ioctl(vfio->fd, IOMMU_IOAS_ALLOC, &alloc_data)) {
		saved_errno = errno;
		log_error("could not allocate i/o address space %s\n", strerror(saved_errno));
		goto close_dev;
	}

	attach_data.pt_id = alloc_data.out_ioas_id;
	if (ioctl(devfd, VFIO_DEVICE_ATTACH_IOMMUFD_PT, &attach_data)) {
		saved_errno = errno;
		log_error("could not attach cdev %d to ioas id %d, %s\n",
			  devfd, attach_data.pt_id, strerror(saved_errno));
		//JAG: Should we deallocate this explicitly?
		goto close_dev;
	}
	vfio->ioas_id = alloc_data.out_ioas_id;

	if (vfio_configure_iommu(vfio)) {
		saved_errno = errno;
		log_error("failed to configure iommu: %s\n", strerror(saved_errno));
		goto close_dev;
	}

	return devfd;

close_dev:
	/* closing devfd will automatically unbind it from iommufd */
	log_fatal_if(close(devfd), "close: %s\n", strerror(errno));

	errno = saved_errno;
	return -1;
}

static int vfio_init_container(struct vfio_container *vfio)
{
	vfio->fd = open("/dev/iommu", O_RDWR);
	if (vfio->fd < 0) {
		log_error("could not open /dev/iommu\n");
		return -1;
	}

	return 0;
}

struct vfio_container *vfio_new(void)
{
	struct vfio_container *vfio = znew_t(struct vfio_container, 1);

	if (vfio_init_container(vfio) < 0) {
		free(vfio);
		return NULL;
	}

	return vfio;
}

static void __attribute__((constructor)) open_default_container(void)
{
	if (vfio_init_container(&vfio_default_container))
		log_debug("default container not initialized\n");
}

int vfio_do_map_dma(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t iova,
		    unsigned long flags)
{
	struct iommu_ioas_map map = {
		.size = sizeof(map),
		.flags = IOMMU_IOAS_MAP_READABLE | IOMMU_IOAS_MAP_WRITEABLE |
			 IOMMU_IOAS_MAP_FIXED_IOVA,
		.__reserved = 0,
		.user_va = (uint64_t)vaddr,
		.iova = iova,
		.length = len,
		.ioas_id = vfio->ioas_id
	};

	if (flags & IOMMU_MAP_NOWRITE)
		map.flags &= ~IOMMU_IOAS_MAP_WRITEABLE;

	if (flags & IOMMU_MAP_NOREAD)
		map.flags &= ~IOMMU_IOAS_MAP_READABLE;

	trace_guard(IOMMU_MAP_DMA) {
		trace_emit("vaddr %p iova 0x%" PRIx64 " len %zu\n", vaddr, iova, len);
	}

	if (ioctl(vfio->fd, IOMMU_IOAS_MAP, &map)) {
		log_error("failed to map: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int vfio_do_unmap_dma(struct vfio_container *vfio, size_t len, uint64_t iova)
{
	struct iommu_ioas_unmap unmap = {
		.size = sizeof(unmap),
		.ioas_id = vfio->ioas_id,
		.iova = iova,
		.length = len
	};

	trace_guard(IOMMU_UNMAP_DMA) {
		trace_emit("iova 0x%" PRIx64 " len %zu\n", iova, len);
	}

	if (ioctl(vfio->fd, IOMMU_IOAS_UNMAP, &unmap)) {
		log_error("failed to unmap: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}
