Memory-mapped I/O Helpers
=========================

These helpers expect and return little endian types (i.e. ``leint32_t``) which
may help with locating endianness related bugs using the sparse static analyzer.

.. kernel-doc:: include/vfn/nvme/mmio.h
