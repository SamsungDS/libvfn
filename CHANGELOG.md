# Changelog

## v5.0.0: (unreleased)

### ``nvme_rq``

``nvme_rq_mapv_prp()`` now expects the ``struct iov *`` to contain virtual
addresses and will translate them when building the data pointer PRPs.

libvfn now supports reating SGLs (and will use them by default if available).
Use the new helper function ``nvme_rq_mapv()`` to map ``struct iov *``'s. This
function will use SGLs if supported by the controller or fall back to PRPs.
