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

#include <sys/eventfd.h>

#include <vfn/nvme.h>

#include <nvme/types.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static unsigned long nsid;

static const struct nvme_ctrl_opts ctrl_opts = {
	.nsqr = 63,
	.ncqr = 63,
};

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_WITH_ARG("-N|--nsid", opt_set_ulongval, opt_show_ulongval, &nsid,
		     "namespace identifier"),
	OPT_ENDTABLE,
};

int main(int argc, char **argv)
{
	void *vaddr;
	uint64_t iova, v;
	int ret, efd, efds[2] = {-1, -1};

	struct nvme_ctrl ctrl = {};
	struct nvme_rq *rq;
	union nvme_cmd cmd;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	opt_free_table();

	if (nvme_init(&ctrl, bdf, &ctrl_opts))
		err(1, "failed to init nvme controller");

	efds[1] = efd = eventfd(0, EFD_CLOEXEC);
	if (efd < 0)
		err(1, "failed to create eventfd");

	if (vfio_set_irq(&ctrl.pci.dev, efds, 2))
		err(1, "failed to set irqs");

	if (nvme_create_ioqpair(&ctrl, 1, 64, 1, 0x0))
		err(1, "could not create io queue pair");

	vaddr = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if (iommu_map_vaddr(__iommu_ctx(&ctrl), vaddr, 0x1000, &iova, 0x0))
		err(1, "failed to reserve iova");

	rq = nvme_rq_acquire(&ctrl.sq[1]);

	cmd.rw = (struct nvme_cmd_rw) {
		.opcode = nvme_cmd_read,
		.nsid = cpu_to_le32(nsid),
	};

	nvme_rq_map_prp(rq, &cmd, iova, 0x1000);

	nvme_rq_exec(rq, &cmd);

	/* wait for eventfd (interrupt) */
	ret = read(efd, &v, sizeof(v));
	if (ret < 0)
		err(1, "error reading eventfd");

	/* consume cqe */
	nvme_rq_spin(rq, NULL);

	fprintf(stderr, "writing payload\n");

	ret = writeallfd(STDOUT_FILENO, vaddr, 0x1000);
	if (ret < 0)
		err(1, "could not write fd");

	fprintf(stderr, "wrote %d bytes\n", ret);

	nvme_rq_release(rq);

	nvme_close(&ctrl);

	return 0;
}
