// SPDX-License-Identifier: GPL-2.0

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

#include <stdio.h>
#include <inttypes.h>

#include "ccan/compiler/compiler.h"

static UNNEEDED int COEFFS[] = {
	63, 61, 59, 58, 56, 55, 52, 49,
	48, 47, 46, 44, 41, 37, 36, 34,
	32, 31, 28, 26, 23, 22, 19, 16,
	13, 12, 10,  9,  6,  4,  3,  0,
};

#define CRC64_NVME_POLY 0x9A6C9329AC4BC9B5ULL

static uint64_t crc64_nvme_table[256] = { 0 };

static void generate(void)
{
	uint8_t byte;
	uint64_t crc;

	for (int i = 0; i < 256; i++) {
		crc = 0;
		byte = i;

		for (int j = 0; j < 8; j++) {
			if ((crc ^ (byte >> j)) & 1)
				crc = (crc >> 1) ^ CRC64_NVME_POLY;
			else
				crc = crc >> 1;
		}

		crc64_nvme_table[i] = crc;
	}
}

static void print(void)
{
	printf("/* GENERATED FILE; DO NOT EDIT! */\n");
	printf("\n");
	printf("static const uint64_t crc64_nvme_table[256] = {\n");

	for (int i = 0; i < 256; i++) {
		if (i % 2 == 0)
			printf("\t");

		printf("0x%016" PRIx64 "ULL", crc64_nvme_table[i]);

		if (i % 2 == 1)
			printf(",\n");
		else
			printf(", ");
	}

	printf("};\n");
}

int main(int argc UNUSED, char *argv[] UNUSED)
{
	generate();
	print();

	return 0;
}
