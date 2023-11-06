vfntool_deps = [
  ccan_config_h,
  support_sources,
  pci_sources,
]

executable('vfntool', [vfntool_deps, 'vfntool.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, vfn_inc],
  install: true,
)
