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

#ifndef LIBVFN_ENDIAN_H
#define LIBVFN_ENDIAN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t __bitwise leint64_t;
typedef uint32_t __bitwise leint32_t;
typedef uint16_t __bitwise leint16_t;
typedef uint64_t __bitwise beint64_t;
typedef uint32_t __bitwise beint32_t;
typedef uint16_t __bitwise beint16_t;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define CPU_TO_LE64(native) ((__force leint64_t)(native))
# define CPU_TO_LE32(native) ((__force leint32_t)(native))
# define CPU_TO_LE16(native) ((__force leint16_t)(native))
# define LE64_TO_CPU(le_val) ((__force uint64_t)(le_val))
# define LE32_TO_CPU(le_val) ((__force uint32_t)(le_val))
# define LE16_TO_CPU(le_val) ((__force uint16_t)(le_val))
# define CPU_TO_BE64(native) ((__force beint64_t)bswap_64(native))
# define CPU_TO_BE32(native) ((__force beint32_t)bswap_32(native))
# define CPU_TO_BE16(native) ((__force beint16_t)bswap_16(native))
# define BE64_TO_CPU(le_val) bswap_64((__force uint64_t)le_val)
# define BE32_TO_CPU(le_val) bswap_32((__force uint32_t)le_val)
# define BE16_TO_CPU(le_val) bswap_16((__force uint16_t)le_val)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define CPU_TO_LE64(native) ((__force leint64_t)bswap_64(native))
# define CPU_TO_LE32(native) ((__force leint32_t)bswap_32(native))
# define CPU_TO_LE16(native) ((__force leint16_t)bswap_16(native))
# define LE64_TO_CPU(le_val) bswap_64((__force uint64_t)le_val)
# define LE32_TO_CPU(le_val) bswap_32((__force uint32_t)le_val)
# define LE16_TO_CPU(le_val) bswap_16((__force uint16_t)le_val)
# define CPU_TO_BE64(native) ((__force beint64_t)(native))
# define CPU_TO_BE32(native) ((__force beint32_t)(native))
# define CPU_TO_BE16(native) ((__force beint16_t)(native))
# define BE64_TO_CPU(le_val) ((__force uint64_t)(le_val))
# define BE32_TO_CPU(le_val) ((__force uint32_t)(le_val))
# define BE16_TO_CPU(le_val) ((__force uint16_t)(le_val))
#else
# error "cannot determine endianness"
#endif

/**
 * cpu_to_le64 - convert a uint64_t value to little-endian
 * @native: value to convert
 */
static inline leint64_t cpu_to_le64(uint64_t native)
{
	return CPU_TO_LE64(native);
}

/**
 * cpu_to_le32 - convert a uint32_t value to little-endian
 * @native: value to convert
 */
static inline leint32_t cpu_to_le32(uint32_t native)
{
	return CPU_TO_LE32(native);
}

/**
 * cpu_to_le16 - convert a uint16_t value to little-endian
 * @native: value to convert
 */
static inline leint16_t cpu_to_le16(uint16_t native)
{
	return CPU_TO_LE16(native);
}

/**
 * le64_to_cpu - convert a little-endian uint64_t value
 * @le_val: little-endian value to convert
 */
static inline uint64_t le64_to_cpu(leint64_t le_val)
{
	return LE64_TO_CPU(le_val);
}

/**
 * le32_to_cpu - convert a little-endian uint32_t value
 * @le_val: little-endian value to convert
 */
static inline uint32_t le32_to_cpu(leint32_t le_val)
{
	return LE32_TO_CPU(le_val);
}

/**
 * le16_to_cpu - convert a little-endian uint16_t value
 * @le_val: little-endian value to convert
 */
static inline uint16_t le16_to_cpu(leint16_t le_val)
{
	return LE16_TO_CPU(le_val);
}

/**
 * cpu_to_be64 - convert a uint64_t value to big endian.
 * @native: value to convert
 */
static inline beint64_t cpu_to_be64(uint64_t native)
{
	return CPU_TO_BE64(native);
}

/**
 * cpu_to_be32 - convert a uint32_t value to big endian.
 * @native: value to convert
 */
static inline beint32_t cpu_to_be32(uint32_t native)
{
	return CPU_TO_BE32(native);
}

/**
 * cpu_to_be16 - convert a uint16_t value to big endian.
 * @native: value to convert
 */
static inline beint16_t cpu_to_be16(uint16_t native)
{
	return CPU_TO_BE16(native);
}

/**
 * be64_to_cpu - convert a big-endian uint64_t value
 * @be_val: big-endian value to convert
 */
static inline uint64_t be64_to_cpu(beint64_t be_val)
{
	return BE64_TO_CPU(be_val);
}

/**
 * be32_to_cpu - convert a big-endian uint32_t value
 * @be_val: big-endian value to convert
 */
static inline uint32_t be32_to_cpu(beint32_t be_val)
{
	return BE32_TO_CPU(be_val);
}

/**
 * be16_to_cpu - convert a big-endian uint16_t value
 * @be_val: big-endian value to convert
 */
static inline uint16_t be16_to_cpu(beint16_t be_val)
{
	return BE16_TO_CPU(be_val);
}

static inline void put_unaligned_be48(const uint64_t v, uint8_t *p)
{
	p[0] = v >> 40;
	p[1] = v >> 32;
	p[2] = v >> 24;
	p[3] = v >> 16;
	p[4] = v >> 8;
	p[5] = v;
}

static inline uint64_t get_unaligned_be48(const uint8_t *p)
{
	uint64_t v = 0;

	v |= (uint64_t)p[0] << 40;
	v |= (uint64_t)p[1] << 32;
	v |= (uint64_t)p[2] << 24;
	v |= (uint64_t)p[3] << 16;
	v |= (uint64_t)p[4] << 8;
	v |= (uint64_t)p[5];

	return v;
}

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_ENDIAN_H */
