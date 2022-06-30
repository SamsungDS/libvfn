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

#include <stdbool.h>
#include <stdint.h>

#include <nvme/types.h>

#include "ccan/opt/opt.h"
#include "ccan/tap/tap.h"

#include "vfn/nvme.h"

#include "common.h"

static int test_io(void)
{
	void *vaddr;
	ssize_t len;
	int ret;

	struct nvme_sq *sq;
	union nvme_cmd cmd;

	if (nvme_create_ioqpair(&ctrl, 1, 8, 0x0)) {
		diag("could not create io queue pair");
		return -1;
	}

	sq = &ctrl.sq[1];

	len = pgmap(&vaddr, __VFN_PAGESIZE);
	if (len < 0)
		goto out;

	cmd.rw = (struct nvme_cmd_rw) {
		.opcode = nvme_cmd_read,
		.nsid = cpu_to_le32(nsid),
	};

	ret = nvme_oneshot(&ctrl, sq, &cmd, vaddr, len, NULL);

	pgunmap(vaddr, len);

out:
	if (nvme_delete_ioqpair(&ctrl, 1)) {
		diag("could not delete io queue pair");
		return -1;
	}

	return ret;
}

static int test_identify(uint8_t cns)
{
	union nvme_cmd cmd;
	void *vaddr;
	ssize_t len;
	int ret;

	len = pgmap(&vaddr, NVME_IDENTIFY_DATA_SIZE);
	if (len < 0)
		return -1;

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.cns = cns,
		.nsid = cpu_to_le32(nsid),
	};

	ret = nvme_admin(&ctrl, &cmd, vaddr, len, NULL);

	pgunmap(vaddr, len);

	return ret;
}

int main(int argc, char **argv)
{
	setup(argc, argv);

	plan_tests(3);

	ok(test_identify(NVME_IDENTIFY_CNS_CTRL) == 0, "identify controller");

	if (nsid) {
		ok(test_identify(NVME_IDENTIFY_CNS_NS) == 0, "identify namespace");
		ok(test_io() == 0, "io read");
	} else {
		skip(2, "namespace identifier not set");
	}

	teardown();

	return 0;
}
