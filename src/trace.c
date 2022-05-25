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
#include <string.h>

#include "ccan/array_size/array_size.h"

#include "trace.h"

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

	for (unsigned int i = 0; i < TRACE_NUM_EVENTS; i++) {
		struct trace_event *event = &trace_events[i];

		if (strncmp(prefix, event->name, len) == 0)
			*(event->active) = active;
	}
}
