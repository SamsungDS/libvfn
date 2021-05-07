vfn_headers = files([
  'atomic.h',
  'compiler.h',
  'endian.h',
  'nvme.h',
  'trace.h',
  'vfio.h'
])

install_headers(vfn_headers, subdir: 'vfn')

docs_deps += vfn_headers

subdir('nvme')
subdir('pci')
subdir('trace')
subdir('vfio')
