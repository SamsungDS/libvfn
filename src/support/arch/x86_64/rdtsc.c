// SPDX-License-Identifier: BSD-3-Clause

/*
 * Copyright(c) 2010-2014 Intel Corporation
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * From DPDK; modified by the libvfn Authors.
 */

#if defined(__x86_64__)

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <cpuid.h>

#include "vfn/support/ticks.h"

static inline uint64_t __x86_64_get_tsc_freq(void)
{
	uint32_t a, b, c, d, maxleaf;

	maxleaf = __get_cpuid_max(0, NULL);

	if (maxleaf >= 0x15) {
		__cpuid(0x15, a, b, c, d);

		if (b && c)
			return c * (b / a);
	}

	return 0;
}

uint64_t get_ticks_freq_arch(void)
{
	return __x86_64_get_tsc_freq();
}

#ifdef __cplusplus
}
#endif

#endif
