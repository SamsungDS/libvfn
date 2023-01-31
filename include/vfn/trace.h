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

# define __trace_prefix(fmt) "T %s (%s:%d) " fmt

/**
 * trace_guard - conditionally open a tracing scope
 * @name: trace event identifier
 *
 * This function behaves as a conditional. That is, it should be used like
 * this:
 *
 * .. code-block:: c
 *
 *     trace_guard(TRACE_EVENT_IDENTIFIER) {
 *         trace_emit("something");
 *     }
 *
 * If debugging is disabled, or if the specific tracing event is statically
 * disabled, this will have zero overhead.
 */
# define trace_guard(name) \
	if (({ \
		bool cond = ((!TRACE_ ## name ## _DISABLED) && TRACE_ ## name ## _ACTIVE); \
		if (cond) \
			__trace_event = TRACE_ ## name; \
		cond; \
	}))

/**
 * trace_emit - emit a trace event message
 * @fmt: format string
 * @...: format string arguments
 *
 * Emit a trace point message. Must be used inside an opened trace point. See
 * trace_guard().
 */
# define trace_emit(fmt, ...) \
	fprintf(stderr, __trace_prefix(fmt), __trace_event, __FILE__, __LINE__, ##__VA_ARGS__)

#else
# define trace_guard(name) if (false)
# define trace_emit(fmt, ...)
#endif

/**
 * trace_event_set_active - Enable or disable a range of trace events
 * @prefix: trace event identifier prefix
 * @active: boolean (enable/disable)
 *
 * Enable/disable all trace points with the given @prefix.
 */
void trace_event_set_active(const char *prefix, bool active);

#endif /* LIBVFN_TRACE_H */
