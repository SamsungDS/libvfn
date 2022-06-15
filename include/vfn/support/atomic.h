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

#ifndef LIBVFN_SUPPORT_ATOMIC_H
#define LIBVFN_SUPPORT_ATOMIC_H

/* sparse does not know about these gcc builtins */
#ifdef __CHECKER__
#define __atomic_load_n(ptr, memorder) (*(ptr))
#define __atomic_store_n(ptr, val, memorder) ({ *(ptr) = val; })
#define __atomic_compare_exchange_n(ptr, expected, desired, weak, ok_memorder, fail_memorder) \
	({ *ptr = desired; })
#endif

/**
 * atomic_load_acquire - Syntactic suger for __atomic_load_n
 * @ptr: Pointer to value
 *
 * Atomically load the value of @ptr with acquire semantics.
 *
 * Return: The contents of ``*ptr``.
 */
#define atomic_load_acquire(ptr) \
	__atomic_load_n(ptr, __ATOMIC_ACQUIRE)

#define atomic_store_release(ptr, val) \
	__atomic_store_n(ptr, val, __ATOMIC_RELEASE)

/**
 * atomic_cmpxchg - Syntactic suger for __atomic_compare_exchange_n
 * @ptr: Pointer to value to compare
 * @expected: Expected value
 * @desired: Value that should be set
 *
 * Perform an atomic compare-and-exchange on @ptr.
 */
#define atomic_cmpxchg(ptr, expected, desired) \
	__atomic_compare_exchange_n(ptr, &expected, desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)

#endif /* LIBVFN_SUPPORT_ATOMIC_H */
