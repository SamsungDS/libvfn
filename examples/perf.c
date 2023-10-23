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
#include "ccan/likely/likely.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

static char *io_pattern = "read";
static unsigned long nsid, runtime_in_seconds = 10, warmup_in_seconds, update_stats_interval = 1;
static int io_depth = 1, io_qsize = -1;

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_WITH_ARG("-N|--nsid NSID", opt_set_ulongval, opt_show_ulongval, &nsid,
		     "namespace identifier"),
	OPT_WITH_ARG("-t|--runtime SECONDS", opt_set_ulongval, opt_show_ulongval,
		     &runtime_in_seconds, "runtime in seconds"),
	OPT_WITH_ARG("-w|--warmup SECONDS", opt_set_ulongval, opt_show_ulongval,
		     &warmup_in_seconds, "warmup time in seconds"),
	OPT_WITH_ARG("-u|--update-stats-interval SECONDS", opt_set_ulongval, opt_show_ulongval,
		     &update_stats_interval, "update stats interval in seconds"),
	OPT_WITH_ARG("-p|--io-pattern", opt_set_charp, opt_show_charp, &io_pattern, "i/o pattern"),
	OPT_WITH_ARG("-q|--io-depth", opt_set_intval, opt_show_intval, &io_depth, "i/o depth"),
	OPT_WITH_ARG("-n|--io-qsize", opt_set_intval, opt_show_intval, &io_qsize, "i/o queue size"),
	OPT_ENDTABLE,
};

static struct nvme_ctrl ctrl;

static struct nvme_sq *sq;
static struct nvme_cq *cq;

static bool draining;
static bool random_io;
static unsigned int queued;
static uint64_t nsze, slba;

static struct {
	unsigned long completed, completed_quantum;
	uint64_t ttotal, tmin, tmax;
} stats;

struct iod {
	uint64_t tsubmit;
	union nvme_cmd cmd;
};

static void io_issue(struct nvme_rq *rq)
{
	struct iod *iod = rq->opaque;

	if (random_io) {
		slba = rand() % nsze;
	} else {
		if (unlikely(++slba == nsze))
			slba = 0;
	}

	iod->cmd.rw.slba = cpu_to_le64(slba);

	iod->tsubmit = get_ticks();

	nvme_sq_post(rq->sq, &iod->cmd);

	queued++;
}

static void io_complete(struct nvme_rq *rq)
{
	struct iod *iod = rq->opaque;
	uint64_t diff;

	stats.completed_quantum++;
	queued--;

	diff = get_ticks() - iod->tsubmit;
	stats.ttotal += diff;

	if (unlikely(diff < stats.tmin))
		stats.tmin = diff;

	if (unlikely(diff > stats.tmax))
		stats.tmax = diff;

	if (unlikely(draining)) {
		nvme_rq_release(rq);
		return;
	}

	io_issue(rq);
}

static void update_and_print_stats(bool warmup)
{
	float iops, mbps;

	if (!isatty(STDOUT_FILENO))
		goto out;

	iops = (float)stats.completed_quantum / update_stats_interval;
	mbps = iops * 512 / (1024 * 1024);

	printf("%10s iops %10.2f mbps %10.2f\r", warmup ? "(warmup)" : "", iops, mbps);
	fflush(stdout);

out:
	stats.completed += stats.completed_quantum;
	stats.completed_quantum = 0;
}

static int reap(void)
{
	struct nvme_cqe *cqe;
	int reaped = 0;

	do {
		cqe = nvme_cq_get_cqe(cq);
		if (!cqe)
			break;

		reaped++;

		io_complete(__nvme_rq_from_cqe(sq, cqe));
	} while (1);

	if (reaped)
		nvme_cq_update_head(cq);

	return reaped;
}

