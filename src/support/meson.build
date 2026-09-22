support_sources = files(
  'io.c',
  'log.c',
  'mem.c',
  'mmio.c',
  'ticks.c',
  'timer.c',
)

if host_machine.cpu_family() == 'x86_64'
  subdir('arch/x86_64')
elif host_machine.cpu_family() == 's390x'
  subdir('arch/s390x')
endif

vfn_sources += support_sources

ticks_test = executable('ticks_test', [support_sources, 'ticks_test.c'],
  link_with: [ccan_lib],
  include_directories: [ccan_inc, core_inc, vfn_inc],
)

test('ticks_test', ticks_test, protocol: 'tap')
