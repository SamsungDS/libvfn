trace_events_c = custom_target('trace events source',
  input: meson.project_source_root() / get_option('trace-events-file'),
  output: 'events.c',
  capture: true,
  command: [trace_pl, '--mode', 'source', '@INPUT@'],
)

core_sources += trace_events_c
