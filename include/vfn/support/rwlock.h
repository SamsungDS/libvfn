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

#ifndef LIBVFN_SUPPORT_RWLOCK_H
#define LIBVFN_SUPPORT_RWLOCK_H

/*
 * DOC: Autolockable read-write lock
 *
 * Define a __autolock() macro that will lock the given rwlock as well as ensure
 * that it is unlocked when going out of scope. This is inspired by the
 * polymorphic locking functions in QEMU (include/qemu/lockable.h), but this
 * version only supports the pthread_rwlock_t.
 */

static inline pthread_rwlock_t *__rwlock_auto_rdlock(pthread_rwlock_t *rwlock)
{
	pthread_rwlock_rdlock(rwlock);
	return rwlock;
}

static inline pthread_rwlock_t *__rwlock_auto_wrlock(pthread_rwlock_t *rwlock)
{
	pthread_rwlock_wrlock(rwlock);
	return rwlock;
}

static inline void __rwlock_auto_unlock(pthread_rwlock_t *rwlock)
{
	if (rwlock)
		pthread_rwlock_unlock(rwlock);
}

DEFINE_AUTOPTR(pthread_rwlock_t, __rwlock_auto_unlock)

/**
 * __autordlock - autolock rdlock
 * @x: pointer to pthread rdlock
 *
 * Lock the rdlock and unlock it automatically when going out of scope.
 */
#define __autordlock(x) \
	__autoptr(pthread_rwlock_t) \
	glue(autordlock, __COUNTER__) __attribute__((__unused__)) = __rwlock_auto_rdlock(x)

/**
 * __autowrlock - autolock wrlock
 * @x: pointer to pthread wrlock
 *
 * Lock the wrlock and unlock it automatically when going out of scope.
 */
#define __autowrlock(x) \
	__autoptr(pthread_rwlock_t) \
	glue(autowrlock, __COUNTER__) __attribute__((__unused__)) = __rwlock_auto_wrlock(x)

#endif /* LIBVFN_SUPPORT_RWLOCK_H */
