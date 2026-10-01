// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2024 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include <string.h>

#include <sys/types.h>

#include <vfn/iommu.h>

#include <vfn/support/autoptr.h>
#include <vfn/iommu/dmabuf.h>

#include <vfn/support.h>

#include "context.h"

int iommu_get_dmabuf(struct iommu_ctx *ctx, struct iommu_dmabuf *buffer, size_t len,
		     unsigned long flags)
{
	size_t alloc_len = ALIGN_UP(len, __VFN_PAGESIZE);
	void *vaddr;
	iova_t iova;

	buffer->ctx = ctx;

	if (ctx->ops.dma_alloc) {
		vaddr = ctx->ops.dma_alloc(ctx, alloc_len, &iova);
		if (!vaddr)
			return -1;

		if (iommu_add_mapping(ctx, vaddr, alloc_len, iova, flags)) {
			ctx->ops.dma_free(ctx, vaddr, alloc_len);
			return -1;
		}

		buffer->vaddr = vaddr;
		buffer->iova = iova;
		buffer->len = (ssize_t)alloc_len;

		return 0;
	}

	buffer->len = pgmap(&buffer->vaddr, len);
	if (buffer->len < 0)
		return -1;

	if (iommu_map_vaddr(ctx, buffer->vaddr, buffer->len, &buffer->iova, flags)) {
		pgunmap(buffer->vaddr, buffer->len);
		return -1;
	}

	return 0;
}

void iommu_put_dmabuf(struct iommu_dmabuf *buffer)
{
	struct iommu_ctx *ctx = buffer->ctx;

	if (!buffer->len)
		return;

	if (ctx->ops.dma_free) {
		iommu_del_mapping(ctx, buffer->vaddr);

		ctx->ops.dma_free(ctx, buffer->vaddr, (size_t)buffer->len);

		memset(buffer, 0x0, sizeof(*buffer));

		return;
	}

	log_fatal_if(iommu_unmap_vaddr(ctx, buffer->vaddr, NULL), "iommu_unmap_vaddr");

	pgunmap(buffer->vaddr, buffer->len);

	memset(buffer, 0x0, sizeof(*buffer));
}
