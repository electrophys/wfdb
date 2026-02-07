# All converters: one .c -> one exe, linked against wfdb
converters = [
  'a2m',
  'ad2m',
  'ahaecg2mit',
  'edf2mit',
  'm2a',
  'makeid',
  'md2a',
  'mit2edf',
  'mit2wav',
  'parsescp',
  'rdedfann',
  'readid',
  'revise',
  'wav2mit',
  'wfdb2mat',
]

foreach app : converters
  executable(app, app + '.c',
    dependencies: wfdb_dep,
    install: true,
  )
endforeach
