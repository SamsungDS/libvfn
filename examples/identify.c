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
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <vfn/nvme.h>

#include <nvme/types.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static unsigned int cns = NVME_IDENTIFY_CNS_CTRL;
static unsigned long nsid;

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_WITH_ARG("-N|--nsid", opt_set_ulongval, opt_show_ulongval_hex, &nsid, "namespace identifier"),
	OPT_WITH_ARG("-C|--cns", opt_set_uintval, opt_show_uintval_hex, &cns, "controller/namespace structure"),
	OPT_ENDTABLE,
};

int main(int argc, char **argv)
{
	void *vaddr;
	ssize_t len;

	struct nvme_ctrl ctrl = {};

	struct nvme_id_ctrl *id_ctrl;
	union nvme_cmd cmd;
	struct nvme_id_ns *id_ns;
	struct nvme_id_independent_id_ns *id_ns_ind;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	opt_free_table();

	len = pgmap(&vaddr, NVME_IDENTIFY_DATA_SIZE);
	if (len < 0)
		err(1, "could not allocate aligned memory");

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.cns = cns,
		.nsid = cpu_to_le32(nsid),
	};

	if (nvme_oneshot(&ctrl, ctrl.adminq.sq, &cmd, vaddr, len, NULL))
		err(1, "nvme_oneshot");

	switch (cns) {
	case NVME_IDENTIFY_CNS_CTRL:
		id_ctrl = vaddr;

		printf("vid 0x%"PRIx8"\n", id_ctrl->vid);

		break;

	case NVME_IDENTIFY_CNS_NS:
		id_ns = vaddr;

		printf("eui64 0x%"PRIx64"\n", (uint64_t)id_ns->eui64);

		break;

	case NVME_IDENTIFY_CNS_CSI_INDEPENDENT_ID_NS:
		id_ns_ind = vaddr;

		printf("nmic 0x%"PRIx8"\n", id_ns_ind->nmic);

		break;

	default:
		printf("unknown cns\n");

		break;
	}

	nvme_close(&ctrl);

	return 0;
}
