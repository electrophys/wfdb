project('wfdb', 'c',
  version: '11.0.0',
  license: 'LGPL-2.1-or-later',
  default_options: ['c_std=gnu11', 'warning_level=2'])

wfdb_version = meson.project_version().split('.')
wfdb_major = wfdb_version[0]
wfdb_minor = wfdb_version[1]
wfdb_release = wfdb_version[2]

cc = meson.get_compiler('c')
fs = import('fs')
m_dep = cc.find_library('m', required: false)

# Optional dependencies
libcurl_dep = dependency('libcurl', version: '>=7.55.0', required: get_option('netfiles'))
libflac_dep = dependency('flac', required: get_option('flac'))
libexpat_dep = dependency('expat', required: get_option('expat'))

subdir('lib')
subdir('app')
subdir('convert')
subdir('psd')
subdir('data')
subdir('xml')
subdir('wave')
if gtk3_dep.found() and vte_dep.found()
  subdir('waverc')
endif
subdir('examples')
subdir('doc')
subdir('checkpkg')
