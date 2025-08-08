// SPDX-License-Identifier: BSD-3-Clause

/*
 * Copyright 2020 Mellanox Technologies, Ltd
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * From DPDK; modified by the libvfn Authors.
 */

#define log_fmt(fmt) "support/ticks: " fmt

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>

#include "vfn/support/align.h"
#include "vfn/support/atomic.h"
#include "vfn/support/log.h"
#include "vfn/support/ticks.h"
#include "vfn/support/timer.h"

#include "ccan/time/time.h"

#define TICKS_PER_10MHZ 10000000ULL

uint64_t __vfn_ticks_freq;

static uint64_t read_cpu_freq_from_sys(void)
{
	uint64_t hz = 0;
	char buf[256];
	double mhz;
	FILE *f;
	int cpu;

	cpu = sched_getcpu();
	if (cpu < 0)
		cpu = 0;

	sprintf(buf, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);
	f = fopen(buf, "r");
	if (f) {
		if (fgets(buf, sizeof(buf), f))
			/* Convert KHz -> Hz */
			hz = strtoull(buf, NULL, 10) * 1000;
		fclose(f);
		if (hz)
			return ROUND(hz, TICKS_PER_10MHZ);
	}

	/* Second chance from procfs */
	f = fopen("/proc/cpuinfo", "r");
	if (f) {
		int current_cpu = 0;
		int proc_cpu;

		while (fgets(buf, sizeof(buf), f)) {
			if (sscanf(buf, "processor : %d", &proc_cpu) == 1)
				current_cpu = proc_cpu;
			else if (current_cpu == cpu) {
				if (sscanf(buf, "cpu MHz : %lf", &mhz) == 1) {
					hz = (uint64_t)(mhz * 1000000);
					break;
				}
			}
		}
		fclose(f);
		if (hz)
			return ROUND(hz, TICKS_PER_10MHZ);
	}

	return 0;
}

static uint64_t measure_ticks_freq(void)
{
	struct timemono t_start, t_end;
	struct timerel t_sleep, t_diff;
	uint64_t hz, end, start, ns;
	double secs;

	log_debug("measuring tick frequency\n");

	t_sleep = time_from_nsec(NS_PER_SEC / 10);

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &t_start.ts))
		return 0;

	start = get_ticks();

	nanosleep(&t_sleep.ts, NULL);

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &t_end.ts))
		return 0;

	end = get_ticks();

	t_diff = timemono_between(t_end, t_start);
	ns = time_to_nsec(t_diff);

	secs = (double)ns/NS_PER_SEC;
	hz = (uint64_t)((double)(end - start)/secs);

	return ROUND(hz, TICKS_PER_10MHZ);
}

static uint64_t estimate_ticks_freq(void)
{
	uint64_t start;

	log_debug("warning: estimating tick frequency; clock timings may be inaccurate\n");

	start = get_ticks();
	__usleep(1E6);

	return ROUND(get_ticks() - start, TICKS_PER_10MHZ);
}

static void __attribute__((constructor)) init_ticks_freq(void)
{
	uint64_t freq;

	freq = get_ticks_freq_arch();
	if (!freq)
		freq = read_cpu_freq_from_sys();

	if (!freq)
		freq = measure_ticks_freq();

	if (!freq)
		freq = estimate_ticks_freq();

	log_debug("tick frequency is ~%" PRIu64 " Hz\n", freq);

	__vfn_ticks_freq = freq;
}
