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

#define log_fmt(fmt) "iommu/vfio: " fmt

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

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "ccan/str/str.h"
#include "ccan/compiler/compiler.h"

#include <linux/types.h>
#include <linux/vfio.h>

#include "container.h"

#include "vfn/iommu.h"
#include "vfn/trace.h"
#include "vfn/pci/util.h"

#include "vfn/support/atomic.h"
#include "vfn/support/compiler.h"
#include "vfn/support/log.h"
#include "vfn/support/mem.h"
#include "vfn/support/align.h"

struct vfio_container vfio_default_container = {};

static void UNNEEDED __unmap_mapping(void *opaque, struct iova_mapping *m)
{
	struct vfio_container *vfio = opaque;

	if (vfio_do_unmap_dma(vfio, m->len, m->iova))
		log_debug("failed to unmap dma: len %zu iova 0x%" PRIx64 "\n", m->len, m->iova);
}

#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE
static void iommu_get_cap_iova_ranges(struct iova_map *map, struct vfio_info_cap_header *cap)
{
	struct vfio_iommu_type1_info_cap_iova_range *cap_iova_range;
	size_t len;

	cap_iova_range = (struct vfio_iommu_type1_info_cap_iova_range *)cap;
	len = sizeof(struct vfio_iova_range) * cap_iova_range->nr_iovas;

	map->nranges = cap_iova_range->nr_iovas;
	map->iova_ranges = realloc(map->iova_ranges, len);
	memcpy(map->iova_ranges, cap_iova_range->iova_ranges, len);

	if (logv(LOG_INFO)) {
		for (int i = 0; i < map->nranges; i++) {
			struct iova_range *r = &map->iova_ranges[i];

			log_info("iova range %d is [0x%" PRIx64 "; 0x%" PRIx64 "]\n", i, r->start, r->end);
		}
	}
}
#endif

#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_DMA_AVAIL
static void iommu_get_cap_dma_avail(struct vfio_info_cap_header *cap)
{
	struct vfio_iommu_type1_info_dma_avail *dma;

	dma = (struct vfio_iommu_type1_info_dma_avail *)cap;

	log_info("dma avail %"PRIu32"\n", dma->avail);
}
#endif

#ifdef VFIO_IOMMU_INFO_CAPS
static int vfio_unmap_all(struct vfio_container *vfio)
{
#ifdef VFIO_UNMAP_ALL
	struct vfio_iommu_type1_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap),
		.flags = VFIO_DMA_UNMAP_FLAG_ALL,
	};

	if (ioctl(vfio->fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)) {
		log_debug("failed to unmap dma\n");
		return -1;
	}

	iova_map_clear(&vfio->map);
#else
	iova_map_clear_with(&vfio->map, __unmap_mapping, vfio);
#endif
	return 0;
}

static int vfio_iommu_uninit(struct vfio_container *vfio)
{
	if (vfio_unmap_all(vfio))
		return -1;

	iova_map_destroy(&vfio->map);

	return 0;
}

static struct vfio_iommu_type1_info *iommu_get_iommu_info(struct vfio_container *vfio)
{
	struct vfio_iommu_type1_info *iommu_info = NULL;
	uint32_t iommu_info_size = sizeof(*iommu_info);

	iommu_info = xmalloc(iommu_info_size);

	memset(iommu_info, 0x0, iommu_info_size);
	iommu_info->argsz = iommu_info_size;

	if (ioctl(vfio->fd, VFIO_IOMMU_GET_INFO, iommu_info)) {
		log_debug("failed to get iommu info\n");
		return NULL;
	}

	if (iommu_info->argsz > iommu_info_size) {
		log_info("iommu has extended info\n");

		iommu_info_size = iommu_info->argsz;
		iommu_info = realloc(iommu_info, iommu_info_size);
		memset(iommu_info, 0x0, iommu_info_size);
		iommu_info->argsz = iommu_info_size;

		if (ioctl(vfio->fd, VFIO_IOMMU_GET_INFO, iommu_info)) {
			log_debug("failed to get extended iommu info\n");
			vfio_iommu_uninit(vfio);
			return NULL;
		}
	}
	return iommu_info;
}

static int iommu_get_capabilities(struct vfio_container *vfio)
{
	__autofree struct vfio_iommu_type1_info *iommu_info = NULL;
	struct vfio_info_cap_header *cap;

	iommu_info = iommu_get_iommu_info(vfio);
	if (!iommu_info)
		return -1;

	if (!(iommu_info->flags & VFIO_IOMMU_INFO_CAPS))
		return -1;

	cap = (void *)iommu_info + iommu_info->cap_offset;

	do {
		switch (cap->id) {
#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE
		case VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE:
			iommu_get_cap_iova_ranges(&vfio->map, cap);
			break;
#endif
#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_DMA_AVAIL
		case VFIO_IOMMU_TYPE1_INFO_CAP_DMA_AVAIL:
			iommu_get_cap_dma_avail(cap);
			break;
#endif
		default:
			break;
		}

		if (!cap->next)
			break;

		cap = (void *)iommu_info + cap->next;
	} while (true);

	return 0;
}
#endif /* VFIO_IOMMU_INFO_CAPS */

