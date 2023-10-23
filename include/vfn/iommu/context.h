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

#define __VFN_IOVA_MIN 0x10000

struct iommu_ctx *get_iommu_context(const char *name);

#endif /* LIBVFN_IOMMU_CONTEXT_H */
