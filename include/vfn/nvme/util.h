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
static inline int nvme_set_errno_from_cqe(struct nvme_cqe *cqe)
{
	errno = le16_to_cpu(cqe->sfp) >> 1 ? EIO : 0;

	return errno ? -1 : 0;
}

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
 * nvme_vm_assign_max_flexible - Assign the maximum number of flexible resources
 *                               to secondary controller
 * @ctrl: primary controller &struct nvme_ctrl
 * @scid: secondary controller identifier
 *
 * Use the VQFRSM and VIFRSM fields of the Primary Controller Capabilities
 * Identify data structure to determine the maximum number of flexible resources
 * that can be assigned to a single VF and assign that.
 *
 * Return: On success, returns ``0``; on error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_vm_assign_max_flexible(struct nvme_ctrl *ctrl, uint16_t scid);

/**
 * nvme_vm_set_online - Online a secondary controller
 * @ctrl: primary controller &struct nvme_ctrl
 * @scid: secondary controller identifier
 *
 * Online a secondary controller.
 *
 * Return: On success, returns ``0``; on error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_vm_set_online(struct nvme_ctrl *ctrl, uint16_t scid);

/**
 * nvme_vm_set_offline - Offline a secondary controller
 * @ctrl: primary controller &struct nvme_ctrl
 * @scid: secondary controller identifier
 *
 * Offline a secondary controller.
 *
 * Return: On success, returns ``0``; on error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_vm_set_offline(struct nvme_ctrl *ctrl, uint16_t scid);

/**
 * nvme_get_vf_cntlid - Get the controller identifier for a VF
 * @ctrl: primary controller &struct nvme_ctrl
 * @vfnum: virtual function number
 * @cntlid: output parameter for the secondary controller identifier
 *
 * Use the Secondary Controller List Identify data structure to determine the
 * controller identifier of the secondary controller identified by a Virtual
 * Function Number.
 *
 * Return: On success, returns ``0``; on error, returns ``-1`` and sets
 * ``errno``.
 */
int nvme_get_vf_cntlid(struct nvme_ctrl *ctrl, int vfnum, uint16_t *cntlid);

#endif /* LIBVFN_NVME_UTIL_H */
