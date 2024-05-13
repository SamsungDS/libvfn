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

#include <os/log.h>
#include <DriverKit/IOLib.h>
#include <vfn/support/atomic.h>

extern struct log_state {
	unsigned int v;
} __log_state;

enum __log_level {
	LOG_ERROR,
	LOG_INFO,
	LOG_DEBUG,
};

/**
 * logv - Determine if log verbosity is as given
 * @v: target verbositry level
 *
 * Return: ``true`` if verbosity is at least @v, ``false`` otherwise.
 */
static inline bool logv(unsigned int v)
{
	if (atomic_load_acquire(&__log_state.v) >= v)
		return true;

	return false;
}

/**
 * logv_set - Set log verbosity level
 * @v: verbosity level
 */
static inline void logv_set(unsigned int v)
{
	atomic_store_release(&__log_state.v, v);
}

#ifdef DEBUG
#define log_error(fmt, ...) os_log(OS_LOG_DEFAULT, "MacVFN ERROR: " fmt "\n", ##__VA_ARGS__)
#define log_info(fmt, ...) os_log(OS_LOG_DEFAULT, "MacVFN INFO: " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) os_log(OS_LOG_DEFAULT, "MacVFN DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define log_error(fmt, ...)
#define log_info(fmt, ...)
#define log_debug(fmt, ...)
#endif /* DEBUG */

#endif /* LIBVFN_SUPPORT_LOG_H */
