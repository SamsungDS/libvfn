/* SPDX-License-Identifier: LGPL-2.1-or-later OR MIT */

#ifndef LIBVFN_SUPPORT_ARCH_S390X_TOD_H
#define LIBVFN_SUPPORT_ARCH_S390X_TOD_H

#include <stdint.h>
#include <time.h>

/*
 * On s390x, use CLOCK_MONOTONIC_RAW in nanoseconds as the timestamp source
 * rather than reading the TOD clock via inline assembly. Ticks are only used
 * for timing/timeout purposes, so a portable implementation is preferable to
 * architecture-specific assembly here.
 */
static inline uint64_t get_ticks_arch(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
		return 0;

	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

#endif /* LIBVFN_SUPPORT_ARCH_S390X_TOD_H */
