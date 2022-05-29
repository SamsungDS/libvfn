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

#define VFIO_IOVA_MIN 0x0
#define VFIO_IOVA_MAX (1ULL << 39)

void vfio_iommu_init(struct vfio_iommu_state *iommu);
int vfio_iommu_close(struct vfio_iommu_state *iommu);
void vfio_iommu_init_capabilities(struct vfio_iommu_state *iommu,
				  struct vfio_iommu_type1_info *iommu_info);

int vfio_iommu_add_mapping(struct vfio_iommu_state *iommu, void *vaddr, size_t len, uint64_t iova);
void vfio_iommu_remove_mapping(struct vfio_iommu_state *iommu, void *vaddr);

int vfio_iommu_allocate_sticky_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova);
int vfio_iommu_allocate_ephemeral_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova);
int vfio_iommu_allocate_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t *iova,
			     bool ephemeral);
int vfio_iommu_unmap_ephemeral_iova(struct vfio_iommu_state *iommu, size_t len, uint64_t iova);
void vfio_iommu_recycle_ephemeral_iovas(struct vfio_iommu_state *iommu);
