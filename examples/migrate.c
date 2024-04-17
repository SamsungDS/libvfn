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

#include <vfn/pci.h>
#include <vfn/nvme.h>

#include <nvme/types.h>

#include "ccan/array_size/array_size.h"
#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

#define __foreach(x, arr) \
	for (typeof(arr[0]) *x = arr; x < &arr[ARRAY_SIZE(arr)]; x++)

struct __packed nvme_mqle0 {
    leint32_t nsid;
    leint32_t nlb;
    leint64_t slba;
    uint8_t   rsvd16[15];
    uint8_t   lbamqa;
};
__static_assert(sizeof(struct nvme_mqle0) == 32);

static void print_mqle0(struct nvme_mqle0 *mqle)
{
	printf("  lbamqa   0x%"PRIx8"\n", mqle->lbamqa);
	printf("    phase  %"PRIx8"\n", mqle->lbamqa & 0x1);
	printf("    esa    0x%"PRIx8, (mqle->lbamqa >> 1) & 0x7);
	switch ((mqle->lbamqa >> 1) & 0x7) {
	case 0x0:
		printf("\n");
		break;
	case 0x1:
		printf(" (first)\n");
		break;
	case 0x2:
		printf(" (last; logging stopped)\n");
		break;
	case 0x3:
		printf(" (last; suspended)\n");
		break;
	case 0x7:
		printf(" (last; queue full)\n");
		break;
	default:
		printf(" (unknown)\n");
		break;
	}
	printf("    lbacir 0x%"PRIx8, (mqle->lbamqa >> 6) & 0x3);
	switch ((mqle->lbamqa >> 6) & 0x3) {
	case 0x0:
		printf(" (lba change)\n");
		break;
	case 0x1:
		printf(" (all lba changed)\n");
		break;
	case 0x2:
		printf(" (no lba change)\n");
		break;
	default:
		printf(" (unknown)\n");
		break;
	}
	if ((mqle->lbamqa >> 6 & 0x3) == 0) {
		printf("      nsid %"PRIu32"\n", le32_to_cpu(mqle->nsid));
		printf("      slba 0x%"PRIx64"\n", le64_to_cpu(mqle->slba));
		printf("      nlb  %"PRIu32"\n", le32_to_cpu(mqle->nlb));
	}
}

struct __packed nvme_iosq_state {
	leint64_t prp1;
	leint16_t size;
	leint16_t qid;
	leint16_t cqid;
	leint16_t attrs;
	leint16_t head;
	leint16_t tail;
	uint8_t   rsvd20[4];
};
__static_assert(sizeof(struct nvme_iosq_state) == 24);

struct __packed nvme_iocq_state {
	leint64_t prp1;
	leint16_t size;
	leint16_t qid;
	leint16_t head;
	leint16_t tail;
	leint32_t attrs;
	uint8_t   rsvd20[4];
};
__static_assert(sizeof(struct nvme_iocq_state) == 24);

union nvme_ioq_state {
	struct nvme_iosq_state sqs;
	struct nvme_iocq_state cqs;
};
__static_assert(sizeof(union nvme_ioq_state) == 24);

struct nvme_ctrl_state {
	leint16_t ver;
	leint16_t niosq;
	leint16_t niocq;
	uint8_t   rsvd6[2];
	union nvme_ioq_state qss[];
};
__static_assert(sizeof(struct nvme_ctrl_state) == 8);

struct nvme_ctrl_state_container {
	leint16_t ver;
	uint8_t   csattr;
	uint8_t   rsvd3[13];
	leint64_t nvmecss[2];
	uint8_t   vss[16];
	struct nvme_ctrl_state nvmecs;
};
__static_assert(sizeof(struct nvme_ctrl_state_container) == 56);

enum nvme_admin_migration {
	nvme_admin_track_send		= 0x3d,
	nvme_admin_migration_send	= 0x41,
	nvme_admin_migration_receive	= 0x42,
	nvme_admin_cdq			= 0x45,
};

