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

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

#define ROUND_DOWN(x, m) ((x) / (m) * (m))
#define ROUND_UP(x, m) ROUND_DOWN((x) + (m) - 1, (m))
#define ROUND(x, m) \
	({ \
		typeof(x) ceil = ROUND_UP(x, m); \
		typeof(x) floor = ROUND_DOWN(x, m); \
		(ceil - (x)) > ((x) - floor) ? floor : ceil; \
	})

#endif /* LIBVFN_SUPPORT_ALIGN_H */
