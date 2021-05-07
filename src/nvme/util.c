// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nvme/types.h>
#include <nvme/util.h>

#include "vfn/nvme.h"

#include "crc64table.h"

const char *nvme_admin_opcode_to_str(uint8_t opcode)
{
	switch (opcode) {
	case nvme_admin_delete_sq:	return "nvme_admin_delete_sq";
	case nvme_admin_create_sq:	return "nvme_admin_create_sq";
	case nvme_admin_get_log_page:	return "nvme_admin_get_log_page";
	case nvme_admin_delete_cq:	return "nvme_admin_delete_cq";
	case nvme_admin_create_cq:	return "nvme_admin_create_cq";
	case nvme_admin_identify:	return "nvme_admin_identify";
	case nvme_admin_abort_cmd:	return "nvme_admin_abort_cmd";
	case nvme_admin_set_features:	return "nvme_admin_set_features";
	case nvme_admin_get_features:	return "nvme_admin_get_features";
	case nvme_admin_async_event:	return "nvme_admin_async_event";
	case nvme_admin_ns_mgmt:	return "nvme_admin_ns_mgmt";
	case nvme_admin_fw_commit:	return "nvme_admin_fw_commit";
	case nvme_admin_fw_download:	return "nvme_admin_fw_download";
	case nvme_admin_dev_self_test:	return "nvme_admin_dev_self_test";
	case nvme_admin_ns_attach:	return "nvme_admin_ns_attach";
	case nvme_admin_keep_alive:	return "nvme_admin_keep_alive";
	case nvme_admin_directive_send:	return "nvme_admin_directive_send";
	case nvme_admin_directive_recv:	return "nvme_admin_directive_recv";
	case nvme_admin_virtual_mgmt:	return "nvme_admin_virtual_mgmt";
	case nvme_admin_nvme_mi_send:	return "nvme_admin_nvme_mi_send";
	case nvme_admin_nvme_mi_recv:	return "nvme_admin_nvme_mi_recv";
	case nvme_admin_dbbuf:		return "nvme_admin_dbbuf";
	case nvme_admin_fabrics:	return "nvme_admin_fabrics";
	case nvme_admin_format_nvm:	return "nvme_admin_format_nvm";
	case nvme_admin_security_send:	return "nvme_admin_security_send";
	case nvme_admin_security_recv:	return "nvme_admin_security_recv";
	case nvme_admin_sanitize_nvm:	return "nvme_admin_sanitize_nvm";
	case nvme_admin_get_lba_status:	return "nvme_admin_get_lba_status";
	default:			return "nvme_admin_unknown";
	}
}

const char *nvme_io_opcode_to_str(uint8_t opcode)
{
	switch (opcode) {
	case nvme_cmd_flush:		return "nvme_cmd_flush";
	case nvme_cmd_write:		return "nvme_cmd_write";
	case nvme_cmd_read:		return "nvme_cmd_read";
	case nvme_cmd_write_uncor:	return "nvme_cmd_write_uncor";
	case nvme_cmd_compare:		return "nvme_cmd_compare";
	case nvme_cmd_write_zeroes:	return "nvme_cmd_write_zeroes";
	case nvme_cmd_dsm:		return "nvme_cmd_dsm";
	case nvme_cmd_verify:		return "nvme_cmd_verify";
	case nvme_cmd_resv_register:	return "nvme_cmd_resv_register";
	case nvme_cmd_resv_report:	return "nvme_cmd_resv_report";
	case nvme_cmd_resv_acquire:	return "nvme_cmd_resv_acquire";
	case nvme_cmd_resv_release:	return "nvme_cmd_resv_release";
	case nvme_cmd_copy:		return "nvme_cmd_copy";
	case nvme_zns_cmd_mgmt_send:	return "nvme_zns_cmd_mgmt_send";
	case nvme_zns_cmd_mgmt_recv:	return "nvme_zns_cmd_mgmt_recv";
	case nvme_zns_cmd_append:	return "nvme_zns_cmd_append";
	default:			return "nvme_cmd_unkown";
	}
}

const char *nvme_opcode_to_str(uint8_t opcode, int qid)
{
	return qid ? nvme_io_opcode_to_str(opcode) : nvme_admin_opcode_to_str(opcode);
}

uint64_t nvme_crc64(uint64_t crc, const unsigned char *buffer, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		crc = (crc >> 8) ^ crc64_nvme_table[(crc & 0xff) ^ buffer[i]];

	return crc ^ (uint64_t)~0;
}

int nvme_set_errno_from_cqe(struct nvme_cqe *cqe)
{
	errno = nvme_status_to_errno(le16_to_cpu(cqe->sfp) >> 1, false);

	return errno ? -1 : 0;
}
