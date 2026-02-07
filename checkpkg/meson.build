# Build lcheck test executable
lcheck = executable('lcheck', 'lcheck.c',
  dependencies: wfdb_dep,
  install: false,
)

# Collect all app executables for test dependencies
# (tests need apps to be built first)
prefix = get_option('prefix')
installed_bindir = prefix / get_option('bindir')
installed_libdir = prefix / get_option('libdir')
installed_incdir = prefix / get_option('includedir')
installed_dbdir = prefix / dbdir
installed_pspdir = prefix / get_option('datadir') / 'wfdb' / 'psp'

# Register libcheck test
test('libcheck',
  find_program('bash'),
  args: ['-c', 'cd "@0@" && ./libcheck "@1@" "@2@"'.format(
    meson.current_source_dir(), installed_dbdir, installed_libdir)],
  env: {
    'WFDB_NO_NET_CHECK': '1',
    'LD_LIBRARY_PATH': meson.current_build_dir() / '..' / 'lib',
  },
  depends: lcheck,
  timeout: 60,
  is_parallel: false,
)

# Register appcheck test
test('appcheck',
  find_program('bash'),
  args: ['-c', 'cd "@0@" && ./appcheck "@1@" "@2@" "@3@" "@4@"'.format(
    meson.current_source_dir(),
    installed_incdir,
    installed_bindir,
    installed_libdir,
    installed_pspdir)],
  env: {
    'WFDB_NO_NET_CHECK': '1',
    'LD_LIBRARY_PATH': meson.current_build_dir() / '..' / 'lib',
    'PATH': meson.current_build_dir() / '..' / 'app' + ':' +
            meson.current_build_dir() / '..' / 'convert',
  },
  timeout: 120,
  is_parallel: false,
)
