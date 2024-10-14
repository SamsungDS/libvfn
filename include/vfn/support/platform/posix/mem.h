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

#ifndef LIBVFN_SUPPORT_PLATFORM_POSIX_MEM_H
#define LIBVFN_SUPPORT_PLATFORM_POSIX_MEM_H

/**
 * xmalloc - version of malloc that cannot fail
 * @sz: number of bytes to allocate
 *
 * Call malloc, but only return NULL when @sz is zero. Otherwise, abort.
 *
 * Return: pointer to allocated memory
 */
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

static inline void *reallocn(void *mem, unsigned int n, size_t sz)
{
	if (would_overflow(n, sz)) {
		fprintf(stderr, "allocation of %d * %zu bytes would overflow\n", n, sz);

		backtrace_abort();
	}

	return realloc(mem, n * sz);
}

#endif /* LIBVFN_SUPPORT_PLATFORM_POSIX_MEM_H */
