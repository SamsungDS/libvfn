# tests
iova_map_list_test = executable('iova_map_list_test',
  [gen_sources, vfn_sources, 'iova_map_list_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

test('iova_map_list_test', iova_map_list_test)

vfn_sources += files(
  'iova_map.c',
)
