// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * This file is part of libsupport.
 *
 * Copyright (C) 2022 Klaus Jensen <its@irrelevant.dk>
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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "support/log.h"

struct log_state __log_state;

static void __attribute__((constructor)) init_log_level(void)
{
	char *buf = getenv("LOGV");
	char *endptr;

	if (!buf)
		goto set_default;

	__log_state.v = strtoul(buf, &endptr, 0);
	if (endptr == buf)
		goto set_default;

	return;

set_default:
#ifdef DEBUG
	__log_state.v = LOG_DEBUG;
#else
	__log_state.v = 0;
#endif
}
