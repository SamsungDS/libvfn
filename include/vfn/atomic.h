/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef LIBVFN_ATOMIC_H
#define LIBVFN_ATOMIC_H

#ifdef __cplusplus
extern "C" {
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

/* sparse does not know about these gcc builtins */
#ifdef __CHECKER__
#define __atomic_load_n(ptr, memorder) (*(ptr))
#define __atomic_compare_exchange_n(ptr, expected, desired, weak, ok_memorder, fail_memorder) \
	({ *ptr = desired; })
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_ATOMIC_H */
