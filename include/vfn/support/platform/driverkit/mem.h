/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2024 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#ifndef LIBVFN_SUPPORT_PLATFORM_DRIVERKIT_MEM_H
#define LIBVFN_SUPPORT_PLATFORM_DRIVERKIT_MEM_H

#include <DriverKit/IOLib.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>

static inline void *xmalloc(size_t sz)
{
	void *mem;

	if (unlikely(!sz))
		return NULL;

	sz += sizeof(uint64_t);

	mem = IOMalloc(sz);
	if (unlikely(!mem))
		backtrace_abort();

	/* prepend length of allocationl (we need it on free) */
	*(uint64_t *)mem = sz;

	return (void *)(((char *)mem) + sizeof(uint64_t));
}

static inline void *zmalloc(size_t sz)
{
	void *mem;

	if (unlikely(!sz))
		return NULL;

	sz += sizeof(uint64_t);

	mem = IOMallocZero(sz);
	if (unlikely(!mem))
		backtrace_abort();

	/* prepend length of allocationl (we need it on free) */
	*(uint64_t *)mem = sz;

	return (void *)(((char *)mem) + sizeof(uint64_t));
}

static inline void free(void *ptr)
{
	void *real_ptr = (void *)(((uint8_t *)ptr) - sizeof(uint64_t));
	uint64_t len = *(uint64_t *)real_ptr;

	IOFree(real_ptr, len);
}

#endif /* LIBVFN_SUPPORT_PLATFORM_DRIVERKIT_MEM_H */
