/* SPDX-License-Identifier: GPL-2.0-or-later */

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

#ifndef LIBVFN_EXAMPLES_COMMON_H
#define LIBVFN_EXAMPLES_COMMON_H

extern char *bdf;
extern bool show_usage;

extern struct opt_table opts_base[];

void opt_show_ulongval_hex(char buf[OPT_SHOW_LEN], const unsigned long *ul);
void opt_show_uintval_hex(char buf[OPT_SHOW_LEN], const unsigned int *ui);

#endif /* LIBVFN_EXAMPLES_COMMON_H */
