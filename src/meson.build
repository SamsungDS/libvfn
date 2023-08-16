core_inc = include_directories('.')

gen_sources = [
  # custom (generated) targets
  config_host_h,
  ccan_config_h,
  trace_events_h,
]

trace_sources = files(
  'trace.c',
)

vfn_sources = trace_sources

subdir('support')
subdir('trace')
subdir('pci')
subdir('vfio')
subdir('nvme')

docs_deps += vfn_sources

_vfn_sources = [
  gen_sources,
  vfn_sources,
]

thread_dep = dependency('threads')

vfn_lib = library('vfn', _vfn_sources,
  dependencies: [thread_dep],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
  version: meson.project_version(),
  install: true,
  c_args: [
    # add some useful warnings
    '-Wconversion', '-Wdouble-promotion', '-Wundef', '-Wno-trigraphs',

    # warn about c99 declaration after statement
    '-Wdeclaration-after-statement',

    # variable-length arrays are a source of bugs
    '-Wvla',

    # sign conversions are useful and generally not a source of bugs
    '-Wno-sign-conversion',

    # don't do bad things on integer overflow
    '-fno-strict-overflow',

    # add -g3 for debug (more symbol information)
    get_option('buildtype') == 'debug' ? ['-g3'] : [],
  ],
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
