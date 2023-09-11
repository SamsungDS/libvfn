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

#ifndef LIBVFN_NVME_RQ_H
#define LIBVFN_NVME_RQ_H

/**
 * struct nvme_rq - Request tracker
 * @opaque: Opaque data pointer
 */
struct nvme_rq {
	void *opaque;

	/* private: */
	struct nvme_sq *sq;

	uint16_t cid;

	struct {
		void *vaddr;
		uint64_t iova;
	} page;

	struct nvme_rq *rq_next;
};

/**
 * nvme_rq_reset - Reset a request tracker for reuse
 * @rq: &struct nvme_rq
 *
 * Reset internal state of a request tracker.
 */
static inline void nvme_rq_reset(struct nvme_rq *rq)
{
	rq->opaque = NULL;
}

/**
 * nvme_rq_release - Release a request tracker
 * @rq: &struct nvme_rq
 *
 * Release the request tracker and push it on the request tracker free stack.
 */
static inline void nvme_rq_release(struct nvme_rq *rq)
{
	struct nvme_sq *sq = rq->sq;

	nvme_rq_reset(rq);

	rq->rq_next = sq->rq_top;
	sq->rq_top = rq;
}

/**
 * nvme_rq_release_atomic - Release a request tracker atomically
 * @rq: &struct nvme_rq
 *
 * Lock-free (compare-and-swap) version of nvme_rq_release().
 */
static inline void nvme_rq_release_atomic(struct nvme_rq *rq)
{
	struct nvme_sq *sq = rq->sq;

	nvme_rq_reset(rq);

	rq->rq_next = atomic_load_acquire(&sq->rq_top);

	while (!atomic_cmpxchg(&sq->rq_top, rq->rq_next, rq))
		;
}

/**
 * nvme_rq_acquire - Acquire a request tracker
 * @sq: Submission queue (&struct nvme_sq)
 *
 * Acquire a request tracker from the free stack.
 *
 * Note: The &nvme_cmd.cid union member is initialized by this function, do not
 * clear it subsequent to this call.
 *
 * Return: A &struct nvme_rq or NULL if none are available.
 */
static inline struct nvme_rq *nvme_rq_acquire(struct nvme_sq *sq)
{
	struct nvme_rq *rq = sq->rq_top;

	if (!rq) {
		errno = EBUSY;
		return NULL;
	}

	sq->rq_top = rq->rq_next;

	return rq;
}

/**
 * nvme_rq_acquire_atomic - Acquire a request tracker atomically
 * @sq: Submission queue (&struct nvme_sq)
 *
 * Lock-free (compare-and-swap) version of nvme_rq_acquire().
 *
 * Return: A &struct nvme_rq or NULL if none are available.
 */
static inline struct nvme_rq *nvme_rq_acquire_atomic(struct nvme_sq *sq)
{
	struct nvme_rq *rq = atomic_load_acquire(&sq->rq_top);

	while (rq && !atomic_cmpxchg(&sq->rq_top, rq, rq->rq_next))
		;

	if (!rq)
		errno = EBUSY;

	return rq;
}

/**
 * __nvme_rq_from_cqe - Get the request tracker associated with completion queue
 *                      entry
 * @sq: Submission queue (&struct nvme_sq)
 * @cqe: Completion queue entry (&struct nvme_cqe)
 *
 * Get the request tracker associated with the completion queue entry @cqe.
 *
 * Note: Only safe when used with CQE's resulting from commands already
 * associated with a request tracker (see nvme_rq_acquire()).
 *
 * Return: The associated request tracker (see &struct nvme_rq).
 */
static inline struct nvme_rq *__nvme_rq_from_cqe(struct nvme_sq *sq, struct nvme_cqe *cqe)
{
	return &sq->rqs[cqe->cid];
}

/**
 * nvme_rq_from_cqe - Get the request tracker associated with completion queue
 *                    entry
 * @ctrl: See &struct nvme_ctrl
 * @cqe: Completion queue entry (&struct nvme_cqe)
 *
 * Get the request tracker associated with the completion queue entry @cqe.
 *
 * Note: Only safe when used with CQE's resulting from commands already
 * associated with a request tracker (see nvme_rq_acquire()).
 *
 * Return: The associated request tracker (see &struct nvme_rq).
 */
