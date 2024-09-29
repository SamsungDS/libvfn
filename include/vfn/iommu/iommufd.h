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

#ifndef LIBVFN_IOMMU_IOMMUFD_H
#define LIBVFN_IOMMU_IOMMUFD_H

#ifdef IOMMU_FAULT_QUEUE_ALLOC
struct iommufd_fault_queue {
	int fault_id;
	int fault_fd;
};

int iommufd_alloc_fault_queue(struct iommufd_fault_queue *fq);
int iommufd_set_fault_queue(struct iommu_ctx *ctx, struct iommufd_fault_queue *fq, int devfd);
#endif

#endif /* LIBVFN_IOMMU_IOMMUFD_H */
