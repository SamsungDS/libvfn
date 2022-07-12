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

#ifndef LIBVFN_SUPPORT_TICKS_H
#define LIBVFN_SUPPORT_TICKS_H

#if defined(__x86_64__)
# include <vfn/support/arch/x86_64/rdtsc.h>
#elif defined(__aarch64__)
# include <vfn/support/arch/arm64/cnt.h>
#else
# error unsupported architecture
#endif

extern uint64_t __vfn_ticks_freq;

static inline uint64_t get_ticks(void)
{
	return get_ticks_arch();
}

uint64_t get_ticks_freq_arch(void);

#endif /* LIBVFN_SUPPORT_TICKS_H */
