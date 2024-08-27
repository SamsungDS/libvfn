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

#ifndef LIBVFN_PCI_UTIL_H
#define LIBVFN_PCI_UTIL_H

/**
 * pci_bind - Bind a pci device to a driver
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
 * pci_unbind - Unbind a device from its current driver
 * @bdf: pci device identifier ("bus:device:function")
 *
 * Unbind the device identified by @bdf from its current driver.
 *
 * Return: On success, returns ``0``. On error, returns ``-1`` and sets
 * ``errno``.
 */
int pci_unbind(const char *bdf);

/**
 * pci_driver_new_id - Add a new vendor/device id to the given driver
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
 * pci_driver_remove_id - Remove a vendor/device id from the given driver
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
 * pci_device_info_get_ull - Get sysfs property
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
 * pci_get_driver - Get the name of the driver that the device is currently
 *                  bound to
 * @bdf: pci device identifier ("bus:device:function")
 *
 * Get the name of the driver the device identified by @bdf is currently bound
 * to.
 *
 * Return: The name of driver device bound to @bdf.
 */
char *pci_get_driver(const char *bdf);

/**
 * pci_get_iommu_group - Get iommu group path
 * @bdf: pci device identifier ("bus:device:function")
 *
 * Get the iommu group path (/dev/vfio/N) of the device identified by @bdf.
 *
 * Return: The path to the iommu group
 */
char *pci_get_iommu_group(const char *bdf);

/**
 * pci_get_device_vfio_id - Get vfio device id
 * @bdf: pci device identifier ("bus:device:function")
 *
 * Get the vfio device id (/sys/bus/pci/devices/%s/vfio-dev/vfio%d) of the
 * device identified by @bdf.
 *
 * Return: The vfio device id name
 */
char *pci_get_device_vfio_id(const char *bdf);

/**
 * pci_get_vf_bdf - Get pci bdf address of a virtual function
 * @pf_bdf: physical device bdf
 * @vfnum: the virtual function number
 *
 * Determine the PCI address ("bus:device:function") of a virtual function,
 * given the associated physical function address and the virtual function
 * number.
 *
 * Return: The virtual function bdf.
 */
char *pci_get_vf_bdf(const char *pf_bdf, int vfnum);

/**
 * pci_is_vf - Determine if a device is a virtual function
 * @bdf: device address
 *
 * Determine if the given device is a virtual function.
 *
 * Return: ``true`` if the device is a virtual function; ``false`` otherwise.
 */
bool pci_is_vf(const char *bdf);

/**
 * pci_vf_get_pf_bdf - Determine the physical function bdf given the bdf of a
 *                     virtual function
 * @bdf: virtual function device address
 *
 * Determine the bdf of the physical function corresponding to the given virtual
 * function.
 *
 * Return: The physical function bdf.
 */
char *pci_vf_get_pf_bdf(const char *bdf);

/**
 * pci_vf_get_vfnum - Get the Virtual Function Number of a VF
 * @bdf: virtual function device address
 *
 * Determine the Virtual Function Number (VFN) of the given Virtual Function.
 *
 * **Note**: While, in sysfs where we look this up, Linux uses "virtfn0" for VF
 * 1 and so on, this function returns the VFN as defined in PCI Express (i.e.,
 * virtfn0 is 1 and so on).
 *
 * Return: The VFs Virtual Function Number.
 */
int pci_vf_get_vfnum(const char *bdf);

#endif /* LIBVFN_PCI_UTIL_H */
