// SPDX-License-Identifier: BSD-3-Clause

/*
 * Copyright 2020 Mellanox Technologies, Ltd
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * From DPDK; modified by the libvfn Authors.
 */

#include <errno.h>

#include <vfn/support/timer.h>

#include "ccan/time/time.h"

void __usleep(unsigned long us)
{
	struct timerel wait[2];
	int idx = 0;

	wait[0] = time_from_usec(us);

	while (nanosleep(&wait[idx].ts, &wait[1 - idx].ts) && errno == EINTR)
		idx = 1 - idx;
}
