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

#ifndef LIBVFN_SUPPORT_TIMER_H
#define LIBVFN_SUPPORT_TIMER_H

#define NS_PER_SEC 1E9
#define US_PER_SEC 1E6
#define MS_PER_SEC 1E3

void __usleep(unsigned long us);

#endif /* LIBVFN_SUPPORT_TIMER_H */
