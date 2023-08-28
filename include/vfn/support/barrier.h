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

#ifndef LIBVFN_SUPPORT_BARRIER_H
#define LIBVFN_SUPPORT_BARRIER_H

#define barrier() asm volatile("" ::: "memory")

#if defined(__aarch64__)
# define rmb()		asm volatile("dsb ld" ::: "memory")
# define wmb()		asm volatile("dsb st" ::: "memory")
# define mb()		asm volatile("dsb sy" ::: "memory")
# define dma_rmb()	asm volatile("dmb oshld" ::: "memory")
#elif defined(__x86_64__)
# define rmb()		asm volatile("lfence" ::: "memory")
# define wmb()		asm volatile("sfence" ::: "memory")
# define mb()		asm volatile("mfence" ::: "memory")
# define dma_rmb()	barrier()
#else
# error unsupported architecture
#endif

#endif /* LIBVFN_SUPPORT_BARRIER_H */
