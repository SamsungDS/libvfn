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

#ifndef LIBVFN_SUPPORT_ALIGN_H
#define LIBVFN_SUPPORT_ALIGN_H

/**
 * ALIGN_UP - Align value up
 * @x: value to be aligned
 * @a: alignment
 *
 * Alignment must be a power of two.
 *
 * Return: Aligned value
 */
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

/**
 * ALIGN_DOWN - Align value down
 * @x: value to be aligned
 * @a: alignment
 *
 * Alignment must be a power of two.
 *
 * Return: Aligned value
 */
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

/**
 * ALIGNED - Check if value is aligned
 * @x: value to be checked
 * @a: alignment
 *
 * Alignment must be a power of two.
 *
 * Return: Boolean
 */
#define ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

/**
 * ROUND_DOWN - Round down to mutiple
 * @x: value to be rounded
 * @m: multiple
 *
 * Round value down to multiple. Safe when @m is not a power of 2.
 *
 * Return: Rounded value
 */
#define ROUND_DOWN(x, m) ((x) / (m) * (m))

/**
 * ROUND_UP - Round up to mutiple
 * @x: value to be rounded
 * @m: multiple
 *
 * Round value up to multiple. Safe when @m is not a power of 2.
 *
 * Return: Rounded value
 */
#define ROUND_UP(x, m) ROUND_DOWN((x) + (m) - 1, (m))

/**
 * ROUND - Round to mutiple
 * @x: value to be rounded
 * @m: multiple
 *
 * Round value to multiple (half to even). Safe when @m is not a power of 2.
 *
 * Return: Rounded value
 */
#define ROUND(x, m) \
	({ \
		typeof(x) ceil = ROUND_UP(x, m); \
		typeof(x) floor = ROUND_DOWN(x, m); \
		(ceil - (x)) > ((x) - floor) ? floor : ceil; \
	})

#endif /* LIBVFN_SUPPORT_ALIGN_H */
