tests_sources = [
  # custom (generated) targets
  trace_events_h,
]

include_bundle_tests = {
  'nvme': ['include-bundle-nvme.c'],
  'vfio': ['include-bundle-vfio.c'],
  'pci': ['include-bundle-pci.c'],
  'support': ['include-bundle-support.c'],
}

foreach name, sources : include_bundle_tests
  test(name, executable(name, [tests_sources, sources],
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
