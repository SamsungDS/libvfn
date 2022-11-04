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

#define __glue(x, y) x ## y
#define glue(x, y) __glue(x, y)

#define __LOAD(type, v) (*(const volatile type *)&(v))
#define __LOAD_PTR(type, ptr) (*(const volatile type)(ptr))

/**
 * LOAD - force volatile load
 * @v: variable to load
 *
 * Force a volatile load, detering the compiler from doing a range of
 * optimizations.
 *
 * Return: The value of the variable.
 */
#define LOAD(v) __LOAD(typeof(v), v)

/**
 * LOAD_PTR - force a volatile load
 * @ptr: pointer to variable to load
 *
 * Force a volatile load, detering the compiler from doing a range of
 * optimizations.
 *
 * Return: the value of the pointed to variable.
 */
#define LOAD_PTR(ptr) __LOAD_PTR(typeof(ptr), ptr)

#define __STORE(type, v, val) (*(volatile type *)&(v) = (val))
#define __STORE_PTR(type, ptr, val) (*(volatile type)(ptr) = (val))

/**
 * STORE - force volatile store
 * @v: variable to store into
 * @val: value to store into @v
 *
 * Force a volatile store, detering the compiler from doing a range of
 * optimizations.
 */
#define STORE(v, val) __STORE(typeof(v), v, val)

/**
 * STORE_PTR - force a volatile store
 * @ptr: pointer to variable to store into
 * @val: value to store into variable pointed to by @ptr
 *
 * Force a volatile store, detering the compiler from doing a range of
 * optimizations.
 */
#define STORE_PTR(ptr, val) __STORE_PTR(typeof(ptr), ptr, val)

#define likely(cond) __builtin_expect(!!(cond), 1)
#define unlikely(cond) __builtin_expect(!!(cond), 0)

#ifdef __CHECKER__
# ifndef __bitwise
#  define __bitwise	__attribute__((bitwise))
# endif
# define __force	__attribute__((force))
#else
# ifndef __bitwise
#  define __bitwise
# endif
# define __force
#endif

#define __static_assert(x) static_assert(x, #x)

#endif /* LIBVFN_SUPPORT_COMPILER_H */
