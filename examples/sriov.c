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

#include <vfn/nvme.h>
#include <vfn/pci.h>

#include <nvme/types.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static int vfnum;

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_WITH_ARG("-s|--secondary VFNUM", opt_set_intval, opt_show_intval, &vfnum,
		     "secondary controller virtual function number"),
	OPT_ENDTABLE,
};

int main(int argc, char **argv)
{
	char *vf_bdf;
	uint16_t scid;

	struct nvme_ctrl ctrl = {}, sctrl = {};

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	opt_free_table();

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	vf_bdf = pci_get_vf_bdf(bdf, vfnum);
	if (!vf_bdf)
		err(1, "pci_get_vf_bdf");

	if (vfio_pci_open(&sctrl.pci, vf_bdf))
		err(1, "vfio_pci_open");

	if (nvme_get_vf_cntlid(&ctrl, vfnum, &scid))
		err(1, "nvme_get_vf_cntlid");

	if (nvme_vm_set_offline(&ctrl, scid))
		err(1, "could not offline secondary controller");

	if (nvme_vm_assign_max_flexible(&ctrl, scid))
		err(1, "could not assign resources");

	if (vfio_reset(&sctrl.pci.dev))
		err(1, "vfio_reset");

	if (nvme_vm_set_online(&ctrl, scid))
		err(1, "could not online secondary controller");

	if (nvme_init(&sctrl, vf_bdf, NULL))
		err(1, "failed to init nvme controller");

	return 0;
}
