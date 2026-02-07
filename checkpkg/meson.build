# Build lcheck test executable
lcheck_c_args = []
if libcurl_dep.found()
  lcheck_c_args += '-DWFDB_NETFILES=1'
endif

lcheck = executable('lcheck', 'lcheck.c',
  c_args: lcheck_c_args,
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

build_lib_dir = meson.current_build_dir() / '..' / 'lib'
build_app_dir = meson.current_build_dir() / '..' / 'app'
build_convert_dir = meson.current_build_dir() / '..' / 'convert'

# Use environment() to prepend to PATH (not replace it)
libcheck_env = environment()
libcheck_env.set('WFDB_NO_NET_CHECK', '1')
libcheck_env.prepend('LD_LIBRARY_PATH', build_lib_dir)

# Register libcheck test -- pass lcheck binary path as 3rd arg
test('libcheck',
  find_program('bash'),
  args: ['-c', 'cd "@0@" && ./libcheck "@1@" "@2@" "@3@"'.format(
    meson.current_source_dir(), installed_dbdir, installed_libdir,
    lcheck.full_path())],
  env: libcheck_env,
  depends: lcheck,
  timeout: 60,
  is_parallel: false,
)

# Use environment() to prepend to PATH (not replace it)
appcheck_env = environment()
appcheck_env.set('WFDB_NO_NET_CHECK', '1')
appcheck_env.prepend('LD_LIBRARY_PATH', build_lib_dir)
appcheck_env.prepend('PATH', build_app_dir)
appcheck_env.prepend('PATH', build_convert_dir)

# Register appcheck test
test('appcheck',
  find_program('bash'),
  args: ['-c', 'cd "@0@" && ./appcheck "@1@" "@2@" "@3@" "@4@"'.format(
    meson.current_source_dir(),
    installed_incdir,
    installed_bindir,
    installed_libdir,
    installed_pspdir)],
  env: appcheck_env,
  timeout: 120,
  is_parallel: false,
)
