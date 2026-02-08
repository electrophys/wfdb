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

install_man(man1_pages, man3_pages, man5_pages)

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

# ---------- Optional HTML documentation ----------

docs_opt = get_option('docs')
docdir = get_option('datadir') / 'doc' / 'wfdb'

mandoc = find_program('mandoc', required: docs_opt)
asciidoctor = find_program('asciidoctor', required: docs_opt)

# Man page names for mandoc HTML conversion
man_page_names = [
  'a2m.1', 'ann2rr.1', 'bxb.1', 'calsig.1', 'coherence.1', 'dfa.1',
  'ecgeval.1', 'ecgpuwave.1', 'edf2mit.1', 'edr.1', 'epicmp.1',
  'fft.1', 'fir.1', 'gqfuse.1', 'gqrs.1', 'hrfft.1', 'hrstats.1',
  'ihr.1', 'imageplt.1', 'log10.1', 'lomb.1', 'lwcat.1', 'memse.1',
  'mfilt.1', 'mrgann.1', 'mxm.1', 'nguess.1', 'nst.1', 'parsescp.1',
  'plot2d.1', 'plotstm.1', 'plt.1', 'pltf.1', 'pnnlist.1',
  'pnwlogin.1', 'pschart.1', 'psfd.1', 'rdann.1', 'rdedfann.1',
  'rdsamp.1', 'rxr.1', 'sampfreq.1', 'setwfdb.1', 'sigamp.1',
  'sigavg.1', 'signame.1', 'signum.1', 'skewedit.1', 'snip.1',
  'sortann.1', 'sqrs.1', 'stepdet.1', 'sumann.1', 'sumstats.1',
  'tach.1', 'time2sec.1', 'wabp.1', 'wav2mit.1', 'wave.1',
  'wfdb2mat.1', 'wfdbcat.1', 'wfdbcollate.1', 'wfdb-config.1',
  'wfdbdesc.1', 'wfdbmap.1', 'wfdbtime.1', 'wfdbwhich.1', 'wqrs.1',
  'wrann.1', 'wrsamp.1', 'xform.1',
  'wfdb.3', 'wfdbf.3',
  'annot.5', 'header.5', 'signal.5', 'wfdbcal.5',
]

# Convert man pages to HTML using mandoc
if mandoc.found()
  foreach page : man_page_names
    html_name = page + '.html'
    custom_target('html-' + page.replace('.', '-'),
      input: 'wag-src' / page,
      output: html_name,
      command: [mandoc, '-T', 'html',
                '-O', 'style=mandoc.css,man=%N.%S.html',
                '@INPUT@'],
      capture: true,
      install: true,
      install_dir: docdir / 'wag',
    )
  endforeach
endif

# Build AsciiDoc pages
if asciidoctor.found()
  wag_adoc_pages = ['intro', 'faq', 'install', 'eval', 'index']
  foreach page : wag_adoc_pages
    custom_target('wag-' + page,
      input: 'wag-src' / page + '.adoc',
      output: page + '.html',
      command: [asciidoctor, '-b', 'html5', '-D', '@OUTDIR@',
                '-o', '@OUTPUT@', '@INPUT@'],
      install: true,
      install_dir: docdir / 'wag',
    )
  endforeach

  # WPG (Programmer's Guide)
  if fs.is_file('wpg-src/wpg.adoc')
    custom_target('wpg',
      input: 'wpg-src/wpg.adoc',
      output: 'wpg.html',
      command: [asciidoctor, '-b', 'html5', '-D', '@OUTDIR@',
                '-o', '@OUTPUT@', '@INPUT@'],
      install: true,
      install_dir: docdir / 'wpg',
    )
  endif

  # WUG (WAVE User's Guide)
  if fs.is_file('wug-src/wug.adoc')
    custom_target('wug',
      input: 'wug-src/wug.adoc',
      output: 'wug.html',
      command: [asciidoctor, '-b', 'html5', '-D', '@OUTDIR@',
                '-o', '@OUTPUT@', '@INPUT@'],
      install: true,
      install_dir: docdir / 'wug',
    )
  endif

  # Top-level index
  if fs.is_file('index.adoc')
    custom_target('doc-index',
      input: 'index.adoc',
      output: 'index.html',
      command: [asciidoctor, '-b', 'html5', '-D', '@OUTDIR@',
                '-o', '@OUTPUT@', '@INPUT@'],
      install: true,
      install_dir: docdir,
    )
  endif
endif
