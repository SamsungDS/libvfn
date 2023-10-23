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

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_ENDTABLE,
};

static inline off_t bar_offset(int bar)
{
	return PCI_BASE_ADDRESS_0 + bar * 4;
}

int main(int argc, char **argv)
{
	struct nvme_ctrl ctrl = {};

	uint64_t cap;
	uint32_t vs, cmbloc, cmbsz, v32;

	uint64_t szu, ofst, cba;
	uint64_t hwaddr;
	size_t len;
	int bir;
	void *cmb;

	union nvme_cmd cmd = {};
	struct nvme_id_ctrl *id_ctrl;
	struct iova_range *iova_ranges;
	int num_iova_ranges;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	vs = le32_to_cpu(mmio_read32(ctrl.regs + NVME_REG_VS));
	if (vs < NVME_VERSION(1, 4, 0))
		errx(1, "controller must be compliant with at least nvme v1.4.0");

	cap = le64_to_cpu(mmio_read64(ctrl.regs + NVME_REG_CAP));
	if (!NVME_GET(cap, CAP_CMBS))
		errx(1, "controller memory buffer not supported (cap 0x%lx)", cap);

	/* enable the CMBSZ and CMBLOC registers */
	mmio_hl_write64(ctrl.regs + NVME_REG_CMBMSC, cpu_to_le64(0x1));

	cmbsz = le32_to_cpu(mmio_read32(ctrl.regs + NVME_REG_CMBSZ));
	cmbloc = le32_to_cpu(mmio_read32(ctrl.regs + NVME_REG_CMBLOC));

	szu = 1 << (12 + 4 * NVME_GET(cmbsz, CMBSZ_SZU));
	len = szu * NVME_GET(cmbsz, CMBSZ_SZ);

	ofst = szu * NVME_GET(cmbloc, CMBLOC_OFST);
	bir = NVME_GET(cmbloc, CMBLOC_BIR);

	printf("cmb bar is %d\n", bir);

	if (vfio_pci_read_config(&ctrl.pci, &v32, sizeof(v32), bar_offset(bir)) < 0)
		err(1, "failed to read pci config");

	hwaddr = v32 & PCI_BASE_ADDRESS_MEM_MASK;

	if (vfio_pci_read_config(&ctrl.pci, &v32, sizeof(v32), bar_offset(bir + 1)) < 0)
		err(1, "failed to read pci config");

	hwaddr |= (uint64_t)v32 << 32;

	printf("cmb bar is mapped at physical address 0x%lx\n", hwaddr);

	/* map the cmb */
	cmb = vfio_pci_map_bar(&ctrl.pci, bir, len, ofst, PROT_READ | PROT_WRITE);
	if (!cmb)
		err(1, "failed to map cmb");

	/* choose a base address that is guaranteed not to be involved in dma */
	num_iova_ranges = iommu_get_iova_ranges(ctrl.pci.dev.vfio, &iova_ranges);
	cba = ALIGN_UP(iova_ranges[num_iova_ranges - 1].end + 1, 4096);
	printf("assigned cmb base address is 0x%lx\n", cba);

	/* set the base address and enable the memory space */
	mmio_hl_write64(ctrl.regs + NVME_REG_CMBMSC, cpu_to_le64(cba | 0x3));

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_identify,
		.dptr.prp1 = cpu_to_le64(cba),
	};

	cmd.identify.cns = NVME_IDENTIFY_CNS_CTRL;

	if (nvme_admin(&ctrl, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");

	id_ctrl = (struct nvme_id_ctrl __force *)cmb;
	printf("identify controller VER field value is %x\n", id_ctrl->ver);

	return 0;

}
