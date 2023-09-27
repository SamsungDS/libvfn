vfio_sources = files(
  'core.c',
  'device.c',
  'pci.c',
)

if not config_host.get('HAVE_VFIO_DEVICE_BIND_IOMMUFD', false)
  as_sources += files('container.c',)
endif

# tests
iommu_test = executable('iommu_test',
  [gen_sources, vfn_sources, as_sources, vfio_sources, 'iommu_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

vfio_sources += files(
  'iommu.c',
)

vfn_sources += [vfio_sources, as_sources]

test('iommu_test', iommu_test)