static int vfio_configure_iommu(struct vfio_container *vfio)
{
	if (vfio->map.nranges)
		return 0;

	if (ioctl(vfio->fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU)) {
		log_debug("failed to set vfio iommu type\n");
		return -1;
	}

	iova_map_init(&vfio->map);

#ifdef VFIO_IOMMU_INFO_CAPS
	if (iommu_get_capabilities(vfio)) {
		log_debug("failed to get iommu capabilities\n");
		return -1;
	}
#endif

	return 0;
}

static int vfio_group_open(const char *path)
{
	int fd;

	struct vfio_group_status group_status = {
		.argsz = sizeof(group_status),
	};

	fd = open(path, O_RDWR);
	if (fd < 0) {
		log_debug("failed to open vfio group file: %s\n", strerror(errno));
		return -1;
	}

	if (ioctl(fd, VFIO_GROUP_GET_STATUS, &group_status)) {
		log_debug("failed to get vfio group status\n");
		goto close_fd;
	}

	if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
		errno = EINVAL;
		log_debug("vfio group is not viable\n");
		goto close_fd;
	}

	return fd;

close_fd:
	close(fd);
	return -1;
}

static int vfio_get_group_fd(struct vfio_container *vfio, const char *path)
{
	struct vfio_group *group;
	int i;

	for (i = 0; i < VFN_MAX_VFIO_GROUPS; i++) {
		group = &vfio->groups[i];

		if (group->path && streq(group->path, path))
			return group->fd;
	}

	for (i = 0; i < VFN_MAX_VFIO_GROUPS; i++) {
		group = &vfio->groups[i];

		if (!group->path)
			break;
	}

	if (i == VFN_MAX_VFIO_GROUPS) {
		errno = EMFILE;
		return -1;
	}

	group->path = strdup(path);
	if (!group->path)
		return -1;

	group->fd = vfio_group_open(group->path);
	if (group->fd < 0) {
		log_debug("failed to open vfio group\n");
		goto free_group_path;
	}

	if (ioctl(group->fd, VFIO_GROUP_SET_CONTAINER, &vfio->fd)) {
		log_debug("failed to add group to vfio container\n");
		goto free_group_path;
	}

	return group->fd;

free_group_path:
	free(group->path);

	return -1;
}

int vfio_get_device_fd(struct vfio_container *vfio, const char *bdf)
{
	__autofree char *group = NULL;
	int gfd, ret_fd;

	group = pci_get_iommu_group(bdf);
	if (!group) {
		log_error("could not determine iommu group for device %s\n", bdf);
		errno = EINVAL;
		return -1;
	}
	log_info("vfio iommu group is %s\n", group);

	gfd = vfio_get_group_fd(vfio, group);
	if (gfd < 0)
		return -1;

	if (vfio_configure_iommu(vfio)) {
		// JAG: Should we unset group VFIO_GROUP_UNSET_CONTAINER on failure?
		log_error("failed to configure iommu: %s\n", strerror(errno));
		return -1;
	}

	ret_fd = ioctl(gfd, VFIO_GROUP_GET_DEVICE_FD, bdf);
	if (ret_fd < 0) {
		log_debug("failed to get device fd\n");
		return -1;
	}

	return ret_fd;
}

int vfio_init_container(struct vfio_container *vfio)
{
	vfio->fd = open("/dev/vfio/vfio", O_RDWR);
	if (vfio->fd < 0) {
		log_debug("failed to open vfio device\n");
		return -1;
	}

	if (ioctl(vfio->fd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
		log_debug("invalid vfio version\n");
		return -1;
	}

	if (!ioctl(vfio->fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
		log_debug("vfio type 1 iommu not supported\n");
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

int vfio_do_map_dma(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t iova)
{
	struct vfio_iommu_type1_dma_map dma_map = {
		.argsz = sizeof(dma_map),
		.vaddr = (uintptr_t)vaddr,
		.size  = len,
		.iova  = iova,
		.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE,
	};

	trace_guard(VFIO_IOMMU_MAP_DMA) {
		trace_emit("vaddr %p iova 0x%" PRIx64 " len %zu\n", vaddr, iova, len);
	}

	if (!ALIGNED(((uintptr_t)vaddr | len | iova), __VFN_PAGESIZE)) {
		log_debug("vaddr, len or iova not page aligned\n");
		errno = EINVAL;
		return -1;
	}

	if (ioctl(vfio->fd, VFIO_IOMMU_MAP_DMA, &dma_map)) {
		log_error("could not map: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int vfio_do_unmap_dma(struct vfio_container *vfio, size_t len, uint64_t iova)
{
	struct vfio_iommu_type1_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap),
		.size = len,
		.iova = iova,
	};

	trace_guard(VFIO_IOMMU_UNMAP_DMA) {
		trace_emit("iova 0x%" PRIx64 " len %zu\n", iova, len);
	}

	if (ioctl(vfio->fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)) {
		log_error("could not unmap: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}
