vfn_sources += files(
  'core.c',
  'iommu.c',
  'pci.c',
)

# tests
iommu_test = executable('iommu_test', [core_sources, 'iommu_test.c'],
  link_with: [ccan_lib, support_lib],
  include_directories: [ccan_inc, support_inc, core_inc, vfn_inc],
)

test('iommu_test', iommu_test)
