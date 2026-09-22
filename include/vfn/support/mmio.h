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
 * Return: read value (little endian)
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
 * Return: read value (little endian)
 */
static inline leint64_t mmio_lh_read64(void *addr)
{
	uint32_t a, b;

	/* read the low-addressed word first (devices may latch the high one) */
	a = mmio_read32(addr);
	b = mmio_read32((char *)addr + 4);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return (leint64_t __force)(((uint64_t)b << 32) | a);
#else
	return (leint64_t __force)(((uint64_t)a << 32) | b);
#endif
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
 * @v: value to write (little endian)
 *
 * Write 8 bytes to memory-mapped register as two 4 byte writes (low bytes
 * first, then high).
 */
static inline void mmio_lh_write64(void *addr, leint64_t v)
{
	uint64_t x = le64_to_cpu(v);

	mmio_write32(addr, cpu_to_le32((uint32_t)x));
	mmio_write32((char *)addr + 4, cpu_to_le32((uint32_t)(x >> 32)));
}

/**
 * mmio_hl_write64 - write 8 bytes to memory-mapped register
 * @addr: memory-mapped register
 * @v: value to write (little endian)
 *
 * Write 8 bytes to memory-mapped register as two 4 byte writes (high bytes
 * first, then low).
 */
static inline void mmio_hl_write64(void *addr, leint64_t v)
{
	uint64_t x = le64_to_cpu(v);

	mmio_write32((char *)addr + 4, cpu_to_le32((uint32_t)(x >> 32)));
	mmio_write32(addr, cpu_to_le32((uint32_t)x));
}

#endif /* LIBVFN_SUPPORT_MMIO_H */
