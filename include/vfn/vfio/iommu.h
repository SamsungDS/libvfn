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

#ifndef LIBVFN_VFIO_IOMMU_H
#define LIBVFN_VFIO_IOMMU_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE
struct vfio_iova_range {
	__u64	start;
	__u64	end;
};
#endif

/**
 * struct vfio_iommu_state - I/O Memory Management Unit state
 * @nranges: number of iommu iova ranges
 * @iova_ranges: array of iommu iova ranges
 */
struct vfio_iommu_state {
	unsigned int nranges;
	struct vfio_iova_range *iova_ranges;

	/* private: */
	pthread_mutex_t lock;

	uint64_t top, bottom;
	unsigned int nephemeral;

	void *map;
};

/**
 * struct vfio_iommu_mapping - iommu mapping
 * @vaddr: virtual memory address
 * @len: length of mapping
 * @iova: assigned I/O virtual address
 */
struct vfio_iommu_mapping {
	void *vaddr;
	size_t len;
	uint64_t iova;

	/* private: */
	unsigned int flags;
};

/**
 * vfio_iommu_map_dma - create mapping between @vaddr and @iova
 * @iommu: iommu state
 * @vaddr: virtual memory address to map
 * @len: mapping length in bytes
 * @iova: I/O virtual address to map
 *
 * Program the IOMMU to create a mapping between @vaddr and @iova.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int vfio_iommu_map_dma(struct vfio_iommu_state *iommu, void *vaddr, size_t len, uint64_t iova);

/**
 * vfio_iommu_unmap_dma - remove mapping associated with @iova
 * @iommu: iommu state
 * @len: mapping length in bytes
 * @iova: I/O virtual address to unmap
 *
 * Remove all mappings that falls within the area covered by @iova and @len.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int vfio_iommu_unmap_dma(struct vfio_iommu_state *iommu, size_t len, uint64_t iova);

/**
 * vfio_iommu_unmap_all - unmap all addresses
 * @iommu: iommu state
 *
 * Unmap all addresses currently mapped.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int vfio_iommu_unmap_all(struct vfio_iommu_state *iommu);

/**
 * vfio_iommu_vaddr_to_iova - get I/O virtual address associated with @vaddr
 * @iommu: see &struct vfio_iommu_state
 * @vaddr: virtual memory address
 * @iova: output parameter
 *
 * Look up the I/O virtual addresss associated with @vaddr. @vaddr my be an
 * address within an already mapped area.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
bool vfio_iommu_vaddr_to_iova(struct vfio_iommu_state *iommu, void *vaddr, uint64_t *iova);

/**
 * vfio_iommu_find_mapping - lookup the iommu mapping for the given virtual
 *                           memory address
 * @iommu: see &struct vfio_iommu_state
 * @vaddr: virtual memory address to look up
 *
 * Look up the iommu mapping corresponding to @vaddr.
 *
 * Return: The mapping associated with @vaddr or NULL if not found.
 */
struct vfio_iommu_mapping *vfio_iommu_find_mapping(struct vfio_iommu_state *iommu, void *vaddr);

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_VFIO_IOMMU_H */
