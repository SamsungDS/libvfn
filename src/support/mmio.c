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

#define log_fmt(fmt) "support/mmio: " fmt

#include <errno.h>
#include <string.h>

#include "vfn/support.h"

/*
 * Emulated MMIO for regions that cannot be mmap()ed. On some platforms
 * (notably s390x/zPCI) a VFIO BAR region is not mmap()able and is reachable
 * only through pread()/pwrite() on the device fd. vfn_mmio_synth_map() hands
 * out synthetic addresses inside a reserved PROT_NONE window; the mmio.h
 * accessors route accesses to such addresses through the fd.
 *
 * The registry is shared by all users and keyed by an opaque @owner (the
 * device), so a device's mappings can be drained when it is torn down. The
 * slot records the fd and file offset needed to service an access.
 */
#define VFN_MMIO_SYNTH_SLOTS	16
#define VFN_MMIO_SYNTH_STRIDE	(4UL * 1024 * 1024)

struct vfn_mmio_synth_region {
	bool used;
	void *owner;
	int fd;
	uint64_t file_off;
	size_t size;
	uintptr_t vbase;
};

static struct vfn_mmio_synth_region vfn_mmio_synth[VFN_MMIO_SYNTH_SLOTS];
static uintptr_t vfn_mmio_synth_base;
static size_t vfn_mmio_synth_span;

/*
 * Serializes slot allocation and release. The decode path
 * (vfn_mmio_synth_read/write/is, called from the mmio.h accessors) stays
 * lock-free: a slot is immutable once ->used is published with release
 * semantics, and a cleared slot decodes to "not synthetic" -- a racy access
 * after unmap then faults on the PROT_NONE window rather than operating on
 * a reused slot.
 */
static pthread_mutex_t vfn_mmio_synth_lock;

/*
 * Initialized at load: a static pthread initializer upsets sparse (see
 * iommu/vfio.c).
 */
static void __attribute__((constructor)) init_vfn_mmio_synth_lock(void)
{
	pthread_mutex_init(&vfn_mmio_synth_lock, NULL);
}

/* in-arena, regardless of slot liveness */
static bool vfn_mmio_synth_arena_contains(const void *addr)
{
	uintptr_t a = (uintptr_t)addr;

	return vfn_mmio_synth_span && a >= vfn_mmio_synth_base &&
	       a - vfn_mmio_synth_base < vfn_mmio_synth_span;
}

static struct vfn_mmio_synth_region *vfn_mmio_synth_lookup(const void *addr)
{
	uintptr_t a = (uintptr_t)addr;
	struct vfn_mmio_synth_region *r;

	if (!vfn_mmio_synth_span || a < vfn_mmio_synth_base ||
	    a - vfn_mmio_synth_base >= vfn_mmio_synth_span)
		return NULL;

	r = &vfn_mmio_synth[(a - vfn_mmio_synth_base) / VFN_MMIO_SYNTH_STRIDE];
	if (!__atomic_load_n(&r->used, __ATOMIC_ACQUIRE))
		return NULL;

	return r;
}

