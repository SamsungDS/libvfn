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

#include <nvme/types.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_ENDTABLE,
};

static struct nvme_ctrl ctrl;

int main(int argc, char **argv)
{
	void *regs;

	uint64_t cap;
	uint32_t vs;
	uint32_t intms, intmc;
	uint32_t cc, csts;
	uint32_t nssr;
	uint32_t aqa;
	uint64_t asq, acq;
	uint32_t cmbloc, cmbsz;
	uint32_t bpinfo, bprsel;
	uint64_t bpmbl;
	uint64_t cmbmsc;
	uint32_t cmbsts;
	uint32_t pmrcap, pmrctl, pmrsts, pmrebs, pmrswtp;
	uint64_t pmrmsc;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	regs = ctrl.regs;

	cap = le64_to_cpu(mmio_read64(regs, NVME_REG_CAP));
	vs = le32_to_cpu(mmio_read32(regs, NVME_REG_VS));
	intms = le32_to_cpu(mmio_read32(regs, NVME_REG_INTMS));
	intmc = le32_to_cpu(mmio_read32(regs, NVME_REG_INTMC));
	cc = le32_to_cpu(mmio_read32(regs, NVME_REG_CC));
	csts = le32_to_cpu(mmio_read32(regs, NVME_REG_CSTS));
	nssr = le32_to_cpu(mmio_read32(regs, NVME_REG_NSSR));
	aqa = le32_to_cpu(mmio_read32(regs, NVME_REG_AQA));
	asq = le64_to_cpu(mmio_read64(regs, NVME_REG_ASQ));
	acq = le64_to_cpu(mmio_read64(regs, NVME_REG_ACQ));
	cmbloc = le32_to_cpu(mmio_read32(regs, NVME_REG_CMBLOC));
	cmbsz = le32_to_cpu(mmio_read32(regs, NVME_REG_CMBSZ));
	bpinfo = le32_to_cpu(mmio_read32(regs, NVME_REG_BPINFO));
	bprsel = le32_to_cpu(mmio_read32(regs, NVME_REG_BPRSEL));
	bpmbl = le64_to_cpu(mmio_read64(regs, NVME_REG_BPMBL));
	cmbmsc = le64_to_cpu(mmio_read64(regs, NVME_REG_CMBMSC));
	cmbsts = le32_to_cpu(mmio_read32(regs, NVME_REG_CMBSTS));
	pmrcap = le32_to_cpu(mmio_read32(regs, NVME_REG_PMRCAP));
	pmrctl = le32_to_cpu(mmio_read32(regs, NVME_REG_PMRCTL));
	pmrsts = le32_to_cpu(mmio_read32(regs, NVME_REG_PMRSTS));
	pmrebs = le32_to_cpu(mmio_read32(regs, NVME_REG_PMREBS));
	pmrswtp = le32_to_cpu(mmio_read32(regs, NVME_REG_PMRSWTP));
	pmrmsc = le32_to_cpu(mmio_read32(regs, NVME_REG_PMRMSCL)) |
		(uint64_t)le32_to_cpu(mmio_read32(regs, NVME_REG_PMRMSCU)) << 32;

	printf("%-16s %lx\n",  "CAP", cap);
	printf("%-16s %lx\n",  "CAP.MQES", NVME_CAP_MQES(cap));
	printf("%-16s %lx\n",  "CAP.CQRS", NVME_CAP_CQR(cap));
	printf("%-16s %lx\n",  "CAP.AMS", NVME_CAP_AMS(cap));
	printf("%-16s %lx\n",  "CAP.TO", NVME_CAP_TO(cap));
	printf("%-16s %lx\n",  "CAP.DSTRD", NVME_CAP_DSTRD(cap));
	printf("%-16s %lx\n",  "CAP.NSSRC", NVME_CAP_NSSRC(cap));
	printf("%-16s %lx\n",  "CAP.CSS", NVME_CAP_CSS(cap));
	printf("%-16s %lx\n",  "CAP.BPS", NVME_CAP_BPS(cap));
	printf("%-16s %lx\n",  "CAP.MPSMIN", NVME_CAP_MPSMIN(cap));
	printf("%-16s %lx\n",  "CAP.MPSMAX", NVME_CAP_MPSMAX(cap));
	printf("%-16s %lx\n",  "CAP.CMBS", NVME_CAP_CMBS(cap));
	printf("%-16s %lx\n",  "CAP.PMRS", NVME_CAP_PMRS(cap));

	printf("%-16s %x\n",   "VS", vs);
	printf("%-16s %x\n",   "VS.MJR", NVME_VS_TER(vs));
	printf("%-16s %x\n",   "VS.MNR", NVME_VS_MNR(vs));
	printf("%-16s %x\n",   "VS.TER", NVME_VS_MJR(vs));

	printf("%-16s %x\n",   "INTMS", intms);
	printf("%-16s %x\n",   "INTMC", intmc);

	printf("%-16s %x\n",   "CC", cc);
	printf("%-16s %x\n",   "CC.EN", NVME_CC_EN(cc));
	printf("%-16s %x\n",   "CC.CSS", NVME_CC_CSS(cc));
	printf("%-16s %x\n",   "CC.MPS", NVME_CC_MPS(cc));
	printf("%-16s %x\n",   "CC.AMS", NVME_CC_AMS(cc));
	printf("%-16s %x\n",   "CC.SHN", NVME_CC_SHN(cc));
	printf("%-16s %x\n",   "CC.IOSQES", NVME_CC_IOSQES(cc));
	printf("%-16s %x\n",   "CC.IOCQES", NVME_CC_IOCQES(cc));

	printf("%-16s %x\n",   "CSTS", csts);
	printf("%-16s %x\n",   "CSTS.RDY", NVME_CSTS_RDY(csts));
	printf("%-16s %x\n",   "CSTS.CFS", NVME_CSTS_CFS(csts));
	printf("%-16s %x\n",   "CSTS.SHST", NVME_CSTS_SHST(csts));
	printf("%-16s %x\n",   "CSTS.NSSRO", NVME_CSTS_NSSRO(csts));
	printf("%-16s %x\n",   "CSTS.PP", NVME_CSTS_PP(csts));

	printf("%-16s %x\n",   "NSSR", nssr);

	printf("%-16s %x\n",   "AQA", aqa);
	printf("%-16s %x\n",   "AQA.ASQS", NVME_AQA_ASQS(aqa));
	printf("%-16s %x\n",   "AQA.ACQS", NVME_AQA_ACQS(aqa));

	printf("%-16s %lx\n",  "ASQ", asq);
	printf("%-16s %lx\n",  "ACQ", acq);

	printf("%-16s %x\n",   "CMBLOC",  cmbloc);
	printf("%-16s %x\n",   "CMBLOC.BIR", NVME_CMBLOC_BIR(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.CQMMS", NVME_CMBLOC_CQMMS(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.CQPDS", NVME_CMBLOC_CQPDS(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.CDPLMS", NVME_CMBLOC_CDPLMS(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.CDPCILS", NVME_CMBLOC_CDPCILS(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.CDMMMS", NVME_CMBLOC_CDMMMS(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.CQDA", NVME_CMBLOC_CQDA(cmbloc));
	printf("%-16s %x\n",   "CMBLOC.OFST", NVME_CMBLOC_OFST(cmbloc));

	printf("%-16s %x\n",   "CMBSZ", cmbsz);
	printf("%-16s %x\n",   "CMBSZ.SQS", NVME_CMBSZ_SQS(cmbsz));
	printf("%-16s %x\n",   "CMBSZ.CQS", NVME_CMBSZ_CQS(cmbsz));
	printf("%-16s %x\n",   "CMBSZ.LISTS", NVME_CMBSZ_LISTS(cmbsz));
	printf("%-16s %x\n",   "CMBSZ.RDS", NVME_CMBSZ_RDS(cmbsz));
	printf("%-16s %x\n",   "CMBSZ.WDS", NVME_CMBSZ_WDS(cmbsz));
	printf("%-16s %x\n",   "CMBSZ.SZU", NVME_CMBSZ_SZU(cmbsz));
	printf("%-16s %x\n",   "CMBSZ.SZ", NVME_CMBSZ_SZ(cmbsz));

	printf("%-16s %x\n",   "BPINFO", bpinfo);
	printf("%-16s %x\n",   "BPINFO.BPSZ", NVME_BPINFO_BPSZ(bpinfo));
	printf("%-16s %x\n",   "BPINFO.BRS", NVME_BPINFO_BRS(bpinfo));
	printf("%-16s %x\n",   "BPINFO.ABPID", NVME_BPINFO_ABPID(bpinfo));

	printf("%-16s %x\n",   "BPRSEL", bprsel);
	printf("%-16s %x\n",   "BPRSEL.BPRSZ", NVME_BPRSEL_BPRSZ(bprsel));
	printf("%-16s %x\n",   "BPRSEL.BPROF", NVME_BPRSEL_BPROF(bprsel));
	printf("%-16s %x\n",   "BPRSEL.BPID", NVME_BPRSEL_BPID(bprsel));

	printf("%-16s %lx\n",  "BPMBL", bpmbl);

	printf("%-16s %lx\n",  "CMBMSC", cmbmsc);
	printf("%-16s %lx\n",  "CMBMSC.CRE", NVME_CMBMSC_CRE(cmbmsc));
	printf("%-16s %lx\n",  "CMBMSC.CMSE", NVME_CMBMSC_CMSE(cmbmsc));
	printf("%-16s %llx\n", "CMBMSC.CBA", NVME_CMBMSC_CBA(cmbmsc));

	printf("%-16s %x\n",   "CMBSTS", cmbsts);
	printf("%-16s %x\n",   "CMBSTS.CBAI", NVME_CMBSTS_CBAI(cmbsts));

	printf("%-16s %x\n",   "PMRCAP", pmrcap);
	printf("%-16s %x\n",   "PMRCAP.RDS", NVME_PMRCAP_RDS(pmrcap));
	printf("%-16s %x\n",   "PMRCAP.WDS", NVME_PMRCAP_WDS(pmrcap));
	printf("%-16s %x\n",   "PMRCAP.BIR", NVME_PMRCAP_BIR(pmrcap));
	printf("%-16s %x\n",   "PMRCAP.PMRTU", NVME_PMRCAP_PMRTU(pmrcap));
	printf("%-16s %x\n",   "PMRCAP.PMRWMB", NVME_PMRCAP_PMRWMB(pmrcap));
	printf("%-16s %x\n",   "PMRCAP.PMRTO", NVME_PMRCAP_PMRTO(pmrcap));
	printf("%-16s %x\n",   "PMRCAP.CMSS", NVME_PMRCAP_CMSS(pmrcap));

	printf("%-16s %x\n",   "PMRCTL", pmrctl);
	printf("%-16s %x\n",   "PMRCTL.EN", NVME_PMRCTL_EN(pmrctl));

	printf("%-16s %x\n",   "PMRSTS", pmrsts);
	printf("%-16s %x\n",   "PMRSTS.ERR", NVME_PMRSTS_ERR(pmrsts));
	printf("%-16s %x\n",   "PMRSTS.NRDY", NVME_PMRSTS_NRDY(pmrsts));
	printf("%-16s %x\n",   "PMRSTS.HSTS", NVME_PMRSTS_HSTS(pmrsts));
	printf("%-16s %x\n",   "PMRSTS.CBAI", NVME_PMRSTS_CBAI(pmrsts));

	printf("%-16s %x\n",   "PMREBS", pmrebs);
	printf("%-16s %x\n",   "PMREBS.PMRSZU", NVME_PMREBS_PMRSZU(pmrebs));
	printf("%-16s %x\n",   "PMREBS.RBB", NVME_PMREBS_RBB(pmrebs));
	printf("%-16s %x\n",   "PMREBS.PMRWBZ", NVME_PMREBS_PMRWBZ(pmrebs));

	printf("%-16s %x\n",   "PMRSWTP", pmrswtp);
	printf("%-16s %x\n",   "PMRSWTP.PMRSWTU", NVME_PMRSWTP_PMRSWTU(pmrswtp));
	printf("%-16s %x\n",   "PMRSWTP.PMRSWTV", NVME_PMRSWTP_PMRSWTV(pmrswtp));

	printf("%-16s %lx\n",  "PMRMSC", pmrmsc);
	printf("%-16s %lx\n",  "PMRMSC.CMSE", NVME_PMRMSC_CMSE(pmrmsc));
	printf("%-16s %llx\n", "PMRMSC.CBA", NVME_PMRMSC_CBA(pmrmsc));

	nvme_close(&ctrl);

	return 0;

}
