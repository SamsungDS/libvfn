/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef LIBVFN_PCI_UTIL_H
#define LIBVFN_PCI_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * pci_bind - bind a pci device to a driver
 * @bdf: pci device identifier ("bus:device:function")
 * @driver: driver name
 *
 * Bind the device identified by @bdf to the driver identified by @driver.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int pci_bind(const char *bdf, const char *driver);

/**
 * pci_unbind - unbind a device from its current driver
 * @bdf: pci device identifier ("bus:device:function")
 *
 * Unbind the device identified by @bdf from its current driver.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int pci_unbind(const char *bdf);

/**
 * pci_driver_new_id - add a new vendor/device id to the given driver
 * @driver: kernel driver
 * @vid: vendor id
 * @did: device id
 *
 * Add a new vendor/device id pair to the given driver.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int pci_driver_new_id(const char *driver, uint16_t vid, uint16_t did);

/**
 * pci_driver_remove_id - remove a vendor/device id from the given driver
 * @driver: kernel driver
 * @vid: vendor id
 * @did: device id
 *
 * Remove a vendor/device id pair from the given driver.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int pci_driver_remove_id(const char *driver, uint16_t vid, uint16_t did);

/**
 * pci_device_info_get_ull - get sysfs property
 * @bdf: pci device identifier ("bus:device:function")
 * @prop: sysfs property
 * @v: output parameter
 *
 * Read an interger value from a sysfs property.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int pci_device_info_get_ull(const char *bdf, const char *prop, unsigned long long *v);

/**
 * pci_get_driver - get name of driver device is currently bound to
 * @bdf: pci device identifier ("bus:device:function")
 *
 * Get the name of the driver the device identified by @bdf is currently bound
 * to.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
char *pci_get_driver(const char *bdf);

#ifdef __cplusplus
}
#endif

#endif /* LIBVFN_PCI_UTIL_H */
