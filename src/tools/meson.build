
src_directory = include_directories('..')
include_dirs = [src_directory, include_directory]

executable('zimdump', 'zimDump.cpp',
           link_with: libzim,
           dependencies : deps,
           include_directories: include_dirs,
           install: true)

executable('zimsearch', 'zimSearch.cpp',
           link_with: libzim,
           dependencies : deps,
           include_directories: include_dirs,
           install: true)

cxxtools_dep = dependency('cxxtools', required:false)

if cxxtools_dep.found()
    executable('zimbench', 'zimBench.cpp',
           link_with: libzim,
           dependencies : deps + [cxxtools_dep],
           include_directories: include_dirs,
           install: true)
endif
