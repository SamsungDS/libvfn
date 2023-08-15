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

#define log_fmt(fmt) "iommufd/core: " fmt

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
#include <dirent.h>
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

#include "vfn/vfio/container.h"

#include "vfio/iommu.h"
#include "vfio/container.h"

static int vfio_configure_iommu(struct vfio_container *vfio)
{
	int ret;
	struct iommu_ioas_iova_ranges iova_ranges = {
		.size = sizeof(iova_ranges),
		.ioas_id = vfio->ioas_id,
		.num_iovas = 0,
	};

	if (vfio->iommu.nranges)
		return 0;

	iommu_init(&vfio->iommu);

	ret = ioctl(vfio->fd, IOMMU_IOAS_IOVA_RANGES, &iova_ranges);
	if (ret && errno == EMSGSIZE) {
		size_t iova_range_len = iova_ranges.num_iovas * sizeof(struct iova_range);

		vfio->iommu.nranges = iova_ranges.num_iovas;
		vfio->iommu.iova_ranges = realloc(vfio->iommu.iova_ranges, iova_range_len);
		iova_ranges.allowed_iovas = (uintptr_t)&vfio->iommu.iova_ranges->start;

		if (ioctl(vfio->fd, IOMMU_IOAS_IOVA_RANGES, &iova_ranges))
			goto ioas_ranges_error;
	} else {
ioas_ranges_error:
		log_error("Error. Could not get IOAS ranges %d\n", errno);
		return -1;
	}

	if (logv(LOG_INFO)) {
		for (int i = 0; i < vfio->iommu.nranges; i++) {
			struct iova_range *r = &vfio->iommu.iova_ranges[i];

			log_info("iova range %d is [0x%llx; 0x%llx]\n", i, r->start, r->end);
		}
	}

	return 0;
}

static int vfio_iommufd_get_device_id(const char *bdf, unsigned long *id)
{
	int err;
	char *path = NULL, *dir_nptr, *dir_endptr = NULL;
	DIR *dp;
	struct dirent *dentry;

	if (asprintf(&path, "/sys/bus/pci/devices/%s/vfio-dev", bdf) < 0) {
		log_debug("asprintf failed\n");
		return -1;
	}

	dp = opendir(path);
	if (!dp) {
		err = errno;
		log_debug("opendir failed (errno %d)\n", err);
		goto out;
	}

	do {
		dentry = readdir(dp);
		if (!dentry) {
			err = errno;
			log_debug("readdir failed (errno %d)\n", err);
			goto close_dir;
		}

		if (strncmp("vfio", dentry->d_name, 4) == 0)
			break;
	} while (dentry != NULL);

	if (dentry == NULL) {
		log_debug("Directory vfioX was not found in %s\n", path);
		err = -1;
		goto close_dir;
	}

	dir_nptr = dentry->d_name + 4;
	*id = strtoul(dir_nptr, &dir_endptr, 10);
	if (*id == ULONG_MAX || dir_nptr == dir_endptr) {
		err = -1;
		log_debug("Could not extrace vfio id from directory\n");
		goto close_dir;
	}

	err = 0;

close_dir:
	if (closedir(dp)) {
		err = errno;
		log_debug("closedir filed (errno %d)\n", err);
	}
out:
	free(path);
	return err;
}

static int __vfio_iommufd_get_dev_fd(const char *bdf)
{
	unsigned long vfio_id;
	int devfd;
	__autofree char *devpath;

	if (vfio_iommufd_get_device_id(bdf, &vfio_id)) {
		log_error("Could not determine the vfio device id for %s\n", bdf);
		errno = EINVAL;
		return -1;
	}

	if (asprintf(&devpath, "/dev/vfio/devices/vfio%ld", vfio_id) < 0) {
		log_debug("asprintf failed\n");
		errno = EINVAL;
		return -1;
	}

	devfd = open(devpath, O_RDWR);
	if (devfd < 0)
		log_error("Error opening %s\n", devpath);

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
		log_error("Error binding iommufd %d to cdev %d, %s\n",
			  vfio->fd, devfd, strerror(saved_errno));
		goto close_dev;
	}

	if (ioctl(vfio->fd, IOMMU_IOAS_ALLOC, &alloc_data)) {
		saved_errno = errno;
		log_error("Error allocating IO address space %s\n", strerror(saved_errno));
		goto close_dev;
	}

	attach_data.pt_id = alloc_data.out_ioas_id;
	if (ioctl(devfd, VFIO_DEVICE_ATTACH_IOMMUFD_PT, &attach_data)) {
		saved_errno = errno;
		log_error("Error attaching cdev %d to ioas id %d, %s\n",
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
	if (close(devfd))
		log_error("Error closing cdev FD: %d\n", devfd);

	errno = saved_errno;
	return -1;
}

int vfio_init_container(struct vfio_container *vfio)
{
	vfio->fd = open("/dev/iommu", O_RDWR);
	if (vfio->fd < 0) {
		log_error("Error opening /dev/iommu\n");
		return -1;
	}

	return 0;
}

int vfio_do_map_dma(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t iova)
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

	if (ioctl(vfio->fd, IOMMU_IOAS_MAP, &map)) {
		log_error("Error while mapping %s\n", strerror(errno));
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

	if (ioctl(vfio->fd, IOMMU_IOAS_UNMAP, &unmap)) {
		log_error("Error while un-mapping %s\n", strerror(errno));
		return -1;
	}

	return 0;
}
