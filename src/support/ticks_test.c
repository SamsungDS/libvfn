// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <stdint.h>
#include <unistd.h>

#include "vfn/support/ticks.h"

#include "ccan/tap/tap.h"

int main(int argc UNUSED, char *argv[] UNUSED)
{
	uint64_t tsc_old, tsc_new;

	plan_no_plan();

	tsc_old = get_ticks();
	sleep(1);
	tsc_new = get_ticks();

	ok1(tsc_new > tsc_old);

	diag("TSC OLD:  %ld\n", tsc_old);
	diag("TSC NEW:  %ld\n", tsc_new);
	diag("TSC DIFF: %ld\n", tsc_new - tsc_old);
	diag("TSC HZ:   %ld\n", __vfn_ticks_freq);
	diag("%.10f\n", (double)(tsc_new - tsc_old) / __vfn_ticks_freq);

	return exit_status();
}
