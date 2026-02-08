# WAVE companion tools (standalone C, no wfdb dependency)

executable('wave-remote', 'wave-remote.c', install: true)
executable('wavescript', 'wavescript.c', install: true)

# Developer test tool — build but don't install
executable('wave-remote-test', 'wave-remote-test.c', install: false)

# Shell script
install_data('url_view',
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)