void *vfn_mmio_synth_map(void *owner, int fd, uint64_t file_off, size_t size)
{
	struct vfn_mmio_synth_region *r;
	void *ret = NULL;

	if (size > VFN_MMIO_SYNTH_STRIDE) {
		errno = E2BIG;
		return NULL;
	}

	pthread_mutex_lock(&vfn_mmio_synth_lock);

	if (!vfn_mmio_synth_span) {
		size_t span = VFN_MMIO_SYNTH_SLOTS * VFN_MMIO_SYNTH_STRIDE;
		void *win;

		win = mmap(NULL, span, PROT_NONE,
			   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
		if (win == MAP_FAILED)
			goto out;

		vfn_mmio_synth_base = (uintptr_t)win;
		vfn_mmio_synth_span = span;
	}

	for (size_t s = 0; s < VFN_MMIO_SYNTH_SLOTS; s++) {
		r = &vfn_mmio_synth[s];

		if (__atomic_load_n(&r->used, __ATOMIC_ACQUIRE))
			continue;

		r->owner = owner;
		r->fd = fd;
		r->file_off = file_off;
		r->size = size;
		r->vbase = vfn_mmio_synth_base +
			   (uintptr_t)s * VFN_MMIO_SYNTH_STRIDE;

		/* publish last: accessors only trust ->used */
		__atomic_store_n(&r->used, true, __ATOMIC_RELEASE);

		ret = (void *)r->vbase;
		break;
	}

	if (!ret)
		errno = ENOSPC;
out:
	pthread_mutex_unlock(&vfn_mmio_synth_lock);
	return ret;
}

bool vfn_mmio_synth_unmap(void *owner, void *addr)
{
	struct vfn_mmio_synth_region *r;

	if (!vfn_mmio_synth_arena_contains(addr))
		return false;

	pthread_mutex_lock(&vfn_mmio_synth_lock);

	for (size_t s = 0; s < VFN_MMIO_SYNTH_SLOTS; s++) {
		r = &vfn_mmio_synth[s];

		if (!__atomic_load_n(&r->used, __ATOMIC_ACQUIRE) ||
		    (void *)r->vbase != addr)
			continue;

		if (r->owner != owner) {
			log_debug("synthetic bar %p not owned by this device (stale or double unmap?)\n",
				  addr);
		} else {
			r->owner = NULL;
			__atomic_store_n(&r->used, false, __ATOMIC_RELEASE);
		}

		pthread_mutex_unlock(&vfn_mmio_synth_lock);
		return true;
	}

	pthread_mutex_unlock(&vfn_mmio_synth_lock);

	log_debug("unmapping stale synthetic bar %p\n", addr);
	return true;
}

void vfn_mmio_synth_drain(void *owner)
{
	struct vfn_mmio_synth_region *r;

	pthread_mutex_lock(&vfn_mmio_synth_lock);

	for (size_t s = 0; s < VFN_MMIO_SYNTH_SLOTS; s++) {
		r = &vfn_mmio_synth[s];

		if (!__atomic_load_n(&r->used, __ATOMIC_ACQUIRE) ||
		    r->owner != owner)
			continue;

		log_debug("releasing leaked synthetic bar mapping %p\n",
			  (void *)r->vbase);
		r->owner = NULL;
		__atomic_store_n(&r->used, false, __ATOMIC_RELEASE);
	}

	pthread_mutex_unlock(&vfn_mmio_synth_lock);
}

bool vfn_mmio_synth_is(const void *addr)
{
	return !!vfn_mmio_synth_lookup(addr);
}

enum vfn_mmio_synth_result vfn_mmio_synth_read(void *addr, void *buf, size_t len)
{
	struct vfn_mmio_synth_region *r = vfn_mmio_synth_lookup(addr);
	size_t off;
	ssize_t n;

	if (!r)
		return VFN_MMIO_NOT_SYNTH;

	/*
	 * The window is larger than the region, so the address decode is not
	 * enough: bound against the mapped size. This must not report "not
	 * synthetic" or the caller would dereference the PROT_NONE window.
	 */
	off = (uintptr_t)addr - r->vbase;
	if (off > r->size || len > r->size - off) {
		log_debug("synthetic bar read out of bounds (off %#zx len %zu, size %#zx)\n",
			  off, len, r->size);
		return VFN_MMIO_SYNTH_ERROR;
	}

	do {
		n = pread(r->fd, buf, len, r->file_off + off);
	} while (n < 0 && errno == EINTR);

	if (n != (ssize_t)len) {
		log_debug("failed to read synthetic bar region (off %#zx len %zu): %s\n",
			  off, len, n < 0 ? strerror(errno) : "short read");
		return VFN_MMIO_SYNTH_ERROR;
	}

	return VFN_MMIO_SYNTH_DONE;
}

enum vfn_mmio_synth_result vfn_mmio_synth_write(void *addr, const void *buf, size_t len)
{
	struct vfn_mmio_synth_region *r = vfn_mmio_synth_lookup(addr);
	size_t off;
	ssize_t n;

	if (!r)
		return VFN_MMIO_NOT_SYNTH;

	off = (uintptr_t)addr - r->vbase;
	if (off > r->size || len > r->size - off) {
		log_debug("synthetic bar write out of bounds (off %#zx len %zu, size %#zx)\n",
			  off, len, r->size);
		return VFN_MMIO_SYNTH_ERROR;
	}

	do {
		n = pwrite(r->fd, buf, len, r->file_off + off);
	} while (n < 0 && errno == EINTR);

	if (n != (ssize_t)len) {
		log_debug("failed to write synthetic bar region (off %#zx len %zu): %s\n",
			  off, len, n < 0 ? strerror(errno) : "short write");
		return VFN_MMIO_SYNTH_ERROR;
	}

	return VFN_MMIO_SYNTH_DONE;
}
