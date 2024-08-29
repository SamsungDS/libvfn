# Changelog

## v6.0.0: (unreleased)

### ``nvme_ctrl``

* ``nvme_pci_init`` has been deprecated and will generate a warning.

``vfio_set_irq`` has been updated to receive ``start`` parameter to specify
start irq number to enable.  With this, ``vfio_disable_irq`` has been updated
to disable specific one or more irqs from ``start`` for ``count`` of irqs.
``vfio_disable_irq_all`` has been newly added to disable all the enabled irqs.

## v5.2.0: (unreleased)

### ``nvme_ctrl``

* A new set of functions for keeping track of controllers has been added to the
  public API. These are ``nvme_{get,add,del}_ctrl``. Please see the updated
  documentation.
* A set of functions to manipulate and configure secondary controllers have been
  added. See the updated documentation.

## ``pci/util``

* Utility functions for handling SR-IOV devices have been added.

## ``iommu``

* A convenient ``iommu_dmabuf`` public API has been added. An ``iommu_dmabuf``
  abstracts the process of allocating a DMA buffer and mapping it. The buffer
  can be automatically managed with an ``__autovar_s()`` annotation to magically
  unmap and deallocate it when going out of scope.

### Bugfixes and minor improvements

* **BUGFIX**: ``nvme_close`` will now deallocate the doorbell buffer memory.
* **Improvement**: ``nvme_close`` and ``vfio_pci_close`` will now zero their
  respective structs.

## v5.0.0:

### ``nvme_rq``

* ``nvme_rq_mapv_prp()`` now expects the ``struct iov *`` to contain virtual
  addresses and will translate them when building the data pointer PRPs.
* libvfn now supports reating SGLs (and will use them by default if available).
  Use the new helper function ``nvme_rq_mapv()`` to map ``struct iov *``'s. This
  function will use SGLs if supported by the controller or fall back to PRPs.
