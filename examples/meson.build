# Example programs from the WFDB Programmer's Guide.
# Built as a compile check; not installed.

examples = [
  'example1',
  'example2',
  'example3',
  'example4',
  'example5',
  'example6',
  'example7',
  'example8',
  'example9',
  'example10',
  'exannstr',
  'exgetann',
  'exgetvec',
  'exputvec',
  'pgain',
  'psamples',
  'psamplex',
  'refhr',
  'stdev',
  'wfdbversion',
]

foreach app : examples
  executable(app, app + '.c',
    dependencies: wfdb_dep,
    install: false,
  )
endforeach

# Thread-safe context API example (requires pthread)
thread_dep = dependency('threads', required: false)
if thread_dep.found()
  executable('parallel_stats', 'parallel_stats.c',
    dependencies: [wfdb_dep, thread_dep],
    install: false,
  )
endif
