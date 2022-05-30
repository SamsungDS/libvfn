.. SPDX-License-Identifier: GPL-2.0-or-later or CC-BY-4.0

Memory-mapped I/O
=================

These helpers expect and return little endian types (i.e. ``leint32_t``) which
may help with locating endianness related bugs using the sparse static analyzer.

.. kernel-doc:: include/vfn/support/mmio.h
