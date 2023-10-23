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

#define log_fmt(fmt) "iommu/dma: " fmt

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "ccan/list/list.h"

#include "vfn/vfio.h"
#include "vfn/iommu.h"
#include "vfn/support.h"

#include "container.h"

int iommu_map_vaddr(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t *iova,
		    unsigned long flags)
{
	uint64_t _iova;

	if (iova_map_translate(&vfio->map, vaddr, &_iova))
		goto out;

	if (flags & IOMMU_MAP_FIXED_IOVA) {
		_iova = *iova;
	} else if (iova_map_reserve(&vfio->map, len, &_iova)) {
		log_debug("failed to allocate iova\n");
		return -1;
	}

	if (vfio_do_map_dma(vfio, vaddr, len, _iova)) {
		log_debug("failed to map dma\n");
		return -1;
	}

	if (iova_map_add(&vfio->map, vaddr, len, _iova)) {
		log_debug("failed to add mapping\n");
		return -1;
	}

out:
	if (iova)
		*iova = _iova;

	return 0;
}

int iommu_unmap_vaddr(struct vfio_container *vfio, void *vaddr, size_t *len)
{
	struct iova_mapping *m;

	m = iova_map_find(&vfio->map, vaddr);
	if (!m) {
		errno = ENOENT;
		return -1;
	}

	if (len)
		*len = m->len;

	if (vfio_do_unmap_dma(vfio, m->len, m->iova)) {
		log_debug("failed to unmap dma\n");
		return -1;
	}

	iova_map_remove(&vfio->map, m->vaddr);

	return 0;
}

int iommu_get_iova_ranges(struct vfio_container *vfio, struct iova_range **ranges)
{
	*ranges = vfio->map.iova_ranges;

	return vfio->map.nranges;
}
