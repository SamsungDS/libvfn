/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef LIBVFN_NVME_H
#define LIBVFN_NVME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <byteswap.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>

#include <vfn/vfio.h>

#include <vfn/compiler.h>
#include <vfn/atomic.h>
#include <vfn/endian.h>
#include <vfn/trace.h>
#include <vfn/trace/events.h>

#include <vfn/nvme/types.h>
#include <vfn/nvme/util.h>
#include <vfn/nvme/mmio.h>
#include <vfn/nvme/queue.h>
#include <vfn/nvme/ctrl.h>
#include <vfn/nvme/rq.h>

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_NVME_H */
