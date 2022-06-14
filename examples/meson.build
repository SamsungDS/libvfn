build_examples = false

examples_opt = get_option('examples')
libnvme_opt = get_option('libnvme')

if not examples_opt.disabled()
  if libnvme_opt in ['auto', 'system']
    libnvme = dependency('libnvme',
      static: get_option('profiling'),
      required: libnvme_opt == 'system' and examples_opt,
      allow_fallback: false
    )

    if libnvme.found()
      libnvme_opt = 'system'
      build_examples = true
    elif libnvme_opt == 'auto'
      libnvme_opt = 'internal'
    endif
  endif

  if libnvme_opt == 'internal'
    libnvme_proj = subproject('libnvme',
      default_options: [
        'warning_level=1',

        # always choose a static version for embedded builds
        'default_library=static',

        # override project options
        'docs-build=false',
        'python=false',
      ],
      required: examples_opt,
    )

    if libnvme_proj.found()
      libnvme = libnvme_proj.get_variable('libnvme_dep')
      build_examples = true
    endif
  endif

  if build_examples
    example_sources = [
      files('common.c'),

      # custom (generated) targets
      ccan_config_h,
      trace_events_h,
    ]

    examples = {
      'aer': ['aer.c'],
      'cmb': ['cmb.c'],
      'identify': ['identify.c'],
      'io': ['io.c'],
      'perf': ['perf.c'],
      'regs': ['regs.c'],
    }

    foreach example, sources : examples
      executable(example, [example_sources, sources],
        dependencies: [libnvme],
        link_with: [ccan_lib, vfn_lib],
        include_directories: [ccan_inc, vfn_inc],
      )
    endforeach
  endif
endif
