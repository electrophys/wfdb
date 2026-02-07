project('wfdb', 'c',
  version: '10.7.0',
  license: 'LGPL-2.1-or-later',
  default_options: ['c_std=gnu11', 'warning_level=2'])

wfdb_version = meson.project_version().split('.')
wfdb_major = wfdb_version[0]
wfdb_minor = wfdb_version[1]
wfdb_release = wfdb_version[2]

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

# Optional dependencies
libcurl_dep = dependency('libcurl', required: get_option('netfiles'))
libflac_dep = dependency('flac', required: get_option('flac'))

subdir('lib')
subdir('app')
subdir('convert')
subdir('psd')
subdir('data')
subdir('checkpkg')
