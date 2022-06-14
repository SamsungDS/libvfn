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

#ifndef LIBVFN_SUPPORT_LOG_H
#define LIBVFN_SUPPORT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

extern struct log_state {
	unsigned int v;
} __log_state;

enum __log_level {
	LOG_ERROR,
	LOG_INFO,
	LOG_DEBUG,
};

/**
 * __logv - Determine if log verbosity is as given
 * @v: target verbositry level
 *
 * Return: ``true`` if verbosity is at least @v, ``false`` otherwise.
 */
static inline bool __logv(unsigned int v)
{
	if (__atomic_load_n(&__log_state.v, __ATOMIC_ACQUIRE) >= v)
		return true;

	return false;
}

static inline void __logv_set(unsigned int v)
{
	__atomic_store_n(&__log_state.v, &v, __ATOMIC_RELEASE);
}

/**
 * __log - Log a message at a given verbosity level
 * @v: verbosity level
 * @fmt: format string
 * @...: format string arguments
 *
 * Log a formatted message to stderr if current verbosity level is at least @v.
 */
static inline void __attribute__((format(printf, 2, 3))) __log(unsigned int v, char const *fmt, ...)
{
	va_list va;

	if (!__logv(v))
		return;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

#define __debug(fmt, ...) \
	__log(LOG_DEBUG, "%s (%s:%d): "fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_SUPPORT_LOG_H */
