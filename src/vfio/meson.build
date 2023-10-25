vfio_sources = files(
  'device.c',
  'pci.c',
)

if not config_host.get('HAVE_VFIO_DEVICE_BIND_IOMMUFD', false)
  as_sources += files('container.c',)
endif

vfn_sources += [vfio_sources, as_sources]
