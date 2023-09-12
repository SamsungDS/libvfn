support_sources = files(
  'io.c',
  'log.c',
  'ticks.c',
  'timer.c',
)

subdir('platform/linux')

if host_machine.cpu_family() == 'x86_64'
  subdir('arch/x86_64')
endif

vfn_sources += support_sources

ticks_test = executable('ticks_test', [support_sources, 'ticks_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

test('ticks_test', ticks_test)
