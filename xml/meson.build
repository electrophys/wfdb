# XML annotation/header converters
# annxml and heaxml need only wfdb; xmlann and xmlhea also need expat.

# Tools that don't need expat
foreach app : ['annxml', 'heaxml']
  executable(app, app + '.c',
    dependencies: wfdb_dep,
    install: true,
  )
endforeach

# Tools that need expat (xmlproc.h #includes <expat.h>)
if libexpat_dep.found()
  foreach app : ['xmlann', 'xmlhea']
    executable(app, app + '.c',
      dependencies: [wfdb_dep, libexpat_dep],
      include_directories: include_directories('.'),
      install: true,
    )
  endforeach
endif

# Data files used by the XML tools
install_data('wfdb.dtd', 'wfdb.xsl',
  install_dir: get_option('datadir') / 'wfdb',
)
