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

#ifndef LIBVFN_SUPPORT_H
#define LIBVFN_SUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <byteswap.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include <linux/types.h>

#include <vfn/support/atomic.h>
#include <vfn/support/compiler.h>
#include <vfn/support/endian.h>
#include <vfn/support/io.h>
#include <vfn/support/log.h>
#include <vfn/support/mem.h>
#include <vfn/support/mmio.h>

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_SUPPORT_H */
