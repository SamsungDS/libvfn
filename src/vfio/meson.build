vfn_sources += files(
  'core.c',
  'device.c',
  'iommu.c',
  'pci.c',
)

# tests
iommu_test = executable('iommu_test', [trace_sources, support_sources, 'iommu_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

test('iommu_test', iommu_test)
