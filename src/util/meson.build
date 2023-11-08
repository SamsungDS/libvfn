# tests
skiplist_test = executable('skiplist_test', [ccan_config_h, support_sources, 'skiplist_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, vfn_inc],
)

test('skiplist_test', skiplist_test)

vfn_sources += files(
  'skiplist.c',
)
