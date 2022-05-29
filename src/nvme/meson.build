crc64table_h = custom_target('crc64table_h',
  output: 'crc64table.h',
  command: [gentable_crc64],
  capture: true,
  build_by_default: true,
)

core_sources += crc64table_h

vfn_sources += files(
  'core.c',
  'queue.c',
  'rq.c',
  'util.c',
)

# tests
rq_test = executable('rq_test', [core_sources, 'queue.c', 'util.c', 'rq_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

test('rq_test', rq_test)
