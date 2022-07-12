vfn_support_headers = files([
  'align.h',
  'atomic.h',
  'autoptr.h',
  'barrier.h',
  'compiler.h',
  'endian.h',
  'io.h',
  'log.h',
  'mem.h',
  'mmio.h',
  'mutex.h',
  'ticks.h',
  'timer.h',
])

install_headers(vfn_support_headers, subdir: 'vfn/support')

docs_deps += vfn_support_headers

if host_machine.cpu_family() == 'x86_64'
  subdir('arch/x86_64')
elif host_machine.cpu_family() == 'aarch64'
  subdir('arch/arm64')
endif
