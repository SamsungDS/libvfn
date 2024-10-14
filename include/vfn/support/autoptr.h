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

#ifndef LIBVFN_SUPPORT_AUTOPTR_H
#define LIBVFN_SUPPORT_AUTOPTR_H

/**
 * DOC: glib-style automatic cleanup.
 *
 * The __autoptr() and __autovar_s() provides a general way of "cleaning up"
 * when going out of scope. Inspired by glib, but simplified a lot (at the
 * expence of flexibility).
 */

#define __AUTOPTR_CLEANUP(t) __autoptr_cleanup_##t
#define __AUTOPTR_T(t) __autoptr_##t

/**
 * DEFINE_AUTOPTR - Defines the appropriate cleanup function for a pointer type
 * @t: type name
 * @cleanup: function to be called to clean up type
 *
 * Defines a function ``__autoptr_cleanup_##t`` that will call @cleanup when
 * invoked.
 */
#define DEFINE_AUTOPTR(t, cleanup) \
	typedef t *__AUTOPTR_T(t); \
	\
	static inline void __AUTOPTR_CLEANUP(t) (t **p) \
	{ \
		if (*p) \
			(cleanup)(*p); \
	}

/**
 * __autoptr - Helper to declare a pointer variable with automatic cleanup
 * @t: type name
 *
 * Declares a pointer that is cleaned up when the variable goes out of scope.
 * How to clean up the type must have been previously declared using
 * DEFINE_AUTOPTR().
 */
#define __autoptr(t) __attribute__((cleanup(__AUTOPTR_CLEANUP(t)))) __AUTOPTR_T(t)

#define __AUTOVAR_STRUCT_CLEANUP(t) __autovar_struct_cleanup_##t
#define __AUTOVAR_STRUCT_T(t) __autovar_struct_##t

/**
 * DEFINE_AUTOVAR_STRUCT - Defines the appropriate cleanup function for a struct
 *                         type
 * @t: type name
 * @cleanup: function to be called to clean up type
 *
 * Defines a function ``__autovar_struct_cleanup_##t`` that will call @cleanup
 * when invoked.
 */
#define DEFINE_AUTOVAR_STRUCT(t, cleanup) \
	typedef struct t __AUTOVAR_STRUCT_T(t); \
	\
	static inline void __AUTOVAR_STRUCT_CLEANUP(t) (struct t *p) \
	{ \
		(cleanup)(p); \
	}

/**
 * __autovar_s - Helper to declare a struct variable with automatic cleanup
 * @t: type name
 *
 * Declares a struct-type variable that is cleaned up when the variable goes out
 * of scope.  How to clean up the type must have been previously declared using
 * DEFINE_AUTOPTR().
 */
#define __autovar_s(t) __attribute__((cleanup(__AUTOVAR_STRUCT_CLEANUP(t)))) __AUTOVAR_STRUCT_T(t)

#endif /* LIBVFN_SUPPORT_AUTOPTR_H */
