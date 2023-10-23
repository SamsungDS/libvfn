vfn_headers = files([
  'nvme.h',
  'pci.h',
  'support.h',
  'trace.h',
  'iommu.h',
  'vfio.h',
])

install_headers(vfn_headers, subdir: 'vfn')

docs_deps += vfn_headers

subdir('support')
subdir('nvme')
subdir('pci')
subdir('trace')
subdir('iommu')
subdir('vfio')
