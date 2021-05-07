example_sources = [
  files('common.c'),

  # custom (generated) targets
  ccan_config_h,
  trace_events_h,
]

examples = {
  'aer': ['aer.c'],
  'cmb': ['cmb.c'],
  'identify': ['identify.c'],
  'io': ['io.c'],
  'perf': ['perf.c'],
  'regs': ['regs.c'],
}

foreach example, sources : examples
  executable(example, [example_sources, sources],
    dependencies: [libnvme],
    link_with: [ccan_lib, support_lib, vfn_lib],
    include_directories: [ccan_inc, support_inc, vfn_inc],
    install: true,
    install_dir: 'bin/vfn',
  )
endforeach
