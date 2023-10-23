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

#ifndef LIBVFN_VFIO_H
#define LIBVFN_VFIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <pthread.h>

#include <linux/vfio.h>
#include <linux/pci_regs.h>

#include <vfn/vfio/device.h>
#include <vfn/vfio/pci.h>

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_VFIO_H */
