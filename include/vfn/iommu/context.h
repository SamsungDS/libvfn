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

/**
 * iommu_get_context - create a new iommu context
 * @name: context identifier
 *
 * Create a new iommu context. The mechanism depends on the backend.
 *
 * Return: a new &struct iommu_ctx.
 */
struct iommu_ctx *iommu_get_context(const char *name);

#endif /* LIBVFN_IOMMU_CONTEXT_H */
