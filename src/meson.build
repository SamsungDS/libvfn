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

subdir('support')
subdir('trace')
subdir('nvme')
subdir('pci')
subdir('vfio')

docs_deps += vfn_sources

_vfn_sources = [
  core_sources,
  vfn_sources,
]

thread_dep = dependency('threads')

vfn_lib = library('vfn', _vfn_sources,
  dependencies: [thread_dep],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
  version: meson.project_version(),
  install: true,
)

libvfn_dep = declare_dependency(
  link_with: vfn_lib,
  include_directories: vfn_inc,

  # generated headers
  sources: trace_events_h,
)

meson.override_dependency('libvfn', libvfn_dep)

pkg = import('pkgconfig')
pkg.generate(vfn_lib,
  filebase: 'libvfn',
)
