# PostScript prolog install directory
pspdir = get_option('datadir') / 'wfdb' / 'psp'

# Simple apps: one .c -> one exe, linked against wfdb
simple_apps = [
  'ann2rr',
  'calsig',
  'ecgeval',
  'fir',
  'gqfuse',
  'gqpost',
  'gqrs',
  'mfilt',
  'mrgann',
  'pscgen',
  'rdann',
  'rdsamp',
  'rr2ann',
  'rxr',
  'sampfreq',
  'sigavg',
  'signame',
  'signum',
  'skewedit',
  'snip',
  'sortann',
  'sqrs',
  'sqrs125',
  'stepdet',
  'sumann',
  'sumstats',
  'tach',
  'time2sec',
  'wabp',
  'wfdb-config',
  'wfdbcat',
  'wfdbcollate',
  'wfdbdesc',
  'wfdbsignals',
  'wfdbtime',
  'wfdbwhich',
  'wrann',
  'wrsamp',
  'xform',
]

foreach app : simple_apps
  executable(app, app + '.c',
    dependencies: wfdb_dep,
    install: true,
  )
endforeach

# Apps that also need -lm
math_apps = [
  'bxb',
  'epicmp',
  'hrstats',
  'ihr',
  'mxm',
  'nguess',
  'nst',
  'sigamp',
  'wqrs',
]

foreach app : math_apps
  executable(app, app + '.c',
    dependencies: [wfdb_dep, m_dep],
    install: true,
  )
endforeach

# wfdbmap depends on signal-colors.h (local header)
executable('wfdbmap', 'wfdbmap.c',
  dependencies: wfdb_dep,
  include_directories: include_directories('.'),
  install: true,
)

# plotstm does NOT link against wfdb (standalone)
executable('plotstm', 'plotstm.c',
  install: true,
)

# pschart and psfd need -DPROLOG pointing to installed .pro path
pschart_prolog = get_option('prefix') / pspdir / 'pschart.pro'
executable('pschart', 'pschart.c',
  c_args: ['-DPROLOG="' + pschart_prolog + '"'],
  dependencies: wfdb_dep,
  install: true,
)

psfd_prolog = get_option('prefix') / pspdir / 'psfd.pro'
executable('psfd', 'psfd.c',
  c_args: ['-DPROLOG="' + psfd_prolog + '"'],
  dependencies: wfdb_dep,
  install: true,
)

# Install PostScript prolog files
install_data(
  'pschart.pro',
  'psfd.pro',
  '12lead.pro',
  install_dir: pspdir,
)

# Install configuration files
install_data(
  'gqrs.conf',
  install_dir: get_option('datadir') / 'wfdb',
)

# Install scripts with DBDIR substitution
script_conf = configuration_data()
script_conf.set('DBDIR', get_option('prefix') / dbdir)

foreach script : ['setwfdb', 'cshsetwfdb', 'pnwlogin']
  configure_file(
    input: script,
    output: script,
    command: ['sed', 's+/usr/local/database+' + get_option('prefix') / dbdir + '+g', '@INPUT@'],
    capture: true,
    install: true,
    install_dir: get_option('bindir'),
    install_mode: 'rwxr-xr-x',
  )
endforeach
