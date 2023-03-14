build_docs = false

sphinx_build = find_program('sphinx-build', required: get_option('docs'))

if sphinx_build.found()
  build_docs = true

  SPHINX_ARGS = [sphinx_build]

  if get_option('werror')
    SPHINX_ARGS += ['-W', '-Dkerneldoc_werror=1']
  endif

  docs = custom_target('libvfn manual',
    build_by_default: build_docs,
    output: 'docs.stamp',
    input: files('conf.py'),
    depfile: 'docs.d',
    depend_files: docs_deps,
    command: [SPHINX_ARGS, '-q', '-Ddepfile=@DEPFILE@', '-Ddepfile_stamp=@OUTPUT@',
      '-b', 'html', '-d', meson.current_build_dir() / 'manual.p',
      meson.current_source_dir(), meson.current_build_dir() / 'manual',
    ],
  )
endif
