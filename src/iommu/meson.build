vfn_sources += files(
  'dma.c',
)

as_sources = []
if config_host.get('HAVE_VFIO_DEVICE_BIND_IOMMUFD', false)
  as_sources += files('iommufd.c',)
endif
