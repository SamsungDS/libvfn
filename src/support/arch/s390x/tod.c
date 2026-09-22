// SPDX-License-Identifier: LGPL-2.1-or-later OR MIT

#include <stdint.h>

#include "vfn/support/ticks.h"

/* get_ticks_arch() returns monotonic nanoseconds on s390x. */
uint64_t get_ticks_freq_arch(void)
{
	return 1000000000ull;
}
