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

#include <inttypes.h>

#include <sys/mman.h>

#include <linux/iommufd.h>

#include <vfn/vfio.h>
#include <vfn/iommu.h>
#include <vfn/iommu/iommufd.h>
#include <vfn/support.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

#define REG_ADDR 0x0
#define REG_CMD  0x8

#define IOVA_BASE 0xfef00000

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_ENDTABLE,
};

struct vfio_pci_device pdev;

int main(int argc, char **argv)
{
	void *bar0;
	uint64_t iova = IOVA_BASE;
	ssize_t len;
	void *vaddr;

	struct iommufd_fault_queue fq;

	struct iommu_hwpt_pgfault pgfault;
	struct iommu_hwpt_page_response pgresp = {
		.code = IOMMUFD_PAGE_RESP_SUCCESS,
	};

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	opt_free_table();

	if (vfio_pci_open(&pdev, bdf))
		err(1, "failed to open pci device");

	if (iommufd_alloc_fault_queue(&fq))
		err(1, "could not allocate fault queue");

	if (iommufd_set_fault_queue(pdev.dev.ctx, &fq, pdev.dev.fd))
		err(1, "could not associate fault queue with device/ioas");

	bar0 = vfio_pci_map_bar(&pdev, 0, 0x1000, 0, PROT_READ | PROT_WRITE);
	if (!bar0)
		err(1, "failed to map bar");

	len = pgmap(&vaddr, 0x1000);
	if (len < 0)
		err(1, "could not allocate aligned memory");

	memset(vaddr, 0x42, 0x1000);

	mmio_lh_write64(bar0 + REG_ADDR, iova);
	mmio_write32(bar0 + REG_CMD, 0x3);

	/* wait for page fault */
	while (read(fq.fault_fd, &pgfault, sizeof(pgfault)) == 0)
		;

	printf("handling page fault on addr 0x%llx\n", pgfault.addr);

	if (iommu_map_vaddr(pdev.dev.ctx, vaddr, 0x1000, &iova, IOMMU_MAP_FIXED_IOVA))
		err(1, "failed to map page");

	pgresp.cookie = pgfault.cookie;

	if (write(fq.fault_fd, &pgresp, sizeof(pgresp)) < 0)
		err(1, "failed to write page response");

	while (mmio_read32(bar0 + REG_CMD) & 0x1)
		;

	memset(vaddr, 0x0, 0x1000);

	mmio_write32(bar0 + REG_CMD, 0x1);

	while (mmio_read32(bar0 + REG_CMD) & 0x1)
		;

	for (int i = 0; i < 0x1000; i++) {
		uint8_t byte = *(uint8_t *)(vaddr + i);

		if (byte != 0x42)
			errx(1, "unexpected byte 0x%"PRIx8, byte);
	}

	return 0;
}