static void run(void)
{
	uint64_t deadline, update_stats, now = get_ticks();
	uint64_t twarmup, trun, tupdate;
	bool warmup = (warmup_in_seconds > 0);
	float iops, mbps, lavg, lmin, lmax;
	uint64_t iova;
	void *mem;
	unsigned int to_submit = io_depth;

	twarmup = warmup_in_seconds * __vfn_ticks_freq;
	trun = runtime_in_seconds * __vfn_ticks_freq;
	tupdate = update_stats_interval * __vfn_ticks_freq;

	deadline = now + (warmup ? twarmup : trun);
	update_stats = now + tupdate;

	stats.tmin = UINT64_MAX;

	mem = mmap(NULL, io_depth * 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (!mem)
		err(1, "mmap");

	if (iommu_map_vaddr(ctrl.pci.dev.vfio, mem, io_depth * 0x1000, &iova, 0x0))
		err(1, "failed to map");

	do {
		struct nvme_rq *rq;
		struct iod *iod;

		rq = nvme_rq_acquire(sq);
		if (!rq)
			break;

		iod = calloc(1, sizeof(*iod));

		iod->cmd.rw.opcode = nvme_cmd_read;
		iod->cmd.rw.nsid = cpu_to_le32(nsid);
		iod->cmd.rw.dptr.prp1 = cpu_to_le64(iova);

		nvme_rq_prep_cmd(rq, &iod->cmd);

		iova += 0x1000;

		rq->opaque = iod;

		io_issue(rq);
	} while (true && --to_submit > 0);

	nvme_sq_update_tail(sq);

	do {

		while (!reap())
			;

		nvme_sq_update_tail(sq);

		now = get_ticks();

		if (now > update_stats) {
			update_stats = update_stats + tupdate;
			update_and_print_stats(warmup);
		}

		if (now > deadline) {
			if (warmup) {
				warmup = false;

				memset(&stats, 0x0, sizeof(stats));
				stats.tmin = UINT64_MAX;
				deadline = now + trun;

				continue;
			}

			break;
		}
	} while (true);

	update_and_print_stats(false);

	iops = (float)stats.completed / runtime_in_seconds;
	mbps = iops * 512 / (1024 * 1024);
	lmin = (float)stats.tmin * 1000 * 1000 / __vfn_ticks_freq;
	lmax = (float)stats.tmax * 1000 * 1000 / __vfn_ticks_freq;
	lavg = ((float)stats.ttotal * 1000 * 1000 / __vfn_ticks_freq) / stats.completed;

	printf("%10s %10s %10s %10s %10s\n", "iops", "mbps", "lavg", "lmin", "lmax");
	printf("%10.2f %10.2f %10.2f %10.2f %10.2f\n", iops, mbps, lavg, lmin, lmax);

	draining = true;

	nvme_sq_update_tail(sq);

	do {
		reap();
	} while (queued);
}

int main(int argc, char **argv)
{
	void *vaddr;
	ssize_t len;
	struct nvme_id_ns *id_ns;
	union nvme_cmd cmd;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	if (!nsid || nsid > (NVME_NSID_ALL - 1))
		opt_usage_exit_fail("missing or invalid --nsid parameter");

	if (strstarts(io_pattern, "rand")) {
		random_io = true;
		io_pattern = &io_pattern[4];
	}

	if (!streq(io_pattern, "read"))
		errx(1, "unsupported i/o pattern");

	if (io_depth < 1)
		errx(1, "invalid io-depth");

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	len = pgmap(&vaddr, NVME_IDENTIFY_DATA_SIZE);
	if (len < 0)
		err(1, "could not allocate aligned memory");

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.nsid = cpu_to_le32(nsid),
		.cns = NVME_IDENTIFY_CNS_NS,
	};

	if (nvme_admin(&ctrl, &cmd, vaddr, len, NULL))
		err(1, "nvme_admin");

	id_ns = (struct nvme_id_ns *)vaddr;

	nsze = le64_to_cpu((__force leint64_t)(id_ns->nsze));

	if (io_qsize < 0)
		io_qsize = ctrl.config.mqes + 1;

	if (io_depth > io_qsize - 1)
		errx(1, "io-depth must be less than io-qsize");

	if (nvme_create_ioqpair(&ctrl, 1, io_qsize, -1, 0x0))
		err(1, "nvme_create_ioqpair");

	sq = &ctrl.sq[1];
	cq = &ctrl.cq[1];

	run();

	return 0;
}
