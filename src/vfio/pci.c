// SPDX-License-Identifier: LGPL-2.1-or-later or MIT

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#include <assert.h>
#include <byteswap.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <linux/limits.h>
#include <linux/vfio.h>
#include <linux/pci_regs.h>

#include <vfn/support/atomic.h>
#include <vfn/support/compiler.h>
#include <vfn/support/log.h>
#include <vfn/support/mem.h>
#include <vfn/vfio/iommu.h>
#include <vfn/vfio/state.h>
#include <vfn/vfio/pci.h>

#include "ccan/array_size/array_size.h"
#include "ccan/minmax/minmax.h"

#include "iommu.h"

static char *find_iommu_group(const char *bdf)
{
	char *p, *link = NULL, *group = NULL, *path = NULL;
	ssize_t ret;

	if (asprintf(&link, "/sys/bus/pci/devices/%s/iommu_group", bdf) < 0) {
		log_debug("asprintf failed\n");
		goto out;
	}

	group = mallocn(PATH_MAX, sizeof(char));

	ret = readlink(link, group, PATH_MAX - 1);
	if (ret < 0) {
		log_debug("failed to resolve iommu group link\n");
		goto out;
	}

	group[ret] = '\0';

	p = strrchr(group, '/');
	if (!p) {
		log_debug("failed to find iommu group number\n");
		goto out;
	}

	if (asprintf(&path, "/dev/vfio/%s", p + 1) < 0) {
		path = NULL;
		goto out;
	}

out:
	free(link);
	free(group);

	return path;
}

static int pci_set_bus_master(struct vfio_pci_state *pci)
{
	uint16_t pci_cmd;

	if (vfio_pci_read_config(pci, &pci_cmd, sizeof(pci_cmd), PCI_COMMAND) < 0) {
		log_debug("failed to read pci config region\n");
		return -1;
	}

	pci_cmd |= PCI_COMMAND_MASTER;

	if (vfio_pci_write_config(pci, &pci_cmd, sizeof(pci_cmd), PCI_COMMAND) < 0) {
		log_debug("failed to write pci config region\n");
		return -1;
	}

	return 0;
}

static int vfio_pci_init_bar(struct vfio_pci_state *pci, unsigned int idx)
{
	assert(idx < ARRAY_SIZE(pci->bar_region_info));

	pci->bar_region_info[idx] = (struct vfio_region_info) {
		.index = VFIO_PCI_BAR0_REGION_INDEX + idx,
		.argsz = sizeof(struct vfio_region_info),
	};

	if (ioctl(pci->vfio.device, VFIO_DEVICE_GET_REGION_INFO, &pci->bar_region_info[idx])) {
		log_debug("failed to get bar region info\n");
		return -1;
	}

	return 0;
}

static int vfio_pci_init_irq(struct vfio_pci_state *pci)
{
	int irq_index = VFIO_PCI_MSIX_IRQ_INDEX;

	memset(&pci->vfio.irq_info, 0x0, sizeof(pci->vfio.irq_info));
	pci->vfio.irq_info.argsz = sizeof(pci->vfio.irq_info);

	/* prefer msi-x/msi and fall back to intx */
	do {
		if (irq_index < 0) {
			errno = EINVAL;
			log_debug("no supported irq types\n");
			return -1;
		}

		pci->vfio.irq_info.index = irq_index--;

		if (ioctl(pci->vfio.device, VFIO_DEVICE_GET_IRQ_INFO, &pci->vfio.irq_info)) {
			log_debug("failed to get device irq info\n");
			return -1;
		}
	} while (!pci->vfio.irq_info.count);

	log_info("irq_info.count %d\n", pci->vfio.irq_info.count);

	return 0;
}

void *vfio_pci_map_bar(struct vfio_pci_state *pci, unsigned int idx, size_t len,
			       uint64_t offset, int prot)
{
	void *mem;

	assert(idx < ARRAY_SIZE(pci->bar_region_info));

	len = min_t(size_t, len, pci->bar_region_info[idx].size - offset);
	offset = pci->bar_region_info[idx].offset + offset;

	mem = mmap(NULL, len, prot, MAP_SHARED, pci->vfio.device, offset);
	if (mem == MAP_FAILED) {
		log_debug("failed to map bar region\n");
		mem = NULL;
	}

	return mem;
}

void vfio_pci_unmap_bar(struct vfio_pci_state *pci, unsigned int idx, void *mem, size_t len,
			uint64_t offset)
{
	assert(idx < ARRAY_SIZE(pci->bar_region_info));

	len = min_t(size_t, len, pci->bar_region_info[idx].size - offset);

	if (munmap(mem, len))
		log_debug("failed to unmap bar region\n");
}

int vfio_pci_open(struct vfio_pci_state *pci, const char *bdf)
{
	struct vfio_state *vfio = &pci->vfio;
	char *group = NULL;

	group = find_iommu_group(bdf);
	if (!group) {
		errno = EINVAL;
		return -1;
	}

	log_info("vfio iommu group is %s\n", group);

	if (access(group, F_OK) < 0) {
		log_error("%s does not exist; did you bind the device to vfio-pci?\n", group);
		goto err_free_group;
	}

	if (vfio_open(vfio, group))
		goto err_free_group;

	vfio->device = ioctl(vfio->group, VFIO_GROUP_GET_DEVICE_FD, bdf);
	if (pci->vfio.device < 0) {
		log_debug("failed to get device fd\n");
		goto err;
	}

	vfio->device_info.argsz = sizeof(struct vfio_device_info);

	if (ioctl(vfio->device, VFIO_DEVICE_GET_INFO, &vfio->device_info)) {
		log_debug("failed to get device info\n");
		goto err;
	}

	assert(vfio->device_info.flags & VFIO_DEVICE_FLAGS_PCI);

	pci->config_region_info = (struct vfio_region_info) {
		.argsz = sizeof(pci->config_region_info),
		.index = VFIO_PCI_CONFIG_REGION_INDEX,
	};

	if (ioctl(pci->vfio.device, VFIO_DEVICE_GET_REGION_INFO, &pci->config_region_info)) {
		log_debug("failed to get config region info\n");
		goto err;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(pci->bar_region_info); i++) {
		if (vfio_pci_init_bar(pci, i))
			goto err;
	}

	if (pci_set_bus_master(pci)) {
		log_debug("failed to set pci bus master\n");
		goto err;
	}

	if (vfio_pci_init_irq(pci)) {
		log_debug("failed to initialize irq\n");
		goto err;
	}

	return 0;

err:
	vfio_close(&pci->vfio);
err_free_group:
	free(group);

	return -1;
}
