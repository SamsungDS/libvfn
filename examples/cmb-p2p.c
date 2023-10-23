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

static inline off_t bar_offset(int bar)
{
	return PCI_BASE_ADDRESS_0 + bar * 4;
}

static void *nvme_configure_cmb(struct nvme_ctrl *ctrl, uint64_t *hwaddr, uint64_t *cba)
{
	uint64_t cap, szu, ofst;
	uint32_t cmbloc, cmbsz, v32;
	size_t len;
	int bir;
	void *cmb;
	struct iova_range *iova_ranges;
	int num_iova_ranges;

	cap = le64_to_cpu(mmio_read64(ctrl->regs + NVME_REG_CAP));

	if (NVME_GET(cap, CAP_CMBS))
		mmio_hl_write64(ctrl->regs + NVME_REG_CMBMSC, cpu_to_le64(0x1));

	cmbsz = le32_to_cpu(mmio_read32(ctrl->regs + NVME_REG_CMBSZ));
	cmbloc = le32_to_cpu(mmio_read32(ctrl->regs + NVME_REG_CMBLOC));

	szu = 1 << (12 + 4 * NVME_GET(cmbsz, CMBSZ_SZU));
	len = szu * NVME_GET(cmbsz, CMBSZ_SZ);

	ofst = szu * NVME_GET(cmbloc, CMBLOC_OFST);
	bir = NVME_GET(cmbloc, CMBLOC_BIR);

	printf("cmb bar is %d\n", bir);

	if (vfio_pci_read_config(&ctrl->pci, &v32, sizeof(v32), bar_offset(bir)) < 0)
		err(1, "failed to read pci config");

	*hwaddr = v32 & PCI_BASE_ADDRESS_MEM_MASK;

	if (vfio_pci_read_config(&ctrl->pci, &v32, sizeof(v32), bar_offset(bir + 1)) < 0)
		err(1, "failed to read pci config");

	*hwaddr |= (uint64_t)v32 << 32;

	printf("cmb bar is mapped at physical address 0x%lx\n", *hwaddr);

	/* map the cmb */
	cmb = vfio_pci_map_bar(&ctrl->pci, bir, len, ofst, PROT_READ | PROT_WRITE);
	if (!cmb)
		err(1, "failed to map cmb");

	if (!NVME_GET(cap, CAP_CMBS)) {
		*cba = *hwaddr;
		return cmb;
	}

	/* choose a base address that is guaranteed not to be involved in dma */
	num_iova_ranges = iommu_get_iova_ranges(__iommu_ctx(ctrl), &iova_ranges);
	*cba = ALIGN_UP(iova_ranges[num_iova_ranges - 1].end + 1, 4096);
	printf("assigned cmb base address is 0x%lx\n", *cba);

	/* set the base address and enable the memory space */
	mmio_hl_write64(ctrl->regs + NVME_REG_CMBMSC, cpu_to_le64(*cba | 0x3));

	return cmb;
}

int main(int argc, char **argv)
{
	struct nvme_ctrl src = {}, dst = {};

	uint64_t hwaddr, cba;
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

	cmb = nvme_configure_cmb(&dst, &hwaddr, &cba);

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.cns = NVME_IDENTIFY_CNS_CTRL,

		/* external reference; use the system address */
		.dptr.prp1 = cpu_to_le64(hwaddr),
	};

	if (nvme_admin(&src, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");

	id_ctrl = (struct nvme_id_ctrl __force *)cmb;
	printf("identity controller VER field value is %x\n", id_ctrl->ver);

	return 0;
}
