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

#ifndef LIBVFN_VFIO_CONTAINER_H
#define LIBVFN_VFIO_CONTAINER_H

#define __VFN_IOVA_MIN 0x10000

/**
 * vfio_new - create a new vfio container
 *
 * Create a new VFIO container.
 *
 * Return: Container handle or ``NULL`` on error.
 */
struct vfio_container *vfio_new(void);

#define IOMMU_MAP_FIXED_IOVA (1 << 0)

/**
 * vfio_map_vaddr - map a virtual memory address to an I/O virtual address
 * @vfio: &struct vfio_container
 * @vaddr: virtual memory address to map
 * @len: number of bytes to map
 * @iova: output parameter for mapped I/O virtual address
 * @flags: combination of IOMMU_MAP flags
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
int vfio_map_vaddr(struct vfio_container *vfio, void *vaddr, size_t len, uint64_t *iova,
		   unsigned long flags);

/**
 * vfio_unmap_vaddr - unmap a virtual memory address in the IOMMU
 * @vfio: &struct vfio_container
 * @vaddr: virtual memory address to unmap
 * @len: output parameter for length of mapping
 *
 * Remove the mapping associated with @vaddr. This can only be used with
 * mappings created using vfio_map_vaddr(). If @len is not NULL, the length of
 * the mapping will be written to the pointee.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_unmap_vaddr(struct vfio_container *vfio, void *vaddr, size_t *len);

#ifndef VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE
struct vfio_iova_range {
	__u64	start;
	__u64	end;
};
#endif

int vfio_get_iova_ranges(struct vfio_container *vfio, struct vfio_iova_range **ranges);

#endif /* LIBVFN_VFIO_CONTAINER_H */
