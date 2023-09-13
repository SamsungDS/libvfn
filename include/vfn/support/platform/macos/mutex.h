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
 * version only supports the IOLock.
 */

static inline IOLock *__mutex_auto_lock(IOLock *mutex)
{
	IOLockLock(mutex);
	return mutex;
}

static inline void __mutex_auto_unlock(IOLock *mutex)
{
	if (mutex)
		IOLockUnlock(mutex);
}

DEFINE_AUTOPTR(IOLock, __mutex_auto_unlock)

#define __autolock(x) \
	__autoptr(IOLock) \
	glue(autolock, __COUNTER__) __attribute__((__unused__)) = __mutex_auto_lock(*x)

#endif /* LIBVFN_SUPPORT_MUTEX_H */
