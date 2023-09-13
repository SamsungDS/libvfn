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

#ifdef __APPLE__
#include <vfn/support/platform/macos/log.h>
#else
#include <vfn/support/platform/linux/log.h>
#endif

#define log_fatal(fmt, ...) \
	do { \
		log_error(fmt, ##__VA_ARGS__); \
		abort(); \
	} while (0)

#define log_fatal_if(expr, fmt, ...) \
	do { \
		if (expr) \
			log_fatal(fmt, ##__VA_ARGS__); \
	} while (0)
