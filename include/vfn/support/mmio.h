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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * On some platforms (notably s390x/zPCI) VFIO BARs cannot be mmap()ed and
 * are reachable only via pread()/pwrite() on the device fd; the accessors
 * below route such accesses through vfn_mmio_synth_* instead of dereferencing.
 */
enum vfn_mmio_synth_result {
	/* @addr is not synthetic; access it as ordinary memory-mapped I/O */
	VFN_MMIO_NOT_SYNTH = 0,

	/* @addr is synthetic; the access was serviced via fd I/O */
	VFN_MMIO_SYNTH_DONE,

	/* @addr is synthetic but the access failed and was logged */
	VFN_MMIO_SYNTH_ERROR,
};

extern enum vfn_mmio_synth_result vfn_mmio_synth_read(void *addr, void *buf,
							 size_t len);
extern enum vfn_mmio_synth_result vfn_mmio_synth_write(void *addr,
							  const void *buf, size_t len);

/*
 * vfn_mmio_synth_is - is @addr a synthetic BAR address?
 *
 * Return: true if @addr falls inside a live synthetic region.
 */
extern bool vfn_mmio_synth_is(const void *addr);

/*
 * vfn_mmio_synth_map - map a non-mmap()able region as emulated MMIO
 * @owner: opaque owner, to drain the region when the owner is torn down
 * @fd: device fd to service accesses with
 * @file_off: region offset in @fd
 * @size: region size
 *
 * Hand out a synthetic address, in a reserved PROT_NONE window, that the
 * accessors above route through @fd. @owner is not interpreted here.
 *
 * Return: the synthetic address, or NULL with errno set.
 */
extern void *vfn_mmio_synth_map(void *owner, int fd, uint64_t file_off,
				size_t size);

/*
 * vfn_mmio_synth_unmap - release the synthetic mapping at @addr
 *
 * Return: true if @addr is (or was) inside the synthetic arena, in which case
 * it must NOT be munmap()ed (the arena is pre-reserved); false if @addr is an
 * ordinary mapping. The region is released only if @owner owns it.
 */
extern bool vfn_mmio_synth_unmap(void *owner, void *addr);

/*
 * vfn_mmio_synth_drain - release every synthetic mapping owned by @owner
 */
extern void vfn_mmio_synth_drain(void *owner);

/**
 * mmio_read32 - read 4 bytes in memory-mapped register
 * @addr: memory-mapped register
 *
 * Return: read value (little endian)
 */
static inline leint32_t mmio_read32(void *addr)
{
	leint32_t v;

	switch (vfn_mmio_synth_read(addr, &v, sizeof(v))) {
	case VFN_MMIO_SYNTH_DONE:
		return v;
	case VFN_MMIO_SYNTH_ERROR:
		/* @addr is a PROT_NONE window: all-ones, never dereference */
		return cpu_to_le32(0xffffffffu);
	case VFN_MMIO_NOT_SYNTH:
	default:
		break;
	}

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
	a = (uint32_t __force)mmio_read32(addr);
	b = (uint32_t __force)mmio_read32((char *)addr + 4);

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
	if (vfn_mmio_synth_write(addr, &v, sizeof(v)) != VFN_MMIO_NOT_SYNTH)
		return;

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
