/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#ifndef LIBVFN_SUPPORT_MMIO_H
#define LIBVFN_SUPPORT_MMIO_H

/**
 * mmio_read32 - read 4 bytes in memory-mapped register
 * @addr: memory-mapped register
 *
 * Return: read value (native endian)
 */
static inline leint32_t mmio_read32(void *addr)
{
	/* memory-mapped register */
	return *(const volatile leint32_t __force *)addr;
}

/**
 * mmio_lh_read64 - read 8 bytes in memory-mapped register
 * @addr: memory-mapped register
 *
 * Read the low 4 bytes first, then the high 4 bytes.
 *
 * Return: read value (native endian)
 */
static inline leint64_t mmio_lh_read64(void *addr)
{
	uint32_t lo, hi;

	/* memory-mapped register */
	lo = *(const volatile uint32_t __force *)addr;
	/* memory-mapped register */
	hi = *(const volatile uint32_t __force *)(addr + 4);

	return (leint64_t __force)(((uint64_t)hi << 32) | lo);
}

#define mmio_read64(addr) mmio_lh_read64(addr)

/**
 * mmio_write32 - write 4 bytes to memory-mapped register
 * @addr: memory-mapped register
 * @v: value to write (native endian)
 */
static inline void mmio_write32(void *addr, leint32_t v)
{
	/* memory-mapped register */
	*(volatile leint32_t __force *)addr = v;
}

/**
 * mmio_lh_write64 - write 8 bytes to memory-mapped register
 * @addr: memory-mapped register
 * @v: value to write (native endian)
 *
 * Write 8 bytes to memory-mapped register as two 4 byte writes (low bytes
 * first, then high).
 */
static inline void mmio_lh_write64(void *addr, leint64_t v)
{
	mmio_write32(addr, (leint32_t __force)v);
	mmio_write32(addr + 4, (leint32_t __force)((uint64_t __force)v >> 32));
}

/**
 * mmio_hl_write64 - write 8 bytes to memory-mapped register
 * @addr: memory-mapped register
 * @v: value to write (native endian)
 *
 * Write 8 bytes to memory-mapped register as two 4 byte writes (high bytes
 * first, then low).
 */
static inline void mmio_hl_write64(void *addr, leint64_t v)
{
	mmio_write32(addr + 4, (leint32_t __force)((uint64_t __force)v >> 32));
	mmio_write32(addr, (leint32_t __force)v);
}

#endif /* LIBVFN_SUPPORT_MMIO_H */
