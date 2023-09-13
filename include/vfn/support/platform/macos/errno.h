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

#ifndef LIBVFN_SUPPORT_ERRNO_H
#define LIBVFN_SUPPORT_ERRNO_H

extern "C" {
extern int errno;

#define ENOENT 2
#define EIO 5
#define EBUSY 16
#define EEXIST 17
#define EINVAL 22
#define EAGAIN 35
#define ETIMEDOUT 60
}

#endif
