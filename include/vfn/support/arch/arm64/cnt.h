/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2015 Cavium, Inc
 * Copyright(c) 2020 Arm Limited
 */

#ifndef LIBVFN_SUPPORT_ARCH_ARM64_CNT_H
#define LIBVFN_SUPPORT_ARCH_ARM64_CNT_H

static inline uint64_t __arm64_cntfrq(void)
{
	uint64_t freq;

	asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));

	return freq;
}

static inline uint64_t __arm64_cntvct(void)
{
	uint64_t tsc;

	asm volatile("mrs %0, cntvct_el0" : "=r" (tsc));

	return tsc;
}

static inline uint64_t get_ticks_arch(void)
{
	return __arm64_cntvct();
}

static inline uint64_t get_ticks_freq_arch(void)
{
	return __arm64_cntfrq();
}

#endif /* LIBVFN_SUPPORT_ARCH_ARM64_CNT_H */
