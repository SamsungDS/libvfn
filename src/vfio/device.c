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

#define log_fmt(fmt) "vfio/device: " fmt

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
#include "vfn/vfio.h"

#include "ccan/array_size/array_size.h"
#include "ccan/minmax/minmax.h"
#include "ccan/str/str.h"

int vfio_set_irq(struct vfio_device *dev, int *eventfds, int start, int count)
{
	struct vfio_irq_set *irq_set;
	size_t irq_set_size;
	int ret;

	if (!(dev->irq_info.flags & VFIO_IRQ_INFO_EVENTFD)) {
		errno = EINVAL;
		log_debug("device irq does not support eventfd\n");
		return -1;
	}

	irq_set_size = sizeof(*irq_set) + sizeof(int) * count;
	irq_set = xmalloc(irq_set_size);

	*irq_set = (struct vfio_irq_set) {
		.argsz = (uint32_t)irq_set_size,
		.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER,
		.index = dev->irq_info.index,
		.start = start,
		.count = count,
	};

	memcpy(irq_set->data, eventfds, sizeof(int) * count);

	ret = ioctl(dev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
	free(irq_set);

	if (ret) {
		log_debug("failed to set device irq\n");
		return -1;
	}

	return 0;
}

int vfio_disable_irq(struct vfio_device *dev, int start, int count)
{
	struct vfio_irq_set *irq_set;
	size_t irq_set_size;
	int *data;
	int ret;

	irq_set_size = sizeof(*irq_set) + sizeof(int) * count;
	irq_set = xmalloc(irq_set_size);

	data = xmalloc(sizeof(int) * count);
	for (int i = 0; i < count; i++)
		data[i] = -1;

	*irq_set = (struct vfio_irq_set) {
		.argsz = (uint32_t)irq_set_size,
		.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER,
		.index = dev->irq_info.index,
		.start = start,
		.count = count,
	};

	memcpy(irq_set->data, data, sizeof(int) * count);

	ret = ioctl(dev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
	free(data);
	free(irq_set);

	if (ret) {
		log_debug("failed to disable device irq\n");
		return -1;
	}

	return 0;
}

int vfio_disable_irq_all(struct vfio_device *dev)
{
	return vfio_disable_irq(dev, 0, dev->irq_info.count);
}

int vfio_reset(struct vfio_device *dev)
{
	if (!(dev->device_info.flags & VFIO_DEVICE_FLAGS_RESET)) {
		errno = ENOTSUP;
		return -1;
	}

	return ioctl(dev->fd, VFIO_DEVICE_RESET);
}
