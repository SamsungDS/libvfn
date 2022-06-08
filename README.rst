libvfn
======

**Low-level NVM Express Mastery Galore!**

|build badge| |coverity badge|

libvfn is library for interacting with PCIe-based NVMe devices from user-space
using the Linux kernel ``vfio-pci`` driver.

The core of the library is **excessively low-level** and aims to allow
controller verification and testing teams to interact with the NVMe device at
the register and queue level. However, it does also provide a range of utility
functions to issue commands and polling for their completions along with helpers
for mapping memory in the commands as well as managing allocation of I/O Virtual
Address through VFIO.

Issuing a crafted command using the high-level API can be as simple as:

.. code:: c

   #include <vfn/nvme.h>

   #include <nvme/types.h>

   int main(void) {
     struct nvme_ctrl ctrl;
     union nvme_cmd cmd;
     void *vaddr;

     nvme_init(&ctrl, "0000:01:00.0", NULL);

     pgmap(&vaddr, NVME_IDENTIFY_DATA_SIZE);

     cmd.identify = (struct nvme_cmd_identify) {
       .opcode = nvme_admin_identify,
       .cns = NVME_IDENTIFY_CNS_CTRL,
     };

     nvme_admin(&ctrl, &cmd, vaddr, NVME_IDENTIFY_DATA_SIZE, NULL);

     printf("vid 0x%"PRIx8"\n", ((struct nvme_id_ctrl *)vaddr)->vid);

     return 0;
   }


The above example uses the ``support`` library from ``lib/support`` for the page
allocation as well as `libnvme`_ for NVM Express data structure and constant
definitions.

However, the true power of the library is only evident when using the lower
level interfaces. See the `cmb <examples/cmb.c>`__ and `perf
<examples/perf.c>`__ examples.

.. |build badge| image:: https://github.com/OpenMPDK/libvfn/actions/workflows/build.yml/badge.svg
   :target: https://github.com/OpenMPDK/libvfn/actions/workflows/build.yml

.. |coverity badge| image:: https://scan.coverity.com/projects/25028/badge.svg
   :target: https://scan.coverity.com/projects/openmpdk-libvfn


Dependencies
------------

The core libvfn library has zero dependencies apart from the included CCAN
components in ``ccan/``. However, the included (and separately licensed)
examples depend on `libnvme`_.

To build the documentation, `Sphinx <https://www.sphinx-doc.org/>`__ is
required.


Building
--------

libvfn uses the `meson build system <https://mesonbuild.com/>`__.

.. code::

	meson setup build
	ninja -C build


Quick start
-----------

Use the included ``vfntool`` to detach a device from the kernel ``nvme`` driver
and attach it to the ``vfio-pci`` driver.

.. code::

	./build/tools/vfntool/vfntool -d 0000:01:00.0

Verify that everything works as expected using one of the included examples.

.. code::

	./build/examples/identify -d 0000:01:00.0


License
-------

Except where otherwise stated, all software contained within this repository is
dual licensed under the GNU Lesser General Public License version 2.1 or later
or the MIT license. See ``COPYING`` and ``LICENSE`` for more information.

However, note that libvfn uses a number of components from the Comprehensive C
Archive Network (CCAN) which are separately licensed. See ``ccan/licenses`` for
a list of included licenses and ``ccan/ccan/*/LICENSE`` for the license used by
an individual component.

All software in the ``examples``, ``tools``, ``scripts`` and ``tests``
directories are licensed under the GNU General Public License version 2 or
later. See ``{examples,tools,scripts,tests}/COPYING``.

Finally, all documentation in the ``docs`` directory is dual licensed under the
GNU General Public License version 2 or later or the Creative Commons
Attribution 4.0 International license. Note however, that this only applies to
the raw rst files, since files resulting from processing by the documentation
build system may contain content tanken from files with other licenses.

.. _libnvme: https://github.com/linux-nvme/libnvme
