// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#define log_fmt(fmt) "vfio/core: " fmt

#include <assert.h>
#include <byteswap.h>
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
#include <sys/stat.h>

#include <linux/limits.h>
#include <linux/pci_regs.h>
#include <linux/vfio.h>

#include "vfn/support/align.h"
#include "vfn/support/atomic.h"
#include "vfn/support/compiler.h"
#include "vfn/support/log.h"
#include "vfn/support/mem.h"

#include "vfn/vfio/container.h"

#include "vfn/trace.h"

#include "ccan/compiler/compiler.h"
#include "ccan/str/str.h"

#include "iommu.h"
#include "container.h"

struct vfio_container vfio_default_container = {
	.fd = -1,
};

static int vfio_init_container(struct vfio_container *vfio)
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
	struct vfio_container *vfio = zmallocn(1, sizeof(*vfio));

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

#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE
static void iommu_get_cap_iova_ranges(struct iommu_state *iommu, struct vfio_info_cap_header *cap)
{
	struct vfio_iommu_type1_info_cap_iova_range *cap_iova_range;
	size_t len;

	cap_iova_range = (struct vfio_iommu_type1_info_cap_iova_range *)cap;
	len = sizeof(struct vfio_iova_range) * cap_iova_range->nr_iovas;

	iommu->nranges = cap_iova_range->nr_iovas;
	iommu->iova_ranges = realloc(iommu->iova_ranges, len);
	memcpy(iommu->iova_ranges, cap_iova_range->iova_ranges, len);

	if (logv(LOG_INFO)) {
		for (int i = 0; i < iommu->nranges; i++) {
			struct vfio_iova_range *r = &iommu->iova_ranges[i];

			log_info("iova range %d is [0x%llx; 0x%llx]\n", i, r->start, r->end);
		}
	}
}
#endif

#ifdef VFIO_IOMU_TYPE1_INFO_CAP_DMA_AVAIL
static void iommu_get_cap_dma_avail(struct iommu_state *iommu, struct vfio_info_cap_header *cap)
{
	struct vfio_iommu_type1_info_dma_avail *dma;

	dma = (struct vfio_iommu_type1_info_dma_avail *)cap;

	log_info("dma avail %"PRIu32"\n", dma->avail);
}
#endif

#ifdef VFIO_IOMMU_INFO_CAPS
static void iommu_get_capabilities(struct iommu_state *iommu,
				   struct vfio_iommu_type1_info *iommu_info)
{
	struct vfio_info_cap_header *cap = (void *)iommu_info + iommu_info->cap_offset;

	do {
		switch (cap->id) {
#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE
		case VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE:
			iommu_get_cap_iova_ranges(iommu, cap);
			break;
#endif
#ifdef VFIO_IOMMU_TYPE1_INFO_CAP_DMA_AVAIL
		case VFIO_IOMMU_TYPE1_INFO_CAP_DMA_AVAIL:
			iommu_get_cap_dma_avail(iommu, cap);
			break;
#endif
		default:
			break;
		}

		if (!cap->next)
			break;

		cap = (void *)iommu_info + cap->next;
	} while (true);
}
#endif

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

static int vfio_do_map_dma(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t iova)
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
		log_debug("failed to map dma\n");
		return -1;
	}

	return 0;
}

static int vfio_do_unmap_dma(struct vfio_container *vfio, size_t len, uint64_t iova)
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
		log_debug("failed to unmap dma\n");
		return -1;
	}

	return 0;
}

static void UNNEEDED __unmap_mapping(void *opaque, struct iova_mapping *m)
{
	struct vfio_container *vfio = opaque;

	if (vfio_do_unmap_dma(vfio, m->len, m->iova))
		log_debug("failed to unmap dma: len %zu iova 0x%" PRIx64 "\n", m->len, m->iova);
}

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

	iommu_clear(&vfio->iommu);
#else
	iommu_clear_with(&vfio->iommu, __unmap_mapping, vfio);
#endif
	return 0;
}

static int vfio_iommu_uninit(struct vfio_container *vfio)
{
	if (vfio_unmap_all(vfio))
		return -1;

	iommu_destroy(&vfio->iommu);

	return 0;
}

