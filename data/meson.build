# Install data files to dbdir
install_data(
  '100a.atr', '100a.hea',
  '100s.atr', '100s.dat', '100s.hea',
  '16.hea', '16l.hea', '8.hea', '8l.hea',
  'multi.hea', 'null.hea',
  'ahalist', 'ahaxlist', 'culist', 'dblist',
  'esclist', 'mitlist', 'mitxlist', 'nstlist',
  'wfdbcal',
  install_dir: dbdir,
)

# Install pipe/*.hea
install_data(
  'pipe/8x1.hea', 'pipe/8x2.hea', 'pipe/8x3.hea', 'pipe/8x4.hea',
  'pipe/8x5.hea', 'pipe/8x6.hea', 'pipe/8x7.hea', 'pipe/8x8.hea',
  'pipe/8x9.hea', 'pipe/8x10.hea', 'pipe/8x11.hea', 'pipe/8x12.hea',
  'pipe/8x13.hea', 'pipe/8x14.hea', 'pipe/8x15.hea', 'pipe/8x16.hea',
  'pipe/16x1.hea', 'pipe/16x2.hea', 'pipe/16x3.hea', 'pipe/16x4.hea',
  'pipe/16x5.hea', 'pipe/16x6.hea', 'pipe/16x7.hea', 'pipe/16x8.hea',
  'pipe/16x9.hea', 'pipe/16x10.hea', 'pipe/16x11.hea', 'pipe/16x12.hea',
  'pipe/16x13.hea', 'pipe/16x14.hea', 'pipe/16x15.hea', 'pipe/16x16.hea',
  install_dir: dbdir / 'pipe',
)

# Install tape/*.hea
install_data(
  'tape/512.hea', 'tape/1024.hea', 'tape/4096.hea', 'tape/10240.hea',
  'tape/6144d.hea', 'tape/ahatape.hea', 'tape/mittape.hea',
  install_dir: dbdir / 'tape',
)

# Create dbcal symlink to wfdbcal
install_symlink('dbcal',
  pointing_to: 'wfdbcal',
  install_dir: dbdir,
)