enum nvme_fid_migration {
	nvme_fid_cdq			= 0x21,
};

enum nvme_track_send_lact {
	NVME_TRACK_SEND_LACT_STOP	= 0x0,
	NVME_TRACK_SEND_LACT_START	= 0x1,
};

struct ctrl_info {
	const char *name;
	char *bdf;
	uint16_t cntlid;

	struct nvme_ctrl ctrl;
	struct nvme_id_ctrl id;
};

static struct ctrl_info parent_s = {.name = "source parent"};
static struct ctrl_info child_s = {.name = "source child"};
static struct ctrl_info parent_d = {.name = "destination parent"};
static struct ctrl_info child_d = {.name = "destination child"};

static struct opt_table opts[] = {
	OPT_WITHOUT_ARG("-h|--help", opt_set_bool, &show_usage, "show usage"),
	OPT_WITH_ARG("-p|--source-parent BDF", opt_set_charp, opt_show_charp, &parent_s.bdf,
		     "source parent pci device"),
	OPT_WITH_ARG("-c|--source-child BDF", opt_set_charp, opt_show_charp, &child_s.bdf,
		     "source child pci device"),
	OPT_WITH_ARG("-P|--dest-parent BDF", opt_set_charp, opt_show_charp, &parent_d.bdf,
		     "destination parent pci device"),
	OPT_WITH_ARG("-C|--dest-child BDF", opt_set_charp, opt_show_charp, &child_d.bdf,
		     "destination child pci device"),

	OPT_ENDTABLE,
};

static const struct nvme_ctrl_opts ctrl_opts = {
	.nsqr = 63,
	.ncqr = 63,
};

#define NVME_CDQ_MQLE0_SIZE 5

struct nvme_cdq {
	void *vaddr;
	uint64_t iova;
	uint32_t head;
	int entry_size;
	int phase;
	uint32_t qsize;
	uint16_t cdqid;
};

static inline void *nvme_cdq_head(struct nvme_cdq *cdq)
{
	return cdq->vaddr + (cdq->head << cdq->entry_size);
}

static inline struct nvme_mqle0 *nvme_cdq_get_mqle0(struct nvme_cdq *cdq)
{
	struct nvme_mqle0 *mqle = nvme_cdq_head(cdq);

	if ((le16_to_cpu(LOAD(mqle->lbamqa)) & 0x1) == cdq->phase)
		return NULL;

	/* prevent load/load reordering between attrs and the rest of the entry */
	dma_rmb();

	if (unlikely(++cdq->head == cdq->qsize)) {
		cdq->head = 0;
		cdq->phase ^= 0x1;
	}

	return mqle;
}

static void nvme_cdq_get_mqle0s(struct nvme_cdq *cdq, struct nvme_mqle0 *mqles, int n)
{
	struct nvme_mqle0 *mqle;

	do {
		mqle = nvme_cdq_get_mqle0(cdq);
		if (!mqle)
			continue;

		n--;

		if (mqles)
			memcpy(mqles++, mqle, sizeof(*mqle));
	} while (n > 0);
}

