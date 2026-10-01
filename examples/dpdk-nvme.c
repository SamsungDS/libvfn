// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2026 The libvfn Authors. All rights reserved.
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

/*
 * Send a block off an nvme device out of a nic, without copying it.
 *
 * libvfn drives the nvme device and dpdk drives the nic, in one process and on
 * one iova space. The nvme read lands in the mbuf that the nic transmits, so
 * the block crosses from one library to the other without being copied and
 * without being mapped twice.
 *
 * dpdk opens the nvme device with rte_vfio_setup_device(), which puts it in
 * the container dpdk maps into, and libvfn drives it from there. Every buffer
 * libvfn needs comes from rte_zmalloc() through struct iommu_dma_ops, so dpdk
 * has mapped all of it and knows the iova of all of it.
 *
 * Both devices have to be bound to vfio-pci:
 *
 *     dpdk-nvme -d <nvme bdf> --nic <nic bdf> -N 1
 *
 * EAL arguments can follow a -- and are rarely needed. The nic is allowlisted
 * from --nic, and EAL takes iova-as-va by itself wherever there is an iommu.
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memory.h>
#include <rte_vfio.h>

#include <vfn/nvme.h>

#include <nvme/types.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

#include "common.h"

#define PCI_SYSFS_BASE	"/sys/bus/pci/devices"

/* this example assumes the namespace is formatted with 512 byte blocks */
#define LBA_SIZE	512

/* IEEE Std 802 local experimental ethertype, to keep the frame recognisable */
#define ETHERTYPE	0x88b5

#define RING_SIZE	128
#define NUM_MBUFS	1023
#define MBUF_CACHE_SIZE	32
#define TX_RETRIES	1000

static unsigned long nsid;
static char *nic = "";

static struct opt_table opts[] = {
	OPT_SUBTABLE(opts_base, NULL),
	OPT_WITH_ARG("-a|--nic", opt_set_charp, opt_show_charp, &nic,
		     "pci device of the nic to transmit from"),
	OPT_WITH_ARG("-N|--nsid", opt_set_ulongval, opt_show_ulongval, &nsid,
		     "namespace identifier"),
	OPT_ENDTABLE,
};

/*
 * dpdk maps the memory it allocates into its own container as it goes, and in
 * iova-as-va mode the iova of a buffer is its virtual address. So libvfn takes
 * its buffers from the dpdk heap and there is nothing left to map.
 */
static void *dpdk_dma_alloc(void *opaque UNUSED, size_t len, iova_t *iova)
{
	void *vaddr = rte_zmalloc("libvfn", len, __VFN_PAGESIZE);

	if (!vaddr)
		return NULL;

	*iova = (iova_t)(uintptr_t)vaddr;

	return vaddr;
}

static void dpdk_dma_free(void *opaque UNUSED, void *vaddr, size_t len UNUSED)
{
	rte_free(vaddr);
}

static int dpdk_dma_map(void *opaque UNUSED, void *vaddr, size_t len UNUSED, iova_t *iova,
			unsigned long flags)
{
	/* the iova of a mapping is its vaddr, so the caller cannot choose one */
	if (flags & IOMMU_MAP_FIXED_IOVA) {
		errno = EOPNOTSUPP;
		return -1;
	}

	/* memory dpdk did not allocate has no iova, and nothing has mapped it */
	if (!rte_mem_virt2memseg(vaddr, NULL)) {
		errno = EINVAL;
		return -1;
	}

	*iova = (iova_t)(uintptr_t)vaddr;

	return 0;
}

/* map reports the iova dpdk gave the memory when it allocated it, and maps nothing */
static int dpdk_dma_unmap(void *opaque UNUSED, iova_t iova UNUSED, size_t len UNUSED)
{
	return 0;
}

static const struct iommu_dma_ops dma_ops = {
	.alloc = dpdk_dma_alloc,
	.free = dpdk_dma_free,
	.map = dpdk_dma_map,
	.unmap = dpdk_dma_unmap,
};

