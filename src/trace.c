// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ccan/array_size/array_size.h"

#include "vfn/trace.h"
#include "vfn/support/ticks.h"
#include "trace.h"

__thread const char *__trace_event;

static void __attribute__((constructor)) init_trace_events(void)
{
	char *tok, *buf, *orig = getenv("VFN_TRACE_EVENTS");

	if (!orig)
		return;

	buf = strdup(orig);
	assert(buf);

	tok = strtok(buf, ",");

	while (tok) {
		bool activate = true;

		if (*tok == '-') {
			activate = false;
			tok++;
		} else if (*tok == '+') {
			tok++;
		}

		trace_set_active(tok, activate);

		tok = strtok(NULL, ",");
	}

	free(buf);
}

void trace_set_active(const char *prefix, bool active)
{
	size_t len;

	assert(prefix);

	len = strlen(prefix);

	for (int i = 0; i < TRACE_NUM_EVENTS; i++) {
		struct trace_event *event = &trace_events[i];

		if (strncmp(prefix, event->name, len) == 0)
			*(event->active) = active;
	}
}

bool __trace_ratelimited(struct trace_ratelimit_state *rs, uint64_t tag, const char *event)
{
	if (!rs->interval)
		return false;

	if (!rs->begin || rs->tag != tag || get_ticks() > rs->end) {
		if (rs->skipped)
			fprintf(stderr, "T %s (%d events skipped)\n", event, rs->skipped);

		rs->begin = get_ticks();
		rs->end = rs->begin + rs->interval * __vfn_ticks_freq;
		rs->skipped = 0;
		rs->tag = tag;

		return false;
	}

	rs->skipped++;

	return true;
}
