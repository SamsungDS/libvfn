# Changelog

## v6.0.0: (unreleased)

### ``nvme_ctrl``

``nvme_pci_init`` has been deprecated and will generate a warning.

## v5.2.0: (unreleased)

### ``nvme_ctrl``

A new set of functions for keeping track of controllers has been added to the
public API. These are ``nvme_{get,add,del}_ctrl``. Please see the updated
documentation.

### Bugfixes and minor improvements

* **BUGFIX**: ``nvme_close`` will now deallocate the doorbell buffer memory.
* **Improvement**: ``nvme_close`` and ``vfio_pci_close`` will now zero their
  respective structs.

## v5.0.0:

### ``nvme_rq``

``nvme_rq_mapv_prp()`` now expects the ``struct iov *`` to contain virtual
addresses and will translate them when building the data pointer PRPs.

libvfn now supports reating SGLs (and will use them by default if available).
Use the new helper function ``nvme_rq_mapv()`` to map ``struct iov *``'s. This
function will use SGLs if supported by the controller or fall back to PRPs.
