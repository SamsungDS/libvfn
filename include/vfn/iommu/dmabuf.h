/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2023 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#ifndef LIBVFN_IOMMU_DMABUF_H
#define LIBVFN_IOMMU_DMABUF_H

/**
 * DOC: DMA-buffer helpers
 *
 * This provides a helper for allocating and mapping DMA buffers.
 *
 * &struct iommu_dmabuf is also registered as an "autovar" and can be
 * automatically unmapped and deallocated when going out of scope. For this
 * reason, this header is not included in <vfn/iommu.h> and must explicitly be
 * included.
 *
 * Required includes:
 *   #include <vfn/iommu.h>
 *   #include <vfn/support/autoptr.h>
 *   #include <vfn/iommu/dmabuf.h>
 */


/**
 * struct iommu_dmabuf - DMA buffer abstraction
 * @ctx: &struct iommu_ctx
 * @vaddr: data buffer
 * @iova: mapped address
 * @len: length of @vaddr
 *
 * Convenience wrapper around a mapped data buffer.
 */
struct iommu_dmabuf {
	struct iommu_ctx *ctx;

	void *vaddr;
	uint64_t iova;
	ssize_t len;
};

/**
 * iommu_get_dmabuf - Allocate and map a DMA buffer
 * @ctx: &struct iommu_ctx
 * @buffer: uninitialized &struct iommu_dmabuf
 * @len: desired minimum length
 * @flags: combination of enum iommu_map_flags
 *
 * Allocate at least @len bytes and map the buffer within the IOVA address space
 * described by @ctx. The actual allocated and mapped length may be larger than
 * requestes due to alignment requirements.
 *
 * Return: On success, returns ``0``; on error, returns ``-1`` and sets
 * ``errno``.
 */
int iommu_get_dmabuf(struct iommu_ctx *ctx, struct iommu_dmabuf *buffer, size_t len,
		     unsigned long flags);

/**
 * iommu_put_dmabuf - Unmap and deallocate a DMA buffer
 * @buffer: &struct iommu_dmabuf
 *
 * Unmap the buffer and deallocate it.
 */
void iommu_put_dmabuf(struct iommu_dmabuf *buffer);

static inline void __do_iommu_put_dmabuf(void *p)
{
	struct iommu_dmabuf *buffer = (struct iommu_dmabuf *)p;

	iommu_put_dmabuf(buffer);
}

DEFINE_AUTOVAR_STRUCT(iommu_dmabuf, __do_iommu_put_dmabuf);

#endif /* LIBVFN_IOMMU_DMABUF_H */
