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

int iommu_get_dmabuf(struct iommu_ctx *ctx, struct iommu_dmabuf *buffer, size_t len,
		     unsigned long flags)
{
	buffer->ctx = ctx;

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
	if (!buffer->len)
		return;

	log_fatal_if(iommu_unmap_vaddr(buffer->ctx, buffer->vaddr, NULL), "iommu_unmap_vaddr");

	pgunmap(buffer->vaddr, buffer->len);

	memset(buffer, 0x0, sizeof(*buffer));
}
