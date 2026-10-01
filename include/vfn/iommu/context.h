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

#ifndef LIBVFN_IOMMU_CONTEXT_H
#define LIBVFN_IOMMU_CONTEXT_H

struct iommu_dma_ops;

/**
 * iommu_get_context - Create a new iommu context
 * @name: Context identifier
 *
 * Create a new iommu context. The mechanism depends on the backend.
 *
 * Return: A new &struct iommu_ctx.
 */
struct iommu_ctx *iommu_get_context(const char *name);

/**
 * iommu_get_context_external - Create an iommu context on another library's iova space
 * @dma_ops: mapping callbacks of that library
 *
 * Create an iommu context for devices that another library has opened, such as
 * the dpdk or spdk process a program shares its iova space with. The context
 * has no container or ioas of its own. It maps through @dma_ops, so that one
 * allocator hands out every iova, and it drives the devices registered with
 * iommu_ctx_add_device(). See &struct iommu_dma_ops.
 *
 * Which of vfio and iommufd the owner is using makes no difference here,
 * because the context does no mapping and opens no device itself.
 *
 * Return: A new &struct iommu_ctx, or ``NULL`` on error.
 */
struct iommu_ctx *iommu_get_context_external(const struct iommu_dma_ops *dma_ops);

/**
 * iommu_ctx_add_device - Give a context a device fd to drive
 * @ctx: &struct iommu_ctx from iommu_get_context_external()
 * @bdf: pci device identifier ("bus:device:function")
 * @fd: vfio device fd, open and attached to the iova space of @ctx
 *
 * Register the device fd that vfio_pci_open() uses for @bdf. Where that fd
 * comes from is up to the owner of the iova space, such as
 * rte_vfio_setup_device() in dpdk.
 *
 * The fd is not closed by vfio_pci_close(), because it belongs to the library
 * that opened it.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int iommu_ctx_add_device(struct iommu_ctx *ctx, const char *bdf, int fd);

/**
 * iommu_put_context - Release a reference to an iommu context
 * @ctx: &struct iommu_ctx
 *
 * Drop the reference returned by iommu_get_context(). The context is
 * destroyed when the last reference is dropped. A context created on
 * behalf of a device by vfio_pci_open() is released by vfio_pci_close().
 */
void iommu_put_context(struct iommu_ctx *ctx);

/**
 * iommu_ctx_is_iommufd - Check whether the given @ctx is binded to iommufd
 * @ctx: &struct iommu_ctx
 *
 * Return: ``true`` if @ctx is binded to iommufd, otherwise ``false``.
 */
bool iommu_ctx_is_iommufd(struct iommu_ctx *ctx);

#endif /* LIBVFN_IOMMU_CONTEXT_H */
