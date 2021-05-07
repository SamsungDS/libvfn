tests_sources = [
  # custom (generated) targets
  ccan_config_h,
  trace_events_h,
]

include_bundle = executable('include-bundle', [tests_sources, 'include-bundle.c'],
  link_with: [vfn_lib],
  include_directories: [vfn_inc],
)

cpp = executable('cpp', [tests_sources, 'cpp.cc'],
  cpp_args: ['-Wno-pointer-arith'],
  link_with: [vfn_lib],
  include_directories: [vfn_inc],
)

test('include-bundle', include_bundle)
test('cpp', cpp)
