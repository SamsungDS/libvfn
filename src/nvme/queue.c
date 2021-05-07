// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vfn/nvme.h"

#include "support/log.h"

struct nvme_cqe *nvme_cq_poll(struct nvme_cq *cq)
{
	struct nvme_cqe *cqe;

	nvme_cq_spin(cq);

	cqe = nvme_cq_get_cqe(cq);
	assert(cqe);

	nvme_cq_update_head(cq);

	return cqe;
}
