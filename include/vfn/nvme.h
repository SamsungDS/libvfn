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

#ifndef LIBVFN_NVME_H
#define LIBVFN_NVME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <byteswap.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include <linux/vfio.h>

#include <vfn/support.h>
#include <vfn/trace.h>
#include <vfn/trace/events.h>
#include <vfn/vfio.h>
#include <vfn/iommu.h>
#include <vfn/nvme/types.h>
#include <vfn/nvme/queue.h>
#include <vfn/nvme/ctrl.h>
#include <vfn/nvme/util.h>
#include <vfn/nvme/rq.h>

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_NVME_H */
