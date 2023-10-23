/* SPDX-License-Identifier: LGPL-2.1-or-later or MIT */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All Rights Reserved.
 *
 * This library (libvfn) is dual licensed under the GNU Lesser General
 * Public License version 2.1 or later or the MIT license. See the
 * COPYING and LICENSE files for more information.
 */

#ifndef LIBVFN_VFIO_DEVICE_H
#define LIBVFN_VFIO_DEVICE_H

struct vfio_device {
	int fd;

	struct iommu_ctx *ctx;

	struct vfio_device_info device_info;
	struct vfio_irq_info irq_info;
};

#define __iommu_ctx(x) (((struct vfio_device *)(x))->ctx)

/**
 * vfio_set_irq - Enable IRQs through eventfds
 * @dev: &struct vfio_device
 * @eventfds: array of eventfds
 * @count: number of eventfds
 *
 * Enable interrupts for a range of vectors. See linux/vfio.h for documentation
 * on the format of @eventfds.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_set_irq(struct vfio_device *dev, int *eventfds, int count);

/**
 * vfio_disable_irq - Disable all IRQs
 * @dev: &struct vfio_device
 *
 * Disable all IRQs.
 */
int vfio_disable_irq(struct vfio_device *dev);

/**
 * vfio_reset - reset vfio device
 * @dev: &struct vfio_device to reset
 *
 * Reset the VFIO device if supported.
 *
 * Return: ``0`` on success, ``-1`` on error and sets ``errno``.
 */
int vfio_reset(struct vfio_device *dev);

#endif /* LIBVFN_VFIO_DEVICE_H */
