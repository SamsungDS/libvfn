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

#ifndef LIBVFN_SUPPORT_COMPILER_H
#define LIBVFN_SUPPORT_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

#define barrier() asm volatile("" ::: "memory")

#if defined(__aarch64__)
# define rmb() asm volatile("dsb ld" ::: "memory")
# define wmb() asm volatile("dsb st" ::: "memory")
# define mb()  asm volatile("dsb sy" ::: "memory")
#elif defined(__x86_64__)
# define rmb() asm volatile("lfence" ::: "memory")
# define wmb() asm volatile("sfence" ::: "memory")
# define mb()  asm volatile("mfence" ::: "memory")
#else
# error unsupported architecture
#endif

/**
 * LOAD - force volatile load
 * @v: variable to load
 *
 * Force a volatile load, detering the compiler from doing a range of optimizations.
 *
 * Return: The value of the variable.
 */
#define LOAD(v) (*(const volatile typeof(v) *)&(v))

/**
 * STORE - force volatile store
 * @v: variable to store into
 * @val: value to store into @v
 *
 * Force a volatile store, detering the compiler from doing a range of optimizations.
 */
#define STORE(v, val) (*(volatile typeof(v) *)&(v) = (val))

#define likely(cond) __builtin_expect(!!(cond), 1)
#define unlikely(cond) __builtin_expect(!!(cond), 0)

#ifdef __CHECKER__
# define __force	__attribute__((force))
#else
# define __force
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_SUPPORT_COMPILER_H */
