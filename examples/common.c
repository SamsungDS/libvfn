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
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <vfn/nvme.h>

#include "ccan/opt/opt.h"

#include "common.h"

char *bdf = "";
bool show_usage;

struct opt_table opts_base[] = {
	OPT_WITHOUT_ARG("-h|--help", opt_set_bool, &show_usage, "show usage"),
	OPT_WITH_ARG("-d|--device BDF", opt_set_charp, opt_show_charp, &bdf, "pci device"),
	OPT_ENDTABLE,
};

void opt_show_ulongval_hex(char buf[OPT_SHOW_LEN], const unsigned long *ul)
{
	snprintf(buf, OPT_SHOW_LEN, "0x%lx", *ul);
}

void opt_show_uintval_hex(char buf[OPT_SHOW_LEN], const unsigned int *ui)
{
	snprintf(buf, OPT_SHOW_LEN, "0x%x", *ui);
}
