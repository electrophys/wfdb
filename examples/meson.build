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
