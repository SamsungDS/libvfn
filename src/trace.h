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

struct trace_event {
	const char *name;
	bool *active;
};

extern struct trace_event trace_events[];
extern unsigned int TRACE_NUM_EVENTS;

void trace_set_active(const char *prefix, bool active);
