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
#include "ccan/time/time.h"
#include "ccan/str/str.h"

#include "common.h"

static char *io_pattern = "read";
static unsigned long nsid, runtime_in_seconds = 10, warmup_in_seconds, update_stats_interval = 1;

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
	OPT_ENDTABLE,
};

static struct nvme_ctrl ctrl;

static const struct nvme_ctrl_opts ctrl_opts = {
	.nsqr = 63,
	.ncqr = 63,
};

static struct nvme_sq *sq;
static struct nvme_cq *cq;

static bool draining;
static bool random_io;
static unsigned int qsize = 64;
static unsigned int queued;
static uint64_t nsze, slba;

static struct {
	unsigned long completed, completed_quantum;
	struct timerel ttotal, tmin, tmax;
} stats;

struct iod {
	struct timemono tsubmit;
	union nvme_cmd cmd;
};

static void io_issue(struct nvme_rq *rq)
{
	struct iod *iod = rq->opaque;

	if (random_io) {
		slba = rand() % nsze;
	} else {
		if (++slba == nsze)
			slba = 0;
	}

	iod->cmd.rw.slba = cpu_to_le64(slba);

	iod->tsubmit = time_mono();

	nvme_sq_post(rq->sq, &iod->cmd);

	queued++;
}

static void io_complete(struct nvme_rq *rq)
{
	struct iod *iod = rq->opaque;
	struct timerel diff;

	stats.completed_quantum++;
	queued--;

	diff = timemono_since(iod->tsubmit);
	stats.ttotal = timerel_add(stats.ttotal, diff);

	if (unlikely(time_greater(stats.tmin, diff)))
		stats.tmin = diff;

	if (unlikely(time_greater(diff, stats.tmax)))
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
	mbps = iops * 4096 / (1024 * 1024);

	printf("%10s iops %10.2f mbps %10.2f\r", warmup ? "(warmup)" : "", iops, mbps);
	fflush(stdout);

out:
	stats.completed += stats.completed_quantum;
	stats.completed_quantum = 0;
}

static void reap(void)
{
	struct nvme_cqe *cqe;
	unsigned int reaped = 0;

	do {
		cqe = nvme_cq_get_cqe(cq);
		if (!cqe)
			break;

		reaped++;

		io_complete(__nvme_rq_from_cqe(sq, cqe));
	} while (1);

	if (likely(reaped))
		nvme_cq_update_head(cq);
}

static void run(void)
{
	struct timeabs deadline, update_stats, now = time_now();
	struct timerel twarmup, trun, tupdate;
	bool warmup = (warmup_in_seconds > 0);
	float iops, mbps, lavg, lmin, lmax;
	uint64_t iova;
	void *mem;

	twarmup = time_from_sec(warmup_in_seconds);
	trun = time_from_sec(runtime_in_seconds);
	tupdate = time_from_sec(update_stats_interval);

	deadline = timeabs_add(now, warmup ? twarmup : trun);
	update_stats = timeabs_add(now, tupdate);

	stats.tmin = time_from_sec(INT_MAX);

	mem = mmap(NULL, 64 * 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (!mem)
		err(1, "mmap");

	if (vfio_map_vaddr(ctrl.pci.dev.vfio, mem, 64 * 0x1000, &iova))
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
	} while (true);

	do {
		nvme_sq_update_tail(sq);

		reap();

		now = time_now();

		if (time_after(now, update_stats)) {
			update_stats = timeabs_add(update_stats, tupdate);
			update_and_print_stats(warmup);
		}

		if (time_after(now, deadline)) {
			if (warmup) {
				warmup = false;

				memset(&stats, 0x0, sizeof(stats));
				stats.tmin = time_from_sec(INT_MAX);
				deadline = timeabs_add(now, trun);

				continue;
			}

			break;
		}
	} while (true);

	update_and_print_stats(false);

	iops = (float)stats.completed / runtime_in_seconds;
	mbps = iops * 4096 / (1024 * 1024);
	lmin = (float)time_to_nsec(stats.tmin) / 1000;
	lmax = (float)time_to_nsec(stats.tmax) / 1000;
	lavg = ((float)time_to_nsec(stats.ttotal) / stats.completed) / 1000;

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

	if (nvme_init(&ctrl, bdf, &ctrl_opts))
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

	if (nvme_create_ioqpair(&ctrl, 1, qsize, 0x0))
		err(1, "nvme_create_ioqpair");

	sq = &ctrl.sq[1];
	cq = &ctrl.cq[1];

	run();

	return 0;
}
