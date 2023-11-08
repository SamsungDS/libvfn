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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "ccan/compiler/compiler.h"
#include "ccan/tap/tap.h"

#include "vfn/support/compiler.h"
#include "vfn/support/mem.h"

#include "skiplist.c"

static struct skiplist list;

struct entry {
	unsigned int v;

	struct skiplist_node list;
};

static int __cmp(const void *key, const struct skiplist_node *n)
{
	struct entry *e = container_of_var(n, e, list);
	const unsigned int *v = key;

	if (*v < e->v)
		return -1;
	else if (*v > e->v)
		return 1;

	return 0;
}

static bool add(unsigned int v)
{
	struct skiplist_node *update[SKIPLIST_LEVELS] = {};
	struct entry *e = znew_t(struct entry, 1);

	e->v = v;

	if (skiplist_find(&list, &v, __cmp, update))
		return false;

	skiplist_link(&list, &e->list, update);

	return true;
}

static UNNEEDED void skiplist_print(struct skiplist *list)
{
	struct skiplist_node *n, *next;

	for (int k = list->height; k >= 0; k--) {
		printf("LEVEL %d: ", k);

		skiplist_for_each_safe(list, n, next, k) {

			if (n == &list->sentinel) {
				printf("SENTINEL ");
				continue;
			}

			printf("NODE(%d) ", skiplist_entry(n, struct entry, list)->v);
		}

		printf("\n");
	}
}

static void __clear(void UNUSED *opaque, struct skiplist_node *n)
{
	free(skiplist_entry(n, struct entry, list));
}

int main(int argc UNUSED, char *argv[] UNUSED)
{
	struct skiplist_node *n, *update[SKIPLIST_LEVELS];
	unsigned int v;

	plan_tests(21);

	skiplist_init(&list);

	ok(list.height == 0, "initial height is zero");

	/* add ok */
	ok(add(1), "add 1");
	ok(add(3), "add 3");
	ok(add(2), "add 2");
	ok(add(0), "add 0");

	for (v = 1; v < 4; v++) {
		n = skiplist_find(&list, &v, __cmp, NULL);

		ok(n && skiplist_entry(n, struct entry, list)->v == v,
		   "find %d ok", v);
	}

	v = 4;

	ok(skiplist_find(&list, &v, __cmp, NULL) == NULL, "find 4 not ok");

	v = 3;
	n = skiplist_find(&list, &v, __cmp, update);
	ok(n, "find 3");
	skiplist_erase(&list, n, update);

	ok(skiplist_find(&list, &v, __cmp, NULL) == NULL, "find 3 not ok");

	skiplist_clear_with(&list, __clear, NULL);

	ok(list.height == 0, "height is zero");

	/* add ok */
	ok(add(1), "add 1");
	ok(add(3), "add 3");
	ok(add(2), "add 2");
	ok(add(0), "add 0");

	for (v = 1; v < 4; v++) {
		n = skiplist_find(&list, &v, __cmp, NULL);

		ok(n && skiplist_entry(n, struct entry, list)->v == v,
		   "find %d ok", v);
	}

	v = 0;
	n = skiplist_find(&list, &v, __cmp, update);
	ok(n, "find 0");
	skiplist_erase(&list, n, update);

	ok(skiplist_find(&list, &v, __cmp, NULL) == NULL, "find 0 not ok");

	return exit_status();
}
