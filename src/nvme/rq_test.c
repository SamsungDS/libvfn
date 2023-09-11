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

#include "ccan/tap/tap.h"

#include "rq.c"

#define MPS 12

int main(void)
{
	struct nvme_rq rq;
	union nvme_cmd cmd;
	leint64_t *prplist;
	struct iovec iov[8];

	plan_tests(62);

	assert(pgmap((void **)&prplist, __VFN_PAGESIZE) > 0);

	rq.page.vaddr = prplist;
	rq.page.iova = 0x8000000;

	/* test 512b aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x200, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x1000, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x2000, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000000, 0x3000, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 512b unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x200, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x1000, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k - 4 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x1000 - 4, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x2000, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k - 4 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x2000 - 4, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	nvme_rq_map_prp(&rq, &cmd, 0x1000004, 0x3000, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);
	ok1(le64_to_cpu(prplist[2]) == 0x1003000);

	/* test 512b aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x200};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 1, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 1, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x2000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 1, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x3000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 1, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k aligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 2, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned 3-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	iov[2] = (struct iovec) {.iov_base = (void *)0x1002000, .iov_len = 0x1000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 3, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k aligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x2000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 3, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 512b unaligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x200};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 1, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k unaligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 1, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k unaligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000 - 4};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 2, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k unaligned 3-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000 - 4};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	iov[2] = (struct iovec) {.iov_base = (void *)0x1002000, .iov_len = 0x1000};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 3, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x8000000);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test handling of unaligned length in last iovec entry */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000 - 4};
	nvme_rq_mapv_prp(&rq, &cmd, iov, 2, MPS);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);


	/*
	 * Failure tests
	 */

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&rq, &cmd, 0x1000000, (__rq_max_prps + 1) * 0x1000, MPS) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000};
	iov[0] = (struct iovec) {.iov_base = (void *)0x1001004, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&rq, &cmd, iov, 2, MPS) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = __rq_max_prps * 0x1000};
	ok1(nvme_rq_mapv_prp(&rq, &cmd, iov, 2, MPS) == -1);

	return exit_status();
}
