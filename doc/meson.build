# Man pages from doc/wag-src/
man1_pages = files(
  'wag-src/a2m.1',
  'wag-src/ann2rr.1',
  'wag-src/bxb.1',
  'wag-src/calsig.1',
  'wag-src/coherence.1',
  'wag-src/dfa.1',
  'wag-src/ecgeval.1',
  'wag-src/ecgpuwave.1',
  'wag-src/edf2mit.1',
  'wag-src/edr.1',
  'wag-src/epicmp.1',
  'wag-src/fft.1',
  'wag-src/fir.1',
  'wag-src/gqfuse.1',
  'wag-src/gqrs.1',
  'wag-src/hrfft.1',
  'wag-src/hrstats.1',
  'wag-src/ihr.1',
  'wag-src/imageplt.1',
  'wag-src/log10.1',
  'wag-src/lomb.1',
  'wag-src/lwcat.1',
  'wag-src/memse.1',
  'wag-src/mfilt.1',
  'wag-src/mrgann.1',
  'wag-src/mxm.1',
  'wag-src/nguess.1',
  'wag-src/nst.1',
  'wag-src/parsescp.1',
  'wag-src/plot2d.1',
  'wag-src/plotstm.1',
  'wag-src/plt.1',
  'wag-src/pltf.1',
  'wag-src/pnnlist.1',
  'wag-src/pnwlogin.1',
  'wag-src/pschart.1',
  'wag-src/psfd.1',
  'wag-src/rdann.1',
  'wag-src/rdedfann.1',
  'wag-src/rdsamp.1',
  'wag-src/rxr.1',
  'wag-src/sampfreq.1',
  'wag-src/setwfdb.1',
  'wag-src/sigamp.1',
  'wag-src/sigavg.1',
  'wag-src/signame.1',
  'wag-src/signum.1',
  'wag-src/skewedit.1',
  'wag-src/snip.1',
  'wag-src/sortann.1',
  'wag-src/sqrs.1',
  'wag-src/stepdet.1',
  'wag-src/sumann.1',
  'wag-src/sumstats.1',
  'wag-src/tach.1',
  'wag-src/time2sec.1',
  'wag-src/wabp.1',
  'wag-src/wav2mit.1',
  'wag-src/wave.1',
  'wag-src/wfdb2mat.1',
  'wag-src/wfdbcat.1',
  'wag-src/wfdbcollate.1',
  'wag-src/wfdb-config.1',
  'wag-src/wfdbdesc.1',
  'wag-src/wfdbmap.1',
  'wag-src/wfdbtime.1',
  'wag-src/wfdbwhich.1',
  'wag-src/wqrs.1',
  'wag-src/wrann.1',
  'wag-src/wrsamp.1',
  'wag-src/xform.1',
)

man3_pages = files(
  'wag-src/wfdb.3',
  'wag-src/wfdbf.3',
)

man5_pages = files(
  'wag-src/annot.5',
  'wag-src/header.5',
  'wag-src/signal.5',
  'wag-src/wfdbcal.5',
)

man7_pages = files(
  'wag-src/xview.7',
)

install_man(man1_pages, man3_pages, man5_pages, man7_pages)

# Symlinks for aliased commands.
# Meson's install_symlink requires Meson >= 0.61.
mandir = get_option('mandir')
man1dir = mandir / 'man1'

aliases = {
  'a2m.1': ['ad2m.1', 'ahaconvert.1', 'ahaecg2mit.1', 'm2a.1', 'md2a.1'],
  'ann2rr.1': ['rr2ann.1'],
  'edf2mit.1': ['mit2edf.1'],
  'gqrs.1': ['gqpost.1'],
  'hrfft.1': ['hrlomb.1', 'hrmem.1', 'hrplot.1'],
  'plot2d.1': ['plot3d.1'],
  'pnnlist.1': ['pNNx.1'],
  'setwfdb.1': ['cshsetwfdb.1'],
  'wav2mit.1': ['mit2wav.1'],
}

foreach target, links : aliases
  foreach link : links
    install_symlink(link,
      pointing_to: target,
      install_dir: man1dir,
    )
  endforeach
endforeach
