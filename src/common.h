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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __APPLE__
#include <stdbool.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/vfio.h>
#ifdef __GLIBC__
#include <execinfo.h>
#endif
#else
#include <vfn/support/platform/macos/byteswap.h>
#include <vfn/support/platform/macos/errno.h>
#define NVME_AQ_QSIZE 32

extern int errno;

#include <time.h>
#endif

#ifdef __cplusplus
}
#endif

#ifdef __APPLE__
#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/PCIDriverKit.h>
#include <DriverKit/IODMACommand.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#endif
