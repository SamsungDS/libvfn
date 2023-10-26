// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include <errno.h>
#ifdef __GLIBC__
#include <execinfo.h>
#endif
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vfn/support.h"

ssize_t writeallfd(int fd, const void *buf, size_t count)
{
	ssize_t ret;
	size_t pos = 0;

	do {
		ret = write(fd, buf + pos, count - pos);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;

			return ret;
		}

		pos += ret;
	} while (pos < count);

	return count;
}

ssize_t writeall(const char *path, const void *buf, size_t count)
{
	int fd;
	ssize_t ret;

	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -1;

	ret = writeallfd(fd, buf, count);

	log_fatal_if(close(fd), "close\n");

	return ret;
}

ssize_t readmaxfd(int fd, void *buf, size_t count)
{
	ssize_t ret;
	size_t pos = 0;

	do {
		ret = read(fd, buf + pos, count - pos);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;

			return ret;
		} else if (ret == 0) {
			break;
		}

		pos += ret;
	} while (pos < count);

	return pos;
}

ssize_t readmax(const char *path, void *buf, size_t count)
{
	int fd;
	ssize_t ret;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = readmaxfd(fd, buf, count);

	log_fatal_if(close(fd), "close\n");

	return ret;
}
