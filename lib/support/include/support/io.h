/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libsupport.
 *
 * Copyright (C) 2022 Klaus Jensen <its@irrelevant.dk>
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
