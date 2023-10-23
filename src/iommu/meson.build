iommu_sources = files(
  'dma.c',
)

if config_host.get('HAVE_VFIO_DEVICE_BIND_IOMMUFD', false)
  iommu_sources += files('iommufd.c',)
else
  iommu_sources += files('vfio.c')
endif

vfn_sources += iommu_sources
