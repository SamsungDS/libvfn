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

#define log_fmt(fmt) "vfio/pci: " fmt

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

#include "vfn/support.h"
#include "vfn/iommu.h"
#include "vfn/vfio.h"
#include "vfn/pci.h"

#include "ccan/array_size/array_size.h"
#include "ccan/minmax/minmax.h"
#include "ccan/str/str.h"
#include "ccan/list/list.h"

#include "iommu/context.h"

/*
 * Emulated BAR access for regions that VFIO will not let us mmap(). On
 * s390x/zPCI the guest kernel compiles out CONFIG_VFIO_PCI_MMAP, so the vfio
 * region never advertises VFIO_REGION_INFO_FLAG_MMAP and mmap() fails with
 * EINVAL. Instead of a real mapping we hand out synthetic addresses inside a
 * reserved PROT_NONE window and service the accesses with pread()/pwrite() on
 * the device fd; vfn_mmio_synth_{read,write}() (support/mmio.h) decode them.
 */
#define VFN_MMIO_SYNTH_SLOTS	16
#define VFN_MMIO_SYNTH_STRIDE	(4UL * 1024 * 1024)

struct vfn_mmio_synth_region {
	bool used;
	int fd;
	uint64_t file_off;
	size_t size;
	uintptr_t vbase;
};

static struct vfn_mmio_synth_region vfn_mmio_synth[VFN_MMIO_SYNTH_SLOTS];
static uintptr_t vfn_mmio_synth_base;
static size_t vfn_mmio_synth_span;

/*
 * Serializes slot allocation and release. The decode path (vfn_mmio_synth_read/
 * write, called from the mmio.h accessors) stays lock-free: a slot is immutable
 * once ->used is published with release semantics, and a cleared slot decodes to
 * "not synthetic" -- a racy access after unmap then faults on the PROT_NONE
 * window rather than operating on a reused slot.
 *
 * Slot ownership lives in the mapping device's pci->synth_bars[], drained by
 * vfio_pci_close() so no synthetic region can outlive its device fd.
 */
static pthread_mutex_t vfn_mmio_synth_lock = PTHREAD_MUTEX_INITIALIZER;

/* in-arena, regardless of slot liveness */
static bool vfn_mmio_synth_arena_contains(const void *addr)
{
	uintptr_t a = (uintptr_t)addr;

	return vfn_mmio_synth_span && a >= vfn_mmio_synth_base &&
	       a - vfn_mmio_synth_base < vfn_mmio_synth_span;
}

