tests_sources = [
  # custom (generated) targets
  trace_events_h,
]

include_bundle_tests = {
  'support': ['include-bundle-support.c'],
  'pci': ['include-bundle-pci.c'],
  'iommu': ['include-bundle-iommu.c'],
  'vfio': ['include-bundle-vfio.c'],
  'nvme': ['include-bundle-nvme.c'],
}

foreach name, sources : include_bundle_tests
  test('include-bundle-' + name, executable(name, [tests_sources, sources],
    link_with: [vfn_lib],
    include_directories: [vfn_inc],
  ))
endforeach

cpp = executable('cpp', [tests_sources, 'cpp.cc'],
  cpp_args: ['-Wno-pointer-arith'],
  link_with: [vfn_lib],
  include_directories: [vfn_inc],
)

test('cpp', cpp)

# device tests
subdir('device')
