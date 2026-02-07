# PSD tools: standalone executables linked against -lm only (NOT wfdb)
psd_apps = ['coherence', 'fft', 'log10', 'lomb', 'memse']

foreach app : psd_apps
  executable(app, app + '.c',
    dependencies: m_dep,
    install: true,
  )
endforeach

# Install shell scripts with BINDIR substitution
bindir_path = get_option('prefix') / get_option('bindir')
scripts_with_bindir = ['hrfft', 'hrlomb', 'hrmem', 'hrplot']

foreach script : scripts_with_bindir
  configure_file(
    input: script,
    output: script,
    command: ['sed', 's+BINDIR+' + bindir_path + '+g', '@INPUT@'],
    capture: true,
    install: true,
    install_dir: get_option('bindir'),
    install_mode: 'rwxr-xr-x',
  )
endforeach

# Install scripts that need no substitution
install_data(
  'plot2d',
  'plot3d',
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)
