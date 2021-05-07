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

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <vfn/nvme.h>

#include <nvme/types.h>
#include <nvme/util.h>

#include <support/log.h>
#include <support/mem.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_ENDTABLE,
};

static struct nvme_ctrl ctrl;

static bool aen_received;

static int get_smart_log(void)
{
	struct nvme_smart_log *log = NULL;
	union nvme_cmd cmd;
	ssize_t len;
	int ret = 0;

	len = pgmap((void **)&log, sizeof(*log));
	if (len < 0)
		return -1;

	cmd.log = (struct nvme_cmd_log) {
		.opcode = nvme_admin_get_log_page,
		.numdl = cpu_to_le16((len >> 2) - 1),
		.nsid = cpu_to_le32(NVME_NSID_ALL),
		.lid = NVME_LOG_LID_SMART,
	};

	ret = nvme_oneshot(&ctrl, ctrl.adminq.sq, &cmd, log, len, NULL);

	pgunmap(log, len);

	return ret;
}

static void handle_aen(struct nvme_cqe *cqe)
{
	uint32_t dw0 = le32_to_cpu(cqe->dw0);
	int type, info, lid;

	uint32_t expected =
		NVME_AER_SMART |
		NVME_AER_SMART_TEMPERATURE_THRESHOLD << 8 |
		NVME_LOG_LID_SMART << 16;

	type = NVME_AEN_TYPE(dw0);
	info = NVME_AEN_INFO(dw0);
	lid = NVME_AEN_LID(dw0);

	printf("got aen 0x%"PRIx32" (type 0x%x info 0x%x lid 0x%x)\n", dw0, type, info, lid);

	if (dw0 == expected)
		aen_received = true;
}

int main(int argc, char **argv)
{
	union nvme_cmd cmd;
	struct nvme_cqe cqe;

	uint32_t temp_thresh;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	opt_free_table();

	cmd.features = (struct nvme_cmd_features) {
		.opcode = nvme_admin_set_features,
		.fid = NVME_FEAT_FID_ASYNC_EVENT,
		.cdw11 = cpu_to_le32(NVME_SET(NVME_SMART_CRIT_TEMPERATURE, FEAT_AE_SMART)),
	};

	if (nvme_oneshot(&ctrl, ctrl.adminq.sq, &cmd, NULL, 0x0, NULL))
		err(1, "could not set asynchronous event configuration");

	cmd.features = (struct nvme_cmd_features) {
		.opcode = nvme_admin_get_features,
		.fid = NVME_FEAT_FID_TEMP_THRESH,
	};

	if (nvme_oneshot(&ctrl, ctrl.adminq.sq, &cmd, NULL, 0x0, &cqe))
		err(1, "could not get current temperature threshold");

	temp_thresh = le32_to_cpu(cqe.dw0);

	printf("current temperature threshold is %"PRIu32" K\n", temp_thresh);

	if (nvme_aen_enable(&ctrl, handle_aen))
		err(1, "could not enable aen");

	cmd.features = (struct nvme_cmd_features) {
		.opcode = nvme_admin_set_features,
		.fid = NVME_FEAT_FID_TEMP_THRESH,
		.cdw11 = cpu_to_le32(NVME_SET(200, FEAT_TT_TMPTH)),
	};

	do {
		if (nvme_oneshot(&ctrl, ctrl.adminq.sq, &cmd, NULL, 0x0, &cqe))
			err(1, "could not set temperature threshold");
	} while (!aen_received);

	cmd.features = (struct nvme_cmd_features) {
		.opcode = nvme_admin_set_features,
		.fid = NVME_FEAT_FID_TEMP_THRESH,
		.cdw11 = cpu_to_le32(NVME_SET(temp_thresh, FEAT_TT_TMPTH)),
	};

	if (nvme_oneshot(&ctrl, ctrl.adminq.sq, &cmd, NULL, 0x0, &cqe))
		err(1, "could not reset temperature threshold");

	get_smart_log();

	nvme_close(&ctrl);

	return 0;
}
