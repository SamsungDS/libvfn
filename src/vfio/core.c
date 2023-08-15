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

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/mman.h>

#include "vfn/support/atomic.h"
#include "vfn/support/compiler.h"
#include "vfn/support/log.h"
#include "vfn/support/mem.h"

#include "iommu.h"
#include "container.h"

struct vfio_container vfio_default_container = {};

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

int vfio_get_iova_ranges(struct vfio_container *vfio, struct iova_range **ranges)
{
	*ranges = vfio->iommu.iova_ranges;

	return vfio->iommu.nranges;
}
