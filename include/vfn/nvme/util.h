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

#define NVME_CID_AER (1 << 15)

#define __mps_to_pageshift(mps) (12 + mps)
#define __mps_to_pagesize(mps) (1ULL << __mps_to_pageshift(mps))

/**
 * nvme_crc64 - Calculate NVMe CRC64
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
 * nvme_aer - Submit an Asynchronous Event Request command
 * @ctrl: Controller reference
 * @opaque: Opaque data pointer
 *
 * Issue an Asynchronous Event Request command and associate @opaque with the
 * request tracker.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_aer(struct nvme_ctrl *ctrl, void *opaque);

/**
 * nvme_sync - Submit a command and wait for completion
 * @ctrl: Controller reference
 * @sq: Submission queue
 * @sqe: Submission queue entry
 * @buf: Command payload
 * @len: Command payload length
 * @cqe_copy: Completion queue entry to fill
 *
 * Submit a command and wait for completion in a synchronous manner. If a
 * spurious completion queue entry is posted (i.e., the command identifier is
 * different from the one set in @sqe), the CQE is ignored and an error message
 * is logged.
 *
 * **Note**: This function should only be used for synchronous commands where no
 * spurious CQEs are expected to be posted on the completion queue. Any spurious
 * CQEs will be logged and dropped.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_sync(struct nvme_ctrl *ctrl, struct nvme_sq *sq, union nvme_cmd *sqe, void *buf,
	      size_t len, struct nvme_cqe *cqe_copy);

/**
 * nvme_admin - Submit an Admin command and wait for completion
 * @ctrl: See &struct nvme_ctrl
 * @sqe: Submission queue entry
 * @buf: Command payload
 * @len: Command payload length
 * @cqe_copy: Completion queue entry to fill
 *
 * Shortcut for nvme_sync(), submitting to the admin submission queue.
 *
 * Return: On success, returns ``0``. On error, returnes ``-1`` and sets
 * ``errno``.
 */
int nvme_admin(struct nvme_ctrl *ctrl, union nvme_cmd *sqe, void *buf, size_t len,
	       struct nvme_cqe *cqe_copy);

/**
 * nvme_map_prp - Set up the Physical Region Pages in the data pointer of the
 *                command from a buffer that is contiguous in iova mapped
 *                memory.
 * @ctrl: &struct nvme_ctrl
 * @prplist: The first PRP list page address
 * @cmd: NVMe command prototype (&union nvme_cmd)
 * @iova: I/O Virtual Address
 * @len: Length of buffer
 *
 * Map a buffer of size @len into the command payload.
 *
 * Return: ``0`` on success, ``-1`` on error and sets errno.
 */
int nvme_map_prp(struct nvme_ctrl *ctrl, leint64_t *prplist, union nvme_cmd *cmd,
		 uint64_t iova, size_t len);

/**
 * nvme_mapv_prp - Set up the Physical Region Pages in the data pointer of
 *                 the command from an iovec.
 * @ctrl: &struct nvme_ctrl
 * @prplist: The first PRP list page address
 * @cmd: NVMe command prototype (&union nvme_cmd)
 * @iov: array of iovecs
 * @niov: number of iovec in @iovec
 *
 * Map the memory contained in @iov into the request PRPs. The first entry is
 * allowed to be unaligned, but the entry MUST end on a page boundary. All
 * subsequent entries MUST be page aligned.
 *
 * Return: ``0`` on success, ``-1`` on error and sets errno.
 */
int nvme_mapv_prp(struct nvme_ctrl *ctrl, leint64_t *prplist,
		  union nvme_cmd *cmd, struct iovec *iov, int niov);

/**
 * nvme_mapv_sgl - Set up a Scatter/Gather List in the data pointer of the
 *                 command from an iovec.
 * @ctrl: &struct nvme_ctrl
 * @seglist: SGL segment list page address
 * @cmd: NVMe command prototype (&union nvme_cmd)
 * @iov: array of iovecs
 * @niov: number of iovec in @iovec
 *
 * Map the memory contained in @iov into the request SGL.
 *
 * Return: ``0`` on success, ``-1`` on error and sets errno.
 */
int nvme_mapv_sgl(struct nvme_ctrl *ctrl, struct nvme_sgld *seglist, union nvme_cmd *cmd,
		  struct iovec *iov, int niov);

#endif /* LIBVFN_NVME_UTIL_H */
