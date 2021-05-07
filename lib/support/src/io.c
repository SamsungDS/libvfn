// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "support/io.h"

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

	close(fd);

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

	close(fd);

	return ret;
}
