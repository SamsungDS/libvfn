example_sources = [
  files('common.c'),

  # custom (generated) targets
  ccan_config_h,
  trace_events_h,
]

examples = {
  'cmb': ['cmb.c'],
  'cmb-p2p': ['cmb-p2p.c'],
  'eventfd': ['eventfd.c'],
  'identify': ['identify.c'],
  'io': ['io.c'],
  'perf': ['perf.c'],
  'regs': ['regs.c'],
  'sriov': ['sriov.c'],
}

if config_host.get('HAVE_IOMMU_FAULT_QUEUE_ALLOC', false)
  examples += {'iopf': ['iopf.c']}
endif

foreach example, sources : examples
  executable(example, [example_sources, sources],
    dependencies: [libnvme],
    link_with: [ccan_lib, vfn_lib],
    include_directories: [ccan_inc, vfn_inc, linux_headers],
  )
endforeach
