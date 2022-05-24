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

#ifndef LIBSUPPORT_IO_H
#define LIBSUPPORT_IO_H

#ifdef __cplusplus
extern "C" {
#endif

ssize_t writeallfd(int fd, const void *buf, size_t count);
ssize_t writeall(const char *path, const void *buf, size_t count);
ssize_t readmaxfd(int fd, void *buf, size_t count);
ssize_t readmax(const char *path, void *buf, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* LIBSUPPORT_IO_H */
