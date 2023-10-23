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

#ifndef LIBVFN_IOMMU_DMA_H
#define LIBVFN_IOMMU_DMA_H

/**
 * enum iommu_map_flags - flags for DMA mapping
 * @IOMMU_MAP_FIXED_IOVA: If cleared, an appropriate IOVA will be allocated
 * @IOMMU_MAP_WRITABLE: DMA is allowed to write to this mapping
 * @IOMMU_MAP_READABLE: DMA is allowed to read from this mapping
 */
enum iommu_map_flags {
	IOMMU_MAP_FIXED_IOVA = 1 << 0,
	IOMMU_MAP_NOWRITE = 1 << 1,
	IOMMU_MAP_NOREAD = 1 << 2,
};

/**
 * iommu_map_vaddr - map a virtual memory address to an I/O virtual address
 * @vfio: &struct vfio_container
 * @vaddr: virtual memory address to map
 * @len: number of bytes to map
 * @iova: output parameter for mapped I/O virtual address
 * @flags: combination of enum iommu_map_flags
 *
 * Allocate an I/O virtual address (iova) and program the IOMMU to create a
 * mapping to the virtual memory address in @vaddr. If @iova is not ``NULL``,
 * store the allocated iova in the pointee.
 *
 * If @vaddr falls within an already mapped area, calculate the corresponding
 * iova instead.
 *
 * Note that the allocated IOVA can never be reused in the lifetime of the
 * process, so use this for mapping memory that will be reused.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int iommu_map_vaddr(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t *iova,
		    unsigned long flags);

/**
 * iommu_unmap_vaddr - unmap a virtual memory address in the IOMMU
 * @vfio: &struct vfio_container
 * @vaddr: virtual memory address to unmap
 * @len: output parameter for length of mapping
 *
 * Remove the mapping associated with @vaddr. This can only be used with
 * mappings created using iommu_map_vaddr(). If @len is not NULL, the length of
 * the mapping will be written to the pointee.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int iommu_unmap_vaddr(struct vfio_container *vfio, void *vaddr, size_t *len);

struct iova_range {
	uint64_t start;
	uint64_t end;
};

int iommu_get_iova_ranges(struct vfio_container *vfio, struct iova_range **ranges);

#endif /* LIBVFN_IOMMU_DMA_H */
