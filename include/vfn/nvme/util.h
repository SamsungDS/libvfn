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

/**
 * nvme_aen_enable - Enable handling of AENs
 * @ctrl: Controller reference
 * @handler: A &cqe_handler_fn
 *
 * Issue an Asynchronous Event Request command and register @handler to be
 * called when the AER completes.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_aen_enable(struct nvme_ctrl *ctrl, cqe_handler handler);

/**
 * nvme_aen_handle - Handle an asynchronous event notification
 * @ctrl: Controller reference
 * @cqe: Completion queue entry for the AEN
 *
 * Handle an AEN by calling the handler previously registered with
 * nvme_aen_enable() and post a fresh Asynchronous Event Request command.
 */
void nvme_aen_handle(struct nvme_ctrl *ctrl, struct nvme_cqe *cqe);

/**
 * nvme_oneshot - Submit a command and wait for completion
 * @ctrl: Controller reference
 * @sq: Submission queue
 * @sqe: Submission queue entry
 * @buf: Command payload
 * @len: Command payload length
 * @cqe: Completion queue entry to fill
 *
 * Submit a command and wait for completion in a synchronous manner. If a
 * spurious completion queue entry is posted (i.e., the command identifier is
 * different from the one set in @sqe), the CQE is ignored and an error message
 * is logged. However, if waiting on the admin completion queue and the command
 * identifier indicates that this is an AEN, pass the cqe to a registered
 * handler (if any).
 *
 * **Note**: As the name indicate, this function should only be used for "one
 * shot" commands where it is guaranteed that no (non AEN) spurious CQEs are
 * posted on the completion queue.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_oneshot(struct nvme_ctrl *ctrl, struct nvme_sq *sq, void *sqe, void *buf, size_t len,
		 void *cqe);

/**
 * nvme_admin - Submit an Admin command and wait for completion
 * @ctrl: See &struct nvme_ctrl
 * @sqe: Submission queue entry
 * @buf: Command payload
 * @len: Command payload length
 * @cqe: Completion queue entry to fill
 *
 * Shortcut for nvme_oneshot(), submitting to the admin submission queue.
 *
 * Return: On success, returns ``0``. On error, returnes ``-1`` and sets
 * ``errno``.
 */
int nvme_admin(struct nvme_ctrl *ctrl, void *sqe, void *buf, size_t len, void *cqe);

#endif /* LIBVFN_NVME_UTIL_H */