static int vfio_container_configure_iommu(struct vfio_container *vfio)
{
	__autofree struct vfio_iommu_type1_info *iommu_info = NULL;
	uint32_t iommu_info_size = sizeof(*iommu_info);

	if (vfio->iommu.nranges)
		return 0;

	if (ioctl(vfio->fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU)) {
		log_debug("failed to set vfio iommu type\n");
		return -1;
	}

	iommu_init(&vfio->iommu);

	iommu_info = xmalloc(iommu_info_size);
	memset(iommu_info, 0x0, iommu_info_size);
	iommu_info->argsz = iommu_info_size;

	if (ioctl(vfio->fd, VFIO_IOMMU_GET_INFO, iommu_info)) {
		log_debug("failed to get iommu info\n");
		return -1;
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
			return -1;
		}

#ifdef VFIO_IOMMU_INFO_CAPS
		if (iommu_info->flags & VFIO_IOMMU_INFO_CAPS)
			iommu_get_capabilities(&vfio->iommu, iommu_info);
#endif
	}

	return 0;
}

static int vfio_configure_iommu(struct vfio_container *vfio)
{
	return vfio_container_configure_iommu(vfio);
}

int vfio_get_group_fd(struct vfio_container *vfio, const char *path)
{
	struct vfio_group *group;
	int i, errno_saved;

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
		// errno is set by strdup
		return -1;

	group->fd = vfio_group_open(group->path);
	if (group->fd < 0) {
		errno_saved = errno;
		goto free_group_path;
	}

	if (ioctl(group->fd, VFIO_GROUP_SET_CONTAINER, &vfio->fd)) {
		log_debug("failed to add group to vfio container\n");
		errno_saved = errno;
		goto free_group_path;
	}

	if (vfio_configure_iommu(vfio)) {
		errno_saved = errno;
		log_error("failed to configure iommu: %s\n", strerror(errno_saved));
		goto unset_container;
	}

	return group->fd;

unset_container:
	if (ioctl(group->fd, VFIO_GROUP_UNSET_CONTAINER))
		log_debug("failed to remove group from vfio container\n");

free_group_path:
	free((void *)group->path);

	errno = errno_saved;

	return -1;
}

int vfio_map_vaddr(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t *iova)
{
	uint64_t _iova;

	if (iommu_vaddr_to_iova(&vfio->iommu, vaddr, &_iova))
		goto out;

	if (iommu_allocate_sticky_iova(&vfio->iommu, len, &_iova)) {
		log_debug("failed to allocate iova\n");
		return -1;
	}

	if (vfio_do_map_dma(vfio, vaddr, len, _iova)) {
		log_debug("failed to map dma\n");
		return -1;
	}

	if (iommu_add_mapping(&vfio->iommu, vaddr, len, _iova)) {
		log_debug("failed to add mapping\n");
		return -1;
	}

out:
	if (iova)
		*iova = _iova;

	return 0;
}

int vfio_unmap_vaddr(struct vfio_container *vfio, void *vaddr, size_t *len)
{
	struct iova_mapping *m;

	m = iommu_find_mapping(&vfio->iommu, vaddr);
	if (!m) {
		errno = ENOENT;
		return 0;
	}

	if (len)
		*len = m->len;

	if (vfio_do_unmap_dma(vfio, m->len, m->iova)) {
		log_debug("failed to unmap dma\n");
		return -1;
	}

	iommu_remove_mapping(&vfio->iommu, m->vaddr);

	return 0;
}

int vfio_map_vaddr_ephemeral(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t *iova)
{
	if (iommu_get_ephemeral_iova(&vfio->iommu, len, iova)) {
		log_error("failed to allocate ephemeral iova\n");

		return -1;
	}

	if (vfio_do_map_dma(vfio, vaddr, len, *iova)) {
		log_error("failed to map dma\n");

		if (atomic_dec_fetch(&vfio->iommu.nephemeral) == 0)
			iommu_recycle_ephemeral_iovas(&vfio->iommu);

		return -1;
	}

	return 0;
}

int vfio_unmap_ephemeral_iova(struct vfio_container *vfio, size_t len, uint64_t iova)
{
	if (vfio_do_unmap_dma(vfio, len, iova))
		return -1;

	iommu_put_ephemeral_iova(&vfio->iommu);

	return 0;
}

int vfio_get_iova_ranges(struct vfio_container *vfio, struct vfio_iova_range **ranges)
{
	*ranges = vfio->iommu.iova_ranges;

	return vfio->iommu.nranges;
}
