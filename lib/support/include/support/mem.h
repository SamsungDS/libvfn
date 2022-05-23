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

#ifndef LIBSUPPORT_MEM_H
#define LIBSUPPORT_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

extern long PAGESIZE;
extern int PAGESHIFT;

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

#define likely(cond) __builtin_expect(!!(cond), 1)
#define unlikely(cond) __builtin_expect(!!(cond), 0)

void backtrace_abort(void);

static inline void __do_autofree(void *mem)
{
	void **memp = (void **)mem;

	free(*memp);
}

#define __autofree	__attribute__((cleanup(__do_autofree)))

static inline bool would_overflow(unsigned int n, size_t sz)
{
	return n > SIZE_MAX / sz;
}

static inline void *xmalloc(size_t sz)
{
	void *mem;

	if (unlikely(!sz))
		return NULL;

	mem = malloc(sz);
	if (unlikely(!mem))
		backtrace_abort();

	return mem;
}

static inline void *zmalloc(size_t sz)
{
	void *mem;

	if (unlikely(!sz))
		return NULL;

	mem = calloc(1, sz);
	if (unlikely(!mem))
		backtrace_abort();

	return mem;
}

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

ssize_t pgmap(void **mem, size_t sz);
ssize_t pgmapn(void **mem, unsigned int n, size_t sz);

static inline void pgunmap(void *mem, size_t len)
{
	if (munmap(mem, len))
		backtrace_abort();
}

#ifdef __cplusplus
}
#endif

#endif /* LIBSUPPORT_MEM_H */
