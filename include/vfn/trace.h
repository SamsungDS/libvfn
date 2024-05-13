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

#ifndef __APPLE__
#include <vfn/trace/events.h>
#endif

#if defined(DEBUG) && !defined(__APPLE__)
extern __thread const char *__trace_event;

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

/**
 * trace_emitrl - emit a trace event message (ratelimited)
 * @interval: ratelimiting interval in seconds
 * @tag: identifies what is subject to ratelimiting
 * @fmt: format string
 * @...: format string arguments
 *
 * Emit a trace point message while being subject to ratelimiting. The @tag
 * identifies what is subject to ratelimiting (e.g. a pointer or fixed tag).
 *
 * See trace_emit().
 */
# define trace_emitrl(interval, tag, fmt, ...) \
	({ \
		static __thread struct trace_ratelimit_state _rs = { \
			interval, 0, 0, 0, 0, \
		}; \
		\
		if (!__trace_ratelimited(&_rs, tag, __trace_event)) \
			trace_emit(fmt, ##__VA_ARGS__); \
	})

#else
# define trace_guard(name) if (false)
# define trace_emit(fmt, ...) ((void)0)
# define trace_emitrl(interval, subject, fmt, ...) ((void)0)
#endif

struct trace_ratelimit_state {
	int interval, skipped;
	uint64_t tag;
	uint64_t begin, end;
};

bool __trace_ratelimited(struct trace_ratelimit_state *rs, uint64_t tag, const char *event);

/**
 * trace_event_set_active - Enable or disable a range of trace events
 * @prefix: trace event identifier prefix
 * @active: boolean (enable/disable)
 *
 * Enable/disable all trace points with the given @prefix.
 */
void trace_event_set_active(const char *prefix, bool active);

#endif /* LIBVFN_TRACE_H */
