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

#ifndef LIBVFN_SUPPORT_MUTEX_H
#define LIBVFN_SUPPORT_MUTEX_H

/*
 * Autolockable mutex
 *
 * Define a __autolock() macro that will lock the given mutex as well as ensure
 * that it is unlocked when going out of scope. This is inspired by the
 * polymorphic locking functions in QEMU (include/qemu/lockable.h), but this
 * version only supports the pthread_mutex_t.
 */

static inline pthread_mutex_t *__mutex_auto_lock(pthread_mutex_t *mutex)
{
	pthread_mutex_lock(mutex);
	return mutex;
}

static inline void __mutex_auto_unlock(pthread_mutex_t *mutex)
{
	if (mutex)
		pthread_mutex_unlock(mutex);
}

DEFINE_AUTOPTR(pthread_mutex_t, __mutex_auto_unlock)

#define __autolock(x) \
	__autoptr(pthread_mutex_t) \
	glue(autolock, __COUNTER__) __attribute__((__unused__)) = __mutex_auto_lock(x)

#endif /* LIBVFN_SUPPORT_MUTEX_H */
