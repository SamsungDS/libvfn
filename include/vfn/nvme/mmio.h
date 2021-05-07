/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
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

#ifndef LIBVFN_NVME_MMIO_H
#define LIBVFN_NVME_MMIO_H

#ifdef __cplusplus
extern "C" {
#endif

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
	mmio_write32(addr + 4, (leint32_t __force)(v >> 32));
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

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_NVME_MMIO_H */