static int port_init(uint16_t portid, struct rte_mempool *mp)
{
	struct rte_eth_conf conf = {};
	int socket = rte_eth_dev_socket_id(portid);
	int ret;

	ret = rte_eth_dev_configure(portid, 1, 1, &conf);
	if (ret)
		return ret;

	ret = rte_eth_rx_queue_setup(portid, 0, RING_SIZE, socket, NULL, mp);
	if (ret)
		return ret;

	ret = rte_eth_tx_queue_setup(portid, 0, RING_SIZE, socket, NULL);
	if (ret)
		return ret;

	return rte_eth_dev_start(portid);
}

static int read_block(struct nvme_ctrl *ctrl, struct iovec *iov)
{
	union nvme_cmd cmd;
	struct nvme_rq *rq;
	int ret;

	cmd.rw = (struct nvme_cmd_rw) {
		.opcode = nvme_cmd_read,
		.nsid = cpu_to_le32(nsid),
	};

	rq = nvme_rq_acquire(&ctrl->sq[1]);
	if (!rq)
		return -1;

	ret = nvme_rq_mapv(ctrl, rq, &cmd, iov, 1);
	if (!ret) {
		nvme_rq_exec(rq, &cmd);
		ret = nvme_rq_spin(rq, NULL);
	}

	nvme_rq_release(rq);

	return ret;
}

/*
 * EAL gets the nic to allowlist, then whatever followed -- on our own command
 * line, so that an argument given there wins over one of ours.
 */
static int eal_init(int argc, char **argv)
{
	int eal_argc = 0;
	char **eal_argv;
	int ret;

	eal_argv = malloc((size_t)(argc + 6) * sizeof(*eal_argv));
	if (!eal_argv)
		err(1, "malloc");

	eal_argv[eal_argc++] = argv[0];

	eal_argv[eal_argc++] = "-a";
	eal_argv[eal_argc++] = nic;

	eal_argv[eal_argc++] = "--no-huge";
	eal_argv[eal_argc++] = "-m";
	eal_argv[eal_argc++] = "16";

	for (int i = 1; i < argc; i++)
		eal_argv[eal_argc++] = argv[i];

	eal_argv[eal_argc] = NULL;

	ret = rte_eal_init(eal_argc, eal_argv);

	free(eal_argv);

	return ret;
}

/* the block goes out as the payload of one frame, addressed to everybody */
static void eth_hdr_init(struct rte_ether_hdr *hdr, uint16_t portid)
{
	memset(&hdr->dst_addr, 0xff, sizeof(hdr->dst_addr));

	if (rte_eth_macaddr_get(portid, &hdr->src_addr))
		memset(&hdr->src_addr, 0x0, sizeof(hdr->src_addr));

	hdr->ether_type = rte_cpu_to_be_16(ETHERTYPE);
}

