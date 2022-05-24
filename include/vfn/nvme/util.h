/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
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

#ifndef LIBVFN_NVME_UTIL_H
#define LIBVFN_NVME_UTIL_H

#define NVME_VERSION(major, minor, tertiary) \
	(((major) << 16) | ((minor) << 8) | (tertiary))

/**
 * nvme_crc64 - calculate NVMe CRC64
 * @crc: starting value
 * @buffer: buffer to calculate CRC for
 * @len: length of buffer
 *
 * Return: the NVMe CRC64 calculated over buffer
 */
uint64_t nvme_crc64(uint64_t crc, const unsigned char *buffer, size_t len);

/**
 * nvme_cqe_ok - Check the status field of CQE
 * @cqe: Completion queue entry
 *
 * Check the Status Field of the CQE.
 *
 * Return: ``true`` on no error, ``false`` otherwise.
 */
static inline bool nvme_cqe_ok(struct nvme_cqe *cqe)
{
	uint16_t status = le16_to_cpu(cqe->sfp) >> 1;

	return status == 0x0;
}

/**
 * nvme_set_errno_from_cqe - Set errno from CQE
 * @cqe: Completion queue entry
 *
 * Set ``errno`` to an error describing the NVMe status indicated by the Status
 * Field in @cqe.
 *
 * Return: ``0`` when no error in @cqe, otherwise ``-1`` and set ``errno``.
 */
int nvme_set_errno_from_cqe(struct nvme_cqe *cqe);

#endif /* LIBVFN_NVME_UTIL_H */
