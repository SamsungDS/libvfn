iommu_sources = files(
  'context.c',
  'dma.c',
  'vfio.c',
)

if config_host.get('HAVE_VFIO_DEVICE_BIND_IOMMUFD', false)
  iommu_sources += files('iommufd.c',)
endif

vfn_sources += iommu_sources