static inline struct nvme_rq *nvme_rq_from_cqe(struct nvme_ctrl *ctrl, struct nvme_cqe *cqe)
{
	struct nvme_sq *sq = &ctrl->sq[le16_to_cpu(cqe->sqid)];

	if (cqe->cid > sq->qsize - 1) {
		errno = EINVAL;

		return NULL;
	}

	return __nvme_rq_from_cqe(sq, cqe);
}

/**
 * nvme_rq_prep_cmd - Associate the request tracker with the given command
 * @rq: Request tracker (&struct nvme_rq)
 * @cmd: NVMe command prototype (&union nvme_cmd)
 *
 * Prepare @cmd and associate it with @rq (by setting the command identifier).
 */
static inline void nvme_rq_prep_cmd(struct nvme_rq *rq, union nvme_cmd *cmd)
{
	cmd->cid = rq->cid;
}

static inline void nvme_rq_post(struct nvme_rq *rq, union nvme_cmd *cmd)
{
	nvme_rq_prep_cmd(rq, cmd);
	nvme_sq_post(rq->sq, cmd);
}

/**
 * nvme_rq_exec - Execute the NVMe command on the submission queue associated
 *                with the given request tracker
 * @rq: Request tracker (&struct nvme_rq)
 * @cmd: NVMe command prototype (&union nvme_cmd)
 *
 * Prepare @cmd, post it to a submission queue and ring the doorbell.
 */
static inline void nvme_rq_exec(struct nvme_rq *rq, union nvme_cmd *cmd)
{
	nvme_rq_post(rq, cmd);
	nvme_sq_update_tail(rq->sq);
}

/**
 * nvme_rq_map_prp - Set up the Physical Region Pages in the data pointer of the
 *                   command from a buffer that is contiguous in iova mapped
 *                   memory.
 * @rq: Request tracker (&struct nvme_rq)
 * @cmd: NVMe command prototype (&union nvme_cmd)
 * @iova: I/O Virtual Address
 * @len: Length of buffer
 *
 * Map a buffer of size @len into the command payload.
 *
 * Return: ``0`` on success, ``-1`` on error and sets errno.
 */
int nvme_rq_map_prp(struct nvme_rq *rq, union nvme_cmd *cmd, uint64_t iova, size_t len, int mps);

/**
 * nvme_rq_mapv_prp - Set up the Physical Region Pages in the data pointer of
 *                    the command from an iovec.
 * @rq: Request tracker (&struct nvme_rq)
 * @cmd: NVMe command prototype (&union nvme_cmd)
 * @iov: array of iovecs
 * @niov: number of iovec in @iovec
 *
 * Map the IOVAs contained in @iov into the request PRPs. The first entry is
 * allowed to be unaligned, but the entry MUST end on a page boundary. All
 * subsequent entries MUST be page aligned.
 *
 * Return: ``0`` on success, ``-1`` on error and sets errno.
 */
int nvme_rq_mapv_prp(struct nvme_rq *rq, union nvme_cmd *cmd, struct iovec *iov, int niov, int mps);

/**
 * nvme_rq_spin - Spin for completion of the command associated with the request
 *                tracker
 * @rq: Request tracker (&struct nvme_rq)
 * @cqe_copy: Output parameter to copy completion queue entry into
 *
 * Spin on the completion queue associated with @rq until a completion queue
 * entry is available and copy it into @cqe_copy (if not NULL). If the
 * completed command identifer is different from the one associated with @rq,
 * set ``errno`` to ``EAGAIN`` and return ``-1``.
 *
 * Return: ``0`` on success, ``-1`` on error and set ``errno``.
 */
int nvme_rq_spin(struct nvme_rq *rq, struct nvme_cqe *cqe_copy);

#endif /* LIBVFN_NVME_RQ_H */
