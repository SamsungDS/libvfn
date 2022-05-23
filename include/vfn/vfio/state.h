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

#ifndef LIBVFN_VFIO_DEVICE_H
#define LIBVFN_VFIO_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct vfio_state - vfio device state
 */
struct vfio_state {
	/**
	 * @container: vfio container handle
	 */
	int container;

	/**
	 * @group: vfio iommu group
	 */
	int group;

	/**
	 * @device: vfio device handle
	 */
	int device;

	/**
	 * @device_info: vfio device information
	 */
	struct vfio_device_info device_info;

	/**
	 * @irq_info: vfio device irq information
	 */
	struct vfio_irq_info irq_info;

	/**
	 * @iommu: see &struct vfio_iommu_state
	 */
	struct vfio_iommu_state iommu;
};

/**
 * vfio_open - open and initialize vfio iommu group
 * @vfio: &struct vfio_state to initialize
 * @group: iommu group to open
 *
 * Open the iommu group identified by @group and initialize @vfio.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_open(struct vfio_state *vfio, const char *group);

/**
 * vfio_close - close vfio
 * @vfio: &struct vfio_state to close
 *
 * Free memory associated with @vfio. Reset if supported and close the vfio file
 * descriptors.
 */
void vfio_close(struct vfio_state *vfio);

/**
 * vfio_reset - reset vfio device
 * @vfio: &struct vfio_state holding device to reset
 *
 * Reset the VFIO device if suported.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_reset(struct vfio_state *vfio);

/**
 * vfio_map_vaddr - map a virtual memory address to an I/O virtual address
 * @vfio: &struct vfio_state
 * @vaddr: virtual memory address to map
 * @len: number of bytes to map
 * @iova: output parameter for mapped I/O virtual address
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
int vfio_map_vaddr(struct vfio_state *vfio, void *vaddr, size_t len, uint64_t *iova);

/**
 * vfio_unmap_vaddr - unmap a virtual memory address in the IOMMU
 * @vfio: &struct vfio_state
 * @vaddr: virtual memory address to unmap
 *
 * Remove the mapping associated with @vaddr. This is used for both normal and
 * ephemeral mappings.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_unmap_vaddr(struct vfio_state *vfio, void *vaddr);

/**
 * vfio_map_vaddr_ephemeral - map a virtual memory address to an I/O virtual address
 * @vfio: &struct vfio_state
 * @vaddr: virtual memory address to map
 * @len: number of bytes to map
 * @iova: output parameter for mapped I/O virtual address
 *
 * Allocate an I/O virtual address (iova) and program the IOMMU to create a
 * mapping to the virtual memory address in @vaddr. If @iova is not ``NULL``,
 * store the allocated iova in the pointee.
 *
 * The mapping will be "ephemeral", that is, it will not be tracked and
 * subsequent calls to either vfio_map_vaddr() or vfio_map_vaddr_ephemeral()
 * will not reuse the mapping. However, in contrast to the IOVAs allocated using
 * vfio_map_vaddr(), ephemeral mappings allows reuse of the allocated IOVAs.
 * Ephemeral mappings should be used for short-lived "one shot" commands and
 * should be unmapped as soon as possible. Any lingering ephemeral mapping will
 * block recycling of ephemeral IOVAs.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_map_vaddr_ephemeral(struct vfio_state *vfio, void *vaddr, size_t len, uint64_t *iova);

/**
 * vfio_unmap_ephemeral_iova - free an ephemeral iova
 * @vfio: &struct vfio_state
 * @len: length of mapping
 * @iova: I/O virtual address to unmap
 *
 * Free the ephemeral I/O virtual address indicated by @iova. If after unmapping
 * the address, there are no ephemeral iovas in use, the library will recycle
 * the ephemeral range.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_unmap_ephemeral_iova(struct vfio_state *vfio, size_t len, uint64_t iova);

/**
 * vfio_set_irq - Enable IRQs through eventfds
 * @vfio: &struct vfio_state
 * @eventfds: array of eventfds
 * @count: number of eventfds
 *
 * Enable interrupts for a range of vectors. See linux/vfio.h for documentation
 * on the format of @eventfds.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_set_irq(struct vfio_state *vfio, int *eventfds, unsigned int count);

/**
 * vfio_disable_irq - Disable all IRQs
 * @vfio: &struct vfio_state
 *
 * Disable all IRQs.
 */
int vfio_disable_irq(struct vfio_state *vfio);

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_VFIO_DEVICE_H */
