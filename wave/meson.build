gtk3_dep = dependency('gtk+-3.0', required: get_option('wave'))
vte_dep = dependency('vte-2.91', required: get_option('wave'))

if gtk3_dep.found() and vte_dep.found()
    wave_sources = files(
        'wave.c',
        'gtkwave.c',
        'init.c',
        'sig.c',
        'grid.c',
        'annot.c',
        'edit.c',
        'mainpan.c',
        'modepan.c',
        'annpan.c',
        'scope.c',
        'logpan.c',
        'helppan.c',
        'analyze.c',
        'search.c',
    )
    executable('wave', wave_sources,
        dependencies: [wfdb_dep, gtk3_dep, vte_dep, m_dep],
        install: true)
endif
