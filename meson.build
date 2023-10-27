##
##  General
##  -------
##

project('libvfn', ['c', 'cpp'],
  version : '3.0.0',
  license : 'LGPL-2.1-or-later',
  default_options: [
    'warning_level=2',
    'c_std=gnu11',
    'werror=true',
  ],
  meson_version: '>=0.59.0',
)

add_project_arguments([
    '-D_GNU_SOURCE',
  ], language: ['c', 'cpp'],
)

if get_option('profiling')
  add_global_arguments('-pg', language: 'c')
  add_global_link_arguments('-pg', language: 'c')
endif

if get_option('debug')
  add_project_arguments(['-DDEBUG'], language: ['c', 'cpp'])
endif

##
##  Programs
##  --------
##

perl = find_program('perl')
trace_pl = find_program('scripts/trace.pl')

##
##  Compiler Configuration
##  ----------------------
##

cc = meson.get_compiler('c')

# strip relative path prefixes from the code if possible
if cc.has_argument('-fmacro-prefix-map=prefix=')
  add_project_arguments([
    '-fmacro-prefix-map=../=',
    ], language: 'c',
  )
endif


##
##  Host Configuration
##  ------------------
##

config_host = configuration_data()

# library configuration
config_host.set('NVME_AQ_QSIZE', get_option('aq_qsize'),
  description: 'admin command queue size')

config_host.set('HAVE_VFIO_DEVICE_BIND_IOMMUFD',
  cc.has_header_symbol('linux/vfio.h', 'VFIO_DEVICE_BIND_IOMMUFD'),
  description: 'weather VFIO_DEVICE_BIND_IOMMUFD is defined in linux/vfio.h')

subdir('internal')

add_project_arguments([
    '-include', meson.current_build_dir() / 'internal/config-host.h',
  ], language: 'c',
)

##
##  Optional dependencies
##  ---------------------
##

# libnvme
libnvme = dependency('libnvme', version: '>=1.0',
  required: false, disabler: true,
)


##
##  Subdirectories
##  --------------
##

# collect a list of files to use as dependencies for generating api
# documentation
docs_deps = files()

# core
subdir('ccan')
subdir('include')
subdir('lib')
subdir('src')

# tests
subdir('tests')

# examples
subdir('examples')

# tools
subdir('tools/vfntool')

# documentation
subdir('docs')


##
##  Run targets
##  -----------
##

# generate tags
ctags = find_program('ctags', required: false)
if ctags.found()
  run_target('ctags', command: [
    find_program('scripts/ctags.sh')
  ])
endif

# sparse
sparse = find_program('cgcc', required: false)
if sparse.found()
  run_target('sparse', command: [
    find_program('scripts/sparse.py'), 'compile_commands.json', sparse.full_path(),
    '-Wbitwise-pointer', '-Wconstant-suffix', '-Wshadow', '-Wundef', '-Wunion-cast',
    '-Wdeclaration-after-statement',
  ], depends: [trace_events_h, trace_events_c, crc64table_h])
endif


##
##  Summaries
##  ---------
##

summary_info = {}
summary_info += {'Install prefix': get_option('prefix')}
summary_info += {'Binary directory': get_option('bindir')}
summary_info += {'Library directory': get_option('libdir')}
summary_info += {'Include directory': get_option('includedir')}
summary_info += {'Build directory': meson.current_build_dir()}
summary(summary_info, section: 'Directories')

summary_info = {}
summary_info += {'Host CPU': host_machine.cpu_family()}
summary_info += {'Host endianness': host_machine.endian()}
summary_info += {'C compiler': ' '.join(cc.cmd_array())}
summary_info += {'CFLAGS': ' '.join(get_option('c_args')
  + ['-O' + get_option('optimization')]
  + (get_option('debug') ? ['-g'] : [])
  + (get_option('profiling') ? ['-pg'] : []))}
summary_info += {'LDFLAGS': ' '.join(get_option('c_link_args')
  + (get_option('profiling') ? ['-pg'] : []))}
summary(summary_info, section: 'Compilation')

summary_info = {}
summary_info += {'libnvme': libnvme.found()}
summary(summary_info, bool_yn: true, section: 'Dependencies')

summary_info = {}
summary_info += {'Debugging': get_option('debug')}
summary_info += {'Documentation': build_docs}
summary_info += {'Profiling': get_option('profiling')}
summary_info += {'iommufd': config_host.get('HAVE_VFIO_DEVICE_BIND_IOMMUFD')}
summary(summary_info, bool_yn: true, section: 'Features')
