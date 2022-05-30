// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <byteswap.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#include "ccan/compiler/compiler.h"
#include "ccan/tap/tap.h"

#include "rq.c"

int main(void)
{
	struct nvme_rq rq;
	union nvme_cmd cmd;
	leint64_t *prplist;

	plan_tests(20);

	pgmap((void **)&prplist, PAGESIZE);

	rq.page.vaddr = prplist;
	rq.page.iova = 0x8000000;

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x1000);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x2000);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x3000);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x200);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x1000);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x2000);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x200);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	memset((void *)prplist, 0x0, PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x1000 - 4);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	return exit_status();
}
