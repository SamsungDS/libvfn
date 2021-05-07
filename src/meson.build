core_inc = include_directories('.')

core_sources = [
  # custom (generated) targets
  config_host_h,
  ccan_config_h,
  trace_events_h,
]

vfn_sources = files(
  'trace.c',
)

subdir('trace')
subdir('nvme')
subdir('pci')
subdir('vfio')

docs_deps += vfn_sources

_vfn_sources = [
  core_sources,
  vfn_sources,
]

vfn_lib = library('vfn', _vfn_sources,
  dependencies: [libnvme],
  link_with: [ccan_lib, support_lib],
  include_directories: [ccan_inc, support_inc, core_inc, vfn_inc],
  version: meson.project_version(),
  install: true,
)

libvfn_dep = declare_dependency(
  link_with: vfn_lib,
  include_directories: vfn_inc,
)

pkg = import('pkgconfig')
pkg.generate(vfn_lib,
  filebase: 'libvfn',
)
