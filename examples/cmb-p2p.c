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

/*
 * Basic example of doing DMA P2P using a CMB.
 *
 * This example expects two devices (a source and a destination) where the
 * destination MUST have a CMB. For instance, testing with QEMU:
 *
 *   -device nvme,serial=deadc0de,drive=...
 *   -device nvme,serial=deadbeef,drive=...,cmb_size_mb=1
 */

#include <linux/pci_regs.h>

#include <vfn/nvme.h>

#include <nvme/types.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static char *bdfs[] = {"", ""};

static struct opt_table opts[] = {
	OPT_WITHOUT_ARG("-h|--help", opt_set_bool, &show_usage, "show usage"),
	OPT_WITH_ARG("-s|--source BDF", opt_set_charp, opt_show_charp, &bdfs[0],
		     "pci source device"),
	OPT_WITH_ARG("-d|--destination BDF", opt_set_charp, opt_show_charp, &bdfs[1],
		     "pci source device"),
	OPT_ENDTABLE,
};

static const struct nvme_ctrl_opts ctrl_opts = {
	.nsqr = 63,
	.ncqr = 63,
};

int main(int argc, char **argv)
{
	struct nvme_ctrl src = {}, dst = {};

	uint64_t iova;
	void *cmb;

	union nvme_cmd cmd = {};
	struct nvme_id_ctrl *id_ctrl;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdfs[0], ""))
		opt_usage_exit_fail("missing --source parameter");

	if (streq(bdfs[1], ""))
		opt_usage_exit_fail("missing --destination parameter");

	if (nvme_init(&src, bdfs[0], &ctrl_opts))
		err(1, "failed to initialize source nvme controller");

	if (nvme_init(&dst, bdfs[1], &ctrl_opts))
		err(1, "failed to initialize destination nvme controller");

	if (nvme_configure_cmb(&dst))
		err(1, "failed to initialize cmb to destination nvme controller");

	iova = dst.cmb.iova;
	cmb = dst.cmb.vaddr;

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.cns = NVME_IDENTIFY_CNS_CTRL,
		.dptr.prp1 = cpu_to_le64(iova),
	};

	if (nvme_admin(&src, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");

	id_ctrl = (struct nvme_id_ctrl __force *)cmb;
	printf("identity controller VER field value is %x\n", id_ctrl->ver);

	return 0;
}
