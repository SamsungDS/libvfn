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

/*
 * glib-style auto pointer
 *
 * The __autoptr() provides a general way of "cleaning up" when going out of
 * scope. Inspired by glib, but simplified a lot (at the expence of
 * flexibility).
 */

#define __AUTOPTR_CLEANUP(t) __autoptr_cleanup_##t
#define __AUTOPTR_T(t) __autoptr_##t

#define DEFINE_AUTOPTR(t, cleanup) \
	typedef t *__AUTOPTR_T(t); \
	\
	static inline void __AUTOPTR_CLEANUP(t) (t **p) \
	{ \
		if (*p) \
			(cleanup)(*p); \
	}

#define __autoptr(t) __attribute__((cleanup(__AUTOPTR_CLEANUP(t)))) __AUTOPTR_T(t)

#endif /* LIBVFN_SUPPORT_AUTOPTR_H */
