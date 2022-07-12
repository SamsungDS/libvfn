/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation.
 * Copyright(c) 2013 6WIND S.A.
 */

#ifndef LIBVFN_SUPPORT_ARCH_X86_64_RDTSC_H
#define LIBVFN_SUPPORT_ARCH_X86_64_RDTSC_H

static inline uint64_t __x86_64_rdtsc(void)
{
	union {
		struct { uint32_t lo, hi; };
		uint64_t a;
	} tsc;

	asm volatile("rdtsc" : "=a" (tsc.lo), "=d" (tsc.hi));

	return tsc.a;
}

static inline uint64_t get_ticks_arch(void)
{
	return __x86_64_rdtsc();
}

#endif /* LIBVFN_SUPPORT_ARCH_X86_64_RDTSC_H */
