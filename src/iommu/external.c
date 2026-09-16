// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2026 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#define log_fmt(fmt) "iommu/external: " fmt

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ccan/compiler/compiler.h"
#include "ccan/list/list.h"
#include "ccan/str/str.h"

#include "vfn/iommu.h"
#include "vfn/support.h"

#include "context.h"

struct external_device {
	char *bdf;
	int fd;

	struct list_node list;
};

struct iommu_external {
	struct iommu_ctx ctx;

	struct iommu_dma_ops dma_ops;

	pthread_mutex_t lock;
	struct list_head devices;
};

static struct external_device *external_find_device(struct iommu_external *external,
						    const char *bdf)
{
	struct external_device *dev;

	list_for_each(&external->devices, dev, list) {
		if (streq(dev->bdf, bdf))
			return dev;
	}

	return NULL;
}

static void external_get_context(struct iommu_ctx *ctx)
{
	atomic_inc(&ctx->refcount);
}

static void external_destroy(struct iommu_external *external)
{
	struct external_device *dev, *next;

	list_for_each_safe(&external->devices, dev, next, list) {
		list_del(&dev->list);

		free(dev->bdf);
		free(dev);
	}

	iommu_clear_mappings(&external->ctx);

	free(external->ctx.iova_ranges);
	free(external);
}

static void external_put_context(struct iommu_ctx *ctx)
{
	struct iommu_external *external = container_of_var(ctx, external, ctx);

	if (atomic_dec_fetch(&ctx->refcount) == 0)
		external_destroy(external);
}

static int external_get_device_fd(struct iommu_ctx *ctx, const char *bdf)
{
	struct iommu_external *external = container_of_var(ctx, external, ctx);
	struct external_device *dev;

	__autolock(&external->lock);

	dev = external_find_device(external, bdf);
	if (!dev) {
		log_debug("no device fd registered for %s\n", bdf);
		errno = ENODEV;
		return -1;
	}

	return dev->fd;
}

/* the fd belongs to whoever opened the device, so it is not closed here */
static int external_put_device_fd(struct iommu_ctx *ctx, const char *bdf, int fd UNUSED)
{
	struct iommu_external *external = container_of_var(ctx, external, ctx);
	struct external_device *dev;

	__autolock(&external->lock);

	dev = external_find_device(external, bdf);
	if (!dev) {
		errno = ENODEV;
		return -1;
	}

	list_del(&dev->list);

	free(dev->bdf);
	free(dev);

	return 0;
}

static int external_dma_map(struct iommu_ctx *ctx, void *vaddr, size_t len, iova_t *iova,
			    unsigned long flags)
{
	struct iommu_external *external = container_of_var(ctx, external, ctx);

	return external->dma_ops.map(external->dma_ops.opaque, vaddr, len, iova, flags);
}

static int external_dma_unmap(struct iommu_ctx *ctx, iova_t iova, size_t len)
{
	struct iommu_external *external = container_of_var(ctx, external, ctx);

	return external->dma_ops.unmap(external->dma_ops.opaque, iova, len);
}

/*
 * The map cannot tell libvfn's mappings from the ones the owner registered for
 * translation, so unmapping all of them would take the owner's with it.
 */
static int external_dma_unmap_all(struct iommu_ctx *ctx UNUSED)
{
	errno = EOPNOTSUPP;

	return -1;
}

static const struct iommu_ctx_ops external_ops = {
	.get_context = external_get_context,
	.put_context = external_put_context,

	.dma_map = external_dma_map,
	.dma_unmap = external_dma_unmap,
	.dma_unmap_all = external_dma_unmap_all,

	.get_device_fd = external_get_device_fd,
	.put_device_fd = external_put_device_fd,
};

static int external_add_device(struct iommu_external *external, const char *bdf, int fd)
{
	struct external_device *dev;

	__autolock(&external->lock);

	if (external_find_device(external, bdf)) {
		errno = EEXIST;
		return -1;
	}

	dev = znew_t(struct external_device, 1);

	dev->bdf = strdup(bdf);
	dev->fd = fd;

	list_add(&external->devices, &dev->list);

	return 0;
}

int iommu_ctx_add_device(struct iommu_ctx *ctx, const char *bdf, int fd)
{
	struct iommu_external *external = NULL;

	if (ctx->ops.get_device_fd != external_get_device_fd) {
		log_debug("context does not take device fds\n");
		errno = EINVAL;
		return -1;
	}

	external = container_of_var(ctx, external, ctx);

	return external_add_device(external, bdf, fd);
}

struct iommu_ctx *iommu_get_context_external(const struct iommu_dma_ops *dma_ops)
{
	struct iommu_external *external;

	if (!dma_ops || !dma_ops->map || !dma_ops->unmap) {
		log_debug("an external context needs dma ops\n");
		errno = EINVAL;
		return NULL;
	}

	external = znew_t(struct iommu_external, 1);

	iommu_ctx_init(&external->ctx);

	list_head_init(&external->devices);
	pthread_mutex_init(&external->lock, NULL);

	external->dma_ops = *dma_ops;
	external->ctx.ops = external_ops;

	return &external->ctx;
}
