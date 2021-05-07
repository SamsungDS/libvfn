/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#define VFIO_IOVA_MIN 0x0
#define VFIO_IOVA_MAX (1ULL << 39)

struct vfio_iommu_mapping {
	void *vaddr;
	size_t len;
	uint64_t iova;

	unsigned int flags;
};

void vfio_iommu_init(struct vfio_iommu_state *iommu);
int vfio_iommu_close(struct vfio_iommu_state *iommu);
void vfio_iommu_init_capabilities(struct vfio_iommu_state *iommu,
				  struct vfio_iommu_type1_info *iommu_info);

struct vfio_iommu_mapping *vfio_iommu_find_mapping(struct vfio_iommu_state *iommu, void *vaddr);
int vfio_iommu_add_mapping(struct vfio_iommu_state *iommu, void *vaddr, size_t len, uint64_t iova);
void vfio_iommu_remove_mapping(struct vfio_iommu_state *iommu, void *vaddr);

int vfio_iommu_allocate_sticky_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova);
int vfio_iommu_allocate_ephemeral_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova);
int vfio_iommu_allocate_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova,
			     bool ephemeral);
int vfio_iommu_free_ephemeral_iovas(struct vfio_iommu_state *iommu, unsigned int n);
int vfio_iommu_recycle_ephemeral_iovas(struct vfio_iommu_state *iommu);
