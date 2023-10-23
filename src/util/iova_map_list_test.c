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
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/vfio.h>

#include "ccan/compiler/compiler.h"
#include "ccan/tap/tap.h"

#include "iova_map.c"

static struct iova_map_list list;

static int add_one(void *vaddr, size_t len)
{
	return iova_map_list_add(&list, vaddr, len, 0x0);
}

static UNNEEDED void iova_map_list_print(struct iova_map_list *list)
{
	struct iova_mapping *m, *next;

	skiplist_for_each_safe(list, m, next) {
		if (m == &list->nil)
			printf("NIL ");
		else if (m == &list->sentinel)
			printf("SENTINEL ");
		else
			printf("NODE(vaddr %p len %lu) ", m->vaddr, m->len);
	}

	printf("\n");
}

int main(int argc UNUSED, char *argv[] UNUSED)
{
	plan_tests(8);

	iova_map_list_init(&list);

	ok1(list.height == 0);

	/* add ok */
	ok1(add_one((void *)0x0, 1) == 0);
	ok1(add_one((void *)0x1, 4) == 0);

	/* overlap fail */
	ok1(add_one((void *)0x2, 1) < 0);
	ok1(add_one((void *)0x3, 1) < 0);
	ok1(add_one((void *)0x4, 1) < 0);

	/* add ok */
	ok1(add_one((void *)0x5, 1) == 0);

	/* remove ok */
	ok1(iova_map_list_remove(&list, (void *)0x1) == 0);

	return exit_status();
}
