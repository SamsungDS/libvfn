// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ccan/array_size/array_size.h"

#include "trace.h"

static void __attribute__((constructor)) init_trace_events(void)
{
	char *tok, *buf, *orig = getenv("TRACE_EVENTS");

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
	const size_t len = strlen(prefix);

	for (unsigned int i = 0; i < TRACE_NUM_EVENTS; i++) {
		struct trace_event *event = &trace_events[i];

		if (!prefix || strncmp(prefix, event->name, len) == 0)
			*(event->active) = active;
	}
}
