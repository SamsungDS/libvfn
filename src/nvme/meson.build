crc64table_h = custom_target('crc64table_h',
  output: 'crc64table.h',
  command: [gentable_crc64],
  capture: true,
  build_by_default: true,
)

gen_sources += crc64table_h

nvme_sources = files(
  'core.c',
  'queue.c',
  'util.c',
)

# tests
rq_test = executable('rq_test', [gen_sources, vfn_sources, nvme_sources, 'rq_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

nvme_sources += files(
  'rq.c',
)

vfn_sources += nvme_sources

test('rq_test', rq_test)
