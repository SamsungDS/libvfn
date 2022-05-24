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

#ifndef LIBVFN_TRACE_H
#define LIBVFN_TRACE_H

#include <vfn/trace/events.h>

#ifdef DEBUG
static __thread const char *__trace_event;

/**
 * __trace - conditionally open a tracing point
 * @name: trace point identifier
 *
 * This functions as an alias of ``if``. That is, it should be used like this:
 *
 * .. code-block:: c
 *
 *     __trace(TRACE_POINT_IDENTIFIER) {
 *         __emit("something");
 *     }
 *
 * If tracing is disabled, or if the specific trace point identifier is
 * disabled, this will have zero overhead.
 */
# define __trace(name) \
	if (({ \
		bool cond = ((!TRACE_ ## name ## _DISABLED) && TRACE_ ## name ## _ACTIVE); \
		if (cond) \
			__trace_event = TRACE_ ## name; \
		cond; \
	}))

/**
 * __emit - emit a trace point message
 * @fmt: format string
 * @...: format string arguments
 *
 * Emit a trace point message. Must be used inside an opened trace point. See
 * __trace().
 */
# define __emit(fmt, ...) \
	fprintf(stderr, "T %s "fmt, __trace_event, ##__VA_ARGS__)

#else
# define __trace(name) if (false)
# define __emit(name, ...)
#endif

/**
 * trace_set_active - Enable or disable a range of trace points
 * @prefix: trace point identifier prefix
 * @active: boolean (enable/disable)
 *
 * Enable/disable all trace points with the given @prefix.
 */
void trace_set_active(const char *prefix, bool active);

#endif /* LIBVFN_TRACE_H */