int main(int argc, char **argv)
{
	struct vfio_device_info dev_info = {.argsz = sizeof(dev_info)};
	struct rte_eth_stats stats;
	struct rte_mempool *mp;
	struct rte_mbuf *m;
	struct nvme_ctrl ctrl = {};
	struct iommu_ctx *ctx;
	struct iovec iov;
	uint32_t pkt_len;
	uint16_t portid;
	uint16_t p;
	void *payload;
	iova_t iova;
	int devfd;

	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	if (streq(nic, ""))
		opt_usage_exit_fail("missing --nic parameter");

	if (!nsid)
		opt_usage_exit_fail("missing --nsid parameter");

	/* opt_parse() has left whatever followed -- for EAL */
	if (eal_init(argc, argv) < 0)
		errx(1, "rte_eal_init: %s", rte_strerror(rte_errno));

	portid = RTE_MAX_ETHPORTS;

	RTE_ETH_FOREACH_DEV(p) {
		portid = p;
		break;
	}

	if (portid == RTE_MAX_ETHPORTS)
		errx(1, "dpdk probed no ethernet device on %s", nic);

	/*
	 * EAL picks iova-as-va by itself when there is an iommu, so this only
	 * catches a caller who asked for iova-as-pa. There the iova of a buffer
	 * is its physical address, and the dma ops below would have to ask
	 * rte_mem_virt2iova() for every one of them.
	 */
	if (rte_eal_iova_mode() != RTE_IOVA_VA)
		errx(1, "this example does not support iova-as-pa");

	/*
	 * dpdk opens the device, which puts it in the container dpdk maps
	 * into, and sets the iommu type on that container if this is the first
	 * device in it.
	 */
	if (rte_vfio_setup_device(PCI_SYSFS_BASE, bdf, &devfd, &dev_info))
		errx(1, "dpdk could not set up %s", bdf);

	ctx = iommu_get_context_external(&dma_ops);
	if (!ctx)
		err(1, "could not create an external context");

	if (iommu_ctx_add_device(ctx, bdf, devfd))
		err(1, "could not give the context the device fd");

	/*
	 * The context is set before nvme_init() so that the controller is
	 * opened on it instead of on one of libvfn's own.
	 */
	ctrl.pci.dev.ctx = ctx;

	if (nvme_init(&ctrl, bdf, NULL))
		err(1, "failed to init nvme controller");

	if (nvme_create_ioqpair(&ctrl, 1, 64, -1, 0x0))
		err(1, "could not create io queue pair");

	mp = rte_pktmbuf_pool_create("payload", NUM_MBUFS, MBUF_CACHE_SIZE, 0,
				     RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (!mp)
		errx(1, "could not create the mbuf pool: %s", rte_strerror(rte_errno));

	if (port_init(portid, mp))
		errx(1, "could not start port %u", portid);

	m = rte_pktmbuf_alloc(mp);
	if (!m)
		errx(1, "could not allocate an mbuf");

	if (!rte_pktmbuf_append(m, sizeof(struct rte_ether_hdr) + LBA_SIZE))
		errx(1, "the mbuf has no room for the block");

	eth_hdr_init(rte_pktmbuf_mtod(m, struct rte_ether_hdr *), portid);

	/* the nvme device writes the frame payload, in place */
	payload = rte_pktmbuf_mtod_offset(m, void *, sizeof(struct rte_ether_hdr));

	iov.iov_base = payload;
	iov.iov_len = LBA_SIZE;

	if (iommu_map_vaddr(ctx, payload, LBA_SIZE, &iova, 0x0))
		err(1, "could not record the mbuf with libvfn");

	if (read_block(&ctrl, &iov))
		err(1, "read");

	printf("read lba 0 into the mbuf at %p: %02x %02x %02x %02x ...\n", payload,
	       ((uint8_t *)payload)[0], ((uint8_t *)payload)[1],
	       ((uint8_t *)payload)[2], ((uint8_t *)payload)[3]);

	/* the nic owns the mbuf once it has taken it, so read the length first */
	pkt_len = rte_pktmbuf_pkt_len(m);

	for (int i = 0; i < TX_RETRIES; i++) {
		if (rte_eth_tx_burst(portid, 0, &m, 1)) {
			printf("transmitted %u bytes on port %u\n", pkt_len, portid);

			m = NULL;

			break;
		}
	}

	if (m) {
		rte_pktmbuf_free(m);
		errx(1, "the nic would not take the frame");
	}

	if (!rte_eth_stats_get(portid, &stats))
		printf("port %u has now sent %" PRIu64 " packets, %" PRIu64 " bytes\n",
		       portid, stats.opackets, stats.obytes);

	nvme_close(&ctrl);

	iommu_put_context(ctx);

	if (rte_vfio_release_device(PCI_SYSFS_BASE, bdf, devfd))
		warnx("dpdk could not release %s", bdf);

	return 0;
}
