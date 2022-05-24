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

#ifndef LIBVFN_NVME_QUEUE_H
#define LIBVFN_NVME_QUEUE_H

#define NVME_AQ   0
#define NVME_SQES 6
#define NVME_CQES 4

#define NVME_CID_AER (1 << 15)

/**
 * struct nvme_cq - Completion Queue
 */
struct nvme_cq {
	/* private: */
	void *vaddr;
	uint64_t iova;

	int id;
	int head;
	int qsize;
	size_t entry_size;

	/* memory-mapped register */
	void *doorbell;

	int phase;
	int efd;
};

/**
 * struct nvme_sq - Submission Queue
 */
struct nvme_sq {
	/* private: */
	struct nvme_cq *cq;

	void *vaddr;
	uint64_t iova;

	struct {
		void *vaddr;
		uint64_t iova;
	} pages;

	int tail, ptail;
	int qsize;
	int id;
	size_t entry_size;

	/* memory-mapped register */
	void *doorbell;

	/* rq stack */
	struct nvme_rq *rqs;
	struct nvme_rq *rq_top;
};

struct nvme_qpair {
	struct nvme_sq *sq;
	struct nvme_cq *cq;
};

/**
 * nvme_sq_post - Add a submission queue entry to a submission queue
 * @sq: Submission queue
 * @sqe: Submission queue entry
 *
 * Add a submission queue entry to a submission queue, updating the queue tail
 * pointer in the process.
 */
static inline void nvme_sq_post(struct nvme_sq *sq, const void *sqe)
{
	memcpy(sq->vaddr + (sq->tail << NVME_SQES), sqe, 1 << NVME_SQES);

	__trace(NVME_SQ_POST) {
		__emit("sqid %d tail %d\n", sq->id, sq->tail);
	}

	if (++sq->tail == sq->qsize)
		sq->tail = 0;
}

/**
 * nvme_sq_run - Ring the submission queue doorbell
 * @sq: Submission queue
 *
 * Ring the queue doorbell if the tail pointer has changed since last ringed.
 */
static inline void nvme_sq_run(struct nvme_sq *sq)
{
	if (sq->tail == sq->ptail)
		return;

	__trace(NVME_SQ_RUN) {
		__emit("sqid %d tail %d\n", sq->id, sq->tail);
	}

	/* force submission queue write before ringing the doorbell */
	wmb(); mmio_write32(sq->doorbell, cpu_to_le32(sq->tail));
	sq->ptail = sq->tail;
}

/**
 * nvme_sq_exec - Post submission queue entry and ring the doorbell
 * @sq: Submission queue
 * @sqe: Submission queue entry
 *
 * Combine the effects of nvme_sq_post() and nvme_sq_run().
 */
static inline void nvme_sq_exec(struct nvme_sq *sq, const void *sqe)
{
	nvme_sq_post(sq, sqe);
	nvme_sq_run(sq);
}

/**
 * nvme_cq_peek - Get a pointer to the current completion queue head
 * @cq: Completion queue
 *
 * Return: Pointer to completion queue head entry
 */
static inline struct nvme_cqe *nvme_cq_peek(struct nvme_cq *cq)
{
	return (struct nvme_cqe *)(cq->vaddr + (cq->head << NVME_CQES));
}

/**
 * nvme_cq_update_head - Ring the completion queue head doorbell
 * @cq: Completion queue
 *
 * Ring the completion queue head doorbell.
 */
static inline void nvme_cq_update_head(struct nvme_cq *cq)
{
	__trace(NVME_CQ_UPDATE_HEAD) {
		__emit("cqid %d head %d\n", cq->id, cq->head);
	}

	mmio_write32(cq->doorbell, cpu_to_le32(cq->head));
}

/**
 * nvme_cq_spin - Continuously read the top completion queue entry until phase
 *                change
 * @cq: Completion queue
 *
 * Keep reading the current head of the completion queue @cq until the phase
 * changes.
 */
static inline void nvme_cq_spin(struct nvme_cq *cq)
{
	struct nvme_cqe *cqe = nvme_cq_peek(cq);

	__trace(NVME_CQ_SPIN) {
		__emit("cq %d\n", cq->id);
	}

	/*
	 * The compiler can easily prove that the loop never changes the memory
	 * location where the top cqe is read from, so it emits code that checks
	 * it once and then just loops forever.
	 *
	 * Deter the compiler from doing this optimization by forcing a volatile
	 * access with LOAD().
	 */
	while ((le16_to_cpu(LOAD(cqe->sfp)) & 0x1) == cq->phase)
		;
}

/**
 * nvme_cq_get_cqe - Get a pointer to the current completion queue head and
 *                   advance it
 * @cq: Completion queue
 *
 * If the current completion queue head entry is valid (has the right phase),
 * advance the head and return a pointer to it. Otherwise, return NULL and leave
 * the head as-is.
 *
 * Return: If the current completion queue head entry is valid (correct phase),
 * return a pointer to it. Otherwise, return NULL.
 */
static inline struct nvme_cqe *nvme_cq_get_cqe(struct nvme_cq *cq)
{
	struct nvme_cqe *cqe = nvme_cq_peek(cq);

	__trace(NVME_CQ_GET_CQE) {
		__emit("cq %d\n", cq->id);
	}

	if ((le16_to_cpu(LOAD(cqe->sfp)) & 0x1) == cq->phase)
		return NULL;

	barrier();

	if (unlikely(++cq->head == cq->qsize)) {
		cq->head = 0;
		cq->phase ^= 0x1;
	}

	return cqe;
}

/**
 * nvme_cq_poll - spin on completion queue, get cqe and update head pointer
 * @cq: Completion queue
 *
 * Combine the effects of calling nvme_cq_spin(), nvme_cq_get_cqe() and
 * nvme_cq_update_head().
 *
 * Return: A completion queue entry.
 */
struct nvme_cqe *nvme_cq_poll(struct nvme_cq *cq);

#endif /* LIBVFN_NVME_QUEUE_H */
