// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * This file is part of libvfn.
 *
 * Copyright (C) 2022 The libvfn Authors. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <vfn/support.h>
#include <vfn/pci.h>

#include "ccan/err/err.h"
#include "ccan/opt/opt.h"
#include "ccan/str/str.h"

static char *bdf = "", *target = "vfio-pci";
static bool show_usage, verbose;

static struct opt_table opts[] = {
	OPT_WITHOUT_ARG("-h|--help", opt_set_bool, &show_usage, "show usage"),
	OPT_WITHOUT_ARG("-v|--verbose", opt_set_bool, &verbose, "verbose"),

	OPT_WITH_ARG("-d|--device BDF", opt_set_charp, opt_show_charp, &bdf, "pci device"),
	OPT_WITH_ARG("-t|--target DRIVER", opt_set_charp, opt_show_charp, &target,
		     "bind to DRIVER"),

	OPT_ENDTABLE,
};

static int do_bind(void)
{
	unsigned long long vid, did;
	__autofree char *driver = NULL;

	if (pci_device_info_get_ull(bdf, "vendor", &vid))
		err(1, "could not get device vendor id");

	if (pci_device_info_get_ull(bdf, "device", &did))
		err(1, "could not get device id");

	if (strcmp("nvme", target) == 0) {
		unsigned long long classcode;

		if (pci_device_info_get_ull(bdf, "class", &classcode))
			err(1, "could not get device class code");

		if ((classcode & 0xffff00) != 0x010800)
			errx(1, "%s is not an NVMe device", bdf);
	}

	driver = pci_get_driver(bdf);
	if (driver) {
		if (verbose)
			printf("device is bound to '%s'; ", driver);

		if (strcmp(driver, target) == 0) {
			if (verbose)
				printf("exiting\n");

			return 0;
		}

		if (verbose)
			printf("unbinding\n");

		if (pci_unbind(bdf))
			err(1, "could not unbind device");

		if (pci_driver_remove_id(driver, vid, did)) {
			if (errno != ENODEV)
				err(1, "could not remove device id from '%s' driver", driver);
		}
	}

	if (verbose)
		printf("binding to '%s'\n", target);

	if (pci_driver_new_id(target, vid, did)) {
		if (errno != EEXIST)
			err(1, "could not add device id to '%s' driver", target);

		if (pci_bind(bdf, target))
			err(1, "could not bind device to '%s'", target);
	}

	return 0;
}

int main(int argc, char **argv)
{
	opt_register_table(opts, NULL);
	opt_parse(&argc, argv, opt_log_stderr_exit);

	if (show_usage)
		opt_usage_and_exit(NULL);

	if (streq(bdf, ""))
		opt_usage_exit_fail("missing --device parameter");

	return do_bind();
}
