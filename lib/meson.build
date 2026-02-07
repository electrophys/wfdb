# Determine database directory
if get_option('dbdir') != ''
  dbdir = get_option('dbdir')
else
  dbdir = get_option('datadir') / 'wfdb'
endif

# Build-config values passed as -D compiler flags to the library.
# Version macros are also in wfdb.h for external consumers; the -D flags
# take precedence during library compilation.
lib_c_args = [
  '-DWFDB_MAJOR=' + wfdb_major,
  '-DWFDB_MINOR=' + wfdb_minor,
  '-DWFDB_RELEASE=' + wfdb_release,
  '-DDBDIR="' + (get_option('prefix') / dbdir) + '"',
]

if libcurl_dep.found()
  lib_c_args += '-DWFDB_NETFILES=1'
endif
if libflac_dep.found()
  lib_c_args += '-DWFDB_FLAC_SUPPORT'
endif

# Apps use #include <wfdb/wfdb.h>.  Headers live in the source tree's lib/,
# so we create a symlink build/wfdb -> <source>/lib at configure time.
# We also copy ecgcodes.h and ecgmap.h to build/lib/ so the symlink from
# build/ works for all headers.
build_lib_dir = meson.current_build_dir()
build_root = meson.project_build_root()
wfdb_link = build_root / 'wfdb'

# Copy source headers to build/lib/ so the build/wfdb symlink works
# for apps that #include <wfdb/wfdb.h>.
foreach hdr : ['wfdb.h', 'wfdblib.h', 'ecgcodes.h', 'ecgmap.h']
  configure_file(
    input: hdr,
    output: hdr,
    copy: true,
  )
endforeach

# Create symlink: build/wfdb -> lib (relative)
run_command('sh', '-c',
  'ln -snf lib "' + wfdb_link + '"',
  check: true,
)

# Include path for both library internals and app consumers
# - include_directories('.') gives -I<build>/lib and -I<source>/lib
#   (for library sources that #include "wfdb.h" or "signal_internal.h")
# - build_root_inc gives -I<build>/ (for apps using #include <wfdb/wfdb.h>
#   which resolves via the build/wfdb -> build/lib symlink)
lib_inc = include_directories('.')
build_root_inc = include_directories('..')

# Library sources (excluding wfdbio.c which needs special defines)
lib_sources = files(
  'wfdbinit.c',
  'annot.c',
  'signal.c',
  'header.c',
  'sigformat.c',
  'flac.c',
  'sigmap.c',
  'timeconv.c',
  'calib.c',
  'wfdb_context.c',
)

# Collect dependency objects
lib_deps = [m_dep]

if libcurl_dep.found()
  lib_deps += libcurl_dep
endif
if libflac_dep.found()
  lib_deps += libflac_dep
endif

# wfdbio.c needs special compile-time defines
wfdbio_c_args = lib_c_args + [
  '-DVERSION="' + meson.project_version() + '"',
  '-DCFLAGS="-I' + (get_option('prefix') / get_option('includedir') / 'wfdb') + '"',
  '-DLDFLAGS="-L' + (get_option('prefix') / get_option('libdir')) + ' -lwfdb"',
]

wfdbio_obj = static_library('wfdbio_obj',
  'wfdbio.c',
  c_args: wfdbio_c_args,
  dependencies: lib_deps,
  include_directories: [lib_inc, build_root_inc],
  pic: true,
)

# Shared library
libwfdb = shared_library('wfdb',
  lib_sources,
  c_args: lib_c_args,
  dependencies: lib_deps,
  link_whole: wfdbio_obj,
  include_directories: [lib_inc, build_root_inc],
  version: meson.project_version(),
  soversion: wfdb_major,
  install: true,
)

# Static library
libwfdb_static = static_library('wfdb',
  lib_sources,
  c_args: lib_c_args,
  dependencies: lib_deps,
  link_whole: wfdbio_obj,
  include_directories: [lib_inc, build_root_inc],
  install: true,
)

# Declare dependency for use by subdirectories.
# - lib_inc: for internal headers (wfdblib.h, signal_internal.h, etc.)
# - build_root_inc: for <wfdb/wfdb.h> via build/wfdb -> <source>/lib symlink
wfdb_dep = declare_dependency(
  link_with: libwfdb,
  include_directories: [lib_inc, build_root_inc],
  dependencies: lib_deps,
)

# Install public headers
install_headers(
  'wfdb.h',
  'ecgcodes.h',
  'ecgmap.h',
  'wfdblib.h',
  subdir: 'wfdb',
)
