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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "vfn/nvme.h"

#include "crc64table.h"

uint64_t nvme_crc64(uint64_t crc, const unsigned char *buffer, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		crc = (crc >> 8) ^ crc64_nvme_table[(crc & 0xff) ^ buffer[i]];

	return crc ^ (uint64_t)~0;
}

int nvme_set_errno_from_cqe(struct nvme_cqe *cqe)
{
	errno = le16_to_cpu(cqe->sfp) >> 1 ? EIO : 0;

	return errno ? -1 : 0;
}