static void nvme_cdq_update_head(struct nvme_ctrl *ctrl, struct nvme_cdq *cdq)
{
	union nvme_cmd cmd;

	printf("nvme_cdq_update_head cdqid %"PRIu16" head %"PRIu32"\n", cdq->cdqid, cdq->head);

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_set_features,
		.cdw10 = cpu_to_le32(nvme_fid_cdq),
		.cdw11 = cpu_to_le32(cdq->cdqid),
		.cdw12 = cpu_to_le32(cdq->head),
	};

	if (nvme_admin(ctrl, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");
}

static void nvme_track_send(struct nvme_ctrl *ctrl, struct nvme_cdq *cdq, uint8_t lact)
{
	union nvme_cmd cmd;

	printf("nvme_track_send cdqid %"PRIu16" lact 0x%"PRIx8"\n", cdq->cdqid, lact);

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_track_send,

		/* logging action */
		.cdw10 = cpu_to_le32(lact << 16),

		.cdw11 = cpu_to_le32(cdq->cdqid),
	};

	if (nvme_admin(ctrl, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");
}

static void migrate_lbas(struct nvme_ctrl *s, struct nvme_ctrl *d, struct nvme_mqle0 *mqle)
{
	ssize_t len;
	void *buf;

	union nvme_cmd cmd;

	printf("migrate_lbas nsid %d slba 0x%"PRIx64" nlb %"PRIu32"\n", le32_to_cpu(mqle->nsid),
	       le64_to_cpu(mqle->slba), le32_to_cpu(mqle->nlb));

	len = pgmapn(&buf, le32_to_cpu(mqle->nlb) + 1, 4096);
	if (len < 0)
		err(1, "could not allocate aligned memory");


	cmd.rw = (struct nvme_cmd_rw) {
		.opcode = nvme_cmd_read,
		.nsid = mqle->nsid,
		.slba = mqle->slba,
		.nlb = mqle->nlb,
	};

	if (nvme_sync(s, &s->sq[1], &cmd, buf, len, NULL))
		err(1, "nvme_sync");

	/* swap to write */
	cmd.rw.opcode = nvme_cmd_write;

	if (nvme_sync(d, &d->sq[1], &cmd, buf, len, NULL))
		err(1, "nvme_sync");
}

struct buffer {
	void *vaddr;
	ssize_t len;
	uint64_t iova;
};

#define NVME_ADMIN_VIRT_MGMT 0x1c

static int nvme_virt_mgmt(struct nvme_ctrl *ctrl, uint16_t cntlid, enum nvme_virt_mgmt_rt rt,
		   enum nvme_virt_mgmt_act act, uint16_t nr)
{
	union nvme_cmd cmd = {
		.opcode = NVME_ADMIN_VIRT_MGMT,

		.cdw10 = cpu_to_le32(cntlid << 16 | rt << 8 | act),
		.cdw11 = cpu_to_le32(nr),
	};

	return nvme_admin(ctrl, &cmd, NULL, 0, NULL);
}

static int init_ctrl_info(struct ctrl_info *info, const struct nvme_ctrl_opts *opts)
{
	union nvme_cmd cmd;
	struct buffer buffer;

	buffer.len = pgmap(&buffer.vaddr, NVME_IDENTIFY_DATA_SIZE);
	if (buffer.len < 0)
		return -1;

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.cns = NVME_IDENTIFY_CNS_CTRL,
	};

	if (!info->bdf || streq(info->bdf, "")) {
		errno = EINVAL;
		return -1;
	}

	if (nvme_init(&info->ctrl, info->bdf, opts))
		return -1;

	if (nvme_admin(&info->ctrl, &cmd, buffer.vaddr, buffer.len, NULL))
		return -1;

	memcpy(&info->id, buffer.vaddr, sizeof(struct nvme_id_ctrl));

	info->cntlid = le16_to_cpu((leint16_t __force)info->id.cntlid);

	return 0;
}

enum setup_flags {
	SETUP_F_SOURCE			= 1 << 0,
	SETUP_F_SOURCE_IO_PARENT	= 1 << 1,
	SETUP_F_SOURCE_IO_CHILD		= 1 << 2,
	SETUP_F_DESTINATION		= 1 << 3,
	SETUP_F_DESTINATION_IO_PARENT	= 1 << 4,
	SETUP_F_DESTINATION_IO_CHILD	= 1 << 5,
};

#define SETUP_F_SOURCE_IO (SETUP_F_SOURCE_IO_PARENT | SETUP_F_SOURCE_IO_CHILD)
#define SETUP_F_DESTINATION_IO (SETUP_F_DESTINATION_IO_PARENT | SETUP_F_DESTINATION_IO_CHILD)
#define SETUP_F_ALL (SETUP_F_SOURCE | SETUP_F_SOURCE_IO | SETUP_F_DESTINATION | SETUP_F_DESTINATION_IO)

static void setup(unsigned long flags)
{
	if (flags & ~SETUP_F_ALL) {
		errno = EINVAL;
		err(1, "invalid setup flags");
	}

	if (flags & SETUP_F_SOURCE) {
		if (init_ctrl_info(&parent_s, &ctrl_opts))
			err(1, "could not initialize %s controller", parent_s.name);

		if (pci_is_vf(child_s.bdf)) {
			uint16_t scid;
			int vfnum;

			vfnum = pci_vf_get_vfnum(child_s.bdf);
			if (vfnum < 0)
				err(1, "pci_vf_get_vfnum");

			if (vfio_pci_open(&child_s.ctrl.pci, child_s.bdf))
				err(1, "vfio_pci_open");

			if (nvme_get_vf_cntlid(&parent_s.ctrl, vfnum, &scid))
				err(1, "nvme_get_vf_cntlid");

			if (nvme_virt_mgmt(&parent_s.ctrl, scid, 0, NVME_VIRT_MGMT_ACT_OFFLINE_SEC_CTRL, 0))
				err(1, "could not offline secondary controller");

			if (nvme_vm_assign_max_flexible(&parent_s.ctrl, scid))
				err(1, "could not assign resources");

			if (vfio_reset(&child_s.ctrl.pci.dev))
				err(1, "vfio_reset");

			if (nvme_virt_mgmt(&parent_s.ctrl, scid, 0, NVME_VIRT_MGMT_ACT_ONLINE_SEC_CTRL, 0))
				err(1, "could not online secondary controller");
		}

		if (init_ctrl_info(&child_s, &ctrl_opts))
			err(1, "could not initialize %s controller", child_s.name);

		if (flags & SETUP_F_SOURCE_IO_PARENT)
			if (nvme_create_ioqpair(&parent_s.ctrl, 1, 64, -1, 0x0))
				err(1, "could not create io queue pair on %s", parent_s.name);

		if (flags & SETUP_F_SOURCE_IO_CHILD)
			if (nvme_create_ioqpair(&child_s.ctrl, 1, 64, -1, 0x0))
				err(1, "could not create io queue pair on %s", child_s.name);
	}

	if (flags & SETUP_F_DESTINATION) {
		if (init_ctrl_info(&parent_d, &ctrl_opts))
			err(1, "could not initialize %s controller", parent_d.name);

		if (pci_is_vf(child_d.bdf)) {
			uint16_t scid;
			int vfnum;

			vfnum = pci_vf_get_vfnum(child_d.bdf);
			if (vfnum < 0)
				err(1, "pci_vf_get_vfnum");

			if (vfio_pci_open(&child_d.ctrl.pci, child_d.bdf))
				err(1, "vfio_pci_open");

			if (nvme_get_vf_cntlid(&parent_d.ctrl, vfnum, &scid))
				err(1, "nvme_get_vf_cntlid");

			if (nvme_virt_mgmt(&parent_d.ctrl, scid, 0, NVME_VIRT_MGMT_ACT_OFFLINE_SEC_CTRL, 0))
				err(1, "could not offline secondary controller");

			if (nvme_vm_assign_max_flexible(&parent_d.ctrl, scid))
				err(1, "could not assign resources");

			if (vfio_reset(&child_d.ctrl.pci.dev))
				err(1, "vfio_reset");

			if (nvme_virt_mgmt(&parent_d.ctrl, scid, 0, NVME_VIRT_MGMT_ACT_ONLINE_SEC_CTRL, 0))
				err(1, "could not online secondary controller");
		}

		if (init_ctrl_info(&child_d, &ctrl_opts))
			err(1, "could not initialize %s controller", child_d.name);

		if (flags & SETUP_F_DESTINATION_IO_PARENT)
			if (nvme_create_ioqpair(&parent_d.ctrl, 1, 64, -1, 0x0))
				err(1, "could not create io queue pair on %s", parent_d.name);

		if (flags & SETUP_F_DESTINATION_IO_CHILD)
			if (nvme_create_ioqpair(&child_d.ctrl, 1, 64, -1, 0x0))
				err(1, "could not create io queue pair on %s", child_d.name);
	}

	return;
}

int main(int argc, char **argv)
{
	struct buffer payload, scratch;
	ssize_t cdqlen;
	size_t nvmecss;
	uint64_t guard;
	struct nvme_rq *rq;

	union nvme_cmd cmd = {};
	struct nvme_cqe cqe;
	struct nvme_cdq cdq;
	struct nvme_mqle0 mqle;
	struct timespec timeout = {.tv_sec = 1};

	struct iommu_ctx *ctx;

	int ret, fd;

	bool skip_migration = true;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	payload.len = pgmap(&payload.vaddr, 4096);
	if (payload.len < 0)
		err(1, "could not allocate aligned memory");

	scratch.len = pgmap(&scratch.vaddr, NVME_IDENTIFY_DATA_SIZE);
	if (scratch.len < 0)
		err(1, "could not allocate aligned memory");

	cmd.identify = (struct nvme_cmd_identify) {
		.opcode = nvme_admin_identify,
		.cns = NVME_IDENTIFY_CNS_CTRL,
	};

	unsigned long flags = SETUP_F_SOURCE | SETUP_F_SOURCE_IO;


	if (parent_d.bdf && !streq(parent_d.bdf, "")) {
		skip_migration = false;

		flags |= SETUP_F_DESTINATION | SETUP_F_DESTINATION_IO_PARENT;

		if (!child_d.bdf || streq(child_d.bdf, ""))
			opt_usage_exit_fail("no child destination controller specified");
	}

	setup(flags);

	opt_free_table();

	ctx = __iommu_ctx(&parent_s.ctrl);

	/* map the payload/scratch buffers */
	if (iommu_map_vaddr(ctx, payload.vaddr, payload.len, &payload.iova, 0x0))
		err(1, "failed to map payload buffer");

	if (iommu_map_vaddr(ctx, scratch.vaddr, scratch.len, &scratch.iova, 0x0))
		err(1, "failed to map payload buffer");

	printf("creating controller data queue on source parent\n");

	cdq = (struct nvme_cdq) {
		.entry_size = NVME_CDQ_MQLE0_SIZE,
	};

	cdqlen = pgmap(&cdq.vaddr, __VFN_PAGESIZE);
	if (cdqlen < 0)
		err(1, "could not allocate aligned memory");

	cdq.qsize = cdqlen / sizeof(struct nvme_mqle0);

	if (iommu_map_vaddr(__iommu_ctx(&parent_s.ctrl), cdq.vaddr, cdqlen, &cdq.iova, 0x0))
		err(1, "failed to map cdq memory");

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_cdq,

		.dptr.prp1 = cpu_to_le64(cdq.iova),

		/* target cntlid; physically contiguous */
		.cdw11 = cpu_to_le32(child_s.cntlid << 16 | 1),

		/* cdqsize */
		.cdw12 = cpu_to_le32(cdqlen >> 2),
	};

	if (nvme_admin(&parent_s.ctrl, &cmd, NULL, 0, &cqe))
		err(1, "nvme_admin");

	cdq.cdqid = le32_to_cpu(cqe.dw0) & 0xffff;

	printf("controller data queue identifier is %"PRIu16"\n", cdq.cdqid);

	printf("start user data logging on source parent\n");

	nvme_track_send(&parent_s.ctrl, &cdq, NVME_TRACK_SEND_LACT_START);

	cmd.rw = (struct nvme_cmd_rw) {
		.opcode = nvme_cmd_write,
		.nsid = cpu_to_le32(1),
	};

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		err(1, "could not open /dev/urandom");

	if (readmaxfd(fd, payload.vaddr, payload.len) < 0)
		err(1, "could not read bytes");

	if (close(fd) < 0)
		err(1, "could not close fd");

	guard = nvme_crc64(0x0, payload.vaddr, payload.len);

	printf("payload crc64 is 0x%"PRIx64"\n", guard);

	printf("issuing sentinel write of lba 0 to source child\n");

	if (nvme_sync(&child_s.ctrl, &child_s.ctrl.sq[1], &cmd, payload.vaddr, payload.len, NULL))
		err(1, "nvme_sync");

	/* clear payload buffer */
	memset(payload.vaddr, 0x0, payload.len);

	printf("stop user data logging on source parent\n");

	nvme_track_send(&parent_s.ctrl, &cdq, NVME_TRACK_SEND_LACT_STOP);

	printf("verify that user data changes have been logged in cdq\n");

	do {
		nvme_cdq_get_mqle0s(&cdq, &mqle, 1);

		printf("cdq entry\n");
		print_mqle0(&mqle);

		if (((mqle.lbamqa >> 6) & 0x3) == 0x0 && !skip_migration)
			migrate_lbas(&parent_s.ctrl, &parent_d.ctrl, &mqle);
	} while (((mqle.lbamqa >> 1) & 0x7) != 0x2);

	printf("update cdq head pointer\n");

	nvme_cdq_update_head(&parent_s.ctrl, &cdq);

	printf("start user data logging on source parent\n");

	nvme_track_send(&parent_s.ctrl, &cdq, NVME_TRACK_SEND_LACT_START);

	printf("pausing source child\n");

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_migration_send,

		/* dudmq; pause type 'Suspend'; cntlid */
		.cdw11 = cpu_to_le32(0x1 << 31 | 0x1 << 16 | child_s.cntlid),
	};

	if (nvme_admin(&parent_s.ctrl, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");

	printf("issuing read of lba 0 to source child\n");

	rq = nvme_rq_acquire(&child_s.ctrl.sq[1]);

	cmd.rw = (struct nvme_cmd_rw) {
		.opcode = nvme_cmd_read,
		.nsid = cpu_to_le32(1),
	};

	if (nvme_rq_map_prp(&child_s.ctrl, rq, &cmd, payload.iova, payload.len))
		err(1, "could not map buffer to prps");

	nvme_rq_exec(rq, &cmd);

	printf("waiting for completion (1s); expecting timeout... ");
	fflush(stdout);

	ret = nvme_cq_wait_cqes(rq->sq->cq, NULL, 1, &timeout);
	if (ret == 1 || errno != ETIMEDOUT) {
		printf("\n");

		err(1, "expected timeout");
	}

	printf("OK\n");

	printf("verify that NO user data changes have been logged in cdq\n");

	do {
		nvme_cdq_get_mqle0s(&cdq, &mqle, 1);

		printf("cdq entry\n");
		print_mqle0(&mqle);

		if (((mqle.lbamqa >> 6) & 0x3) == 0x0)
			errx(1, "Oops; migration queue entry reports lba changes\n");
	} while (((mqle.lbamqa >> 1) & 0x7) != 0x3);

	printf("getting source child controller state\n");

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_migration_receive,

		/* csvi; 'Get Controller State' */
		.cdw10 = cpu_to_le32(0x1 << 16),

		/* cntlid */
		.cdw11 = cpu_to_le32(child_s.cntlid),

		/* numdl */
		.cdw15 = cpu_to_le32((scratch.len >> 2) - 1), /* 0s based value */
	};

	if (nvme_admin(&parent_s.ctrl, &cmd, scratch.vaddr, scratch.len, NULL))
		err(1, "nvme_admin");

	struct nvme_ctrl_state_container *ncsc = (struct nvme_ctrl_state_container *)scratch.vaddr;

	printf("source child state\n");
	printf("  ver         %"PRIu16"\n", le16_to_cpu(ncsc->ver));
	printf("  csaattr     0x%"PRIx8"\n", ncsc->csattr);

	nvmecss = le64_to_cpu(ncsc->nvmecss[0]) << 2ULL;

	printf("  nvmecss     %"PRIu64" (%zu bytes)\n", nvmecss >> 2, nvmecss);

	if (nvmecss) {
		int i;

		int niosq = le16_to_cpu(ncsc->nvmecs.niosq);
		int niocq = le16_to_cpu(ncsc->nvmecs.niocq);

		printf("    ver       %"PRIu16"\n", le16_to_cpu(ncsc->nvmecs.ver));
		printf("    niosq     %"PRIu16"\n", niosq);

		for (i = 0; i < niosq; i++) {
			struct nvme_iosq_state *iosq = &ncsc->nvmecs.qss[i].sqs;

			printf("      iosq    %"PRIu16"\n", le16_to_cpu(iosq->qid));
			printf("        prp1  0x%"PRIx64"\n", le64_to_cpu(iosq->prp1));
			printf("        size  %"PRIu16"\n", le16_to_cpu(iosq->size));
			printf("        cqid  %"PRIu16"\n", le16_to_cpu(iosq->cqid));
			printf("        attrs 0x%"PRIx16"\n", le16_to_cpu(iosq->attrs));
			printf("        head  %"PRIu16"\n", le16_to_cpu(iosq->head));
			printf("        tail  %"PRIu16"\n", le16_to_cpu(iosq->tail));

		}

		printf("    niocq     %"PRIu16"\n", niocq);

		for (; i < niosq + niocq; i++) {
			struct nvme_iocq_state *iocq = &ncsc->nvmecs.qss[i].cqs;

			printf("      iocq    %"PRIu16"\n", le16_to_cpu(iocq->qid));
			printf("        prp1  0x%"PRIx64"\n", le64_to_cpu(iocq->prp1));
			printf("        size  %"PRIu16"\n", le16_to_cpu(iocq->size));
			printf("        attrs 0x%"PRIx32"\n", le32_to_cpu(iocq->attrs));
			printf("        head  %"PRIu16"\n", le16_to_cpu(iocq->head));
			printf("        tail  %"PRIu16"\n", le16_to_cpu(iocq->tail));

		}
	}

	if (skip_migration)
		goto out;

	printf("pausing destination child\n");

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_migration_send,

		/* pause type 'Suspend'; cntlid */
		.cdw11 = cpu_to_le32(0x1 << 16 | child_d.cntlid),
	};

	if (nvme_admin(&parent_d.ctrl, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");

	printf("setting destination child controller state\n");

	int numq = le16_to_cpu(ncsc->nvmecs.niosq) + le16_to_cpu(ncsc->nvmecs.niocq);
	size_t plen = sizeof(*ncsc) + numq * sizeof(union nvme_ioq_state);

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_migration_send,

		/* full state; 'Set Controller State' */
		.cdw10 = cpu_to_le32((0x3 << 16) | 0x2),

		/* csvi 0x1; cntlid */
		.cdw11 = cpu_to_le32(0x1 << 16 | child_d.cntlid),

		/* numd */
		.cdw15 = cpu_to_le32(plen >> 2),
	};

	if (nvme_admin(&parent_d.ctrl, &cmd, scratch.vaddr, scratch.len, NULL))
		err(1, "nvme_admin");

	printf("resuming destination child\n");

	cmd = (union nvme_cmd) {
		.opcode = nvme_admin_migration_send,

		/* resume */
		.cdw10 = cpu_to_le32(0x1),

		/* cntlid */
		.cdw11 = cpu_to_le32(child_d.cntlid),
	};

	if (nvme_admin(&parent_d.ctrl, &cmd, NULL, 0, NULL))
		err(1, "nvme_admin");

	printf("consuming completion of migrated sqe on destination child\n");

	nvme_cq_get_cqes(rq->sq->cq, NULL, 1);

	printf("verifying payload crc64...");

	if (guard != nvme_crc64(0x0, payload.vaddr, payload.len)) {
		printf("\n");
		errx(1, "Oops; migrated lba verification failed");
	}

	printf(" OK\n");

out:
	printf("profit 🤑\n");

	return 0;
}
