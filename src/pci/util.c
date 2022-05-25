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
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <linux/limits.h>

#include <support/io.h>
#include <support/log.h>
#include <support/mem.h>

#include "vfn/pci/util.h"

int pci_unbind(const char *bdf)
{
	char *path = NULL;
	struct stat sb;
	ssize_t ret;

	if (asprintf(&path, "/sys/bus/pci/devices/%s/driver/unbind", bdf) < 0) {
		__debug("asprintf failed\n");
		return -1;
	}

	ret = stat(path, &sb);
	if (ret < 0)
		goto out;

	ret = writeall(path, bdf, strlen(bdf));

out:
	free(path);

	return ret < 0 ? -1 : 0;
}

int pci_bind(const char *bdf, const char *driver)
{
	char *path = NULL;
	ssize_t ret;

	if (asprintf(&path, "/sys/bus/pci/drivers/%s/bind", driver) < 0) {
		__debug("asprintf failed\n");
		return -1;
	}

	ret = writeall(path, bdf, strlen(bdf));

	free(path);

	return ret < 0 ? -1 : 0;
}

int pci_driver_new_id(const char *driver, uint16_t vid, uint16_t did)
{
	char *path = NULL;
	char *id = NULL;
	ssize_t ret;

	if (asprintf(&path, "/sys/bus/pci/drivers/%s/new_id", driver) < 0) {
		__debug("asprintf failed\n");
		return -1;
	}

	if (asprintf(&id, "%x %x", vid, did) < 0) {
		__debug("asprintf failed\n");
		free(path);
		return -1;
	}

	ret = writeall(path, id, strlen(id));

	free(id);
	free(path);

	return ret < 0 ? -1 : 0;
}

int pci_driver_remove_id(const char *driver, uint16_t vid, uint16_t did)
{
	char *path = NULL;
	char *id = NULL;
	ssize_t ret;

	if (asprintf(&path, "/sys/bus/pci/drivers/%s/remove_id", driver) < 0) {
		__debug("asprintf failed\n");
		return -1;
	}

	if (asprintf(&id, "%x %x", vid, did) < 0) {
		__debug("asprintf failed\n");
		free(path);
		return -1;
	}

	ret = writeall(path, id, strlen(id));

	free(id);
	free(path);

	return ret < 0 ? -1 : 0;
}

int pci_device_info_get_ull(const char *bdf, const char *prop, unsigned long long *v)
{
	char buf[32], *endptr, *path = NULL;
	ssize_t ret;

	if (asprintf(&path, "/sys/bus/pci/devices/%s/%s", bdf, prop) < 0) {
		__debug("asprintf failed\n");
		return -1;
	}

	ret = readmax(path, buf, 32);
	if (ret < 0)
		goto out;

	buf[ret] = '\0';

	errno = 0;
	*v = strtoull(buf, &endptr, 0);
	if (endptr == buf)
		errno = EINVAL;

out:
	free(path);

	return errno ? -1 : 0;
}

char *pci_get_driver(const char *bdf)
{
	char *p, *link = NULL, *driver = NULL, *name = NULL;
	ssize_t ret;

	if (asprintf(&link, "/sys/bus/pci/devices/%s/driver", bdf) < 0) {
		link = NULL;
		__debug("asprintf failed\n");
		goto out;
	}

	driver = mallocn(PATH_MAX, sizeof(char));

	ret = readlink(link, driver, PATH_MAX - 1);
	if (ret < 0) {
		if (errno == ENOENT)
			goto out;

		__debug("failed to resolve driver link\n");
		goto out;
	}

	driver[ret] = '\0';

	p = strrchr(driver, '/');
	if (!p) {
		__debug("failed to determine driver name\n");
		goto out;
	}

	if (asprintf(&name, "%s", p + 1) < 0) {
		name = NULL;
		__debug("asprintf failed\n");
		goto out;
	}

out:
	free(link);
	free(driver);

	return name;
}
