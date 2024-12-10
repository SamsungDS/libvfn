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

#define __max_prps 513

bool iommu_translate_vaddr(struct iommu_ctx *ctx UNUSED, void *vaddr, uint64_t *iova)
{
	*iova = (uint64_t)vaddr;

	return true;
}

int iommu_map_vaddr(struct iommu_ctx *ctx UNUSED, void *vaddr UNUSED, size_t len UNUSED,
		    uint64_t *iova UNUSED, unsigned long flags UNUSED)
{
	return 0;
}

int iommu_unmap_vaddr(struct iommu_ctx *ctx UNUSED, void *vaddr UNUSED, size_t *len UNUSED)
{
	return 0;
}

int main(void)
{
	struct nvme_ctrl ctrl = {
		.config.mps = 0,
	};

	struct nvme_rq rq;
	union nvme_cmd cmd;
	leint64_t *prplist;
	struct nvme_sgld *sglds;
	struct iovec iov[8];

	plan_tests(126);

	assert(pgmap((void **)&rq.page.vaddr, __VFN_PAGESIZE) > 0);

	rq.page.iova = (uint64_t)rq.page.vaddr;
	prplist = rq.page.vaddr;
	sglds = rq.page.vaddr;

	/* test 512b aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x200) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x1000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k + 8 aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x1008) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x2000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k + 16 aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x2010) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x3000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k + 24 aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, 0x3018) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 512b unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x200) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 512 unaligned (nasty) */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1001000 - 4, 0x200) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1001000 - 4);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x1000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k + 8 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x1008) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k + 8 unaligned (nasty) */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1001000 - 4, 0x1008) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1001000 - 4);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 4k - 4 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x1000 - 4) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x2000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k + 16 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x2010) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k - 4 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x2000 - 4) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x3000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);
	ok1(le64_to_cpu(prplist[2]) == 0x1003000);

	/* test 12k + 24 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000004, 0x3018) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);
	ok1(le64_to_cpu(prplist[2]) == 0x1003000);

	/* test 512b aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x200};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x2000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x3000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k aligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned 3-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	iov[2] = (struct iovec) {.iov_base = (void *)0x1002000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 3) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k aligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x2000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 3) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 512b unaligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x200};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k unaligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k unaligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000 - 4};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k unaligned 3-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000 - 4};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000};
	iov[2] = (struct iovec) {.iov_base = (void *)0x1002000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 3) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test handling of unaligned length in last iovec entry */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = 0x1000 - 4};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);


	/*
	 * Failure tests
	 */

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, 0x1000000, (__max_prps + 1) * 0x1000) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001004, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = __max_prps * 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == -1);

	/*
	 * SGLs
	 */

	memset((void *)sglds, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_sgl(&ctrl, &rq, &cmd, iov, 1) == 0);
	ok1(le64_to_cpu(cmd.dptr.sgl.addr) == 0x1000000);
	ok1(le32_to_cpu(cmd.dptr.sgl.len) == 0x1000);
	ok1(cmd.dptr.sgl.type == NVME_SGLD_TYPE_DATA_BLOCK);

	memset((void *)sglds, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1002000, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_sgl(&ctrl, &rq, &cmd, iov, 2) == 0);
	ok1(le64_to_cpu(sglds[0].addr) == 0x1000000);
	ok1(le64_to_cpu(sglds[1].addr) == 0x1002000);

	return exit_status();
}
