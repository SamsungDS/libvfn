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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <vfn/support/atomic.h>
#include <vfn/support/log.h>

struct log_state __log_state;

static void __attribute__((constructor)) init_log_level(void)
{
	char *buf = getenv("LOGV");
	char *endptr;
	long v;

	if (!buf)
		goto set_default;

	v = strtol(buf, &endptr, 0);
	if (endptr == buf)
		goto set_default;

	__log_state.v = (int)v;

	return;

set_default:
#ifdef DEBUG
	__log_state.v = LOG_DEBUG;
#else
	__log_state.v = 0;
#endif
}