static void *vfn_mmio_synth_map(struct vfio_pci_device *pci,
				uint64_t file_off, size_t size)
{
	struct vfn_mmio_synth_region *r;
	size_t i;
	void *ret = NULL;

	if (size > VFN_MMIO_SYNTH_STRIDE) {
		errno = E2BIG;
		return NULL;
	}

	pthread_mutex_lock(&vfn_mmio_synth_lock);

	if (!vfn_mmio_synth_span) {
		size_t span = VFN_MMIO_SYNTH_SLOTS * VFN_MMIO_SYNTH_STRIDE;
		void *win;

		win = mmap(NULL, span, PROT_NONE,
			   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
		if (win == MAP_FAILED)
			goto out;

		vfn_mmio_synth_base = (uintptr_t)win;
		vfn_mmio_synth_span = span;
	}

	/* find (and reserve) an ownership slot for this device */
	for (i = 0; i < ARRAY_SIZE(pci->synth_bars); i++) {
		if (pci->synth_bars[i])
			continue;

		for (size_t s = 0; s < VFN_MMIO_SYNTH_SLOTS; s++) {
			r = &vfn_mmio_synth[s];

			if (__atomic_load_n(&r->used, __ATOMIC_ACQUIRE))
				continue;

			r->fd = pci->dev.fd;
			r->file_off = file_off;
			r->size = size;
			r->vbase = vfn_mmio_synth_base +
				   (uintptr_t)s * VFN_MMIO_SYNTH_STRIDE;

			pci->synth_bars[i] = r;
			/* publish last: accessors only trust ->used */
			__atomic_store_n(&r->used, true, __ATOMIC_RELEASE);

			ret = (void *)r->vbase;
			goto out;
		}

		break; /* device slot free but address space exhausted */
	}

	errno = ENOSPC;
out:
	pthread_mutex_unlock(&vfn_mmio_synth_lock);
	return ret;
}

static struct vfn_mmio_synth_region *vfn_mmio_synth_lookup(const void *addr)
{
	uintptr_t a = (uintptr_t)addr;
	struct vfn_mmio_synth_region *r;

	if (!vfn_mmio_synth_span || a < vfn_mmio_synth_base ||
	    a - vfn_mmio_synth_base >= vfn_mmio_synth_span)
		return NULL;

	r = &vfn_mmio_synth[(a - vfn_mmio_synth_base) / VFN_MMIO_SYNTH_STRIDE];
	if (!__atomic_load_n(&r->used, __ATOMIC_ACQUIRE))
		return NULL;

	return r;
}

bool vfn_mmio_synth_is(const void *addr)
{
	return !!vfn_mmio_synth_lookup(addr);
}

enum vfn_mmio_synth_result vfn_mmio_synth_read(void *addr, void *buf, size_t len)
{
	struct vfn_mmio_synth_region *r = vfn_mmio_synth_lookup(addr);
	size_t off;
	ssize_t n;

	if (!r)
		return VFN_MMIO_NOT_SYNTH;

	/*
	 * The window is larger than the region, so the address decode is not
	 * enough: bound against the mapped size. This must not report "not
	 * synthetic" or the caller would dereference the PROT_NONE window.
	 */
	off = (uintptr_t)addr - r->vbase;
	if (off > r->size || len > r->size - off) {
		log_debug("synthetic bar read out of bounds (off %#zx len %zu, size %#zx)\n",
			  off, len, r->size);
		return VFN_MMIO_SYNTH_ERROR;
	}

	do {
		n = pread(r->fd, buf, len, r->file_off + off);
	} while (n < 0 && errno == EINTR);

	if (n != (ssize_t)len) {
		log_debug("failed to read synthetic bar region (off %#zx len %zu): %s\n",
			  off, len, n < 0 ? strerror(errno) : "short read");
		return VFN_MMIO_SYNTH_ERROR;
	}

	return VFN_MMIO_SYNTH_DONE;
}

enum vfn_mmio_synth_result vfn_mmio_synth_write(void *addr, const void *buf, size_t len)
{
	struct vfn_mmio_synth_region *r = vfn_mmio_synth_lookup(addr);
	size_t off;
	ssize_t n;

	if (!r)
		return VFN_MMIO_NOT_SYNTH;

	off = (uintptr_t)addr - r->vbase;
	if (off > r->size || len > r->size - off) {
		log_debug("synthetic bar write out of bounds (off %#zx len %zu, size %#zx)\n",
			  off, len, r->size);
		return VFN_MMIO_SYNTH_ERROR;
	}

	do {
		n = pwrite(r->fd, buf, len, r->file_off + off);
	} while (n < 0 && errno == EINTR);

	if (n != (ssize_t)len) {
		log_debug("failed to write synthetic bar region (off %#zx len %zu): %s\n",
			  off, len, n < 0 ? strerror(errno) : "short write");
		return VFN_MMIO_SYNTH_ERROR;
	}

	return VFN_MMIO_SYNTH_DONE;
}

/*
 * Release the synthetic mapping at @mem, but only if it is owned by @pci.
 *
 * Return: true if @mem is (or was) an arena address and must NOT be
 * munmap()ed -- the arena is pre-reserved and munmap()ing part of it would
 * silently destroy the window (and a future mmap() could then alias a
 * synthetic slot). A stale or double unmap, or an unmap of a region owned
 * by another device, logs and stops here rather than releasing someone
 * else's live slot.
 */
static bool vfn_mmio_synth_release(struct vfio_pci_device *pci, void *mem)
{
	size_t i;

	if (!vfn_mmio_synth_arena_contains(mem))
		return false;

	pthread_mutex_lock(&vfn_mmio_synth_lock);

	for (i = 0; i < ARRAY_SIZE(pci->synth_bars); i++) {
		struct vfn_mmio_synth_region *r = pci->synth_bars[i];

		if (!r || (void *)r->vbase != mem)
			continue;

		pci->synth_bars[i] = NULL;
		__atomic_store_n(&r->used, false, __ATOMIC_RELEASE);

		pthread_mutex_unlock(&vfn_mmio_synth_lock);
		return true;
	}

	pthread_mutex_unlock(&vfn_mmio_synth_lock);

	log_debug("unmapping synthetic bar %p not owned by this device (stale or double unmap?)\n",
		  mem);
	return true;
}

/*
 * Release any synthetic mappings still owned by @pci. Called from
 * vfio_pci_close() before the fd is closed; a mapping that survived this
 * long means the caller leaked (unmap-before-close is expected).
 */
static void vfn_mmio_synth_drain(struct vfio_pci_device *pci)
{
	size_t i;

	pthread_mutex_lock(&vfn_mmio_synth_lock);

	for (i = 0; i < ARRAY_SIZE(pci->synth_bars); i++) {
		struct vfn_mmio_synth_region *r = pci->synth_bars[i];

		if (!r)
			continue;

		log_debug("releasing leaked synthetic bar mapping %p\n",
			  (void *)r->vbase);
		__atomic_store_n(&r->used, false, __ATOMIC_RELEASE);
		pci->synth_bars[i] = NULL;
	}

	pthread_mutex_unlock(&vfn_mmio_synth_lock);
}

static int pci_set_bus_master(struct vfio_pci_device *pci)
{
	leint16_t le_cmd;
	uint16_t pci_cmd;

	if (vfio_pci_read_config(pci, &le_cmd, sizeof(le_cmd), PCI_COMMAND) < 0) {
		log_debug("failed to read pci config region\n");
		return -1;
	}

	pci_cmd = le16_to_cpu(le_cmd) | PCI_COMMAND_MASTER;
	le_cmd = cpu_to_le16(pci_cmd);

	if (vfio_pci_write_config(pci, &le_cmd, sizeof(le_cmd), PCI_COMMAND) < 0) {
		log_debug("failed to write pci config region\n");
		return -1;
	}

	return 0;
}

static int vfio_pci_init_bar(struct vfio_pci_device *pci, int idx)
{
	assert(idx < PCI_STD_NUM_BARS);

	pci->bar_region_info[idx] = (struct vfio_region_info) {
		.index = VFIO_PCI_BAR0_REGION_INDEX + idx,
		.argsz = sizeof(struct vfio_region_info),
	};

	if (ioctl(pci->dev.fd, VFIO_DEVICE_GET_REGION_INFO, &pci->bar_region_info[idx])) {
		log_debug("failed to get bar region info\n");
		return -1;
	}

	return 0;
}

static int vfio_pci_init_irq(struct vfio_pci_device *pci)
{
	int irq_index = VFIO_PCI_MSIX_IRQ_INDEX;

	memset(&pci->dev.irq_info, 0x0, sizeof(pci->dev.irq_info));
	pci->dev.irq_info.argsz = sizeof(pci->dev.irq_info);

	/* prefer msi-x/msi and fall back to intx */
	do {
		if (irq_index < 0) {
			errno = EINVAL;
			log_debug("no supported irq types\n");
			return -1;
		}

		pci->dev.irq_info.index = irq_index--;

		if (ioctl(pci->dev.fd, VFIO_DEVICE_GET_IRQ_INFO, &pci->dev.irq_info)) {
			log_debug("failed to get device irq info\n");
			return -1;
		}
	} while (!pci->dev.irq_info.count);

	log_debug("irq_info.count %d\n", pci->dev.irq_info.count);

	return 0;
}

int vfio_pci_get_irq_info(struct vfio_pci_device *pci, struct vfio_irq_info *irq)
{
	int size = irq->argsz ? irq->argsz : sizeof(struct vfio_irq_info);

	if (!pci->bdf) {
		errno = ENODEV;
		return -1;
	}

	memcpy(irq, &pci->dev.irq_info, size);
	return 0;
}

void *vfio_pci_map_bar(struct vfio_pci_device *pci, int idx, size_t len, uint64_t offset,
		       int prot)
{
	void *mem;

	assert(idx < PCI_STD_NUM_BARS);

	len = min_t(size_t, len, pci->bar_region_info[idx].size - offset);
	offset = pci->bar_region_info[idx].offset + offset;

	if (!(pci->bar_region_info[idx].flags & VFIO_REGION_INFO_FLAG_MMAP)) {
		/*
		 * FLAG_MMAP is unset (s390x/zPCI compiles out CONFIG_VFIO_PCI_MMAP),
		 * so fall back to pread()/pwrite(). Key off the flag, not an
		 * mmap() failure, so a real mmap() error on a mappable region
		 * still fails loudly instead of silently switching mechanism.
		 */
		mem = vfn_mmio_synth_map(pci, offset, len);
	} else {
		mem = mmap(NULL, len, prot, MAP_SHARED, pci->dev.fd, offset);
		if (mem == MAP_FAILED)
			mem = NULL;
	}

	if (!mem)
		log_debug("failed to map bar region\n");

	return mem;
}

void vfio_pci_unmap_bar(struct vfio_pci_device *pci, int idx, void *mem, size_t len,
			uint64_t offset)
{
	assert(idx < PCI_STD_NUM_BARS);

	if (vfn_mmio_synth_release(pci, mem))
		return;

	len = min_t(size_t, len, pci->bar_region_info[idx].size - offset);

	if (munmap(mem, len))
		log_debug("failed to unmap bar region\n");
}

int vfio_pci_open(struct vfio_pci_device *pci, const char *bdf)
{
	if (pci->bdf) {
		errno = EALREADY;
		return -1;
	}

	if (pci_device_info_get_ull(bdf, "class", &pci->classcode)) {
		log_debug("could not get device class code\n");
		return -1;
	}

	log_info("pci class code is 0x%06llx\n", pci->classcode);

	if (!pci->dev.ctx)
		pci->dev.ctx = iommu_get_context(bdf);

	if (!pci->dev.ctx) {
		log_debug("failed to get IOMMU context\n");
		return -1;
	}

	pci->dev.fd = pci->dev.ctx->ops.get_device_fd(pci->dev.ctx, bdf);
	if (pci->dev.fd < 0) {
		log_debug("failed to get device fd\n");
		return -1;
	}

	pci->dev.device_info.argsz = sizeof(struct vfio_device_info);

	if (ioctl(pci->dev.fd, VFIO_DEVICE_GET_INFO, &pci->dev.device_info)) {
		log_debug("failed to get device info\n");
		return -1;
	}

	assert(pci->dev.device_info.flags & VFIO_DEVICE_FLAGS_PCI);

	pci->config_region_info = (struct vfio_region_info) {
		.argsz = sizeof(pci->config_region_info),
		.index = VFIO_PCI_CONFIG_REGION_INDEX,
	};

	if (ioctl(pci->dev.fd, VFIO_DEVICE_GET_REGION_INFO, &pci->config_region_info)) {
		log_debug("failed to get config region info\n");
		return -1;
	}

	for (int i = 0; i < PCI_STD_NUM_BARS; i++) {
		if (vfio_pci_init_bar(pci, i))
			return -1;
	}

	if (pci_set_bus_master(pci)) {
		log_debug("failed to set pci bus master\n");
		return -1;
	}

	if (vfio_pci_init_irq(pci)) {
		log_debug("failed to initialize irq\n");
		return -1;
	}

	pci->bdf = strdup(bdf);

	return 0;
}

int vfio_pci_close(struct vfio_pci_device *pci)
{
	struct iommu_ctx *ctx = pci->dev.ctx;

	vfn_mmio_synth_drain(pci);

	log_fatal_if(close(pci->dev.fd), "close");

	if (ctx->ops.put_device_fd)
		if (ctx->ops.put_device_fd(ctx, pci->bdf))
			return -1;

	free(pci->bdf);
	memset(pci, 0x0, sizeof(*pci));

	return 0;
}
