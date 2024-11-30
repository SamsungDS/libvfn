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

#ifndef LIBVFN_SUPPORT_MEM_H
#define LIBVFN_SUPPORT_MEM_H

extern size_t __VFN_PAGESIZE;
extern int __VFN_PAGESHIFT;

void backtrace_abort(void);

static inline void __do_autofree(void *mem)
{
	void **memp = (void **)mem;

	free(*memp);
}

#define __autofree __attribute__((cleanup(__do_autofree)))

static inline bool would_overflow(unsigned int n, size_t sz)
{
	return n > SIZE_MAX / sz;
}

static inline size_t __abort_on_overflow(unsigned int n, size_t sz)
{
	if (would_overflow(n, sz))
		backtrace_abort();

	return n * sz;
}

#ifdef __APPLE__
# include "vfn/support/platform/driverkit/mem.h"
#else
# include "vfn/support/platform/posix/mem.h"
#endif

static inline void *mallocn(unsigned int n, size_t sz)
{
	if (would_overflow(n, sz)) {
		fprintf(stderr, "allocation of %d * %zu bytes would overflow\n", n, sz);

		backtrace_abort();
	}

	return xmalloc(n * sz);
}

static inline void *zmallocn(unsigned int n, size_t sz)
{
	if (would_overflow(n, sz)) {
		fprintf(stderr, "allocation of %d * %zu bytes would overflow\n", n, sz);

		backtrace_abort();
	}

	return zmalloc(n * sz);
}

#define _new_t(t, n, f) \
	((t *) f(n, sizeof(t)))

#define new_t(t, n) _new_t(t, n, mallocn)
#define znew_t(t, n) _new_t(t, n, zmallocn)

ssize_t pgmap(void **mem, size_t sz);
ssize_t pgmapn(void **mem, unsigned int n, size_t sz);

static inline void pgunmap(void *mem, size_t len)
{
	if (munmap(mem, len))
		backtrace_abort();
}

#endif /* LIBVFN_SUPPORT_MEM_H */
