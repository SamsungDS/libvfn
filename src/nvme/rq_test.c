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

/* for mps=0: one prplist page holds this many entries */
#define __max_prps_per_page (1 << (__mps_to_pageshift(0) - 3))

bool iommu_translate_vaddr(struct iommu_ctx *ctx UNUSED, void *vaddr, iova_t *iova)
{
	*iova = (uint64_t)vaddr;

	return true;
}

int iommu_map_vaddr(struct iommu_ctx *ctx UNUSED, void *vaddr UNUSED, size_t len UNUSED,
		    iova_t *iova UNUSED, unsigned long flags UNUSED)
{
	return 0;
}

int iommu_unmap_vaddr(struct iommu_ctx *ctx UNUSED, void *vaddr UNUSED, size_t *len UNUSED)
{
	return 0;
}

int iommu_get_dmabuf(struct iommu_ctx *ctx UNUSED, struct iommu_dmabuf *buffer UNUSED,
		     size_t len UNUSED, unsigned long flags UNUSED)
{
	return 0;
}

void iommu_put_dmabuf(struct iommu_dmabuf *buffer UNUSED)
{
	;
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
	struct iova_vec iovav[8];

	/* multi-page prplist: two contiguous mapped pages used as one array */
	leint64_t *mprplists;
	void *mppages;

	plan_tests(179 + 12);

	assert(pgmap((void **)&rq.page.vaddr, __VFN_PAGESIZE) > 0);

	rq.page.iova = (uint64_t)rq.page.vaddr;
	prplist = rq.page.vaddr;
	sglds = rq.page.vaddr;

	/* two contiguous pages for multi-page prplist chaining */
	assert(pgmapn(&mppages, 2, __VFN_PAGESIZE) > 0);
	mprplists = mppages;

	/* test 512b aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x200) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x1000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k + 8 aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x1008) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x2000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k + 16 aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x2010) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x3000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k + 24 aligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, 0x3018) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 512b unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x200) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 512 unaligned (nasty) */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1001000 - 4, 0x200) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1001000 - 4);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x1000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k + 8 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x1008) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 4k + 8 unaligned (nasty) */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1001000 - 4, 0x1008) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1001000 - 4);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 4k - 4 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x1000 - 4) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x2000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k + 16 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x2010) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k - 4 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x2000 - 4) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x3000) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);
	ok1(le64_to_cpu(prplist[2]) == 0x1003000);

	/* test 12k + 24 unaligned */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000004, 0x3018) == 0);

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
	 * PRPs with iova tests
	 */

	/* test 512b aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x200};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 8k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x2000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x3000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 8k aligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 2) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k aligned 3-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = 0x1000};
	iovav[2] = (struct iova_vec) {.iova = (iova_t)0x1002000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 3) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 12k aligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = 0x2000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 3) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test 512b unaligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000004, .len = 0x200};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x0);

	/* test 4k unaligned 1-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000004, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 1) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 8k unaligned 2-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000004, .len = 0x1000 - 4};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 2) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);

	/* test 12k unaligned 3-iovec */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000004, .len = 0x1000 - 4};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = 0x1000};
	iovav[2] = (struct iova_vec) {.iova = (iova_t)0x1002000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 3) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000004);
	ok1(le64_to_cpu(cmd.dptr.prp2) == rq.page.iova);
	ok1(le64_to_cpu(prplist[0]) == 0x1001000);
	ok1(le64_to_cpu(prplist[1]) == 0x1002000);

	/* test handling of unaligned length in last iovec entry */
	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = 0x1000 - 4};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 2) == 0);

	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == 0x1001000);


	/*
	 * Failure tests
	 */

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	ok1(nvme_rq_map_prp(&ctrl, &rq, &cmd, (iova_t)0x1000000, (__max_prps + 1) * 0x1000) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000004, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001004, .iov_len = 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iov[0] = (struct iovec) {.iov_base = (void *)0x1000000, .iov_len = 0x1000};
	iov[1] = (struct iovec) {.iov_base = (void *)0x1001000, .iov_len = __max_prps * 0x1000};
	ok1(nvme_rq_mapv_prp(&ctrl, &rq, &cmd, iov, 2) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000004, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001004, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 2) == -1);

	memset((void *)prplist, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1001000, .len = __max_prps * 0x1000};
	ok1(nvme_rq_mapv_iova_prp(&ctrl, &rq, &cmd, iovav, 2) == -1);


	/*
	 * Multi-page prplist chaining (direct nvme_map_prp/nvme_mapv_prp with two
	 * pages). For mps=0, max_prps is 512 and two pages hold up to
	 * (2 - 1) * (512 - 1) + 512 = 1023 data PRPs.
	 */

	/*
	 * (__max_prps_per_page + 1) PRPs (prp1 + __max_prps_per_page list entries)
	 * overflow a single page. With two pages the list is chained: the last list
	 * entry is written to the second page, and the first page's last slot holds
	 * the chain link.
	 */
	memset(mppages, 0x0, 2 * __VFN_PAGESIZE);
	ok1(nvme_map_prp(&ctrl, mprplists, 2, &cmd, (iova_t)0x1000000,
			 (__max_prps_per_page + 1) * 0x1000) == 0);
	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == (uint64_t)mprplists);
	/* last slot of the first page chains to the second page */
	ok1(le64_to_cpu(mprplists[__max_prps_per_page - 1]) ==
	    (uint64_t)mprplists + __VFN_PAGESIZE);
	/* the chained entry on the second page is the last list PRP */
	ok1(le64_to_cpu(mprplists[__max_prps_per_page]) ==
	    0x1000000 + (uint64_t)__max_prps_per_page * 0x1000);

	/*
	 * Two pages hold at most __max_prps_per_page + (__max_prps_per_page - 1)
	 * list PRPs (i.e. that many + 1 total). Requiring one more total PRP must
	 * fail.
	 */
	memset(mppages, 0x0, 2 * __VFN_PAGESIZE);
	ok1(nvme_map_prp(&ctrl, mprplists, 2, &cmd, (iova_t)0x1000000,
			 (2 * __max_prps_per_page) * 0x1000 + 0x1000) == -1);

	/*
	 * Multi-page chaining across iovec segments: the first iovec fills the
	 * first page and spills one entry onto the second page, the second iovec
	 * appends a further entry on the second page.
	 */
	memset(mppages, 0x0, 2 * __VFN_PAGESIZE);
	iov[0] = (struct iovec) {
		.iov_base = (void *)0x1000000,
		.iov_len = (__max_prps_per_page + 1) * 0x1000,
	};
	iov[1] = (struct iovec) {
		.iov_base = (void *)(0x1000000 + (__max_prps_per_page + 1) * 0x1000),
		.iov_len = 0x1000,
	};
	ok1(nvme_mapv_prp(&ctrl, mprplists, 2, &cmd, iov, 2) == 0);
	ok1(le64_to_cpu(cmd.dptr.prp1) == 0x1000000);
	ok1(le64_to_cpu(cmd.dptr.prp2) == (uint64_t)mprplists);
	/* first page chains to the second */
	ok1(le64_to_cpu(mprplists[__max_prps_per_page - 1]) ==
	    (uint64_t)mprplists + __VFN_PAGESIZE);
	/* second page: first entry is the spill from iov[0], next is iov[1] */
	ok1(le64_to_cpu(mprplists[__max_prps_per_page]) ==
	    0x1000000 + (uint64_t)__max_prps_per_page * 0x1000);
	ok1(le64_to_cpu(mprplists[__max_prps_per_page + 1]) ==
	    0x1000000 + (uint64_t)(__max_prps_per_page + 1) * 0x1000);

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


	/*
	 * SGLs with iova tests
	 */

	memset((void *)sglds, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_sgl(&ctrl, &rq, &cmd, iovav, 1) == 0);
	ok1(le64_to_cpu(cmd.dptr.sgl.addr) == 0x1000000);
	ok1(le32_to_cpu(cmd.dptr.sgl.len) == 0x1000);
	ok1(cmd.dptr.sgl.type == NVME_SGLD_TYPE_DATA_BLOCK);

	memset((void *)sglds, 0x0, __VFN_PAGESIZE);
	iovav[0] = (struct iova_vec) {.iova = (iova_t)0x1000000, .len = 0x1000};
	iovav[1] = (struct iova_vec) {.iova = (iova_t)0x1002000, .len = 0x1000};
	ok1(nvme_rq_mapv_iova_sgl(&ctrl, &rq, &cmd, iovav, 2) == 0);
	ok1(le64_to_cpu(sglds[0].addr) == 0x1000000);
	ok1(le64_to_cpu(sglds[1].addr) == 0x1002000);

	return exit_status();
}
